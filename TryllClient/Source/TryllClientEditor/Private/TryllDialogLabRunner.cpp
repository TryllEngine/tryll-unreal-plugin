// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabRunner.h"

#include "TryllDialogLabChecks.h"
#include "TryllDialogLabDialogues.h"
#include "TryllDialogScriptValidation.h"
#include "TryllTurnInspectorModels.h"
#include "TryllEditorSubsystem.h"
#include "TryllNodeFieldReflection.h"
#include "TryllAgent.h"
#include "TryllAgentComponent.h"
#include "TryllDialogLabToolResolution.h"
#include "TryllToolStateReset.h"
#include "TryllDialogInteraction.h"
#include "GameFramework/Actor.h"
#include "UObject/Class.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "Generated/Nodes/TryllSamplingOverrides.h"
#include "Generated/Nodes/TryllToolCallParams.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/** tryll/common/include/tryll/ErrorCodes.h — param_name is not settable on that node type. */
	constexpr int32 ParamNotMutableCode = 3006;

	const TCHAR* SeedFieldPath = TEXT("Sampling.Seed");

	bool HasSamplingField(const UTryllNodeParamsBase* Params)
	{
		if (Params == nullptr)
		{
			return false;
		}
		for (TFieldIterator<FProperty> It(Params->GetClass()); It; ++It)
		{
			const FStructProperty* AsStruct = CastField<FStructProperty>(*It);
			if (AsStruct != nullptr && AsStruct->Struct == FTryllSamplingOverrides::StaticStruct())
			{
				return true;
			}
		}
		return false;
	}

	FString VariantLetter(int32 Index)
	{
		return FString::Chr(static_cast<TCHAR>(TEXT('A') + Index));
	}

	TSharedPtr<FJsonObject> ParseJsonObject(const FString& Text)
	{
		if (Text.IsEmpty())
		{
			return nullptr;
		}
		TSharedPtr<FJsonObject> Root;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		return FJsonSerializer::Deserialize(Reader, Root) ? Root : nullptr;
	}

	/** debug_info.errors[0], used as the cell message for a server-side turn failure. */
	FString FirstError(const TSharedPtr<FJsonObject>& DebugInfo)
	{
		if (!DebugInfo.IsValid())
		{
			return FString();
		}
		const TArray<TSharedPtr<FJsonValue>>* Errors = nullptr;
		if (!DebugInfo->TryGetArrayField(TEXT("errors"), Errors) || Errors->Num() == 0)
		{
			return FString();
		}
		FString Out;
		(*Errors)[0]->TryGetString(Out);
		return Out;
	}

	bool BakeOverrides(
		FTryllGraphDescription& Graph, const FTryllLabVariantSnapshot& Variant, FString& OutError)
	{
		OutError.Reset();

		// The per-variant model is a CreateAgentRequest field (a sibling of graph) rather than a
		// node param, so baking it here changes only what this run's CreateAgent asks for.
		if (!Variant.ModelOverride.IsEmpty())
		{
			Graph.DefaultModelName.Name = Variant.ModelOverride;
		}
		if (Variant.Overrides.Num() == 0)
		{
			return true;
		}

		TMap<FString, UTryllNodeParamsBase*> ByName;
		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (Node.Params != nullptr)
			{
				ByName.Add(Node.Name, Node.Params);
			}
		}

		// Grouped by node so a whole node's overrides apply together, as Unity does.
		TMap<FString, TArray<FTryllLabParamOverride>> Grouped;
		for (const FTryllLabParamOverride& Override : Variant.Overrides)
		{
			Grouped.FindOrAdd(Override.NodeName).Add(Override);
		}

		for (const TPair<FString, TArray<FTryllLabParamOverride>>& Group : Grouped)
		{
			UTryllNodeParamsBase** Target = ByName.Find(Group.Key);
			if (Target == nullptr)
			{
				OutError = FString::Printf(
					TEXT("override targets node '%s', which is not in %s's workflow"),
					*Group.Key, *Variant.AgentName);
				return false;
			}
			FString ApplyError;
			if (!FTryllNodeFieldReflection::TryApply(*Target, Group.Value, ApplyError))
			{
				OutError = FString::Printf(TEXT("node '%s': %s"), *Group.Key, *ApplyError);
				return false;
			}
		}
		return true;
	}

	bool BakeSeed(FTryllGraphDescription& Graph, int32 Seed, FString& OutError)
	{
		OutError.Reset();
		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (!HasSamplingField(Node.Params))
			{
				continue;
			}
			FTryllLabParamOverride SeedOverride;
			SeedOverride.NodeName  = Node.Name;
			SeedOverride.FieldPath = SeedFieldPath;
			SeedOverride.Value     = FString::FromInt(Seed);

			FString ApplyError;
			if (!FTryllNodeFieldReflection::TryApply(Node.Params, {SeedOverride}, ApplyError))
			{
				OutError = FString::Printf(TEXT("node '%s': %s"), *Node.Name, *ApplyError);
				return false;
			}
		}
		return true;
	}
}

FTryllDialogLabRunner::~FTryllDialogLabRunner()
{
	// Unhook without touching the agent: a destructing runner has no one left to notify.
	if (UTryllEditorSubsystem* Sub = Subsystem.Get())
	{
		if (PausedHandle.IsValid())     { Sub->OnPausedNative.Remove(PausedHandle); }
		if (ConnectionHandle.IsValid()) { Sub->OnConnectionChangedNative.Remove(ConnectionHandle); }
	}
}

