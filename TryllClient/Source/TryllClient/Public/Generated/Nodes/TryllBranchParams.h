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
#include "TryllSlotInput.h"
#include "CodegenFingerprint.gen.h"

#include <string_view>
#include "TryllBranchParams.generated.h"

namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllBranchParams = void;
}

/**
 * Conditional routing node. Evaluates a test against a turn-local slot or an agent variable and takes then_exit or else_exit. Reader-only: no model, no slot write, no wire emission.
 */
UCLASS(EditInlineNew, BlueprintType, meta=(DisplayName="Branch"))
class TRYLLCLIENT_API UTryllBranchParams : public UTryllNodeParamsBase
{
    GENERATED_BODY()
public:
    /** What to check: NonEmpty, IsTrue (bool Variable), Equals, EqualsIgnoreCase, Contains, Regex (against Values), or Expression (Condition). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
    ETryllBranchTest Test = ETryllBranchTest::NonEmpty;

    /** Text to test: the player's message or an earlier node's output. Empty = the player's message. Leave empty when Variable is set. */
    /** Set to true to override the inherited Input value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(InlineEditConditionToggle))
    bool bOverrideInput = false;
    /** Upstream inter-node slot this field consumes. Empty = context default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(EditCondition="bOverrideInput", TryllStructural))
    FTryllSlotInput Input;

    /** Agent variable to test instead of Input. Leave empty to test Input. */
    /** Set to true to override the inherited Variable value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(InlineEditConditionToggle))
    bool bOverrideVariable = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input",
              meta=(EditCondition="bOverrideVariable", TryllStructural))
    FString Variable;

    /** Values for Equals, EqualsIgnoreCase, Contains and Regex; the test passes if any one matches. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior")
    TArray<FString> Values;

    /** JSON condition for the Expression test, over agent variables and {"slot":"<name>"} values. Unused by other tests. */
    /** Set to true to override the inherited Condition value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior", meta=(InlineEditConditionToggle))
    bool bOverrideCondition = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior",
              meta=(MultiLine="true", EditCondition="bOverrideCondition"))
    FString Condition;

    /** Send the game a branch_taken event with the exit taken. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Output")
    bool bNotifyClient = false;

    /** Node to run when the test passes. Empty = end the turn. */
    /** Graph exit "then" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="then", TryllStructural))
    FString ThenExit;

    /** Node to run when the test fails or the value to test is missing. Empty = end the turn. */
    /** Graph exit "else" — target node name; empty = END. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Exits",
              meta=(GetOptions="GetExitTargetOptions", TryllExit="else", TryllStructural))
    FString ElseExit;


    virtual ETryllNodeType GetNodeType() const override { return ETryllNodeType::Branch; }
    virtual FString GetDocsUrl() const override { return TEXT("https://docs.tryllengine.com/v0.10/reference/nodes/branch.html"); }
    virtual flatbuffers::Offset<void> Pack(flatbuffers::FlatBufferBuilder& Fbb,
                                           Tryll::NodeParams::NodeParams& OutType) const override;
};
