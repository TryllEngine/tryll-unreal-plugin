// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "IPropertyTypeCustomization.h"

/**
 * Replaces the `Name` row of FTryllNodeDescription with a commit-on-text-committed
 * SEditableTextBox: on commit it enforces the identifier grammar and reserved-name
 * ("user_message") rule, uniquifies against existing node names, and propagates the
 * rename into every exit field, StartNode, and (when this node's effective slot was
 * its own name) every `input` selector and `{{slot.<old>}}` template tag in the
 * enclosing FTryllGraphDescription — see TryllRenamePropagation. The `Params` child
 * is drawn by FTryllLocalizablePreview::AddParamsObject, which hides the inline text of
 * a ticked Localized field like the graph editor does.
 */
class FTryllNodeDescriptionCustomization : public IPropertyTypeCustomization
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
	FString CurrentName() const;
	void    CommitRename(const FString& RequestedName);

	TSharedPtr<IPropertyHandle> NameHandle;
	TSharedPtr<IPropertyHandle> ParamsHandle;
};
