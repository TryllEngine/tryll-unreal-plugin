// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Saved into every package that holds Tryll node params. Packages saved before it
 * existed read as -1. One entry per change that needs a conversion in
 * TryllMigrations.cpp. Append only: never remove or reorder entries, or saved assets
 * will skip conversions or fail to load.
 */
struct TRYLLCLIENT_API FTryllAssetVersion
{
	enum Type
	{
		BeforeCustomVersionWasAdded = 0,
		ClassifyIntentLLMIntentsList,
		LocalizableInlineText,

		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	static const FGuid Guid;
};
