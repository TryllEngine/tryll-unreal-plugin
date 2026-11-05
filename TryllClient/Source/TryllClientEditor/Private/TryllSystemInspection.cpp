// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllSystemInspection.h"

#include "TryllAgentLogInspection.h"

namespace SystemInspectionPrivate
{

FString BuildDetail(const FTryllObservationEnvelope& Envelope)
{
	TArray<FString> Lines;

	if (!Envelope.NodeName.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Node: %s"), *Envelope.NodeName));
	}
	if (!Envelope.Text.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Text: %s"), *Envelope.Text));
	}
	if (!Envelope.Message.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Message: %s"), *Envelope.Message));
	}
	if (!Envelope.Text2.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Text2: %s"), *Envelope.Text2));
	}
	if (Envelope.VariableUpdateCount > 0)
	{
		Lines.Add(FString::Printf(TEXT("Variables: %d"), Envelope.VariableUpdateCount));
	}
	if (Envelope.DialogInteractionCount > 0)
	{
		Lines.Add(FString::Printf(
			TEXT("Dialog interactions: %d"), Envelope.DialogInteractionCount));
	}
	if (Envelope.RemoveInteractionCount > 0)
	{
		Lines.Add(FString::Printf(
			TEXT("Removed interactions: %u"), Envelope.RemoveInteractionCount));
	}
	if (Envelope.BinaryByteCount > 0)
	{
		Lines.Add(FString::Printf(
			TEXT("Binary: %u bytes (%s)"), Envelope.BinaryByteCount, *Envelope.BinaryKind));
	}
	if (Envelope.TokensGenerated > 0)
	{
		Lines.Add(FString::Printf(TEXT("Tokens: %u"), Envelope.TokensGenerated));
	}
	if (Envelope.TranscriptKind != 0)
	{
		Lines.Add(FString::Printf(TEXT("Transcript kind: %u"), Envelope.TranscriptKind));
	}
	if (Envelope.AudioMsConsumed > 0)
	{
		Lines.Add(FString::Printf(TEXT("Audio consumed: %u ms"), Envelope.AudioMsConsumed));
	}

	return FString::Join(Lines, TEXT("\n"));
}

FString BuildAckSummary(const FTryllObservationEnvelope& Envelope)
{
	const FString Label = FTryllAgentLogInspection::FormatWireLabel(Envelope);
	if (Envelope.IsReplyTo(::Tryll::MessageBody_ChangeAgentParamRequest, true)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_ResumeAgentRequest, true)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_UpdateAgentVariablesRequest, true)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_GetAgentVariablesRequest, true)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_PrefillAgentKvCacheRequest, true)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_EvictAgentKvCacheRequest, true)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_GetAgentKvCacheStatusRequest, true)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_AppendInteractionsRequest, true)
		|| Envelope.IsReplyTo(::Tryll::MessageBody_RemoveInteractionsFromEndRequest, true))
	{
		return FString::Printf(TEXT("Ack: %s"), *Label);
	}
	if (Envelope.Code != 0 || !Envelope.Message.IsEmpty())
	{
		return FString::Printf(TEXT("%s — code %u: %s"), *Label, Envelope.Code, *Envelope.Message);
	}
	return Label;
}

} // namespace SystemInspectionPrivate

FTryllSystemInspection FTryllSystemInspectionFactory::Empty(const FString& Hint)
{
	FTryllSystemInspection System;
	System.Title       = TEXT("System");
	System.Hint        = Hint;
	System.AbsenceNote = Hint.IsEmpty() ? TEXT("select a row") : Hint;
	return System;
}

FTryllSystemInspection FTryllSystemInspectionFactory::FromCaptureGap(const FTryllObservationEnvelope& Envelope)
{
	FTryllSystemInspection System;
	System.bIsCaptureGap = true;
	System.Title         = TEXT("Capture gap");
	System.EventLabel    = TEXT("Capture resumed");
	System.Detail        = TEXT(
		"The Agent Log was detached or the editor session ended. "
		"Events during the gap were not recorded.");
	System.Timestamp     = Envelope.WallTimeUtc;
	System.Hint          = FTryllAgentLogInspection::FormatTimestamp(System.Timestamp);
	System.AbsenceNote   = System.Detail;
	return System;
}

FTryllSystemInspection FTryllSystemInspectionFactory::FromAgentLogRow(const FTryllObservationEnvelope& Envelope)
{
	using namespace SystemInspectionPrivate;

	if (Envelope.bIsCaptureGap)
	{
		return FromCaptureGap(Envelope);
	}

	FTryllSystemInspection System;
	System.AgentId     = Envelope.AgentId;
	System.RequestId   = Envelope.RequestId;
	System.Timestamp   = Envelope.WallTimeUtc;

	System.EventLabel = FTryllAgentLogInspection::FormatWireLabel(Envelope);
	System.Title      = System.EventLabel;
	if (Envelope.Direction != ETryllObservationDirection::Outbound
		|| Envelope.Connection != ETryllConnectionObservation::None)
	{
		System.AckSummary = BuildAckSummary(Envelope);
	}

	System.Detail = BuildDetail(Envelope);
	System.Hint   = FString::Printf(
		TEXT("agent %llu · req %llu · %s"),
		System.AgentId,
		System.RequestId,
		*FTryllAgentLogInspection::FormatTimestamp(System.Timestamp));

	if (System.Detail.IsEmpty() && System.AckSummary.IsEmpty())
	{
		System.AbsenceNote = TEXT(
			"This system row has no extra fields beyond its timeline summary.");
	}

	return System;
}

