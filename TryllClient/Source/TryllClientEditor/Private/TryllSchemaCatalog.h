// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class UTryllRuntimeSettings;

/**
 * Testable reader for data/schemas.json. Production callers resolve the
 * bundled server variant; tests pass JSON or a directory.
 */
struct TRYLLCLIENTEDITOR_API FTryllSchemaCatalog
{
	TArray<FString> Names;
	bool bIsAvailable = false;
	FString SourcePath;

	static const FTryllSchemaCatalog& Empty();

	[[nodiscard]] bool Contains(const FString& Name) const;

	static FTryllSchemaCatalog Parse(const FString& Json, const FString& InSourcePath = FString());
	static FTryllSchemaCatalog LoadFromDirectory(const FString& ServerDir);
	static FTryllSchemaCatalog LoadBundled(const UTryllRuntimeSettings* Settings = nullptr);
};
