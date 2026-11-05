// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllAgentLogInspection.h"

#include "AgentLog/AgentLogStore.h"
#include "messages_generated.h"
#include "TryllChatTurnState.h"
#include "TryllGraphDescription.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace AgentLogInspectionPrivate
{

FString TurnKeyString(const FTryllTurnKey& Key)
{
	if (!Key.bHasValue)
	{
		return TEXT("turn:orphan");
	}
	if (Key.RequestId != 0)
	{
		return FString::Printf(
			TEXT("turn:%llu:%llu:%llu"),
			Key.ConnectionEpoch,
			Key.AgentId,
			Key.RequestId);
	}
	return FString::Printf(
		TEXT("turn:%llu:%llu:synthetic:%llu"),
		Key.ConnectionEpoch,
		Key.AgentId,
		Key.SyntheticSequence);
}

FString TruncateForRow(const FString& Text, const int32 MaxChars = 96)
{
	FString Flat = Text;
	Flat.ReplaceInline(TEXT("\r"), TEXT(" "));
	Flat.ReplaceInline(TEXT("\n"), TEXT(" "));
	if (Flat.Len() <= MaxChars)
	{
		return Flat;
	}
	return Flat.Left(MaxChars - 1) + TEXT("…");
}

} // namespace AgentLogInspectionPrivate

FString FTryllAgentLogInspection::FormatWireLabel(const FTryllObservationEnvelope& Envelope)
{
	switch (Envelope.Connection)
	{
	case ETryllConnectionObservation::Connected:     return TEXT("Connected");
	case ETryllConnectionObservation::ConnectFailed: return TEXT("ConnectFailed");
	case ETryllConnectionObservation::Disconnected:  return TEXT("Disconnected");
	default: break;
	}
	if (const char* Name = ::Tryll::EnumNameMessageBody(Envelope.Body))
	{
		return UTF8_TO_TCHAR(Name);
	}
	return TEXT("Unknown");
}
FString FTryllAgentLogInspection::FormatTimestamp(const FDateTime& WallTimeUtc)
{
	if (WallTimeUtc.GetTicks() == 0)
	{
		return TEXT("--:--:--.---");
	}
	// FDateTime::ToString has no millisecond token — format explicitly.
	return FString::Printf(
		TEXT("%02i:%02i:%02i.%03i"),
		WallTimeUtc.GetHour(),
		WallTimeUtc.GetMinute(),
		WallTimeUtc.GetSecond(),
		WallTimeUtc.GetMillisecond());
}

FString FTryllAgentLogInspection::FormatAgentGutterLabel(
	const uint64 AgentId,
	const FAgentDescriptorRegistry& Descriptors)
{
	if (const FAgentDescriptor* Descriptor = Descriptors.Find(AgentId))
	{
		return Descriptor->Label;
	}
	return FString::Printf(TEXT("Agent %llu"), AgentId);
}

namespace
{

FString FormatTurnCount(const int32 CompletedTurns)
{
	if (CompletedTurns <= 0)
	{
		return TEXT("no turns");
	}
	return CompletedTurns == 1
		? TEXT("1 turn")
		: FString::Printf(TEXT("%d turns"), CompletedTurns);
}

} // namespace

FString FTryllAgentLogInspection::FormatAgentListStatus(
	const int32 CompletedTurns,
	const bool bDestroyed,
	const bool bLimitedDiagnostics,
	const bool bPending)
{
	if (bPending)
	{
		return TEXT("creating…");
	}

	TArray<FString> Parts;
	Parts.Add(FormatTurnCount(CompletedTurns));
	if (bDestroyed)
	{
		Parts.Add(TEXT("destroyed"));
	}
	if (bLimitedDiagnostics)
	{
		Parts.Add(TEXT("limited diag"));
	}
	return FString::Join(Parts, TEXT(" · "));
}

FString FTryllAgentLogInspection::FormatAllAgentsStatus(
	const int32 AgentCount,
	const int32 CompletedTurns)
{
	if (AgentCount <= 0)
	{
		return TEXT("no agents");
	}

	const FString Agents = AgentCount == 1
		? TEXT("1 agent")
		: FString::Printf(TEXT("%d agents"), AgentCount);
	if (CompletedTurns <= 0)
	{
		return Agents;
	}
	return Agents + TEXT(" · ") + FormatTurnCount(CompletedTurns);
}

