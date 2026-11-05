// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllAgentComponent.h"
#include "TryllSubsystem.h"
#include "TryllAgent.h"
#include "TryllAgentVariables.h"
#include "TryllError.h"
#include "TryllSpeakerComponent.h"
#include "TryllWorkflowAsset.h"
#include "TryllVariableOverrideUtils.h"
#include "TryllClientModule.h"
#include "TryllVoiceProfile.h"

#include "Engine/GameInstance.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UTryllAgentComponent::UTryllAgentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTryllAgentComponent::BeginPlay()
{
	Super::BeginPlay();

	const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	TryllSubsystem = GI ? GI->GetSubsystem<UTryllSubsystem>() : nullptr;

	if (!TryllSubsystem)
	{
		UE_LOG(LogTryll, Warning, TEXT("UTryllAgentComponent: UTryllSubsystem not found"));
		return;
	}

	TryllSubsystem->OnConnectionChanged.AddDynamic(this, &UTryllAgentComponent::HandleConnectionChanged);
	TryllSubsystem->OnCreateSessionComplete.AddDynamic(this, &UTryllAgentComponent::HandleCreateSessionComplete);

	// Create when the session is ready. If it's already configured (e.g. this
	// component was added after the session came up), CreateAgent() runs now;
	// otherwise it queues itself and fires on OnCreateSessionComplete.
	if (bAutoCreateOnConnect)
	{
		CreateAgent();
	}
}

void UTryllAgentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TryllSubsystem)
	{
		TryllSubsystem->OnConnectionChanged.RemoveDynamic(this, &UTryllAgentComponent::HandleConnectionChanged);
		TryllSubsystem->OnCreateSessionComplete.RemoveDynamic(this, &UTryllAgentComponent::HandleCreateSessionComplete);
	}

	DestroyAgent();

	Super::EndPlay(EndPlayReason);
}

void UTryllAgentComponent::HandleConnectionChanged(bool bConnected)
{
	if (!bConnected)
	{
		// Connection lost — local agent handle is stale; reset state.
		if (Speaker)
		{
			Speaker->HandleTurnComplete();
		}
		Agent.Reset();
		bAgentReady    = false;
		AgentIdNumeric = 0;
		bCreateQueued  = false;
		InvalidateCreateState();
		AccumulatedText.Empty();
		ClearSendQueue();
	}
	// Creation is driven by HandleCreateSessionComplete, not raw connection:
	// the session must be configured before CreateAgent can succeed.
}

void UTryllAgentComponent::HandleCreateSessionComplete(bool bSuccess)
{
	if (bSuccess && !bAgentReady && (bCreateQueued || bAutoCreateOnConnect))
	{
		CreateAgent();
	}
}

