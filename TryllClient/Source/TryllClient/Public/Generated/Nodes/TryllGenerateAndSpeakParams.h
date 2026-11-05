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
#include "Generated/Nodes/TryllSamplingOverrides.h"
#include "Generated/Nodes/TryllOutputFilterOverrides.h"
#include "Generated/Nodes/TryllContextOverrides.h"
#include "Generated/Nodes/TryllModelAdaptation.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllGenerateAndSpeakParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllGenerateAndSpeakParams = void;
}

/**
 * LLM-driven text generation + streaming TTS synthesis in a single fused node. Streams LLM tokens, segments them into sentences, synthesizes each sentence on a dedicated TTS strand, and emits PCM frames via OnTtsAudio callbacks. The voice (model, speaker, delivery mode, visemes) belongs to the agent: CreateAgentRequest.voice. Agent creation fails without one (3025).
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="GenerateAndSpeak"))
class TRYLLCLIENT_API UTryllGenerateAndSpeakParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Text this node answers: the player's message or an earlier node's output. Empty = the player's message. */
    /** Set to true to override the inherited Input value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(InlineEditConditionToggle))
    bool bOverrideInput = false;
    /** Upstream inter-node slot this field consumes. Empty = context default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(EditCondition="bOverrideInput", TryllStructural))
    FTryllSlotInput Input;

    /** Language model this node runs. Empty = the workflow's Default Model Name. */
    /** Set to true to override the inherited ModelName value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model", meta=(InlineEditConditionToggle))
    bool bOverrideModelName = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model",
              meta=(TryllModelType="language", EditCondition="bOverrideModelName", TryllStructural))
    FTryllModelName ModelName;

    /** Context window for this node, in tokens; must fit the model's trained maximum. 0 = the model's own size, else the server default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Model",
              meta=(ClampMin="0.0", UIMin="0.0", TryllStructural))
    int32 ContextSize = 0;

    /** System message the model reads before the dialog. Empty = none. */
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

    /** Mustache text added next to the Input (where: see Placement), for example the slot.<name> tag or retrieved knowledge. Empty = none. */
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

    /** Where Template goes: before or after the Input, as a system or a user message. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt")
    ETryllPlacement Placement = ETryllPlacement::BeforeUserAsSystem;

    /** GBNF grammar the spoken answer must follow; needs a `root` rule. Empty = free text. Use it to limit speech to a fixed set of lines. */
    /** Set to true to override the inherited Grammar value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(InlineEditConditionToggle))
    bool bOverrideGrammar = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt",
              meta=(MultiLine="true", EditCondition="bOverrideGrammar"))
    FString Grammar;

    /** Minimum length of a spoken chunk before a sentence end cuts it (CJK characters count double). Lower = speech starts sooner, more TTS calls. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="1.0", UIMin="1.0"))
    int32 MinSentenceChars = 12;

    /** Lets the first chunk end early, at a comma, dash or space once it is this long, so speech starts sooner. 0 = off; try 48. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(ClampMin="0.0", UIMin="0.0"))
    int32 FirstChunkMaxChars = 0;

    /** Name later nodes use to read this node's answer (in their Input, or as the slot.<name> tag). Empty = this node's name. */
    /** Set to true to override the inherited OutputName value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output", meta=(InlineEditConditionToggle))
    bool bOverrideOutputName = false;
    /** Effective output slot name for this node. Empty = the node's own name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output",
              meta=(EditCondition="bOverrideOutputName", TryllStructural))
    FTryllSlotName OutputName;

    /** Whether later turns remember this answer as an assistant message. None = forgotten after this turn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
    ETryllHistoryRole HistoryRole = ETryllHistoryRole::Assistant;

    /** Replace __NAME__ markers in the answer with the values of agent variables that allow output substitution. Not case-sensitive. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
    bool bSubstituteAgentVariables = false;

    /** Removes roleplay artifacts before they are spoken: "Bob:" prefixes, *actions*, (asides), [notes]. All off by default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
    FTryllOutputFilterOverrides OutputFilter;

    /** Overrides for the model's sampling settings (temperature, top-p, max tokens…). Unset values keep the model's own. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced")
    FTryllSamplingOverrides Sampling;

    /** Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(TryllStructural))
    FTryllContextOverrides Context;

    /** LoRA adapters applied to this node's model. Empty = the base model. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Advanced",
              meta=(TryllStructural))
    FTryllModelAdaptation Adaptation;

    /** Node to run after the answer is generated and spoken. Empty = end the turn. */
    /** Graph exit "default" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="default", TryllStructural))
    FString DefaultExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::GenerateAndSpeak; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/generate-and-speak.html"); }
    virtual bool IsTtsConsumer() const override { return true; }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
