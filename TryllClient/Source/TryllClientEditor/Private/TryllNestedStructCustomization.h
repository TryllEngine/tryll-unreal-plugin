// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "IPropertyTypeCustomization.h"

class IDetailChildrenBuilder;
class IDetailPropertyRow;
class IPropertyHandle;

/**
 * Adds one child of a details row. A string tagged meta=(MultiLine="true")
 * is drawn as a text area. The stock details row keeps those strings on one
 * line once the owning struct is an element of an array, which is how choice
 * prompts and attribute system prompts are authored.
 */
namespace TryllMultilineProperty
{
	/** Returns the added row (null for an invalid handle), e.g. to bind its visibility. */
	IDetailPropertyRow* Add(IDetailChildrenBuilder& ChildBuilder, const TSharedPtr<IPropertyHandle>& Handle);
}

/**
 * Expands a nested params struct and routes each field through
 * TryllMultilineProperty::Add. Registered for the structs that have no other
 * customization (choice rows, intent entries, tool definitions).
 */
class FTryllNestedStructCustomization : public IPropertyTypeCustomization
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
};
