// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.cpp.j2
// See: docs/dev/codegen.md
#include "Generated/Nodes/TryllRegexGuardrailParams.h"
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
using _TryllFpCheck_TryllRegexGuardrailParams = void;
}
flatbuffers::Offset<void> UTryllRegexGuardrailParams::Pack(
    flatbuffers::FlatBufferBuilder& Fbb,
    ::Tryll::NodeParams::NodeParams& OutType) const
{
    using namespace ::Tryll::NodeParams;

    RegexGuardrailParamsT paramsT;

    paramsT.string_storage = std::string(TCHAR_TO_UTF8(*UTryllRuntimeSettings::NormalizeStorageRelativePath(StringStorage.Path)));
    if (InlineStrings.Num() > 0)
    {
        paramsT.inline_strings.reserve(InlineStrings.Num());
        for (const FString& S : InlineStrings)
            paramsT.inline_strings.push_back(std::string(TCHAR_TO_UTF8(*S)));
    }
    paramsT.input = std::string(TCHAR_TO_UTF8(*Input));
    paramsT.triggered_exit = std::string(TCHAR_TO_UTF8(*TriggeredExit));
    paramsT.not_triggered_exit = std::string(TCHAR_TO_UTF8(*NotTriggeredExit));

    OutType = NodeParams_RegexGuardrailParams;
    auto offset = RegexGuardrailParams::Pack(Fbb, &paramsT);
    return offset.Union();
}
