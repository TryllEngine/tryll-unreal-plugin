// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/messages.fbs  +  tools/codegen/templates/protocol_call_stubs_unreal.cpp.j2
// See: docs/dev/codegen.md
//
// One stub per request except SendMessage and DownloadModel (streaming; the
// wrapper calls Call) and AudioBuffer (manual fire-and-forget). A stub encodes
// on the caller, registers the pending handler, and posts the unpacked reply.
// The note callback runs before the frame is queued.

#include "TryllCalls.gen.h"

#include "ClientCodec.h"
#include "TryllConnection.h"
#include "TryllConnectionCalls.h"

#include "CodegenFingerprint.gen.h"

#include <string_view>
namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_TryllCalls_gen = void;
}



namespace Tryll::Client::Calls {


void SetInferenceThrottle(
	FTryllConnection& Conn, float throttle)
{
	Conn.Send(BuildSetInferenceThrottle(throttle));
}


void CreateSession(
	FTryllConnection& Conn,
	const void* Owner, const FString& game_name, const FString& locale, const ::Tryll::SessionModelConfigT& model_config, TUniqueFunction<void(TUniquePtr<::Tryll::CreateSessionResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildCreateSessionRequest(RequestId, game_name, locale, model_config);
	CallUnary<::Tryll::CreateSessionResponse, ::Tryll::CreateSessionResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_CreateSessionRequest,
		::Tryll::MessageBody_CreateSessionResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void CreateAgent(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64)> Encode, TUniqueFunction<void(TUniquePtr<::Tryll::CreateAgentResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = Encode ? Encode(RequestId) : TArray<uint8>();
	CallUnary<::Tryll::CreateAgentResponse, ::Tryll::CreateAgentResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_CreateAgentRequest,
		::Tryll::MessageBody_CreateAgentResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void DestroyAgent(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildDestroyAgentRequest(RequestId, agent_id);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_DestroyAgentRequest, MoveTemp(Done), MoveTemp(Note));
}


void ListModels(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<void(TUniquePtr<::Tryll::ListModelsResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildListModelsRequest(RequestId);
	CallUnary<::Tryll::ListModelsResponse, ::Tryll::ListModelsResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_ListModelsRequest,
		::Tryll::MessageBody_ListModelsResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void CreateStringStorage(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, ::Tryll::StringStorageKind kind, const TArray<FString>& strings, const TArray<FString>& keys, const FString& file_path, TUniqueFunction<void(TUniquePtr<::Tryll::CreateStringStorageResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildCreateStringStorageRequest(RequestId, name, kind, strings, keys, file_path);
	CallUnary<::Tryll::CreateStringStorageResponse, ::Tryll::CreateStringStorageResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_CreateStringStorageRequest,
		::Tryll::MessageBody_CreateStringStorageResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void DestroyStringStorage(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildDestroyStringStorageRequest(RequestId, name);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_DestroyStringStorageRequest, MoveTemp(Done), MoveTemp(Note));
}


void LoadModel(
	FTryllConnection& Conn,
	const void* Owner, const FString& model_name, ::Tryll::MemoryPlacement placement, bool placement_is_override, TUniqueFunction<void(TUniquePtr<::Tryll::LoadModelResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildLoadModelRequest(RequestId, model_name, placement, placement_is_override);
	CallUnary<::Tryll::LoadModelResponse, ::Tryll::LoadModelResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_LoadModelRequest,
		::Tryll::MessageBody_LoadModelResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void UnloadModel(
	FTryllConnection& Conn,
	const void* Owner, const FString& model_name, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildUnloadModelRequest(RequestId, model_name);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_UnloadModelRequest, MoveTemp(Done), MoveTemp(Note));
}


void CreateEmbeddedStringStorage(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const FString& config_path, const TArray<FString>& strings, const FString& embedding_model, TUniqueFunction<void(TUniquePtr<::Tryll::CreateEmbeddedStringStorageResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildCreateEmbeddedStringStorageRequest(RequestId, name, config_path, strings, embedding_model);
	CallUnary<::Tryll::CreateEmbeddedStringStorageResponse, ::Tryll::CreateEmbeddedStringStorageResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_CreateEmbeddedStringStorageRequest,
		::Tryll::MessageBody_CreateEmbeddedStringStorageResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void DestroyEmbeddedStringStorage(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildDestroyEmbeddedStringStorageRequest(RequestId, name);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_DestroyEmbeddedStringStorageRequest, MoveTemp(Done), MoveTemp(Note));
}


void ChangeAgentParam(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64)> Encode, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = Encode ? Encode(RequestId) : TArray<uint8>();
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_ChangeAgentParamRequest, MoveTemp(Done), MoveTemp(Note));
}


void CreateVoiceInput(
	FTryllConnection& Conn,
	const void* Owner, const FString& model_name, const ::Tryll::WireAudioFormatT& input_format, float vad_threshold, uint32 vad_min_silence_ms, uint32 vad_speech_pad_ms, const FString& hotwords_storage_path, float hotwords_score, TUniqueFunction<void(TUniquePtr<::Tryll::CreateVoiceInputResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildCreateVoiceInputRequest(RequestId, model_name, input_format, vad_threshold, vad_min_silence_ms, vad_speech_pad_ms, hotwords_storage_path, hotwords_score);
	CallUnary<::Tryll::CreateVoiceInputResponse, ::Tryll::CreateVoiceInputResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_CreateVoiceInputRequest,
		::Tryll::MessageBody_CreateVoiceInputResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void BeginUtterance(
	FTryllConnection& Conn,
	const void* Owner, uint64 voice_input_id, bool auto_finish_on_silence, uint32 max_utterance_ms, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildBeginUtteranceRequest(RequestId, voice_input_id, auto_finish_on_silence, max_utterance_ms);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_BeginUtteranceRequest, MoveTemp(Done), MoveTemp(Note));
}


void EndUtterance(
	FTryllConnection& Conn,
	const void* Owner, uint64 voice_input_id, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildEndUtteranceRequest(RequestId, voice_input_id);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_EndUtteranceRequest, MoveTemp(Done), MoveTemp(Note));
}


void CancelUtterance(
	FTryllConnection& Conn,
	const void* Owner, uint64 voice_input_id, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildCancelUtteranceRequest(RequestId, voice_input_id);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_CancelUtteranceRequest, MoveTemp(Done), MoveTemp(Note));
}


void DestroyVoiceInput(
	FTryllConnection& Conn,
	const void* Owner, uint64 voice_input_id, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildDestroyVoiceInputRequest(RequestId, voice_input_id);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_DestroyVoiceInputRequest, MoveTemp(Done), MoveTemp(Note));
}


void DeleteModel(
	FTryllConnection& Conn,
	const void* Owner, const FString& model_name, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildDeleteModelRequest(RequestId, model_name);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_DeleteModelRequest, MoveTemp(Done), MoveTemp(Note));
}


void DebugCommand(
	FTryllConnection& Conn,
	const void* Owner, const FString& command, const FString& payload, TUniqueFunction<void(TUniquePtr<::Tryll::DebugCommandResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildDebugCommandRequest(RequestId, command, payload);
	CallUnary<::Tryll::DebugCommandResponse, ::Tryll::DebugCommandResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_DebugCommandRequest,
		::Tryll::MessageBody_DebugCommandResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void Cancel(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, ::Tryll::CancelMode mode, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildCancelRequest(RequestId, agent_id, mode);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_CancelRequest, MoveTemp(Done), MoveTemp(Note));
}


void ResumeAgent(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64)> Encode, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = Encode ? Encode(RequestId) : TArray<uint8>();
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_ResumeAgentRequest, MoveTemp(Done), MoveTemp(Note));
}


void UpdateAgentVariables(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64)> Encode, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = Encode ? Encode(RequestId) : TArray<uint8>();
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_UpdateAgentVariablesRequest, MoveTemp(Done), MoveTemp(Note));
}


