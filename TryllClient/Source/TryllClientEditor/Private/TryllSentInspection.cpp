// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllSentInspection.h"
#include "TryllAgentLogInspection.h"

namespace SentInspectionPrivate
{

FString DescribeDeferredBatch(const FTryllObservationEnvelope& Envelope)
{
	if (!Envelope.PayloadSummary.IsEmpty())
	{
		return Envelope.PayloadSummary;
	}
	if (Envelope.VariableUpdateCount > 0)
	{
		return FString::Printf(
			TEXT("%d variable update(s) on carrier req %llu"),
			Envelope.VariableUpdateCount,
			Envelope.CarrierRequestId);
	}
	return TEXT("deferred variable flush");
}

ETryllSentPayloadForm ClassifyForm(const FTryllObservationEnvelope& Envelope)
{
	if (Envelope.IsRequest(::Tryll::MessageBody_SendMessageRequest))
	{
		return ETryllSentPayloadForm::TypedText;
	}
	if (Envelope.IsRequest(::Tryll::MessageBody_ResumeAgentRequest))
	{
		return ETryllSentPayloadForm::Resume;
	}
	if (Envelope.IsRequest(::Tryll::MessageBody_ChangeAgentParamRequest))
	{
		return ETryllSentPayloadForm::ChangeParams;
	}
	return ETryllSentPayloadForm::OtherCommand;
}

FString FormLabel(const ETryllSentPayloadForm Form)
{
	switch (Form)
	{
	case ETryllSentPayloadForm::TypedText:       return TEXT("typed text");
	case ETryllSentPayloadForm::ClientInjected:  return TEXT("client injected");
	case ETryllSentPayloadForm::Resume:          return TEXT("resume");
	case ETryllSentPayloadForm::ChangeParams:    return TEXT("change params");
	default:                                     return TEXT("command");
	}
}

FString BuildPayloadSummary(const FTryllObservationEnvelope& Envelope, const ETryllSentPayloadForm Form)
{
	switch (Form)
	{
	case ETryllSentPayloadForm::TypedText:
		return Envelope.Text.IsEmpty() ? TEXT("(empty message)") : Envelope.Text;
	case ETryllSentPayloadForm::Resume:
		return Envelope.Text.IsEmpty() ? TEXT("(default resume)") : Envelope.Text;
	case ETryllSentPayloadForm::ChangeParams:
		return Envelope.NodeName.IsEmpty() ? TEXT("(node params)") : Envelope.NodeName;
	default:
		break;
	}

	if (!Envelope.Text.IsEmpty())
	{
		return Envelope.Text;
	}
	if (!Envelope.NodeName.IsEmpty())
	{
		return Envelope.NodeName;
	}
	if (!Envelope.Message.IsEmpty())
	{
		return Envelope.Message;
	}
	if (Envelope.VariableUpdateCount > 0)
	{
		return FString::Printf(TEXT("%d variable update(s)"), Envelope.VariableUpdateCount);
	}
	if (Envelope.DialogInteractionCount > 0)
	{
		return FString::Printf(TEXT("%d interaction(s)"), Envelope.DialogInteractionCount);
	}
	if (Envelope.BinaryByteCount > 0)
	{
		return FString::Printf(TEXT("%u bytes %s"), Envelope.BinaryByteCount, *Envelope.BinaryKind);
	}
	if (!Envelope.GraphDefaultModelName.IsEmpty())
	{
		return Envelope.GraphDefaultModelName;
	}
	// Prefer the command label over an opaque placeholder when Kind was unexpected
	// for a Sent row (or BeginUtterance / Cancel with no text fields).
	const FString CmdLabel = FTryllAgentLogInspection::FormatWireLabel(Envelope);
	return CmdLabel.IsEmpty() ? TEXT("(no primary payload field captured)") : CmdLabel;
}

FString BuildPayloadDetail(const FTryllObservationEnvelope& Envelope, const ETryllSentPayloadForm Form)
{
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("Form: %s"), *FormLabel(Form)));

	if (Envelope.VoiceInputId != 0)
	{
		Lines.Add(FString::Printf(TEXT("Voice input id: %llu"), Envelope.VoiceInputId));
	}
	if (!Envelope.Text2.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Text2: %s"), *Envelope.Text2));
	}
	if (!Envelope.Message.IsEmpty() && Envelope.Message != Envelope.Text)
	{
		Lines.Add(FString::Printf(TEXT("Message: %s"), *Envelope.Message));
	}
	if (Envelope.RemoveInteractionCount > 0)
	{
		Lines.Add(FString::Printf(
			TEXT("Remove interactions: %u"), Envelope.RemoveInteractionCount));
	}
	if (Envelope.ThrottleLevel > 0.f)
	{
		Lines.Add(FString::Printf(TEXT("Throttle: %.2f"), Envelope.ThrottleLevel));
	}
	if (Envelope.SampleRate > 0)
	{
		Lines.Add(FString::Printf(
			TEXT("Audio format: %u Hz · %u ch · %u-bit"),
			Envelope.SampleRate,
			Envelope.Channels,
			Envelope.BitsPerSample));
	}

	return FString::Join(Lines, TEXT("\n"));
}

} // namespace SentInspectionPrivate

