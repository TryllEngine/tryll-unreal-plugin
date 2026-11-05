// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.cpp.j2
// See: docs/dev/codegen.md
#include "Generated/Nodes/TryllToolCallParams.h"
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
using _TryllFpCheck_TryllToolCallParams = void;
}
flatbuffers::Offset<void> UTryllToolCallParams::Pack(
    flatbuffers::FlatBufferBuilder& Fbb,
    ::Tryll::NodeParams::NodeParams& OutType) const
{
    using namespace ::Tryll::NodeParams;

    ToolCallParamsT paramsT;

    paramsT.model_name = std::string(TCHAR_TO_UTF8(*ModelName));
    paramsT.context_size = ContextSize;
    if (bOverrideSystemPrompt || bSystemPromptLocalized)
        paramsT.system_prompt = std::string(TCHAR_TO_UTF8(*SystemPrompt));
    paramsT.input = std::string(TCHAR_TO_UTF8(*Input));
    paramsT.tool_call_format = std::string(TCHAR_TO_UTF8(*ToolCallFormat));
    if (Tools.Num() > 0)
    {
        for (const FTryllToolDefinition& Item : Tools)
        {
            auto SubT = std::make_unique<Tryll::NodeParams::ToolDefinitionT>();
            SubT->name = std::string(TCHAR_TO_UTF8(*Item.Name));
            SubT->description = std::string(TCHAR_TO_UTF8(*Item.Description));
            if (Item.Parameters.Num() > 0)
            {
                for (const FTryllToolParamDefinition& Inner : Item.Parameters)
                {
                    auto InnerT = std::make_unique<Tryll::NodeParams::ToolParamDefinitionT>();
                    InnerT->name = std::string(TCHAR_TO_UTF8(*Inner.Name));
                    InnerT->type = std::string(TCHAR_TO_UTF8(*Inner.Type));
                    InnerT->description = std::string(TCHAR_TO_UTF8(*Inner.Description));
                    SubT->parameters.push_back(std::move(InnerT));
                }
            }
            paramsT.tools.push_back(std::move(SubT));
        }
    }
    paramsT.mode = static_cast<Tryll::ToolCallMode>(static_cast<uint8>(Mode));
    paramsT.parallel_tool_calls = bParallelToolCalls;
    {
        // Sub-table: map override bools to optional scalars in the sub-table T.
        FTryllSamplingOverrides& src = const_cast<UTryllToolCallParams*>(this)->Sampling;
        Tryll::NodeParams::SamplingOverridesT sub;
        if (src.bOverrideTemperature)
            sub.temperature = src.Temperature;
        if (src.bOverrideTopP)
            sub.top_p = src.TopP;
        if (src.bOverrideTopK)
            sub.top_k = src.TopK;
        if (src.bOverrideMinP)
            sub.min_p = src.MinP;
        if (src.bOverrideMaxTokens)
            sub.max_tokens = src.MaxTokens;
        if (src.bOverrideSeed)
            sub.seed = src.Seed;
        if (src.bOverrideRepeatPenalty)
            sub.repeat_penalty = src.RepeatPenalty;
        if (src.bOverridePresencePenalty)
            sub.presence_penalty = src.PresencePenalty;
        if (src.bOverrideFrequencyPenalty)
            sub.frequency_penalty = src.FrequencyPenalty;
        paramsT.sampling.reset(new Tryll::NodeParams::SamplingOverridesT(std::move(sub)));
    }
    paramsT.tool_called_exit = std::string(TCHAR_TO_UTF8(*ToolCalledExit));
    paramsT.no_tool_called_exit = std::string(TCHAR_TO_UTF8(*NoToolCalledExit));
    paramsT.disposition = static_cast<Tryll::ToolCallDisposition>(static_cast<uint8>(Disposition));
    if (bOverrideAcknowledgeText || bAcknowledgeTextLocalized)
        paramsT.acknowledge_text = std::string(TCHAR_TO_UTF8(*AcknowledgeText));
    paramsT.pause_timeout_ms = PauseTimeoutMs;
    {
        // Sub-table: map override bools to optional scalars in the sub-table T.
        FTryllContextOverrides& src = const_cast<UTryllToolCallParams*>(this)->Context;
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
        FTryllModelAdaptation& src = const_cast<UTryllToolCallParams*>(this)->Adaptation;
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

    OutType = NodeParams_ToolCallParams;
    auto offset = ToolCallParams::Pack(Fbb, &paramsT);
    return offset.Union();
}
