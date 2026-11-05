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
#include "Generated/Nodes/TryllSamplingOverrides.h"
#include "Generated/Nodes/TryllContextOverrides.h"
#include "Generated/Nodes/TryllModelAdaptation.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllSummarizeParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllSummarizeParams = void;
}

/**
 * Rolling conversation summary. Every time the history since the last fold crosses a trigger, this node compresses the older interactions (everything outside `keep_recent_turns`) into a SummaryComponent. Generate-class projection then renders that summary at the end of its system message and replays history only from `covered_through + 1`, so a long dialogue stays coherent at a fixed token cost instead of being trimmed away. Place it last in the graph, after the node that produced the player-facing reply.
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="Summarize"))
class TRYLLCLIENT_API UTryllSummarizeParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Language model that writes the summaries. Empty = the workflow's Default Model Name. Prefer a plain instruct model over a roleplay fine-tune. */
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

    /** Instructions for the summarizer. Empty = the built-in default (third person, named speakers, no speculation). */
    /** Set to true to override the inherited SystemPrompt value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(InlineEditConditionToggle))
    bool bOverrideSystemPrompt = false;
    /** Ticked: SystemPromptTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(TryllLocalizedToggle))
    bool bSystemPromptLocalized = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt",
              meta=(MultiLine="true", EditCondition="bOverrideSystemPrompt", TryllLocalizable))
    FString SystemPrompt;
    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(EditCondition="bSystemPromptLocalized", EditConditionHides, TryllTextId))
    FString SystemPromptTextId;

    /** Request sent to the summarizer. Can use the tags previous_summary, #turns (index, user, assistant) and max_summary_tokens. Empty = built-in. */
    /** Set to true to override the inherited Template value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(InlineEditConditionToggle))
    bool bOverrideTemplate = false;
    /** Ticked: TemplateTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(TryllLocalizedToggle))
    bool bTemplateLocalized = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt",
              meta=(MultiLine="true", EditCondition="bOverrideTemplate", TryllLocalizable))
    FString Template;
    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(EditCondition="bTemplateLocalized", EditConditionHides, TryllTextId))
    FString TemplateTextId;

    /** Summarize when unsummarized history reaches this many tokens. Keep it below the point where the reply node trims history. 0 = off. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", UIMin="0.0"))
    int32 TriggerTokens = 1024;

    /** Summarize every N turns. 0 = off. With both triggers off, the node does nothing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", UIMin="0.0"))
    int32 TriggerTurns = 0;

    /** Latest turns never summarized; they are always replayed word for word. Max 512. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", ClampMax="512.0", UIMin="0.0", UIMax="512.0"))
    int32 KeepRecentTurns = 3;

    /** Longest summary, in tokens. Must leave at least 64 tokens of Context Size free. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="16.0", UIMin="16.0"))
    int32 MaxSummaryTokens = 256;

    /** Send the game a summarized event after each summary. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
    bool bNotifyClient = false;

    /** Overrides for the model's sampling settings (temperature, top-p, max tokens…). Unset values keep the model's own. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced")
    FTryllSamplingOverrides Sampling;

    /** Must stay on: summarize inside the turn that triggered it, which adds the summary time to that turn. Background mode is not available yet. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced")
    bool bRunInline = true;

    /** Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(TryllStructural))
    FTryllContextOverrides Context;

    /** LoRA adapters applied to this node's model. Empty = the base model. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(TryllStructural))
    FTryllModelAdaptation Adaptation;

    /** Node to run next, whether or not a summary was written. Empty = end the turn. */
    /** Graph exit "default" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="default", TryllStructural))
    FString DefaultExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::Summarize; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/summarize.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
