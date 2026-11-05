// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/messages.fbs  +  tools/codegen/templates/protocol_codec_cpp_client.cpp.j2
// See: docs/dev/codegen.md
//
// flavor: unreal

#include "ClientCodec.gen.h"
#include "RpcTraits.gen.h"
#include "TryllMessageDescriptors.gen.h"

#include "Win32Undef.h"
#include "messages_generated.h"
#include <tryll/FbbHelpers.h>
#include <flatbuffers/flatbuffers.h>

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

// WrapMessage / FStringToUtf8 are defined in ClientCodec.gen.h so this unit and
// the hand-written ClientCodec.cpp share one definition (UE unity builds merge
// both .cpp into a single translation unit; per-file anonymous-namespace copies
// would collide — C2995 / C2084).

TArray<uint8>
BuildCreateSessionRequest(uint64 request_id, const FString& game_name, const FString& locale, const ::Tryll::SessionModelConfigT& model_config)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> game_name_off;
	if (!game_name.IsEmpty())
		game_name_off = Fbb.CreateString(FStringToUtf8(game_name));
	flatbuffers::Offset<flatbuffers::String> locale_off;
	if (!locale.IsEmpty())
		locale_off = Fbb.CreateString(FStringToUtf8(locale));
	auto model_config_off = ::Tryll::SessionModelConfig::Pack(Fbb, &model_config);

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateCreateSessionRequest(Fbb, request_id, game_name_off, locale_off, model_config_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_CreateSessionRequest);
}

TArray<uint8>
BuildSendMessageRequest(uint64 request_id, uint64 agent_id, const FString& text)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> text_off;
	if (!text.IsEmpty())
		text_off = Fbb.CreateString(FStringToUtf8(text));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateSendMessageRequest(Fbb, request_id, agent_id, text_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_SendMessageRequest);
}

TArray<uint8>
BuildDestroyAgentRequest(uint64 request_id, uint64 agent_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateDestroyAgentRequest(Fbb, request_id, agent_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_DestroyAgentRequest);
}

TArray<uint8>
BuildListModelsRequest(uint64 request_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateListModelsRequest(Fbb, request_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_ListModelsRequest);
}

TArray<uint8>
BuildDownloadModelRequest(uint64 request_id, const FString& model_name)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> model_name_off;
	if (!model_name.IsEmpty())
		model_name_off = Fbb.CreateString(FStringToUtf8(model_name));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateDownloadModelRequest(Fbb, request_id, model_name_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_DownloadModelRequest);
}

TArray<uint8>
BuildCreateStringStorageRequest(uint64 request_id, const FString& name, ::Tryll::StringStorageKind kind, const TArray<FString>& strings, const TArray<FString>& keys, const FString& file_path)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));
	TArray<flatbuffers::Offset<flatbuffers::String>> strings_str_offs;
	strings_str_offs.Reserve(strings.Num());
	for (const FString& _s : strings)
		strings_str_offs.Add(Fbb.CreateString(FStringToUtf8(_s)));
	flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> strings_off;
	if (strings.Num() > 0)
		strings_off = Fbb.CreateVector(strings_str_offs.GetData(), static_cast<size_t>(strings_str_offs.Num()));
	TArray<flatbuffers::Offset<flatbuffers::String>> keys_str_offs;
	keys_str_offs.Reserve(keys.Num());
	for (const FString& _s : keys)
		keys_str_offs.Add(Fbb.CreateString(FStringToUtf8(_s)));
	flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> keys_off;
	if (keys.Num() > 0)
		keys_off = Fbb.CreateVector(keys_str_offs.GetData(), static_cast<size_t>(keys_str_offs.Num()));
	flatbuffers::Offset<flatbuffers::String> file_path_off;
	if (!file_path.IsEmpty())
		file_path_off = Fbb.CreateString(FStringToUtf8(file_path));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateCreateStringStorageRequest(Fbb, request_id, name_off, kind, strings_off, keys_off, file_path_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_CreateStringStorageRequest);
}

