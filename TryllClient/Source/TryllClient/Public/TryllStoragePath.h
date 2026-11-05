// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllStoragePath.generated.h"

/**
 * Path relative to TryllRuntimeSettings::StorageDataFolder (the process storage root),
 * using forward slashes — e.g. "RAG/lore.json", not "StringStorages/RAG/lore.json".
 * Editor UI: browse + validation via FTryllStoragePathCustomization (TryllClientEditor).
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllStoragePath
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll")
	FString Path;

	bool operator==(const FTryllStoragePath& Other) const
	{
		return Path == Other.Path;
	}
};