FString FTryllAgentLogInspection::RowSummary(
	const FAgentLogRow&                  Row,
	const FTryllObservationEnvelope&     Envelope,
	const FAgentDescriptorRegistry&      Descriptors)
{
	using namespace AgentLogInspectionPrivate;

	if (Row.bIsCaptureGap || Envelope.bIsCaptureGap)
	{
		return TEXT("Capture gap — events during detach were not recorded");
	}

	if (Row.bVoiceSentFinal)
	{
		return TruncateForRow(Envelope.Text.IsEmpty() ? TEXT("Voice sent") : Envelope.Text);
	}
	if (Row.Kind == EAgentLogRowKind::Sent)
	{
		const FTryllSentInspection Sent = BuildSentInspection(Row, Envelope);
		FString Summary = Sent.PayloadSummary;
		if (Row.DeferredVariableUpdates.Num() > 0)
		{
			Summary += TEXT(" · +deferred vars");
		}
		return TruncateForRow(Summary);
	}

	if (Row.Kind == EAgentLogRowKind::Received)
	{
		if (Envelope.IsPush(::Tryll::MessageBody_AnswerText))
		{
			return TruncateForRow(Envelope.Text.IsEmpty() ? TEXT("(empty answer chunk)") : Envelope.Text);
		}
		if (Envelope.IsPush(::Tryll::MessageBody_TurnComplete))
		{
			const ETryllTurnStatus Status = static_cast<ETryllTurnStatus>(Envelope.TurnStatus);
			if (Status == ETryllTurnStatus::Cancelled)
			{
				return TEXT("Turn complete — cancelled");
			}
			if (Status == ETryllTurnStatus::Error)
			{
				return TEXT("Turn complete — error");
			}
			return TEXT("Turn complete");
		}
		if (Envelope.IsReplyTo(::Tryll::MessageBody_SendMessageRequest, false))
		{
			return TruncateForRow(
				Envelope.Message.IsEmpty() ? TEXT("Send error") : Envelope.Message);
		}
		if (Envelope.IsPush(::Tryll::MessageBody_NodeEvent))
		{
			return TruncateForRow(FString::Printf(
				TEXT("Node %s · %s"),
				*Envelope.NodeName,
				*Envelope.Message));
		}
		if (Envelope.IsPush(::Tryll::MessageBody_TtsAudioFormatFrame))
		{
			return FString::Printf(TEXT("TTS format %u Hz"), Envelope.SampleRate);
		}
		if (Envelope.IsPush(::Tryll::MessageBody_TtsAudioFrame))
		{
			return FString::Printf(TEXT("TTS audio (%u bytes)"), Envelope.BinaryByteCount);
		}
		if (Envelope.IsPush(::Tryll::MessageBody_TtsVisemeFrame))
		{
			return FString::Printf(TEXT("TTS visemes (%u events)"), Envelope.BinaryByteCount);
		}
		return FormatWireLabel(Envelope);
	}

	const FTryllSystemInspection System = BuildSystemInspection(Row, Envelope);
	FString Summary = System.EventLabel;
	if (!Envelope.GraphDefaultModelName.IsEmpty())
	{
		// Unity Summarize(CreateAgent): "CreateAgent (model)".
		Summary += TEXT(" (") + Envelope.GraphDefaultModelName + TEXT(")");
	}
	else if (!Envelope.Text.IsEmpty())
	{
		Summary += TEXT(" · ") + TruncateForRow(Envelope.Text, 64);
	}
	else if (!Envelope.NodeName.IsEmpty())
	{
		Summary += TEXT(" · ") + Envelope.NodeName;
	}
	else if (!Envelope.Message.IsEmpty())
	{
		Summary += TEXT(" · ") + TruncateForRow(Envelope.Message, 64);
	}
	return Summary;
}

bool FTryllAgentLogInspection::RowUsesDoubleHeight(const FString& Summary)
{
	return Summary.Len() > 72 || Summary.Contains(TEXT("\n"));
}

