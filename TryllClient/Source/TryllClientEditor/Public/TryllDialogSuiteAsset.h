// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TryllDialogSuiteAsset.generated.h"

class UTryllDialogScriptAsset;

/**
 * An ordered set of dialog scripts the Dialog Lab runs together. Drop it on the Lab's Dialog
 * Script field instead of a single script: every variant x seed column then runs each dialogue
 * in order, and the results group per dialogue. Each dialogue gets a fresh agent, so nothing
 * one dialogue sets (variables, slots, summaries) leaks into the next.
 */
UCLASS(BlueprintType)
class TRYLLCLIENTEDITOR_API UTryllDialogSuiteAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll", meta = (MultiLine = true))
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll")
	TArray<TObjectPtr<UTryllDialogScriptAsset>> Dialogues;
};
