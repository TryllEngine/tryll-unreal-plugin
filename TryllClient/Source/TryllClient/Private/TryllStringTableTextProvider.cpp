// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllStringTableTextProvider.h"

#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/LocalizedTextSourceTypes.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Internationalization/TextKey.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Internationalization/TextLocalizationResource.h"
#include "Misc/App.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Internationalization/StringTable.h"
#include "Subsystems/AssetEditorSubsystem.h"
#endif

namespace
{
	/** Compiled translations and the culture list change rarely; re-read them at most this often. */
	constexpr double PreviewCacheSeconds = 5.0;

	FStringTableConstPtr FindLoadedTable(FName TableId)
	{
		return TableId.IsNone() ? nullptr : FStringTableRegistry::Get().FindStringTable(TableId);
	}

	bool SourceOf(FName TableId, const FString& Id, FName& OutResolved, FString& OutSource)
	{
		if (Id.IsEmpty() || !UTryllStringTableTextProvider::FindOrLoadTable(TableId, OutResolved))
		{
			return false;
		}
		FStringTableConstPtr Table = FindLoadedTable(OutResolved);
		return Table.IsValid() && Table->GetSourceString(FTextKey(Id), OutSource);
	}
}

bool UTryllStringTableTextProvider::FindOrLoadTable(FName TableId, FName& OutResolvedId)
{
	OutResolvedId = TableId;
	if (TableId.IsNone())
	{
		return false;
	}
	if (FindLoadedTable(TableId).IsValid())
	{
		return true;
	}

	// String-table assets register themselves only once loaded. A prompts table that
	// nothing else references is not loaded yet, so load it here (blocking).
	if (IsInGameThread() && IStringTableEngineBridge::CanFindOrLoadStringTableAsset())
	{
		FName Loaded = TableId;
		IStringTableEngineBridge::FullyLoadStringTableAsset(Loaded);
		if (FindLoadedTable(Loaded).IsValid())
		{
			OutResolvedId = Loaded;
			return true;
		}
	}
	return false;
}

void UTryllStringTableTextProvider::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	LastNotifiedLocale = GetCurrentLocale();

	// OnCultureChanged fires before Unreal loads the new language's text, so a read
	// there returns the old language. The revision event fires after the load.
	TWeakObjectPtr<UTryllStringTableTextProvider> Weak(this);
	RevisionHandle = FTextLocalizationManager::Get().OnTextRevisionChangedEvent.AddLambda([Weak]()
	{
		if (UTryllStringTableTextProvider* Self = Weak.Get())
		{
			Self->HandleTextRevisionChanged();
		}
	});
}

void UTryllStringTableTextProvider::BeginDestroy()
{
	if (RevisionHandle.IsValid() && !IsEngineExitRequested())
	{
		FTextLocalizationManager::Get().OnTextRevisionChangedEvent.Remove(RevisionHandle);
	}
	RevisionHandle.Reset();
	Super::BeginDestroy();
}

void UTryllStringTableTextProvider::HandleTextRevisionChanged()
{
	InvalidatePreviewCache();

	// The revision also changes for reasons other than a language switch (hot reload,
	// a new table). Only a changed culture is a language change.
	const FString Now = GetCurrentLocale();
	if (Now.Equals(LastNotifiedLocale, ESearchCase::IgnoreCase))
	{
		return;
	}
	LastNotifiedLocale = Now;

	// A provider replaced in Project Settings stays alive until the next GC; it must not
	// speak for the project any more.
	if (UTryllTextProvider::Get() != this)
	{
		return;
	}
	BroadcastLocaleChanged();
}

FString UTryllStringTableTextProvider::NativeCulture() const
{
	FString Native = FTextLocalizationManager::Get().GetNativeCultureName(ELocalizedTextSourceCategory::Game);
	if (Native.IsEmpty())
	{
		// No localization target: the table source is the only language there is.
		Native = GetCurrentLocale();
	}
	return Native;
}

bool UTryllStringTableTextProvider::FindTranslation(
	FName ResolvedTable, const FString& Id, const FString& Source, const FString& Culture, FString& OutText) const
{
	FStringTableConstPtr Table = FindLoadedTable(ResolvedTable);
	if (!Table.IsValid() || Culture.IsEmpty())
	{
		return false;
	}

	const double Now = FPlatformTime::Seconds();
	FCultureResources& Cached = PreviewResources.FindOrAdd(Culture);
	if (!Cached.Resource.IsValid() || Now - Cached.LoadedAt > PreviewCacheSeconds)
	{
		Cached.Resource = MakeShared<FTextLocalizationResource>();
		for (const FString& Path : FPaths::GetGameLocalizationPaths())
		{
			Cached.Resource->LoadFromDirectory(Path / Culture, 0);
		}
		Cached.LoadedAt = Now;
	}

	const FTextId TextId(FTextKey(Table->GetNamespace()), FTextKey(Id));
	const FTextLocalizationResource::FEntry* Entry = Cached.Resource->Entries.Find(TextId);
	if (Entry == nullptr || !Entry->LocalizedString.IsValid() || Entry->LocalizedString->IsEmpty())
	{
		return false;
	}
	// A translation made for an older source text is not used by Unreal either.
	if (Entry->SourceStringHash != 0 && Entry->SourceStringHash != FTextLocalizationResource::HashString(Source))
	{
		return false;
	}
	OutText = *Entry->LocalizedString;
	return true;
}