void UTryllAgentComponent::CreateAgent()
{
	if (!TryllSubsystem)
	{
		UE_LOG(LogTryll, Warning, TEXT("UTryllAgentComponent: CreateAgent called but no subsystem"));
		return;
	}
	if (bAgentReady)
	{
		UE_LOG(LogTryll, Verbose, TEXT("UTryllAgentComponent: CreateAgent called but agent already ready"));
		return;
	}
	// BeginPlay and a (join-late replayed) OnCreateSessionComplete can both call this
	// before the first response arrives; a second request would orphan a server agent
	// and fire OnAgentReady twice.
	if (bCreateInFlight)
	{
		UE_LOG(LogTryll, Verbose, TEXT("UTryllAgentComponent: CreateAgent called but a request is already in flight"));
		return;
	}
	if (!TryllSubsystem->IsSessionReady())
	{
		// Session not configured yet — postpone until OnCreateSessionComplete.
		bCreateQueued = true;
		UE_LOG(LogTryll, Verbose, TEXT("UTryllAgentComponent: session not ready; queueing CreateAgent"));
		return;
	}
	bCreateQueued = false;

	if (!WorkflowAsset)
	{
		UE_LOG(LogTryll, Error, TEXT("UTryllAgentComponent: CreateAgent failed: WorkflowAsset is not assigned"));
		OnError.Broadcast(TEXT("WorkflowAsset is not assigned"));
		return;
	}

	FTryllGraphDescription ResolvedGraph = WorkflowAsset->Graph;
	ResolvedGraph.TextTable = WorkflowAsset->TextTable;
	ResolvedGraph.LocalizedAssetName = WorkflowAsset->GetName();
	const TArray<FTryllVariableDecl>& DeclaredVariables = WorkflowAsset->Variables;
	const TArray<FTryllVariableDecl> MergedVariables =
		TryllVariableOverrideUtils::MergeVariableOverrides(DeclaredVariables, VariableOverrides);

	ResolveSpeaker(ResolvedGraph);

	const TOptional<FTryllVoiceProfileData> Voice = ResolveCreateVoice(ResolvedGraph);

	UE_LOG(LogTryll, Log, TEXT("UTryllAgentComponent: requesting CreateAgent (%d node(s))"),
		ResolvedGraph.Nodes.Num());

	bCreateInFlight = true;
	const uint32 Generation = ++CreateGeneration;

	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	TWeakObjectPtr<UTryllSubsystem> WeakSubsystem(TryllSubsystem);
	TryllSubsystem->RequestCreateAgent(
		ResolvedGraph,
		[WeakThis, WeakSubsystem, Generation](TSharedPtr<FTryllAgent> CreatedAgent, FTryllError Err)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->HandleCreateAgentResponse(Generation, CreatedAgent, Err);
			}
			else if (CreatedAgent.IsValid())
			{
				// The component is gone; nothing will ever destroy this agent otherwise.
				if (UTryllSubsystem* Subsystem = WeakSubsystem.Get())
				{
					Subsystem->RequestDestroyAgent(CreatedAgent);
				}
			}
		},
		bEnableDiagnostics,
		bMaintainDialogueHistory,
		MergedVariables,
		KvCacheInitialization,
		Workload,
		Voice);
}

TOptional<FTryllVoiceProfileData> UTryllAgentComponent::ResolveCreateVoice(const FTryllGraphDescription& Graph) const
{
	const TOptional<FTryllVoiceProfileData> Voice = GetEffectiveVoiceProfile();
	if (Voice.IsSet())
	{
		return Voice;
	}
	if (Graph.UsesTts())
	{
		// The server rejects this with AgentVoiceRequired (3025), which reaches OnError
		// as usual; say here what to do about it.
		UE_LOG(LogTryll, Error,
			TEXT("UTryllAgentComponent on %s: workflow '%s' speaks but no Voice Profile is assigned. ")
			TEXT("Assign one (Content Browser > Tryll > Voice Profile) on the agent component."),
			*GetNameSafe(GetOwner()), WorkflowAsset ? *WorkflowAsset->GetName() : TEXT("(none)"));
	}
	return {};
}

TOptional<FTryllVoiceProfileData> UTryllAgentComponent::GetEffectiveVoiceProfile() const
{
	if (!VoiceProfile)
	{
		return {};
	}
	FTryllVoiceProfileData Voice = VoiceProfile->Voice;
	if (bOverrideVoiceDelivery)
	{
		Voice.DefaultDeliveryMode = VoiceDelivery.Equals(FTryllVoiceProfileData::NeutralMode)
			? FString()
			: VoiceDelivery;
	}
	return Voice;
}

void UTryllAgentComponent::HandleCreateAgentResponse(
	uint32 Generation, TSharedPtr<FTryllAgent> CreatedAgent, const FTryllError& Err)
{
	if (Generation != CreateGeneration)
	{
		// Orphaned by DestroyAgent / EndPlay / disconnect while pending.
		if (CreatedAgent.IsValid() && TryllSubsystem)
		{
			UE_LOG(LogTryll, Verbose, TEXT("UTryllAgentComponent: destroying stale created agent (id=%llu)"),
				(unsigned long long)CreatedAgent->GetAgentId());
			TryllSubsystem->RequestDestroyAgent(CreatedAgent);
		}
		return;
	}
	bCreateInFlight = false;

	if (!Err.IsOk() || !CreatedAgent.IsValid())
	{
		UE_LOG(LogTryll, Warning, TEXT("UTryllAgentComponent: CreateAgent failed: %s"), *Err.Message);
		OnError.Broadcast(Err.Message);
		return;
	}

	Agent          = CreatedAgent;
	AgentIdNumeric = CreatedAgent->GetAgentId();
	bAgentReady    = true;
	AccumulatedText.Empty();
	if (AActor* Owner = GetOwner())
	{
		CreatedAgent->SetDisplayName(Owner->GetActorNameOrLabel());
	}
	UE_LOG(LogTryll, Log, TEXT("UTryllAgentComponent: agent created (id=%llu)"),
		(unsigned long long)AgentIdNumeric);

	BindAgentCallbacks();
	for (const TPair<FString, FTryllToolHandler>& Pair : ToolHandlers)
	{
		CreatedAgent->RegisterTool(Pair.Key, Pair.Value);
	}
	OnAgentReady.Broadcast();
}

