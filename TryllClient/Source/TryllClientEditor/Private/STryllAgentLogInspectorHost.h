// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AgentLog/AgentLogProjection.h"

class FAgentDescriptorRegistry;
class FAgentLogStore;
class STryllSentInspectorView;
class STryllSystemInspectorView;
class STryllTurnInspectorView;

/** Polymorphic inspector host for Agent Log rows (R12.5). */
class STryllAgentLogInspectorHost : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllAgentLogInspectorHost) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void RenderNone();
	void RenderRow(
		const FAgentLogRow&             Row,
		const FAgentLogStore&           Store,
		const FAgentDescriptorRegistry& Descriptors);

private:
	enum class EActiveInspector : uint8
	{
		None,
		Turn,
		Sent,
		System,
	};

	TSharedPtr<STryllTurnInspectorView>   TurnView;
	TSharedPtr<STryllSentInspectorView>   SentView;
	TSharedPtr<STryllSystemInspectorView> SystemView;
	TSharedPtr<class STryllAgentLogPayloadView> PayloadView;
	TSharedPtr<class SWidgetSwitcher>     Switcher;

	EActiveInspector ActiveInspector = EActiveInspector::None;
};