FTryllSentInspection FTryllSentInspectionFactory::Empty(const FString& Hint)
{
	FTryllSentInspection Sent;
	Sent.Title       = TEXT("Sent");
	Sent.Hint        = Hint;
	Sent.AbsenceNote = Hint.IsEmpty() ? TEXT("select a row") : Hint;
	return Sent;
}

FTryllSentInspection FTryllSentInspectionFactory::FromAgentLogRow(
	const FTryllObservationEnvelope&           Envelope,
	const TArray<FTryllObservationEnvelope>&    DeferredUpdates,
	const TOptional<FTryllObservationEnvelope>& JoinedAck)
{
	using namespace SentInspectionPrivate;

	FTryllSentInspection Sent;
	Sent.CommandType  = Envelope.Body;
	Sent.CommandLabel = FTryllAgentLogInspection::FormatWireLabel(Envelope);
	Sent.Form         = ClassifyForm(Envelope);
	Sent.RequestId    = Envelope.RequestId;
	Sent.AgentId      = Envelope.AgentId;
	Sent.Timestamp    = Envelope.WallTimeUtc;
	Sent.JoinedAck    = JoinedAck;

	Sent.Title = Sent.CommandLabel;
	Sent.Hint  = FString::Printf(
		TEXT("req %llu · agent %llu · %s"),
		Sent.RequestId,
		Sent.AgentId,
		*FTryllAgentLogInspection::FormatTimestamp(Sent.Timestamp));

	Sent.PayloadSummary = BuildPayloadSummary(Envelope, Sent.Form);
	Sent.PayloadDetail  = BuildPayloadDetail(Envelope, Sent.Form);
	Sent.bHasDetailBeyondRow = !Sent.PayloadDetail.IsEmpty()
		|| DeferredUpdates.Num() > 0
		|| JoinedAck.IsSet();

	for (const FTryllObservationEnvelope& Deferred : DeferredUpdates)
	{
		FTryllDeferredVariableBatch Batch;
		Batch.UpdateCount      = Deferred.VariableUpdateCount;
		Batch.CarrierRequestId = Deferred.CarrierRequestId != 0
			? Deferred.CarrierRequestId
			: Envelope.RequestId;
		Batch.Summary = DescribeDeferredBatch(Deferred);
		Sent.DeferredVariableBatches.Add(MoveTemp(Batch));
	}

	if (!Sent.bHasDetailBeyondRow)
	{
		Sent.AbsenceNote = TEXT(
			"This sent row carries only what the timeline already shows — "
			"the command label and primary payload field.");
	}

	return Sent;
}

