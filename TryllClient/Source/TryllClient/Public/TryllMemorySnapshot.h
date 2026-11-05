// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllMemorySnapshot.generated.h"

namespace Tryll
{
struct GetMemoryConsumptionResponse;
struct GetMemoryConsumptionResponseT;
}

/**
 * How much to trust a reported memory figure. Rendering a number without
 * its tier is a client bug — an unmarked estimate will be compared against
 * Task Manager and believed.
 */
UENUM(BlueprintType)
enum class ETryllMemoryTier : uint8
{
	Measured  = 0,
	Derived   = 1,
	Estimated = 2,
};

/** What owns a component's bytes. Weights belong to a model; KV to an agent × node. */
UENUM(BlueprintType)
enum class ETryllMemoryOwnerKind : uint8
{
	Process      = 0,
	Device       = 1,
	Model        = 2,
	AgentNode    = 3,
	Storage      = 4,
	Unattributed = 5,
};

/**
 * Scheduler class of the last turn on a language-model context. Describes
 * work, not the agent — always read alongside LastTurnAtMs.
 */
UENUM(BlueprintType)
enum class ETryllInferenceConsumerClass : uint8
{
	Unspecified          = 0,
	InteractiveVisible   = 1,
	InteractiveBuffered  = 2,
	Background           = 3,
};

/** One memory space a component occupies (host RAM when DeviceIndex is -1). */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllMemoryComponentDevice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int32 DeviceIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 Bytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 ComputeBytes = 0;
};

/** One row of the memory breakdown. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllMemoryComponent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	FString Kind;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	ETryllMemoryOwnerKind Owner = ETryllMemoryOwnerKind::Model;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	ETryllMemoryTier Tier = ETryllMemoryTier::Measured;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	FString EstimateBasis;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 RamBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 VramBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 RamBytesHigh = 0;

	/** GPU index 0 is valid; host RAM is -1. Do not treat 0 as missing. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int32 DeviceIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	TArray<FTryllMemoryComponentDevice> PerDevice;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 SessionId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 AgentId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	FString NodeName;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int32 UsedTokens = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int32 CapacityTokens = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	bool bResident = true;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	bool bOccupancyStale = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 ComputeBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	ETryllInferenceConsumerClass ConsumerClass = ETryllInferenceConsumerClass::Unspecified;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 LastTurnAtMs = 0;
};

/** One device's memory situation. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllMemoryDevice
{
	GENERATED_BODY()

	/** GPU index 0 is valid; host RAM is -1. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int32 Index = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	FString Kind;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 TotalBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 FreeBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 SelfModelBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 SelfContextBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 SelfComputeBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 OtherBytes = 0;

	[[nodiscard]] int64 GetSelfBytes() const
	{
		return SelfModelBytes + SelfContextBytes + SelfComputeBytes;
	}
};

/** A complete memory picture at one instant, pulled over the wire. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllMemorySnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 CapturedAtMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 ProcessWorkingSetBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 ProcessPrivateBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	bool bProcessAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	bool bBackendInitialized = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	int64 UnattributedRamBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	TArray<FTryllMemoryDevice> Devices;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Memory")
	TArray<FTryllMemoryComponent> Components;

	static FTryllMemorySnapshot FromWire(const ::Tryll::GetMemoryConsumptionResponse* Resp);
	static FTryllMemorySnapshot FromWire(const ::Tryll::GetMemoryConsumptionResponseT& Resp);
};
