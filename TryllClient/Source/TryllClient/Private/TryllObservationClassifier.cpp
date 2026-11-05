// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllObservationClassifier.h"
#include "TryllObservationCurated.h"
#include "Generated/TryllMessageDescriptors.gen.h"

namespace
{

bool HasAgentScope(::Tryll::MessageBody Body)
{
	if (Body == ::Tryll::MessageBody_NONE)
	{
		return false;
	}
	const ::Tryll::FTryllMessageDescriptor* Descriptor = ::Tryll::MessageDescriptor(Body);
	return Descriptor && Descriptor->ScopeKey && FCStringAnsi::Strcmp(Descriptor->ScopeKey, "agent_id") == 0;
}

bool IsMetadata(::Tryll::MessageBody Body)
{
	return Body == ::Tryll::MessageBody_AudioBuffer
		|| Body == ::Tryll::MessageBody_SetInferenceThrottle;
}

} // namespace

ETryllObservationScope FTryllObservationClassifier::Classify(const FTryllObservationNote& Note)
{
	if (Note.Connection != ETryllConnectionObservation::None)
	{
		return ETryllObservationScope::RawSession;
	}

	if (TryllObservationIsCuratedOverride(Note.Body))
	{
		return ETryllObservationScope::CuratedAgent;
	}

	const bool bAnswersRequest = Note.Direction == ETryllObservationDirection::Inbound
		&& Note.RequestBody != ::Tryll::MessageBody_NONE
		&& Note.RequestBody != Note.Body;

	if (bAnswersRequest)
	{
		if (Note.RequestBody == ::Tryll::MessageBody_CreateAgentRequest
			&& TryllObservationIsCuratedOverride(Note.RequestBody))
		{
			return ETryllObservationScope::CuratedAgent;
		}
		if (HasAgentScope(Note.RequestBody))
		{
			return ETryllObservationScope::CuratedAgent;
		}
	}
	else if (HasAgentScope(Note.Body))
	{
		return ETryllObservationScope::CuratedAgent;
	}

	if (IsMetadata(Note.Body) || IsMetadata(Note.RequestBody))
	{
		return ETryllObservationScope::MetadataOnly;
	}

	return ETryllObservationScope::RawSession;
}
