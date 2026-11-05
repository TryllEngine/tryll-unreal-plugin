// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "IPropertyTypeCustomization.h"

class UTryllNodeParamsBase;
class SWidget;
class FMenuBuilder;

/**
 * Renders FTryllSlotInput as a dropdown menu (SComboButton + FMenuBuilder) listing every
 * producer slot in the graph — no free-text mode:
 * <list type="bullet">
 *   <item>The node's default (empty value, labelled "User Message") — always enabled,
 *     always first.</item>
 *   <item>One "Slot X" entry per producer slot in the graph — enabled (available or
 *     branch-dependent, with a short "(...)" note) or disabled with a short inline note and
 *     a longer tooltip explaining why (TryllSlotGraphAnalysis::ClassifySlots).</item>
 * </list>
 * Entries are grouped: an "Available" section (default entry + enabled slots) followed by a
 * separator and an "Unavailable" section (disabled slots), so large graphs stay scannable.
 *
 * A non-empty current value that does not match any producer slot in the graph (stale after
 * a rename/rewire, or authored out-of-band) is surfaced as a disabled, tinted entry so it
 * stays visible; the button face is tinted and tooltipped in that case too. The default entry
 * and every enabled slot remain selectable, so the field is always escapable.
 */
class FTryllSlotInputCustomization : public IPropertyTypeCustomization
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
	FString CurrentValue() const;
	void    SetValue(const FString& InValue);
	FString LabelForValue(const FString& Value) const;

	/** Builds the dropdown menu contents on open (SComboButton::OnGetMenuContent). */
	TSharedRef<SWidget> BuildMenu();

	/** True when the current value is non-empty and not among the graph's enabled slots. */
	bool IsCurrentValueInvalid() const;

	/** Tooltip text for the button face when the current value is invalid; empty otherwise. */
	FText CurrentValueTooltip() const;

	TSharedPtr<IPropertyHandle>          ValueHandle;
	TWeakObjectPtr<UTryllNodeParamsBase> OwningParams;
};