FTryllTurnInspection FTryllAgentLogInspection::BuildTurnInspection(
	const FAgentLogRow&              Row,
	const FAgentLogStore&            Store,
	const FAgentDescriptorRegistry&  Descriptors)
{
	using namespace AgentLogInspectionPrivate;

	if (!Row.TurnKey.bHasValue)
	{
		return FTryllTurnInspectionFactory::Empty(TEXT("no turn key on this row"));
	}

	FString AnswerText;
	FString ErrorText;
	FString DebugInfoRaw;
	TSharedPtr<FJsonObject> DebugInfo;
	ETryllTurnStatus TurnStatus = ETryllTurnStatus::Success;
	bool bFromVoice = false;
	bool bHasTerminal = false;

	Store.ForEachEnvelope(
		[&](const FTryllObservationEnvelope& Envelope)
		{
			if (!(Envelope.TurnKey == Row.TurnKey))
			{
				return;
			}

			if (Envelope.IsRequest(::Tryll::MessageBody_SendMessageRequest))
			{
				bFromVoice = Envelope.VoiceInputId != 0;
				return;
			}
			if (Envelope.Direction == ETryllObservationDirection::Outbound
				&& Envelope.Connection == ETryllConnectionObservation::None)
			{
				return;
			}

			if (Envelope.IsPush(::Tryll::MessageBody_AnswerText))
			{
				if (Envelope.bFlag)
				{
					AnswerText += Envelope.Text;
				}
				else
				{
					AnswerText = Envelope.Text;
				}
			}
			else if (Envelope.IsPush(::Tryll::MessageBody_TurnComplete))
			{
				bHasTerminal = true;
				TurnStatus = static_cast<ETryllTurnStatus>(Envelope.TurnStatus);
				DebugInfoRaw = Envelope.Text;
			}
			else if (Envelope.IsReplyTo(::Tryll::MessageBody_SendMessageRequest, false))
			{
				bHasTerminal = true;
				TurnStatus = ETryllTurnStatus::Error;
				ErrorText = Envelope.Message;
			}
		});

	if (!bHasTerminal)
	{
		FTryllTurnInspection Turn = FTryllTurnInspectionFactory::Empty(TEXT("turn still in flight"));
		Turn.TurnKey    = TurnKeyString(Row.TurnKey);
		Turn.AnswerText = AnswerText;
		return Turn;
	}

	if (!DebugInfoRaw.IsEmpty())
	{
		TSharedPtr<FJsonObject> Parsed;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(DebugInfoRaw);
		if (FJsonSerializer::Deserialize(Reader, Parsed) && Parsed.IsValid())
		{
			DebugInfo = Parsed;
		}
	}

	ETryllDiagnosticsAbsenceReason FallbackAbsence = ETryllDiagnosticsAbsenceReason::Empty;
	if (const FAgentDescriptor* Descriptor = Descriptors.Find(Row.AgentId))
	{
		if (Descriptor->bCreatedBeforeDiagnosticsCapture)
		{
			FallbackAbsence = ETryllDiagnosticsAbsenceReason::CreatedBeforeDiagnosticsCapture;
		}
		else if (!Descriptor->bDiagnosticsAvailable)
		{
			FallbackAbsence = ETryllDiagnosticsAbsenceReason::Unavailable;
		}
	}

	const FString Hint = FString::Printf(
		TEXT("req %llu%s"),
		Row.TurnKey.RequestId,
		bFromVoice ? TEXT(" · voice") : TEXT(""));

	const TArray<FTryllTurnRouteNode> Route = FTryllChatTurnLogic::ExtractRoute(DebugInfo);
	if (ErrorText.IsEmpty() && TurnStatus == ETryllTurnStatus::Error)
	{
		ErrorText = FTryllChatTurnLogic::ResolveErrorText(TurnStatus, DebugInfo, ErrorText);
	}

	return FTryllTurnInspectionFactory::FromChatTurn(
		TurnKeyString(Row.TurnKey),
		Hint,
		AnswerText,
		ErrorText,
		FTryllChatTurnLogic::ToTerminalStatus(TurnStatus),
		Route,
		0,
		0,
		DebugInfo,
		DebugInfoRaw,
		FallbackAbsence);
}

FTryllSentInspection FTryllAgentLogInspection::BuildSentInspection(
	const FAgentLogRow& Row,
	const FTryllObservationEnvelope& Envelope)
{
	return FTryllSentInspectionFactory::FromAgentLogRow(
		Envelope,
		Row.DeferredVariableUpdates,
		Row.JoinedAck);
}

FTryllSystemInspection FTryllAgentLogInspection::BuildSystemInspection(
	const FAgentLogRow& Row,
	const FTryllObservationEnvelope& Envelope)
{
	if (Row.bIsCaptureGap || Envelope.bIsCaptureGap)
	{
		return FTryllSystemInspectionFactory::FromCaptureGap(Envelope);
	}
	return FTryllSystemInspectionFactory::FromAgentLogRow(Envelope);
}

bool FTryllAgentLogInspection::InspectionNeverBlank(const FTryllTurnInspection& Turn)
{
	if (!Turn.Hint.IsEmpty())
	{
		return true;
	}
	if (!Turn.AnswerText.IsEmpty() || !Turn.ErrorText.IsEmpty())
	{
		return true;
	}
	if (Turn.DebugInfo.IsValid())
	{
		return true;
	}
	if (!Turn.RawDebugInfoText.IsEmpty())
	{
		return true;
	}
	return Turn.AbsenceReason != ETryllDiagnosticsAbsenceReason::None;
}

bool FTryllAgentLogInspection::InspectionNeverBlank(const FTryllSentInspection& Sent)
{
	return !Sent.Title.IsEmpty()
		|| !Sent.PayloadSummary.IsEmpty()
		|| !Sent.AbsenceNote.IsEmpty()
		|| Sent.DeferredVariableBatches.Num() > 0
		|| Sent.JoinedAck.IsSet();
}

bool FTryllAgentLogInspection::InspectionNeverBlank(const FTryllSystemInspection& System)
{
	return !System.Title.IsEmpty()
		|| !System.EventLabel.IsEmpty()
		|| !System.Detail.IsEmpty()
		|| !System.AckSummary.IsEmpty()
		|| !System.AbsenceNote.IsEmpty();
}

FAgentLogFilter FTryllAgentLogInspection::MakeAllAgentsFilter(const bool bRawMode)
{
	FAgentLogFilter Filter;
	Filter.bAllAgents = true;
	Filter.bRawMode   = bRawMode;
	return Filter;
}

FAgentLogFilter FTryllAgentLogInspection::MakeSingleAgentFilter(const uint64 AgentId, const bool bRawMode)
{
	FAgentLogFilter Filter;
	Filter.bAllAgents    = false;
	Filter.bRawMode      = bRawMode;
	Filter.SingleAgentId = AgentId;
	return Filter;
}