void UTryllAgentComponent::InvalidateCreateState()
{
	++CreateGeneration;
	bCreateInFlight = false;
}

void UTryllAgentComponent::ResolveSpeaker(const FTryllGraphDescription& Graph)
{
	AActor* Owner = GetOwner();
	if (Speaker || !Owner)
	{
		return;
	}

	// Auto-discover a sibling speaker on the same actor when none was assigned
	// explicitly. Unreal's property editor doesn't natively list sibling
	// components in a dropdown (the picker for TObjectPtr<UActorComponent> is
	// an asset picker), so requiring an explicit pick would be hostile UX.
	Speaker = Owner->FindComponentByClass<UTryllSpeakerComponent>();
	if (Speaker || !Graph.UsesTts())
	{
		return;
	}

	Speaker = NewObject<UTryllSpeakerComponent>(
		Owner, MakeUniqueObjectName(Owner, UTryllSpeakerComponent::StaticClass(), TEXT("TryllSpeaker")));
	Speaker->RegisterComponent();
	UE_LOG(LogTryll, Log, TEXT("UTryllAgentComponent: workflow speaks and %s has no Tryll Speaker; added one"),
		*Owner->GetActorNameOrLabel());
}

void UTryllAgentComponent::DestroyAgent()
{
	// Runs even with no agent yet: a create still in flight must not land afterwards.
	InvalidateCreateState();
	if (!Agent.IsValid() || !TryllSubsystem)
	{
		return;
	}
	bAgentReady    = false;
	AgentIdNumeric = 0;
	AccumulatedText.Empty();
	ClearSendQueue();
	if (Speaker)
	{
		Speaker->HandleTurnComplete();
	}

	TryllSubsystem->RequestDestroyAgent(Agent);
	Agent.Reset();
}

void UTryllAgentComponent::SendMessage(const FString& Message)
{
	// An empty turn is not a message — the server rejects it with MalformedMessage.
	if (Message.IsEmpty())
	{
		return;
	}

	if (!Agent.IsValid() || !bAgentReady)
	{
		// No agent at all is a programming error rather than a busy window; queueing it would hide
		// the mistake, so this still reports instead of deferring.
		UE_LOG(LogTryll, Warning, TEXT("UTryllAgentComponent: SendMessage called but agent not ready"));
		return;
	}

	PendingMessage     = Message;
	bHasPendingMessage = true;
	if (!TryFlushPendingMessage())
	{
		OnMessageQueued.Broadcast(Message);
	}
}

bool UTryllAgentComponent::TryFlushPendingMessage()
{
	if (!bHasPendingMessage || PendingMessage.IsEmpty())
	{
		bHasPendingMessage = false;
		PendingMessage.Empty();
		return false;
	}
	if (!Agent.IsValid() || !bAgentReady) return false;
	if (bTurnInFlight)                    return false;

	const FString Text = PendingMessage;
	PendingMessage.Empty();
	bHasPendingMessage = false;
	InFlightMessage    = Text;
	bTurnInFlight      = true;

	AccumulatedText.Empty();
	Agent->SendMessage(Text);
	return true;
}

void UTryllAgentComponent::ChangeParams(
	const FString&        NodeName,
	UTryllNodeParamsBase* Params)
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		UE_LOG(LogTryll, Warning, TEXT("UTryllAgentComponent: ChangeParams called but agent not ready"));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->ChangeParams(NodeName, Params,
		[WeakThis](const FTryllError& Err)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			if (Err.Code == 0)
				Self->OnParamChanged.Broadcast(true, FString{});
			else
				Self->OnParamChanged.Broadcast(false, Err.Message);
		});
}

