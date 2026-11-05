// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TryllDialogChecks.generated.h"

/**
 * One deterministic Dialog Lab assertion on a script step (or, in the script's GlobalChecks, on
 * every generated turn). Pure data: evaluation lives in FTryllDialogLabChecks.
 *
 * Instanced, like workflow node params: pick the type from the element's class dropdown in the
 * Details panel, and only that type's fields show. GetTypeId() is the stable snake_case name the
 * JSONL export uses — the same ids as Unity, so exports are interchangeable.
 */
UCLASS(Abstract, EditInlineNew, CollapseCategories, BlueprintType)
class TRYLLCLIENTEDITOR_API UTryllDialogCheck : public UObject
{
	GENERATED_BODY()

public:
	/** Off = kept in the asset but not evaluated, shown, or counted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	bool bEnabled = true;

	/** Optional author label; replaces the generated label in the Lab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Note;

	virtual FString GetTypeId() const PURE_VIRTUAL(UTryllDialogCheck::GetTypeId, return FString(););
};

/** The answer contains Text (case-insensitive unless bCaseSensitive). */
UCLASS(meta = (DisplayName = "Text: Must contain"))
class TRYLLCLIENTEDITOR_API UTryllMustContainCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	bool bCaseSensitive = false;
	virtual FString GetTypeId() const override { return TEXT("must_contain"); }
};

/** The answer does not contain Text (case-insensitive unless bCaseSensitive). */
UCLASS(meta = (DisplayName = "Text: Must not contain"))
class TRYLLCLIENTEDITOR_API UTryllMustNotContainCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	bool bCaseSensitive = false;
	virtual FString GetTypeId() const override { return TEXT("must_not_contain"); }
};

/** The answer does not match Pattern (ICU regex). */
UCLASS(meta = (DisplayName = "Regex: Forbid"))
class TRYLLCLIENTEDITOR_API UTryllRegexForbidCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Pattern;
	virtual FString GetTypeId() const override { return TEXT("regex_forbid"); }
};

/** The answer matches Pattern (ICU regex). */
UCLASS(meta = (DisplayName = "Regex: Match"))
class TRYLLCLIENTEDITOR_API UTryllRegexMatchCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Pattern;
	virtual FString GetTypeId() const override { return TEXT("regex_match"); }
};

/** Whitespace-delimited word count within [Min, Max]; 0 leaves a bound unchecked. */
UCLASS(meta = (DisplayName = "Length: Word count"))
class TRYLLCLIENTEDITOR_API UTryllWordCountCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check", meta = (ClampMin = "0"))
	int32 Min = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check", meta = (ClampMin = "0"))
	int32 Max = 0;
	virtual FString GetTypeId() const override { return TEXT("word_count"); }
};

/** An addressed classifier accepted Intent. Empty Node = any classifier that ran. */
UCLASS(meta = (DisplayName = "Intent: Intent is"))
class TRYLLCLIENTEDITOR_API UTryllIntentIsCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Node;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Intent;
	virtual FString GetTypeId() const override { return TEXT("intent_is"); }
};

/** Every addressed classifier took not_found. Empty Node = any classifier that ran. */
UCLASS(meta = (DisplayName = "Intent: Intent not found"))
class TRYLLCLIENTEDITOR_API UTryllIntentNotFoundCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Node;
	virtual FString GetTypeId() const override { return TEXT("intent_not_found"); }
};

/** A ClassifyAttributesLLM slot equals Value exactly. */
UCLASS(meta = (DisplayName = "Attribute: Attribute equals"))
class TRYLLCLIENTEDITOR_API UTryllAttributeEqualsCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Node;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Value;
	virtual FString GetTypeId() const override { return TEXT("attribute_equals"); }
};

/** A ClassifyAttributesLLM score lies in [Min, Max], inclusive. */
UCLASS(meta = (DisplayName = "Attribute: Attribute in range"))
class TRYLLCLIENTEDITOR_API UTryllAttributeInRangeCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Node;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	float Min = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	float Max = 1.f;
	virtual FString GetTypeId() const override { return TEXT("attribute_in_range"); }
};

/** The turn called ToolName at least once. */
UCLASS(meta = (DisplayName = "Tools: Calls tool"))
class TRYLLCLIENTEDITOR_API UTryllCallsToolCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
	FString ToolName;
	virtual FString GetTypeId() const override { return TEXT("calls_tool"); }
};

/**
 * The turn called no tool at all — canned and live calls included. Stricter than the Lab's
 * built-in "no unexpected tool call" gate, which only fails calls that had no canned result.
 */
UCLASS(meta = (DisplayName = "Tools: Calls no tool"))
class TRYLLCLIENTEDITOR_API UTryllCallsNoToolCheck : public UTryllDialogCheck
{
	GENERATED_BODY()
public:
	virtual FString GetTypeId() const override { return TEXT("calls_no_tool"); }
};