void GetAgentVariables(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, const TArray<FString>& names, TUniqueFunction<void(TUniquePtr<::Tryll::GetAgentVariablesResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildGetAgentVariablesRequest(RequestId, agent_id, names);
	CallUnary<::Tryll::GetAgentVariablesResponse, ::Tryll::GetAgentVariablesResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_GetAgentVariablesRequest,
		::Tryll::MessageBody_GetAgentVariablesResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void PrefillAgentKvCache(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, TUniqueFunction<void(TUniquePtr<::Tryll::PrefillAgentKvCacheResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildPrefillAgentKvCacheRequest(RequestId, agent_id);
	CallUnary<::Tryll::PrefillAgentKvCacheResponse, ::Tryll::PrefillAgentKvCacheResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_PrefillAgentKvCacheRequest,
		::Tryll::MessageBody_PrefillAgentKvCacheResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void EvictAgentKvCache(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, TUniqueFunction<void(TUniquePtr<::Tryll::EvictAgentKvCacheResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildEvictAgentKvCacheRequest(RequestId, agent_id);
	CallUnary<::Tryll::EvictAgentKvCacheResponse, ::Tryll::EvictAgentKvCacheResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_EvictAgentKvCacheRequest,
		::Tryll::MessageBody_EvictAgentKvCacheResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void GetAgentKvCacheStatus(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, TUniqueFunction<void(TUniquePtr<::Tryll::GetAgentKvCacheStatusResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildGetAgentKvCacheStatusRequest(RequestId, agent_id);
	CallUnary<::Tryll::GetAgentKvCacheStatusResponse, ::Tryll::GetAgentKvCacheStatusResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_GetAgentKvCacheStatusRequest,
		::Tryll::MessageBody_GetAgentKvCacheStatusResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void AppendInteractions(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64)> Encode, TUniqueFunction<void(TUniquePtr<::Tryll::AppendInteractionsResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = Encode ? Encode(RequestId) : TArray<uint8>();
	CallUnary<::Tryll::AppendInteractionsResponse, ::Tryll::AppendInteractionsResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_AppendInteractionsRequest,
		::Tryll::MessageBody_AppendInteractionsResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void RemoveInteractionsFromEnd(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, uint32 count, TUniqueFunction<void(TUniquePtr<::Tryll::RemoveInteractionsFromEndResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildRemoveInteractionsFromEndRequest(RequestId, agent_id, count);
	CallUnary<::Tryll::RemoveInteractionsFromEndResponse, ::Tryll::RemoveInteractionsFromEndResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_RemoveInteractionsFromEndRequest,
		::Tryll::MessageBody_RemoveInteractionsFromEndResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void SetSessionLocale(
	FTryllConnection& Conn,
	const void* Owner, const FString& locale, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildSetSessionLocaleRequest(RequestId, locale);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_SetSessionLocaleRequest, MoveTemp(Done), MoveTemp(Note));
}


void ExportDialog(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, TUniqueFunction<void(TUniquePtr<::Tryll::ExportDialogResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildExportDialogRequest(RequestId, agent_id);
	CallUnary<::Tryll::ExportDialogResponse, ::Tryll::ExportDialogResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_ExportDialogRequest,
		::Tryll::MessageBody_ExportDialogResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void ImportDialog(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64)> Encode, TUniqueFunction<void(TUniquePtr<::Tryll::ImportDialogResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = Encode ? Encode(RequestId) : TArray<uint8>();
	CallUnary<::Tryll::ImportDialogResponse, ::Tryll::ImportDialogResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_ImportDialogRequest,
		::Tryll::MessageBody_ImportDialogResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void GetMemoryConsumption(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<void(TUniquePtr<::Tryll::GetMemoryConsumptionResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildGetMemoryConsumptionRequest(RequestId);
	CallUnary<::Tryll::GetMemoryConsumptionResponse, ::Tryll::GetMemoryConsumptionResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_GetMemoryConsumptionRequest,
		::Tryll::MessageBody_GetMemoryConsumptionResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void EnsureMutableKb(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const FString& schema, TUniqueFunction<void(TUniquePtr<::Tryll::EnsureMutableKbResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildEnsureMutableKbRequest(RequestId, name, schema);
	CallUnary<::Tryll::EnsureMutableKbResponse, ::Tryll::EnsureMutableKbResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_EnsureMutableKbRequest,
		::Tryll::MessageBody_EnsureMutableKbResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void EnumerateMutableKbs(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<void(TUniquePtr<::Tryll::EnumerateMutableKbsResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildEnumerateMutableKbsRequest(RequestId);
	CallUnary<::Tryll::EnumerateMutableKbsResponse, ::Tryll::EnumerateMutableKbsResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_EnumerateMutableKbsRequest,
		::Tryll::MessageBody_EnumerateMutableKbsResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void MutableKbWrite(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const FString& schema, const TArray<::Tryll::MutableKbUpsertT>& records, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbWriteResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildMutableKbWriteRequest(RequestId, name, schema, records);
	CallUnary<::Tryll::MutableKbWriteResponse, ::Tryll::MutableKbWriteResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_MutableKbWriteRequest,
		::Tryll::MessageBody_MutableKbWriteResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void MutableKbDelete(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const TArray<FString>& ids, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbDeleteResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildMutableKbDeleteRequest(RequestId, name, ids);
	CallUnary<::Tryll::MutableKbDeleteResponse, ::Tryll::MutableKbDeleteResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_MutableKbDeleteRequest,
		::Tryll::MessageBody_MutableKbDeleteResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void MutableKbClear(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbClearResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildMutableKbClearRequest(RequestId, name);
	CallUnary<::Tryll::MutableKbClearResponse, ::Tryll::MutableKbClearResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_MutableKbClearRequest,
		::Tryll::MessageBody_MutableKbClearResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void MutableKbQuery(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const FString& query_text, uint32 top_k, ::Tryll::RetrievalMode retrieval_mode, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbQueryResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildMutableKbQueryRequest(RequestId, name, query_text, top_k, retrieval_mode);
	CallUnary<::Tryll::MutableKbQueryResponse, ::Tryll::MutableKbQueryResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_MutableKbQueryRequest,
		::Tryll::MessageBody_MutableKbQueryResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void MutableKbGet(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const FString& id, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbGetResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildMutableKbGetRequest(RequestId, name, id);
	CallUnary<::Tryll::MutableKbGetResponse, ::Tryll::MutableKbGetResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_MutableKbGetRequest,
		::Tryll::MessageBody_MutableKbGetResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void MutableKbList(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, uint32 offset, uint32 limit, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbListResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildMutableKbListRequest(RequestId, name, offset, limit);
	CallUnary<::Tryll::MutableKbListResponse, ::Tryll::MutableKbListResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_MutableKbListRequest,
		::Tryll::MessageBody_MutableKbListResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void MutableKbStats(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbStatsResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildMutableKbStatsRequest(RequestId, name);
	CallUnary<::Tryll::MutableKbStatsResponse, ::Tryll::MutableKbStatsResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_MutableKbStatsRequest,
		::Tryll::MessageBody_MutableKbStatsResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void ExportMutableKb(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, TUniqueFunction<void(TUniquePtr<::Tryll::ExportMutableKbResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildExportMutableKbRequest(RequestId, name);
	CallUnary<::Tryll::ExportMutableKbResponse, ::Tryll::ExportMutableKbResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_ExportMutableKbRequest,
		::Tryll::MessageBody_ExportMutableKbResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void ImportMutableKb(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64)> Encode, TUniqueFunction<void(TUniquePtr<::Tryll::ImportMutableKbResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = Encode ? Encode(RequestId) : TArray<uint8>();
	CallUnary<::Tryll::ImportMutableKbResponse, ::Tryll::ImportMutableKbResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_ImportMutableKbRequest,
		::Tryll::MessageBody_ImportMutableKbResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void SaveGameMemory(
	FTryllConnection& Conn,
	const void* Owner, const FString& path, const FString& save_id, TUniqueFunction<void(TUniquePtr<::Tryll::SaveGameMemoryResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildSaveGameMemoryRequest(RequestId, path, save_id);
	CallUnary<::Tryll::SaveGameMemoryResponse, ::Tryll::SaveGameMemoryResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_SaveGameMemoryRequest,
		::Tryll::MessageBody_SaveGameMemoryResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void LoadGameMemory(
	FTryllConnection& Conn,
	const void* Owner, const FString& path, ::Tryll::MutableKbCompatibility compatibility, ::Tryll::MutableKbEmbedTiming embed_timing, TUniqueFunction<void(TUniquePtr<::Tryll::LoadGameMemoryResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildLoadGameMemoryRequest(RequestId, path, compatibility, embed_timing);
	CallUnary<::Tryll::LoadGameMemoryResponse, ::Tryll::LoadGameMemoryResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_LoadGameMemoryRequest,
		::Tryll::MessageBody_LoadGameMemoryResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void SaveDialog(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, const FString& path, const FString& save_id, TUniqueFunction<void(TUniquePtr<::Tryll::SaveDialogResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildSaveDialogRequest(RequestId, agent_id, path, save_id);
	CallUnary<::Tryll::SaveDialogResponse, ::Tryll::SaveDialogResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_SaveDialogRequest,
		::Tryll::MessageBody_SaveDialogResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void LoadDialog(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, const FString& path, ::Tryll::DialogCompatibility compatibility, TUniqueFunction<void(TUniquePtr<::Tryll::LoadDialogResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildLoadDialogRequest(RequestId, agent_id, path, compatibility);
	CallUnary<::Tryll::LoadDialogResponse, ::Tryll::LoadDialogResponseT>(
		Conn,
		Owner,
		RequestId,
		MoveTemp(Frame),
		::Tryll::MessageBody_LoadDialogRequest,
		::Tryll::MessageBody_LoadDialogResponse,
		MoveTemp(Done),
		MoveTemp(Note));
}


void SetAgentVoice(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, const ::Tryll::VoiceProfileT& voice, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildSetAgentVoiceRequest(RequestId, agent_id, voice);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_SetAgentVoiceRequest, MoveTemp(Done), MoveTemp(Note));
}


void SetAgentDeliveryMode(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, const FString& mode, float intensity, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note)
{
	const uint64 RequestId = Conn.AllocateRequestId();
	TArray<uint8> Frame = BuildSetAgentDeliveryModeRequest(RequestId, agent_id, mode, intensity);
	CallAck(Conn, Owner, RequestId, MoveTemp(Frame), ::Tryll::MessageBody_SetAgentDeliveryModeRequest, MoveTemp(Done), MoveTemp(Note));
}
} // namespace Tryll::Client::Calls
