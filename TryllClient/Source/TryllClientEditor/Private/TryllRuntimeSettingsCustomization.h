// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"

/**
 * Details panel for Project Settings > Tryll Client.
 *
 * Jobs:
 *   - Hide the ServerBuildVariant dropdown unless both server variants exist.
 *   - Replace the Locale text field with a language picker whose entries carry
 *     a catalog model count ("German (de) - 7 models").
 *   - Custom models catalog path with a project-scoped file picker.
 *   - Reject StorageDataFolder under Content/ with a Move to TryllStorage action.
 *
 * The Locale row needs a real customization rather than the `GetOptions` meta it
 * also declares, because GetOptions writes the *selected string* into the
 * property: an annotated entry would store "German (de) - 7 models" as the
 * locale. This combo shows a label and stores the bare code, the same split
 * FTryllModelNameCustomization uses for model names.
 */
class FTryllRuntimeSettingsCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** Replace the Locale row with the annotated language picker. */
	void CustomizeLocaleRow(IDetailLayoutBuilder& DetailBuilder);

	/** Project-relative custom models catalog, with a file picker that stays inside the project. */
	void CustomizeUserModelsRow(IDetailLayoutBuilder& DetailBuilder);

	/** Error row + Move action when StorageDataFolder resolves under Content/. */
	void CustomizeStorageRow(IDetailLayoutBuilder& DetailBuilder);

	/** Rebuild Options from the registered models; called when the combo opens. */
	void RebuildLocaleOptions();

	/** Display text for a code: "German (de) - 7 models", or "(none ...)" for empty. */
	static FText LocaleOptionLabel(const FString& Code);

	/** Currently stored locale, read through the property handle. */
	FString CurrentLocale() const;

	/** Summary under the picker: registered voice models that cannot serve it. */
	FText LocaleSummary() const;

	TSharedPtr<IPropertyHandle>  LocaleHandle;
	TArray<TSharedPtr<FString>>  LocaleOptions;
};
