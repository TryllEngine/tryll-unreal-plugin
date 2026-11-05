// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllVariableDecl.generated.h"

/**
 * Wire type of one declared per-agent variable. Mirrors the Tryll::VariableValue
 * union member that carries the value (VarInt/VarFloat/VarString/VarBool/VarStringSet).
 */
UENUM(BlueprintType)
enum class ETryllVariableType : uint8
{
	Int       = 0,
	Float     = 1,
	String    = 2,
	Bool      = 3,
	StringSet = 4,
};

/**
 * One declared per-agent variable — name, type, and initial value. Authored on
 * UTryllWorkflowAsset::Variables; the complete list is sent once in CreateAgentRequest
 * and locks each variable's type for the agent's lifetime. Only one of the
 * value fields is meaningful, selected by Type — the Details customization
 * shows only that field.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllVariableDecl
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	ETryllVariableType Type = ETryllVariableType::Int;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables",
		meta = (DisplayName = "Allow Output Substitution"))
	bool bAllowOutputSubstitution = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	int64 IntValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	double FloatValue = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	FString StringValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	bool bBoolValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	TArray<FString> StringSetValue;
};

/**
 * Per-component value override for one of the workflow asset's declared variables
 * (Material-Instance pattern: the asset owns the declaration — name and type —
 * the component may only override the initial value). Name and Value.Type must
 * match a variable declared on the asset; non-matching (stale) overrides are
 * skipped with a warning at agent-creation time.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllVariableOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	FTryllVariableDecl Value;
};

/**
 * Mutation applied to one declared agent variable in a batched
 * UpdateAgentVariablesRequest. Mirrors Tryll::VariableOp (wire enum) — ordinal
 * values must stay in sync.
 *
 * Assign: overwrite; value must match the variable's declared type.
 * AddToSet/RemoveFromSet: target must be StringSet; value carried in
 *   StringValue (idempotent — a no-op success if already present/absent).
 * Reset: restore the CreateAgent-time initial value; no value fields are read.
 */
enum class ETryllVariableOp : uint8
{
	Assign        = 0,
	AddToSet      = 1,
	RemoveFromSet = 2,
	Reset         = 3,
};

/**
 * One staged mutation, queued by FTryllAgentVariables and flushed as a single
 * UpdateAgentVariablesRequest. Plain internal plumbing (not a USTRUCT — never
 * exposed to Blueprint/editor).
 */
struct TRYLLCLIENT_API FTryllVariableUpdate
{
	FString            Name;
	ETryllVariableOp   Op        = ETryllVariableOp::Assign;
	ETryllVariableType ValueType = ETryllVariableType::Int; // ignored for Reset

	// Only the field matching ValueType is meaningful (Assign/AddToSet/RemoveFromSet);
	// AddToSet/RemoveFromSet always use ValueType::String + StringValue (the element).
	int64           IntValue = 0;
	double          FloatValue = 0.0;
	FString         StringValue;
	bool            bBoolValue = false;
	TArray<FString> StringSetValue;
};
