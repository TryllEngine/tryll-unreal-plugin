// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class FJsonObject;
class FJsonValue;
struct FTryllLabCheckSpec;
struct FTryllLabCheckOutcome;

/**
 * JSON form of typed checks and their outcomes for the Lab's JSONL export (format 4):
 * {"type": "must_contain", "enabled": true, "note": "", "text": "…", …}. A port of Unity's
 * TryllDialogCheckJson — key order and number formatting match it, so either engine's export
 * re-exports byte-for-byte on the other. Write and read live together so they cannot drift.
 */
struct TRYLLCLIENTEDITOR_API FTryllDialogCheckJson
{
	static FString Write(const FTryllLabCheckSpec& Check);
	static FString WriteList(const TArray<FTryllLabCheckSpec>& Checks);

	/** False (with OutError) for an unknown or missing type. */
	static bool Read(const TSharedPtr<FJsonObject>& Object, FTryllLabCheckSpec& OutCheck, FString& OutError);
	static bool TryReadList(
		const TArray<TSharedPtr<FJsonValue>>& Values, TArray<FTryllLabCheckSpec>& Out, FString& OutError);

	static FString WriteOutcomes(const TArray<FTryllLabCheckOutcome>& Outcomes);
	static TArray<FTryllLabCheckOutcome> ReadOutcomes(const TArray<TSharedPtr<FJsonValue>>& Values);

	/** .NET's float "R" format: the shortest text that parses back to the same float ("1", "0.25"). */
	static FString FormatFloatR(float Value);
};