void FTryllDialogLabRunner::Start(
	UTryllEditorSubsystem* InSubsystem,
	const FTryllDialogLabRunSnapshot& InSnapshot,
	FOnProgress InOnProgress,
	FOnFinished InOnFinished,
	const TArray<TWeakObjectPtr<UTryllAgentComponent>>& InLiveToolAgents,
	FOnDialogueCompleted InOnDialogueCompleted)
{
	if (bRunning)
	{
		FTryllDialogLabRunResult Busy;
		Busy.Snapshot = InSnapshot;
		Busy.Status   = ETryllDialogLabRunStatus::Failed;
		Busy.Issues.Add(TEXT("A Dialog Lab run is already in progress."));
		Busy.StartedAtUtc   = FDateTime::UtcNow();
		Busy.CompletedAtUtc = Busy.StartedAtUtc;
		InOnFinished.ExecuteIfBound(Busy);
		return;
	}

	Subsystem      = InSubsystem;
	Snapshot       = InSnapshot;
	LiveToolAgents = InLiveToolAgents;
	OnProgress     = InOnProgress;
	OnFinished     = InOnFinished;
	OnDialogueCompleted = InOnDialogueCompleted;
	bRunning       = true;
	bStopRequested = false;
	bTimedOut      = false;
	++Generation;

	Result = FTryllDialogLabRunResult();
	Result.Snapshot     = Snapshot;
	Result.StartedAtUtc = FDateTime::UtcNow();
	Result.Status       = ETryllDialogLabRunStatus::Completed;

	if (UTryllEditorSubsystem* Sub = Subsystem.Get())
	{
		Result.Server.ExePath     = Sub->ServerExePath;
		Result.Server.BuildConfig = Sub->ServerBuildConfig;
		Result.Server.Version     = Sub->ServerVersion;
		Result.Server.RunId       = Sub->ServerRunId;
		Result.Server.bManaged    = Sub->bServerManaged;
	}

	DialogueIndex = 0;
	VariantIndex = 0;
	RepeatIndex  = 0;
	DoneUnits    = 0;
	TotalUnits   = Snapshot.PlannedGeneratedTurns();

	// One sample per variant x seed column, created up front with every dialogue's turns as
	// "not run": each dialogue fills its own slice of each column (StoreSample).
	for (int32 V = 0; V < Snapshot.Variants.Num(); ++V)
	{
		for (int32 R = 0; R < FMath::Max(1, Snapshot.Repeats); ++R)
		{
			FTryllLabSampleResult Column = MakeSample(V, R, Snapshot.DeriveSeed(V, R));
			for (DialogueIndex = 0; DialogueIndex < Snapshot.Dialogues.Num(); ++DialogueIndex)
			{
				FTryllLabSampleResult Slice;
				MakeRectangularTurns(Slice, TEXT("Not run"));
				Column.Turns.Append(Slice.Turns);
			}
			Result.Samples.Add(MoveTemp(Column));
		}
	}
	DialogueIndex = 0;

	const uint64 Gen = Generation;
	TWeakPtr<FTryllDialogLabRunner> SelfWeak = AsShared();

	if (UTryllEditorSubsystem* Sub = Subsystem.Get())
	{
		// A pause on a node that is not a ToolCall is resumed immediately; only ToolCall
		// pauses consult the script's canned results.
		PausedHandle = Sub->OnPausedNative.AddLambda(
			[SelfWeak, Gen](uint64 AgentId, const FString& NodeName, const FString&)
			{
				TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
				if (!Self.IsValid() || Self->IsStale(Gen) || !Self->Agent.IsValid())
				{
					return;
				}
				if (Self->Agent->GetAgentId() != AgentId)
				{
					return;
				}
				const FTryllNodeDescription* Node = Self->WorkingGraph.Nodes.FindByPredicate(
					[&NodeName](const FTryllNodeDescription& N) { return N.Name == NodeName; });
				const bool bToolCallPause = Node != nullptr
					&& Node->Params != nullptr
					&& Node->Params->IsA<UTryllToolCallParams>();
				if (!bToolCallPause)
				{
					Self->Agent->Resume();
				}
			});

		// A dead connection cannot produce TurnComplete, so settle locally instead of waiting.
		ConnectionHandle = Sub->OnConnectionChangedNative.AddLambda(
			[SelfWeak, Gen](bool bConnected)
			{
				TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
				if (!Self.IsValid() || Self->IsStale(Gen) || bConnected)
				{
					return;
				}
				Self->SettleTurn(ETryllTurnStatus::Error, FString(), 0, TEXT("Disconnected"));
			});
	}

	if (Snapshot.Dialogues.Num() == 0)
	{
		Result.Issues.Add(TEXT("No dialogue to run."));
		Result.Status = ETryllDialogLabRunStatus::Failed;
		Finish();
		return;
	}
	BeginVariant();
}

void FTryllDialogLabRunner::Stop()
{
	if (!bRunning)
	{
		return;
	}
	bStopRequested = true;
	if (Agent.IsValid() && Agent->IsValid())
	{
		Agent->Cancel(ETryllCancelMode::StopAndDiscard);
	}
	SettleTurn(ETryllTurnStatus::Cancelled, FString(), 0, TEXT("Cancelled"));
}

const FTryllLabVariantSnapshot& FTryllDialogLabRunner::CurrentVariant() const
{
	return Snapshot.Variants[VariantIndex];
}

const FTryllLabScriptSnapshot& FTryllDialogLabRunner::CurrentScript() const
{
	return Snapshot.Dialogues[DialogueIndex];
}

const FTryllLabStepSnapshot& FTryllDialogLabRunner::CurrentStep() const
{
	return CurrentScript().Steps[TurnIndex];
}

