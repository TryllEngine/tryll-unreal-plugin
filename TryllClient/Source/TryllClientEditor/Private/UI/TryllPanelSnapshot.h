// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllEditorStatus.h"

/**
 * What a panel shows, read back for tests. Tests assert on this rather than
 * walking the widget tree, so a layout change does not break them. Mirrors
 * Unity's TryllEditorWindowSnapshot.
 */
struct FTryllPanelSnapshot
{
	ETryllOwnerPhase OwnerPhase = ETryllOwnerPhase::Idle;
	/** Panel-local state name (Idle, Connecting, …, Ready, Error). */
	FString LocalState;
	FString StatusCaption;
	/** Status-bar action button text ("Retry", …), empty when hidden. */
	FString StatusAction;
	/** Banner text, empty when cleared. */
	FString BannerText;
	/** Model names (Model Manager), component names (Memory), assistant replies (Chat). */
	TArray<FString> Rows;
	bool bLeaseHeld = false;
	int32 ListGeneration = 0;
};

/**
 * FTagMetaData tags on the widgets tests and automation touch. Engine-local;
 * not part of editor_ui.json.
 */
namespace TryllEditorWidgetIds
{
	inline const FName StatusCaption(TEXT("Tryll.StatusCaption"));
	inline const FName StatusAction(TEXT("Tryll.StatusAction"));
	inline const FName Banner(TEXT("Tryll.Banner"));
	inline const FName Refresh(TEXT("Tryll.Refresh"));
	inline const FName ChatStart(TEXT("Tryll.Chat.Start"));
	inline const FName ChatInput(TEXT("Tryll.Chat.Input"));
	inline const FName ChatSend(TEXT("Tryll.Chat.Send"));
}
