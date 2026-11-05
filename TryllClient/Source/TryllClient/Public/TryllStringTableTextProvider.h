// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllTextProvider.h"
#include "TryllStringTableTextProvider.generated.h"

class FTextLocalizationResource;

/**
 * Reads Unreal string tables (FText::FromStringTable) and follows the game culture.
 * Opt-in: pick it in Project Settings → Tryll Client → Text Provider.
 *
 * Smart-string formatting is not applied: the table text is sent as authored,
 * so `{}` is not nested inside Mustache `{{ }}`.
 *
 * A table that is not loaded yet is loaded on first use (blocking, game thread).
 * The language-change signal fires after Unreal has loaded the new language's
 * text (FTextLocalizationManager::OnTextRevisionChangedEvent), never before.
 */
UCLASS()
class TRYLLCLIENT_API UTryllStringTableTextProvider : public UTryllTextProvider
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

	virtual bool SupportsEditorPreview() const override { return true; }
	virtual TArray<FString> ListLocales() const override;
	virtual TArray<FString> ListIds(FName Table) const override;

	/**
	 * Exact text for Locale, read from that culture's compiled .locres files without
	 * changing the editor or game culture. The native culture reads the table source.
	 * False when Locale has no translation (or only a stale one) for the entry.
	 */
	virtual bool GetTextForPreview(
		FName Table, const FString& Id, const FString& Locale, FString& OutText) const override;

	virtual void InvalidatePreviewCache() override;
	virtual bool OpenInEditor(FName Table, const FString& Id, FString& OutMessage) const override;

	/** Find a registered string table, loading its asset when needed. Game thread only. */
	static bool FindOrLoadTable(FName TableId, FName& OutResolvedId);

protected:
	virtual bool GetText_Implementation(FName Table, const FString& Id, FString& OutText) const override;
	virtual FString GetCurrentLocale_Implementation() const override;

private:
	void HandleTextRevisionChanged();

	/** Translation of (Table, Id) in Culture, or false. Source text is compared by hash. */
	bool FindTranslation(FName ResolvedTable, const FString& Id, const FString& Source, const FString& Culture, FString& OutText) const;
	FString NativeCulture() const;

	FDelegateHandle RevisionHandle;
	FString LastNotifiedLocale;

	struct FCultureResources
	{
		TSharedPtr<FTextLocalizationResource> Resource;
		double LoadedAt = 0.0;
	};
	mutable TMap<FString, FCultureResources> PreviewResources;
	mutable TArray<FString> CachedLocales;
	mutable double CachedLocalesAt = -1.0;
};
