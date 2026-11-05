// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMetaHumanLipSyncPIEGuard.h"

#include "Editor.h"
#include "Features/IModularFeatures.h"
#include "Framework/Notifications/NotificationManager.h"
#include "TryllMetaHumanLipSyncSetup.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "TryllMetaHumanLipSync"

FTryllMetaHumanLipSyncPIEGuard FTryllMetaHumanLipSyncPIEGuard::Instance;

void FTryllMetaHumanLipSyncPIEGuard::Register()
{
	IModularFeatures::Get().RegisterModularFeature(IPIEAuthorizer::GetModularFeatureName(), &Instance);
}

void FTryllMetaHumanLipSyncPIEGuard::Unregister()
{
	IModularFeatures::Get().UnregisterModularFeature(IPIEAuthorizer::GetModularFeatureName(), &Instance);
}

bool FTryllMetaHumanLipSyncPIEGuard::RequestPIEPermissionImpl(FString& OutReason) const
{
	if (!GEditor)
	{
		return true;
	}

	if (TryllMetaHumanLipSyncSetup::IsSetUp())
	{
		return true;
	}

	const FText Message = LOCTEXT("PIEGuardDenied",
		"Tryll MetaHuman Lip Sync is not set up.\n"
		"Run Set Up in Project Settings > Plugins > Tryll MetaHuman Lip Sync, then press Play again.");
	FNotificationInfo Info(Message);
	Info.ExpireDuration = 8.0f;
	Info.bUseSuccessFailIcons = true;
	if (TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info))
	{
		Item->SetCompletionState(SNotificationItem::CS_Fail);
	}

	OutReason = TEXT("Tryll MetaHuman Lip Sync is not set up (Project Settings > Plugins > Tryll MetaHuman Lip Sync)");
	return false;
}

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8)
TValueOrError<bool, FText> FTryllMetaHumanLipSyncPIEGuard::IsPIEAuthorizedInternal(bool /*bIsSimulateInEditor*/) const
{
	// Keep the engine's silent query free of UI; the request path performs the check and shows the notification.
	return MakeValue(true);
}

TValueOrError<bool, FText> FTryllMetaHumanLipSyncPIEGuard::RequestPIEPermissionInternal(bool /*bIsSimulateInEditor*/) const
{
	FString Reason;
	if (RequestPIEPermissionImpl(Reason))
	{
		return MakeValue(true);
	}
	return MakeError(FText::FromString(Reason));
}
#else
bool FTryllMetaHumanLipSyncPIEGuard::RequestPIEPermission(bool /*bIsSimulateInEditor*/, FString& OutReason) const
{
	return RequestPIEPermissionImpl(OutReason);
}
#endif

#undef LOCTEXT_NAMESPACE
