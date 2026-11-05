// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.h.j2
// See: docs/dev/codegen.md
#pragma once



#include "CoreMinimal.h"
#include "TryllGraphDescription.h"
#include "Generated/Nodes/TryllClassifyAttributeChoice.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllClassifyAttributeDef.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllClassifyAttributeDef = void;
}

/**
 * One specialist question scored in the same batched call as its siblings.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllClassifyAttributeDef
{
    GENERATED_BODY()

    /** Attribute name; the answer is written to <Output Name>_<Name>. Must be unique within the node. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Name;

    /** Choice: picks one option and writes its id. Boolean: decides one Statement, writes true or false. Score: writes a 0–1 level. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETryllAttributeKind Kind = ETryllAttributeKind::Choice;

    /** Choice and Score only: the options, 2–26. For Score, order them from lowest to highest. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FTryllClassifyAttributeChoice> Choices;

    /** Boolean only. The statement judged true or false for the input. */
    /** Ticked: StatementTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TryllLocalizedToggle))
    bool bStatementLocalized = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(MultiLine="true", EditCondition="!bStatementLocalized", EditConditionHides, TryllLocalizable))
    FString Statement;

    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bStatementLocalized", EditConditionHides, TryllTextId))
    FString StatementTextId;

    /** Instructions for this attribute. Empty = a built-in default. Choice and Score must include the intents_block tag; Boolean must include the statement tag. */
    /** Set to true to override the inherited SystemPrompt value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(InlineEditConditionToggle))
    bool bOverrideSystemPrompt = false;
    /** Ticked: SystemPromptTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TryllLocalizedToggle))
    bool bSystemPromptLocalized = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(MultiLine="true", EditCondition="bOverrideSystemPrompt", TryllLocalizable))
    FString SystemPrompt;
    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bSystemPromptLocalized", EditConditionHides, TryllTextId))
    FString SystemPromptTextId;

    /** Choice: minimum probability of the winner. Boolean: probability of True needed to answer true. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float Threshold = 0.5f;

    /** Choice only. Minimum lead of the winner over the runner-up. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float Margin = 0.15f;

    /** Choice only. Answer written when the winner is too weak: a choice id or #uncertain. A choice id fires Triggered if that choice does. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString UncertainChoice;

    /** Boolean only. Which answer fires the node's Triggered exit: Never, True or False. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETryllBooleanTrigger BooleanTrigger = ETryllBooleanTrigger::Never;

    /** Score only. Fire the node's Triggered exit when the score is AtOrAbove or Below Score Threshold, or Never. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETryllScoreTrigger ScoreTrigger = ETryllScoreTrigger::Never;

    /** Score only. Level (0–1) that Score Trigger compares against. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float ScoreThreshold = 0.5f;

};
