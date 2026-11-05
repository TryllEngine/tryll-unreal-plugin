// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/nodes.fbs  +  tools/codegen/templates/unreal_node.cpp.j2
// See: docs/dev/codegen.md
#include "Generated/Nodes/TryllClassifyAttributesLLMParams.h"
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
using _TryllFpCheck_TryllClassifyAttributesLLMParams = void;
}
flatbuffers::Offset<void> UTryllClassifyAttributesLLMParams::Pack(
    flatbuffers::FlatBufferBuilder& Fbb,
    ::Tryll::NodeParams::NodeParams& OutType) const
{
    using namespace ::Tryll::NodeParams;

    ClassifyAttributesLLMParamsT paramsT;

    paramsT.model_name = std::string(TCHAR_TO_UTF8(*ModelName));
    paramsT.context_size = ContextSize;
    if (Attributes.Num() > 0)
    {
        for (const FTryllClassifyAttributeDef& Item : Attributes)
        {
            auto SubT = std::make_unique<Tryll::NodeParams::ClassifyAttributeDefT>();
            SubT->name = std::string(TCHAR_TO_UTF8(*Item.Name));
            SubT->kind = static_cast<Tryll::AttributeKind>(static_cast<uint8>(Item.Kind));
            if (Item.Choices.Num() > 0)
            {
                for (const FTryllClassifyAttributeChoice& Inner : Item.Choices)
                {
                    auto InnerT = std::make_unique<Tryll::NodeParams::ClassifyAttributeChoiceT>();
                    InnerT->id = std::string(TCHAR_TO_UTF8(*Inner.Id));
                    InnerT->prompt = std::string(TCHAR_TO_UTF8(*Inner.Prompt));
                    InnerT->triggers = Inner.bTriggers;
                    SubT->choices.push_back(std::move(InnerT));
                }
            }
            SubT->statement = std::string(TCHAR_TO_UTF8(*Item.Statement));
            if (Item.bOverrideSystemPrompt || Item.bSystemPromptLocalized)
                SubT->system_prompt = std::string(TCHAR_TO_UTF8(*Item.SystemPrompt));
            SubT->threshold = Item.Threshold;
            SubT->margin = Item.Margin;
            SubT->uncertain_choice = std::string(TCHAR_TO_UTF8(*Item.UncertainChoice));
            SubT->boolean_trigger = static_cast<Tryll::BooleanTrigger>(static_cast<uint8>(Item.BooleanTrigger));
            SubT->score_trigger = static_cast<Tryll::ScoreTrigger>(static_cast<uint8>(Item.ScoreTrigger));
            SubT->score_threshold = Item.ScoreThreshold;
            paramsT.attributes.push_back(std::move(SubT));
        }
    }
    paramsT.input = std::string(TCHAR_TO_UTF8(*Input));
    paramsT.history_turns = HistoryTurns;
    paramsT.temperature = Temperature;
    paramsT.on_failure = static_cast<Tryll::AttributeFailureRoute>(static_cast<uint8>(OnFailure));
    paramsT.notify_client = static_cast<Tryll::NotifyClient>(static_cast<uint8>(NotifyClient));
    paramsT.output_name = std::string(TCHAR_TO_UTF8(*OutputName));
    paramsT.default_exit = std::string(TCHAR_TO_UTF8(*DefaultExit));
    paramsT.triggered_exit = std::string(TCHAR_TO_UTF8(*TriggeredExit));
    {
        // Sub-table: map override bools to optional scalars in the sub-table T.
        FTryllContextOverrides& src = const_cast<UTryllClassifyAttributesLLMParams*>(this)->Context;
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
        FTryllModelAdaptation& src = const_cast<UTryllClassifyAttributesLLMParams*>(this)->Adaptation;
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

    OutType = NodeParams_ClassifyAttributesLLMParams;
    auto offset = ClassifyAttributesLLMParams::Pack(Fbb, &paramsT);
    return offset.Union();
}
