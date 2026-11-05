// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabVariantCodec.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TryllAgentComponent.h"

FString FTryllDialogLabVariantCodec::Encode(const FTryllDialogLabVariant& Variant)
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("label"), Variant.Label);
	Root->SetNumberField(TEXT("r"), Variant.Swatch.R);
	Root->SetNumberField(TEXT("g"), Variant.Swatch.G);
	Root->SetNumberField(TEXT("b"), Variant.Swatch.B);
	Root->SetStringField(TEXT("model"), Variant.ModelOverride);
	// A path, not a pointer: an ini cannot hold a live UObject, and the scene may not be loaded
	// when this is read back.
	Root->SetStringField(TEXT("agent"),
		Variant.AgentOverride.IsValid()
			? Variant.AgentOverride->GetPathName()
			: Variant.AgentOverrideId);
	Root->SetBoolField(TEXT("use_live_tools"), Variant.bUseLiveTools);

	TArray<TSharedPtr<FJsonValue>> Overrides;
	for (const FTryllLabParamOverride& Override : Variant.Overrides)
	{
		TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("node"),  Override.NodeName);
		Entry->SetStringField(TEXT("field"), Override.FieldPath);
		Entry->SetStringField(TEXT("value"), Override.Value);
		Overrides.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Root->SetArrayField(TEXT("overrides"), Overrides);

	FString Out;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

bool FTryllDialogLabVariantCodec::Decode(const FString& Line, FTryllDialogLabVariant& OutVariant)
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Line);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	OutVariant.Label         = Root->GetStringField(TEXT("label"));
	OutVariant.ModelOverride = Root->GetStringField(TEXT("model"));
	// Absent in variants persisted before the opt-in, and false is what those runs used.
	// Set explicitly so a decode over a used target cannot keep a stale true.
	bool bUseLiveTools = false;
	Root->TryGetBoolField(TEXT("use_live_tools"), bUseLiveTools);
	OutVariant.bUseLiveTools = bUseLiveTools;
	OutVariant.Swatch = FLinearColor(
		static_cast<float>(Root->GetNumberField(TEXT("r"))),
		static_cast<float>(Root->GetNumberField(TEXT("g"))),
		static_cast<float>(Root->GetNumberField(TEXT("b"))));

	const FString AgentPath = Root->GetStringField(TEXT("agent"));
	OutVariant.AgentOverrideId = AgentPath;
	if (AgentPath.IsEmpty())
	{
		OutVariant.AgentOverride.Reset();
	}
	else
	{
		// A component that no longer exists restores as null, which the Variants tab already
		// reports as "→ no default assigned" rather than failing at run time.
		OutVariant.AgentOverride = FindObject<UTryllAgentComponent>(nullptr, *AgentPath);
	}

	OutVariant.Overrides.Reset();
	const TArray<TSharedPtr<FJsonValue>>* Overrides = nullptr;
	if (Root->TryGetArrayField(TEXT("overrides"), Overrides))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Overrides)
		{
			const TSharedPtr<FJsonObject>* Entry = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Entry))
			{
				continue;
			}
			FTryllLabParamOverride Override;
			Override.NodeName  = (*Entry)->GetStringField(TEXT("node"));
			Override.FieldPath = (*Entry)->GetStringField(TEXT("field"));
			Override.Value     = (*Entry)->GetStringField(TEXT("value"));
			OutVariant.Overrides.Add(Override);
		}
	}
	return true;
}
