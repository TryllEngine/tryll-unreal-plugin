// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "IPropertyTypeCustomization.h"

class UTryllClassifyAttributesLLMParams;

/**
 * Shows the ClassifyAttributeDef fields that belong to the current kind.
 * Boolean hides the choice menu. Score hides margin and the uncertain value.
 * Choice picks that value from its choice ids plus the reserved #uncertain.
 */
class FTryllClassifyAttributeDefCustomization : public IPropertyTypeCustomization
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
	void OnNameChanged();
	void RebuildUncertainOptions();

	TSharedPtr<IPropertyHandle> StructHandle;
	TSharedPtr<IPropertyHandle> KindHandle;
	TSharedPtr<IPropertyHandle> NameHandle;
	TSharedPtr<IPropertyHandle> UncertainHandle;
	TArray<TSharedPtr<FString>> UncertainOptions;
	TWeakObjectPtr<UTryllClassifyAttributesLLMParams> OwningParams;
	FString PreviousName;
};
