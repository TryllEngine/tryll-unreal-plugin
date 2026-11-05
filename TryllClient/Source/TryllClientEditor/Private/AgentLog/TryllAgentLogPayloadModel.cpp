// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "AgentLog/TryllAgentLogPayloadModel.h"

#include "Generated/TryllAgentLogJson.gen.h"
#include "TryllJsonWriter.h"

#include <utility>
#include <vector>

namespace
{

std::vector<FTryllOmittedField> OmittedOf(const FTryllObservationEnvelope& Envelope)
{
	std::vector<FTryllOmittedField> Items;
	Items.reserve(static_cast<size_t>(Envelope.Omitted.Num()));
	for (const FTryllObservationEnvelope::FOmittedField& Field : Envelope.Omitted)
	{
		const FTCHARToUTF8 Path(*Field.Path);
		FTryllOmittedField Item;
		Item.Path = Path.Get();
		Item.Count = Field.Count;
		Item.Bytes = Field.Bytes;
		Items.push_back(std::move(Item));
	}
	return Items;
}

} // namespace

FString FTryllAgentLogPayloadModel::WriteJson(const FTryllObservationEnvelope& Envelope, bool bIndent)
{
	if (Envelope.Frame.Num() == 0)
	{
		return FString();
	}
	const std::vector<FTryllOmittedField> Omitted = OmittedOf(Envelope);
	FTryllJsonWriter Writer(bIndent);
	if (Envelope.bIsLocal)
	{
		::Tryll::AgentLog::WriteLocal(Writer, Envelope.Frame.GetData(), static_cast<size_t>(Envelope.Frame.Num()), Omitted);
	}
	else
	{
		::Tryll::AgentLog::WriteMessage(Writer, Envelope.Frame.GetData(), static_cast<size_t>(Envelope.Frame.Num()), Omitted);
	}
	return FString(UTF8_TO_TCHAR(Writer.Utf8().c_str()));
}
