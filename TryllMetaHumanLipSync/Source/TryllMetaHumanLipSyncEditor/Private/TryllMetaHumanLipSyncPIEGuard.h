// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "IPIEAuthorizer.h"
#include "Runtime/Launch/Resources/Version.h"

class FTryllMetaHumanLipSyncPIEGuard : public IPIEAuthorizer
{
public:
	static void Register();
	static void Unregister();

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8)
	virtual TValueOrError<bool, FText> IsPIEAuthorizedInternal(bool bIsSimulateInEditor) const override;
	virtual TValueOrError<bool, FText> RequestPIEPermissionInternal(bool bIsSimulateInEditor) const override;
#else
	virtual bool RequestPIEPermission(bool bIsSimulateInEditor, FString& OutReason) const override;
#endif

private:
	bool RequestPIEPermissionImpl(FString& OutReason) const;

	static FTryllMetaHumanLipSyncPIEGuard Instance;
};
