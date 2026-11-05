// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.cpp.j2
// See: docs/dev/codegen.md
#include "Generated/Nodes/TryllClassifyIntentLLMParams.h"
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
using _TryllFpCheck_TryllClassifyIntentLLMParams = void;
}
flatbuffers::Offset<void> UTryllClassifyIntentLLMParams::Pack(
    flatbuffers::FlatBufferBuilder& Fbb,
    ::Tryll::NodeParams::NodeParams& OutType) const
{
    using namespace ::Tryll::NodeParams;

    ClassifyIntentLLMParamsT paramsT;

    paramsT.model_name = std::string(TCHAR_TO_UTF8(*ModelName));
    paramsT.context_size = ContextSize;
    paramsT.system_prompt = std::string(TCHAR_TO_UTF8(*SystemPrompt));
    if (Intents.Num() > 0)
    {
        for (const FTryllClassifyIntentEntry& Item : Intents)
        {
            auto SubT = std::make_unique<Tryll::NodeParams::ClassifyIntentEntryT>();
            SubT->id = std::string(TCHAR_TO_UTF8(*Item.Id));
            SubT->prompt = std::string(TCHAR_TO_UTF8(*Item.Prompt));
            paramsT.intents.push_back(std::move(SubT));
        }
    }
    paramsT.input = std::string(TCHAR_TO_UTF8(*Input));
    paramsT.output_name = std::string(TCHAR_TO_UTF8(*OutputName));
    paramsT.history_turns = HistoryTurns;
    paramsT.threshold = Threshold;
    paramsT.margin = Margin;
    paramsT.temperature = Temperature;
    paramsT.notify_client = static_cast<Tryll::NotifyClient>(static_cast<uint8>(NotifyClient));
    paramsT.not_found_intent = std::string(TCHAR_TO_UTF8(*NotFoundIntent));
    paramsT.found_exit = std::string(TCHAR_TO_UTF8(*FoundExit));
    paramsT.not_found_exit = std::string(TCHAR_TO_UTF8(*NotFoundExit));
    {
        // Sub-table: map override bools to optional scalars in the sub-table T.
        FTryllContextOverrides& src = const_cast<UTryllClassifyIntentLLMParams*>(this)->Context;
        Tryll::NodeParams::ContextOverridesT sub;
        sub.kv_cache_type = static_cast<Tryll::KvCacheType>(static_cast<uint8>(src.KvCacheType));
        if (src.bOverrideOffloadKqv)
            sub.offload_kqv = src.OffloadKqv;
        if (src.bOverrideNUbatch)
            sub.n_ubatch = src.NUbatch;
        if (src.bOverrideNOutputsMax)
            sub.n_outputs_max = src.NOutputsMax;
        if (src.bOverrideNBatch)
            sub.n_batch = src.NBatch;
        paramsT.context.reset(new Tryll::NodeParams::ContextOverridesT(std::move(sub)));
    }
    {
        // Sub-table: map override bools to optional scalars in the sub-table T.
        FTryllModelAdaptation& src = const_cast<UTryllClassifyIntentLLMParams*>(this)->Adaptation;
        Tryll::NodeParams::ModelAdaptationT sub;
        if (src.Loras.Num() > 0)
        {
            for (const FTryllLoraAdapter& VecItem : src.Loras)
            {
                auto VecSubT = std::make_unique<Tryll::NodeParams::LoraAdapterT>();
                VecSubT->path = std::string(TCHAR_TO_UTF8(*UTryllRuntimeSettings::NormalizeStorageRelativePath(VecItem.Path.Path)));
                VecSubT->scale = VecItem.Scale;
                sub.loras.push_back(std::move(VecSubT));
            }
        }
        paramsT.adaptation.reset(new Tryll::NodeParams::ModelAdaptationT(std::move(sub)));
    }

    OutType = NodeParams_ClassifyIntentLLMParams;
    auto offset = ClassifyIntentLLMParams::Pack(Fbb, &paramsT);
    return offset.Union();
}