FString FTryllDialogLabRunner::Who() const
{
	const FString Variant = FString::Printf(TEXT("Variant '%s'"), *CurrentVariant().Label);
	return Snapshot.Dialogues.Num() > 1
		? FString::Printf(TEXT("Dialogue %d '%s': %s"), DialogueIndex + 1, *CurrentScript().Name, *Variant)
		: Variant;
}

void FTryllDialogLabRunner::StoreSample(const FTryllLabSampleResult& Sample)
{
	const int32 Column = Sample.VariantIndex * FMath::Max(1, Snapshot.Repeats) + Sample.Repeat;
	if (!Result.Samples.IsValidIndex(Column))
	{
		return;
	}
	const int32 Offset = Snapshot.TurnOffset(DialogueIndex);
	TArray<FTryllLabTurnRecord>& Turns = Result.Samples[Column].Turns;
	for (int32 Index = 0; Index < Sample.Turns.Num() && Offset + Index < Turns.Num(); ++Index)
	{
		Turns[Offset + Index] = Sample.Turns[Index];
	}
}

void FTryllDialogLabRunner::BeginVariant()
{
	if (bStopRequested)
	{
		Finish();
		return;
	}
	if (VariantIndex >= Snapshot.Variants.Num())
	{
		AdvanceDialogue();
		return;
	}

	if (VariantIndex == 0)
	{
		// Script-level, so once per dialogue rather than once per variant.
		const FString Prefix = FTryllDialogLabDialogues::Prefix(DialogueIndex, CurrentScript().Name, Snapshot.Dialogues.Num());
		for (const FString& Warn : FTryllDialogScriptValidation::CollectSeededGlobalCheckWarnings(CurrentScript()))
		{
			Result.Issues.Add(Prefix + Warn);
		}
	}

	const FTryllLabVariantSnapshot& Variant = CurrentVariant();
	RepeatIndex = 0;

	if (Variant.Graph.Nodes.Num() == 0)
	{
		FailCurrentVariant(TEXT("No workflow graph"), /*bMarkRunFailed*/ true);
		return;
	}

	// Defence in depth: the panel refuses this before Run, but an imported or synthesised
	// snapshot could still carry it.
	FString StatelessError;
	if (!FTryllDialogScriptValidation::TryValidateForAgent(
			CurrentScript().SeedTurnCount, Variant.bMaintainDialogueHistory,
			Variant.AgentName, StatelessError))
	{
		FailCurrentVariant(StatelessError, /*bMarkRunFailed*/ true);
		return;
	}

	GraphKeepAlive.Reset();
	BaseGraph = FTryllDialogLabRunSnapshot::CloneGraph(Variant.Graph, GraphKeepAlive);

	FString BakeError;
	if (!BakeOverrides(BaseGraph, Variant, BakeError))
	{
		FailCurrentVariant(BakeError, /*bMarkRunFailed*/ false);
		return;
	}

	SamplingNodeNames.Reset();
	for (const FTryllNodeDescription& Node : BaseGraph.Nodes)
	{
		if (HasSamplingField(Node.Params))
		{
			SamplingNodeNames.Add(Node.Name);
		}
	}

	WorkingGraph = FTryllDialogLabRunSnapshot::CloneGraph(BaseGraph, GraphKeepAlive);
	if (!BakeSeed(WorkingGraph, Snapshot.DeriveSeed(VariantIndex, 0), BakeError))
	{
		FailCurrentVariant(BakeError, /*bMarkRunFailed*/ false);
		return;
	}

	for (const FString& Warn : FTryllDialogScriptValidation::CollectIntentCheckWarnings(
		CurrentScript(), &BaseGraph))
	{
		Result.Issues.Add(FString::Printf(TEXT("%s: %s"), *Who(), *Warn));
	}

	CreateAgentForVariant(/*bIsSeedRecreate*/ false);
}

void FTryllDialogLabRunner::CreateAgentForVariant(bool bIsSeedRecreate)
{
	UTryllEditorSubsystem* Sub = Subsystem.Get();
	if (Sub == nullptr)
	{
		FailCurrentVariant(TEXT("Editor subsystem unavailable"), /*bMarkRunFailed*/ true);
		return;
	}

	const uint64 Gen = Generation;
	TWeakPtr<FTryllDialogLabRunner> SelfWeak = AsShared();
	Sub->RequestCreateAgent(
		WorkingGraph,
		[SelfWeak, Gen, bIsSeedRecreate](TSharedPtr<FTryllAgent> NewAgent, FTryllError Error)
		{
			TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
			if (!Self.IsValid() || Self->IsStale(Gen))
			{
				return;
			}
			Self->HandleAgentCreated(NewAgent, Error, bIsSeedRecreate);
		},
		/*bEnableDiagnostics*/ true,
		CurrentVariant().bMaintainDialogueHistory,
		CurrentVariant().Variables,
		ETryllKvCacheInitialization::AllocateOnly,
		ETryllWorkload::Interactive,
		CurrentVariant().Voice);
}

void FTryllDialogLabRunner::HandleAgentCreated(
	TSharedPtr<FTryllAgent> NewAgent, FTryllError Error, bool bIsSeedRecreate)
{
	if (bStopRequested)
	{
		Agent = NewAgent;
		DestroyCurrentAgent();
		Finish();
		return;
	}

	if (!Error.IsOk() || !NewAgent.IsValid())
	{
		const FString Message = FString::Printf(
			TEXT("%s failed — %s"),
			bIsSeedRecreate ? TEXT("RecreateAgent") : TEXT("CreateAgent"),
			*Error.Message);
		if (bIsSeedRecreate)
		{
			AbortRepeatsWithSeedFailure(Message);
		}
		else
		{
			FailCurrentVariant(Message, /*bMarkRunFailed*/ false);
		}
		return;
	}

	Agent = NewAgent;
	HookAgentCallbacks();
	RegisterCannedToolHandlers();

	if (bIsSeedRecreate)
	{
		AppendSeedHistory();
	}
	else
	{
		BeginRepeat();
	}
}

