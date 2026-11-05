// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllModelName.generated.h"

/**
 * A model name chosen from the project's registered models (the TryllModelManifest).
 *
 * In the editor this renders as a type-filtered dropdown plus the model's
 * registration tier (Production / Experimental). Tag the property with
 * meta=(TryllModelType="stt"|"tts"|"language"|"embedding"|"vad") to filter the list.
 *
 * Implicitly converts to/from FString so it is mostly drop-in where a model-name
 * string was used before.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllModelName
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll")
	FString Name;

	FTryllModelName() = default;
	FTryllModelName(const FString& InName) : Name(InName) {}
	FTryllModelName(const TCHAR* InName) : Name(InName) {}

	/// @cond DOXYGEN_SKIP
	operator const FString&() const { return Name; }
	/// @endcond
	const TCHAR* operator*() const { return *Name; }
	bool IsEmpty() const { return Name.IsEmpty(); }
	const FString& ToString() const { return Name; }

	bool operator==(const FTryllModelName& Other) const { return Name == Other.Name; }
	bool operator!=(const FTryllModelName& Other) const { return Name != Other.Name; }
};
