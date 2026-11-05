// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.cpp.j2
// See: docs/dev/codegen.md
#include "Generated/Nodes/TryllIntentToInstructionParams.h"
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
using _TryllFpCheck_TryllIntentToInstructionParams = void;
}
flatbuffers::Offset<void> UTryllIntentToInstructionParams::Pack(
    flatbuffers::FlatBufferBuilder& Fbb,
    ::Tryll::NodeParams::NodeParams& OutType) const
{
    using namespace ::Tryll::NodeParams;

    IntentToInstructionParamsT paramsT;

    paramsT.string_storage = std::string(TCHAR_TO_UTF8(*UTryllRuntimeSettings::NormalizeStorageRelativePath(StringStorage.Path)));
    if (InlineKeys.Num() > 0)
    {
        paramsT.inline_keys.reserve(InlineKeys.Num());
        for (const FString& S : InlineKeys)
            paramsT.inline_keys.push_back(std::string(TCHAR_TO_UTF8(*S)));
    }
    paramsT.output_name = std::string(TCHAR_TO_UTF8(*OutputName));
    paramsT.default_exit = std::string(TCHAR_TO_UTF8(*DefaultExit));
    if (Instructions.Num() > 0)
    {
        for (const FTryllLocalizableText& Item : Instructions)
        {
            auto SubT = std::make_unique<Tryll::NodeParams::LocalizableTextT>();
            SubT->text = std::string(TCHAR_TO_UTF8(*Item.Text));
            paramsT.instructions.push_back(std::move(SubT));
        }
    }

    OutType = NodeParams_IntentToInstructionParams;
    auto offset = IntentToInstructionParams::Pack(Fbb, &paramsT);
    return offset.Union();
}
