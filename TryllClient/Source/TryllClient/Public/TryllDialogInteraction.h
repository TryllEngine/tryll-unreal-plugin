// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllDialogInteraction.generated.h"

/** One scripted user/assistant exchange for AppendInteractions. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllDialogInteraction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Dialog")
	FString UserMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Dialog")
	FString AssistantMessage;
};