void UTryllAgentComponent::SetVoice(UTryllVoiceProfile* NewVoice)
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		// CreateAgent already captured the previous profile. Adopting the new
		// one here would make the component disagree with the agent that is
		// about to come back, and a success callback would hide that.
		if (bCreateInFlight)
		{
			UE_LOG(LogTryll, Warning,
				TEXT("UTryllAgentComponent: SetVoice during CreateAgent; the agent being created keeps the previous voice. Call SetVoice again after OnAgentReady."));
			OnVoiceChanged.Broadcast(FTryllError(
				ETryllErrorCode::AgentBusy,
				TEXT("SetVoice during CreateAgent; the agent being created keeps the previous voice. Call SetVoice again after OnAgentReady.")));
			return;
		}
		// No live agent: the profile is used by the next CreateAgent.
		VoiceProfile = NewVoice;
		OnVoiceChanged.Broadcast(FTryllError{});
		return;
	}
	if (!NewVoice)
	{
		OnVoiceChanged.Broadcast(FTryllError(ETryllErrorCode::InvalidVoiceProfile,
			TEXT("SetVoice needs a voice profile; an agent cannot drop its voice once created.")));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	TWeakObjectPtr<UTryllVoiceProfile> WeakVoice(NewVoice);
	Agent->SetVoice(NewVoice->Voice,
		[WeakThis, WeakVoice](const FTryllError& Err)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			if (Err.IsOk())
			{
				Self->VoiceProfile = WeakVoice.Get();
			}
			Self->OnVoiceChanged.Broadcast(Err);
		});
}

void UTryllAgentComponent::SetDeliveryMode(const FString& Mode, float Intensity)
{
	TSharedPtr<FTryllAgent> LiveAgent;
	if (Agent.IsValid() && bAgentReady)
	{
		LiveAgent = Agent;
	}
#if WITH_EDITOR
	else
	{
		LiveAgent = EditorPreviewAgent.Pin();
	}
#endif
	if (!LiveAgent.IsValid())
	{
		OnDeliveryModeChanged.Broadcast(Mode, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	LiveAgent->SetDeliveryMode(Mode, Intensity,
		[WeakThis, Mode](const FTryllError& Err)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnDeliveryModeChanged.Broadcast(Mode, Err);
			}
		});
}

FString UTryllAgentComponent::GetDeliveryMode() const
{
	if (Agent.IsValid() && bAgentReady)
	{
		return Agent->GetDeliveryMode();
	}
#if WITH_EDITOR
	if (TSharedPtr<FTryllAgent> Preview = EditorPreviewAgent.Pin())
	{
		return Preview->GetDeliveryMode();
	}
#endif
	return FString();
}

UTryllAgentComponent* UTryllAgentComponent::FindForSpeaker(const AActor* Owner, const UTryllSpeakerComponent* InSpeaker)
{
	if (!Owner)
	{
		return nullptr;
	}
	TInlineComponentArray<UTryllAgentComponent*> Agents(Owner);
	UTryllAgentComponent* Unbound = nullptr;
	for (UTryllAgentComponent* Candidate : Agents)
	{
		if (!Candidate)
		{
			continue;
		}
		if (InSpeaker && Candidate->Speaker == InSpeaker)
		{
			return Candidate;
		}
		if (!Candidate->Speaker && !Unbound)
		{
			Unbound = Candidate;
		}
	}
	return Unbound;
}

#if WITH_EDITOR
EDataValidationResult UTryllAgentComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (WorkflowAsset && !VoiceProfile && WorkflowAsset->Graph.UsesTts())
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("%s: workflow '%s' speaks (Speak / Generate And Speak) but no Voice Profile is assigned. ")
			TEXT("The server rejects this agent with AgentVoiceRequired (3025)."),
			*GetPathName(), *WorkflowAsset->GetName())));
		Result = EDataValidationResult::Invalid;
	}
	if (VoiceProfile && bOverrideVoiceDelivery && !VoiceDelivery.IsEmpty()
		&& !VoiceProfile->Voice.HasMode(VoiceDelivery))
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("%s: Voice Delivery override '%s' is not defined by Voice Profile '%s'."),
			*GetPathName(), *VoiceDelivery, *VoiceProfile->GetName())));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

void UTryllAgentComponent::Resume(const FString& ResumeNode)
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		UE_LOG(LogTryll, Warning, TEXT("UTryllAgentComponent: Resume called but agent not ready"));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->Resume(ResumeNode,
		[WeakThis](const FTryllError& Err)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			if (Err.Code == 0)
				Self->OnResumed.Broadcast(true, FString{});
			else
				Self->OnResumed.Broadcast(false, Err.Message);
		});
}