void FTryllDialogLabRunner::HookAgentCallbacks()
{
	const uint64 Gen = Generation;
	TWeakPtr<FTryllDialogLabRunner> SelfWeak = AsShared();

	Agent->SetOnAnswerText(
		[SelfWeak, Gen](const FString&, const FString& Text, bool, bool)
		{
			TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
			if (Self.IsValid() && !Self->IsStale(Gen))
			{
				Self->AnswerBuffer += Text;
			}
		});

	Agent->SetOnTurnComplete(
		[SelfWeak, Gen](ETryllTurnStatus Status, const FString& DebugInfo, int32 Tokens)
		{
			TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
			if (Self.IsValid() && !Self->IsStale(Gen))
			{
				Self->SettleTurn(Status, DebugInfo, Tokens);
			}
		});

	Agent->SetOnError(
		[SelfWeak, Gen](const FTryllError& Error)
		{
			TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
			if (!Self.IsValid() || Self->IsStale(Gen))
			{
				return;
			}
			if (Self->Agent.IsValid() && Self->Agent->HasPausedToolBatch())
			{
				Self->Agent->Cancel(ETryllCancelMode::StopAndDiscard);
			}
			Self->SettleTurn(ETryllTurnStatus::Error, FString(), 0,
				Error.Message.IsEmpty() ? TEXT("Agent error") : Error.Message);
		});

	Agent->SetOnToolHandlerError(
		[SelfWeak, Gen](const FTryllError& Error)
		{
			TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
			if (!Self.IsValid() || Self->IsStale(Gen))
			{
				return;
			}
			const FString Message = Error.Message.IsEmpty()
				? FString(TEXT("Tool handler failed")) : Error.Message;
			Self->TurnFailure = Message;
			if (Self->Agent.IsValid() && Self->Agent->IsValid())
			{
				Self->Agent->Cancel(ETryllCancelMode::StopAndDiscard);
			}
			else
			{
				Self->SettleTurn(ETryllTurnStatus::Error, FString(), 0, Message);
			}
		});
}

void FTryllDialogLabRunner::RegisterCannedToolHandlers()
{
	// The union of every tool named by the script and every tool declared on a ToolCall node.
	// A handler invoked for a step with no canned entry resumes with an empty result so the
	// turn can complete; EvaluateStep then CheckFails the cell. That is what makes
	// "the model must not call this tool here" testable. The one exception is a variant that
	// opted into live tools *and* whose scene component carries a handler: it answers instead.
	// See ResolveDialogLabToolResult.
	TSet<FString> ToolNames;
	for (const FTryllLabStepSnapshot& Step : CurrentScript().Steps)
	{
		for (const FTryllCannedToolResult& Canned : Step.ToolResults)
		{
			if (!Canned.ToolName.IsEmpty())
			{
				ToolNames.Add(Canned.ToolName);
			}
		}
	}
	for (const FTryllNodeDescription& Node : WorkingGraph.Nodes)
	{
		const UTryllToolCallParams* ToolParams = Cast<UTryllToolCallParams>(Node.Params);
		if (ToolParams == nullptr)
		{
			continue;
		}
		for (const FTryllToolDefinition& Tool : ToolParams->Tools)
		{
			if (!Tool.Name.IsEmpty())
			{
				ToolNames.Add(Tool.Name);
			}
		}
	}

	const uint64 Gen = Generation;
	TWeakPtr<FTryllDialogLabRunner> SelfWeak = AsShared();
	for (const FString& ToolName : ToolNames)
	{
		Agent->RegisterTool(ToolName, FTryllToolHandler(
			[SelfWeak, Gen, ToolName](const FTryllToolCall& Call) -> FTryllToolHandlerResult
			{
				TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
				if (!Self.IsValid() || Self->IsStale(Gen))
				{
					return FTryllToolHandlerResult::Failure(TEXT("run is no longer active"));
				}
				const FTryllLabStepSnapshot& Step = Self->CurrentStep();
				const FTryllCannedToolResult* Canned = Step.ToolResults.FindByPredicate(
					[&ToolName](const FTryllCannedToolResult& R) { return R.ToolName == ToolName; });
				const bool bHasCanned = Canned != nullptr;
				const FTryllToolHandler* LivePtr = nullptr;
				if (const UTryllAgentComponent* Component = Self->CurrentLiveToolAgent())
				{
					LivePtr = Component->FindToolHandler(ToolName);
				}
				const ETryllDialogLabToolSource Source = ResolveDialogLabToolResult(
					bHasCanned, Self->CurrentVariant().bUseLiveTools, LivePtr != nullptr);
				if (Source == ETryllDialogLabToolSource::Canned)
				{
					return FTryllToolHandlerResult::Success(Canned->Result);
				}
				if (Source == ETryllDialogLabToolSource::Live)
				{
					// Copy before invoke: the map pointer is invalidated by Register/Unregister.
					FTryllToolHandler Live = *LivePtr;
					return Live(Call);
				}
				// Missing is a check, not a handler error: resume with an empty
				// result so the turn completes and EvaluateStep can mark CheckFail.
				Self->MissingResumedTools.AddUnique(ToolName);
				return FTryllToolHandlerResult::Success(FString());
			}));
	}
}

UTryllAgentComponent* FTryllDialogLabRunner::CurrentLiveToolAgent() const
{
	return LiveToolAgents.IsValidIndex(VariantIndex) ? LiveToolAgents[VariantIndex].Get() : nullptr;
}