TArray<uint8>
BuildDestroyStringStorageRequest(uint64 request_id, const FString& name)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateDestroyStringStorageRequest(Fbb, request_id, name_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_DestroyStringStorageRequest);
}

TArray<uint8>
BuildLoadModelRequest(uint64 request_id, const FString& model_name, ::Tryll::MemoryPlacement placement, bool placement_is_override)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> model_name_off;
	if (!model_name.IsEmpty())
		model_name_off = Fbb.CreateString(FStringToUtf8(model_name));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateLoadModelRequest(Fbb, request_id, model_name_off, placement, placement_is_override);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_LoadModelRequest);
}

TArray<uint8>
BuildUnloadModelRequest(uint64 request_id, const FString& model_name)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> model_name_off;
	if (!model_name.IsEmpty())
		model_name_off = Fbb.CreateString(FStringToUtf8(model_name));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateUnloadModelRequest(Fbb, request_id, model_name_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_UnloadModelRequest);
}

TArray<uint8>
BuildCreateEmbeddedStringStorageRequest(uint64 request_id, const FString& name, const FString& config_path, const TArray<FString>& strings, const FString& embedding_model)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));
	flatbuffers::Offset<flatbuffers::String> config_path_off;
	if (!config_path.IsEmpty())
		config_path_off = Fbb.CreateString(FStringToUtf8(config_path));
	TArray<flatbuffers::Offset<flatbuffers::String>> strings_str_offs;
	strings_str_offs.Reserve(strings.Num());
	for (const FString& _s : strings)
		strings_str_offs.Add(Fbb.CreateString(FStringToUtf8(_s)));
	flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> strings_off;
	if (strings.Num() > 0)
		strings_off = Fbb.CreateVector(strings_str_offs.GetData(), static_cast<size_t>(strings_str_offs.Num()));
	flatbuffers::Offset<flatbuffers::String> embedding_model_off;
	if (!embedding_model.IsEmpty())
		embedding_model_off = Fbb.CreateString(FStringToUtf8(embedding_model));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateCreateEmbeddedStringStorageRequest(Fbb, request_id, name_off, config_path_off, strings_off, embedding_model_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest);
}

TArray<uint8>
BuildDestroyEmbeddedStringStorageRequest(uint64 request_id, const FString& name)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateDestroyEmbeddedStringStorageRequest(Fbb, request_id, name_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_DestroyEmbeddedStringStorageRequest);
}

TArray<uint8>
BuildCreateVoiceInputRequest(uint64 request_id, const FString& model_name, const ::Tryll::WireAudioFormatT& input_format, float vad_threshold, uint32 vad_min_silence_ms, uint32 vad_speech_pad_ms, const FString& hotwords_storage_path, float hotwords_score)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> model_name_off;
	if (!model_name.IsEmpty())
		model_name_off = Fbb.CreateString(FStringToUtf8(model_name));
	auto input_format_off = ::Tryll::WireAudioFormat::Pack(Fbb, &input_format);
	flatbuffers::Offset<flatbuffers::String> hotwords_storage_path_off;
	if (!hotwords_storage_path.IsEmpty())
		hotwords_storage_path_off = Fbb.CreateString(FStringToUtf8(hotwords_storage_path));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateCreateVoiceInputRequest(Fbb, request_id, model_name_off, input_format_off, vad_threshold, vad_min_silence_ms, vad_speech_pad_ms, hotwords_storage_path_off, hotwords_score);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_CreateVoiceInputRequest);
}

TArray<uint8>
BuildBeginUtteranceRequest(uint64 request_id, uint64 voice_input_id, bool auto_finish_on_silence, uint32 max_utterance_ms)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateBeginUtteranceRequest(Fbb, request_id, voice_input_id, auto_finish_on_silence, max_utterance_ms);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_BeginUtteranceRequest);
}

