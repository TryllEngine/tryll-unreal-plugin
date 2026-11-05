// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllModelCatalogFile.h"

#include "TryllClientEditorModule.h"
#include "TryllRuntimeSettings.h"
#include "TryllSubsystem.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

TArray<FString>      FTryllModelCatalogFile::CachedPaths;
TArray<FDateTime>    FTryllModelCatalogFile::CachedStamps;
TMap<FString, int32> FTryllModelCatalogFile::Counts;
TMap<FString, FTryllTtsCapabilities> FTryllModelCatalogFile::TtsCaps;
TMap<FString, FString> FTryllModelCatalogFile::LicenceUrls;
int32                FTryllModelCatalogFile::TotalModels = 0;
bool                 FTryllModelCatalogFile::bParsed     = false;

namespace
{

/** models.json sits beside the bundled server exe the plugin would launch. */
FString ResolveBaseCatalogPath()
{
	const FString ExePath = UTryllSubsystem::ResolveServerExePath(EServerBuildVariant::Default);
	if (ExePath.IsEmpty())
	{
		return FString();
	}
	return FPaths::Combine(FPaths::GetPath(ExePath), TEXT("data"), TEXT("models.json"));
}

FString ResolvePayloadModelsD()
{
	const FString ExePath = UTryllSubsystem::ResolveServerExePath(EServerBuildVariant::Default);
	if (ExePath.IsEmpty())
	{
		return FString();
	}
	return FPaths::Combine(FPaths::GetPath(ExePath), TEXT("data"), TEXT("models.d"));
}

void AppendSortedJsonFiles(TArray<FString>& OutPaths, const FString& Dir)
{
	if (Dir.IsEmpty() || !FPaths::DirectoryExists(Dir))
	{
		return;
	}
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.json")), true, false);
	Files.Sort();
	for (const FString& Name : Files)
	{
		OutPaths.Add(FPaths::Combine(Dir, Name));
	}
}

TArray<FString> ResolveLayerPaths()
{
	TArray<FString> Paths;
	const FString Base = ResolveBaseCatalogPath();
	if (Base.IsEmpty() || !FPaths::FileExists(Base))
	{
		return Paths;
	}
	Paths.Add(Base);
	if (const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>())
	{
		const FString UserCatalog = Settings->ResolveUserModelsCatalogForLaunch();
		if (!UserCatalog.IsEmpty() && FPaths::FileExists(UserCatalog))
		{
			Paths.Add(UserCatalog);
		}
	}
	AppendSortedJsonFiles(Paths, ResolvePayloadModelsD());
	return Paths;
}

bool MergeModelsFromFile(
	const FString& Path,
	TMap<FString, TSharedPtr<FJsonObject>>& ModelsByName,
	bool bIsBase)
{
	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *Path))
	{
		if (bIsBase)
		{
			UE_LOG(LogTryllEditor, Warning,
				TEXT("[Tryll] Could not read the model catalog at '%s'."), *Path);
		}
		else
		{
			UE_LOG(LogTryllEditor, Warning,
				TEXT("[Tryll] [user catalog] '%s' could not be read and was skipped."), *Path);
		}
		return bIsBase;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		if (bIsBase)
		{
			UE_LOG(LogTryllEditor, Warning,
				TEXT("[Tryll] Model catalog at '%s' is not valid JSON."), *Path);
			return false;
		}
		UE_LOG(LogTryllEditor, Warning,
			TEXT("[Tryll] [user catalog] '%s' is not valid JSON and was skipped."), *Path);
		return true;
	}

	const TArray<TSharedPtr<FJsonValue>>* Models = nullptr;
	if (!Root->TryGetArrayField(TEXT("models"), Models) || !Models)
	{
		return true;
	}

	for (const TSharedPtr<FJsonValue>& ModelValue : *Models)
	{
		const TSharedPtr<FJsonObject>* Model = nullptr;
		if (!ModelValue.IsValid() || !ModelValue->TryGetObject(Model) || !Model)
		{
			continue;
		}
		FString Name;
		if (!(*Model)->TryGetStringField(TEXT("name"), Name) || Name.IsEmpty())
		{
			continue;
		}
		if (ModelsByName.Contains(Name))
		{
			if (!bIsBase)
			{
				UE_LOG(LogTryllEditor, Warning,
					TEXT("[Tryll] [user catalog] '%s' defines model '%s', which an earlier "
						 "catalog already defines. The earlier/base entry wins."),
					*Path, *Name);
			}
			continue;
		}
		ModelsByName.Add(Name, *Model);
	}
	return true;
}

FTryllTtsCapabilities ParseTtsCapabilities(const FJsonObject& Caps)
{
	FTryllTtsCapabilities Out;
	Caps.TryGetBoolField(TEXT("voice_description"), Out.bVoiceDescription);
	Caps.TryGetBoolField(TEXT("delivery_instruction"), Out.bDeliveryInstruction);
	Caps.TryGetBoolField(TEXT("reference_voice"), Out.bReferenceVoice);
	Caps.TryGetBoolField(TEXT("speed"), Out.bSpeed);
	Caps.TryGetBoolField(TEXT("language_per_call"), Out.bLanguagePerCall);
	Caps.TryGetBoolField(TEXT("expressiveness"), Out.bExpressiveness);
	Caps.TryGetNumberField(TEXT("num_speakers"), Out.NumSpeakers);
	const TArray<TSharedPtr<FJsonValue>>* Presets = nullptr;
	if (Caps.TryGetArrayField(TEXT("voice_presets"), Presets) && Presets)
	{
		for (const TSharedPtr<FJsonValue>& Value : *Presets)
		{
			const TSharedPtr<FJsonObject>* Preset = nullptr;
			if (!Value.IsValid() || !Value->TryGetObject(Preset) || !Preset)
			{
				continue;
			}
			FTryllTtsVoicePreset P;
			(*Preset)->TryGetStringField(TEXT("id"), P.Id);
			(*Preset)->TryGetStringField(TEXT("label"), P.Label);
			(*Preset)->TryGetStringField(TEXT("description"), P.Description);
			Out.VoicePresets.Add(MoveTemp(P));
		}
	}
	return Out;
}

} // namespace