bool UTryllStringTableTextProvider::GetText_Implementation(
	FName Table, const FString& Id, FString& OutText) const
{
	OutText.Reset();
	FName Resolved;
	FString Source;
	if (!SourceOf(Table, Id, Resolved, Source))
	{
		return false;
	}

#if WITH_EDITOR
	if (GIsEditor && !FApp::IsGame())
	{
		// Outside play the editor loads no game translations, so FText would show the
		// source for every culture. Read the compiled translation, then fall back the
		// way Unreal does (more specific culture first, then the source).
		const FString Native = NativeCulture();
		for (const FString& Culture : FInternationalization::Get().GetPrioritizedCultureNames(GetCurrentLocale()))
		{
			if (Culture.Equals(Native, ESearchCase::IgnoreCase))
			{
				break;
			}
			if (FindTranslation(Resolved, Id, Source, Culture, OutText))
			{
				return true;
			}
		}
		OutText = Source;
		return !OutText.IsEmpty();
	}
#endif

	OutText = FText::FromStringTable(Resolved, Id).ToString();
	return !OutText.IsEmpty();
}

FString UTryllStringTableTextProvider::GetCurrentLocale_Implementation() const
{
	const FCulturePtr Culture = FInternationalization::Get().GetCurrentCulture();
	return Culture.IsValid() ? Culture->GetName() : FString();
}

TArray<FString> UTryllStringTableTextProvider::ListLocales() const
{
	const double Now = FPlatformTime::Seconds();
	if (CachedLocalesAt >= 0.0 && Now - CachedLocalesAt <= PreviewCacheSeconds)
	{
		return CachedLocales;
	}

	// GetLocalizedCultureNames scans the localization folders on disk; cache it.
	TArray<FString> Cultures = FTextLocalizationManager::Get().GetLocalizedCultureNames(ELocalizationLoadFlags::Game);
	const FString Native = NativeCulture();
	if (!Native.IsEmpty())
	{
		Cultures.AddUnique(Native);
	}
	const FString Current = GetCurrentLocale();
	if (!Current.IsEmpty())
	{
		Cultures.AddUnique(Current);
	}
	Cultures.Sort();
	CachedLocales = Cultures;
	CachedLocalesAt = Now;
	return CachedLocales;
}

TArray<FString> UTryllStringTableTextProvider::ListIds(FName TableId) const
{
	TArray<FString> Ids;
	FName Resolved;
	if (!FindOrLoadTable(TableId, Resolved))
	{
		return Ids;
	}
	FStringTableConstPtr Table = FindLoadedTable(Resolved);
	if (!Table.IsValid())
	{
		return Ids;
	}
	Table->EnumerateSourceStrings([&Ids](const FString& Key, const FString& /*Source*/)
	{
		Ids.Add(Key);
		return true;
	});
	Ids.Sort();
	return Ids;
}

bool UTryllStringTableTextProvider::GetTextForPreview(
	FName Table, const FString& Id, const FString& Locale, FString& OutText) const
{
	OutText.Reset();
	FName Resolved;
	FString Source;
	if (!SourceOf(Table, Id, Resolved, Source))
	{
		return false;
	}

	// The native culture is the table source. Every other culture needs its own
	// translation: Unreal's display falls back to the source, which would hide a
	// missing translation.
	if (Locale.IsEmpty() || Locale.Equals(NativeCulture(), ESearchCase::IgnoreCase))
	{
		OutText = Source;
		return !OutText.IsEmpty();
	}
	return FindTranslation(Resolved, Id, Source, Locale, OutText);
}

void UTryllStringTableTextProvider::InvalidatePreviewCache()
{
	PreviewResources.Reset();
	CachedLocales.Reset();
	CachedLocalesAt = -1.0;
}

bool UTryllStringTableTextProvider::OpenInEditor(FName Table, const FString& /*Id*/, FString& OutMessage) const
{
#if WITH_EDITOR
	FName Resolved;
	if (!FindOrLoadTable(Table, Resolved))
	{
		OutMessage = FString::Printf(TEXT("String table '%s' was not found."), *Table.ToString());
		return false;
	}
	UStringTable* Asset = FStringTableRegistry::Get().FindStringTableAsset(Resolved);
	if (Asset == nullptr)
	{
		OutMessage = FString::Printf(
			TEXT("String table '%s' is registered from code or a CSV file, not an asset. Edit its CSV file."),
			*Resolved.ToString());
		return false;
	}
	if (GEditor == nullptr)
	{
		OutMessage = TEXT("No editor is running.");
		return false;
	}
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
	return true;
#else
	OutMessage = TEXT("Tables open only in the editor.");
	return false;
#endif
}