TArray<uint8>
BuildEndUtteranceRequest(uint64 request_id, uint64 voice_input_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateEndUtteranceRequest(Fbb, request_id, voice_input_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_EndUtteranceRequest);
}

TArray<uint8>
BuildCancelUtteranceRequest(uint64 request_id, uint64 voice_input_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateCancelUtteranceRequest(Fbb, request_id, voice_input_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_CancelUtteranceRequest);
}

TArray<uint8>
BuildDestroyVoiceInputRequest(uint64 request_id, uint64 voice_input_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateDestroyVoiceInputRequest(Fbb, request_id, voice_input_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_DestroyVoiceInputRequest);
}

TArray<uint8>
BuildDeleteModelRequest(uint64 request_id, const FString& model_name)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> model_name_off;
	if (!model_name.IsEmpty())
		model_name_off = Fbb.CreateString(FStringToUtf8(model_name));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateDeleteModelRequest(Fbb, request_id, model_name_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_DeleteModelRequest);
}

TArray<uint8>
BuildDebugCommandRequest(uint64 request_id, const FString& command, const FString& payload)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> command_off;
	if (!command.IsEmpty())
		command_off = Fbb.CreateString(FStringToUtf8(command));
	flatbuffers::Offset<flatbuffers::String> payload_off;
	if (!payload.IsEmpty())
		payload_off = Fbb.CreateString(FStringToUtf8(payload));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateDebugCommandRequest(Fbb, request_id, command_off, payload_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_DebugCommandRequest);
}

TArray<uint8>
BuildCancelRequest(uint64 request_id, uint64 agent_id, ::Tryll::CancelMode mode)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateCancelRequest(Fbb, request_id, agent_id, mode);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_CancelRequest);
}

TArray<uint8>
BuildGetAgentVariablesRequest(uint64 request_id, uint64 agent_id, const TArray<FString>& names)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	TArray<flatbuffers::Offset<flatbuffers::String>> names_str_offs;
	names_str_offs.Reserve(names.Num());
	for (const FString& _s : names)
		names_str_offs.Add(Fbb.CreateString(FStringToUtf8(_s)));
	flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> names_off;
	if (names.Num() > 0)
		names_off = Fbb.CreateVector(names_str_offs.GetData(), static_cast<size_t>(names_str_offs.Num()));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateGetAgentVariablesRequest(Fbb, request_id, agent_id, names_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_GetAgentVariablesRequest);
}

TArray<uint8>
BuildPrefillAgentKvCacheRequest(uint64 request_id, uint64 agent_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreatePrefillAgentKvCacheRequest(Fbb, request_id, agent_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_PrefillAgentKvCacheRequest);
}

TArray<uint8>
BuildEvictAgentKvCacheRequest(uint64 request_id, uint64 agent_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateEvictAgentKvCacheRequest(Fbb, request_id, agent_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_EvictAgentKvCacheRequest);
}

TArray<uint8>
BuildGetAgentKvCacheStatusRequest(uint64 request_id, uint64 agent_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateGetAgentKvCacheStatusRequest(Fbb, request_id, agent_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_GetAgentKvCacheStatusRequest);
}

TArray<uint8>
BuildRemoveInteractionsFromEndRequest(uint64 request_id, uint64 agent_id, uint32 count)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateRemoveInteractionsFromEndRequest(Fbb, request_id, agent_id, count);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_RemoveInteractionsFromEndRequest);
}

TArray<uint8>
BuildSetInferenceThrottle(float throttle)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateSetInferenceThrottle(Fbb, throttle);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_SetInferenceThrottle);
}

TArray<uint8>
BuildSetSessionLocaleRequest(uint64 request_id, const FString& locale)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> locale_off;
	if (!locale.IsEmpty())
		locale_off = Fbb.CreateString(FStringToUtf8(locale));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateSetSessionLocaleRequest(Fbb, request_id, locale_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_SetSessionLocaleRequest);
}

