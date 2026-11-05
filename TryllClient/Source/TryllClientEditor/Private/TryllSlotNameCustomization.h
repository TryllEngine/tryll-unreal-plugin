// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "IPropertyTypeCustomization.h"

class UTryllNodeParamsBase;

/**
 * Renders FTryllSlotName (output_name) as a commit-on-text-committed
 * SEditableTextBox. Empty stays empty (meaning "use the node's own name" — the default).
 * On commit, a non-empty value is grammar/reserved-name checked and uniquified against
 * every other node name and effective slot name in the enclosing graph, then the change
 * in *effective* slot name (old override, or the node's own name when the override was
 * empty -> new override, or the node's own name when the new override is empty) is
 * propagated via TryllRenamePropagation::PropagateSlotRename.
 */
class FTryllSlotNameCustomization : public IPropertyTypeCustomization
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
	FString CurrentOverride() const;
	void    CommitRename(const FString& RequestedOverride);

	TSharedPtr<IPropertyHandle> ValueHandle;
	TWeakObjectPtr<UTryllNodeParamsBase> OwningParams;
};
