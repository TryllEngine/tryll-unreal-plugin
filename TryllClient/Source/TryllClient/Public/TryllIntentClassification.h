// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TryllIntentClassification.generated.h"

/**
 * Embedding ClassifyIntent payload (event_type="intent_classified").
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllIntentClassification
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	FString NodeName;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	FString Intent;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	FString RecordId;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	int64 RecordIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	float Distance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	FString NotFoundReason;

	bool Accepted() const { return NotFoundReason.IsEmpty(); }
};

USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllIntentLlmLabelProb
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	FString Label;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	float Prob = 0.f;
};

/**
 * ClassifyIntentLLM payload (event_type="intent_llm_classified").
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllIntentLlmClassification
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	FString NodeName;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	FString Intent;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	float TopProb = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	float SecondProb = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	FString NotFoundReason;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Intent")
	TArray<FTryllIntentLlmLabelProb> LabelProbs;

	bool Accepted() const { return NotFoundReason.IsEmpty(); }
};

UCLASS()
class TRYLLCLIENT_API UTryllIntentClassificationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Tryll|Intent", meta = (DisplayName = "Accepted"))
	static bool Accepted(const FTryllIntentClassification& Classification)
	{
		return Classification.Accepted();
	}

	UFUNCTION(BlueprintPure, Category = "Tryll|Intent", meta = (DisplayName = "LLM Accepted"))
	static bool LlmAccepted(const FTryllIntentLlmClassification& Classification)
	{
		return Classification.Accepted();
	}
};
