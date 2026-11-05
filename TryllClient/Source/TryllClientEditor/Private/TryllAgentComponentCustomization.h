// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "IDetailCustomization.h"
#include "Templates/SharedPointer.h"

class IDetailCategoryBuilder;
class IDetailGroup;
class IDetailLayoutBuilder;
class IPropertyHandle;
class IPropertyUtilities;
class UTryllAgentComponent;

/** Lifetime-safe status for async Flush Now callbacks (outlives customization). */
struct FTryllLiveVariableUIState : public TSharedFromThis<FTryllLiveVariableUIState>
{
	FString LiveStatus;
	bool bLiveStatusIsError = false;
};

/**
 * Declaration-driven Variable Overrides UX for UTryllAgentComponent (Material-
 * Instance pattern), plus Live Agent — Variables flush controls when a PIE or
 * editor-preview agent is attached.
 */
class FTryllAgentComponentCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	void BuildOverrideRows(IDetailGroup& VariablesGroup, UTryllAgentComponent* Component);
	void BuildStaleGroup(IDetailGroup& VariablesGroup, UTryllAgentComponent* Component);
	void BuildLiveAgentGroup(IDetailCategoryBuilder& TryllCategory, UTryllAgentComponent* Component);

	void ApplyRowLive(UTryllAgentComponent* Component, const FString& DeclName, bool bEnabled);

	TSharedPtr<IPropertyHandle> OverridesHandle;
	TSharedPtr<IPropertyUtilities> PropertyUtils;
	TSharedPtr<FTryllLiveVariableUIState> LiveUI;
};
