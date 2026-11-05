// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllToolCall.generated.h"

USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllToolCall
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Tools")
	FString CallId;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Tools")
	FString NodeName;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Tools")
	FString ToolName;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Tools")
	FString ArgumentsJson;

	/**
	 * Snapshot of disposition ("route_only" / "acknowledge" / "notify" /
	 * "notify_and_acknowledge" / "pause" / "pause_and_acknowledge" /
	 * "await_result"). RegisterTool uses this to decide whether auto-resume
	 * must carry tool results (await_result only).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Tools")
	FString Disposition;

	bool RequiresNativeResults() const
	{
		return Disposition.Equals(TEXT("await_result"), ESearchCase::CaseSensitive);
	}
};

USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllToolResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Tools")
	FString CallId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Tools")
	FString Result;
};

/** Outcome returned by a registered native tool handler. */
struct TRYLLCLIENT_API FTryllToolHandlerResult
{
	bool bSuccess = true;
	FString Result;
	FString Error;

	static FTryllToolHandlerResult Success(FString InResult = {})
	{
		return {true, MoveTemp(InResult), {}};
	}

	static FTryllToolHandlerResult Failure(FString InError)
	{
		return {false, {}, MoveTemp(InError)};
	}
};

using FTryllToolHandler =
	TFunction<FTryllToolHandlerResult(const FTryllToolCall& Call)>;
using FTryllSimpleToolHandler =
	TFunction<FString(const FTryllToolCall& Call)>;
