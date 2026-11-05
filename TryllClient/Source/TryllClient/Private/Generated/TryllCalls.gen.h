// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/messages.fbs  +  tools/codegen/templates/protocol_call_stubs_unreal.h.j2
// See: docs/dev/codegen.md
//
// Declarations only. No anonymous-namespace helpers: Unreal unity builds merge
// headers into one translation unit. The fingerprint check lives in the .cpp.

#pragma once

#include "CoreMinimal.h"
#include "TryllError.h"

#include "messages_generated.h"

#include <memory>

class FTryllConnection;



namespace Tryll::Client::Calls {

using FTryllCallNote = TUniqueFunction<void(uint64 /*RequestId*/)>;



TRYLLCLIENT_API void SetInferenceThrottle(
	FTryllConnection& Conn, float throttle);


/// CreateSession.

TRYLLCLIENT_API void CreateSession(
	FTryllConnection& Conn,
	const void* Owner, const FString& game_name, const FString& locale, const ::Tryll::SessionModelConfigT& model_config, TUniqueFunction<void(TUniquePtr<::Tryll::CreateSessionResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// CreateAgent.

TRYLLCLIENT_API void CreateAgent(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64 /*RequestId*/)> Encode, TUniqueFunction<void(TUniquePtr<::Tryll::CreateAgentResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// DestroyAgent.

TRYLLCLIENT_API void DestroyAgent(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// ListModels.

TRYLLCLIENT_API void ListModels(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<void(TUniquePtr<::Tryll::ListModelsResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// CreateStringStorage.

TRYLLCLIENT_API void CreateStringStorage(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, ::Tryll::StringStorageKind kind, const TArray<FString>& strings, const TArray<FString>& keys, const FString& file_path, TUniqueFunction<void(TUniquePtr<::Tryll::CreateStringStorageResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// DestroyStringStorage.

TRYLLCLIENT_API void DestroyStringStorage(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// LoadModel.

TRYLLCLIENT_API void LoadModel(
	FTryllConnection& Conn,
	const void* Owner, const FString& model_name, ::Tryll::MemoryPlacement placement, bool placement_is_override, TUniqueFunction<void(TUniquePtr<::Tryll::LoadModelResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// UnloadModel.

TRYLLCLIENT_API void UnloadModel(
	FTryllConnection& Conn,
	const void* Owner, const FString& model_name, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// CreateEmbeddedStringStorage.

TRYLLCLIENT_API void CreateEmbeddedStringStorage(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const FString& config_path, const TArray<FString>& strings, const FString& embedding_model, TUniqueFunction<void(TUniquePtr<::Tryll::CreateEmbeddedStringStorageResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// DestroyEmbeddedStringStorage.

TRYLLCLIENT_API void DestroyEmbeddedStringStorage(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// ChangeAgentParam.

TRYLLCLIENT_API void ChangeAgentParam(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64 /*RequestId*/)> Encode, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// CreateVoiceInput.

TRYLLCLIENT_API void CreateVoiceInput(
	FTryllConnection& Conn,
	const void* Owner, const FString& model_name, const ::Tryll::WireAudioFormatT& input_format, float vad_threshold, uint32 vad_min_silence_ms, uint32 vad_speech_pad_ms, const FString& hotwords_storage_path, float hotwords_score, TUniqueFunction<void(TUniquePtr<::Tryll::CreateVoiceInputResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// BeginUtterance.

TRYLLCLIENT_API void BeginUtterance(
	FTryllConnection& Conn,
	const void* Owner, uint64 voice_input_id, bool auto_finish_on_silence, uint32 max_utterance_ms, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// EndUtterance.

TRYLLCLIENT_API void EndUtterance(
	FTryllConnection& Conn,
	const void* Owner, uint64 voice_input_id, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// CancelUtterance.

TRYLLCLIENT_API void CancelUtterance(
	FTryllConnection& Conn,
	const void* Owner, uint64 voice_input_id, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// DestroyVoiceInput.

TRYLLCLIENT_API void DestroyVoiceInput(
	FTryllConnection& Conn,
	const void* Owner, uint64 voice_input_id, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// DeleteModel.

TRYLLCLIENT_API void DeleteModel(
	FTryllConnection& Conn,
	const void* Owner, const FString& model_name, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// DebugCommand. May never reply.

TRYLLCLIENT_API void DebugCommand(
	FTryllConnection& Conn,
	const void* Owner, const FString& command, const FString& payload, TUniqueFunction<void(TUniquePtr<::Tryll::DebugCommandResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// Cancel.

TRYLLCLIENT_API void Cancel(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, ::Tryll::CancelMode mode, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// ResumeAgent.

TRYLLCLIENT_API void ResumeAgent(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64 /*RequestId*/)> Encode, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// UpdateAgentVariables.

TRYLLCLIENT_API void UpdateAgentVariables(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64 /*RequestId*/)> Encode, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// GetAgentVariables.

TRYLLCLIENT_API void GetAgentVariables(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, const TArray<FString>& names, TUniqueFunction<void(TUniquePtr<::Tryll::GetAgentVariablesResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// PrefillAgentKvCache.

TRYLLCLIENT_API void PrefillAgentKvCache(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, TUniqueFunction<void(TUniquePtr<::Tryll::PrefillAgentKvCacheResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// EvictAgentKvCache.

TRYLLCLIENT_API void EvictAgentKvCache(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, TUniqueFunction<void(TUniquePtr<::Tryll::EvictAgentKvCacheResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// GetAgentKvCacheStatus.

TRYLLCLIENT_API void GetAgentKvCacheStatus(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, TUniqueFunction<void(TUniquePtr<::Tryll::GetAgentKvCacheStatusResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// AppendInteractions.

TRYLLCLIENT_API void AppendInteractions(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64 /*RequestId*/)> Encode, TUniqueFunction<void(TUniquePtr<::Tryll::AppendInteractionsResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// RemoveInteractionsFromEnd.

TRYLLCLIENT_API void RemoveInteractionsFromEnd(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, uint32 count, TUniqueFunction<void(TUniquePtr<::Tryll::RemoveInteractionsFromEndResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// SetSessionLocale.

TRYLLCLIENT_API void SetSessionLocale(
	FTryllConnection& Conn,
	const void* Owner, const FString& locale, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// ExportDialog.

TRYLLCLIENT_API void ExportDialog(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, TUniqueFunction<void(TUniquePtr<::Tryll::ExportDialogResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// ImportDialog.

TRYLLCLIENT_API void ImportDialog(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64 /*RequestId*/)> Encode, TUniqueFunction<void(TUniquePtr<::Tryll::ImportDialogResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// GetMemoryConsumption.

TRYLLCLIENT_API void GetMemoryConsumption(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<void(TUniquePtr<::Tryll::GetMemoryConsumptionResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// EnsureMutableKb.

TRYLLCLIENT_API void EnsureMutableKb(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const FString& schema, TUniqueFunction<void(TUniquePtr<::Tryll::EnsureMutableKbResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// EnumerateMutableKbs.

TRYLLCLIENT_API void EnumerateMutableKbs(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<void(TUniquePtr<::Tryll::EnumerateMutableKbsResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// MutableKbWrite.

TRYLLCLIENT_API void MutableKbWrite(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const FString& schema, const TArray<::Tryll::MutableKbUpsertT>& records, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbWriteResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// MutableKbDelete.

TRYLLCLIENT_API void MutableKbDelete(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const TArray<FString>& ids, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbDeleteResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// MutableKbClear.

TRYLLCLIENT_API void MutableKbClear(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbClearResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// MutableKbQuery.

TRYLLCLIENT_API void MutableKbQuery(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const FString& query_text, uint32 top_k, ::Tryll::RetrievalMode retrieval_mode, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbQueryResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// MutableKbGet.

TRYLLCLIENT_API void MutableKbGet(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, const FString& id, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbGetResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// MutableKbList.

TRYLLCLIENT_API void MutableKbList(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, uint32 offset, uint32 limit, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbListResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// MutableKbStats.

TRYLLCLIENT_API void MutableKbStats(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, TUniqueFunction<void(TUniquePtr<::Tryll::MutableKbStatsResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// ExportMutableKb.

TRYLLCLIENT_API void ExportMutableKb(
	FTryllConnection& Conn,
	const void* Owner, const FString& name, TUniqueFunction<void(TUniquePtr<::Tryll::ExportMutableKbResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// ImportMutableKb.

TRYLLCLIENT_API void ImportMutableKb(
	FTryllConnection& Conn,
	const void* Owner, TUniqueFunction<TArray<uint8>(uint64 /*RequestId*/)> Encode, TUniqueFunction<void(TUniquePtr<::Tryll::ImportMutableKbResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// SaveGameMemory.

TRYLLCLIENT_API void SaveGameMemory(
	FTryllConnection& Conn,
	const void* Owner, const FString& path, const FString& save_id, TUniqueFunction<void(TUniquePtr<::Tryll::SaveGameMemoryResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// LoadGameMemory.

TRYLLCLIENT_API void LoadGameMemory(
	FTryllConnection& Conn,
	const void* Owner, const FString& path, ::Tryll::MutableKbCompatibility compatibility, ::Tryll::MutableKbEmbedTiming embed_timing, TUniqueFunction<void(TUniquePtr<::Tryll::LoadGameMemoryResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// SaveDialog.

TRYLLCLIENT_API void SaveDialog(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, const FString& path, const FString& save_id, TUniqueFunction<void(TUniquePtr<::Tryll::SaveDialogResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// LoadDialog.

TRYLLCLIENT_API void LoadDialog(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, const FString& path, ::Tryll::DialogCompatibility compatibility, TUniqueFunction<void(TUniquePtr<::Tryll::LoadDialogResponseT>, const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// SetAgentVoice.

TRYLLCLIENT_API void SetAgentVoice(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, const ::Tryll::VoiceProfileT& voice, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});


/// SetAgentDeliveryMode.

TRYLLCLIENT_API void SetAgentDeliveryMode(
	FTryllConnection& Conn,
	const void* Owner, uint64 agent_id, const FString& mode, float intensity, TUniqueFunction<void(const FTryllError&)> Done,
	FTryllCallNote Note = {});

} // namespace Tryll::Client::Calls
