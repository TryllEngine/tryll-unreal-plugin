// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMetaHumanLipSyncSetupCommandlet.h"

#include "TryllMetaHumanLipSyncSetup.h"

DEFINE_LOG_CATEGORY_STATIC(LogTryllMetaHumanLipSyncSetup, Log, All);

int32 UTryllMetaHumanLipSyncSetupCommandlet::Main(const FString& Params)
{
	if (Params.Contains(TEXT("verify")))
	{
		const bool bSetUp = TryllMetaHumanLipSyncSetup::IsSetUp();
		UE_LOG(LogTryllMetaHumanLipSyncSetup, Display, TEXT("Verify: %s"), bSetUp ? TEXT("set up") : TEXT("NOT set up"));
		return bSetUp ? 0 : 1;
	}

	FText Message;
	const bool bOk = TryllMetaHumanLipSyncSetup::RunSetup(Message);
	UE_LOG(LogTryllMetaHumanLipSyncSetup, Display, TEXT("%s"), *Message.ToString());
	return bOk ? 0 : 1;
}
