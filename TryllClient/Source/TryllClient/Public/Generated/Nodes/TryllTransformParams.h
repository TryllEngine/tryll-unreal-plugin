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
#include "TryllSlotName.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllTransformParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllTransformParams = void;
}

/**
 * Model-free input-composition node.  Renders a Mustache template over `user_message` and the current turn's slots into its own slot; performs no inference and never emits on the wire (`send = None`, `history_role = none`, fixed). Typical use: query rewriting, fan-out composition feeding a downstream Retrieve/Generate `input`.
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="Transform"))
class TRYLLCLIENT_API UTryllTransformParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Mustache text this node outputs. Can use the tags user_message, slot.<name>, knowledge_<source> and #instructions. */
    /** Ticked: TemplateTextId is resolved by the text provider and that text is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(TryllLocalizedToggle))
    bool bTemplateLocalized = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt",
              meta=(MultiLine="true", EditCondition="!bTemplateLocalized", EditConditionHides, TryllLocalizable))
    FString Template;

    /** Text ID in the workflow text table. The provider's text is what is sent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prompt", meta=(EditCondition="bTemplateLocalized", EditConditionHides, TryllTextId))
    FString TemplateTextId;

    /** Name later nodes use to read the rendered text (in their Input, or as the slot.<name> tag). Empty = this node's name. */
    /** Set to true to override the inherited OutputName value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output", meta=(InlineEditConditionToggle))
    bool bOverrideOutputName = false;
    /** Effective output slot name for this node. Empty = the node's own name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output",
              meta=(EditCondition="bOverrideOutputName", TryllStructural))
    FTryllSlotName OutputName;

    /** Node to run next. Empty = end the turn. */
    /** Graph exit "default" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="default", TryllStructural))
    FString DefaultExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::Transform; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/transform.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