void UTryllAgentComponent::ResumeWithToolResult(
	const FTryllToolResult& ToolResult,
	const FString& ResumeNode)
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		OnResumed.Broadcast(false, TEXT("Agent not ready"));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->ResumeWithToolResult(
		ToolResult,
		ResumeNode,
		[WeakThis](const FTryllError& Error)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnResumed.Broadcast(Error.IsOk(), Error.IsOk() ? FString{} : Error.Message);
			}
		});
}

void UTryllAgentComponent::ResumeWithToolResults(
	const TArray<FTryllToolResult>& ToolResults,
	const FString& ResumeNode)
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		OnResumed.Broadcast(false, TEXT("Agent not ready"));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->ResumeWithToolResults(
		ToolResults,
		ResumeNode,
		[WeakThis](const FTryllError& Error)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnResumed.Broadcast(Error.IsOk(), Error.IsOk() ? FString{} : Error.Message);
			}
		});
}

void UTryllAgentComponent::RegisterTool(
	const FString& ToolName,
	FTryllToolHandler Handler)
{
	if (ToolName.IsEmpty() || !Handler)
	{
		return;
	}
	ToolHandlers.Add(ToolName, Handler);
	if (Agent.IsValid())
	{
		Agent->RegisterTool(ToolName, Handler);
	}
#if WITH_EDITOR
	if (TSharedPtr<FTryllAgent> Preview = EditorPreviewAgent.Pin())
	{
		Preview->RegisterTool(ToolName, Handler);
	}
#endif
}

void UTryllAgentComponent::RegisterTool(
	const FString& ToolName,
	FTryllSimpleToolHandler Handler)
{
	if (!Handler)
	{
		return;
	}
	FTryllToolHandler Wrapped =
		[Handler = MoveTemp(Handler)](const FTryllToolCall& Call) mutable
		{
			return FTryllToolHandlerResult::Success(Handler(Call));
		};
	RegisterTool(ToolName, MoveTemp(Wrapped));
}

bool UTryllAgentComponent::UnregisterTool(const FString& ToolName)
{
	if (Agent.IsValid())
	{
		Agent->UnregisterTool(ToolName);
	}
#if WITH_EDITOR
	if (TSharedPtr<FTryllAgent> Preview = EditorPreviewAgent.Pin())
	{
		Preview->UnregisterTool(ToolName);
	}
#endif
	return ToolHandlers.Remove(ToolName) > 0;
}

const FTryllToolHandler* UTryllAgentComponent::FindToolHandler(const FString& ToolName) const
{
	return ToolName.IsEmpty() ? nullptr : ToolHandlers.Find(ToolName);
}

bool UTryllAgentComponent::HasPausedToolBatch() const
{
	return Agent.IsValid() && Agent->HasPausedToolBatch();
}

FString UTryllAgentComponent::GetPausedNode() const
{
	return Agent.IsValid() ? Agent->GetPausedNode() : FString{};
}

void UTryllAgentComponent::RetryPausedToolBatch()
{
	if (Agent.IsValid())
	{
		Agent->RetryPausedToolBatch();
	}
}

void UTryllAgentComponent::Cancel(ETryllCancelMode Mode)
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		UE_LOG(LogTryll, Warning, TEXT("UTryllAgentComponent: Cancel called but agent not ready"));
		return;
	}
	Agent->Cancel(Mode);
}

void UTryllAgentComponent::PrefillKvCache()
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		OnKvCachePrefillError.Broadcast(TEXT("Agent not ready"));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->PrefillKvCache(
		[WeakThis](const FTryllKvCachePrefillResult& Result, const FTryllError& Error)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			if (Error.IsOk())
				Self->OnKvCachePrefilled.Broadcast(Result);
			else
				Self->OnKvCachePrefillError.Broadcast(Error.Message);

			Self->TryFlushPendingMessage(); // the busy window this opened has closed
		});
}

