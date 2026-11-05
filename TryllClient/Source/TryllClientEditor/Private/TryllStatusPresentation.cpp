// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllStatusPresentation.h"
#include "TryllEditorUiContract.h"

FTryllStatusPresentation FTryllStatusPresentation::Compose(
	const ETryllOwnerPhase OwnerPhase,
	const FString& OwnerDetail,
	const ETryllPanelOperation LocalOp,
	const ETryllEditorPanel Panel,
	const int32 ModelCount)
{
	FTryllStatusPresentation Out;

	// Attach-only Agent Log never starts a session; Idle is a wait, not a start offer.
	if (Panel == ETryllEditorPanel::AgentLog && OwnerPhase == ETryllOwnerPhase::Idle)
	{
		Out.Severity = ETryllStatusSeverity::Neutral;
		Out.Caption = FTryllEditorUiContract::StatusAgentLogIdle;
		return Out;
	}

	if (OwnerPhase == ETryllOwnerPhase::Error)
	{
		Out.Severity = ETryllStatusSeverity::Error;
		Out.Caption = OwnerDetail.IsEmpty()
			? FString(FTryllEditorUiContract::StatusError)
			: OwnerDetail;
		Out.Detail = OwnerDetail;
		Out.Action = ETryllStatusActionIntent::Retry;
		return Out;
	}

	if (LocalOp != ETryllPanelOperation::None)
	{
		Out.Severity = ETryllStatusSeverity::Progress;
		switch (LocalOp)
		{
		case ETryllPanelOperation::CreatingAgent:
			Out.Caption = FTryllEditorUiContract::StatusCreatingAgent;
			break;
		case ETryllPanelOperation::CreatingVoiceInput:
			Out.Caption = FTryllEditorUiContract::StatusCreatingVoiceInput;
			break;
		case ETryllPanelOperation::Refreshing:
			Out.Caption = FTryllEditorUiContract::StatusRefreshing;
			break;
		case ETryllPanelOperation::Listing:
			Out.Caption = FTryllEditorUiContract::StatusListing;
			break;
		case ETryllPanelOperation::Waiting:
			Out.Severity = ETryllStatusSeverity::Neutral;
			Out.Caption = FTryllEditorUiContract::StatusWaiting;
			break;
		case ETryllPanelOperation::Capturing:
			Out.Caption = FTryllEditorUiContract::StatusCapturing;
			break;
		default:
			Out.Severity = ETryllStatusSeverity::Neutral;
			Out.Caption = FTryllEditorUiContract::StatusIdle;
			break;
		}
		return Out;
	}

	switch (OwnerPhase)
	{
	case ETryllOwnerPhase::Connecting:
		Out.Severity = ETryllStatusSeverity::Progress;
		Out.Caption = OwnerDetail.IsEmpty()
			? FString(FTryllEditorUiContract::StatusConnecting)
			: OwnerDetail;
		break;
	case ETryllOwnerPhase::Configuring:
		Out.Severity = ETryllStatusSeverity::Progress;
		Out.Caption = OwnerDetail.IsEmpty()
			? FString(FTryllEditorUiContract::StatusConfiguring)
			: OwnerDetail;
		break;
	case ETryllOwnerPhase::Ready:
		Out.Severity = ETryllStatusSeverity::Success;
		Out.Caption = Panel == ETryllEditorPanel::ModelManager
			? FTryllEditorUiContract::FormatReadyModels(ModelCount)
			: FString(FTryllEditorUiContract::StatusReady);
		break;
	default:
		Out.Severity = ETryllStatusSeverity::Neutral;
		Out.Caption = FTryllEditorUiContract::StatusIdle;
		break;
	}
	return Out;
}
