// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllSlotName.generated.h"

/**
 * Names a node's effective output slot ("output_name" fields). Empty = the
 * node's own name.
 *
 * In the editor this renders a rename-commit widget: grammar
 * ([A-Za-z_][A-Za-z0-9_]*) + reserved-name ("user_message") enforcement,
 * uniquify against existing names/slots, and propagation of the old slot
 * name into every `input` selector and `{{slot.<old>}}` template tag in the
 * graph on commit.
 *
 * Implicitly converts to/from FString so it is mostly drop-in where a
 * slot-name string was used before.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllSlotName
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll")
	FString Value;

	FTryllSlotName() = default;
	FTryllSlotName(const FString& InValue) : Value(InValue) {}
	FTryllSlotName(const TCHAR* InValue) : Value(InValue) {}

	/// @cond DOXYGEN_SKIP
	operator const FString&() const { return Value; }
	/// @endcond
	const TCHAR* operator*() const { return *Value; }
	bool IsEmpty() const { return Value.IsEmpty(); }
	const FString& ToString() const { return Value; }

	bool operator==(const FTryllSlotName& Other) const { return Value == Other.Value; }
	bool operator!=(const FTryllSlotName& Other) const { return Value != Other.Value; }
};
