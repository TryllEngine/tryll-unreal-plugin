// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_builder.h.j2
// See: docs/dev/codegen.md
#pragma once

#include "CoreMinimal.h"
#include "TryllGraphDescription.h"

#include "Generated/Nodes/TryllGenerateParams.h"
#include "Generated/Nodes/TryllRegexGuardrailParams.h"
#include "Generated/Nodes/TryllCannedResponseParams.h"
#include "Generated/Nodes/TryllToolCallParams.h"
#include "Generated/Nodes/TryllRetrieveParams.h"
#include "Generated/Nodes/TryllInstructionParams.h"
#include "Generated/Nodes/TryllClassifyIntentParams.h"
#include "Generated/Nodes/TryllClassifyIntentLLMParams.h"
#include "Generated/Nodes/TryllIntentToInstructionParams.h"
#include "Generated/Nodes/TryllGenerateAndSpeakParams.h"
#include "Generated/Nodes/TryllSpeakParams.h"
#include "Generated/Nodes/TryllPauseParams.h"
#include "Generated/Nodes/TryllTransformParams.h"
#include "Generated/Nodes/TryllBranchParams.h"
#include "Generated/Nodes/TryllWriteToMemoryParams.h"
#include "Generated/Nodes/TryllSummarizeParams.h"
#include "Generated/Nodes/TryllClassifyAttributesLLMParams.h"

#include "CodegenFingerprint.gen.h"

#include <string_view>
namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllGraphBuilder_Nodes = void;
}

/**
 * Generated typed AddX builder methods mixed into FTryllGraphBuilder.
 * Include this header in TryllGraphDescription.h.
 */

// These are free functions; use them as builder helpers or mixin them.
/** Add a Generate node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddGenerateNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllGenerateParams* Params);
/** Add a RegexGuardrail node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddRegexGuardrailNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllRegexGuardrailParams* Params);
/** Add a CannedResponse node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddCannedResponseNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllCannedResponseParams* Params);
/** Add a ToolCall node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddToolCallNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllToolCallParams* Params);
/** Add a Retrieve node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddRetrieveNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllRetrieveParams* Params);
/** Add a Instruction node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddInstructionNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllInstructionParams* Params);
/** Add a ClassifyIntent node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddClassifyIntentNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllClassifyIntentParams* Params);
/** Add a ClassifyIntentLLM node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddClassifyIntentLLMNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllClassifyIntentLLMParams* Params);
/** Add a IntentToInstruction node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddIntentToInstructionNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllIntentToInstructionParams* Params);
/** Add a GenerateAndSpeak node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddGenerateAndSpeakNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllGenerateAndSpeakParams* Params);
/** Add a Speak node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddSpeakNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllSpeakParams* Params);
/** Add a Pause node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddPauseNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllPauseParams* Params);
/** Add a Transform node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddTransformNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllTransformParams* Params);
/** Add a Branch node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddBranchNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllBranchParams* Params);
/** Add a WriteToMemory node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddWriteToMemoryNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllWriteToMemoryParams* Params);
/** Add a Summarize node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddSummarizeNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllSummarizeParams* Params);
/** Add a ClassifyAttributesLLM node to the graph. */
TRYLLCLIENT_API FTryllGraphBuilder& AddClassifyAttributesLLMNode(
    FTryllGraphBuilder& Builder,
    FString Name,
    UTryllClassifyAttributesLLMParams* Params);
