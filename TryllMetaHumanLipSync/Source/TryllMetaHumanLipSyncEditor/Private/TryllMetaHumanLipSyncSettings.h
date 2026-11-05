// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TryllMetaHumanLipSyncSettings.generated.h"

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Tryll MetaHuman Lip Sync"))
class UTryllMetaHumanLipSyncSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	UPROPERTY(VisibleAnywhere, Transient, Category = "Setup")
	FString Status;

	/** Runs setup and reports the result as a notification. Called by the Set Up button (FTryllMetaHumanLipSyncSettingsCustomization). */
	void SetUpMetaHumanLipSync();

	void RefreshStatus();
};