void FTryllModelCatalogFile::Refresh()
{
	const TArray<FString> Paths = ResolveLayerPaths();
	if (Paths.Num() == 0)
	{
		CachedPaths.Reset();
		CachedStamps.Reset();
		Counts.Reset();
		TtsCaps.Reset();
		LicenceUrls.Reset();
		TotalModels = 0;
		bParsed     = false;
		return;
	}

	bool bUnchanged = bParsed && CachedPaths.Num() == Paths.Num();
	if (bUnchanged)
	{
		for (int32 i = 0; i < Paths.Num(); ++i)
		{
			if (Paths[i] != CachedPaths[i]
				|| IFileManager::Get().GetTimeStamp(*Paths[i]) != CachedStamps[i])
			{
				bUnchanged = false;
				break;
			}
		}
	}
	if (bUnchanged)
	{
		return;
	}

	CachedPaths  = Paths;
	CachedStamps.Reset();
	for (const FString& Path : Paths)
	{
		CachedStamps.Add(IFileManager::Get().GetTimeStamp(*Path));
	}
	Counts.Reset();
	TtsCaps.Reset();
	LicenceUrls.Reset();
	TotalModels = 0;
	bParsed     = false;

	TMap<FString, TSharedPtr<FJsonObject>> ModelsByName;
	for (int32 i = 0; i < Paths.Num(); ++i)
	{
		if (!MergeModelsFromFile(Paths[i], ModelsByName, /*bIsBase=*/i == 0) && i == 0)
		{
			return;
		}
	}

	for (const auto& Pair : ModelsByName)
	{
		const TSharedPtr<FJsonObject>& Model = Pair.Value;
		const TArray<TSharedPtr<FJsonValue>>* Variants = nullptr;
		if (!Model.IsValid() || !Model->TryGetArrayField(TEXT("variants"), Variants) || !Variants)
		{
			continue;
		}

		TSet<FString> Declared;
		for (const TSharedPtr<FJsonValue>& VariantValue : *Variants)
		{
			const TSharedPtr<FJsonObject>* Variant = nullptr;
			if (!VariantValue.IsValid() || !VariantValue->TryGetObject(Variant) || !Variant)
			{
				continue;
			}

			const TSharedPtr<FJsonObject>* Caps = nullptr;
			if (!TtsCaps.Contains(Pair.Key)
				&& (*Variant)->TryGetObjectField(TEXT("tts_capabilities"), Caps) && Caps && Caps->IsValid())
			{
				TtsCaps.Add(Pair.Key, ParseTtsCapabilities(**Caps));
			}

			FString LicenceUrl;
			if ((*Variant)->TryGetStringField(TEXT("licence_url"), LicenceUrl) && !LicenceUrl.IsEmpty())
			{
				if (!LicenceUrls.Contains(LicenceKey(Pair.Key, FString())))
				{
					LicenceUrls.Add(LicenceKey(Pair.Key, FString()), LicenceUrl);
				}
				FString Engine;
				if ((*Variant)->TryGetStringField(TEXT("engine"), Engine) && !Engine.IsEmpty()
					&& !LicenceUrls.Contains(LicenceKey(Pair.Key, Engine)))
				{
					LicenceUrls.Add(LicenceKey(Pair.Key, Engine), LicenceUrl);
				}
			}

			const TArray<TSharedPtr<FJsonValue>>* Languages = nullptr;
			if (!(*Variant)->TryGetArrayField(TEXT("languages"), Languages) || !Languages)
			{
				continue;
			}

			for (const TSharedPtr<FJsonValue>& LangValue : *Languages)
			{
				FString Code;
				if (LangValue.IsValid() && LangValue->TryGetString(Code) && !Code.IsEmpty())
				{
					Declared.Add(Code.ToLower());
				}
			}
		}

		for (const FString& Code : Declared)
		{
			Counts.FindOrAdd(Code)++;
		}
	}

	TotalModels = ModelsByName.Num();
	bParsed     = true;
}

const TMap<FString, int32>& FTryllModelCatalogFile::CountsByLanguage()
{
	Refresh();
	return Counts;
}

int32 FTryllModelCatalogFile::ModelCount()
{
	Refresh();
	return TotalModels;
}

bool FTryllModelCatalogFile::IsAvailable()
{
	Refresh();
	return bParsed;
}

const FTryllTtsCapabilities* FTryllModelCatalogFile::FindTtsCapabilities(const FString& ModelName)
{
	Refresh();
	return ModelName.IsEmpty() ? nullptr : TtsCaps.Find(ModelName);
}

FString FTryllModelCatalogFile::FindLicenceUrl(const FString& ModelName, const FString& Engine)
{
	Refresh();
	if (ModelName.IsEmpty())
	{
		return FString();
	}
	if (!Engine.IsEmpty())
	{
		if (const FString* Url = LicenceUrls.Find(LicenceKey(ModelName, Engine)))
		{
			return *Url;
		}
	}
	const FString* Url = LicenceUrls.Find(LicenceKey(ModelName, FString()));
	return Url ? *Url : FString();
}

FString FTryllModelCatalogFile::LicenceKey(const FString& ModelName, const FString& Engine)
{
	return ModelName + TEXT("|") + Engine;
}
