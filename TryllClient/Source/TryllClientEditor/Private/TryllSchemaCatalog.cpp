// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllSchemaCatalog.h"

#include "TryllRuntimeSettings.h"
#include "TryllSubsystem.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

const FTryllSchemaCatalog& FTryllSchemaCatalog::Empty()
{
	static const FTryllSchemaCatalog Catalog;
	return Catalog;
}

bool FTryllSchemaCatalog::Contains(const FString& Name) const
{
	if (Name.IsEmpty())
	{
		return true;
	}
	return Names.Contains(Name);
}

FTryllSchemaCatalog FTryllSchemaCatalog::Parse(const FString& Json, const FString& InSourcePath)
{
	FTryllSchemaCatalog Catalog;
	Catalog.SourcePath = InSourcePath;
	if (Json.IsEmpty())
	{
		return Catalog;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return Catalog;
	}

	const TArray<TSharedPtr<FJsonValue>>* Schemas = nullptr;
	if (!Root->TryGetArrayField(TEXT("schemas"), Schemas) || Schemas == nullptr)
	{
		return Catalog;
	}

	TSet<FString> Seen;
	for (const TSharedPtr<FJsonValue>& Value : *Schemas)
	{
		const TSharedPtr<FJsonObject> Obj = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Obj.IsValid())
		{
			continue;
		}
		FString Name;
		if (!Obj->TryGetStringField(TEXT("name"), Name) || Name.IsEmpty())
		{
			continue;
		}
		if (!Seen.Contains(Name))
		{
			Seen.Add(Name);
			Catalog.Names.Add(Name);
		}
	}
	Catalog.Names.Sort();
	Catalog.bIsAvailable = true;
	return Catalog;
}

FTryllSchemaCatalog FTryllSchemaCatalog::LoadFromDirectory(const FString& ServerDir)
{
	if (ServerDir.IsEmpty())
	{
		return Empty();
	}
	const FString Path = FPaths::Combine(ServerDir, TEXT("data"), TEXT("schemas.json"));
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Path))
	{
		FTryllSchemaCatalog Catalog;
		Catalog.SourcePath = Path;
		return Catalog;
	}
	return Parse(Json, Path);
}

FTryllSchemaCatalog FTryllSchemaCatalog::LoadBundled(const UTryllRuntimeSettings* Settings)
{
	const UTryllRuntimeSettings* Resolved = Settings
		? Settings
		: GetDefault<UTryllRuntimeSettings>();
	const EServerBuildVariant Variant = Resolved
		? Resolved->ServerBuildVariant
		: EServerBuildVariant::Default;
	const FString Exe = UTryllSubsystem::ResolveServerExePath(Variant);
	if (Exe.IsEmpty())
	{
		return Empty();
	}
	return LoadFromDirectory(FPaths::GetPath(Exe));
}
