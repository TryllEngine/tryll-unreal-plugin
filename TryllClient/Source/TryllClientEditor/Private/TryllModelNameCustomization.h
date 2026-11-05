// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "IPropertyTypeCustomization.h"

/**
 * Renders FTryllModelName as a type-filtered dropdown of the project's registered
 * models (from TryllModelManifest) plus the selected model's registration tier.
 * The dropdown is filtered by the property's meta=(TryllModelType="...") tag.
 */
class FTryllModelNameCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	void    RebuildOptions();
	FString CurrentValue() const;
	void    SetValue(const FString& InValue);

	TSharedPtr<IPropertyHandle> NameHandle;
	FString                     ModelType;

	/**
	 * Warning shown next to the picker when the chosen voice model cannot serve the
	 * project's selected language; empty when there is nothing to say.
	 */
	FText LanguageWarning() const;
	TArray<TSharedPtr<FString>> Options;
};
