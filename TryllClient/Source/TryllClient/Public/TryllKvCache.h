// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllKvCache.generated.h"

/** How language-model KV contexts are materialised during CreateAgent. */
UENUM(BlueprintType)
enum class ETryllKvCacheInitialization : uint8
{
	AllocateOnly    UMETA(DisplayName = "Allocate Only"),
	Prefill         UMETA(DisplayName = "Prefill"),
	DeferAllocation UMETA(DisplayName = "Defer Allocation"),
};

/** Whether every eligible language-model node has a backend KV context. */
UENUM(BlueprintType)
enum class ETryllKvCacheResidency : uint8
{
	Evicted,
	Resident,
};

/** Whether every eligible language-model node matches the reusable prefix. */
UENUM(BlueprintType)
enum class ETryllKvCachePrefixStatus : uint8
{
	NotCurrent,
	Current,
};

/** Aggregate KV-cache lifecycle state for an agent. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllKvCacheStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	int64 AgentId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	bool bApplicable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	ETryllKvCacheResidency Residency = ETryllKvCacheResidency::Evicted;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	ETryllKvCachePrefixStatus PrefixStatus = ETryllKvCachePrefixStatus::NotCurrent;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	int32 EligibleNodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	FString DebugInfo;
};

/** Aggregate result of PrefillKvCache. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllKvCachePrefillResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	int64 AgentId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	bool bApplicable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	ETryllKvCacheResidency Residency = ETryllKvCacheResidency::Evicted;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	ETryllKvCachePrefixStatus PrefixStatus = ETryllKvCachePrefixStatus::NotCurrent;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	int32 EligibleNodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	int32 ContextsCreated = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	int32 TokensDecoded = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	int32 TokensReused = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	FString DebugInfo;
};

/** Aggregate result of EvictKvCache. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllKvCacheEvictResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	int64 AgentId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	bool bApplicable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	ETryllKvCacheResidency Residency = ETryllKvCacheResidency::Evicted;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	ETryllKvCachePrefixStatus PrefixStatus = ETryllKvCachePrefixStatus::NotCurrent;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	int32 EligibleNodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	int32 ContextsEvicted = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|KV Cache")
	FString DebugInfo;
};
