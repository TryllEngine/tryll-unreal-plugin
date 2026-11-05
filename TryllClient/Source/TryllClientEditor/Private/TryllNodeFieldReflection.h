// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllDialogLabExperiment.h"

class UTryllNodeParamsBase;
struct FTryllGraphDescription;

enum class ETryllFieldKind : uint8
{
	String,
	MultilineString,
	Int,
	Float,
	Bool,
	Enum,
};

/** One field a variant is allowed to override, addressed by a dotted path relative to a
 *  node's params object. */
struct FTryllOverridableField
{
	FString FieldPath;
	FString DisplayName;
	ETryllFieldKind Kind = ETryllFieldKind::String;
	const UEnum* EnumType = nullptr;

	/**
	 * The field is guarded by a sibling bOverrideX toggle. Writing a value must switch the
	 * toggle on, and clearing the value must switch it off — the codegen's stand-in for an
	 * optional. Missing that on Sampling.Seed would leave every sample running with a random
	 * seed while appearing to be seeded.
	 */
	bool bIsOptional = false;

	TOptional<double> Min;
	TOptional<double> Max;
};

/**
 * Reflects over a node's params properties to find what a Dialog Lab variant may override,
 * and applies string-valued overrides onto a cloned params object. Generic by design — it
 * does not enumerate node types, so a new node type needs no change here.
 */
struct TRYLLCLIENTEDITOR_API FTryllNodeFieldReflection
{
	static TArray<FTryllOverridableField> EnumerateOverridableFields(const UClass* ParamsClass);

	static bool TryGetField(
		const UClass* ParamsClass, const FString& FieldPath, FTryllOverridableField& OutField);

	static FString GetValue(const UTryllNodeParamsBase* Params, const FTryllOverridableField& Field);

	/** Empty return means success; anything else is the user-facing reason. */
	static FString TrySet(
		UTryllNodeParamsBase* Params, const FTryllOverridableField& Field, const FString& Value);

	/** Empty return means the value is acceptable for that field. */
	static FString ValidateValue(const FTryllOverridableField& Field, const FString& Value);

	/** Applies overrides in order, stopping at the first failure. */
	static bool TryApply(
		UTryllNodeParamsBase* Params,
		const TArray<FTryllLabParamOverride>& Overrides,
		FString& OutError);

	/** Validates every override against a graph without mutating anything. */
	static TArray<FString> CollectOverrideProblems(
		const FTryllGraphDescription& Graph,
		const TArray<FTryllLabParamOverride>& Overrides);
};
