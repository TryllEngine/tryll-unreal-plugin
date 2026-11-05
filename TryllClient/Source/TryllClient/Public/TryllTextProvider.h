// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TryllTextProvider.generated.h"

class UTryllRuntimeSettings;

/**
 * Game-owned lookup for Text IDs on a workflow. Tryll does not own a string table.
 * The class is chosen in Project Settings → Tryll Client → Text Provider.
 * None means prompts stay inline and the Tryll Locale field is the session language.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew)
class TRYLLCLIENT_API UTryllTextProvider : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Text for Id in Table, in the provider's current language, after the
	 * provider's own fallback. False when the ID is unknown.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tryll|Text")
	bool GetText(FName Table, const FString& Id, FString& OutText) const;

	/** BCP-47 tag of the game's current language. Empty = unset. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tryll|Text")
	FString GetCurrentLocale() const;

	/**
	 * Editor preview. The string-table provider supports it. A Blueprint-only
	 * provider returns false and the details panel shows no preview.
	 */
	virtual bool SupportsEditorPreview() const { return false; }

	virtual TArray<FString> ListLocales() const { return {}; }
	virtual TArray<FString> ListIds(FName Table) const { return {}; }

	/** Exact locale, no fallback. False when that locale has no entry. */
	virtual bool GetTextForPreview(FName Table, const FString& Id, const FString& Locale, FString& OutText) const
	{
		return false;
	}

	/** Drop cached preview data (table edits, recompiled translations). */
	virtual void InvalidatePreviewCache() {}

	/** Jump to the table entry in the editor. False with OutMessage when the table has no editor. */
	virtual bool OpenInEditor(FName Table, const FString& Id, FString& OutMessage) const
	{
		OutMessage = TEXT("This text provider has no editor for its tables.");
		return false;
	}

	/** The instance from the project settings class, or null when the class is None. */
	static UTryllTextProvider* Get();

	/** Raised after the game's language changed. Game and editor subsystems both listen. */
	static FSimpleMulticastDelegate& OnLocaleChanged();

	static void BroadcastLocaleChanged();

	/**
	 * Session language for an empty CreateSession locale: the provider's current
	 * language when one is set, otherwise UTryllRuntimeSettings::Locale.
	 */
	static FString ResolveSessionLocale();

	/** The provider's current language, or empty when no provider is set. */
	static FString CurrentProviderLocale();

	/**
	 * Session language to send after a language change. False (send nothing) when
	 * there is no provider or CreateSession was given an explicit locale. An empty
	 * provider language falls back to UTryllRuntimeSettings::Locale, never "".
	 */
	static bool LocaleForLanguageChange(bool bSessionLocaleExplicit, FString& OutLocale);
};

/** Inspector preview for one Text ID. Shared by the editor and automation tests. */
struct TRYLLCLIENT_API FTryllTextPreview
{
	enum class EKind : uint8
	{
		Found,
		MissingHere,
		EmptyId,
		NoEditorSupport,
	};

	EKind Kind = EKind::NoEditorSupport;
	FString Text;
	bool bIsFallback = false;
	FString Message;
	TArray<FString> MissingLocales;

	static FTryllTextPreview Evaluate(
		const UTryllTextProvider* Provider, FName Table, const FString& Id, const FString& PreviewLocale);
};
