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
#include "TryllStoragePath.h"
#include "TryllSlotName.h"
#include "Generated/Nodes/TryllLocalizableText.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllIntentToInstructionParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllIntentToInstructionParams = void;
}

/**
 * Maps an intent label to an instruction text from a Map-kind string storage. Resolution priority (factory): named string_storage → inline_keys + instructions (Map-kind in-memory storage built from the equal-length pair).
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="IntentToInstruction"))
class TRYLLCLIENT_API UTryllIntentToInstructionParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** Storage file that maps intent labels to instruction text. Used instead of Inline Keys and Instructions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStorageKind="plain"))
    FTryllStoragePath StringStorage;

    /** Intent labels, paired by position with Instructions. Used when String Storage is empty. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStructural))
    TArray<FString> InlineKeys;

    /** Instruction text for each label in Inline Keys, in the same order. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage",
              meta=(TryllStructural))
    TArray<FTryllLocalizableText> Instructions;

    /** Name later nodes use to read the chosen instruction (in their Input, or as the slot.<name> tag). Empty = this node's name. */
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


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::IntentToInstruction; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/intent-to-instruction.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;

    UPROPERTY()
    TArray<FString> InlineStrings_DEPRECATED;
};
