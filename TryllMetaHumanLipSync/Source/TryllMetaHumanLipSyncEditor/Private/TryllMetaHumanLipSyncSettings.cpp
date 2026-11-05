// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMetaHumanLipSyncSettings.h"

#include "Framework/Notifications/NotificationManager.h"
#include "TryllMetaHumanLipSyncSetup.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "TryllMetaHumanLipSync"

void UTryllMetaHumanLipSyncSettings::RefreshStatus()
{
	if (TryllMetaHumanLipSyncSetup::IsSetUp())
	{
		Status = TEXT("Set up.");
	}
	else
	{
		Status = TEXT("Not set up. Run Set Up.");
	}
}

void UTryllMetaHumanLipSyncSettings::SetUpMetaHumanLipSync()
{
	FText Message;
	const bool bOk = TryllMetaHumanLipSyncSetup::RunSetup(Message);
	RefreshStatus();

	FNotificationInfo Info(Message);
	Info.ExpireDuration = 6.0f;
	if (TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info))
	{
		Item->SetCompletionState(bOk ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}
}

#undef LOCTEXT_NAMESPACE