void FTryllDialogLabRunner::ResetLiveToolState()
{
	if (!Snapshot.Variants.IsValidIndex(VariantIndex) || !CurrentVariant().bUseLiveTools)
	{
		return;
	}

	TryllToolStateReset::ResetOnOwner(CurrentLiveToolAgent());
}

void FTryllDialogLabRunner::BeginRepeat()
{
	if (bStopRequested)
	{
		Finish();
		return;
	}

	const int32 Seed = Snapshot.DeriveSeed(VariantIndex, RepeatIndex);
	CurrentSample = MakeSample(VariantIndex, RepeatIndex, Seed);
	MakeRectangularTurns(CurrentSample, FString());
	bColumnFailed = false;

	// Exactly once per repeat, and before any turn of it runs. BeginRepeat is the only
	// place that is true: the seed-recreate path re-enters AppendSeedHistory, so resetting
	// there would fire twice for one repeat.
	ResetLiveToolState();

	if (RepeatIndex > 0)
	{
		// Prefer ChangeParams so the KV cache stays warm; a seed change touches no context.
		ChangeSeedForNode(0);
		return;
	}

	AppendSeedHistory();
}

void FTryllDialogLabRunner::ChangeSeedForNode(int32 NodeCursor)
{
	if (bStopRequested)
	{
		Finish();
		return;
	}
	if (NodeCursor >= SamplingNodeNames.Num())
	{
		AppendSeedHistory();
		return;
	}
	if (!Agent.IsValid())
	{
		AbortRepeatsWithSeedFailure(TEXT("agent is no longer valid"));
		return;
	}

	const FString& NodeName = SamplingNodeNames[NodeCursor];
	UTryllNodeParamsBase* Baseline = Agent->GetNodeParamsBaseline(NodeName);
	if (Baseline == nullptr)
	{
		AbortRepeatsWithSeedFailure(FString::Printf(TEXT("no baseline for '%s'"), *NodeName));
		return;
	}

	FTryllLabParamOverride SeedOverride;
	SeedOverride.NodeName  = NodeName;
	SeedOverride.FieldPath = SeedFieldPath;
	SeedOverride.Value     = FString::FromInt(Snapshot.DeriveSeed(VariantIndex, RepeatIndex));

	FString ApplyError;
	if (!FTryllNodeFieldReflection::TryApply(Baseline, {SeedOverride}, ApplyError))
	{
		AbortRepeatsWithSeedFailure(ApplyError);
		return;
	}

	const uint64 Gen = Generation;
	TWeakPtr<FTryllDialogLabRunner> SelfWeak = AsShared();
	Agent->ChangeParams(NodeName, Baseline,
		[SelfWeak, Gen, NodeCursor](const FTryllError& Error)
		{
			TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
			if (Self.IsValid() && !Self->IsStale(Gen))
			{
				Self->HandleSeedChanged(Error, NodeCursor);
			}
		});
}

void FTryllDialogLabRunner::HandleSeedChanged(const FTryllError& Error, int32 NodeCursor)
{
	if (bStopRequested)
	{
		Finish();
		return;
	}
	if (Error.IsOk())
	{
		ChangeSeedForNode(NodeCursor + 1);
		return;
	}

	// A ToolCall node can reject the round-trip on its structural `tools` field when the wire
	// form is not byte-identical. Recover with a fresh agent carrying the seed baked into the
	// graph — same effective params, cold KV — rather than failing the column.
	if (Error.Code == ParamNotMutableCode)
	{
		DestroyCurrentAgent();

		GraphKeepAlive.Reset();
		BaseGraph = FTryllDialogLabRunSnapshot::CloneGraph(BaseGraph, GraphKeepAlive);
		WorkingGraph = FTryllDialogLabRunSnapshot::CloneGraph(BaseGraph, GraphKeepAlive);

		FString BakeError;
		if (!BakeSeed(WorkingGraph, Snapshot.DeriveSeed(VariantIndex, RepeatIndex), BakeError))
		{
			AbortRepeatsWithSeedFailure(BakeError);
			return;
		}
		CreateAgentForVariant(/*bIsSeedRecreate*/ true);
		return;
	}

	AbortRepeatsWithSeedFailure(Error.Message);
}

void FTryllDialogLabRunner::AppendSeedHistory()
{
	if (bStopRequested)
	{
		Finish();
		return;
	}
	if (CurrentScript().SeedTurnCount <= 0)
	{
		EvaluateSeededCells();
		return;
	}
	if (!Agent.IsValid())
	{
		AbortRepeatsWithSeedFailure(TEXT("agent is no longer valid"));
		return;
	}

	TArray<FTryllDialogInteraction> Interactions;
	for (int32 Index = 0; Index < CurrentScript().SeedTurnCount; ++Index)
	{
		FTryllDialogInteraction Interaction;
		Interaction.UserMessage      = CurrentScript().Steps[Index].User;
		Interaction.AssistantMessage = CurrentScript().Steps[Index].Assistant;
		Interactions.Add(MoveTemp(Interaction));
	}

	const int32  Expected = CurrentScript().SeedTurnCount;
	const uint64 Gen      = Generation;
	TWeakPtr<FTryllDialogLabRunner> SelfWeak = AsShared();
	Agent->AppendInteractions(Interactions,
		[SelfWeak, Gen, Expected](uint32 Count, const FTryllError& Error)
		{
			TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
			if (!Self.IsValid() || Self->IsStale(Gen))
			{
				return;
			}
			if (!Error.IsOk())
			{
				Self->AbortRepeatsWithSeedFailure(FString::Printf(
					TEXT("AppendInteractions failed — %s"), *Error.Message));
				return;
			}
			if (static_cast<int32>(Count) != Expected)
			{
				Self->AbortRepeatsWithSeedFailure(FString::Printf(
					TEXT("AppendInteractions applied %u of %d seeded turns"), Count, Expected));
				return;
			}
			Self->EvaluateSeededCells();
		});
}