void UTryllAgentComponent::EvictKvCache()
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		OnKvCacheEvictError.Broadcast(TEXT("Agent not ready"));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->EvictKvCache(
		[WeakThis](const FTryllKvCacheEvictResult& Result, const FTryllError& Error)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			Self->TryFlushPendingMessage(); // the busy window this opened has closed
			if (Error.IsOk())
				Self->OnKvCacheEvicted.Broadcast(Result);
			else
				Self->OnKvCacheEvictError.Broadcast(Error.Message);
		});
}

void UTryllAgentComponent::GetKvCacheStatus()
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		OnKvCacheStatusError.Broadcast(TEXT("Agent not ready"));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->GetKvCacheStatus(
		[WeakThis](const FTryllKvCacheStatus& Result, const FTryllError& Error)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			if (Error.IsOk())
				Self->OnKvCacheStatus.Broadcast(Result);
			else
				Self->OnKvCacheStatusError.Broadcast(Error.Message);
		});
}

void UTryllAgentComponent::AppendInteractions(const TArray<FTryllDialogInteraction>& Interactions)
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		OnAppendInteractionsError.Broadcast(TEXT("Agent not ready"));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->AppendInteractions(Interactions,
		[WeakThis](uint32 Count, const FTryllError& Error)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			if (Error.IsOk())
				Self->OnInteractionsAppended.Broadcast(static_cast<int32>(Count));
			else
				Self->OnAppendInteractionsError.Broadcast(Error.Message);
		});
}

void UTryllAgentComponent::RemoveInteractionsFromEnd(int32 Count)
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		OnRemoveInteractionsError.Broadcast(TEXT("Agent not ready"));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->RemoveInteractionsFromEnd(Count,
		[WeakThis](uint32 RemovedCount, const FTryllError& Error)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			if (Error.IsOk())
				Self->OnInteractionsRemoved.Broadcast(static_cast<int32>(RemovedCount));
			else
				Self->OnRemoveInteractionsError.Broadcast(Error.Message);
		});
}

void UTryllAgentComponent::ExportDialog()
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		OnDialogExported.Broadcast({}, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->ExportDialog(
		[WeakThis](const FTryllDialogSnapshot& Snapshot, const FTryllError& Error)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnDialogExported.Broadcast(Snapshot, Error);
			}
		});
}

void UTryllAgentComponent::ImportDialog(
	const FTryllDialogSnapshot& Snapshot, ETryllDialogCompatibility Compatibility)
{
	if (!Agent.IsValid() || !bAgentReady)
	{
		OnDialogImported.Broadcast({}, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	Agent->ImportDialog(Snapshot, Compatibility,
		[WeakThis](const FTryllDialogImportResult& Result, const FTryllError& Error)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnDialogImported.Broadcast(Result, Error);
			}
		});
}

UTryllNodeParamsBase* UTryllAgentComponent::GetNodeParamsBaseline(const FString& NodeName) const
{
	if (!Agent.IsValid())
		return nullptr;
	return Agent->GetNodeParamsBaseline(NodeName);
}

UTryllAgentVariables* UTryllAgentComponent::GetVariables()
{
	if (!Agent.IsValid())
		return nullptr;
	if (!VariablesObject)
		VariablesObject = NewObject<UTryllAgentVariables>(this);
	// Re-bind each call so the wrapper follows agent re-creation (Destroy +
	// CreateAgent) without going stale.
	VariablesObject->Initialize(TWeakPtr<FTryllAgent>(Agent));
	return VariablesObject;
}

TArray<FTryllVariableDecl> UTryllAgentComponent::MergeVariableOverrides(
	const TArray<FTryllVariableDecl>& Declared, const TArray<FTryllVariableOverride>& Overrides)
{
	return TryllVariableOverrideUtils::MergeVariableOverrides(Declared, Overrides);
}

FTryllAgentVariables* UTryllAgentComponent::GetLiveVariables()
{
	if (Agent.IsValid() && bAgentReady)
	{
		return &Agent->GetVariables();
	}
#if WITH_EDITOR
	if (TSharedPtr<FTryllAgent> Preview = EditorPreviewAgent.Pin())
	{
		if (Preview->IsValid())
		{
			return &Preview->GetVariables();
		}
	}
#endif
	return nullptr;
}

bool UTryllAgentComponent::HasLiveAgent() const
{
	if (Agent.IsValid() && bAgentReady)
	{
		return true;
	}
#if WITH_EDITOR
	if (TSharedPtr<FTryllAgent> Preview = EditorPreviewAgent.Pin())
	{
		return Preview->IsValid();
	}
#endif
	return false;
}

