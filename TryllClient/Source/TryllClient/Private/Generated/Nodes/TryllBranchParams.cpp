// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.cpp.j2
// See: docs/dev/codegen.md
#include "Generated/Nodes/TryllBranchParams.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "TryllRuntimeSettings.h"
#include <flatbuffers/flatbuffers.h>
#include "nodes_generated.h"

#include "CodegenFingerprint.gen.h"

#include <string_view>
namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllBranchParams = void;
}
flatbuffers::Offset<void> UTryllBranchParams::Pack(
    flatbuffers::FlatBufferBuilder& Fbb,
    ::Tryll::NodeParams::NodeParams& OutType) const
{
    using namespace ::Tryll::NodeParams;

    BranchParamsT paramsT;

    paramsT.test = static_cast<Tryll::BranchTest>(static_cast<uint8>(Test));
    paramsT.input = std::string(TCHAR_TO_UTF8(*Input));
    if (bOverrideVariable)
        paramsT.variable = std::string(TCHAR_TO_UTF8(*Variable));
    if (Values.Num() > 0)
    {
        paramsT.values.reserve(Values.Num());
        for (const FString& S : Values)
            paramsT.values.push_back(std::string(TCHAR_TO_UTF8(*S)));
    }
    if (bOverrideCondition)
        paramsT.condition = std::string(TCHAR_TO_UTF8(*Condition));
    paramsT.notify_client = bNotifyClient;
    paramsT.then_exit = std::string(TCHAR_TO_UTF8(*ThenExit));
    paramsT.else_exit = std::string(TCHAR_TO_UTF8(*ElseExit));

    OutType = NodeParams_BranchParams;
    auto offset = BranchParams::Pack(Fbb, &paramsT);
    return offset.Union();
}
