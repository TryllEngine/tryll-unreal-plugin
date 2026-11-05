// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllAgentLogInspectorHost.h"

#include "AgentLog/AgentDescriptor.h"
#include "AgentLog/AgentLogStore.h"
#include "TryllAgentLogInspection.h"
#include "STryllSentInspectorView.h"
#include "STryllSystemInspectorView.h"
#include "STryllTurnInspectorView.h"
#include "TryllObservationTypes.h"
#include "TryllTurnInspectorModels.h"
#include "AgentLog/TryllAgentLogPayloadModel.h"
#include "AgentLog/TryllAgentLogPayloadView.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"

void STryllAgentLogInspectorHost::Construct(const FArguments& /*InArgs*/)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SAssignNew(Switcher, SWidgetSwitcher)
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(TurnView, STryllTurnInspectorView)
			]
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(SentView, STryllSentInspectorView)
			]
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(SystemView, STryllSystemInspectorView)
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(PayloadView, STryllAgentLogPayloadView)
		]
	];

	RenderNone();
}

void STryllAgentLogInspectorHost::RenderNone()
{
	ActiveInspector = EActiveInspector::None;
	Switcher->SetActiveWidgetIndex(0);
	TurnView->Render(FTryllTurnInspectionFactory::Empty(TEXT("select a row")));
	if (PayloadView.IsValid())
	{
		PayloadView->Clear();
	}
}

void STryllAgentLogInspectorHost::RenderRow(
	const FAgentLogRow&             Row,
	const FAgentLogStore&           Store,
	const FAgentDescriptorRegistry& Descriptors)
{
	const FTryllObservationEnvelope* Envelope = Store.FindEnvelope(Row.EventId);
	if (Envelope == nullptr && Row.Kind != EAgentLogRowKind::Received)
	{
		RenderNone();
		return;
	}

	switch (Row.Kind)
	{
	case EAgentLogRowKind::Received:
	{
		ActiveInspector = EActiveInspector::Turn;
		Switcher->SetActiveWidgetIndex(0);
		TurnView->Render(FTryllAgentLogInspection::BuildTurnInspection(Row, Store, Descriptors));
		break;
	}
	case EAgentLogRowKind::Sent:
	{
		ActiveInspector = EActiveInspector::Sent;
		Switcher->SetActiveWidgetIndex(1);
		SentView->Render(FTryllAgentLogInspection::BuildSentInspection(Row, *Envelope));
		break;
	}
	case EAgentLogRowKind::System:
	case EAgentLogRowKind::CaptureGap:
	default:
	{
		ActiveInspector = EActiveInspector::System;
		Switcher->SetActiveWidgetIndex(2);
		const FString RawJson = (Envelope != nullptr && Envelope->Frame.Num() > 0)
			? FTryllAgentLogPayloadModel::WriteJson(*Envelope, true)
			: FString();
		SystemView->Render(FTryllAgentLogInspection::BuildSystemInspection(Row, *Envelope), RawJson);
		if (PayloadView.IsValid())
		{
			PayloadView->Clear();
		}
		return;
	}
	}

	if (PayloadView.IsValid() && Envelope != nullptr && Envelope->Frame.Num() > 0)
	{
		PayloadView->SetSource(*Envelope);
	}
	else if (PayloadView.IsValid())
	{
		PayloadView->Clear();
	}
}
