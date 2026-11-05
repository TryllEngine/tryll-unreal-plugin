// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllAssetVersion.h"
#include "Serialization/CustomVersion.h"

const FGuid FTryllAssetVersion::Guid(0x3AF9F5F1, 0x07F24570, 0x90946A06, 0x42FD599F);

static FCustomVersionRegistration GRegisterTryllAssetVersion(
	FTryllAssetVersion::Guid, FTryllAssetVersion::LatestVersion, TEXT("TryllAsset"));