#if WITH_EDITOR
void UTryllAgentComponent::AttachEditorPreviewAgent(const TSharedPtr<FTryllAgent>& Preview)
{
	EditorPreviewAgent = Preview;
	if (Preview.IsValid())
	{
		for (const TPair<FString, FTryllToolHandler>& Pair : ToolHandlers)
		{
			Preview->RegisterTool(Pair.Key, Pair.Value);
		}
	}
	UE_LOG(LogTryll, Log,
		TEXT("[Tryll] VariableOverrides: attached editor preview agent %llu to '%s'"),
		Preview.IsValid() ? Preview->GetAgentId() : 0ull, *GetName());
}

void UTryllAgentComponent::DetachEditorPreviewAgent()
{
	if (EditorPreviewAgent.IsValid())
	{
		UE_LOG(LogTryll, Log,
			TEXT("[Tryll] VariableOverrides: detached editor preview agent from '%s'"), *GetName());
	}
	EditorPreviewAgent.Reset();
}
#endif

void UTryllAgentComponent::ClearSendQueue()
{
	PendingMessage.Empty();
	bHasPendingMessage = false;
	InFlightMessage.Empty();
	bTurnInFlight      = false;
}

void UTryllAgentComponent::HandleInboundError(const FTryllError& Err)
{
	// A send refused because the agent was busy must not be lost. Prefills driven by
	// UTryllKvResidencyPolicy go through FTryllAgent directly, so bTurnInFlight is not set
	// for them. A rejection is ErrorResponse only — no TurnComplete follows — so this
	// re-queues the line and waits for the policy's TryFlushPendingMessage (or the next
	// real TurnComplete) rather than resending immediately.
	if (Err.Code == static_cast<int32>(ETryllErrorCode::AgentBusy) && !InFlightMessage.IsEmpty())
	{
		// Keep a newer queued line if there is one — it is the more recent intent.
		if (!bHasPendingMessage)
		{
			PendingMessage     = InFlightMessage;
			bHasPendingMessage = true;
		}
		InFlightMessage.Empty();
		bTurnInFlight = false;
	}
	else if (Err.Code == static_cast<int32>(ETryllErrorCode::FrameTooLarge) && bTurnInFlight)
	{
		// Local size reject: no TurnComplete follows. Drop the oversized
		// line and release the busy window so a newer queued message can go.
		InFlightMessage.Empty();
		bTurnInFlight = false;
		TryFlushPendingMessage();
	}

	UE_LOG(LogTryll, Warning, TEXT("UTryllAgentComponent: agent error code=%d msg=%s"),
		Err.Code, *Err.Message);
	OnError.Broadcast(Err.Message);
}