void FTryllDialogLabRunner::EvaluateSeededCells()
{
	for (int32 Index = 0; Index < CurrentScript().SeedTurnCount; ++Index)
	{
		const FTryllLabStepSnapshot& Step = CurrentScript().Steps[Index];
		FTryllLabTurnRecord& Record = CurrentSample.Turns[Index];
		Record.bSeeded    = true;
		Record.UserText   = Step.User;
		Record.AnswerText = Step.Assistant;
		Record.Words      = FTryllDialogLabChecks::CountWords(Record.AnswerText);
		Record.Status     = FTryllDialogLabChecks::EvaluateStep(
			CurrentScript(), Step, Record.AnswerText,
			nullptr, ETryllDiagnosticsAbsenceReason::Seeded, Record.CheckOutcomes);
	}

	TurnIndex = CurrentScript().SeedTurnCount;
	RunNextGeneratedStep();
}

void FTryllDialogLabRunner::RunNextGeneratedStep()
{
	if (TurnIndex >= CurrentScript().Steps.Num())
	{
		FinishRepeat();
		return;
	}

	const FTryllLabStepSnapshot& Step = CurrentStep();
	FTryllLabTurnRecord& Record = CurrentSample.Turns[TurnIndex];
	Record.bSeeded  = false;
	Record.UserText = Step.User;

	if (bStopRequested || bColumnFailed)
	{
		Record.Status = ETryllLabCellStatus::Skipped;
		if (bStopRequested && !bColumnFailed)
		{
			Record.ErrorMessage = TEXT("Cancelled");
		}
		++DoneUnits;
		ReportProgress();
		++TurnIndex;
		RunNextGeneratedStep();
		return;
	}

	if (!Agent.IsValid())
	{
		Record.Status       = ETryllLabCellStatus::Error;
		Record.ErrorMessage = TEXT("agent is no longer valid");
		bColumnFailed = true;
		++DoneUnits;
		ReportProgress();
		++TurnIndex;
		RunNextGeneratedStep();
		return;
	}

	if (Step.User.TrimStartAndEnd().IsEmpty())
	{
		Record.Status       = ETryllLabCellStatus::Error;
		Record.ErrorMessage = TEXT("empty user line");
		bColumnFailed = true;
		++DoneUnits;
		ReportProgress();
		++TurnIndex;
		RunNextGeneratedStep();
		return;
	}

	AnswerBuffer.Reset();
	TurnFailure.Reset();
	MissingResumedTools.Reset();
	bTurnSettled     = false;
	TurnStartSeconds = FPlatformTime::Seconds();
	Agent->SendMessage(Step.User);

	if (TurnTimeoutSeconds > 0.0)
	{
		TWeakPtr<FTryllDialogLabRunner> SelfWeak = AsShared();
		const uint64 Gen = Generation;
		const int32  Turn = TurnIndex;
		TurnWatchdog = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[SelfWeak, Gen, Turn](float)
			{
				TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
				if (!Self.IsValid() || Self->IsStale(Gen) || Self->bTurnSettled || Self->TurnIndex != Turn)
				{
					return false;
				}
				Self->TurnWatchdog.Reset();
				Self->bTimedOut      = true;
				Self->bStopRequested = true;
				if (Self->Agent.IsValid() && Self->Agent->IsValid())
				{
					Self->Agent->Cancel(ETryllCancelMode::StopAndDiscard);
				}
				Self->SettleTurn(ETryllTurnStatus::Error, FString(), 0, TimeoutMessage(Self->TurnTimeoutSeconds));
				return false;
			}), static_cast<float>(TurnTimeoutSeconds));
	}
}

FString FTryllDialogLabRunner::TimeoutMessage(const double Seconds)
{
	// Unity's "0.###": at most three decimals, trailing zeros and point dropped.
	FString Number = FString::Printf(TEXT("%.3f"), Seconds);
	while (Number.EndsWith(TEXT("0")))
	{
		Number.LeftChopInline(1);
	}
	if (Number.EndsWith(TEXT(".")))
	{
		Number.LeftChopInline(1);
	}
	return FString::Printf(TEXT("Turn timed out after %s s"), *Number);
}

void FTryllDialogLabRunner::SettleTurn(
	ETryllTurnStatus Status, const FString& DebugInfo, int32 Tokens, const FString& FailureMessage)
{
	if (!FailureMessage.IsEmpty())
	{
		TurnFailure = FailureMessage;
	}
	if (bTurnSettled)
	{
		return;
	}
	// Only settle while a generated turn is actually outstanding; a stray disconnect between
	// turns must not advance the sequence.
	if (TurnIndex >= CurrentScript().Steps.Num() || TurnStartSeconds <= 0.0)
	{
		return;
	}
	bTurnSettled = true;
	FinishTurn(Status, DebugInfo, Tokens);
}

