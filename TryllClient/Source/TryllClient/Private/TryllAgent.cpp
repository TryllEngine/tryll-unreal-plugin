// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllAgent.h"
#include "TryllClientModule.h"  // LogTryll
#include "ClientCodec.h"
#include "TryllConnection.h"
#include "TryllUtf8.h"
#include "Generated/TryllCalls.gen.h"
#include "TryllWeakCapture.h"
#include "TryllTextProvider.h"
#include "TryllTextResolve.h"
#include "TryllConnectionCalls.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "tryll/Protocol.h"

using Tryll::Client::Utf8ToFString;

FTryllAgent::FTryllAgent(TSharedPtr<FTryllConnection> InConnection, std::uint64_t InAgentId,
                         const TArray<FTryllVariableDecl>& InDeclaredVariables)
	: Connection(MoveTemp(InConnection))
	, AgentId(InAgentId)
	, Variables(MakeUnique<FTryllAgentVariables>(this, InDeclaredVariables))
{
}

FTryllAgent::~FTryllAgent() = default;

void FTryllAgent::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (TPair<FString, TObjectPtr<UTryllNodeParamsBase>>& Pair : ParamsBaseline)
	{
		Collector.AddReferencedObject(Pair.Value);
	}
}

void FTryllAgent::SendMessage(const FString& Text)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		return;
	}
	if (Text.IsEmpty())
	{
		return;
	}
	const std::uint64_t Rid = Connection->AllocateRequestId();
	Variables->FlushIfDirty(nullptr, Rid);
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	Connection->Call(
		Rid,
		Tryll::Client::BuildSendMessageRequest(Rid, Aid, Text),
		this,
		[Weak, Rid, Aid](const ::Tryll::Message* Msg, const FTryllErrorContext& Err) -> bool
		{
			const auto Body = Msg ? Msg->body_type() : ::Tryll::MessageBody_NONE;
			if (!Err.IsEmpty() || Body == ::Tryll::MessageBody_ErrorResponse)
			{
				FTryllError Error = !Err.IsEmpty()
					? FTryllError(Err.Code, Err.Message)
					: FTryllError(ETryllErrorCode::MalformedMessage, TEXT("unexpected reply"));
				if (Err.IsEmpty() && Msg)
				{
					if (const auto* ER = Msg->body_as<::Tryll::ErrorResponse>())
					{
						Error = FTryllError(ER->code(), Utf8ToFString(ER->message()));
					}
				}
				auto Run = [Weak, Error, Aid, Rid]()
				{
					if (auto Live = Weak.Pin())
					{
						if (Live->Connection.IsValid())
						{
							FTryllObservationNote Note;
							Note.Direction = ETryllObservationDirection::Inbound;
							Note.Body = ::Tryll::MessageBody_ErrorResponse;
							Note.RequestBody = ::Tryll::MessageBody_SendMessageRequest;
							Note.bIsError = true;
							Note.RequestId = Rid;
							Note.AgentId = Aid;
							Note.Code = static_cast<uint32>(Error.Code);
							Note.Message = Error.Message;
							Live->Connection->GetObserverHub().AnnotateNote(Note);
						}
						Live->HandleError(Error);
					}
				};
				if (IsInGameThread())
				{
					Run();
				}
				else if (auto Live = Weak.Pin())
				{
					if (Live->Connection.IsValid())
					{
						Live->Connection->PostToGameThread(MoveTemp(Run));
					}
				}
				return true;
			}
			if (Body == ::Tryll::MessageBody_AnswerText)
			{
				return false;
			}
			if (Body == ::Tryll::MessageBody_TurnComplete)
			{
				return true;
			}
			auto Run = [Weak]()
			{
				if (auto Live = Weak.Pin())
				{
					Live->HandleError(FTryllError(ETryllErrorCode::MalformedMessage, TEXT("unexpected reply")));
				}
			};
			if (IsInGameThread())
			{
				Run();
			}
			else if (auto Live = Weak.Pin())
			{
				if (Live->Connection.IsValid())
				{
					Live->Connection->PostToGameThread(MoveTemp(Run));
				}
			}
			return true;
		},
		::Tryll::MessageBody_SendMessageRequest,
		[Weak, Aid, Text](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_SendMessageRequest;
				Note.RequestBody = ::Tryll::MessageBody_SendMessageRequest;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Note.Text = Text;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::Cancel(ETryllCancelMode Mode)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		return;
	}
	const uint64 Aid = AgentId;
	const auto Weak = CaptureTryllLive(this);
	Tryll::Client::Calls::Cancel(
		*Connection,
		this,
		Aid,
		static_cast<::Tryll::CancelMode>(Mode),
		[](const FTryllError&) {},
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_CancelRequest;
				Note.RequestBody = ::Tryll::MessageBody_CancelRequest;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::Resume(
	const FString&                       ResumeNode,
	TFunction<void(const FTryllError&)>  OnComplete)
{
	ResumeInternal(ResumeNode, {}, MoveTemp(OnComplete));
}

void FTryllAgent::ResumeWithToolResult(
	const FTryllToolResult& ToolResult,
	const FString& ResumeNode,
	TFunction<void(const FTryllError&)> OnComplete)
{
	TArray<FTryllToolResult> Results;
	Results.Add(ToolResult);
	ResumeWithToolResults(Results, ResumeNode, MoveTemp(OnComplete));
}

void FTryllAgent::ResumeWithToolResults(
	const TArray<FTryllToolResult>& ToolResults,
	const FString& ResumeNode,
	TFunction<void(const FTryllError&)> OnComplete)
{
	TArray<TPair<FString, FString>> WireResults;
	WireResults.Reserve(ToolResults.Num());
	for (const FTryllToolResult& ToolResult : ToolResults)
	{
		WireResults.Emplace(ToolResult.CallId, ToolResult.Result);
	}
	ResumeInternal(ResumeNode, MoveTemp(WireResults), MoveTemp(OnComplete));
}

void FTryllAgent::ResumeInternal(
	const FString& ResumeNode,
	TArray<TPair<FString, FString>> ToolResults,
	TFunction<void(const FTryllError&)> OnComplete)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete(FTryllError{0, TEXT("Agent not ready")});
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	Tryll::Client::Calls::ResumeAgent(
		*Connection,
		this,
		[Aid, ResumeNode, ToolResults = MoveTemp(ToolResults)](uint64 RequestId)
		{
			return Tryll::Client::BuildResumeAgentRequest(RequestId, Aid, ResumeNode, ToolResults);
		},
		[Weak, OnComplete = MoveTemp(OnComplete)](const FTryllError& Error)
		{
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(Error);
				}
			}
		},
		[Weak, Aid, ResumeNode](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Variables->FlushIfDirty(nullptr, RequestId);
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_ResumeAgentRequest;
				Note.RequestBody = ::Tryll::MessageBody_ResumeAgentRequest;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Note.Text = ResumeNode;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::RegisterTool(const FString& ToolName, FTryllToolHandler Handler)
{
	if (ToolName.IsEmpty() || !Handler)
	{
		return;
	}
	ToolHandlers.Add(ToolName, MoveTemp(Handler));
}

void FTryllAgent::RegisterTool(
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

bool FTryllAgent::UnregisterTool(const FString& ToolName)
{
	return ToolHandlers.Remove(ToolName) > 0;
}

bool FTryllAgent::HasNodeEventListener(ETryllAgentNodeEventKind Kind) const
{
	if (ListenerProbe)
	{
		return ListenerProbe(Kind);
	}
	switch (Kind)
	{
	case ETryllAgentNodeEventKind::ToolCall:            return static_cast<bool>(OnToolCall);
	case ETryllAgentNodeEventKind::ToolCallWithId:      return static_cast<bool>(OnToolCallWithId);
	case ETryllAgentNodeEventKind::ToolCallDetailed:    return static_cast<bool>(OnToolCallEvent);
	case ETryllAgentNodeEventKind::IntentClassified:    return static_cast<bool>(OnIntentClassified);
	case ETryllAgentNodeEventKind::IntentLlmClassified: return static_cast<bool>(OnIntentLlmClassified);
	case ETryllAgentNodeEventKind::Paused:              return static_cast<bool>(OnPaused);
	case ETryllAgentNodeEventKind::NodeEvent:           return static_cast<bool>(OnNodeEvent);
	default:                                            return false;
	}
}

void FTryllAgent::SetOnToolCall(TFunction<void(const FString& ToolName, const FString& ArgumentsJson)> Callback)
{
	OnToolCall = MoveTemp(Callback);
}

void FTryllAgent::SetOnToolCallWithId(TFunction<void(const FString& CallId, const FString& ToolName, const FString& ArgumentsJson)> Callback)
{
	OnToolCallWithId = MoveTemp(Callback);
}

void FTryllAgent::SetOnToolCallEvent(TFunction<void(const FTryllToolCall& Call)> Callback)
{
	OnToolCallEvent = MoveTemp(Callback);
}

void FTryllAgent::SetOnIntentClassified(TFunction<void(const FTryllIntentClassification& Classification)> Callback)
{
	OnIntentClassified = MoveTemp(Callback);
}

void FTryllAgent::SetOnIntentLlmClassified(TFunction<void(const FTryllIntentLlmClassification& Classification)> Callback)
{
	OnIntentLlmClassified = MoveTemp(Callback);
}

void FTryllAgent::SetOnPaused(TFunction<void(const FString& NodeName, const FString& PendingExit)> Callback)
{
	OnPaused = MoveTemp(Callback);
}

void FTryllAgent::SetOnNodeEvent(TFunction<void(const FString& NodeName, const FString& EventType, const TArray<FTryllNodeEventKeyValue>& KvPairs)> Callback)
{
	OnNodeEvent = MoveTemp(Callback);
}

void FTryllAgent::SetListenerProbe(TFunction<bool(ETryllAgentNodeEventKind)> Probe)
{
	ListenerProbe = MoveTemp(Probe);
}

void FTryllAgent::DispatchGenericNodeEvent(const FTryllDecodedNodeEvent& Event)
{
	if (HasNodeEventListener(ETryllAgentNodeEventKind::NodeEvent) && OnNodeEvent)
	{
		OnNodeEvent(Event.NodeName, Event.EventType, Event.KvPairs);
	}
}

void FTryllAgent::HandleNodeEvent(const FTryllDecodedNodeEvent& Event, bool bSessionListenerBound)
{
	switch (Event.Kind)
	{
	case ETryllNodeEventKind::ToolCall:
	{
		// Record before any agent listener. Session listeners already ran; the
		// pending batch is not observable to them, and the matching pause frame
		// is a later event, so the batch is in place before auto-resume.
		HandleToolCall(
			Event.ToolCall.CallId,
			Event.ToolCall.NodeName,
			Event.ToolCall.ToolName,
			Event.ToolCall.ArgumentsJson,
			Event.ToolCall.Disposition);

		const bool bDetailed = HasNodeEventListener(ETryllAgentNodeEventKind::ToolCallDetailed);
		const bool bWithId = HasNodeEventListener(ETryllAgentNodeEventKind::ToolCallWithId);
		const bool bLegacy = HasNodeEventListener(ETryllAgentNodeEventKind::ToolCall);
		bPendingBatchObserved |= bSessionListenerBound || bDetailed || bWithId || bLegacy;
		if (bDetailed && OnToolCallEvent)
		{
			OnToolCallEvent(Event.ToolCall);
		}
		if (bWithId && OnToolCallWithId)
		{
			OnToolCallWithId(Event.ToolCall.CallId, Event.ToolCall.ToolName, Event.ToolCall.ArgumentsJson);
		}
		if (bLegacy && OnToolCall)
		{
			OnToolCall(Event.ToolCall.ToolName, Event.ToolCall.ArgumentsJson);
		}
		if (!bDetailed && !bWithId && !bLegacy)
		{
			DispatchGenericNodeEvent(Event);
		}
		break;
	}
	case ETryllNodeEventKind::IntentClassified:
		if (HasNodeEventListener(ETryllAgentNodeEventKind::IntentClassified))
		{
			if (OnIntentClassified)
			{
				OnIntentClassified(Event.Intent);
			}
		}
		else
		{
			DispatchGenericNodeEvent(Event);
		}
		break;
	case ETryllNodeEventKind::IntentLlmClassified:
		if (HasNodeEventListener(ETryllAgentNodeEventKind::IntentLlmClassified))
		{
			if (OnIntentLlmClassified)
			{
				OnIntentLlmClassified(Event.IntentLlm);
			}
		}
		else
		{
			DispatchGenericNodeEvent(Event);
		}
		break;
	case ETryllNodeEventKind::Paused:
	{
		// Agent listeners run before RegisterTool auto-resume, same window as
		// the session OnPaused broadcast the caller already performed.
		const bool bAgentPaused = HasNodeEventListener(ETryllAgentNodeEventKind::Paused);
		const bool bAgentGeneric = HasNodeEventListener(ETryllAgentNodeEventKind::NodeEvent);
		if (bAgentPaused)
		{
			if (OnPaused)
			{
				OnPaused(Event.NodeName, Event.PendingExit);
			}
		}
		else
		{
			DispatchGenericNodeEvent(Event);
		}
		HandlePaused(
			Event.NodeName,
			bSessionListenerBound || bAgentPaused || bAgentGeneric);
		break;
	}
	default:
		DispatchGenericNodeEvent(Event);
		break;
	}
}

void FTryllAgent::HandleToolCall(
	const FString& CallId,
	const FString& NodeName,
	const FString& ToolName,
	const FString& ArgumentsJson,
	const FString& Disposition)
{
	FTryllToolCall Call;
	Call.CallId = CallId;
	Call.NodeName = NodeName;
	Call.ToolName = ToolName;
	Call.ArgumentsJson = ArgumentsJson;
	Call.Disposition = Disposition;
	PendingToolCalls.Add(MoveTemp(Call));
}

void FTryllAgent::HandlePaused(const FString& NodeName, bool bExternalListenerBound)
{
	PausedNodeName = NodeName;

	// A tool-call listener that saw this batch can answer it with
	// ResumeWithToolResult(s); it says nothing about a pause no call preceded.
	const bool bBatchObserved = bPendingBatchObserved;
	bPendingBatchObserved = false;

	// Nothing here can resume the turn, and no client delegate is listening for the
	// pause either — so this turn will stay open until Cancel or teardown. The
	// server cannot detect this (it has no idea what the client is listening for),
	// which is exactly why the check lives on this side.
	const auto WarnIfUnresumable = [this, &NodeName](bool bResumableByListener)
	{
		if (bResumableByListener)
			return;
		UE_LOG(LogTryll, Warning,
			TEXT("Agent %llu paused at node '%s' and nothing can resume it: no OnPaused ")
			TEXT("listener is bound, no tool-call listener received a call for this pause, ")
			TEXT("and no registered tool handler covers it. The turn stays open and further ")
			TEXT("SendMessage calls fail with AgentBusy. Bind UTryllAgentComponent::OnPaused ")
			TEXT("or UTryllSubsystem::OnPaused and call Resume, handle OnToolCallWithId and ")
			TEXT("call ResumeWithToolResult(s), or RegisterTool a handler for every tool in ")
			TEXT("the batch."),
			AgentId, *NodeName);
	};

	if (!PendingToolCalls.IsEmpty())
	{
		if (!PausedToolCalls.IsEmpty())
		{
			// Defensive: a fresh batch should never surface while a previous one
			// is still retained (unresolved handler failure or resume rejection) —
			// discard the stale batch rather than silently drop the new calls.
			PausedToolCalls.Reset();
			PausedToolResults.Reset();
		}
		PausedToolCalls.Append(MoveTemp(PendingToolCalls));
		PendingToolCalls.Reset();
	}

	if (PausedToolCalls.IsEmpty())
	{
		// A Pause node (or a pausing disposition with no calls): only game code can
		// resume this one.
		WarnIfUnresumable(bExternalListenerBound);
		return;
	}

	for (const FTryllToolCall& Call : PausedToolCalls)
	{
		if (Call.NodeName != NodeName)
		{
			WarnIfUnresumable(bExternalListenerBound || bBatchObserved);
			return; // mixed/foreign batch — stays entirely manual
		}
	}

	if (!RunPausedBatch())
	{
		WarnIfUnresumable(bExternalListenerBound || bBatchObserved);
	}
}

bool FTryllAgent::RunPausedBatch()
{
	// A mixed registered/unregistered batch stays entirely manual. This avoids
	// executing side effects that cannot yet be atomically resumed.
	for (const FTryllToolCall& Call : PausedToolCalls)
	{
		if (!ToolHandlers.Contains(Call.ToolName))
		{
			return false;
		}
	}

	for (const FTryllToolCall& Call : PausedToolCalls)
	{
		if (PausedToolResults.Contains(Call.CallId))
		{
			continue; // already produced a result on a previous attempt
		}

		const FTryllToolHandlerResult HandlerResult = ToolHandlers[Call.ToolName](Call);
		if (!HandlerResult.bSuccess)
		{
			ReportToolHandlerError(FTryllError(
				ETryllErrorCode::InvalidToolResults,
				FString::Printf(
					TEXT("Tool handler '%s' failed; turn remains paused: %s"),
					*Call.ToolName,
					*HandlerResult.Error)));
			// The failure was already surfaced through OnToolHandlerError; the caller
			// must not additionally warn about an unresumable pause.
			return true;
		}

		FTryllToolResult ToolResult;
		ToolResult.CallId = Call.CallId;
		ToolResult.Result = HandlerResult.Result;
		PausedToolResults.Add(Call.CallId, MoveTemp(ToolResult));
	}

	TArray<FTryllToolResult> Results;
	Results.Reserve(PausedToolCalls.Num());
	bool bNeedsNativeResults = false;
	for (const FTryllToolCall& Call : PausedToolCalls)
	{
		if (const FTryllToolResult* Found = PausedToolResults.Find(Call.CallId))
		{
			Results.Add(*Found);
		}
		bNeedsNativeResults |= Call.RequiresNativeResults();
	}

	auto OnComplete = [this](const FTryllError& Error)
	{
		if (Error.IsOk())
		{
			// Clear only on a successful Ack — a rejected/disconnected resume keeps
			// the batch (with any cached results) for RetryPausedToolBatch.
			PausedToolCalls.Reset();
			PausedToolResults.Reset();
			PausedNodeName.Reset();
		}
		else
		{
			ReportToolHandlerError(Error);
		}
	};
	// AwaitResult needs the result batch. Acknowledgement / omit dispositions
	// already have synthetic/no pending results — sending tool_results would 3016.
	if (bNeedsNativeResults)
	{
		ResumeWithToolResults(Results, {}, MoveTemp(OnComplete));
	}
	else
	{
		Resume({}, MoveTemp(OnComplete));
	}
	return true;
}

void FTryllAgent::RetryPausedToolBatch()
{
	if (PausedToolCalls.IsEmpty())
	{
		return;
	}
	RunPausedBatch();
}

void FTryllAgent::ClearPausedToolBatch()
{
	PausedToolCalls.Reset();
	PausedToolResults.Reset();
	PausedNodeName.Reset();
}

void FTryllAgent::SetOnToolHandlerError(TFunction<void(const FTryllError&)> Callback)
{
	OnToolHandlerError = MoveTemp(Callback);
}

void FTryllAgent::ReportToolHandlerError(const FTryllError& Error)
{
	if (OnToolHandlerError)
	{
		OnToolHandlerError(Error);
	}
	HandleError(Error);
}

void FTryllAgent::SetCreatedVoice(const TOptional<FTryllVoiceProfileData>& InVoice)
{
	Voice = InVoice;
	DeliveryMode = InVoice.IsSet() ? InVoice->DefaultDeliveryMode : FString();
}

void FTryllAgent::SetVoice(
	const FTryllVoiceProfileData& NewVoice,
	TFunction<void(const FTryllError&)> OnComplete)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete(FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	::Tryll::VoiceProfileT Wire;
	NewVoice.ToWire(Wire);
	Tryll::Client::Calls::SetAgentVoice(
		*Connection,
		this,
		Aid,
		Wire,
		[Weak, NewVoice, OnComplete = MoveTemp(OnComplete)](const FTryllError& Error)
		{
			if (auto Live = Weak.Pin())
			{
				if (Error.IsOk())
				{
					// Server rule: neutral (stored here as an empty mode) survives a
					// swap, and so does any named mode the new profile also defines.
					// Otherwise the server falls back to the new profile's default.
					const FString Previous = Live->DeliveryMode.IsEmpty()
						? FTryllVoiceProfileData::NeutralMode
						: Live->DeliveryMode;
					Live->Voice = NewVoice;
					if (NewVoice.HasMode(Previous))
					{
						Live->DeliveryMode = Previous.Equals(FTryllVoiceProfileData::NeutralMode)
							? FString()
							: Previous;
					}
					else
					{
						Live->DeliveryMode = NewVoice.DefaultDeliveryMode;
					}
				}
				if (OnComplete)
				{
					OnComplete(Error);
				}
			}
		},
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Variables->FlushIfDirty(nullptr, RequestId);
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_SetAgentVoiceRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::SetDeliveryMode(
	const FString& Mode,
	float Intensity,
	TFunction<void(const FTryllError&)> OnComplete)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete(FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	Tryll::Client::Calls::SetAgentDeliveryMode(
		*Connection,
		this,
		Aid,
		Mode,
		Intensity,
		[Weak, Mode, OnComplete = MoveTemp(OnComplete)](const FTryllError& Error)
		{
			if (auto Live = Weak.Pin())
			{
				if (Error.IsOk())
				{
					Live->DeliveryMode = Mode;
				}
				if (OnComplete)
				{
					OnComplete(Error);
				}
			}
		},
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_SetAgentDeliveryModeRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::PrefillKvCache(
	TFunction<void(const FTryllKvCachePrefillResult&, const FTryllError&)> OnComplete)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete({}, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	Tryll::Client::Calls::PrefillAgentKvCache(
		*Connection,
		this,
		Aid,
		[Weak, OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::PrefillAgentKvCacheResponseT> Resp, const FTryllError& Error)
		{
			FTryllKvCachePrefillResult Result;
			if (Resp)
			{
				Result.AgentId = static_cast<int64>(Resp->agent_id);
				Result.bApplicable = Resp->applicable;
				Result.Residency = static_cast<ETryllKvCacheResidency>(Resp->residency);
				Result.PrefixStatus = static_cast<ETryllKvCachePrefixStatus>(Resp->prefix_status);
				Result.EligibleNodeCount = static_cast<int32>(Resp->eligible_node_count);
				Result.ContextsCreated = static_cast<int32>(Resp->contexts_created);
				Result.TokensDecoded = static_cast<int32>(Resp->tokens_decoded);
				Result.TokensReused = static_cast<int32>(Resp->tokens_reused);
				Result.DebugInfo = UTF8_TO_TCHAR(Resp->debug_info.c_str());
			}
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(Result, Error);
				}
			}
		},
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Variables->FlushIfDirty(nullptr, RequestId);
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_PrefillAgentKvCacheRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::EvictKvCache(
	TFunction<void(const FTryllKvCacheEvictResult&, const FTryllError&)> OnComplete)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete({}, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	Tryll::Client::Calls::EvictAgentKvCache(
		*Connection,
		this,
		Aid,
		[Weak, OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::EvictAgentKvCacheResponseT> Resp, const FTryllError& Error)
		{
			FTryllKvCacheEvictResult Result;
			if (Resp)
			{
				Result.AgentId = static_cast<int64>(Resp->agent_id);
				Result.bApplicable = Resp->applicable;
				Result.Residency = static_cast<ETryllKvCacheResidency>(Resp->residency);
				Result.PrefixStatus = static_cast<ETryllKvCachePrefixStatus>(Resp->prefix_status);
				Result.EligibleNodeCount = static_cast<int32>(Resp->eligible_node_count);
				Result.ContextsEvicted = static_cast<int32>(Resp->contexts_evicted);
				Result.DebugInfo = UTF8_TO_TCHAR(Resp->debug_info.c_str());
			}
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(Result, Error);
				}
			}
		},
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Variables->FlushIfDirty(nullptr, RequestId);
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_EvictAgentKvCacheRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::GetKvCacheStatus(
	TFunction<void(const FTryllKvCacheStatus&, const FTryllError&)> OnComplete)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete({}, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	Tryll::Client::Calls::GetAgentKvCacheStatus(
		*Connection,
		this,
		Aid,
		[Weak, OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::GetAgentKvCacheStatusResponseT> Resp, const FTryllError& Error)
		{
			FTryllKvCacheStatus Result;
			if (Resp)
			{
				Result.AgentId = static_cast<int64>(Resp->agent_id);
				Result.bApplicable = Resp->applicable;
				Result.Residency = static_cast<ETryllKvCacheResidency>(Resp->residency);
				Result.PrefixStatus = static_cast<ETryllKvCachePrefixStatus>(Resp->prefix_status);
				Result.EligibleNodeCount = static_cast<int32>(Resp->eligible_node_count);
				Result.DebugInfo = UTF8_TO_TCHAR(Resp->debug_info.c_str());
			}
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(Result, Error);
				}
			}
		},
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Variables->FlushIfDirty(nullptr, RequestId);
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_GetAgentKvCacheStatusRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::AppendInteractions(
	const TArray<FTryllDialogInteraction>& Interactions,
	TFunction<void(uint32 Count, const FTryllError&)> OnComplete)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete(0, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	Tryll::Client::Calls::AppendInteractions(
		*Connection,
		this,
		[Aid, Interactions](uint64 RequestId)
		{
			return Tryll::Client::BuildAppendInteractionsRequest(RequestId, Aid, Interactions);
		},
		[Weak, OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::AppendInteractionsResponseT> Resp, const FTryllError& Error)
		{
			const uint32 Count = Resp ? Resp->appended_count : 0;
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(Count, Error);
				}
			}
		},
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Variables->FlushIfDirty(nullptr, RequestId);
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_AppendInteractionsRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::RemoveInteractionsFromEnd(
	int32 Count,
	TFunction<void(uint32 RemovedCount, const FTryllError&)> OnComplete)
{
	if (Count < 0)
	{
		if (OnComplete)
			OnComplete(0, FTryllError(ETryllErrorCode::InvalidVariableValue, TEXT("count must be >= 0")));
		return;
	}
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete(0, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	const uint32 RemoveCount = static_cast<uint32>(Count);
	Tryll::Client::Calls::RemoveInteractionsFromEnd(
		*Connection,
		this,
		Aid,
		RemoveCount,
		[Weak, OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::RemoveInteractionsFromEndResponseT> Resp, const FTryllError& Error)
		{
			const uint32 Removed = Resp ? Resp->removed_count : 0;
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(Removed, Error);
				}
			}
		},
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Variables->FlushIfDirty(nullptr, RequestId);
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_RemoveInteractionsFromEndRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::ExportDialog(
	TFunction<void(const FTryllDialogSnapshot&, const FTryllError&)> OnComplete)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete({}, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	const uint64 Rid = Connection->AllocateRequestId();
	Connection->Call(
		Rid,
		Tryll::Client::BuildExportDialogRequest(Rid, Aid),
		this,
		[Weak, OnComplete = MoveTemp(OnComplete)](const ::Tryll::Message* Msg, const FTryllErrorContext& Err) mutable -> bool
		{
			FTryllDialogSnapshot Snapshot;
			FTryllError Error;
			if (!Err.IsEmpty())
			{
				Error = FTryllError(Err.Code, Err.Message);
			}
			else if (Msg && Msg->body_type() == ::Tryll::MessageBody_ExportDialogResponse)
			{
				const auto* Resp = Msg->body_as<::Tryll::ExportDialogResponse>();
				if (!Resp || !Resp->snapshot()
					|| !Tryll::Client::UnpackPersistedDialogSnapshot(*Resp->snapshot(), Snapshot, Error))
				{
					if (Error.IsOk())
					{
						Error = FTryllError(ETryllErrorCode::MalformedMessage, TEXT("ExportDialogResponse missing snapshot"));
					}
				}
			}
			else
			{
				Error = FTryllError(ETryllErrorCode::MalformedMessage, TEXT("unexpected ExportDialog body"));
			}
			auto Run = [Weak, OnComplete = MoveTemp(OnComplete), Snapshot = MoveTemp(Snapshot), Error]() mutable
			{
				if (auto Live = Weak.Pin())
				{
					if (OnComplete)
					{
						OnComplete(Snapshot, Error);
					}
				}
			};
			if (IsInGameThread())
			{
				Run();
			}
			else if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Connection->PostToGameThread(MoveTemp(Run));
			}
			return true;
		},
		::Tryll::MessageBody_ExportDialogRequest,
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Variables->FlushIfDirty(nullptr, RequestId);
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_ExportDialogRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::ImportDialog(
	const FTryllDialogSnapshot& Snapshot,
	ETryllDialogCompatibility Compatibility,
	TFunction<void(const FTryllDialogImportResult&, const FTryllError&)> OnComplete)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete({}, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	TArray<uint8> Encoded;
	FTryllError EncError;
	if (Snapshot.ToFlatBufferBytes(Encoded, EncError)
		&& static_cast<uint32>(Encoded.Num()) > static_cast<uint32>(Tryll::MaxDialogSnapshotBytes))
	{
		if (OnComplete)
		{
			OnComplete({}, FTryllError(
				ETryllErrorCode::DialogSnapshotTooLarge,
				TEXT("Encoded dialog snapshot exceeds MaxDialogSnapshotBytes")));
		}
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	Tryll::Client::Calls::ImportDialog(
		*Connection,
		this,
		[Aid, Snapshot, Compatibility](uint64 RequestId)
		{
			FTryllError EncError;
			return Tryll::Client::BuildImportDialogRequest(RequestId, Aid, Snapshot, Compatibility, EncError);
		},
		[Weak, OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::ImportDialogResponseT> Resp, const FTryllError& Error)
		{
			FTryllDialogImportResult Result;
			if (Resp)
			{
				Result.InteractionCount = Resp->interaction_count;
				Result.Warnings.Reserve(static_cast<int32>(Resp->warnings.size()));
				for (const std::string& Warning : Resp->warnings)
				{
					Result.Warnings.Add(UTF8_TO_TCHAR(Warning.c_str()));
				}
			}
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(Result, Error);
				}
			}
		},
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Variables->FlushIfDirty(nullptr, RequestId);
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_ImportDialogRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::SendUpdateVariables(
	TArray<FTryllVariableUpdate> Updates,
	TFunction<void(const FTryllError&)> OnComplete,
	uint64 CarrierRequestId)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete(FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	Tryll::Client::Calls::UpdateAgentVariables(
		*Connection,
		this,
		[Aid, Updates = MoveTemp(Updates)](uint64 RequestId)
		{
			return Tryll::Client::BuildUpdateAgentVariablesRequest(RequestId, Aid, Updates);
		},
		[Weak, OnComplete = MoveTemp(OnComplete)](const FTryllError& Error)
		{
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(Error);
				}
			}
		},
		[Weak, Aid, CarrierRequestId](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_UpdateAgentVariablesRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Note.CarrierRequestId = CarrierRequestId;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::SendGetVariables(TArray<FString> Names,
	TFunction<void(const TArray<FTryllVariableDecl>&, const FTryllError&)> OnComplete)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete(TArray<FTryllVariableDecl>{}, FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	Tryll::Client::Calls::GetAgentVariables(
		*Connection,
		this,
		Aid,
		Names,
		[Weak, OnComplete = MoveTemp(OnComplete)](TUniquePtr<::Tryll::GetAgentVariablesResponseT> Resp, const FTryllError& Error)
		{
			TArray<FTryllVariableDecl> VariablesResult;
			if (Resp)
			{
				VariablesResult.Reserve(static_cast<int32>(Resp->variables.size()));
				for (const std::unique_ptr<::Tryll::AgentVariableT>& Variable : Resp->variables)
				{
					if (!Variable)
					{
						continue;
					}
					FTryllVariableDecl Decl;
					Decl.Name = UTF8_TO_TCHAR(Variable->name.c_str());
					switch (Variable->value.type)
					{
					case ::Tryll::VariableValue_VarInt:
						Decl.Type = ETryllVariableType::Int;
						if (const ::Tryll::VarIntT* Value = Variable->value.AsVarInt())
						{
							Decl.IntValue = Value->v;
						}
						break;
					case ::Tryll::VariableValue_VarFloat:
						Decl.Type = ETryllVariableType::Float;
						if (const ::Tryll::VarFloatT* Value = Variable->value.AsVarFloat())
						{
							Decl.FloatValue = Value->v;
						}
						break;
					case ::Tryll::VariableValue_VarString:
						Decl.Type = ETryllVariableType::String;
						if (const ::Tryll::VarStringT* Value = Variable->value.AsVarString())
						{
							Decl.StringValue = UTF8_TO_TCHAR(Value->v.c_str());
						}
						break;
					case ::Tryll::VariableValue_VarBool:
						Decl.Type = ETryllVariableType::Bool;
						if (const ::Tryll::VarBoolT* Value = Variable->value.AsVarBool())
						{
							Decl.bBoolValue = Value->v;
						}
						break;
					case ::Tryll::VariableValue_VarStringSet:
						Decl.Type = ETryllVariableType::StringSet;
						if (const ::Tryll::VarStringSetT* Value = Variable->value.AsVarStringSet())
						{
							for (const std::string& Item : Value->v)
							{
								Decl.StringSetValue.Add(UTF8_TO_TCHAR(Item.c_str()));
							}
						}
						break;
					default:
						break;
					}
					VariablesResult.Add(MoveTemp(Decl));
				}
			}
			if (auto Live = Weak.Pin())
			{
				if (OnComplete)
				{
					OnComplete(VariablesResult, Error);
				}
			}
		},
		[Weak, Aid](uint64 RequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_GetAgentVariablesRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = RequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void FTryllAgent::ChangeParams(
	const FString&               NodeName,
	UTryllNodeParamsBase*        Params,
	TFunction<void(const FTryllError&)> OnComplete)
{
	if (Params != nullptr && FTryllTextResolve::HasTickedFields(Params))
	{
		// Resolve on a copy so the caller's object keeps its Text IDs. The same
		// fail-fast rules as CreateAgent: nothing is sent when an ID cannot resolve.
		TStrongObjectPtr<UTryllNodeParamsBase> Resolved(
			DuplicateObject<UTryllNodeParamsBase>(Params, GetTransientPackage()));
		bool bChanged = false;
		FTryllError Error;
		if (!FTryllTextResolve::ResolveParams(Resolved.Get(), UTryllTextProvider::Get(), TextTable,
				LocalizedAssetName, NodeName, /*bLiveOnly=*/false, bChanged, Error))
		{
			UE_LOG(LogTryll, Warning, TEXT("ChangeParams: %s"), *Error.Message);
			if (OnComplete)
			{
				OnComplete(Error);
			}
			return;
		}
		SendChangeParams(NodeName, Resolved.Get(), MoveTemp(OnComplete), /*bBusyIsExpected=*/false);
		return;
	}
	SendChangeParams(NodeName, Params, MoveTemp(OnComplete), /*bBusyIsExpected=*/false);
}

void FTryllAgent::ChangeLocalizedParams(
	const FString&               NodeName,
	UTryllNodeParamsBase*        Params,
	TFunction<void(const FTryllError&)> OnComplete)
{
	SendChangeParams(NodeName, Params, MoveTemp(OnComplete), /*bBusyIsExpected=*/true);
}

void FTryllAgent::SendChangeParams(
	const FString&               NodeName,
	UTryllNodeParamsBase*        Params,
	TFunction<void(const FTryllError&)> OnComplete,
	bool                         bBusyIsExpected)
{
	if (!Connection.IsValid() || AgentId == 0)
	{
		if (OnComplete)
			OnComplete(FTryllError{0, TEXT("Agent not ready")});
		return;
	}
	const auto Weak = CaptureTryllLive(this);
	const uint64 Aid = AgentId;
	TStrongObjectPtr<UTryllNodeParamsBase> ParamsClone(
		Params ? DuplicateObject<UTryllNodeParamsBase>(Params, GetTransientPackage()) : nullptr);
	const uint64 RequestId = Connection->AllocateRequestId();
	TArray<uint8> Frame = Tryll::Client::BuildChangeAgentParamsRequest(RequestId, Aid, NodeName, Params);
	Tryll::Client::CallAck(
		*Connection,
		this,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_ChangeAgentParamRequest,
		[Weak, OnComplete = MoveTemp(OnComplete), NodeName, ParamsClone](const FTryllError& Error)
		{
			if (auto Live = Weak.Pin())
			{
				if (Error.IsOk() && ParamsClone.IsValid())
				{
					Live->SetNodeParamsBaseline(NodeName, ParamsClone.Get());
				}
				if (OnComplete)
				{
					OnComplete(Error);
				}
			}
		},
		[Weak, Aid](uint64 NotedRequestId)
		{
			if (auto Live = Weak.Pin(); Live && Live->Connection.IsValid())
			{
				Live->Variables->FlushIfDirty(nullptr, NotedRequestId);
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_ChangeAgentParamRequest;
				Note.RequestBody = Note.Body;
				Note.RequestId = NotedRequestId;
				Note.AgentId = Aid;
				Live->Connection->GetObserverHub().AnnotateNote(Note);
			}
		},
		bBusyIsExpected);
}

UTryllNodeParamsBase* FTryllAgent::GetNodeParamsBaseline(const FString& NodeName) const
{
	if (const TObjectPtr<UTryllNodeParamsBase>* Found = ParamsBaseline.Find(NodeName))
		return *Found;
	return nullptr;
}

void FTryllAgent::SetNodeParamsBaseline(const FString& NodeName, UTryllNodeParamsBase* Params)
{
	if (Params)
		ParamsBaseline.Add(NodeName, TObjectPtr<UTryllNodeParamsBase>(Params));
}

TArray<FString> FTryllAgent::GetNodeParamsBaselineNames() const
{
	TArray<FString> Names;
	ParamsBaseline.GetKeys(Names);
	return Names;
}

void FTryllAgent::ClearAfterDestroyRequest()
{
	AgentId = 0;
}

void FTryllAgent::DrainPendingLifecycleCallbacks(const FTryllError& TerminalError)
{
	if (Connection.IsValid())
	{
		Connection->FailOwnedBy(this, TerminalError);
	}
	ClearPausedToolBatch();
}
void FTryllAgent::SetOnAnswerText(TFunction<void(const FString&, const FString&, bool, bool)> Callback)
{
	OnAnswerText = MoveTemp(Callback);
}

void FTryllAgent::SetOnTurnComplete(TFunction<void(ETryllTurnStatus, const FString&, int32)> Callback)
{
	OnTurnComplete = MoveTemp(Callback);
}

void FTryllAgent::SetLocalizedTurnEnded(TFunction<void()> Callback)
{
	LocalizedTurnEnded = MoveTemp(Callback);
}

void FTryllAgent::SetOnError(TFunction<void(const FTryllError&)> Callback)
{
	OnError = MoveTemp(Callback);
}

void FTryllAgent::DispatchError(const FTryllError& Error) const
{
	if (OnError)
		OnError(Error);
}

void FTryllAgent::SetOnTtsAudioFormat(TFunction<void(const FString&, uint32, uint16, uint16)> Callback)
{
	OnTtsAudioFormat = MoveTemp(Callback);
}

void FTryllAgent::SetOnTtsAudio(TFunction<void(const FString&, TArrayView<const int16>)> Callback)
{
	OnTtsAudio = MoveTemp(Callback);
}

void FTryllAgent::SetOnTtsVisemes(TFunction<void(const FString&, TArrayView<const FTryllVisemeEvent>)> Callback)
{
	OnTtsVisemes = MoveTemp(Callback);
}

void FTryllAgent::HandleAnswerText(const FString& NodeName, const FString& Text, bool bIsDelta, bool bIsFinal)
{
	if (OnAnswerText)
	{
		OnAnswerText(NodeName, Text, bIsDelta, bIsFinal);
	}
}

void FTryllAgent::HandleTurnComplete(ETryllTurnStatus Status, const FString& DebugInfo, int32 TokensGenerated)
{
	PendingToolCalls.Reset();
	bPendingBatchObserved = false;
	PausedToolCalls.Reset();
	PausedToolResults.Reset();
	PausedNodeName.Reset();
	// Localized params that came back AgentBusy go first: a gameplay handler that
	// sends the next line from OnTurnComplete would otherwise keep the agent busy
	// and the language change would never land.
	if (LocalizedTurnEnded)
	{
		LocalizedTurnEnded();
	}
	if (OnTurnComplete)
	{
		OnTurnComplete(Status, DebugInfo, TokensGenerated);
	}
}

void FTryllAgent::HandleError(const FTryllError& Error)
{
	if (OnError)
	{
		OnError(Error);
	}
}

void FTryllAgent::HandleTtsAudioFormat(const FString& NodeName, uint32 SampleRate, uint16 Channels, uint16 BitsPerSample)
{
	if (OnTtsAudioFormat)
	{
		OnTtsAudioFormat(NodeName, SampleRate, Channels, BitsPerSample);
	}
}

void FTryllAgent::HandleTtsAudio(const FString& NodeName, TArrayView<const int16> Pcm)
{
	if (OnTtsAudio)
	{
		OnTtsAudio(NodeName, Pcm);
	}
}

void FTryllAgent::HandleTtsVisemes(const FString& NodeName, TArrayView<const FTryllVisemeEvent> Events)
{
	if (OnTtsVisemes)
	{
		OnTtsVisemes(NodeName, Events);
	}
}
