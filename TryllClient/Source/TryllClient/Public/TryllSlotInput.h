// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllSlotInput.generated.h"

/**
 * Selects an upstream inter-node slot ("input"-style fields: GenerateParams::Input,
 * GenerateAndSpeakParams::Input, RetrieveParams::Input, SpeakParams::Input,
 * ClassifyIntentParams::Input, ClassifyIntentLLMParams::Input, ToolCallParams::Input).
 *
 * In the editor this renders as a two-kind dropdown — "User Message" plus one
 * "Slot <X>" entry per slot reachable upstream of the owning node on some
 * path — no free-text mode. Empty Value = "user_message" (every input-bearing
 * node, including Speak, shares this default).
 *
 * Implicitly converts to/from FString so it is mostly drop-in where a slot-name
 * string was used before.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllSlotInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll")
	FString Value;

	FTryllSlotInput() = default;
	FTryllSlotInput(const FString& InValue) : Value(InValue) {}
	FTryllSlotInput(const TCHAR* InValue) : Value(InValue) {}

	/// @cond DOXYGEN_SKIP
	operator const FString&() const { return Value; }
	/// @endcond
	const TCHAR* operator*() const { return *Value; }
	bool IsEmpty() const { return Value.IsEmpty(); }
	const FString& ToString() const { return Value; }

	bool operator==(const FTryllSlotInput& Other) const { return Value == Other.Value; }
	bool operator!=(const FTryllSlotInput& Other) const { return Value != Other.Value; }
};
