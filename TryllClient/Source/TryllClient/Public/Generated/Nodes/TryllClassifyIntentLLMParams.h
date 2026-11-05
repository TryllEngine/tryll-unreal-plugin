// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.h.j2
// See: docs/dev/codegen.md
#pragma once



#include "CoreMinimal.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "TryllGraphDescription.h"
#include "TryllModelName.h"
#include "TryllSlotInput.h"
#include "TryllSlotName.h"
#include "Generated/Nodes/TryllClassifyIntentEntry.h"
#include "Generated/Nodes/TryllContextOverrides.h"
#include "Generated/Nodes/TryllModelAdaptation.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllClassifyIntentLLMParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllClassifyIntentLLMParams = void;
}

/**
 * Intent classification node (LLM first-token logprob over letter candidates). Scores labels via a dedicated LLM, attaches an IntentionComponent on found, and writes the decision to slot.<output_name>.
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="ClassifyIntentLLM"))
class TRYLLCLIENT_API UTryllClassifyIntentLLMParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Text to classify: the player's message or an earlier node's output. Empty = the player's message. */
    /** Set to true to override the inherited Input value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(InlineEditConditionToggle))
    bool bOverrideInput = false;
    /** Upstream inter-node slot this field consumes. Empty = context default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(EditCondition="bOverrideInput", TryllStructural))
    FTryllSlotInput Input;

    /** How many earlier dialog turns the classifier sees. 0 = the current message only. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(ClampMin="0.0", ClampMax="64.0", UIMin="0.0", UIMax="64.0"))
    int32 HistoryTurns = 2;

    /** Language model that scores the intents. Empty = the workflow's Default Model Name. */
    /** Set to true to override the inherited ModelName value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model", meta=(InlineEditConditionToggle))
    bool bOverrideModelName = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model",
              meta=(TryllModelType="language", EditCondition="bOverrideModelName", TryllStructural))
    FTryllModelName ModelName;

    /** Context window for this node, in tokens; must fit the model's trained maximum. 0 = 2048. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model",
              meta=(ClampMin="0.0", UIMin="0.0", TryllStructural))
    int32 ContextSize = 0;

    /** Instructions for the classifier. Must include the intents_block tag (the lettered intent list); the letters tag ("A, B, C") is optional. */
    /** Ticked: SystemPromptTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(TryllLocalizedToggle))
    bool bSystemPromptLocalized = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt",
              meta=(MultiLine="true", EditCondition="!bSystemPromptLocalized", EditConditionHides, TryllLocalizable))
    FString SystemPrompt;

    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(EditCondition="bSystemPromptLocalized", EditConditionHides, TryllTextId))
    FString SystemPromptTextId;

    /** The intents to choose from, 2–26. Each has an id (the label written to the output) and a description the model reads. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt",
              meta=(TryllStructural))
    TArray<FTryllClassifyIntentEntry> Intents;

    /** Minimum probability (0–1) the winning intent needs; below it = Not Found. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float Threshold = 0.5f;

    /** Minimum lead (0–1) of the winner over the runner-up; less = Not Found. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
    float Margin = 0.15f;

    /** Sharpens (below 1) or flattens (above 1) the probabilities before Threshold and Margin. 0 = unchanged. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", ClampMax="8.0", UIMin="0.0", UIMax="8.0"))
    float Temperature = 0.0f;

    /** Intent id that means "none of these": if it wins, take Not Found. Also the output on Not Found. May be #uncertain or empty. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
    FString NotFoundIntent;

    /** Name later nodes use to read the result: the winning intent id, or Not Found Intent. Empty = this node's name. */
    /** Set to true to override the inherited OutputName value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output", meta=(InlineEditConditionToggle))
    bool bOverrideOutputName = false;
    /** Effective output slot name for this node. Empty = the node's own name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output",
              meta=(EditCondition="bOverrideOutputName", TryllStructural))
    FTryllSlotName OutputName;

    /** When to send the game an intent_llm_classified event: Disabled, OnFound (Found exit only), or Always. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
    ETryllNotifyClient NotifyClient = ETryllNotifyClient::Disabled;

    /** Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(TryllStructural))
    FTryllContextOverrides Context;

    /** LoRA adapters applied to this node's model. Empty = the base model. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(TryllStructural))
    FTryllModelAdaptation Adaptation;

    /** Node to run when an intent wins clearly (passes Threshold and Margin). Empty = end the turn. */
    /** Graph exit "found" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="found", TryllStructural))
    FString FoundExit;

    /** Node to run when no intent wins clearly. Empty = end the turn. */
    /** Graph exit "not_found" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="not_found", TryllStructural))
    FString NotFoundExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::ClassifyIntentLLM; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/classify-intent-llm.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;

    UPROPERTY()
    bool bOverrideIntentsIds_DEPRECATED = false;
    UPROPERTY()
    FString IntentsIds_DEPRECATED;
    UPROPERTY()
    bool bOverrideIntentsPrompt_DEPRECATED = false;
    UPROPERTY()
    FString IntentsPrompt_DEPRECATED;
};
