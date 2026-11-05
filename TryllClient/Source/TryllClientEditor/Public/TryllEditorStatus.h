// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

enum class ETryllOwnerPhase : uint8
{
	Idle,
	Connecting,
	Configuring,
	Ready,
	Error,
};

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FTryllOnOwnerPhaseChangedNative,
	ETryllOwnerPhase /*Phase*/,
	const FString& /*Detail*/);
