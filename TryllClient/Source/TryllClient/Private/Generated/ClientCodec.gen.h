// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/messages.fbs  +  tools/codegen/templates/protocol_codec_cpp_client.h.j2
// See: docs/dev/codegen.md
//
// Declares Build... helpers for all non-manual client→server messages.
// Manual carve-outs (codec:"manual") are declared in ClientCodec.h alongside
// their hand-written implementations.
// Convenience multi-signature wrappers (e.g. BuildCreateStringStorageRequest
// with inline strings, BuildCreateVoiceInputRequest with flat format fields)
// also remain in ClientCodec.h.
//
// flavor: unreal

#pragma once

#include "CoreMinimal.h"
#include "messages_generated.h"
#include <flatbuffers/flatbuffers.h>
#include <string>
#include "CodegenFingerprint.gen.h"

#include <string_view>
namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_ClientCodec_gen = void;
}

namespace Tryll::Client {

// ── Shared codec helpers ──────────────────────────────────────────────────────
// Defined at namespace scope (template is implicitly inline; FStringToUtf8 is
// marked inline) so the hand-written ClientCodec.cpp and this generated unit
// share ONE definition. Previously each .cpp carried its own anonymous-namespace
// copy; those collide under UE unity builds, which merge both translation units
// into one and trip C2995 / C2084. See docs/dev/codegen.md.
template<typename TableOffset>
TArray<uint8> WrapMessage(
	flatbuffers::FlatBufferBuilder& Fbb,
	TableOffset Offset,
	::Tryll::MessageBody BodyType)
{
	auto Msg = ::Tryll::CreateMessage(Fbb, BodyType, Offset.Union());
	Fbb.Finish(Msg);
	const uint8* Buf = Fbb.GetBufferPointer();
	const int32  Sz  = static_cast<int32>(Fbb.GetSize());
	TArray<uint8> Out;
	Out.SetNumUninitialized(Sz);
	FMemory::Memcpy(Out.GetData(), Buf, Sz);
	return Out;
}

[[nodiscard]] inline std::string FStringToUtf8(const FString& Str)
{
	return std::string(TCHAR_TO_UTF8(*Str));
}

/// Build a CreateSessionRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildCreateSessionRequest(uint64 request_id, const FString& game_name, const FString& locale, const ::Tryll::SessionModelConfigT& model_config);

/// Build a SendMessageRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildSendMessageRequest(uint64 request_id, uint64 agent_id, const FString& text);

/// Build a DestroyAgentRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildDestroyAgentRequest(uint64 request_id, uint64 agent_id);

/// Build a ListModelsRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildListModelsRequest(uint64 request_id);

/// Build a DownloadModelRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildDownloadModelRequest(uint64 request_id, const FString& model_name);

/// Build a CreateStringStorageRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildCreateStringStorageRequest(uint64 request_id, const FString& name, ::Tryll::StringStorageKind kind, const TArray<FString>& strings, const TArray<FString>& keys, const FString& file_path);

/// Build a DestroyStringStorageRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildDestroyStringStorageRequest(uint64 request_id, const FString& name);

/// Build a LoadModelRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildLoadModelRequest(uint64 request_id, const FString& model_name, ::Tryll::MemoryPlacement placement, bool placement_is_override);

/// Build a UnloadModelRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildUnloadModelRequest(uint64 request_id, const FString& model_name);

/// Build a CreateEmbeddedStringStorageRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildCreateEmbeddedStringStorageRequest(uint64 request_id, const FString& name, const FString& config_path, const TArray<FString>& strings, const FString& embedding_model);

/// Build a DestroyEmbeddedStringStorageRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildDestroyEmbeddedStringStorageRequest(uint64 request_id, const FString& name);

/// Build a CreateVoiceInputRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildCreateVoiceInputRequest(uint64 request_id, const FString& model_name, const ::Tryll::WireAudioFormatT& input_format, float vad_threshold, uint32 vad_min_silence_ms, uint32 vad_speech_pad_ms, const FString& hotwords_storage_path, float hotwords_score);

/// Build a BeginUtteranceRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildBeginUtteranceRequest(uint64 request_id, uint64 voice_input_id, bool auto_finish_on_silence, uint32 max_utterance_ms);

/// Build a EndUtteranceRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildEndUtteranceRequest(uint64 request_id, uint64 voice_input_id);

/// Build a CancelUtteranceRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildCancelUtteranceRequest(uint64 request_id, uint64 voice_input_id);

/// Build a DestroyVoiceInputRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildDestroyVoiceInputRequest(uint64 request_id, uint64 voice_input_id);

/// Build a DeleteModelRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildDeleteModelRequest(uint64 request_id, const FString& model_name);

/// Build a DebugCommandRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildDebugCommandRequest(uint64 request_id, const FString& command, const FString& payload);

/// Build a CancelRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildCancelRequest(uint64 request_id, uint64 agent_id, ::Tryll::CancelMode mode);

/// Build a GetAgentVariablesRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildGetAgentVariablesRequest(uint64 request_id, uint64 agent_id, const TArray<FString>& names);

/// Build a PrefillAgentKvCacheRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildPrefillAgentKvCacheRequest(uint64 request_id, uint64 agent_id);

/// Build a EvictAgentKvCacheRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildEvictAgentKvCacheRequest(uint64 request_id, uint64 agent_id);

/// Build a GetAgentKvCacheStatusRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildGetAgentKvCacheStatusRequest(uint64 request_id, uint64 agent_id);

/// Build a RemoveInteractionsFromEndRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildRemoveInteractionsFromEndRequest(uint64 request_id, uint64 agent_id, uint32 count);

/// Build a SetInferenceThrottle FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildSetInferenceThrottle(float throttle);

/// Build a SetSessionLocaleRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildSetSessionLocaleRequest(uint64 request_id, const FString& locale);

/// Build a ExportDialogRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildExportDialogRequest(uint64 request_id, uint64 agent_id);

/// Build a GetMemoryConsumptionRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildGetMemoryConsumptionRequest(uint64 request_id);

/// Build a EnsureMutableKbRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildEnsureMutableKbRequest(uint64 request_id, const FString& name, const FString& schema);

/// Build a EnumerateMutableKbsRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildEnumerateMutableKbsRequest(uint64 request_id);

/// Build a MutableKbWriteRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildMutableKbWriteRequest(uint64 request_id, const FString& name, const FString& schema, const TArray<::Tryll::MutableKbUpsertT>& records);

/// Build a MutableKbDeleteRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildMutableKbDeleteRequest(uint64 request_id, const FString& name, const TArray<FString>& ids);

/// Build a MutableKbClearRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildMutableKbClearRequest(uint64 request_id, const FString& name);

/// Build a MutableKbQueryRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildMutableKbQueryRequest(uint64 request_id, const FString& name, const FString& query_text, uint32 top_k, ::Tryll::RetrievalMode retrieval_mode);

/// Build a MutableKbGetRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildMutableKbGetRequest(uint64 request_id, const FString& name, const FString& id);

/// Build a MutableKbListRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildMutableKbListRequest(uint64 request_id, const FString& name, uint32 offset, uint32 limit);

/// Build a MutableKbStatsRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildMutableKbStatsRequest(uint64 request_id, const FString& name);

/// Build a ExportMutableKbRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildExportMutableKbRequest(uint64 request_id, const FString& name);

/// Build a SaveGameMemoryRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildSaveGameMemoryRequest(uint64 request_id, const FString& path, const FString& save_id);

/// Build a LoadGameMemoryRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildLoadGameMemoryRequest(uint64 request_id, const FString& path, ::Tryll::MutableKbCompatibility compatibility, ::Tryll::MutableKbEmbedTiming embed_timing);

/// Build a SaveDialogRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildSaveDialogRequest(uint64 request_id, uint64 agent_id, const FString& path, const FString& save_id);

/// Build a LoadDialogRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildLoadDialogRequest(uint64 request_id, uint64 agent_id, const FString& path, ::Tryll::DialogCompatibility compatibility);

/// Build a SetAgentVoiceRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildSetAgentVoiceRequest(uint64 request_id, uint64 agent_id, const ::Tryll::VoiceProfileT& voice);

/// Build a SetAgentDeliveryModeRequest FlatBuffers payload.
[[nodiscard]] TArray<uint8>
BuildSetAgentDeliveryModeRequest(uint64 request_id, uint64 agent_id, const FString& mode, float intensity);


} // namespace Tryll::Client