TArray<uint8>
BuildExportDialogRequest(uint64 request_id, uint64 agent_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateExportDialogRequest(Fbb, request_id, agent_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_ExportDialogRequest);
}

TArray<uint8>
BuildGetMemoryConsumptionRequest(uint64 request_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateGetMemoryConsumptionRequest(Fbb, request_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_GetMemoryConsumptionRequest);
}

TArray<uint8>
BuildEnsureMutableKbRequest(uint64 request_id, const FString& name, const FString& schema)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));
	flatbuffers::Offset<flatbuffers::String> schema_off;
	if (!schema.IsEmpty())
		schema_off = Fbb.CreateString(FStringToUtf8(schema));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateEnsureMutableKbRequest(Fbb, request_id, name_off, schema_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_EnsureMutableKbRequest);
}

TArray<uint8>
BuildEnumerateMutableKbsRequest(uint64 request_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateEnumerateMutableKbsRequest(Fbb, request_id);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_EnumerateMutableKbsRequest);
}

TArray<uint8>
BuildMutableKbWriteRequest(uint64 request_id, const FString& name, const FString& schema, const TArray<::Tryll::MutableKbUpsertT>& records)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));
	flatbuffers::Offset<flatbuffers::String> schema_off;
	if (!schema.IsEmpty())
		schema_off = Fbb.CreateString(FStringToUtf8(schema));
	TArray<flatbuffers::Offset<::Tryll::MutableKbUpsert>> records_offs;
	records_offs.Reserve(records.Num());
	for (const auto& _item : records)
		records_offs.Add(::Tryll::MutableKbUpsert::Pack(Fbb, &_item));
	flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<::Tryll::MutableKbUpsert>>> records_off;
	if (records.Num() > 0)
		records_off = Fbb.CreateVector(records_offs.GetData(), static_cast<size_t>(records_offs.Num()));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateMutableKbWriteRequest(Fbb, request_id, name_off, schema_off, records_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_MutableKbWriteRequest);
}

TArray<uint8>
BuildMutableKbDeleteRequest(uint64 request_id, const FString& name, const TArray<FString>& ids)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));
	TArray<flatbuffers::Offset<flatbuffers::String>> ids_str_offs;
	ids_str_offs.Reserve(ids.Num());
	for (const FString& _s : ids)
		ids_str_offs.Add(Fbb.CreateString(FStringToUtf8(_s)));
	flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> ids_off;
	if (ids.Num() > 0)
		ids_off = Fbb.CreateVector(ids_str_offs.GetData(), static_cast<size_t>(ids_str_offs.Num()));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateMutableKbDeleteRequest(Fbb, request_id, name_off, ids_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_MutableKbDeleteRequest);
}

TArray<uint8>
BuildMutableKbClearRequest(uint64 request_id, const FString& name)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateMutableKbClearRequest(Fbb, request_id, name_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_MutableKbClearRequest);
}

TArray<uint8>
BuildMutableKbQueryRequest(uint64 request_id, const FString& name, const FString& query_text, uint32 top_k, ::Tryll::RetrievalMode retrieval_mode)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));
	flatbuffers::Offset<flatbuffers::String> query_text_off;
	if (!query_text.IsEmpty())
		query_text_off = Fbb.CreateString(FStringToUtf8(query_text));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateMutableKbQueryRequest(Fbb, request_id, name_off, query_text_off, top_k, retrieval_mode);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_MutableKbQueryRequest);
}

TArray<uint8>
BuildMutableKbGetRequest(uint64 request_id, const FString& name, const FString& id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));
	flatbuffers::Offset<flatbuffers::String> id_off;
	if (!id.IsEmpty())
		id_off = Fbb.CreateString(FStringToUtf8(id));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateMutableKbGetRequest(Fbb, request_id, name_off, id_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_MutableKbGetRequest);
}