void FTryllDialogLabRunner::FinishTurn(
	ETryllTurnStatus Status, const FString& DebugInfo, int32 Tokens)
{
	if (TurnWatchdog.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TurnWatchdog);
		TurnWatchdog.Reset();
	}
	FTryllLabTurnRecord& Record = CurrentSample.Turns[TurnIndex];
	Record.AnswerText      = AnswerBuffer;
	Record.Words           = FTryllDialogLabChecks::CountWords(Record.AnswerText);
	Record.TokensGenerated = Tokens;
	Record.DebugInfoRaw    = DebugInfo;
	Record.DurationMs      = static_cast<int32>(
		(FPlatformTime::Seconds() - TurnStartSeconds) * 1000.0);
	TurnStartSeconds = 0.0;

	Record.DebugInfo = ParseJsonObject(DebugInfo);
	if (Record.DebugInfo.IsValid())
	{
		double Seconds = 0.0;
		if (Record.DebugInfo->TryGetNumberField(TEXT("duration_s"), Seconds))
		{
			Record.DurationMs = static_cast<int32>(Seconds * 1000.0);
		}
		double Ttft = 0.0;
		if (Record.DebugInfo->TryGetNumberField(TEXT("time_to_first_token_ms"), Ttft))
		{
			Record.TtftMs = static_cast<int32>(Ttft);
		}
	}

	if (!TurnFailure.IsEmpty())
	{
		Record.Status       = ETryllLabCellStatus::Error;
		Record.ErrorMessage = TurnFailure;
	}
	else if (Status == ETryllTurnStatus::Error)
	{
		const FString Reported = FirstError(Record.DebugInfo);
		Record.Status       = ETryllLabCellStatus::Error;
		Record.ErrorMessage = Reported.IsEmpty() ? TEXT("Turn failed") : Reported;
	}
	else if (Status == ETryllTurnStatus::Cancelled)
	{
		Record.Status       = ETryllLabCellStatus::Error;
		Record.ErrorMessage = TEXT("Cancelled");
	}
	else
	{
		Record.Status = FTryllDialogLabChecks::EvaluateStep(
			CurrentScript(), CurrentStep(), Record.AnswerText,
			Record.DebugInfo, ETryllDiagnosticsAbsenceReason::None, Record.CheckOutcomes,
			CurrentVariant().bUseLiveTools, MissingResumedTools);
	}

	if (Record.Status == ETryllLabCellStatus::Error)
	{
		bColumnFailed = true;
	}

	++DoneUnits;
	ReportProgress();
	++TurnIndex;
	RunNextGeneratedStep();
}

void FTryllDialogLabRunner::FinishRepeat()
{
	StoreSample(CurrentSample);

	// A stateless agent has nothing to rewind: the server cleared its dialogue at the end of
	// every turn, so the next repeat already starts clean. Sending the request anyway would
	// imply a rewind that never happened.
	if (!CurrentVariant().bMaintainDialogueHistory)
	{
		AdvanceRepeat();
		return;
	}
	RewindThenAdvance();
}

void FTryllDialogLabRunner::RewindThenAdvance()
{
	if (bStopRequested || !Agent.IsValid())
	{
		AdvanceRepeat();
		return;
	}

	const uint64 Gen = Generation;
	TWeakPtr<FTryllDialogLabRunner> SelfWeak = AsShared();
	Agent->RemoveInteractionsFromEnd(MAX_int32,
		[SelfWeak, Gen](uint32, const FTryllError& Error)
		{
			TSharedPtr<FTryllDialogLabRunner> Self = SelfWeak.Pin();
			if (!Self.IsValid() || Self->IsStale(Gen))
			{
				return;
			}
			if (Error.IsOk())
			{
				Self->AdvanceRepeat();
				return;
			}

			// Remaining repeats on this agent would see contaminated history.
			Self->Result.Issues.Add(FString::Printf(
				TEXT("%s sample %s: RemoveInteractionsFromEnd failed — %s"),
				*Self->Who(), *Self->CurrentSample.SampleId, *Error.Message));

			const int32 Repeats = FMath::Max(1, Self->Snapshot.Repeats);
			for (int32 Skipped = Self->RepeatIndex + 1; Skipped < Repeats; ++Skipped)
			{
				FTryllLabSampleResult Sample = Self->MakeSample(
					Self->VariantIndex, Skipped, Self->Snapshot.DeriveSeed(Self->VariantIndex, Skipped));
				Self->MarkAllTurns(Sample, ETryllLabCellStatus::Skipped, TEXT("Prior rewind failed"));
				Self->StoreSample(Sample);
				Self->DoneUnits += Self->CurrentScript().GeneratedStepCount();
				Self->ReportProgress();
			}
			Self->AdvanceVariant();
		});
}

void FTryllDialogLabRunner::AdvanceRepeat()
{
	++RepeatIndex;
	if (bStopRequested || RepeatIndex >= FMath::Max(1, Snapshot.Repeats))
	{
		AdvanceVariant();
		return;
	}
	BeginRepeat();
}

void FTryllDialogLabRunner::AdvanceVariant()
{
	DestroyCurrentAgent();
	++VariantIndex;
	BeginVariant();
}

void FTryllDialogLabRunner::AdvanceDialogue()
{
	DestroyCurrentAgent();
	++DialogueIndex;
	VariantIndex = 0;
	if (bStopRequested || DialogueIndex >= Snapshot.Dialogues.Num())
	{
		Finish();
		return;
	}
	// A finished dialogue is complete in every column, so showing it now shows no biased
	// subset of its own cells.
	OnDialogueCompleted.ExecuteIfBound(Result, DialogueIndex);
	if (!bRunning)
	{
		return; // the listener stopped the run
	}
	BeginVariant();
}

void FTryllDialogLabRunner::FailCurrentVariant(const FString& Reason, bool bMarkRunFailed)
{
	Result.Issues.Add(FString::Printf(TEXT("%s: %s"), *Who(), *Reason));
	if (bMarkRunFailed)
	{
		Result.Status = ETryllDialogLabRunStatus::Failed;
	}

	const int32 Repeats = FMath::Max(1, Snapshot.Repeats);
	for (int32 Repeat = 0; Repeat < Repeats; ++Repeat)
	{
		FTryllLabSampleResult Sample = MakeSample(
			VariantIndex, Repeat, Snapshot.DeriveSeed(VariantIndex, Repeat));
		MarkAllTurns(Sample, ETryllLabCellStatus::Error, Reason);
		StoreSample(Sample);
		DoneUnits += CurrentScript().GeneratedStepCount();
		ReportProgress();
	}

	DestroyCurrentAgent();
	++VariantIndex;
	BeginVariant();
}

