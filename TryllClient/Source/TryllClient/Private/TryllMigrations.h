// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class UTryllNodeParamsBase;

TRYLLCLIENT_API void TryllApplyOldDefaults(UTryllNodeParamsBase& Params, int32 SavedVersion);
TRYLLCLIENT_API bool TryllUpgradeParams(UTryllNodeParamsBase& Params, int32 SavedVersion);