TArray<uint8>
BuildMutableKbListRequest(uint64 request_id, const FString& name, uint32 offset, uint32 limit)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateMutableKbListRequest(Fbb, request_id, name_off, offset, limit);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_MutableKbListRequest);
}

TArray<uint8>
BuildMutableKbStatsRequest(uint64 request_id, const FString& name)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateMutableKbStatsRequest(Fbb, request_id, name_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_MutableKbStatsRequest);
}

TArray<uint8>
BuildExportMutableKbRequest(uint64 request_id, const FString& name)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> name_off;
	if (!name.IsEmpty())
		name_off = Fbb.CreateString(FStringToUtf8(name));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateExportMutableKbRequest(Fbb, request_id, name_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_ExportMutableKbRequest);
}

TArray<uint8>
BuildSaveGameMemoryRequest(uint64 request_id, const FString& path, const FString& save_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> path_off;
	if (!path.IsEmpty())
		path_off = Fbb.CreateString(FStringToUtf8(path));
	flatbuffers::Offset<flatbuffers::String> save_id_off;
	if (!save_id.IsEmpty())
		save_id_off = Fbb.CreateString(FStringToUtf8(save_id));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateSaveGameMemoryRequest(Fbb, request_id, path_off, save_id_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_SaveGameMemoryRequest);
}

TArray<uint8>
BuildLoadGameMemoryRequest(uint64 request_id, const FString& path, ::Tryll::MutableKbCompatibility compatibility, ::Tryll::MutableKbEmbedTiming embed_timing)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> path_off;
	if (!path.IsEmpty())
		path_off = Fbb.CreateString(FStringToUtf8(path));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateLoadGameMemoryRequest(Fbb, request_id, path_off, compatibility, embed_timing);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_LoadGameMemoryRequest);
}

TArray<uint8>
BuildSaveDialogRequest(uint64 request_id, uint64 agent_id, const FString& path, const FString& save_id)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> path_off;
	if (!path.IsEmpty())
		path_off = Fbb.CreateString(FStringToUtf8(path));
	flatbuffers::Offset<flatbuffers::String> save_id_off;
	if (!save_id.IsEmpty())
		save_id_off = Fbb.CreateString(FStringToUtf8(save_id));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateSaveDialogRequest(Fbb, request_id, agent_id, path_off, save_id_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_SaveDialogRequest);
}

TArray<uint8>
BuildLoadDialogRequest(uint64 request_id, uint64 agent_id, const FString& path, ::Tryll::DialogCompatibility compatibility)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> path_off;
	if (!path.IsEmpty())
		path_off = Fbb.CreateString(FStringToUtf8(path));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateLoadDialogRequest(Fbb, request_id, agent_id, path_off, compatibility);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_LoadDialogRequest);
}

TArray<uint8>
BuildSetAgentVoiceRequest(uint64 request_id, uint64 agent_id, const ::Tryll::VoiceProfileT& voice)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	auto voice_off = ::Tryll::VoiceProfile::Pack(Fbb, &voice);

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateSetAgentVoiceRequest(Fbb, request_id, agent_id, voice_off);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_SetAgentVoiceRequest);
}

TArray<uint8>
BuildSetAgentDeliveryModeRequest(uint64 request_id, uint64 agent_id, const FString& mode, float intensity)
{
	flatbuffers::FlatBufferBuilder Fbb;

	// ── Pre-encode string / vector / sub-table offsets ────────────────────
	flatbuffers::Offset<flatbuffers::String> mode_off;
	if (!mode.IsEmpty())
		mode_off = Fbb.CreateString(FStringToUtf8(mode));

	// ── Create message body ───────────────────────────────────────────────
	auto Body = ::Tryll::CreateSetAgentDeliveryModeRequest(Fbb, request_id, agent_id, mode_off, intensity);
	return WrapMessage(Fbb, Body, ::Tryll::MessageBody_SetAgentDeliveryModeRequest);
}


} // namespace Tryll::Client