void UTryllAgentComponent::BindAgentCallbacks()
{
	if (!Agent.IsValid())
	{
		return;
	}

	// The agent outlives this component while a destroy request is in flight, so every
	// callback it holds must re-check the component rather than capture a raw pointer.
	TWeakObjectPtr<UTryllAgentComponent> WeakThis(this);
	TWeakObjectPtr<UTryllSpeakerComponent> WeakSpeaker = Speaker;

	Agent->SetOnAnswerText(
		[WeakThis](const FString& NodeName, const FString& Text, bool bIsDelta, bool bIsFinal)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			// Accumulate deltas into a running buffer. Note: this single buffer conflates
			// text from multiple nodes in a multi-sender turn — callers that care about
			// per-node text should key off NodeName in OnAnswerText rather than relying on
			// OnAnswerFull's single accumulated string.
			if (bIsDelta)
			{
				Self->AccumulatedText += Text;
			}
			else
			{
				// Non-delta: server sent the full accumulated text so far.
				Self->AccumulatedText = Text;
			}
			Self->OnAnswerText.Broadcast(NodeName, Text, bIsFinal);
		});

	Agent->SetOnTurnComplete(
		[WeakThis, WeakSpeaker](ETryllTurnStatus Status, const FString& DebugInfo, int32 TokensGenerated)
		{
			if (UTryllSpeakerComponent* Sp = WeakSpeaker.Get())
			{
				Sp->HandleTurnComplete();
			}
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			Self->OnAnswerFull.Broadcast(Self->AccumulatedText);
			Self->bTurnInFlight = false;
			Self->InFlightMessage.Empty();
			Self->OnTurnComplete.Broadcast(Status, DebugInfo, TokensGenerated);

			// A line sent during the turn goes out now rather than waiting for anything else.
			Self->TryFlushPendingMessage();
			Self->AccumulatedText.Empty();
		});

	Agent->SetOnError(
		[WeakThis](const FTryllError& Err)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			Self->HandleInboundError(Err);
		});

	if (Speaker)
	{
		Agent->SetOnTtsAudioFormat(
			[WeakSpeaker](const FString& NodeName, uint32 SampleRate, uint16 Channels, uint16 BitsPerSample)
			{
				if (UTryllSpeakerComponent* Sp = WeakSpeaker.Get())
				{
					Sp->HandleFormat(NodeName, SampleRate, Channels, BitsPerSample);
				}
			});
		Agent->SetOnTtsAudio(
			[WeakSpeaker](const FString& NodeName, TArrayView<const int16> Pcm)
			{
				if (UTryllSpeakerComponent* Sp = WeakSpeaker.Get())
				{
					Sp->HandleAudio(NodeName, Pcm);
				}
			});
		Agent->SetOnTtsVisemes(
			[WeakSpeaker](const FString& NodeName, TArrayView<const FTryllVisemeEvent> Events)
			{
				if (UTryllSpeakerComponent* Sp = WeakSpeaker.Get())
				{
					Sp->HandleVisemes(NodeName, Events);
				}
			});
	}

	// Forwarders are always installed. The probe reports real IsBound() so an
	// unbound Blueprint delegate neither suppresses OnNodeEvent nor hides the
	// unresumable-pause warning.
	Agent->SetOnToolCall(
		[WeakThis](const FString& ToolName, const FString& ArgumentsJson)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnToolCall.Broadcast(ToolName, ArgumentsJson);
			}
		});
	Agent->SetOnToolCallWithId(
		[WeakThis](const FString& CallId, const FString& ToolName, const FString& ArgumentsJson)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnToolCallWithId.Broadcast(CallId, ToolName, ArgumentsJson);
			}
		});
	Agent->SetOnToolCallEvent(
		[WeakThis](const FTryllToolCall& Call)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnToolCallDetailed.Broadcast(Call);
			}
		});
	Agent->SetOnIntentClassified(
		[WeakThis](const FTryllIntentClassification& Classification)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnIntentClassified.Broadcast(Classification);
			}
		});
	Agent->SetOnIntentLlmClassified(
		[WeakThis](const FTryllIntentLlmClassification& Classification)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnIntentLlmClassified.Broadcast(Classification);
			}
		});
	Agent->SetOnPaused(
		[WeakThis](const FString& NodeName, const FString& PendingExit)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnPaused.Broadcast(NodeName, PendingExit);
			}
		});
	Agent->SetOnNodeEvent(
		[WeakThis](const FString& NodeName, const FString& EventType, const TArray<FTryllNodeEventKeyValue>& KvPairs)
		{
			if (UTryllAgentComponent* Self = WeakThis.Get())
			{
				Self->OnNodeEvent.Broadcast(NodeName, EventType, KvPairs);
			}
		});
	Agent->SetListenerProbe(
		[WeakThis](ETryllAgentNodeEventKind Kind)
		{
			UTryllAgentComponent* Self = WeakThis.Get();
			if (!Self)
			{
				return false;
			}
			switch (Kind)
			{
			case ETryllAgentNodeEventKind::ToolCall:            return Self->OnToolCall.IsBound();
			case ETryllAgentNodeEventKind::ToolCallWithId:      return Self->OnToolCallWithId.IsBound();
			case ETryllAgentNodeEventKind::ToolCallDetailed:    return Self->OnToolCallDetailed.IsBound();
			case ETryllAgentNodeEventKind::IntentClassified:    return Self->OnIntentClassified.IsBound();
			case ETryllAgentNodeEventKind::IntentLlmClassified: return Self->OnIntentLlmClassified.IsBound();
			case ETryllAgentNodeEventKind::Paused:              return Self->OnPaused.IsBound();
			case ETryllAgentNodeEventKind::NodeEvent:           return Self->OnNodeEvent.IsBound();
			default:                                            return false;
			}
		});
}