void FTryllDialogLabRunner::AbortRepeatsWithSeedFailure(const FString& Reason)
{
	MarkAllTurns(CurrentSample, ETryllLabCellStatus::Error, Reason);
	StoreSample(CurrentSample);
	DoneUnits += CurrentScript().GeneratedStepCount();
	ReportProgress();
	Result.Issues.Add(FString::Printf(TEXT("%s sample %s: %s"),
		*Who(), *CurrentSample.SampleId, *Reason));

	// History may be partial, so no further repeats run on this agent.
	for (int32 Skipped = RepeatIndex + 1; Skipped < FMath::Max(1, Snapshot.Repeats); ++Skipped)
	{
		FTryllLabSampleResult Sample = MakeSample(VariantIndex, Skipped, Snapshot.DeriveSeed(VariantIndex, Skipped));
		MarkAllTurns(Sample, ETryllLabCellStatus::Skipped, TEXT("Prior seeding failed"));
		StoreSample(Sample);
		DoneUnits += CurrentScript().GeneratedStepCount();
		ReportProgress();
	}
	AdvanceVariant();
}

void FTryllDialogLabRunner::Finish()
{
	if (!bRunning)
	{
		return;
	}
	bRunning = false;
	if (TurnWatchdog.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TurnWatchdog);
		TurnWatchdog.Reset();
	}

	DestroyCurrentAgent();

	if (UTryllEditorSubsystem* Sub = Subsystem.Get())
	{
		if (PausedHandle.IsValid())
		{
			Sub->OnPausedNative.Remove(PausedHandle);
			PausedHandle.Reset();
		}
		if (ConnectionHandle.IsValid())
		{
			Sub->OnConnectionChangedNative.Remove(ConnectionHandle);
			ConnectionHandle.Reset();
		}
	}

	if (bTimedOut)
	{
		Result.Status = ETryllDialogLabRunStatus::Failed;
		Result.Issues.Add(FString::Printf(TEXT("Run stopped: %s."), *TimeoutMessage(TurnTimeoutSeconds)));
	}
	else if (bStopRequested)
	{
		Result.Status = ETryllDialogLabRunStatus::Stopped;
	}
	else if (Result.Issues.Num() > 0 && !Result.Samples.ContainsByPredicate(
		[](const FTryllLabSampleResult& Sample)
		{
			return Sample.Turns.ContainsByPredicate([](const FTryllLabTurnRecord& T)
			{
				return T.Status != ETryllLabCellStatus::Error;
			});
		}))
	{
		Result.Status = ETryllDialogLabRunStatus::Failed;
	}
	Result.CompletedAtUtc = FDateTime::UtcNow();

	OnFinished.ExecuteIfBound(Result);
}

void FTryllDialogLabRunner::DestroyCurrentAgent()
{
	if (!Agent.IsValid())
	{
		return;
	}
	Agent->SetOnAnswerText(nullptr);
	Agent->SetOnTurnComplete(nullptr);
	Agent->SetOnError(nullptr);
	Agent->SetOnToolHandlerError(nullptr);
	if (UTryllEditorSubsystem* Sub = Subsystem.Get())
	{
		Sub->RequestDestroyAgent(Agent);
	}
	Agent.Reset();
}

void FTryllDialogLabRunner::ReportProgress()
{
	OnProgress.ExecuteIfBound(DoneUnits, TotalUnits);
}

FTryllLabSampleResult FTryllDialogLabRunner::MakeSample(
	int32 InVariantIndex, int32 Repeat, int32 Seed) const
{
	const FTryllLabVariantSnapshot& Variant = Snapshot.Variants[InVariantIndex];
	FTryllLabSampleResult Sample;
	Sample.SampleId      = VariantLetter(InVariantIndex) + FString::FromInt(Repeat + 1);
	Sample.VariantIndex  = InVariantIndex;
	Sample.VariantLabel  = Variant.Label;
	Sample.VariantSwatch = Variant.Swatch;
	Sample.Repeat        = Repeat;
	Sample.Seed          = Seed;
	return Sample;
}

void FTryllDialogLabRunner::MakeRectangularTurns(
	FTryllLabSampleResult& Sample, const FString& SkipReason) const
{
	Sample.Turns.Reset();
	for (int32 Index = 0; Index < CurrentScript().Steps.Num(); ++Index)
	{
		const FTryllLabStepSnapshot& Step = CurrentScript().Steps[Index];
		const bool bSeeded = Index < CurrentScript().SeedTurnCount;

		FTryllLabTurnRecord Record;
		Record.DialogueIndex = DialogueIndex;
		Record.TurnIndex    = Index;
		Record.bSeeded      = bSeeded;
		Record.UserText     = Step.User;
		Record.AnswerText   = bSeeded ? Step.Assistant : FString();
		Record.Status       = ETryllLabCellStatus::Skipped;
		Record.ErrorMessage = SkipReason;
		Sample.Turns.Add(MoveTemp(Record));
	}
}

void FTryllDialogLabRunner::MarkAllTurns(
	FTryllLabSampleResult& Sample, ETryllLabCellStatus Status, const FString& Error) const
{
	MakeRectangularTurns(Sample, Error);
	for (FTryllLabTurnRecord& Record : Sample.Turns)
	{
		Record.Status       = Status;
		Record.ErrorMessage = Error;
	}
}
