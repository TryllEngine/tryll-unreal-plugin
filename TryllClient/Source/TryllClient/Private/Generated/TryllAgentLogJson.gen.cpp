// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/*.fbs + tools/codegen/agent_log_emit.py
// See: docs/dev/codegen.md

#include "TryllAgentLogJson.gen.h"
#include "nodes_generated.h"
#include "dialog_snapshot_generated.h"
#include "mkb_snapshot_generated.h"
#include "voice_generated.h"
#include "common_enums_generated.h"

#include <cstring>
#include <map>
#include <string>
#include <utility>

namespace Tryll::AgentLog
{
namespace
{
std::map<std::string, FTryllOmittedField> Index(const std::vector<FTryllOmittedField>& Items)
{
    std::map<std::string, FTryllOmittedField> Map;
    for (const auto& Item : Items) Map.emplace(Item.Path, Item);
    return Map;
}

void Write_Tryll_Ack(FTryllJsonWriter& W, const ::Tryll::Ack* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_AgentVariable(FTryllJsonWriter& W, const ::Tryll::AgentVariable* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_AnswerText(FTryllJsonWriter& W, const ::Tryll::AnswerText* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
std::string Label_Tryll_AnswerText(const ::Tryll::AnswerText* T);
void Write_Tryll_AppendInteractionsRequest(FTryllJsonWriter& W, const ::Tryll::AppendInteractionsRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_AppendInteractionsResponse(FTryllJsonWriter& W, const ::Tryll::AppendInteractionsResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_AudioBuffer(FTryllJsonWriter& W, const ::Tryll::AudioBuffer* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::AudioBuffer> Pack_Tryll_AudioBuffer(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::AudioBuffer* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_BeginUtteranceRequest(FTryllJsonWriter& W, const ::Tryll::BeginUtteranceRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CancelRequest(FTryllJsonWriter& W, const ::Tryll::CancelRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CancelUtteranceRequest(FTryllJsonWriter& W, const ::Tryll::CancelUtteranceRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ChangeAgentParamRequest(FTryllJsonWriter& W, const ::Tryll::ChangeAgentParamRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
std::string Label_Tryll_ChangeAgentParamRequest(const ::Tryll::ChangeAgentParamRequest* T);
void Write_Tryll_ConnectionReady(FTryllJsonWriter& W, const ::Tryll::ConnectionReady* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CreateAgentRequest(FTryllJsonWriter& W, const ::Tryll::CreateAgentRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CreateAgentResponse(FTryllJsonWriter& W, const ::Tryll::CreateAgentResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CreateEmbeddedStringStorageRequest(FTryllJsonWriter& W, const ::Tryll::CreateEmbeddedStringStorageRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::CreateEmbeddedStringStorageRequest> Pack_Tryll_CreateEmbeddedStringStorageRequest(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::CreateEmbeddedStringStorageRequest* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CreateEmbeddedStringStorageResponse(FTryllJsonWriter& W, const ::Tryll::CreateEmbeddedStringStorageResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CreateSessionRequest(FTryllJsonWriter& W, const ::Tryll::CreateSessionRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CreateSessionResponse(FTryllJsonWriter& W, const ::Tryll::CreateSessionResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CreateStringStorageRequest(FTryllJsonWriter& W, const ::Tryll::CreateStringStorageRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::CreateStringStorageRequest> Pack_Tryll_CreateStringStorageRequest(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::CreateStringStorageRequest* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CreateStringStorageResponse(FTryllJsonWriter& W, const ::Tryll::CreateStringStorageResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CreateVoiceInputRequest(FTryllJsonWriter& W, const ::Tryll::CreateVoiceInputRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_CreateVoiceInputResponse(FTryllJsonWriter& W, const ::Tryll::CreateVoiceInputResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DebugCommandRequest(FTryllJsonWriter& W, const ::Tryll::DebugCommandRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DebugCommandResponse(FTryllJsonWriter& W, const ::Tryll::DebugCommandResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DeleteModelRequest(FTryllJsonWriter& W, const ::Tryll::DeleteModelRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DeliveryMode(FTryllJsonWriter& W, const ::Tryll::DeliveryMode* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DestroyAgentRequest(FTryllJsonWriter& W, const ::Tryll::DestroyAgentRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DestroyEmbeddedStringStorageRequest(FTryllJsonWriter& W, const ::Tryll::DestroyEmbeddedStringStorageRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DestroyStringStorageRequest(FTryllJsonWriter& W, const ::Tryll::DestroyStringStorageRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DestroyVoiceInputRequest(FTryllJsonWriter& W, const ::Tryll::DestroyVoiceInputRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DialogInteraction(FTryllJsonWriter& W, const ::Tryll::DialogInteraction* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DownloadComplete(FTryllJsonWriter& W, const ::Tryll::DownloadComplete* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DownloadModelRequest(FTryllJsonWriter& W, const ::Tryll::DownloadModelRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_DownloadProgress(FTryllJsonWriter& W, const ::Tryll::DownloadProgress* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
std::string Label_Tryll_DownloadProgress(const ::Tryll::DownloadProgress* T);
void Write_Tryll_EndUtteranceRequest(FTryllJsonWriter& W, const ::Tryll::EndUtteranceRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_EngineSelection(FTryllJsonWriter& W, const ::Tryll::EngineSelection* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_EnsureMutableKbRequest(FTryllJsonWriter& W, const ::Tryll::EnsureMutableKbRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_EnsureMutableKbResponse(FTryllJsonWriter& W, const ::Tryll::EnsureMutableKbResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_EnumerateMutableKbsRequest(FTryllJsonWriter& W, const ::Tryll::EnumerateMutableKbsRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_EnumerateMutableKbsResponse(FTryllJsonWriter& W, const ::Tryll::EnumerateMutableKbsResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ErrorResponse(FTryllJsonWriter& W, const ::Tryll::ErrorResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_EvictAgentKvCacheRequest(FTryllJsonWriter& W, const ::Tryll::EvictAgentKvCacheRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_EvictAgentKvCacheResponse(FTryllJsonWriter& W, const ::Tryll::EvictAgentKvCacheResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ExportDialogRequest(FTryllJsonWriter& W, const ::Tryll::ExportDialogRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ExportDialogResponse(FTryllJsonWriter& W, const ::Tryll::ExportDialogResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::ExportDialogResponse> Pack_Tryll_ExportDialogResponse(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::ExportDialogResponse* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ExportMutableKbRequest(FTryllJsonWriter& W, const ::Tryll::ExportMutableKbRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ExportMutableKbResponse(FTryllJsonWriter& W, const ::Tryll::ExportMutableKbResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::ExportMutableKbResponse> Pack_Tryll_ExportMutableKbResponse(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::ExportMutableKbResponse* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_GameMemory(FTryllJsonWriter& W, const ::Tryll::GameMemory* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::GameMemory> Pack_Tryll_GameMemory(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::GameMemory* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_GetAgentKvCacheStatusRequest(FTryllJsonWriter& W, const ::Tryll::GetAgentKvCacheStatusRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_GetAgentKvCacheStatusResponse(FTryllJsonWriter& W, const ::Tryll::GetAgentKvCacheStatusResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_GetAgentVariablesRequest(FTryllJsonWriter& W, const ::Tryll::GetAgentVariablesRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_GetAgentVariablesResponse(FTryllJsonWriter& W, const ::Tryll::GetAgentVariablesResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_GetMemoryConsumptionRequest(FTryllJsonWriter& W, const ::Tryll::GetMemoryConsumptionRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_GetMemoryConsumptionResponse(FTryllJsonWriter& W, const ::Tryll::GetMemoryConsumptionResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_GraphDescription(FTryllJsonWriter& W, const ::Tryll::GraphDescription* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ImportDialogRequest(FTryllJsonWriter& W, const ::Tryll::ImportDialogRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::ImportDialogRequest> Pack_Tryll_ImportDialogRequest(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::ImportDialogRequest* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ImportDialogResponse(FTryllJsonWriter& W, const ::Tryll::ImportDialogResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ImportMutableKbRequest(FTryllJsonWriter& W, const ::Tryll::ImportMutableKbRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::ImportMutableKbRequest> Pack_Tryll_ImportMutableKbRequest(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::ImportMutableKbRequest* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ImportMutableKbResponse(FTryllJsonWriter& W, const ::Tryll::ImportMutableKbResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ListModelsRequest(FTryllJsonWriter& W, const ::Tryll::ListModelsRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ListModelsResponse(FTryllJsonWriter& W, const ::Tryll::ListModelsResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_LoadDialogRequest(FTryllJsonWriter& W, const ::Tryll::LoadDialogRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_LoadDialogResponse(FTryllJsonWriter& W, const ::Tryll::LoadDialogResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_LoadGameMemoryRequest(FTryllJsonWriter& W, const ::Tryll::LoadGameMemoryRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_LoadGameMemoryResponse(FTryllJsonWriter& W, const ::Tryll::LoadGameMemoryResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_LoadModelRequest(FTryllJsonWriter& W, const ::Tryll::LoadModelRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_LoadModelResponse(FTryllJsonWriter& W, const ::Tryll::LoadModelResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_LogCaptureGap(FTryllJsonWriter& W, const ::Tryll::LogCaptureGap* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_LogConnectionState(FTryllJsonWriter& W, const ::Tryll::LogConnectionState* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_LogLocalError(FTryllJsonWriter& W, const ::Tryll::LogLocalError* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MemoryComponent(FTryllJsonWriter& W, const ::Tryll::MemoryComponent* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MemoryComponentDevice(FTryllJsonWriter& W, const ::Tryll::MemoryComponentDevice* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MemoryDevice(FTryllJsonWriter& W, const ::Tryll::MemoryDevice* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MemorySnapshot(FTryllJsonWriter& W, const ::Tryll::MemorySnapshot* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ModelDependency(FTryllJsonWriter& W, const ::Tryll::ModelDependency* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ModelInfo(FTryllJsonWriter& W, const ::Tryll::ModelInfo* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ModelPlacementHint(FTryllJsonWriter& W, const ::Tryll::ModelPlacementHint* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ModelSampling(FTryllJsonWriter& W, const ::Tryll::ModelSampling* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbClearRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbClearRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbClearResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbClearResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbDeleteRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbDeleteRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbDeleteResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbDeleteResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbGetRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbGetRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbGetResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbGetResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbHit(FTryllJsonWriter& W, const ::Tryll::MutableKbHit* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbInfo(FTryllJsonWriter& W, const ::Tryll::MutableKbInfo* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbListRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbListRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbListResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbListResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbQueryRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbQueryRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbQueryResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbQueryResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbRecord(FTryllJsonWriter& W, const ::Tryll::MutableKbRecord* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbStatsRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbStatsRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbStatsResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbStatsResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbUpsert(FTryllJsonWriter& W, const ::Tryll::MutableKbUpsert* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbWriteRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbWriteRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbWriteResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbWriteResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_MutableKbWriteResult(FTryllJsonWriter& W, const ::Tryll::MutableKbWriteResult* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeDescription(FTryllJsonWriter& W, const ::Tryll::NodeDescription* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeEvent(FTryllJsonWriter& W, const ::Tryll::NodeEvent* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
std::string Label_Tryll_NodeEvent(const ::Tryll::NodeEvent* T);
void Write_Tryll_NodeParam(FTryllJsonWriter& W, const ::Tryll::NodeParam* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_BranchParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::BranchParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_CannedResponseParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::CannedResponseParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ClassifyAttributeChoice(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyAttributeChoice* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ClassifyAttributeDef(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyAttributeDef* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ClassifyAttributesLLMParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyAttributesLLMParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ClassifyIntentEntry(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyIntentEntry* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ClassifyIntentLLMParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyIntentLLMParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ClassifyIntentParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyIntentParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ContextOverrides(FTryllJsonWriter& W, const ::Tryll::NodeParams::ContextOverrides* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_GenerateAndSpeakParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::GenerateAndSpeakParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_GenerateParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::GenerateParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_InstructionParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::InstructionParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_IntentToInstructionParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::IntentToInstructionParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_LocalizableText(FTryllJsonWriter& W, const ::Tryll::NodeParams::LocalizableText* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_LoraAdapter(FTryllJsonWriter& W, const ::Tryll::NodeParams::LoraAdapter* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ModelAdaptation(FTryllJsonWriter& W, const ::Tryll::NodeParams::ModelAdaptation* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_OutputFilterOverrides(FTryllJsonWriter& W, const ::Tryll::NodeParams::OutputFilterOverrides* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_PauseParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::PauseParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_RegexGuardrailParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::RegexGuardrailParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_RetrieveParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::RetrieveParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_SamplingOverrides(FTryllJsonWriter& W, const ::Tryll::NodeParams::SamplingOverrides* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_SpeakParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::SpeakParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_SummarizeParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::SummarizeParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ToolCallParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::ToolCallParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ToolDefinition(FTryllJsonWriter& W, const ::Tryll::NodeParams::ToolDefinition* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_ToolParamDefinition(FTryllJsonWriter& W, const ::Tryll::NodeParams::ToolParamDefinition* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_TransformParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::TransformParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_NodeParams_WriteToMemoryParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::WriteToMemoryParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedComponentEntry(FTryllJsonWriter& W, const ::Tryll::PersistedComponentEntry* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedDialogSnapshot(FTryllJsonWriter& W, const ::Tryll::PersistedDialogSnapshot* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::PersistedDialogSnapshot> Pack_Tryll_PersistedDialogSnapshot(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::PersistedDialogSnapshot* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedIntention(FTryllJsonWriter& W, const ::Tryll::PersistedIntention* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedInteraction(FTryllJsonWriter& W, const ::Tryll::PersistedInteraction* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedKbRecord(FTryllJsonWriter& W, const ::Tryll::PersistedKbRecord* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedKnowledge(FTryllJsonWriter& W, const ::Tryll::PersistedKnowledge* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedKnowledgeChunk(FTryllJsonWriter& W, const ::Tryll::PersistedKnowledgeChunk* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedMutableKb(FTryllJsonWriter& W, const ::Tryll::PersistedMutableKb* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::PersistedMutableKb> Pack_Tryll_PersistedMutableKb(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::PersistedMutableKb* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedProjectionWindow(FTryllJsonWriter& W, const ::Tryll::PersistedProjectionWindow* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedSlot(FTryllJsonWriter& W, const ::Tryll::PersistedSlot* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedStringPair(FTryllJsonWriter& W, const ::Tryll::PersistedStringPair* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedSummary(FTryllJsonWriter& W, const ::Tryll::PersistedSummary* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedToolCall(FTryllJsonWriter& W, const ::Tryll::PersistedToolCall* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PersistedToolResult(FTryllJsonWriter& W, const ::Tryll::PersistedToolResult* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PrefillAgentKvCacheRequest(FTryllJsonWriter& W, const ::Tryll::PrefillAgentKvCacheRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_PrefillAgentKvCacheResponse(FTryllJsonWriter& W, const ::Tryll::PrefillAgentKvCacheResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_RemoveInteractionsFromEndRequest(FTryllJsonWriter& W, const ::Tryll::RemoveInteractionsFromEndRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_RemoveInteractionsFromEndResponse(FTryllJsonWriter& W, const ::Tryll::RemoveInteractionsFromEndResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ResumeAgentRequest(FTryllJsonWriter& W, const ::Tryll::ResumeAgentRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_SaveDialogRequest(FTryllJsonWriter& W, const ::Tryll::SaveDialogRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_SaveDialogResponse(FTryllJsonWriter& W, const ::Tryll::SaveDialogResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_SaveGameMemoryRequest(FTryllJsonWriter& W, const ::Tryll::SaveGameMemoryRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_SaveGameMemoryResponse(FTryllJsonWriter& W, const ::Tryll::SaveGameMemoryResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_SendMessageRequest(FTryllJsonWriter& W, const ::Tryll::SendMessageRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
std::string Label_Tryll_SendMessageRequest(const ::Tryll::SendMessageRequest* T);
void Write_Tryll_SessionModelConfig(FTryllJsonWriter& W, const ::Tryll::SessionModelConfig* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_SetAgentDeliveryModeRequest(FTryllJsonWriter& W, const ::Tryll::SetAgentDeliveryModeRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_SetAgentVoiceRequest(FTryllJsonWriter& W, const ::Tryll::SetAgentVoiceRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_SetInferenceThrottle(FTryllJsonWriter& W, const ::Tryll::SetInferenceThrottle* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_SetSessionLocaleRequest(FTryllJsonWriter& W, const ::Tryll::SetSessionLocaleRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_ToolResult(FTryllJsonWriter& W, const ::Tryll::ToolResult* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_TtsAudioFormatFrame(FTryllJsonWriter& W, const ::Tryll::TtsAudioFormatFrame* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_TtsAudioFrame(FTryllJsonWriter& W, const ::Tryll::TtsAudioFrame* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::TtsAudioFrame> Pack_Tryll_TtsAudioFrame(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::TtsAudioFrame* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
std::string Label_Tryll_TtsAudioFrame(const ::Tryll::TtsAudioFrame* T);
void Write_Tryll_TtsCapabilitiesInfo(FTryllJsonWriter& W, const ::Tryll::TtsCapabilitiesInfo* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_TtsInlineTagInfo(FTryllJsonWriter& W, const ::Tryll::TtsInlineTagInfo* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_TtsVisemeFrame(FTryllJsonWriter& W, const ::Tryll::TtsVisemeFrame* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::TtsVisemeFrame> Pack_Tryll_TtsVisemeFrame(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::TtsVisemeFrame* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
std::string Label_Tryll_TtsVisemeFrame(const ::Tryll::TtsVisemeFrame* T);
void Write_Tryll_TtsVoicePresetInfo(FTryllJsonWriter& W, const ::Tryll::TtsVoicePresetInfo* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_TurnComplete(FTryllJsonWriter& W, const ::Tryll::TurnComplete* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_UnloadModelRequest(FTryllJsonWriter& W, const ::Tryll::UnloadModelRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_UpdateAgentVariablesRequest(FTryllJsonWriter& W, const ::Tryll::UpdateAgentVariablesRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_VarBool(FTryllJsonWriter& W, const ::Tryll::VarBool* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_VarFloat(FTryllJsonWriter& W, const ::Tryll::VarFloat* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_VarInt(FTryllJsonWriter& W, const ::Tryll::VarInt* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_VarString(FTryllJsonWriter& W, const ::Tryll::VarString* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_VarStringSet(FTryllJsonWriter& W, const ::Tryll::VarStringSet* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_VariableUpdate(FTryllJsonWriter& W, const ::Tryll::VariableUpdate* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_VisemeEvent(FTryllJsonWriter& W, const ::Tryll::VisemeEvent* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_VisemeEventList(FTryllJsonWriter& W, const ::Tryll::VisemeEventList* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
::flatbuffers::Offset<::Tryll::VisemeEventList> Pack_Tryll_VisemeEventList(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::VisemeEventList* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_VoiceProfile(FTryllJsonWriter& W, const ::Tryll::VoiceProfile* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_WireAudioFormat(FTryllJsonWriter& W, const ::Tryll::WireAudioFormat* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);
void Write_Tryll_WireTranscriptUpdate(FTryllJsonWriter& W, const ::Tryll::WireTranscriptUpdate* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix);

void Write_Tryll_Ack(FTryllJsonWriter& W, const ::Tryll::Ack* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->warning() != nullptr)
    {
        W.Key("warning");
        W.String(std::string_view(T->warning()->c_str(), T->warning()->size()));
    }
    W.EndObject();
}

void Write_Tryll_AgentVariable(FTryllJsonWriter& W, const ::Tryll::AgentVariable* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->value_type() != ::Tryll::VariableValue_NONE)
    {
        W.Key("value_type");
        switch (T->value_type())
        {
        case ::Tryll::VariableValue_VarInt: W.String("VarInt"); break;
        case ::Tryll::VariableValue_VarFloat: W.String("VarFloat"); break;
        case ::Tryll::VariableValue_VarString: W.String("VarString"); break;
        case ::Tryll::VariableValue_VarBool: W.String("VarBool"); break;
        case ::Tryll::VariableValue_VarStringSet: W.String("VarStringSet"); break;
        default: W.Int64(static_cast<std::int64_t>(T->value_type())); break;
        }
        W.Key("value");
        switch (T->value_type())
        {
        case ::Tryll::VariableValue_VarInt:
            Write_Tryll_VarInt(W, T->value_as_VarInt(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::VariableValue_VarFloat:
            Write_Tryll_VarFloat(W, T->value_as_VarFloat(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::VariableValue_VarString:
            Write_Tryll_VarString(W, T->value_as_VarString(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::VariableValue_VarBool:
            Write_Tryll_VarBool(W, T->value_as_VarBool(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::VariableValue_VarStringSet:
            Write_Tryll_VarStringSet(W, T->value_as_VarStringSet(), Omitted, Prefix + "value.");
            break;
        default: W.BeginObject(); W.EndObject(); break;
        }
    }
    W.Key("allow_output_substitution");
    W.Bool(T->allow_output_substitution());
    W.EndObject();
}

void Write_Tryll_AnswerText(FTryllJsonWriter& W, const ::Tryll::AnswerText* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.Key("is_delta");
    W.Bool(T->is_delta());
    W.Key("is_final");
    W.Bool(T->is_final());
    if (T->node_name() != nullptr)
    {
        W.Key("node_name");
        W.String(std::string_view(T->node_name()->c_str(), T->node_name()->size()));
    }
    W.EndObject();
}

std::string Label_Tryll_AnswerText(const ::Tryll::AnswerText* T)
{
    if (T == nullptr) return {};
    return TryllFormatLogLabel(
        "{node_name}",
        {
            { "node_name", (T->node_name() ? T->node_name()->str() : std::string()) },
        });
}

void Write_Tryll_AppendInteractionsRequest(FTryllJsonWriter& W, const ::Tryll::AppendInteractionsRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->interactions() != nullptr)
    {
        W.Key("interactions");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->interactions()->size(); ++i)
        {
            Write_Tryll_DialogInteraction(W, T->interactions()->Get(i), Omitted, Prefix + "interactions.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_AppendInteractionsResponse(FTryllJsonWriter& W, const ::Tryll::AppendInteractionsResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("appended_count");
    W.UInt64(T->appended_count());
    W.EndObject();
}

void Write_Tryll_AudioBuffer(FTryllJsonWriter& W, const ::Tryll::AudioBuffer* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("voice_input_id");
    W.UInt64(T->voice_input_id());
    const auto Path_pcm = (Prefix.empty() ? std::string("pcm") : Prefix + "pcm");
    const auto Found_pcm = Omitted.find(Path_pcm);
    if (T->pcm() != nullptr || Found_pcm != Omitted.end())
    {
        W.Key("pcm");
        std::int64_t Count_pcm = 0;
        std::int64_t Bytes_pcm = 0;
        if (Found_pcm != Omitted.end())
        {
            Count_pcm = Found_pcm->second.Count;
            Bytes_pcm = Found_pcm->second.Bytes;
        }
        else
        {
            Count_pcm = static_cast<std::int64_t>(T->pcm()->size());
            Bytes_pcm = static_cast<std::int64_t>(T->pcm()->size()) * 1;
        }
        W.BeginObject();
        W.Key("omitted"); W.Bool(true);
        W.Key("count"); W.Int64(Count_pcm);
        W.Key("bytes"); W.Int64(Bytes_pcm);
        W.EndObject();
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::AudioBuffer> Pack_Tryll_AudioBuffer(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::AudioBuffer* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    ::flatbuffers::Offset<::flatbuffers::Vector<uint8_t>> _pcm;
    if (T->pcm() != nullptr)
    {
        const auto Path = (Prefix.empty() ? std::string("pcm") : Prefix + "pcm");
        Omitted.push_back(FTryllOmittedField{ Path, static_cast<std::int64_t>(T->pcm()->size()), static_cast<std::int64_t>(T->pcm()->size()) * 1 });
        _pcm = Fbb.CreateVector(static_cast<const uint8_t*>(nullptr), 0);
    }
    return ::Tryll::CreateAudioBuffer(Fbb, T->voice_input_id(), _pcm);
}

void Write_Tryll_BeginUtteranceRequest(FTryllJsonWriter& W, const ::Tryll::BeginUtteranceRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("voice_input_id");
    W.UInt64(T->voice_input_id());
    W.Key("auto_finish_on_silence");
    W.Bool(T->auto_finish_on_silence());
    W.Key("max_utterance_ms");
    W.UInt64(T->max_utterance_ms());
    W.EndObject();
}

void Write_Tryll_CancelRequest(FTryllJsonWriter& W, const ::Tryll::CancelRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("mode");
    switch (T->mode())
    {
    case ::Tryll::CancelMode_StopAndKeep: W.String("StopAndKeep"); break;
    case ::Tryll::CancelMode_StopAndDiscard: W.String("StopAndDiscard"); break;
    default: W.Int64(static_cast<std::int64_t>(T->mode())); break;
    }
    W.EndObject();
}

void Write_Tryll_CancelUtteranceRequest(FTryllJsonWriter& W, const ::Tryll::CancelUtteranceRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("voice_input_id");
    W.UInt64(T->voice_input_id());
    W.EndObject();
}

void Write_Tryll_ChangeAgentParamRequest(FTryllJsonWriter& W, const ::Tryll::ChangeAgentParamRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->node_name() != nullptr)
    {
        W.Key("node_name");
        W.String(std::string_view(T->node_name()->c_str(), T->node_name()->size()));
    }
    if (T->params_type() != ::Tryll::NodeParams::NodeParams_NONE)
    {
        W.Key("params_type");
        switch (T->params_type())
        {
        case ::Tryll::NodeParams::NodeParams_GenerateParams: W.String("GenerateParams"); break;
        case ::Tryll::NodeParams::NodeParams_RegexGuardrailParams: W.String("RegexGuardrailParams"); break;
        case ::Tryll::NodeParams::NodeParams_CannedResponseParams: W.String("CannedResponseParams"); break;
        case ::Tryll::NodeParams::NodeParams_ToolCallParams: W.String("ToolCallParams"); break;
        case ::Tryll::NodeParams::NodeParams_RetrieveParams: W.String("RetrieveParams"); break;
        case ::Tryll::NodeParams::NodeParams_InstructionParams: W.String("InstructionParams"); break;
        case ::Tryll::NodeParams::NodeParams_ClassifyIntentParams: W.String("ClassifyIntentParams"); break;
        case ::Tryll::NodeParams::NodeParams_ClassifyIntentLLMParams: W.String("ClassifyIntentLLMParams"); break;
        case ::Tryll::NodeParams::NodeParams_IntentToInstructionParams: W.String("IntentToInstructionParams"); break;
        case ::Tryll::NodeParams::NodeParams_GenerateAndSpeakParams: W.String("GenerateAndSpeakParams"); break;
        case ::Tryll::NodeParams::NodeParams_SpeakParams: W.String("SpeakParams"); break;
        case ::Tryll::NodeParams::NodeParams_PauseParams: W.String("PauseParams"); break;
        case ::Tryll::NodeParams::NodeParams_TransformParams: W.String("TransformParams"); break;
        case ::Tryll::NodeParams::NodeParams_BranchParams: W.String("BranchParams"); break;
        case ::Tryll::NodeParams::NodeParams_WriteToMemoryParams: W.String("WriteToMemoryParams"); break;
        case ::Tryll::NodeParams::NodeParams_SummarizeParams: W.String("SummarizeParams"); break;
        case ::Tryll::NodeParams::NodeParams_ClassifyAttributesLLMParams: W.String("ClassifyAttributesLLMParams"); break;
        default: W.Int64(static_cast<std::int64_t>(T->params_type())); break;
        }
        W.Key("params");
        switch (T->params_type())
        {
        case ::Tryll::NodeParams::NodeParams_GenerateParams:
            Write_Tryll_NodeParams_GenerateParams(W, T->params_as_GenerateParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_RegexGuardrailParams:
            Write_Tryll_NodeParams_RegexGuardrailParams(W, T->params_as_RegexGuardrailParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_CannedResponseParams:
            Write_Tryll_NodeParams_CannedResponseParams(W, T->params_as_CannedResponseParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_ToolCallParams:
            Write_Tryll_NodeParams_ToolCallParams(W, T->params_as_ToolCallParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_RetrieveParams:
            Write_Tryll_NodeParams_RetrieveParams(W, T->params_as_RetrieveParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_InstructionParams:
            Write_Tryll_NodeParams_InstructionParams(W, T->params_as_InstructionParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_ClassifyIntentParams:
            Write_Tryll_NodeParams_ClassifyIntentParams(W, T->params_as_ClassifyIntentParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_ClassifyIntentLLMParams:
            Write_Tryll_NodeParams_ClassifyIntentLLMParams(W, T->params_as_ClassifyIntentLLMParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_IntentToInstructionParams:
            Write_Tryll_NodeParams_IntentToInstructionParams(W, T->params_as_IntentToInstructionParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_GenerateAndSpeakParams:
            Write_Tryll_NodeParams_GenerateAndSpeakParams(W, T->params_as_GenerateAndSpeakParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_SpeakParams:
            Write_Tryll_NodeParams_SpeakParams(W, T->params_as_SpeakParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_PauseParams:
            Write_Tryll_NodeParams_PauseParams(W, T->params_as_PauseParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_TransformParams:
            Write_Tryll_NodeParams_TransformParams(W, T->params_as_TransformParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_BranchParams:
            Write_Tryll_NodeParams_BranchParams(W, T->params_as_BranchParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_WriteToMemoryParams:
            Write_Tryll_NodeParams_WriteToMemoryParams(W, T->params_as_WriteToMemoryParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_SummarizeParams:
            Write_Tryll_NodeParams_SummarizeParams(W, T->params_as_SummarizeParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_ClassifyAttributesLLMParams:
            Write_Tryll_NodeParams_ClassifyAttributesLLMParams(W, T->params_as_ClassifyAttributesLLMParams(), Omitted, Prefix + "params.");
            break;
        default: W.BeginObject(); W.EndObject(); break;
        }
    }
    W.EndObject();
}

std::string Label_Tryll_ChangeAgentParamRequest(const ::Tryll::ChangeAgentParamRequest* T)
{
    if (T == nullptr) return {};
    return TryllFormatLogLabel(
        "{node_name} · {params_type}",
        {
            { "node_name", (T->node_name() ? T->node_name()->str() : std::string()) },
            { "params_type", (T->params_type() == ::Tryll::NodeParams::NodeParams_NONE ? std::string() : std::string(::Tryll::NodeParams::EnumNameNodeParams(T->params_type()))) },
        });
}

void Write_Tryll_ConnectionReady(FTryllJsonWriter& W, const ::Tryll::ConnectionReady* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("protocol_version");
    W.UInt64(T->protocol_version());
    if (T->codegen_fingerprint() != nullptr)
    {
        W.Key("codegen_fingerprint");
        W.String(std::string_view(T->codegen_fingerprint()->c_str(), T->codegen_fingerprint()->size()));
    }
    if (T->exe_path() != nullptr)
    {
        W.Key("exe_path");
        W.String(std::string_view(T->exe_path()->c_str(), T->exe_path()->size()));
    }
    if (T->build_config() != nullptr)
    {
        W.Key("build_config");
        W.String(std::string_view(T->build_config()->c_str(), T->build_config()->size()));
    }
    if (T->version() != nullptr)
    {
        W.Key("version");
        W.String(std::string_view(T->version()->c_str(), T->version()->size()));
    }
    if (T->run_id() != nullptr)
    {
        W.Key("run_id");
        W.String(std::string_view(T->run_id()->c_str(), T->run_id()->size()));
    }
    W.Key("managed");
    W.Bool(T->managed());
    W.Key("idle_timeout_seconds");
    W.UInt64(T->idle_timeout_seconds());
    if (T->mode() != nullptr)
    {
        W.Key("mode");
        W.String(std::string_view(T->mode()->c_str(), T->mode()->size()));
    }
    W.EndObject();
}

void Write_Tryll_CreateAgentRequest(FTryllJsonWriter& W, const ::Tryll::CreateAgentRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->graph() != nullptr)
    {
        W.Key("graph");
        Write_Tryll_GraphDescription(W, T->graph(), Omitted, Prefix + "graph.");
    }
    if (T->default_model_name() != nullptr)
    {
        W.Key("default_model_name");
        W.String(std::string_view(T->default_model_name()->c_str(), T->default_model_name()->size()));
    }
    W.Key("enable_diagnostics");
    W.Bool(T->enable_diagnostics());
    W.Key("maintain_dialogue_history");
    W.Bool(T->maintain_dialogue_history());
    if (T->variables() != nullptr)
    {
        W.Key("variables");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->variables()->size(); ++i)
        {
            Write_Tryll_AgentVariable(W, T->variables()->Get(i), Omitted, Prefix + "variables.");
        }
        W.EndArray();
    }
    W.Key("kv_cache_initialization");
    switch (T->kv_cache_initialization())
    {
    case ::Tryll::AgentKvCacheInitialization_AllocateOnly: W.String("AllocateOnly"); break;
    case ::Tryll::AgentKvCacheInitialization_Prefill: W.String("Prefill"); break;
    case ::Tryll::AgentKvCacheInitialization_DeferAllocation: W.String("DeferAllocation"); break;
    default: W.Int64(static_cast<std::int64_t>(T->kv_cache_initialization())); break;
    }
    W.Key("workload");
    switch (T->workload())
    {
    case ::Tryll::AgentWorkload_Interactive: W.String("Interactive"); break;
    case ::Tryll::AgentWorkload_Background: W.String("Background"); break;
    default: W.Int64(static_cast<std::int64_t>(T->workload())); break;
    }
    if (T->voice() != nullptr)
    {
        W.Key("voice");
        Write_Tryll_VoiceProfile(W, T->voice(), Omitted, Prefix + "voice.");
    }
    W.EndObject();
}

void Write_Tryll_CreateAgentResponse(FTryllJsonWriter& W, const ::Tryll::CreateAgentResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.EndObject();
}

void Write_Tryll_CreateEmbeddedStringStorageRequest(FTryllJsonWriter& W, const ::Tryll::CreateEmbeddedStringStorageRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->config_path() != nullptr)
    {
        W.Key("config_path");
        W.String(std::string_view(T->config_path()->c_str(), T->config_path()->size()));
    }
    const auto Path_strings = (Prefix.empty() ? std::string("strings") : Prefix + "strings");
    const auto Found_strings = Omitted.find(Path_strings);
    if (T->strings() != nullptr || Found_strings != Omitted.end())
    {
        W.Key("strings");
        std::int64_t Count_strings = 0;
        std::int64_t Bytes_strings = 0;
        if (Found_strings != Omitted.end())
        {
            Count_strings = Found_strings->second.Count;
            Bytes_strings = Found_strings->second.Bytes;
        }
        else
        {
            Count_strings = static_cast<std::int64_t>(T->strings()->size());
            Bytes_strings = [&]() { std::int64_t N = 0; for (flatbuffers::uoffset_t i = 0; i < T->strings()->size(); ++i) N += static_cast<std::int64_t>(T->strings()->Get(i)->size()); return N; }();
        }
        W.BeginObject();
        W.Key("omitted"); W.Bool(true);
        W.Key("count"); W.Int64(Count_strings);
        W.Key("bytes"); W.Int64(Bytes_strings);
        W.EndObject();
    }
    if (T->embedding_model() != nullptr)
    {
        W.Key("embedding_model");
        W.String(std::string_view(T->embedding_model()->c_str(), T->embedding_model()->size()));
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::CreateEmbeddedStringStorageRequest> Pack_Tryll_CreateEmbeddedStringStorageRequest(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::CreateEmbeddedStringStorageRequest* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    ::flatbuffers::Offset<::flatbuffers::String> _name = T->name() ? Fbb.CreateString(T->name()->str()) : 0;
    ::flatbuffers::Offset<::flatbuffers::String> _config_path = T->config_path() ? Fbb.CreateString(T->config_path()->str()) : 0;
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<::flatbuffers::String>>> _strings;
    if (T->strings() != nullptr)
    {
        const auto Path = (Prefix.empty() ? std::string("strings") : Prefix + "strings");
        Omitted.push_back(FTryllOmittedField{ Path, static_cast<std::int64_t>(T->strings()->size()), [&]() { std::int64_t N = 0; for (flatbuffers::uoffset_t i = 0; i < T->strings()->size(); ++i) N += static_cast<std::int64_t>(T->strings()->Get(i)->size()); return N; }() });
        _strings = Fbb.CreateVector(static_cast<const ::flatbuffers::Offset<::flatbuffers::String>*>(nullptr), 0);
    }
    ::flatbuffers::Offset<::flatbuffers::String> _embedding_model = T->embedding_model() ? Fbb.CreateString(T->embedding_model()->str()) : 0;
    return ::Tryll::CreateCreateEmbeddedStringStorageRequest(Fbb, T->request_id(), _name, _config_path, _strings, _embedding_model);
}

void Write_Tryll_CreateEmbeddedStringStorageResponse(FTryllJsonWriter& W, const ::Tryll::CreateEmbeddedStringStorageResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.Key("record_count");
    W.UInt64(T->record_count());
    W.Key("embedding_dim");
    W.UInt64(T->embedding_dim());
    W.EndObject();
}

void Write_Tryll_CreateSessionRequest(FTryllJsonWriter& W, const ::Tryll::CreateSessionRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->game_name() != nullptr)
    {
        W.Key("game_name");
        W.String(std::string_view(T->game_name()->c_str(), T->game_name()->size()));
    }
    if (T->locale() != nullptr)
    {
        W.Key("locale");
        W.String(std::string_view(T->locale()->c_str(), T->locale()->size()));
    }
    if (T->model_config() != nullptr)
    {
        W.Key("model_config");
        Write_Tryll_SessionModelConfig(W, T->model_config(), Omitted, Prefix + "model_config.");
    }
    W.EndObject();
}

void Write_Tryll_CreateSessionResponse(FTryllJsonWriter& W, const ::Tryll::CreateSessionResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("session_id");
    W.UInt64(T->session_id());
    W.EndObject();
}

void Write_Tryll_CreateStringStorageRequest(FTryllJsonWriter& W, const ::Tryll::CreateStringStorageRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.Key("kind");
    switch (T->kind())
    {
    case ::Tryll::StringStorageKind_List: W.String("List"); break;
    case ::Tryll::StringStorageKind_Map: W.String("Map"); break;
    case ::Tryll::StringStorageKind_Multimap: W.String("Multimap"); break;
    default: W.Int64(static_cast<std::int64_t>(T->kind())); break;
    }
    const auto Path_strings = (Prefix.empty() ? std::string("strings") : Prefix + "strings");
    const auto Found_strings = Omitted.find(Path_strings);
    if (T->strings() != nullptr || Found_strings != Omitted.end())
    {
        W.Key("strings");
        std::int64_t Count_strings = 0;
        std::int64_t Bytes_strings = 0;
        if (Found_strings != Omitted.end())
        {
            Count_strings = Found_strings->second.Count;
            Bytes_strings = Found_strings->second.Bytes;
        }
        else
        {
            Count_strings = static_cast<std::int64_t>(T->strings()->size());
            Bytes_strings = [&]() { std::int64_t N = 0; for (flatbuffers::uoffset_t i = 0; i < T->strings()->size(); ++i) N += static_cast<std::int64_t>(T->strings()->Get(i)->size()); return N; }();
        }
        W.BeginObject();
        W.Key("omitted"); W.Bool(true);
        W.Key("count"); W.Int64(Count_strings);
        W.Key("bytes"); W.Int64(Bytes_strings);
        W.EndObject();
    }
    const auto Path_keys = (Prefix.empty() ? std::string("keys") : Prefix + "keys");
    const auto Found_keys = Omitted.find(Path_keys);
    if (T->keys() != nullptr || Found_keys != Omitted.end())
    {
        W.Key("keys");
        std::int64_t Count_keys = 0;
        std::int64_t Bytes_keys = 0;
        if (Found_keys != Omitted.end())
        {
            Count_keys = Found_keys->second.Count;
            Bytes_keys = Found_keys->second.Bytes;
        }
        else
        {
            Count_keys = static_cast<std::int64_t>(T->keys()->size());
            Bytes_keys = [&]() { std::int64_t N = 0; for (flatbuffers::uoffset_t i = 0; i < T->keys()->size(); ++i) N += static_cast<std::int64_t>(T->keys()->Get(i)->size()); return N; }();
        }
        W.BeginObject();
        W.Key("omitted"); W.Bool(true);
        W.Key("count"); W.Int64(Count_keys);
        W.Key("bytes"); W.Int64(Bytes_keys);
        W.EndObject();
    }
    if (T->file_path() != nullptr)
    {
        W.Key("file_path");
        W.String(std::string_view(T->file_path()->c_str(), T->file_path()->size()));
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::CreateStringStorageRequest> Pack_Tryll_CreateStringStorageRequest(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::CreateStringStorageRequest* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    ::flatbuffers::Offset<::flatbuffers::String> _name = T->name() ? Fbb.CreateString(T->name()->str()) : 0;
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<::flatbuffers::String>>> _strings;
    if (T->strings() != nullptr)
    {
        const auto Path = (Prefix.empty() ? std::string("strings") : Prefix + "strings");
        Omitted.push_back(FTryllOmittedField{ Path, static_cast<std::int64_t>(T->strings()->size()), [&]() { std::int64_t N = 0; for (flatbuffers::uoffset_t i = 0; i < T->strings()->size(); ++i) N += static_cast<std::int64_t>(T->strings()->Get(i)->size()); return N; }() });
        _strings = Fbb.CreateVector(static_cast<const ::flatbuffers::Offset<::flatbuffers::String>*>(nullptr), 0);
    }
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<::flatbuffers::String>>> _keys;
    if (T->keys() != nullptr)
    {
        const auto Path = (Prefix.empty() ? std::string("keys") : Prefix + "keys");
        Omitted.push_back(FTryllOmittedField{ Path, static_cast<std::int64_t>(T->keys()->size()), [&]() { std::int64_t N = 0; for (flatbuffers::uoffset_t i = 0; i < T->keys()->size(); ++i) N += static_cast<std::int64_t>(T->keys()->Get(i)->size()); return N; }() });
        _keys = Fbb.CreateVector(static_cast<const ::flatbuffers::Offset<::flatbuffers::String>*>(nullptr), 0);
    }
    ::flatbuffers::Offset<::flatbuffers::String> _file_path = T->file_path() ? Fbb.CreateString(T->file_path()->str()) : 0;
    return ::Tryll::CreateCreateStringStorageRequest(Fbb, T->request_id(), _name, T->kind(), _strings, _keys, _file_path);
}

void Write_Tryll_CreateStringStorageResponse(FTryllJsonWriter& W, const ::Tryll::CreateStringStorageResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.EndObject();
}

void Write_Tryll_CreateVoiceInputRequest(FTryllJsonWriter& W, const ::Tryll::CreateVoiceInputRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    if (T->input_format() != nullptr)
    {
        W.Key("input_format");
        Write_Tryll_WireAudioFormat(W, T->input_format(), Omitted, Prefix + "input_format.");
    }
    W.Key("vad_threshold");
    W.Float(T->vad_threshold());
    W.Key("vad_min_silence_ms");
    W.UInt64(T->vad_min_silence_ms());
    W.Key("vad_speech_pad_ms");
    W.UInt64(T->vad_speech_pad_ms());
    if (T->hotwords_storage_path() != nullptr)
    {
        W.Key("hotwords_storage_path");
        W.String(std::string_view(T->hotwords_storage_path()->c_str(), T->hotwords_storage_path()->size()));
    }
    W.Key("hotwords_score");
    W.Float(T->hotwords_score());
    W.EndObject();
}

void Write_Tryll_CreateVoiceInputResponse(FTryllJsonWriter& W, const ::Tryll::CreateVoiceInputResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("voice_input_id");
    W.UInt64(T->voice_input_id());
    W.EndObject();
}

void Write_Tryll_DebugCommandRequest(FTryllJsonWriter& W, const ::Tryll::DebugCommandRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->command() != nullptr)
    {
        W.Key("command");
        W.String(std::string_view(T->command()->c_str(), T->command()->size()));
    }
    if (T->payload() != nullptr)
    {
        W.Key("payload");
        W.String(std::string_view(T->payload()->c_str(), T->payload()->size()));
    }
    W.EndObject();
}

void Write_Tryll_DebugCommandResponse(FTryllJsonWriter& W, const ::Tryll::DebugCommandResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->result() != nullptr)
    {
        W.Key("result");
        W.String(std::string_view(T->result()->c_str(), T->result()->size()));
    }
    W.EndObject();
}

void Write_Tryll_DeleteModelRequest(FTryllJsonWriter& W, const ::Tryll::DeleteModelRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.EndObject();
}

void Write_Tryll_DeliveryMode(FTryllJsonWriter& W, const ::Tryll::DeliveryMode* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->reference_voice() != nullptr)
    {
        W.Key("reference_voice");
        W.String(std::string_view(T->reference_voice()->c_str(), T->reference_voice()->size()));
    }
    if (T->instruction() != nullptr)
    {
        W.Key("instruction");
        W.String(std::string_view(T->instruction()->c_str(), T->instruction()->size()));
    }
    W.Key("expressiveness");
    W.Float(T->expressiveness());
    W.Key("speed_scale");
    W.Float(T->speed_scale());
    W.EndObject();
}

void Write_Tryll_DestroyAgentRequest(FTryllJsonWriter& W, const ::Tryll::DestroyAgentRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.EndObject();
}

void Write_Tryll_DestroyEmbeddedStringStorageRequest(FTryllJsonWriter& W, const ::Tryll::DestroyEmbeddedStringStorageRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.EndObject();
}

void Write_Tryll_DestroyStringStorageRequest(FTryllJsonWriter& W, const ::Tryll::DestroyStringStorageRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.EndObject();
}

void Write_Tryll_DestroyVoiceInputRequest(FTryllJsonWriter& W, const ::Tryll::DestroyVoiceInputRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("voice_input_id");
    W.UInt64(T->voice_input_id());
    W.EndObject();
}

void Write_Tryll_DialogInteraction(FTryllJsonWriter& W, const ::Tryll::DialogInteraction* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->user_message() != nullptr)
    {
        W.Key("user_message");
        W.String(std::string_view(T->user_message()->c_str(), T->user_message()->size()));
    }
    if (T->assistant_message() != nullptr)
    {
        W.Key("assistant_message");
        W.String(std::string_view(T->assistant_message()->c_str(), T->assistant_message()->size()));
    }
    W.EndObject();
}

void Write_Tryll_DownloadComplete(FTryllJsonWriter& W, const ::Tryll::DownloadComplete* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.Key("success");
    W.Bool(T->success());
    if (T->error() != nullptr)
    {
        W.Key("error");
        W.String(std::string_view(T->error()->c_str(), T->error()->size()));
    }
    W.EndObject();
}

void Write_Tryll_DownloadModelRequest(FTryllJsonWriter& W, const ::Tryll::DownloadModelRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.EndObject();
}

void Write_Tryll_DownloadProgress(FTryllJsonWriter& W, const ::Tryll::DownloadProgress* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.Key("bytes_downloaded");
    W.UInt64(T->bytes_downloaded());
    W.Key("total_bytes");
    W.UInt64(T->total_bytes());
    W.Key("percent");
    W.Float(T->percent());
    W.EndObject();
}

std::string Label_Tryll_DownloadProgress(const ::Tryll::DownloadProgress* T)
{
    if (T == nullptr) return {};
    return TryllFormatLogLabel(
        "{model_name}",
        {
            { "model_name", (T->model_name() ? T->model_name()->str() : std::string()) },
        });
}

void Write_Tryll_EndUtteranceRequest(FTryllJsonWriter& W, const ::Tryll::EndUtteranceRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("voice_input_id");
    W.UInt64(T->voice_input_id());
    W.EndObject();
}

void Write_Tryll_EngineSelection(FTryllJsonWriter& W, const ::Tryll::EngineSelection* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("role");
    switch (T->role())
    {
    case ::Tryll::ModelRole_Language: W.String("Language"); break;
    case ::Tryll::ModelRole_Embedding: W.String("Embedding"); break;
    case ::Tryll::ModelRole_Stt: W.String("Stt"); break;
    case ::Tryll::ModelRole_Tts: W.String("Tts"); break;
    case ::Tryll::ModelRole_Vad: W.String("Vad"); break;
    default: W.Int64(static_cast<std::int64_t>(T->role())); break;
    }
    W.Key("engine");
    switch (T->engine())
    {
    case ::Tryll::InferenceEngine_Mock: W.String("Mock"); break;
    case ::Tryll::InferenceEngine_LlamaCpp: W.String("LlamaCpp"); break;
    case ::Tryll::InferenceEngine_OnnxGenAI: W.String("OnnxGenAI"); break;
    case ::Tryll::InferenceEngine_WindowsML: W.String("WindowsML"); break;
    case ::Tryll::InferenceEngine_OpenVino: W.String("OpenVino"); break;
    case ::Tryll::InferenceEngine_TensorRtLlm: W.String("TensorRtLlm"); break;
    case ::Tryll::InferenceEngine_SherpaOnnx: W.String("SherpaOnnx"); break;
    case ::Tryll::InferenceEngine_OnnxRuntime: W.String("OnnxRuntime"); break;
    default: W.Int64(static_cast<std::int64_t>(T->engine())); break;
    }
    W.EndObject();
}

void Write_Tryll_EnsureMutableKbRequest(FTryllJsonWriter& W, const ::Tryll::EnsureMutableKbRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->schema() != nullptr)
    {
        W.Key("schema");
        W.String(std::string_view(T->schema()->c_str(), T->schema()->size()));
    }
    W.EndObject();
}

void Write_Tryll_EnsureMutableKbResponse(FTryllJsonWriter& W, const ::Tryll::EnsureMutableKbResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->schema() != nullptr)
    {
        W.Key("schema");
        W.String(std::string_view(T->schema()->c_str(), T->schema()->size()));
    }
    W.Key("created");
    W.Bool(T->created());
    W.EndObject();
}

void Write_Tryll_EnumerateMutableKbsRequest(FTryllJsonWriter& W, const ::Tryll::EnumerateMutableKbsRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.EndObject();
}

void Write_Tryll_EnumerateMutableKbsResponse(FTryllJsonWriter& W, const ::Tryll::EnumerateMutableKbsResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->stores() != nullptr)
    {
        W.Key("stores");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->stores()->size(); ++i)
        {
            Write_Tryll_MutableKbInfo(W, T->stores()->Get(i), Omitted, Prefix + "stores.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_ErrorResponse(FTryllJsonWriter& W, const ::Tryll::ErrorResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("code");
    W.UInt64(T->code());
    if (T->message() != nullptr)
    {
        W.Key("message");
        W.String(std::string_view(T->message()->c_str(), T->message()->size()));
    }
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.EndObject();
}

void Write_Tryll_EvictAgentKvCacheRequest(FTryllJsonWriter& W, const ::Tryll::EvictAgentKvCacheRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.EndObject();
}

void Write_Tryll_EvictAgentKvCacheResponse(FTryllJsonWriter& W, const ::Tryll::EvictAgentKvCacheResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("applicable");
    W.Bool(T->applicable());
    W.Key("residency");
    switch (T->residency())
    {
    case ::Tryll::AgentKvCacheResidency_Evicted: W.String("Evicted"); break;
    case ::Tryll::AgentKvCacheResidency_Resident: W.String("Resident"); break;
    default: W.Int64(static_cast<std::int64_t>(T->residency())); break;
    }
    W.Key("prefix_status");
    switch (T->prefix_status())
    {
    case ::Tryll::AgentKvCachePrefixStatus_NotCurrent: W.String("NotCurrent"); break;
    case ::Tryll::AgentKvCachePrefixStatus_Current: W.String("Current"); break;
    default: W.Int64(static_cast<std::int64_t>(T->prefix_status())); break;
    }
    W.Key("eligible_node_count");
    W.UInt64(T->eligible_node_count());
    W.Key("contexts_evicted");
    W.UInt64(T->contexts_evicted());
    if (T->debug_info() != nullptr)
    {
        W.Key("debug_info");
        W.String(std::string_view(T->debug_info()->c_str(), T->debug_info()->size()));
    }
    W.EndObject();
}

void Write_Tryll_ExportDialogRequest(FTryllJsonWriter& W, const ::Tryll::ExportDialogRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.EndObject();
}

void Write_Tryll_ExportDialogResponse(FTryllJsonWriter& W, const ::Tryll::ExportDialogResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->snapshot() != nullptr)
    {
        W.Key("snapshot");
        Write_Tryll_PersistedDialogSnapshot(W, T->snapshot(), Omitted, Prefix + "snapshot.");
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::ExportDialogResponse> Pack_Tryll_ExportDialogResponse(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::ExportDialogResponse* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    auto _snapshot = T->snapshot() ? Pack_Tryll_PersistedDialogSnapshot(Fbb, T->snapshot(), Omitted, Prefix + "snapshot.") : 0;
    return ::Tryll::CreateExportDialogResponse(Fbb, T->request_id(), T->agent_id(), _snapshot);
}

void Write_Tryll_ExportMutableKbRequest(FTryllJsonWriter& W, const ::Tryll::ExportMutableKbRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.EndObject();
}

void Write_Tryll_ExportMutableKbResponse(FTryllJsonWriter& W, const ::Tryll::ExportMutableKbResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->snapshot() != nullptr)
    {
        W.Key("snapshot");
        Write_Tryll_PersistedMutableKb(W, T->snapshot(), Omitted, Prefix + "snapshot.");
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::ExportMutableKbResponse> Pack_Tryll_ExportMutableKbResponse(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::ExportMutableKbResponse* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    auto _snapshot = T->snapshot() ? Pack_Tryll_PersistedMutableKb(Fbb, T->snapshot(), Omitted, Prefix + "snapshot.") : 0;
    return ::Tryll::CreateExportMutableKbResponse(Fbb, T->request_id(), _snapshot);
}

void Write_Tryll_GameMemory(FTryllJsonWriter& W, const ::Tryll::GameMemory* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("format_version");
    W.UInt64(T->format_version());
    W.Key("saved_at_ms");
    W.UInt64(T->saved_at_ms());
    if (T->save_id() != nullptr)
    {
        W.Key("save_id");
        W.String(std::string_view(T->save_id()->c_str(), T->save_id()->size()));
    }
    const auto Path_stores = (Prefix.empty() ? std::string("stores") : Prefix + "stores");
    const auto Found_stores = Omitted.find(Path_stores);
    if (T->stores() != nullptr || Found_stores != Omitted.end())
    {
        W.Key("stores");
        std::int64_t Count_stores = 0;
        std::int64_t Bytes_stores = 0;
        if (Found_stores != Omitted.end())
        {
            Count_stores = Found_stores->second.Count;
            Bytes_stores = Found_stores->second.Bytes;
        }
        else
        {
            Count_stores = static_cast<std::int64_t>(T->stores()->size());
            Bytes_stores = static_cast<std::int64_t>(T->stores()->size()) * 4;
        }
        W.BeginObject();
        W.Key("omitted"); W.Bool(true);
        W.Key("count"); W.Int64(Count_stores);
        W.Key("bytes"); W.Int64(Bytes_stores);
        W.EndObject();
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::GameMemory> Pack_Tryll_GameMemory(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::GameMemory* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    ::flatbuffers::Offset<::flatbuffers::String> _save_id = T->save_id() ? Fbb.CreateString(T->save_id()->str()) : 0;
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<::Tryll::PersistedMutableKb>>> _stores;
    if (T->stores() != nullptr)
    {
        const auto Path = (Prefix.empty() ? std::string("stores") : Prefix + "stores");
        Omitted.push_back(FTryllOmittedField{ Path, static_cast<std::int64_t>(T->stores()->size()), static_cast<std::int64_t>(T->stores()->size()) * 4 });
        _stores = Fbb.CreateVector(static_cast<const ::flatbuffers::Offset<::Tryll::PersistedMutableKb>*>(nullptr), 0);
    }
    return ::Tryll::CreateGameMemory(Fbb, T->format_version(), T->saved_at_ms(), _save_id, _stores);
}

void Write_Tryll_GetAgentKvCacheStatusRequest(FTryllJsonWriter& W, const ::Tryll::GetAgentKvCacheStatusRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.EndObject();
}

void Write_Tryll_GetAgentKvCacheStatusResponse(FTryllJsonWriter& W, const ::Tryll::GetAgentKvCacheStatusResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("applicable");
    W.Bool(T->applicable());
    W.Key("residency");
    switch (T->residency())
    {
    case ::Tryll::AgentKvCacheResidency_Evicted: W.String("Evicted"); break;
    case ::Tryll::AgentKvCacheResidency_Resident: W.String("Resident"); break;
    default: W.Int64(static_cast<std::int64_t>(T->residency())); break;
    }
    W.Key("prefix_status");
    switch (T->prefix_status())
    {
    case ::Tryll::AgentKvCachePrefixStatus_NotCurrent: W.String("NotCurrent"); break;
    case ::Tryll::AgentKvCachePrefixStatus_Current: W.String("Current"); break;
    default: W.Int64(static_cast<std::int64_t>(T->prefix_status())); break;
    }
    W.Key("eligible_node_count");
    W.UInt64(T->eligible_node_count());
    if (T->debug_info() != nullptr)
    {
        W.Key("debug_info");
        W.String(std::string_view(T->debug_info()->c_str(), T->debug_info()->size()));
    }
    W.EndObject();
}

void Write_Tryll_GetAgentVariablesRequest(FTryllJsonWriter& W, const ::Tryll::GetAgentVariablesRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->names() != nullptr)
    {
        W.Key("names");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->names()->size(); ++i)
        {
            W.String(std::string_view(T->names()->Get(i)->c_str(), T->names()->Get(i)->size()));
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_GetAgentVariablesResponse(FTryllJsonWriter& W, const ::Tryll::GetAgentVariablesResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->variables() != nullptr)
    {
        W.Key("variables");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->variables()->size(); ++i)
        {
            Write_Tryll_AgentVariable(W, T->variables()->Get(i), Omitted, Prefix + "variables.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_GetMemoryConsumptionRequest(FTryllJsonWriter& W, const ::Tryll::GetMemoryConsumptionRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.EndObject();
}

void Write_Tryll_GetMemoryConsumptionResponse(FTryllJsonWriter& W, const ::Tryll::GetMemoryConsumptionResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->snapshot() != nullptr)
    {
        W.Key("snapshot");
        Write_Tryll_MemorySnapshot(W, T->snapshot(), Omitted, Prefix + "snapshot.");
    }
    W.EndObject();
}

void Write_Tryll_GraphDescription(FTryllJsonWriter& W, const ::Tryll::GraphDescription* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->nodes() != nullptr)
    {
        W.Key("nodes");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->nodes()->size(); ++i)
        {
            Write_Tryll_NodeDescription(W, T->nodes()->Get(i), Omitted, Prefix + "nodes.");
        }
        W.EndArray();
    }
    if (T->start_node() != nullptr)
    {
        W.Key("start_node");
        W.String(std::string_view(T->start_node()->c_str(), T->start_node()->size()));
    }
    W.EndObject();
}

void Write_Tryll_ImportDialogRequest(FTryllJsonWriter& W, const ::Tryll::ImportDialogRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->snapshot() != nullptr)
    {
        W.Key("snapshot");
        Write_Tryll_PersistedDialogSnapshot(W, T->snapshot(), Omitted, Prefix + "snapshot.");
    }
    W.Key("compatibility");
    switch (T->compatibility())
    {
    case ::Tryll::DialogCompatibility_Strict: W.String("Strict"); break;
    case ::Tryll::DialogCompatibility_AllowGraphMismatch: W.String("AllowGraphMismatch"); break;
    default: W.Int64(static_cast<std::int64_t>(T->compatibility())); break;
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::ImportDialogRequest> Pack_Tryll_ImportDialogRequest(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::ImportDialogRequest* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    auto _snapshot = T->snapshot() ? Pack_Tryll_PersistedDialogSnapshot(Fbb, T->snapshot(), Omitted, Prefix + "snapshot.") : 0;
    return ::Tryll::CreateImportDialogRequest(Fbb, T->request_id(), T->agent_id(), _snapshot, T->compatibility());
}

void Write_Tryll_ImportDialogResponse(FTryllJsonWriter& W, const ::Tryll::ImportDialogResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("interaction_count");
    W.UInt64(T->interaction_count());
    if (T->warnings() != nullptr)
    {
        W.Key("warnings");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->warnings()->size(); ++i)
        {
            W.String(std::string_view(T->warnings()->Get(i)->c_str(), T->warnings()->Get(i)->size()));
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_ImportMutableKbRequest(FTryllJsonWriter& W, const ::Tryll::ImportMutableKbRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->snapshot() != nullptr)
    {
        W.Key("snapshot");
        Write_Tryll_PersistedMutableKb(W, T->snapshot(), Omitted, Prefix + "snapshot.");
    }
    W.Key("compatibility");
    switch (T->compatibility())
    {
    case ::Tryll::MutableKbCompatibility_Strict: W.String("Strict"); break;
    case ::Tryll::MutableKbCompatibility_AllowSchemaDrift: W.String("AllowSchemaDrift"); break;
    default: W.Int64(static_cast<std::int64_t>(T->compatibility())); break;
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::ImportMutableKbRequest> Pack_Tryll_ImportMutableKbRequest(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::ImportMutableKbRequest* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    auto _snapshot = T->snapshot() ? Pack_Tryll_PersistedMutableKb(Fbb, T->snapshot(), Omitted, Prefix + "snapshot.") : 0;
    return ::Tryll::CreateImportMutableKbRequest(Fbb, T->request_id(), _snapshot, T->compatibility());
}

void Write_Tryll_ImportMutableKbResponse(FTryllJsonWriter& W, const ::Tryll::ImportMutableKbResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->warnings() != nullptr)
    {
        W.Key("warnings");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->warnings()->size(); ++i)
        {
            W.String(std::string_view(T->warnings()->Get(i)->c_str(), T->warnings()->Get(i)->size()));
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_ListModelsRequest(FTryllJsonWriter& W, const ::Tryll::ListModelsRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.EndObject();
}

void Write_Tryll_ListModelsResponse(FTryllJsonWriter& W, const ::Tryll::ListModelsResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->models() != nullptr)
    {
        W.Key("models");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->models()->size(); ++i)
        {
            Write_Tryll_ModelInfo(W, T->models()->Get(i), Omitted, Prefix + "models.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_LoadDialogRequest(FTryllJsonWriter& W, const ::Tryll::LoadDialogRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->path() != nullptr)
    {
        W.Key("path");
        W.String(std::string_view(T->path()->c_str(), T->path()->size()));
    }
    W.Key("compatibility");
    switch (T->compatibility())
    {
    case ::Tryll::DialogCompatibility_Strict: W.String("Strict"); break;
    case ::Tryll::DialogCompatibility_AllowGraphMismatch: W.String("AllowGraphMismatch"); break;
    default: W.Int64(static_cast<std::int64_t>(T->compatibility())); break;
    }
    W.EndObject();
}

void Write_Tryll_LoadDialogResponse(FTryllJsonWriter& W, const ::Tryll::LoadDialogResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("interaction_count");
    W.UInt64(T->interaction_count());
    if (T->warnings() != nullptr)
    {
        W.Key("warnings");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->warnings()->size(); ++i)
        {
            W.String(std::string_view(T->warnings()->Get(i)->c_str(), T->warnings()->Get(i)->size()));
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_LoadGameMemoryRequest(FTryllJsonWriter& W, const ::Tryll::LoadGameMemoryRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->path() != nullptr)
    {
        W.Key("path");
        W.String(std::string_view(T->path()->c_str(), T->path()->size()));
    }
    W.Key("compatibility");
    switch (T->compatibility())
    {
    case ::Tryll::MutableKbCompatibility_Strict: W.String("Strict"); break;
    case ::Tryll::MutableKbCompatibility_AllowSchemaDrift: W.String("AllowSchemaDrift"); break;
    default: W.Int64(static_cast<std::int64_t>(T->compatibility())); break;
    }
    W.Key("embed_timing");
    switch (T->embed_timing())
    {
    case ::Tryll::MutableKbEmbedTiming_Unspecified: W.String("Unspecified"); break;
    case ::Tryll::MutableKbEmbedTiming_Now: W.String("Now"); break;
    case ::Tryll::MutableKbEmbedTiming_Lazy: W.String("Lazy"); break;
    default: W.Int64(static_cast<std::int64_t>(T->embed_timing())); break;
    }
    W.EndObject();
}

void Write_Tryll_LoadGameMemoryResponse(FTryllJsonWriter& W, const ::Tryll::LoadGameMemoryResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("store_count");
    W.UInt64(T->store_count());
    if (T->warnings() != nullptr)
    {
        W.Key("warnings");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->warnings()->size(); ++i)
        {
            W.String(std::string_view(T->warnings()->Get(i)->c_str(), T->warnings()->Get(i)->size()));
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_LoadModelRequest(FTryllJsonWriter& W, const ::Tryll::LoadModelRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.Key("placement");
    switch (T->placement())
    {
    case ::Tryll::MemoryPlacement_Auto: W.String("Auto"); break;
    case ::Tryll::MemoryPlacement_Ram: W.String("Ram"); break;
    case ::Tryll::MemoryPlacement_Vram: W.String("Vram"); break;
    default: W.Int64(static_cast<std::int64_t>(T->placement())); break;
    }
    W.Key("placement_is_override");
    W.Bool(T->placement_is_override());
    W.EndObject();
}

void Write_Tryll_LoadModelResponse(FTryllJsonWriter& W, const ::Tryll::LoadModelResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("placement_resolved");
    switch (T->placement_resolved())
    {
    case ::Tryll::MemoryPlacement_Auto: W.String("Auto"); break;
    case ::Tryll::MemoryPlacement_Ram: W.String("Ram"); break;
    case ::Tryll::MemoryPlacement_Vram: W.String("Vram"); break;
    default: W.Int64(static_cast<std::int64_t>(T->placement_resolved())); break;
    }
    if (T->backend() != nullptr)
    {
        W.Key("backend");
        W.String(std::string_view(T->backend()->c_str(), T->backend()->size()));
    }
    W.EndObject();
}

void Write_Tryll_LogCaptureGap(FTryllJsonWriter& W, const ::Tryll::LogCaptureGap* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("reason");
    switch (T->reason())
    {
    case ::Tryll::LogGapReason_paused: W.String("paused"); break;
    case ::Tryll::LogGapReason_epoch_change: W.String("epoch_change"); break;
    case ::Tryll::LogGapReason_reload_discard: W.String("reload_discard"); break;
    default: W.Int64(static_cast<std::int64_t>(T->reason())); break;
    }
    W.EndObject();
}

void Write_Tryll_LogConnectionState(FTryllJsonWriter& W, const ::Tryll::LogConnectionState* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("state");
    switch (T->state())
    {
    case ::Tryll::LogConnState_connected: W.String("connected"); break;
    case ::Tryll::LogConnState_failed: W.String("failed"); break;
    case ::Tryll::LogConnState_dropped: W.String("dropped"); break;
    default: W.Int64(static_cast<std::int64_t>(T->state())); break;
    }
    if (T->reason() != nullptr)
    {
        W.Key("reason");
        W.String(std::string_view(T->reason()->c_str(), T->reason()->size()));
    }
    W.EndObject();
}

void Write_Tryll_LogLocalError(FTryllJsonWriter& W, const ::Tryll::LogLocalError* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("code");
    W.UInt64(T->code());
    if (T->code_name() != nullptr)
    {
        W.Key("code_name");
        W.String(std::string_view(T->code_name()->c_str(), T->code_name()->size()));
    }
    if (T->message() != nullptr)
    {
        W.Key("message");
        W.String(std::string_view(T->message()->c_str(), T->message()->size()));
    }
    W.Key("origin");
    switch (T->origin())
    {
    case ::Tryll::LogErrorOrigin_transport: W.String("transport"); break;
    case ::Tryll::LogErrorOrigin_unexpected_reply: W.String("unexpected_reply"); break;
    case ::Tryll::LogErrorOrigin_client_validation: W.String("client_validation"); break;
    default: W.Int64(static_cast<std::int64_t>(T->origin())); break;
    }
    W.EndObject();
}

void Write_Tryll_MemoryComponent(FTryllJsonWriter& W, const ::Tryll::MemoryComponent* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->kind() != nullptr)
    {
        W.Key("kind");
        W.String(std::string_view(T->kind()->c_str(), T->kind()->size()));
    }
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.Key("owner");
    switch (T->owner())
    {
    case ::Tryll::MemoryOwnerKind_Process: W.String("Process"); break;
    case ::Tryll::MemoryOwnerKind_Device: W.String("Device"); break;
    case ::Tryll::MemoryOwnerKind_Model: W.String("Model"); break;
    case ::Tryll::MemoryOwnerKind_AgentNode: W.String("AgentNode"); break;
    case ::Tryll::MemoryOwnerKind_Storage: W.String("Storage"); break;
    case ::Tryll::MemoryOwnerKind_Unattributed: W.String("Unattributed"); break;
    default: W.Int64(static_cast<std::int64_t>(T->owner())); break;
    }
    W.Key("tier");
    switch (T->tier())
    {
    case ::Tryll::MemoryTier_Measured: W.String("Measured"); break;
    case ::Tryll::MemoryTier_Derived: W.String("Derived"); break;
    case ::Tryll::MemoryTier_Estimated: W.String("Estimated"); break;
    default: W.Int64(static_cast<std::int64_t>(T->tier())); break;
    }
    if (T->estimate_basis() != nullptr)
    {
        W.Key("estimate_basis");
        W.String(std::string_view(T->estimate_basis()->c_str(), T->estimate_basis()->size()));
    }
    W.Key("ram_bytes");
    W.UInt64(T->ram_bytes());
    W.Key("vram_bytes");
    W.UInt64(T->vram_bytes());
    W.Key("ram_bytes_high");
    W.UInt64(T->ram_bytes_high());
    W.Key("device_index");
    W.Int64(T->device_index());
    if (T->per_device() != nullptr)
    {
        W.Key("per_device");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->per_device()->size(); ++i)
        {
            Write_Tryll_MemoryComponentDevice(W, T->per_device()->Get(i), Omitted, Prefix + "per_device.");
        }
        W.EndArray();
    }
    W.Key("session_id");
    W.UInt64(T->session_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->node_name() != nullptr)
    {
        W.Key("node_name");
        W.String(std::string_view(T->node_name()->c_str(), T->node_name()->size()));
    }
    W.Key("used_tokens");
    W.UInt64(T->used_tokens());
    W.Key("capacity_tokens");
    W.UInt64(T->capacity_tokens());
    W.Key("resident");
    W.Bool(T->resident());
    W.Key("occupancy_stale");
    W.Bool(T->occupancy_stale());
    W.Key("compute_bytes");
    W.UInt64(T->compute_bytes());
    W.Key("consumer_class");
    switch (T->consumer_class())
    {
    case ::Tryll::InferenceConsumerClass_Unspecified: W.String("Unspecified"); break;
    case ::Tryll::InferenceConsumerClass_InteractiveVisible: W.String("InteractiveVisible"); break;
    case ::Tryll::InferenceConsumerClass_InteractiveBuffered: W.String("InteractiveBuffered"); break;
    case ::Tryll::InferenceConsumerClass_Background: W.String("Background"); break;
    default: W.Int64(static_cast<std::int64_t>(T->consumer_class())); break;
    }
    W.Key("last_turn_at_ms");
    W.Int64(T->last_turn_at_ms());
    W.EndObject();
}

void Write_Tryll_MemoryComponentDevice(FTryllJsonWriter& W, const ::Tryll::MemoryComponentDevice* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("device_index");
    W.Int64(T->device_index());
    W.Key("bytes");
    W.UInt64(T->bytes());
    W.Key("compute_bytes");
    W.UInt64(T->compute_bytes());
    W.EndObject();
}

void Write_Tryll_MemoryDevice(FTryllJsonWriter& W, const ::Tryll::MemoryDevice* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("index");
    W.Int64(T->index());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->description() != nullptr)
    {
        W.Key("description");
        W.String(std::string_view(T->description()->c_str(), T->description()->size()));
    }
    if (T->kind() != nullptr)
    {
        W.Key("kind");
        W.String(std::string_view(T->kind()->c_str(), T->kind()->size()));
    }
    W.Key("total_bytes");
    W.UInt64(T->total_bytes());
    W.Key("free_bytes");
    W.UInt64(T->free_bytes());
    W.Key("self_model_bytes");
    W.UInt64(T->self_model_bytes());
    W.Key("self_context_bytes");
    W.UInt64(T->self_context_bytes());
    W.Key("self_compute_bytes");
    W.UInt64(T->self_compute_bytes());
    W.Key("other_bytes");
    W.UInt64(T->other_bytes());
    W.EndObject();
}

void Write_Tryll_MemorySnapshot(FTryllJsonWriter& W, const ::Tryll::MemorySnapshot* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("captured_at_ms");
    W.Int64(T->captured_at_ms());
    W.Key("process_working_set_bytes");
    W.UInt64(T->process_working_set_bytes());
    W.Key("process_private_bytes");
    W.UInt64(T->process_private_bytes());
    W.Key("process_available");
    W.Bool(T->process_available());
    W.Key("backend_initialized");
    W.Bool(T->backend_initialized());
    W.Key("unattributed_ram_bytes");
    W.UInt64(T->unattributed_ram_bytes());
    if (T->devices() != nullptr)
    {
        W.Key("devices");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->devices()->size(); ++i)
        {
            Write_Tryll_MemoryDevice(W, T->devices()->Get(i), Omitted, Prefix + "devices.");
        }
        W.EndArray();
    }
    if (T->components() != nullptr)
    {
        W.Key("components");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->components()->size(); ++i)
        {
            Write_Tryll_MemoryComponent(W, T->components()->Get(i), Omitted, Prefix + "components.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_ModelDependency(FTryllJsonWriter& W, const ::Tryll::ModelDependency* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->role() != nullptr)
    {
        W.Key("role");
        W.String(std::string_view(T->role()->c_str(), T->role()->size()));
    }
    W.Key("status");
    switch (T->status())
    {
    case ::Tryll::ModelStatus_Absent: W.String("Absent"); break;
    case ::Tryll::ModelStatus_Local: W.String("Local"); break;
    case ::Tryll::ModelStatus_Downloading: W.String("Downloading"); break;
    case ::Tryll::ModelStatus_Loaded: W.String("Loaded"); break;
    case ::Tryll::ModelStatus_Downloaded: W.String("Downloaded"); break;
    case ::Tryll::ModelStatus_Incomplete: W.String("Incomplete"); break;
    default: W.Int64(static_cast<std::int64_t>(T->status())); break;
    }
    W.Key("size_bytes");
    W.UInt64(T->size_bytes());
    if (T->folder() != nullptr)
    {
        W.Key("folder");
        W.String(std::string_view(T->folder()->c_str(), T->folder()->size()));
    }
    if (T->local_path() != nullptr)
    {
        W.Key("local_path");
        W.String(std::string_view(T->local_path()->c_str(), T->local_path()->size()));
    }
    if (T->failure_reason() != nullptr)
    {
        W.Key("failure_reason");
        W.String(std::string_view(T->failure_reason()->c_str(), T->failure_reason()->size()));
    }
    W.EndObject();
}

void Write_Tryll_ModelInfo(FTryllJsonWriter& W, const ::Tryll::ModelInfo* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.Key("status");
    switch (T->status())
    {
    case ::Tryll::ModelStatus_Absent: W.String("Absent"); break;
    case ::Tryll::ModelStatus_Local: W.String("Local"); break;
    case ::Tryll::ModelStatus_Downloading: W.String("Downloading"); break;
    case ::Tryll::ModelStatus_Loaded: W.String("Loaded"); break;
    case ::Tryll::ModelStatus_Downloaded: W.String("Downloaded"); break;
    case ::Tryll::ModelStatus_Incomplete: W.String("Incomplete"); break;
    default: W.Int64(static_cast<std::int64_t>(T->status())); break;
    }
    if (T->huggingface_repo() != nullptr)
    {
        W.Key("huggingface_repo");
        W.String(std::string_view(T->huggingface_repo()->c_str(), T->huggingface_repo()->size()));
    }
    W.Key("size_bytes");
    W.UInt64(T->size_bytes());
    if (T->model_type() != nullptr)
    {
        W.Key("model_type");
        W.String(std::string_view(T->model_type()->c_str(), T->model_type()->size()));
    }
    if (T->engine() != nullptr)
    {
        W.Key("engine");
        W.String(std::string_view(T->engine()->c_str(), T->engine()->size()));
    }
    if (T->audience() != nullptr)
    {
        W.Key("audience");
        W.String(std::string_view(T->audience()->c_str(), T->audience()->size()));
    }
    if (T->tool_call_format() != nullptr)
    {
        W.Key("tool_call_format");
        W.String(std::string_view(T->tool_call_format()->c_str(), T->tool_call_format()->size()));
    }
    W.Key("context_size");
    W.Int64(T->context_size());
    if (T->files() != nullptr)
    {
        W.Key("files");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->files()->size(); ++i)
        {
            W.String(std::string_view(T->files()->Get(i)->c_str(), T->files()->Get(i)->size()));
        }
        W.EndArray();
    }
    if (T->default_sampling() != nullptr)
    {
        W.Key("default_sampling");
        Write_Tryll_ModelSampling(W, T->default_sampling(), Omitted, Prefix + "default_sampling.");
    }
    if (T->local_path() != nullptr)
    {
        W.Key("local_path");
        W.String(std::string_view(T->local_path()->c_str(), T->local_path()->size()));
    }
    if (T->tool_call_support() != nullptr)
    {
        W.Key("tool_call_support");
        W.String(std::string_view(T->tool_call_support()->c_str(), T->tool_call_support()->size()));
    }
    if (T->languages() != nullptr)
    {
        W.Key("languages");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->languages()->size(); ++i)
        {
            W.String(std::string_view(T->languages()->Get(i)->c_str(), T->languages()->Get(i)->size()));
        }
        W.EndArray();
    }
    if (T->dependencies() != nullptr)
    {
        W.Key("dependencies");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->dependencies()->size(); ++i)
        {
            Write_Tryll_ModelDependency(W, T->dependencies()->Get(i), Omitted, Prefix + "dependencies.");
        }
        W.EndArray();
    }
    if (T->tts_capabilities() != nullptr)
    {
        W.Key("tts_capabilities");
        Write_Tryll_TtsCapabilitiesInfo(W, T->tts_capabilities(), Omitted, Prefix + "tts_capabilities.");
    }
    W.EndObject();
}

void Write_Tryll_ModelPlacementHint(FTryllJsonWriter& W, const ::Tryll::ModelPlacementHint* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.Key("placement");
    switch (T->placement())
    {
    case ::Tryll::MemoryPlacement_Auto: W.String("Auto"); break;
    case ::Tryll::MemoryPlacement_Ram: W.String("Ram"); break;
    case ::Tryll::MemoryPlacement_Vram: W.String("Vram"); break;
    default: W.Int64(static_cast<std::int64_t>(T->placement())); break;
    }
    W.Key("hotwords");
    switch (T->hotwords())
    {
    case ::Tryll::HotwordBias_Default: W.String("Default"); break;
    case ::Tryll::HotwordBias_Off: W.String("Off"); break;
    case ::Tryll::HotwordBias_On: W.String("On"); break;
    default: W.Int64(static_cast<std::int64_t>(T->hotwords())); break;
    }
    W.EndObject();
}

void Write_Tryll_ModelSampling(FTryllJsonWriter& W, const ::Tryll::ModelSampling* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("temperature");
    W.Float(T->temperature());
    W.Key("top_p");
    W.Float(T->top_p());
    W.Key("top_k");
    W.Int64(T->top_k());
    W.Key("min_p");
    W.Float(T->min_p());
    W.Key("max_tokens");
    W.Int64(T->max_tokens());
    W.Key("seed");
    W.UInt64(T->seed());
    W.Key("repeat_penalty");
    W.Float(T->repeat_penalty());
    W.Key("presence_penalty");
    W.Float(T->presence_penalty());
    W.Key("frequency_penalty");
    W.Float(T->frequency_penalty());
    W.EndObject();
}

void Write_Tryll_MutableKbClearRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbClearRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.EndObject();
}

void Write_Tryll_MutableKbClearResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbClearResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("cleared_count");
    W.UInt64(T->cleared_count());
    W.EndObject();
}

void Write_Tryll_MutableKbDeleteRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbDeleteRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->ids() != nullptr)
    {
        W.Key("ids");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->ids()->size(); ++i)
        {
            W.String(std::string_view(T->ids()->Get(i)->c_str(), T->ids()->Get(i)->size()));
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_MutableKbDeleteResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbDeleteResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("deleted_count");
    W.UInt64(T->deleted_count());
    W.EndObject();
}

void Write_Tryll_MutableKbGetRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbGetRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    W.EndObject();
}

void Write_Tryll_MutableKbGetResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbGetResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("found");
    W.Bool(T->found());
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.Key("created_at_ms");
    W.UInt64(T->created_at_ms());
    W.Key("updated_at_ms");
    W.UInt64(T->updated_at_ms());
    W.EndObject();
}

void Write_Tryll_MutableKbHit(FTryllJsonWriter& W, const ::Tryll::MutableKbHit* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.Key("distance");
    W.Float(T->distance());
    W.EndObject();
}

void Write_Tryll_MutableKbInfo(FTryllJsonWriter& W, const ::Tryll::MutableKbInfo* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->schema() != nullptr)
    {
        W.Key("schema");
        W.String(std::string_view(T->schema()->c_str(), T->schema()->size()));
    }
    W.Key("live_count");
    W.UInt64(T->live_count());
    W.Key("write_generation");
    W.UInt64(T->write_generation());
    W.Key("materialized");
    W.Bool(T->materialized());
    W.Key("bytes_estimate");
    W.UInt64(T->bytes_estimate());
    W.EndObject();
}

void Write_Tryll_MutableKbListRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbListRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.Key("offset");
    W.UInt64(T->offset());
    W.Key("limit");
    W.UInt64(T->limit());
    W.EndObject();
}

void Write_Tryll_MutableKbListResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbListResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->records() != nullptr)
    {
        W.Key("records");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->records()->size(); ++i)
        {
            Write_Tryll_MutableKbRecord(W, T->records()->Get(i), Omitted, Prefix + "records.");
        }
        W.EndArray();
    }
    W.Key("next_offset");
    W.UInt64(T->next_offset());
    W.Key("has_more");
    W.Bool(T->has_more());
    W.EndObject();
}

void Write_Tryll_MutableKbQueryRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbQueryRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->query_text() != nullptr)
    {
        W.Key("query_text");
        W.String(std::string_view(T->query_text()->c_str(), T->query_text()->size()));
    }
    W.Key("top_k");
    W.UInt64(T->top_k());
    W.Key("retrieval_mode");
    switch (T->retrieval_mode())
    {
    case ::Tryll::RetrievalMode_Dense: W.String("Dense"); break;
    case ::Tryll::RetrievalMode_Lexical: W.String("Lexical"); break;
    case ::Tryll::RetrievalMode_Hybrid: W.String("Hybrid"); break;
    default: W.Int64(static_cast<std::int64_t>(T->retrieval_mode())); break;
    }
    W.EndObject();
}

void Write_Tryll_MutableKbQueryResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbQueryResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->hits() != nullptr)
    {
        W.Key("hits");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->hits()->size(); ++i)
        {
            Write_Tryll_MutableKbHit(W, T->hits()->Get(i), Omitted, Prefix + "hits.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_MutableKbRecord(FTryllJsonWriter& W, const ::Tryll::MutableKbRecord* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.Key("created_at_ms");
    W.UInt64(T->created_at_ms());
    W.Key("updated_at_ms");
    W.UInt64(T->updated_at_ms());
    W.EndObject();
}

void Write_Tryll_MutableKbStatsRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbStatsRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.EndObject();
}

void Write_Tryll_MutableKbStatsResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbStatsResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("live_count");
    W.UInt64(T->live_count());
    W.Key("tombstoned_count");
    W.UInt64(T->tombstoned_count());
    W.Key("slot_count");
    W.UInt64(T->slot_count());
    W.Key("write_generation");
    W.UInt64(T->write_generation());
    W.Key("bytes_estimate");
    W.UInt64(T->bytes_estimate());
    W.Key("materialized");
    W.Bool(T->materialized());
    W.EndObject();
}

void Write_Tryll_MutableKbUpsert(FTryllJsonWriter& W, const ::Tryll::MutableKbUpsert* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.EndObject();
}

void Write_Tryll_MutableKbWriteRequest(FTryllJsonWriter& W, const ::Tryll::MutableKbWriteRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->schema() != nullptr)
    {
        W.Key("schema");
        W.String(std::string_view(T->schema()->c_str(), T->schema()->size()));
    }
    if (T->records() != nullptr)
    {
        W.Key("records");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->records()->size(); ++i)
        {
            Write_Tryll_MutableKbUpsert(W, T->records()->Get(i), Omitted, Prefix + "records.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_MutableKbWriteResponse(FTryllJsonWriter& W, const ::Tryll::MutableKbWriteResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->results() != nullptr)
    {
        W.Key("results");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->results()->size(); ++i)
        {
            Write_Tryll_MutableKbWriteResult(W, T->results()->Get(i), Omitted, Prefix + "results.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_MutableKbWriteResult(FTryllJsonWriter& W, const ::Tryll::MutableKbWriteResult* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    W.Key("inserted");
    W.Bool(T->inserted());
    W.Key("accepted");
    W.Bool(T->accepted());
    if (T->error() != nullptr)
    {
        W.Key("error");
        W.String(std::string_view(T->error()->c_str(), T->error()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeDescription(FTryllJsonWriter& W, const ::Tryll::NodeDescription* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->params_type() != ::Tryll::NodeParams::NodeParams_NONE)
    {
        W.Key("params_type");
        switch (T->params_type())
        {
        case ::Tryll::NodeParams::NodeParams_GenerateParams: W.String("GenerateParams"); break;
        case ::Tryll::NodeParams::NodeParams_RegexGuardrailParams: W.String("RegexGuardrailParams"); break;
        case ::Tryll::NodeParams::NodeParams_CannedResponseParams: W.String("CannedResponseParams"); break;
        case ::Tryll::NodeParams::NodeParams_ToolCallParams: W.String("ToolCallParams"); break;
        case ::Tryll::NodeParams::NodeParams_RetrieveParams: W.String("RetrieveParams"); break;
        case ::Tryll::NodeParams::NodeParams_InstructionParams: W.String("InstructionParams"); break;
        case ::Tryll::NodeParams::NodeParams_ClassifyIntentParams: W.String("ClassifyIntentParams"); break;
        case ::Tryll::NodeParams::NodeParams_ClassifyIntentLLMParams: W.String("ClassifyIntentLLMParams"); break;
        case ::Tryll::NodeParams::NodeParams_IntentToInstructionParams: W.String("IntentToInstructionParams"); break;
        case ::Tryll::NodeParams::NodeParams_GenerateAndSpeakParams: W.String("GenerateAndSpeakParams"); break;
        case ::Tryll::NodeParams::NodeParams_SpeakParams: W.String("SpeakParams"); break;
        case ::Tryll::NodeParams::NodeParams_PauseParams: W.String("PauseParams"); break;
        case ::Tryll::NodeParams::NodeParams_TransformParams: W.String("TransformParams"); break;
        case ::Tryll::NodeParams::NodeParams_BranchParams: W.String("BranchParams"); break;
        case ::Tryll::NodeParams::NodeParams_WriteToMemoryParams: W.String("WriteToMemoryParams"); break;
        case ::Tryll::NodeParams::NodeParams_SummarizeParams: W.String("SummarizeParams"); break;
        case ::Tryll::NodeParams::NodeParams_ClassifyAttributesLLMParams: W.String("ClassifyAttributesLLMParams"); break;
        default: W.Int64(static_cast<std::int64_t>(T->params_type())); break;
        }
        W.Key("params");
        switch (T->params_type())
        {
        case ::Tryll::NodeParams::NodeParams_GenerateParams:
            Write_Tryll_NodeParams_GenerateParams(W, T->params_as_GenerateParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_RegexGuardrailParams:
            Write_Tryll_NodeParams_RegexGuardrailParams(W, T->params_as_RegexGuardrailParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_CannedResponseParams:
            Write_Tryll_NodeParams_CannedResponseParams(W, T->params_as_CannedResponseParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_ToolCallParams:
            Write_Tryll_NodeParams_ToolCallParams(W, T->params_as_ToolCallParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_RetrieveParams:
            Write_Tryll_NodeParams_RetrieveParams(W, T->params_as_RetrieveParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_InstructionParams:
            Write_Tryll_NodeParams_InstructionParams(W, T->params_as_InstructionParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_ClassifyIntentParams:
            Write_Tryll_NodeParams_ClassifyIntentParams(W, T->params_as_ClassifyIntentParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_ClassifyIntentLLMParams:
            Write_Tryll_NodeParams_ClassifyIntentLLMParams(W, T->params_as_ClassifyIntentLLMParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_IntentToInstructionParams:
            Write_Tryll_NodeParams_IntentToInstructionParams(W, T->params_as_IntentToInstructionParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_GenerateAndSpeakParams:
            Write_Tryll_NodeParams_GenerateAndSpeakParams(W, T->params_as_GenerateAndSpeakParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_SpeakParams:
            Write_Tryll_NodeParams_SpeakParams(W, T->params_as_SpeakParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_PauseParams:
            Write_Tryll_NodeParams_PauseParams(W, T->params_as_PauseParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_TransformParams:
            Write_Tryll_NodeParams_TransformParams(W, T->params_as_TransformParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_BranchParams:
            Write_Tryll_NodeParams_BranchParams(W, T->params_as_BranchParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_WriteToMemoryParams:
            Write_Tryll_NodeParams_WriteToMemoryParams(W, T->params_as_WriteToMemoryParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_SummarizeParams:
            Write_Tryll_NodeParams_SummarizeParams(W, T->params_as_SummarizeParams(), Omitted, Prefix + "params.");
            break;
        case ::Tryll::NodeParams::NodeParams_ClassifyAttributesLLMParams:
            Write_Tryll_NodeParams_ClassifyAttributesLLMParams(W, T->params_as_ClassifyAttributesLLMParams(), Omitted, Prefix + "params.");
            break;
        default: W.BeginObject(); W.EndObject(); break;
        }
    }
    W.EndObject();
}

void Write_Tryll_NodeEvent(FTryllJsonWriter& W, const ::Tryll::NodeEvent* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->node_name() != nullptr)
    {
        W.Key("node_name");
        W.String(std::string_view(T->node_name()->c_str(), T->node_name()->size()));
    }
    if (T->event_type() != nullptr)
    {
        W.Key("event_type");
        W.String(std::string_view(T->event_type()->c_str(), T->event_type()->size()));
    }
    if (T->kv_pairs() != nullptr)
    {
        W.Key("kv_pairs");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->kv_pairs()->size(); ++i)
        {
            Write_Tryll_NodeParam(W, T->kv_pairs()->Get(i), Omitted, Prefix + "kv_pairs.");
        }
        W.EndArray();
    }
    W.EndObject();
}

std::string Label_Tryll_NodeEvent(const ::Tryll::NodeEvent* T)
{
    if (T == nullptr) return {};
    return TryllFormatLogLabel(
        "{event_type} · {node_name}",
        {
            { "event_type", (T->event_type() ? T->event_type()->str() : std::string()) },
            { "node_name", (T->node_name() ? T->node_name()->str() : std::string()) },
        });
}

void Write_Tryll_NodeParam(FTryllJsonWriter& W, const ::Tryll::NodeParam* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->key() != nullptr)
    {
        W.Key("key");
        W.String(std::string_view(T->key()->c_str(), T->key()->size()));
    }
    if (T->value() != nullptr)
    {
        W.Key("value");
        W.String(std::string_view(T->value()->c_str(), T->value()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_BranchParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::BranchParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("test");
    switch (T->test())
    {
    case ::Tryll::BranchTest_NonEmpty: W.String("NonEmpty"); break;
    case ::Tryll::BranchTest_IsTrue: W.String("IsTrue"); break;
    case ::Tryll::BranchTest_Equals: W.String("Equals"); break;
    case ::Tryll::BranchTest_EqualsIgnoreCase: W.String("EqualsIgnoreCase"); break;
    case ::Tryll::BranchTest_Contains: W.String("Contains"); break;
    case ::Tryll::BranchTest_Regex: W.String("Regex"); break;
    case ::Tryll::BranchTest_Expression: W.String("Expression"); break;
    default: W.Int64(static_cast<std::int64_t>(T->test())); break;
    }
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    if (T->variable() != nullptr)
    {
        W.Key("variable");
        W.String(std::string_view(T->variable()->c_str(), T->variable()->size()));
    }
    if (T->values() != nullptr)
    {
        W.Key("values");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->values()->size(); ++i)
        {
            W.String(std::string_view(T->values()->Get(i)->c_str(), T->values()->Get(i)->size()));
        }
        W.EndArray();
    }
    if (T->condition() != nullptr)
    {
        W.Key("condition");
        W.String(std::string_view(T->condition()->c_str(), T->condition()->size()));
    }
    W.Key("notify_client");
    W.Bool(T->notify_client());
    if (T->then_exit() != nullptr)
    {
        W.Key("then_exit");
        W.String(std::string_view(T->then_exit()->c_str(), T->then_exit()->size()));
    }
    if (T->else_exit() != nullptr)
    {
        W.Key("else_exit");
        W.String(std::string_view(T->else_exit()->c_str(), T->else_exit()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_CannedResponseParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::CannedResponseParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->string_storage() != nullptr)
    {
        W.Key("string_storage");
        W.String(std::string_view(T->string_storage()->c_str(), T->string_storage()->size()));
    }
    W.Key("selection_strategy");
    switch (T->selection_strategy())
    {
    case ::Tryll::CannedResponseSelectionStrategy_Random: W.String("Random"); break;
    case ::Tryll::CannedResponseSelectionStrategy_First: W.String("First"); break;
    case ::Tryll::CannedResponseSelectionStrategy_RoundRobin: W.String("RoundRobin"); break;
    default: W.Int64(static_cast<std::int64_t>(T->selection_strategy())); break;
    }
    if (T->output_name() != nullptr)
    {
        W.Key("output_name");
        W.String(std::string_view(T->output_name()->c_str(), T->output_name()->size()));
    }
    W.Key("send");
    switch (T->send())
    {
    case ::Tryll::SendAnswer_None: W.String("None"); break;
    case ::Tryll::SendAnswer_Whole: W.String("Whole"); break;
    case ::Tryll::SendAnswer_Streamed: W.String("Streamed"); break;
    default: W.Int64(static_cast<std::int64_t>(T->send())); break;
    }
    W.Key("history_role");
    switch (T->history_role())
    {
    case ::Tryll::HistoryRole_None: W.String("None"); break;
    case ::Tryll::HistoryRole_Assistant: W.String("Assistant"); break;
    case ::Tryll::HistoryRole_User: W.String("User"); break;
    case ::Tryll::HistoryRole_System: W.String("System"); break;
    default: W.Int64(static_cast<std::int64_t>(T->history_role())); break;
    }
    if (T->default_exit() != nullptr)
    {
        W.Key("default_exit");
        W.String(std::string_view(T->default_exit()->c_str(), T->default_exit()->size()));
    }
    if (T->lines() != nullptr)
    {
        W.Key("lines");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->lines()->size(); ++i)
        {
            Write_Tryll_NodeParams_LocalizableText(W, T->lines()->Get(i), Omitted, Prefix + "lines.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_ClassifyAttributeChoice(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyAttributeChoice* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->prompt() != nullptr)
    {
        W.Key("prompt");
        W.String(std::string_view(T->prompt()->c_str(), T->prompt()->size()));
    }
    W.Key("triggers");
    W.Bool(T->triggers());
    W.EndObject();
}

void Write_Tryll_NodeParams_ClassifyAttributeDef(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyAttributeDef* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.Key("kind");
    switch (T->kind())
    {
    case ::Tryll::AttributeKind_Choice: W.String("Choice"); break;
    case ::Tryll::AttributeKind_Boolean: W.String("Boolean"); break;
    case ::Tryll::AttributeKind_Score: W.String("Score"); break;
    default: W.Int64(static_cast<std::int64_t>(T->kind())); break;
    }
    if (T->choices() != nullptr)
    {
        W.Key("choices");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->choices()->size(); ++i)
        {
            Write_Tryll_NodeParams_ClassifyAttributeChoice(W, T->choices()->Get(i), Omitted, Prefix + "choices.");
        }
        W.EndArray();
    }
    if (T->statement() != nullptr)
    {
        W.Key("statement");
        W.String(std::string_view(T->statement()->c_str(), T->statement()->size()));
    }
    if (T->system_prompt() != nullptr)
    {
        W.Key("system_prompt");
        W.String(std::string_view(T->system_prompt()->c_str(), T->system_prompt()->size()));
    }
    W.Key("threshold");
    W.Float(T->threshold());
    W.Key("margin");
    W.Float(T->margin());
    if (T->uncertain_choice() != nullptr)
    {
        W.Key("uncertain_choice");
        W.String(std::string_view(T->uncertain_choice()->c_str(), T->uncertain_choice()->size()));
    }
    W.Key("boolean_trigger");
    switch (T->boolean_trigger())
    {
    case ::Tryll::BooleanTrigger_Never: W.String("Never"); break;
    case ::Tryll::BooleanTrigger_True: W.String("True"); break;
    case ::Tryll::BooleanTrigger_False: W.String("False"); break;
    default: W.Int64(static_cast<std::int64_t>(T->boolean_trigger())); break;
    }
    W.Key("score_trigger");
    switch (T->score_trigger())
    {
    case ::Tryll::ScoreTrigger_Never: W.String("Never"); break;
    case ::Tryll::ScoreTrigger_AtOrAbove: W.String("AtOrAbove"); break;
    case ::Tryll::ScoreTrigger_Below: W.String("Below"); break;
    default: W.Int64(static_cast<std::int64_t>(T->score_trigger())); break;
    }
    W.Key("score_threshold");
    W.Float(T->score_threshold());
    W.EndObject();
}

void Write_Tryll_NodeParams_ClassifyAttributesLLMParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyAttributesLLMParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.Key("context_size");
    W.Int64(T->context_size());
    if (T->attributes() != nullptr)
    {
        W.Key("attributes");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->attributes()->size(); ++i)
        {
            Write_Tryll_NodeParams_ClassifyAttributeDef(W, T->attributes()->Get(i), Omitted, Prefix + "attributes.");
        }
        W.EndArray();
    }
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    W.Key("history_turns");
    W.Int64(T->history_turns());
    W.Key("temperature");
    W.Float(T->temperature());
    W.Key("on_failure");
    switch (T->on_failure())
    {
    case ::Tryll::AttributeFailureRoute_Default: W.String("Default"); break;
    case ::Tryll::AttributeFailureRoute_Triggered: W.String("Triggered"); break;
    default: W.Int64(static_cast<std::int64_t>(T->on_failure())); break;
    }
    W.Key("notify_client");
    switch (T->notify_client())
    {
    case ::Tryll::NotifyClient_Disabled: W.String("Disabled"); break;
    case ::Tryll::NotifyClient_OnFound: W.String("OnFound"); break;
    case ::Tryll::NotifyClient_Always: W.String("Always"); break;
    default: W.Int64(static_cast<std::int64_t>(T->notify_client())); break;
    }
    if (T->output_name() != nullptr)
    {
        W.Key("output_name");
        W.String(std::string_view(T->output_name()->c_str(), T->output_name()->size()));
    }
    if (T->default_exit() != nullptr)
    {
        W.Key("default_exit");
        W.String(std::string_view(T->default_exit()->c_str(), T->default_exit()->size()));
    }
    if (T->triggered_exit() != nullptr)
    {
        W.Key("triggered_exit");
        W.String(std::string_view(T->triggered_exit()->c_str(), T->triggered_exit()->size()));
    }
    if (T->context() != nullptr)
    {
        W.Key("context");
        Write_Tryll_NodeParams_ContextOverrides(W, T->context(), Omitted, Prefix + "context.");
    }
    if (T->adaptation() != nullptr)
    {
        W.Key("adaptation");
        Write_Tryll_NodeParams_ModelAdaptation(W, T->adaptation(), Omitted, Prefix + "adaptation.");
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_ClassifyIntentEntry(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyIntentEntry* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->prompt() != nullptr)
    {
        W.Key("prompt");
        W.String(std::string_view(T->prompt()->c_str(), T->prompt()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_ClassifyIntentLLMParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyIntentLLMParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.Key("context_size");
    W.Int64(T->context_size());
    if (T->system_prompt() != nullptr)
    {
        W.Key("system_prompt");
        W.String(std::string_view(T->system_prompt()->c_str(), T->system_prompt()->size()));
    }
    if (T->intents() != nullptr)
    {
        W.Key("intents");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->intents()->size(); ++i)
        {
            Write_Tryll_NodeParams_ClassifyIntentEntry(W, T->intents()->Get(i), Omitted, Prefix + "intents.");
        }
        W.EndArray();
    }
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    if (T->output_name() != nullptr)
    {
        W.Key("output_name");
        W.String(std::string_view(T->output_name()->c_str(), T->output_name()->size()));
    }
    W.Key("history_turns");
    W.Int64(T->history_turns());
    W.Key("threshold");
    W.Float(T->threshold());
    W.Key("margin");
    W.Float(T->margin());
    W.Key("temperature");
    W.Float(T->temperature());
    W.Key("notify_client");
    switch (T->notify_client())
    {
    case ::Tryll::NotifyClient_Disabled: W.String("Disabled"); break;
    case ::Tryll::NotifyClient_OnFound: W.String("OnFound"); break;
    case ::Tryll::NotifyClient_Always: W.String("Always"); break;
    default: W.Int64(static_cast<std::int64_t>(T->notify_client())); break;
    }
    if (T->not_found_intent() != nullptr)
    {
        W.Key("not_found_intent");
        W.String(std::string_view(T->not_found_intent()->c_str(), T->not_found_intent()->size()));
    }
    if (T->found_exit() != nullptr)
    {
        W.Key("found_exit");
        W.String(std::string_view(T->found_exit()->c_str(), T->found_exit()->size()));
    }
    if (T->not_found_exit() != nullptr)
    {
        W.Key("not_found_exit");
        W.String(std::string_view(T->not_found_exit()->c_str(), T->not_found_exit()->size()));
    }
    if (T->context() != nullptr)
    {
        W.Key("context");
        Write_Tryll_NodeParams_ContextOverrides(W, T->context(), Omitted, Prefix + "context.");
    }
    if (T->adaptation() != nullptr)
    {
        W.Key("adaptation");
        Write_Tryll_NodeParams_ModelAdaptation(W, T->adaptation(), Omitted, Prefix + "adaptation.");
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_ClassifyIntentParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::ClassifyIntentParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->embedded_string_storage() != nullptr)
    {
        W.Key("embedded_string_storage");
        W.String(std::string_view(T->embedded_string_storage()->c_str(), T->embedded_string_storage()->size()));
    }
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    if (T->metadata_field() != nullptr)
    {
        W.Key("metadata_field");
        W.String(std::string_view(T->metadata_field()->c_str(), T->metadata_field()->size()));
    }
    W.Key("threshold");
    W.Float(T->threshold());
    if (T->filter() != nullptr)
    {
        W.Key("filter");
        W.String(std::string_view(T->filter()->c_str(), T->filter()->size()));
    }
    W.Key("notify_client");
    switch (T->notify_client())
    {
    case ::Tryll::NotifyClient_Disabled: W.String("Disabled"); break;
    case ::Tryll::NotifyClient_OnFound: W.String("OnFound"); break;
    case ::Tryll::NotifyClient_Always: W.String("Always"); break;
    default: W.Int64(static_cast<std::int64_t>(T->notify_client())); break;
    }
    W.Key("diagnostic_topk");
    W.Int64(T->diagnostic_topk());
    if (T->found_exit() != nullptr)
    {
        W.Key("found_exit");
        W.String(std::string_view(T->found_exit()->c_str(), T->found_exit()->size()));
    }
    if (T->not_found_exit() != nullptr)
    {
        W.Key("not_found_exit");
        W.String(std::string_view(T->not_found_exit()->c_str(), T->not_found_exit()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_ContextOverrides(FTryllJsonWriter& W, const ::Tryll::NodeParams::ContextOverrides* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("kv_cache_type");
    switch (T->kv_cache_type())
    {
    case ::Tryll::KvCacheType_Inherit: W.String("Inherit"); break;
    case ::Tryll::KvCacheType_F16: W.String("F16"); break;
    case ::Tryll::KvCacheType_Q8_0: W.String("Q8_0"); break;
    case ::Tryll::KvCacheType_Q4_0: W.String("Q4_0"); break;
    default: W.Int64(static_cast<std::int64_t>(T->kv_cache_type())); break;
    }
    W.Key("offload_kqv");
    if (const auto Value = T->offload_kqv())
        W.Bool((*Value));
    else W.Null();
    W.Key("n_ubatch");
    if (const auto Value = T->n_ubatch())
        W.Int64((*Value));
    else W.Null();
    W.Key("n_outputs_max");
    if (const auto Value = T->n_outputs_max())
        W.Int64((*Value));
    else W.Null();
    W.Key("n_batch");
    if (const auto Value = T->n_batch())
        W.Int64((*Value));
    else W.Null();
    W.EndObject();
}

void Write_Tryll_NodeParams_GenerateAndSpeakParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::GenerateAndSpeakParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.Key("context_size");
    W.Int64(T->context_size());
    if (T->system_prompt() != nullptr)
    {
        W.Key("system_prompt");
        W.String(std::string_view(T->system_prompt()->c_str(), T->system_prompt()->size()));
    }
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    if (T->template_() != nullptr)
    {
        W.Key("template");
        W.String(std::string_view(T->template_()->c_str(), T->template_()->size()));
    }
    W.Key("placement");
    switch (T->placement())
    {
    case ::Tryll::Placement_BeforeUserAsUser: W.String("BeforeUserAsUser"); break;
    case ::Tryll::Placement_BeforeUserAsSystem: W.String("BeforeUserAsSystem"); break;
    case ::Tryll::Placement_AfterUserAsUser: W.String("AfterUserAsUser"); break;
    case ::Tryll::Placement_AfterUserAsSystem: W.String("AfterUserAsSystem"); break;
    default: W.Int64(static_cast<std::int64_t>(T->placement())); break;
    }
    if (T->output_name() != nullptr)
    {
        W.Key("output_name");
        W.String(std::string_view(T->output_name()->c_str(), T->output_name()->size()));
    }
    W.Key("history_role");
    switch (T->history_role())
    {
    case ::Tryll::HistoryRole_None: W.String("None"); break;
    case ::Tryll::HistoryRole_Assistant: W.String("Assistant"); break;
    case ::Tryll::HistoryRole_User: W.String("User"); break;
    case ::Tryll::HistoryRole_System: W.String("System"); break;
    default: W.Int64(static_cast<std::int64_t>(T->history_role())); break;
    }
    if (T->sampling() != nullptr)
    {
        W.Key("sampling");
        Write_Tryll_NodeParams_SamplingOverrides(W, T->sampling(), Omitted, Prefix + "sampling.");
    }
    if (T->grammar() != nullptr)
    {
        W.Key("grammar");
        W.String(std::string_view(T->grammar()->c_str(), T->grammar()->size()));
    }
    W.Key("min_sentence_chars");
    W.Int64(T->min_sentence_chars());
    W.Key("first_chunk_max_chars");
    W.Int64(T->first_chunk_max_chars());
    if (T->default_exit() != nullptr)
    {
        W.Key("default_exit");
        W.String(std::string_view(T->default_exit()->c_str(), T->default_exit()->size()));
    }
    W.Key("substitute_agent_variables");
    W.Bool(T->substitute_agent_variables());
    if (T->output_filter() != nullptr)
    {
        W.Key("output_filter");
        Write_Tryll_NodeParams_OutputFilterOverrides(W, T->output_filter(), Omitted, Prefix + "output_filter.");
    }
    if (T->context() != nullptr)
    {
        W.Key("context");
        Write_Tryll_NodeParams_ContextOverrides(W, T->context(), Omitted, Prefix + "context.");
    }
    if (T->adaptation() != nullptr)
    {
        W.Key("adaptation");
        Write_Tryll_NodeParams_ModelAdaptation(W, T->adaptation(), Omitted, Prefix + "adaptation.");
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_GenerateParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::GenerateParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.Key("context_size");
    W.Int64(T->context_size());
    if (T->system_prompt() != nullptr)
    {
        W.Key("system_prompt");
        W.String(std::string_view(T->system_prompt()->c_str(), T->system_prompt()->size()));
    }
    if (T->output_name() != nullptr)
    {
        W.Key("output_name");
        W.String(std::string_view(T->output_name()->c_str(), T->output_name()->size()));
    }
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    if (T->template_() != nullptr)
    {
        W.Key("template");
        W.String(std::string_view(T->template_()->c_str(), T->template_()->size()));
    }
    W.Key("placement");
    switch (T->placement())
    {
    case ::Tryll::Placement_BeforeUserAsUser: W.String("BeforeUserAsUser"); break;
    case ::Tryll::Placement_BeforeUserAsSystem: W.String("BeforeUserAsSystem"); break;
    case ::Tryll::Placement_AfterUserAsUser: W.String("AfterUserAsUser"); break;
    case ::Tryll::Placement_AfterUserAsSystem: W.String("AfterUserAsSystem"); break;
    default: W.Int64(static_cast<std::int64_t>(T->placement())); break;
    }
    W.Key("send");
    switch (T->send())
    {
    case ::Tryll::SendAnswer_None: W.String("None"); break;
    case ::Tryll::SendAnswer_Whole: W.String("Whole"); break;
    case ::Tryll::SendAnswer_Streamed: W.String("Streamed"); break;
    default: W.Int64(static_cast<std::int64_t>(T->send())); break;
    }
    W.Key("history_role");
    switch (T->history_role())
    {
    case ::Tryll::HistoryRole_None: W.String("None"); break;
    case ::Tryll::HistoryRole_Assistant: W.String("Assistant"); break;
    case ::Tryll::HistoryRole_User: W.String("User"); break;
    case ::Tryll::HistoryRole_System: W.String("System"); break;
    default: W.Int64(static_cast<std::int64_t>(T->history_role())); break;
    }
    if (T->sampling() != nullptr)
    {
        W.Key("sampling");
        Write_Tryll_NodeParams_SamplingOverrides(W, T->sampling(), Omitted, Prefix + "sampling.");
    }
    if (T->grammar() != nullptr)
    {
        W.Key("grammar");
        W.String(std::string_view(T->grammar()->c_str(), T->grammar()->size()));
    }
    if (T->default_exit() != nullptr)
    {
        W.Key("default_exit");
        W.String(std::string_view(T->default_exit()->c_str(), T->default_exit()->size()));
    }
    W.Key("substitute_agent_variables");
    W.Bool(T->substitute_agent_variables());
    if (T->output_filter() != nullptr)
    {
        W.Key("output_filter");
        Write_Tryll_NodeParams_OutputFilterOverrides(W, T->output_filter(), Omitted, Prefix + "output_filter.");
    }
    if (T->context() != nullptr)
    {
        W.Key("context");
        Write_Tryll_NodeParams_ContextOverrides(W, T->context(), Omitted, Prefix + "context.");
    }
    if (T->adaptation() != nullptr)
    {
        W.Key("adaptation");
        Write_Tryll_NodeParams_ModelAdaptation(W, T->adaptation(), Omitted, Prefix + "adaptation.");
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_InstructionParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::InstructionParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->instruction() != nullptr)
    {
        W.Key("instruction");
        W.String(std::string_view(T->instruction()->c_str(), T->instruction()->size()));
    }
    if (T->output_name() != nullptr)
    {
        W.Key("output_name");
        W.String(std::string_view(T->output_name()->c_str(), T->output_name()->size()));
    }
    if (T->default_exit() != nullptr)
    {
        W.Key("default_exit");
        W.String(std::string_view(T->default_exit()->c_str(), T->default_exit()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_IntentToInstructionParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::IntentToInstructionParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->string_storage() != nullptr)
    {
        W.Key("string_storage");
        W.String(std::string_view(T->string_storage()->c_str(), T->string_storage()->size()));
    }
    if (T->inline_keys() != nullptr)
    {
        W.Key("inline_keys");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->inline_keys()->size(); ++i)
        {
            W.String(std::string_view(T->inline_keys()->Get(i)->c_str(), T->inline_keys()->Get(i)->size()));
        }
        W.EndArray();
    }
    if (T->output_name() != nullptr)
    {
        W.Key("output_name");
        W.String(std::string_view(T->output_name()->c_str(), T->output_name()->size()));
    }
    if (T->default_exit() != nullptr)
    {
        W.Key("default_exit");
        W.String(std::string_view(T->default_exit()->c_str(), T->default_exit()->size()));
    }
    if (T->instructions() != nullptr)
    {
        W.Key("instructions");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->instructions()->size(); ++i)
        {
            Write_Tryll_NodeParams_LocalizableText(W, T->instructions()->Get(i), Omitted, Prefix + "instructions.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_LocalizableText(FTryllJsonWriter& W, const ::Tryll::NodeParams::LocalizableText* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_LoraAdapter(FTryllJsonWriter& W, const ::Tryll::NodeParams::LoraAdapter* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->path() != nullptr)
    {
        W.Key("path");
        W.String(std::string_view(T->path()->c_str(), T->path()->size()));
    }
    W.Key("scale");
    W.Float(T->scale());
    W.EndObject();
}

void Write_Tryll_NodeParams_ModelAdaptation(FTryllJsonWriter& W, const ::Tryll::NodeParams::ModelAdaptation* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->loras() != nullptr)
    {
        W.Key("loras");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->loras()->size(); ++i)
        {
            Write_Tryll_NodeParams_LoraAdapter(W, T->loras()->Get(i), Omitted, Prefix + "loras.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_OutputFilterOverrides(FTryllJsonWriter& W, const ::Tryll::NodeParams::OutputFilterOverrides* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("strip_speaker_prefix");
    W.Bool(T->strip_speaker_prefix());
    W.Key("max_prefix_bytes");
    W.Int64(T->max_prefix_bytes());
    W.Key("strip_asterisk_spans");
    W.Bool(T->strip_asterisk_spans());
    W.Key("strip_paren_spans");
    W.Bool(T->strip_paren_spans());
    W.Key("strip_bracket_spans");
    W.Bool(T->strip_bracket_spans());
    W.Key("max_span_bytes");
    W.Int64(T->max_span_bytes());
    W.EndObject();
}

void Write_Tryll_NodeParams_PauseParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::PauseParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->default_exit() != nullptr)
    {
        W.Key("default_exit");
        W.String(std::string_view(T->default_exit()->c_str(), T->default_exit()->size()));
    }
    W.Key("pause_timeout_ms");
    W.Int64(T->pause_timeout_ms());
    if (T->timeout_exit() != nullptr)
    {
        W.Key("timeout_exit");
        W.String(std::string_view(T->timeout_exit()->c_str(), T->timeout_exit()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_RegexGuardrailParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::RegexGuardrailParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->string_storage() != nullptr)
    {
        W.Key("string_storage");
        W.String(std::string_view(T->string_storage()->c_str(), T->string_storage()->size()));
    }
    if (T->inline_strings() != nullptr)
    {
        W.Key("inline_strings");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->inline_strings()->size(); ++i)
        {
            W.String(std::string_view(T->inline_strings()->Get(i)->c_str(), T->inline_strings()->Get(i)->size()));
        }
        W.EndArray();
    }
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    if (T->triggered_exit() != nullptr)
    {
        W.Key("triggered_exit");
        W.String(std::string_view(T->triggered_exit()->c_str(), T->triggered_exit()->size()));
    }
    if (T->not_triggered_exit() != nullptr)
    {
        W.Key("not_triggered_exit");
        W.String(std::string_view(T->not_triggered_exit()->c_str(), T->not_triggered_exit()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_RetrieveParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::RetrieveParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->embedded_string_storage() != nullptr)
    {
        W.Key("embedded_string_storage");
        W.String(std::string_view(T->embedded_string_storage()->c_str(), T->embedded_string_storage()->size()));
    }
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    W.Key("top_k");
    W.Int64(T->top_k());
    W.Key("threshold");
    W.Float(T->threshold());
    W.Key("retrieval_mode");
    switch (T->retrieval_mode())
    {
    case ::Tryll::RetrievalMode_Dense: W.String("Dense"); break;
    case ::Tryll::RetrievalMode_Lexical: W.String("Lexical"); break;
    case ::Tryll::RetrievalMode_Hybrid: W.String("Hybrid"); break;
    default: W.Int64(static_cast<std::int64_t>(T->retrieval_mode())); break;
    }
    W.Key("rrf_k");
    W.Int64(T->rrf_k());
    if (T->source() != nullptr)
    {
        W.Key("source");
        W.String(std::string_view(T->source()->c_str(), T->source()->size()));
    }
    if (T->filter() != nullptr)
    {
        W.Key("filter");
        W.String(std::string_view(T->filter()->c_str(), T->filter()->size()));
    }
    if (T->found_exit() != nullptr)
    {
        W.Key("found_exit");
        W.String(std::string_view(T->found_exit()->c_str(), T->found_exit()->size()));
    }
    if (T->not_found_exit() != nullptr)
    {
        W.Key("not_found_exit");
        W.String(std::string_view(T->not_found_exit()->c_str(), T->not_found_exit()->size()));
    }
    if (T->storage_name() != nullptr)
    {
        W.Key("storage_name");
        W.String(std::string_view(T->storage_name()->c_str(), T->storage_name()->size()));
    }
    if (T->schema() != nullptr)
    {
        W.Key("schema");
        W.String(std::string_view(T->schema()->c_str(), T->schema()->size()));
    }
    W.Key("source_kind");
    switch (T->source_kind())
    {
    case ::Tryll::KnowledgeSourceKind_FromFile: W.String("FromFile"); break;
    case ::Tryll::KnowledgeSourceKind_FromName: W.String("FromName"); break;
    default: W.Int64(static_cast<std::int64_t>(T->source_kind())); break;
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_SamplingOverrides(FTryllJsonWriter& W, const ::Tryll::NodeParams::SamplingOverrides* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("temperature");
    if (const auto Value = T->temperature())
        W.Float((*Value));
    else W.Null();
    W.Key("top_p");
    if (const auto Value = T->top_p())
        W.Float((*Value));
    else W.Null();
    W.Key("top_k");
    if (const auto Value = T->top_k())
        W.Int64((*Value));
    else W.Null();
    W.Key("min_p");
    if (const auto Value = T->min_p())
        W.Float((*Value));
    else W.Null();
    W.Key("max_tokens");
    if (const auto Value = T->max_tokens())
        W.Int64((*Value));
    else W.Null();
    W.Key("seed");
    if (const auto Value = T->seed())
        W.Int64((*Value));
    else W.Null();
    W.Key("repeat_penalty");
    if (const auto Value = T->repeat_penalty())
        W.Float((*Value));
    else W.Null();
    W.Key("presence_penalty");
    if (const auto Value = T->presence_penalty())
        W.Float((*Value));
    else W.Null();
    W.Key("frequency_penalty");
    if (const auto Value = T->frequency_penalty())
        W.Float((*Value));
    else W.Null();
    W.EndObject();
}

void Write_Tryll_NodeParams_SpeakParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::SpeakParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    W.Key("min_sentence_chars");
    W.Int64(T->min_sentence_chars());
    if (T->default_exit() != nullptr)
    {
        W.Key("default_exit");
        W.String(std::string_view(T->default_exit()->c_str(), T->default_exit()->size()));
    }
    W.Key("first_chunk_max_chars");
    W.Int64(T->first_chunk_max_chars());
    W.EndObject();
}

void Write_Tryll_NodeParams_SummarizeParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::SummarizeParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.Key("context_size");
    W.Int64(T->context_size());
    if (T->system_prompt() != nullptr)
    {
        W.Key("system_prompt");
        W.String(std::string_view(T->system_prompt()->c_str(), T->system_prompt()->size()));
    }
    if (T->template_() != nullptr)
    {
        W.Key("template");
        W.String(std::string_view(T->template_()->c_str(), T->template_()->size()));
    }
    W.Key("trigger_tokens");
    W.Int64(T->trigger_tokens());
    W.Key("trigger_turns");
    W.Int64(T->trigger_turns());
    W.Key("keep_recent_turns");
    W.Int64(T->keep_recent_turns());
    W.Key("max_summary_tokens");
    W.Int64(T->max_summary_tokens());
    if (T->sampling() != nullptr)
    {
        W.Key("sampling");
        Write_Tryll_NodeParams_SamplingOverrides(W, T->sampling(), Omitted, Prefix + "sampling.");
    }
    W.Key("notify_client");
    W.Bool(T->notify_client());
    W.Key("run_inline");
    W.Bool(T->run_inline());
    if (T->default_exit() != nullptr)
    {
        W.Key("default_exit");
        W.String(std::string_view(T->default_exit()->c_str(), T->default_exit()->size()));
    }
    if (T->context() != nullptr)
    {
        W.Key("context");
        Write_Tryll_NodeParams_ContextOverrides(W, T->context(), Omitted, Prefix + "context.");
    }
    if (T->adaptation() != nullptr)
    {
        W.Key("adaptation");
        Write_Tryll_NodeParams_ModelAdaptation(W, T->adaptation(), Omitted, Prefix + "adaptation.");
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_ToolCallParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::ToolCallParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.Key("context_size");
    W.Int64(T->context_size());
    if (T->system_prompt() != nullptr)
    {
        W.Key("system_prompt");
        W.String(std::string_view(T->system_prompt()->c_str(), T->system_prompt()->size()));
    }
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    if (T->tool_call_format() != nullptr)
    {
        W.Key("tool_call_format");
        W.String(std::string_view(T->tool_call_format()->c_str(), T->tool_call_format()->size()));
    }
    if (T->tools() != nullptr)
    {
        W.Key("tools");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->tools()->size(); ++i)
        {
            Write_Tryll_NodeParams_ToolDefinition(W, T->tools()->Get(i), Omitted, Prefix + "tools.");
        }
        W.EndArray();
    }
    W.Key("mode");
    switch (T->mode())
    {
    case ::Tryll::ToolCallMode_DetectOnly: W.String("DetectOnly"); break;
    case ::Tryll::ToolCallMode_CallOrAnswer: W.String("CallOrAnswer"); break;
    case ::Tryll::ToolCallMode_RequireCall: W.String("RequireCall"); break;
    default: W.Int64(static_cast<std::int64_t>(T->mode())); break;
    }
    W.Key("parallel_tool_calls");
    W.Bool(T->parallel_tool_calls());
    if (T->sampling() != nullptr)
    {
        W.Key("sampling");
        Write_Tryll_NodeParams_SamplingOverrides(W, T->sampling(), Omitted, Prefix + "sampling.");
    }
    if (T->tool_called_exit() != nullptr)
    {
        W.Key("tool_called_exit");
        W.String(std::string_view(T->tool_called_exit()->c_str(), T->tool_called_exit()->size()));
    }
    if (T->no_tool_called_exit() != nullptr)
    {
        W.Key("no_tool_called_exit");
        W.String(std::string_view(T->no_tool_called_exit()->c_str(), T->no_tool_called_exit()->size()));
    }
    W.Key("disposition");
    switch (T->disposition())
    {
    case ::Tryll::ToolCallDisposition_RouteOnly: W.String("RouteOnly"); break;
    case ::Tryll::ToolCallDisposition_Acknowledge: W.String("Acknowledge"); break;
    case ::Tryll::ToolCallDisposition_Notify: W.String("Notify"); break;
    case ::Tryll::ToolCallDisposition_NotifyAndAcknowledge: W.String("NotifyAndAcknowledge"); break;
    case ::Tryll::ToolCallDisposition_Pause: W.String("Pause"); break;
    case ::Tryll::ToolCallDisposition_PauseAndAcknowledge: W.String("PauseAndAcknowledge"); break;
    case ::Tryll::ToolCallDisposition_AwaitResult: W.String("AwaitResult"); break;
    default: W.Int64(static_cast<std::int64_t>(T->disposition())); break;
    }
    if (T->acknowledge_text() != nullptr)
    {
        W.Key("acknowledge_text");
        W.String(std::string_view(T->acknowledge_text()->c_str(), T->acknowledge_text()->size()));
    }
    W.Key("pause_timeout_ms");
    W.Int64(T->pause_timeout_ms());
    if (T->context() != nullptr)
    {
        W.Key("context");
        Write_Tryll_NodeParams_ContextOverrides(W, T->context(), Omitted, Prefix + "context.");
    }
    if (T->adaptation() != nullptr)
    {
        W.Key("adaptation");
        Write_Tryll_NodeParams_ModelAdaptation(W, T->adaptation(), Omitted, Prefix + "adaptation.");
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_ToolDefinition(FTryllJsonWriter& W, const ::Tryll::NodeParams::ToolDefinition* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->description() != nullptr)
    {
        W.Key("description");
        W.String(std::string_view(T->description()->c_str(), T->description()->size()));
    }
    if (T->parameters() != nullptr)
    {
        W.Key("parameters");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->parameters()->size(); ++i)
        {
            Write_Tryll_NodeParams_ToolParamDefinition(W, T->parameters()->Get(i), Omitted, Prefix + "parameters.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_ToolParamDefinition(FTryllJsonWriter& W, const ::Tryll::NodeParams::ToolParamDefinition* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->type() != nullptr)
    {
        W.Key("type");
        W.String(std::string_view(T->type()->c_str(), T->type()->size()));
    }
    if (T->description() != nullptr)
    {
        W.Key("description");
        W.String(std::string_view(T->description()->c_str(), T->description()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_TransformParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::TransformParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->template_() != nullptr)
    {
        W.Key("template");
        W.String(std::string_view(T->template_()->c_str(), T->template_()->size()));
    }
    if (T->output_name() != nullptr)
    {
        W.Key("output_name");
        W.String(std::string_view(T->output_name()->c_str(), T->output_name()->size()));
    }
    if (T->default_exit() != nullptr)
    {
        W.Key("default_exit");
        W.String(std::string_view(T->default_exit()->c_str(), T->default_exit()->size()));
    }
    W.EndObject();
}

void Write_Tryll_NodeParams_WriteToMemoryParams(FTryllJsonWriter& W, const ::Tryll::NodeParams::WriteToMemoryParams* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->storage_name() != nullptr)
    {
        W.Key("storage_name");
        W.String(std::string_view(T->storage_name()->c_str(), T->storage_name()->size()));
    }
    if (T->schema() != nullptr)
    {
        W.Key("schema");
        W.String(std::string_view(T->schema()->c_str(), T->schema()->size()));
    }
    if (T->input() != nullptr)
    {
        W.Key("input");
        W.String(std::string_view(T->input()->c_str(), T->input()->size()));
    }
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->written_exit() != nullptr)
    {
        W.Key("written_exit");
        W.String(std::string_view(T->written_exit()->c_str(), T->written_exit()->size()));
    }
    if (T->failed_exit() != nullptr)
    {
        W.Key("failed_exit");
        W.String(std::string_view(T->failed_exit()->c_str(), T->failed_exit()->size()));
    }
    W.EndObject();
}

void Write_Tryll_PersistedComponentEntry(FTryllJsonWriter& W, const ::Tryll::PersistedComponentEntry* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->value_type() != ::Tryll::PersistedComponent_NONE)
    {
        W.Key("value_type");
        switch (T->value_type())
        {
        case ::Tryll::PersistedComponent_PersistedSlot: W.String("PersistedSlot"); break;
        case ::Tryll::PersistedComponent_PersistedKnowledge: W.String("PersistedKnowledge"); break;
        case ::Tryll::PersistedComponent_PersistedIntention: W.String("PersistedIntention"); break;
        case ::Tryll::PersistedComponent_PersistedToolCall: W.String("PersistedToolCall"); break;
        case ::Tryll::PersistedComponent_PersistedToolResult: W.String("PersistedToolResult"); break;
        case ::Tryll::PersistedComponent_PersistedSummary: W.String("PersistedSummary"); break;
        default: W.Int64(static_cast<std::int64_t>(T->value_type())); break;
        }
        W.Key("value");
        switch (T->value_type())
        {
        case ::Tryll::PersistedComponent_PersistedSlot:
            Write_Tryll_PersistedSlot(W, T->value_as_PersistedSlot(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::PersistedComponent_PersistedKnowledge:
            Write_Tryll_PersistedKnowledge(W, T->value_as_PersistedKnowledge(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::PersistedComponent_PersistedIntention:
            Write_Tryll_PersistedIntention(W, T->value_as_PersistedIntention(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::PersistedComponent_PersistedToolCall:
            Write_Tryll_PersistedToolCall(W, T->value_as_PersistedToolCall(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::PersistedComponent_PersistedToolResult:
            Write_Tryll_PersistedToolResult(W, T->value_as_PersistedToolResult(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::PersistedComponent_PersistedSummary:
            Write_Tryll_PersistedSummary(W, T->value_as_PersistedSummary(), Omitted, Prefix + "value.");
            break;
        default: W.BeginObject(); W.EndObject(); break;
        }
    }
    W.EndObject();
}

void Write_Tryll_PersistedDialogSnapshot(FTryllJsonWriter& W, const ::Tryll::PersistedDialogSnapshot* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("format_version");
    W.UInt64(T->format_version());
    if (T->graph_signature() != nullptr)
    {
        W.Key("graph_signature");
        W.String(std::string_view(T->graph_signature()->c_str(), T->graph_signature()->size()));
    }
    const auto Path_interactions = (Prefix.empty() ? std::string("interactions") : Prefix + "interactions");
    const auto Found_interactions = Omitted.find(Path_interactions);
    if (T->interactions() != nullptr || Found_interactions != Omitted.end())
    {
        W.Key("interactions");
        std::int64_t Count_interactions = 0;
        std::int64_t Bytes_interactions = 0;
        if (Found_interactions != Omitted.end())
        {
            Count_interactions = Found_interactions->second.Count;
            Bytes_interactions = Found_interactions->second.Bytes;
        }
        else
        {
            Count_interactions = static_cast<std::int64_t>(T->interactions()->size());
            Bytes_interactions = static_cast<std::int64_t>(T->interactions()->size()) * 4;
        }
        W.BeginObject();
        W.Key("omitted"); W.Bool(true);
        W.Key("count"); W.Int64(Count_interactions);
        W.Key("bytes"); W.Int64(Bytes_interactions);
        W.EndObject();
    }
    W.Key("next_tool_call_seq");
    W.UInt64(T->next_tool_call_seq());
    W.Key("next_exchange_id");
    W.UInt64(T->next_exchange_id());
    const auto Path_projection_windows = (Prefix.empty() ? std::string("projection_windows") : Prefix + "projection_windows");
    const auto Found_projection_windows = Omitted.find(Path_projection_windows);
    if (T->projection_windows() != nullptr || Found_projection_windows != Omitted.end())
    {
        W.Key("projection_windows");
        std::int64_t Count_projection_windows = 0;
        std::int64_t Bytes_projection_windows = 0;
        if (Found_projection_windows != Omitted.end())
        {
            Count_projection_windows = Found_projection_windows->second.Count;
            Bytes_projection_windows = Found_projection_windows->second.Bytes;
        }
        else
        {
            Count_projection_windows = static_cast<std::int64_t>(T->projection_windows()->size());
            Bytes_projection_windows = static_cast<std::int64_t>(T->projection_windows()->size()) * 4;
        }
        W.BeginObject();
        W.Key("omitted"); W.Bool(true);
        W.Key("count"); W.Int64(Count_projection_windows);
        W.Key("bytes"); W.Int64(Bytes_projection_windows);
        W.EndObject();
    }
    if (T->save_id() != nullptr)
    {
        W.Key("save_id");
        W.String(std::string_view(T->save_id()->c_str(), T->save_id()->size()));
    }
    W.Key("saved_at_ms");
    W.UInt64(T->saved_at_ms());
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::PersistedDialogSnapshot> Pack_Tryll_PersistedDialogSnapshot(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::PersistedDialogSnapshot* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    ::flatbuffers::Offset<::flatbuffers::String> _graph_signature = T->graph_signature() ? Fbb.CreateString(T->graph_signature()->str()) : 0;
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<::Tryll::PersistedInteraction>>> _interactions;
    if (T->interactions() != nullptr)
    {
        const auto Path = (Prefix.empty() ? std::string("interactions") : Prefix + "interactions");
        Omitted.push_back(FTryllOmittedField{ Path, static_cast<std::int64_t>(T->interactions()->size()), static_cast<std::int64_t>(T->interactions()->size()) * 4 });
        _interactions = Fbb.CreateVector(static_cast<const ::flatbuffers::Offset<::Tryll::PersistedInteraction>*>(nullptr), 0);
    }
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<::Tryll::PersistedProjectionWindow>>> _projection_windows;
    if (T->projection_windows() != nullptr)
    {
        const auto Path = (Prefix.empty() ? std::string("projection_windows") : Prefix + "projection_windows");
        Omitted.push_back(FTryllOmittedField{ Path, static_cast<std::int64_t>(T->projection_windows()->size()), static_cast<std::int64_t>(T->projection_windows()->size()) * 4 });
        _projection_windows = Fbb.CreateVector(static_cast<const ::flatbuffers::Offset<::Tryll::PersistedProjectionWindow>*>(nullptr), 0);
    }
    ::flatbuffers::Offset<::flatbuffers::String> _save_id = T->save_id() ? Fbb.CreateString(T->save_id()->str()) : 0;
    return ::Tryll::CreatePersistedDialogSnapshot(Fbb, T->format_version(), _graph_signature, _interactions, T->next_tool_call_seq(), T->next_exchange_id(), _projection_windows, _save_id, T->saved_at_ms());
}

void Write_Tryll_PersistedIntention(FTryllJsonWriter& W, const ::Tryll::PersistedIntention* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->intent() != nullptr)
    {
        W.Key("intent");
        W.String(std::string_view(T->intent()->c_str(), T->intent()->size()));
    }
    W.EndObject();
}

void Write_Tryll_PersistedInteraction(FTryllJsonWriter& W, const ::Tryll::PersistedInteraction* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("interrupted");
    W.Bool(T->interrupted());
    if (T->components() != nullptr)
    {
        W.Key("components");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->components()->size(); ++i)
        {
            Write_Tryll_PersistedComponentEntry(W, T->components()->Get(i), Omitted, Prefix + "components.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_PersistedKbRecord(FTryllJsonWriter& W, const ::Tryll::PersistedKbRecord* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.Key("created_at_ms");
    W.UInt64(T->created_at_ms());
    W.Key("updated_at_ms");
    W.UInt64(T->updated_at_ms());
    W.EndObject();
}

void Write_Tryll_PersistedKnowledge(FTryllJsonWriter& W, const ::Tryll::PersistedKnowledge* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->source() != nullptr)
    {
        W.Key("source");
        W.String(std::string_view(T->source()->c_str(), T->source()->size()));
    }
    if (T->chunks() != nullptr)
    {
        W.Key("chunks");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->chunks()->size(); ++i)
        {
            Write_Tryll_PersistedKnowledgeChunk(W, T->chunks()->Get(i), Omitted, Prefix + "chunks.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_PersistedKnowledgeChunk(FTryllJsonWriter& W, const ::Tryll::PersistedKnowledgeChunk* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.Key("distance");
    W.Float(T->distance());
    W.Key("distance_is_nan");
    W.Bool(T->distance_is_nan());
    W.Key("bm25_score");
    if (const auto Value = T->bm25_score())
        W.Float((*Value));
    else W.Null();
    W.Key("rank_dense");
    if (const auto Value = T->rank_dense())
        W.UInt64((*Value));
    else W.Null();
    W.Key("rank_sparse");
    if (const auto Value = T->rank_sparse())
        W.UInt64((*Value));
    else W.Null();
    W.EndObject();
}

void Write_Tryll_PersistedMutableKb(FTryllJsonWriter& W, const ::Tryll::PersistedMutableKb* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->schema_name() != nullptr)
    {
        W.Key("schema_name");
        W.String(std::string_view(T->schema_name()->c_str(), T->schema_name()->size()));
    }
    if (T->schema_fingerprint() != nullptr)
    {
        W.Key("schema_fingerprint");
        W.String(std::string_view(T->schema_fingerprint()->c_str(), T->schema_fingerprint()->size()));
    }
    if (T->embedding_model() != nullptr)
    {
        W.Key("embedding_model");
        W.String(std::string_view(T->embedding_model()->c_str(), T->embedding_model()->size()));
    }
    W.Key("write_generation");
    W.UInt64(T->write_generation());
    W.Key("next_generated_id");
    W.UInt64(T->next_generated_id());
    const auto Path_records = (Prefix.empty() ? std::string("records") : Prefix + "records");
    const auto Found_records = Omitted.find(Path_records);
    if (T->records() != nullptr || Found_records != Omitted.end())
    {
        W.Key("records");
        std::int64_t Count_records = 0;
        std::int64_t Bytes_records = 0;
        if (Found_records != Omitted.end())
        {
            Count_records = Found_records->second.Count;
            Bytes_records = Found_records->second.Bytes;
        }
        else
        {
            Count_records = static_cast<std::int64_t>(T->records()->size());
            Bytes_records = static_cast<std::int64_t>(T->records()->size()) * 4;
        }
        W.BeginObject();
        W.Key("omitted"); W.Bool(true);
        W.Key("count"); W.Int64(Count_records);
        W.Key("bytes"); W.Int64(Bytes_records);
        W.EndObject();
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::PersistedMutableKb> Pack_Tryll_PersistedMutableKb(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::PersistedMutableKb* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    ::flatbuffers::Offset<::flatbuffers::String> _name = T->name() ? Fbb.CreateString(T->name()->str()) : 0;
    ::flatbuffers::Offset<::flatbuffers::String> _schema_name = T->schema_name() ? Fbb.CreateString(T->schema_name()->str()) : 0;
    ::flatbuffers::Offset<::flatbuffers::String> _schema_fingerprint = T->schema_fingerprint() ? Fbb.CreateString(T->schema_fingerprint()->str()) : 0;
    ::flatbuffers::Offset<::flatbuffers::String> _embedding_model = T->embedding_model() ? Fbb.CreateString(T->embedding_model()->str()) : 0;
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<::Tryll::PersistedKbRecord>>> _records;
    if (T->records() != nullptr)
    {
        const auto Path = (Prefix.empty() ? std::string("records") : Prefix + "records");
        Omitted.push_back(FTryllOmittedField{ Path, static_cast<std::int64_t>(T->records()->size()), static_cast<std::int64_t>(T->records()->size()) * 4 });
        _records = Fbb.CreateVector(static_cast<const ::flatbuffers::Offset<::Tryll::PersistedKbRecord>*>(nullptr), 0);
    }
    return ::Tryll::CreatePersistedMutableKb(Fbb, _name, _schema_name, _schema_fingerprint, _embedding_model, T->write_generation(), T->next_generated_id(), _records);
}

void Write_Tryll_PersistedProjectionWindow(FTryllJsonWriter& W, const ::Tryll::PersistedProjectionWindow* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->node_name() != nullptr)
    {
        W.Key("node_name");
        W.String(std::string_view(T->node_name()->c_str(), T->node_name()->size()));
    }
    W.Key("window_start");
    W.UInt64(T->window_start());
    W.EndObject();
}

void Write_Tryll_PersistedSlot(FTryllJsonWriter& W, const ::Tryll::PersistedSlot* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.Key("kind");
    switch (T->kind())
    {
    case ::Tryll::PersistedSlotKind_User: W.String("User"); break;
    case ::Tryll::PersistedSlotKind_Instruction: W.String("Instruction"); break;
    case ::Tryll::PersistedSlotKind_General: W.String("General"); break;
    default: W.Int64(static_cast<std::int64_t>(T->kind())); break;
    }
    W.Key("history_role");
    switch (T->history_role())
    {
    case ::Tryll::HistoryRole_None: W.String("None"); break;
    case ::Tryll::HistoryRole_Assistant: W.String("Assistant"); break;
    case ::Tryll::HistoryRole_User: W.String("User"); break;
    case ::Tryll::HistoryRole_System: W.String("System"); break;
    default: W.Int64(static_cast<std::int64_t>(T->history_role())); break;
    }
    if (T->producer() != nullptr)
    {
        W.Key("producer");
        W.String(std::string_view(T->producer()->c_str(), T->producer()->size()));
    }
    W.EndObject();
}

void Write_Tryll_PersistedStringPair(FTryllJsonWriter& W, const ::Tryll::PersistedStringPair* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->key() != nullptr)
    {
        W.Key("key");
        W.String(std::string_view(T->key()->c_str(), T->key()->size()));
    }
    if (T->value() != nullptr)
    {
        W.Key("value");
        W.String(std::string_view(T->value()->c_str(), T->value()->size()));
    }
    W.EndObject();
}

void Write_Tryll_PersistedSummary(FTryllJsonWriter& W, const ::Tryll::PersistedSummary* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.Key("covered_through");
    W.UInt64(T->covered_through());
    if (T->producer() != nullptr)
    {
        W.Key("producer");
        W.String(std::string_view(T->producer()->c_str(), T->producer()->size()));
    }
    W.EndObject();
}

void Write_Tryll_PersistedToolCall(FTryllJsonWriter& W, const ::Tryll::PersistedToolCall* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->tool_name() != nullptr)
    {
        W.Key("tool_name");
        W.String(std::string_view(T->tool_name()->c_str(), T->tool_name()->size()));
    }
    if (T->arguments() != nullptr)
    {
        W.Key("arguments");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->arguments()->size(); ++i)
        {
            Write_Tryll_PersistedStringPair(W, T->arguments()->Get(i), Omitted, Prefix + "arguments.");
        }
        W.EndArray();
    }
    if (T->arguments_json() != nullptr)
    {
        W.Key("arguments_json");
        W.String(std::string_view(T->arguments_json()->c_str(), T->arguments_json()->size()));
    }
    W.Key("dispatched");
    W.Bool(T->dispatched());
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    W.Key("disposition");
    switch (T->disposition())
    {
    case ::Tryll::ToolCallDisposition_RouteOnly: W.String("RouteOnly"); break;
    case ::Tryll::ToolCallDisposition_Acknowledge: W.String("Acknowledge"); break;
    case ::Tryll::ToolCallDisposition_Notify: W.String("Notify"); break;
    case ::Tryll::ToolCallDisposition_NotifyAndAcknowledge: W.String("NotifyAndAcknowledge"); break;
    case ::Tryll::ToolCallDisposition_Pause: W.String("Pause"); break;
    case ::Tryll::ToolCallDisposition_PauseAndAcknowledge: W.String("PauseAndAcknowledge"); break;
    case ::Tryll::ToolCallDisposition_AwaitResult: W.String("AwaitResult"); break;
    default: W.Int64(static_cast<std::int64_t>(T->disposition())); break;
    }
    if (T->model_id() != nullptr)
    {
        W.Key("model_id");
        W.String(std::string_view(T->model_id()->c_str(), T->model_id()->size()));
    }
    W.Key("exchange_id");
    W.UInt64(T->exchange_id());
    W.EndObject();
}

void Write_Tryll_PersistedToolResult(FTryllJsonWriter& W, const ::Tryll::PersistedToolResult* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->call_id() != nullptr)
    {
        W.Key("call_id");
        W.String(std::string_view(T->call_id()->c_str(), T->call_id()->size()));
    }
    if (T->tool_name() != nullptr)
    {
        W.Key("tool_name");
        W.String(std::string_view(T->tool_name()->c_str(), T->tool_name()->size()));
    }
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.Key("exchange_id");
    W.UInt64(T->exchange_id());
    W.EndObject();
}

void Write_Tryll_PrefillAgentKvCacheRequest(FTryllJsonWriter& W, const ::Tryll::PrefillAgentKvCacheRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.EndObject();
}

void Write_Tryll_PrefillAgentKvCacheResponse(FTryllJsonWriter& W, const ::Tryll::PrefillAgentKvCacheResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("applicable");
    W.Bool(T->applicable());
    W.Key("residency");
    switch (T->residency())
    {
    case ::Tryll::AgentKvCacheResidency_Evicted: W.String("Evicted"); break;
    case ::Tryll::AgentKvCacheResidency_Resident: W.String("Resident"); break;
    default: W.Int64(static_cast<std::int64_t>(T->residency())); break;
    }
    W.Key("prefix_status");
    switch (T->prefix_status())
    {
    case ::Tryll::AgentKvCachePrefixStatus_NotCurrent: W.String("NotCurrent"); break;
    case ::Tryll::AgentKvCachePrefixStatus_Current: W.String("Current"); break;
    default: W.Int64(static_cast<std::int64_t>(T->prefix_status())); break;
    }
    W.Key("eligible_node_count");
    W.UInt64(T->eligible_node_count());
    W.Key("contexts_created");
    W.UInt64(T->contexts_created());
    W.Key("tokens_decoded");
    W.UInt64(T->tokens_decoded());
    W.Key("tokens_reused");
    W.UInt64(T->tokens_reused());
    if (T->debug_info() != nullptr)
    {
        W.Key("debug_info");
        W.String(std::string_view(T->debug_info()->c_str(), T->debug_info()->size()));
    }
    W.EndObject();
}

void Write_Tryll_RemoveInteractionsFromEndRequest(FTryllJsonWriter& W, const ::Tryll::RemoveInteractionsFromEndRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("count");
    W.UInt64(T->count());
    W.EndObject();
}

void Write_Tryll_RemoveInteractionsFromEndResponse(FTryllJsonWriter& W, const ::Tryll::RemoveInteractionsFromEndResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("removed_count");
    W.UInt64(T->removed_count());
    W.EndObject();
}

void Write_Tryll_ResumeAgentRequest(FTryllJsonWriter& W, const ::Tryll::ResumeAgentRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->resume_node() != nullptr)
    {
        W.Key("resume_node");
        W.String(std::string_view(T->resume_node()->c_str(), T->resume_node()->size()));
    }
    if (T->tool_results() != nullptr)
    {
        W.Key("tool_results");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->tool_results()->size(); ++i)
        {
            Write_Tryll_ToolResult(W, T->tool_results()->Get(i), Omitted, Prefix + "tool_results.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_SaveDialogRequest(FTryllJsonWriter& W, const ::Tryll::SaveDialogRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->path() != nullptr)
    {
        W.Key("path");
        W.String(std::string_view(T->path()->c_str(), T->path()->size()));
    }
    if (T->save_id() != nullptr)
    {
        W.Key("save_id");
        W.String(std::string_view(T->save_id()->c_str(), T->save_id()->size()));
    }
    W.EndObject();
}

void Write_Tryll_SaveDialogResponse(FTryllJsonWriter& W, const ::Tryll::SaveDialogResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("saved_at_ms");
    W.UInt64(T->saved_at_ms());
    W.EndObject();
}

void Write_Tryll_SaveGameMemoryRequest(FTryllJsonWriter& W, const ::Tryll::SaveGameMemoryRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->path() != nullptr)
    {
        W.Key("path");
        W.String(std::string_view(T->path()->c_str(), T->path()->size()));
    }
    if (T->save_id() != nullptr)
    {
        W.Key("save_id");
        W.String(std::string_view(T->save_id()->c_str(), T->save_id()->size()));
    }
    W.EndObject();
}

void Write_Tryll_SaveGameMemoryResponse(FTryllJsonWriter& W, const ::Tryll::SaveGameMemoryResponse* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("saved_at_ms");
    W.UInt64(T->saved_at_ms());
    W.Key("store_count");
    W.UInt64(T->store_count());
    W.EndObject();
}

void Write_Tryll_SendMessageRequest(FTryllJsonWriter& W, const ::Tryll::SendMessageRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.EndObject();
}

std::string Label_Tryll_SendMessageRequest(const ::Tryll::SendMessageRequest* T)
{
    if (T == nullptr) return {};
    return TryllFormatLogLabel(
        "{text}",
        {
            { "text", (T->text() ? T->text()->str() : std::string()) },
        });
}

void Write_Tryll_SessionModelConfig(FTryllJsonWriter& W, const ::Tryll::SessionModelConfig* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->engines() != nullptr)
    {
        W.Key("engines");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->engines()->size(); ++i)
        {
            Write_Tryll_EngineSelection(W, T->engines()->Get(i), Omitted, Prefix + "engines.");
        }
        W.EndArray();
    }
    if (T->placement_hints() != nullptr)
    {
        W.Key("placement_hints");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->placement_hints()->size(); ++i)
        {
            Write_Tryll_ModelPlacementHint(W, T->placement_hints()->Get(i), Omitted, Prefix + "placement_hints.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_SetAgentDeliveryModeRequest(FTryllJsonWriter& W, const ::Tryll::SetAgentDeliveryModeRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->mode() != nullptr)
    {
        W.Key("mode");
        W.String(std::string_view(T->mode()->c_str(), T->mode()->size()));
    }
    W.Key("intensity");
    W.Float(T->intensity());
    W.EndObject();
}

void Write_Tryll_SetAgentVoiceRequest(FTryllJsonWriter& W, const ::Tryll::SetAgentVoiceRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->voice() != nullptr)
    {
        W.Key("voice");
        Write_Tryll_VoiceProfile(W, T->voice(), Omitted, Prefix + "voice.");
    }
    W.EndObject();
}

void Write_Tryll_SetInferenceThrottle(FTryllJsonWriter& W, const ::Tryll::SetInferenceThrottle* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("throttle");
    W.Float(T->throttle());
    W.EndObject();
}

void Write_Tryll_SetSessionLocaleRequest(FTryllJsonWriter& W, const ::Tryll::SetSessionLocaleRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->locale() != nullptr)
    {
        W.Key("locale");
        W.String(std::string_view(T->locale()->c_str(), T->locale()->size()));
    }
    W.EndObject();
}

void Write_Tryll_ToolResult(FTryllJsonWriter& W, const ::Tryll::ToolResult* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->call_id() != nullptr)
    {
        W.Key("call_id");
        W.String(std::string_view(T->call_id()->c_str(), T->call_id()->size()));
    }
    if (T->result() != nullptr)
    {
        W.Key("result");
        W.String(std::string_view(T->result()->c_str(), T->result()->size()));
    }
    W.EndObject();
}

void Write_Tryll_TtsAudioFormatFrame(FTryllJsonWriter& W, const ::Tryll::TtsAudioFormatFrame* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->format() != nullptr)
    {
        W.Key("format");
        Write_Tryll_WireAudioFormat(W, T->format(), Omitted, Prefix + "format.");
    }
    if (T->node_name() != nullptr)
    {
        W.Key("node_name");
        W.String(std::string_view(T->node_name()->c_str(), T->node_name()->size()));
    }
    W.EndObject();
}

void Write_Tryll_TtsAudioFrame(FTryllJsonWriter& W, const ::Tryll::TtsAudioFrame* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    const auto Path_pcm = (Prefix.empty() ? std::string("pcm") : Prefix + "pcm");
    const auto Found_pcm = Omitted.find(Path_pcm);
    if (T->pcm() != nullptr || Found_pcm != Omitted.end())
    {
        W.Key("pcm");
        std::int64_t Count_pcm = 0;
        std::int64_t Bytes_pcm = 0;
        if (Found_pcm != Omitted.end())
        {
            Count_pcm = Found_pcm->second.Count;
            Bytes_pcm = Found_pcm->second.Bytes;
        }
        else
        {
            Count_pcm = static_cast<std::int64_t>(T->pcm()->size());
            Bytes_pcm = static_cast<std::int64_t>(T->pcm()->size()) * 1;
        }
        W.BeginObject();
        W.Key("omitted"); W.Bool(true);
        W.Key("count"); W.Int64(Count_pcm);
        W.Key("bytes"); W.Int64(Bytes_pcm);
        W.EndObject();
    }
    if (T->node_name() != nullptr)
    {
        W.Key("node_name");
        W.String(std::string_view(T->node_name()->c_str(), T->node_name()->size()));
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::TtsAudioFrame> Pack_Tryll_TtsAudioFrame(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::TtsAudioFrame* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    ::flatbuffers::Offset<::flatbuffers::Vector<uint8_t>> _pcm;
    if (T->pcm() != nullptr)
    {
        const auto Path = (Prefix.empty() ? std::string("pcm") : Prefix + "pcm");
        Omitted.push_back(FTryllOmittedField{ Path, static_cast<std::int64_t>(T->pcm()->size()), static_cast<std::int64_t>(T->pcm()->size()) * 1 });
        _pcm = Fbb.CreateVector(static_cast<const uint8_t*>(nullptr), 0);
    }
    ::flatbuffers::Offset<::flatbuffers::String> _node_name = T->node_name() ? Fbb.CreateString(T->node_name()->str()) : 0;
    return ::Tryll::CreateTtsAudioFrame(Fbb, T->request_id(), T->agent_id(), _pcm, _node_name);
}

std::string Label_Tryll_TtsAudioFrame(const ::Tryll::TtsAudioFrame* T)
{
    if (T == nullptr) return {};
    return TryllFormatLogLabel(
        "{node_name}",
        {
            { "node_name", (T->node_name() ? T->node_name()->str() : std::string()) },
        });
}

void Write_Tryll_TtsCapabilitiesInfo(FTryllJsonWriter& W, const ::Tryll::TtsCapabilitiesInfo* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("voice_description");
    W.Bool(T->voice_description());
    W.Key("delivery_instruction");
    W.Bool(T->delivery_instruction());
    W.Key("reference_voice");
    W.Bool(T->reference_voice());
    W.Key("speed");
    W.Bool(T->speed());
    W.Key("language_per_call");
    W.Bool(T->language_per_call());
    if (T->voice_presets() != nullptr)
    {
        W.Key("voice_presets");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->voice_presets()->size(); ++i)
        {
            Write_Tryll_TtsVoicePresetInfo(W, T->voice_presets()->Get(i), Omitted, Prefix + "voice_presets.");
        }
        W.EndArray();
    }
    if (T->inline_tags() != nullptr)
    {
        W.Key("inline_tags");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->inline_tags()->size(); ++i)
        {
            Write_Tryll_TtsInlineTagInfo(W, T->inline_tags()->Get(i), Omitted, Prefix + "inline_tags.");
        }
        W.EndArray();
    }
    W.Key("expressiveness");
    W.Bool(T->expressiveness());
    W.Key("num_speakers");
    W.Int64(T->num_speakers());
    W.EndObject();
}

void Write_Tryll_TtsInlineTagInfo(FTryllJsonWriter& W, const ::Tryll::TtsInlineTagInfo* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->token() != nullptr)
    {
        W.Key("token");
        W.String(std::string_view(T->token()->c_str(), T->token()->size()));
    }
    if (T->kind() != nullptr)
    {
        W.Key("kind");
        W.String(std::string_view(T->kind()->c_str(), T->kind()->size()));
    }
    if (T->verification() != nullptr)
    {
        W.Key("verification");
        W.String(std::string_view(T->verification()->c_str(), T->verification()->size()));
    }
    W.EndObject();
}

void Write_Tryll_TtsVisemeFrame(FTryllJsonWriter& W, const ::Tryll::TtsVisemeFrame* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->node_name() != nullptr)
    {
        W.Key("node_name");
        W.String(std::string_view(T->node_name()->c_str(), T->node_name()->size()));
    }
    if (T->events() != nullptr)
    {
        W.Key("events");
        Write_Tryll_VisemeEventList(W, T->events(), Omitted, Prefix + "events.");
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::TtsVisemeFrame> Pack_Tryll_TtsVisemeFrame(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::TtsVisemeFrame* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    ::flatbuffers::Offset<::flatbuffers::String> _node_name = T->node_name() ? Fbb.CreateString(T->node_name()->str()) : 0;
    auto _events = T->events() ? Pack_Tryll_VisemeEventList(Fbb, T->events(), Omitted, Prefix + "events.") : 0;
    return ::Tryll::CreateTtsVisemeFrame(Fbb, T->request_id(), T->agent_id(), _node_name, _events);
}

std::string Label_Tryll_TtsVisemeFrame(const ::Tryll::TtsVisemeFrame* T)
{
    if (T == nullptr) return {};
    return TryllFormatLogLabel(
        "{node_name}",
        {
            { "node_name", (T->node_name() ? T->node_name()->str() : std::string()) },
        });
}

void Write_Tryll_TtsVoicePresetInfo(FTryllJsonWriter& W, const ::Tryll::TtsVoicePresetInfo* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->id() != nullptr)
    {
        W.Key("id");
        W.String(std::string_view(T->id()->c_str(), T->id()->size()));
    }
    if (T->label() != nullptr)
    {
        W.Key("label");
        W.String(std::string_view(T->label()->c_str(), T->label()->size()));
    }
    if (T->description() != nullptr)
    {
        W.Key("description");
        W.String(std::string_view(T->description()->c_str(), T->description()->size()));
    }
    W.EndObject();
}

void Write_Tryll_TurnComplete(FTryllJsonWriter& W, const ::Tryll::TurnComplete* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    W.Key("status");
    switch (T->status())
    {
    case ::Tryll::TurnStatus_Success: W.String("Success"); break;
    case ::Tryll::TurnStatus_Error: W.String("Error"); break;
    case ::Tryll::TurnStatus_Cancelled: W.String("Cancelled"); break;
    case ::Tryll::TurnStatus_PauseTimedOut: W.String("PauseTimedOut"); break;
    default: W.Int64(static_cast<std::int64_t>(T->status())); break;
    }
    if (T->debug_info() != nullptr)
    {
        W.Key("debug_info");
        std::string Reason;
        if (FTryllJsonWriter::TryParseJson(std::string_view(T->debug_info()->c_str(), T->debug_info()->size()), Reason)) W.Raw(std::string_view(T->debug_info()->c_str(), T->debug_info()->size()));
        else
        {
            W.String(std::string_view(T->debug_info()->c_str(), T->debug_info()->size()));
            W.Key("debug_info__parse_error");
            W.String(Reason);
        }
    }
    W.Key("tokens_generated");
    W.UInt64(T->tokens_generated());
    W.EndObject();
}

void Write_Tryll_UnloadModelRequest(FTryllJsonWriter& W, const ::Tryll::UnloadModelRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    if (T->model_name() != nullptr)
    {
        W.Key("model_name");
        W.String(std::string_view(T->model_name()->c_str(), T->model_name()->size()));
    }
    W.EndObject();
}

void Write_Tryll_UpdateAgentVariablesRequest(FTryllJsonWriter& W, const ::Tryll::UpdateAgentVariablesRequest* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("request_id");
    W.UInt64(T->request_id());
    W.Key("agent_id");
    W.UInt64(T->agent_id());
    if (T->updates() != nullptr)
    {
        W.Key("updates");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->updates()->size(); ++i)
        {
            Write_Tryll_VariableUpdate(W, T->updates()->Get(i), Omitted, Prefix + "updates.");
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_VarBool(FTryllJsonWriter& W, const ::Tryll::VarBool* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("v");
    W.Bool(T->v());
    W.EndObject();
}

void Write_Tryll_VarFloat(FTryllJsonWriter& W, const ::Tryll::VarFloat* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("v");
    W.Double(T->v());
    W.EndObject();
}

void Write_Tryll_VarInt(FTryllJsonWriter& W, const ::Tryll::VarInt* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("v");
    W.Int64(T->v());
    W.EndObject();
}

void Write_Tryll_VarString(FTryllJsonWriter& W, const ::Tryll::VarString* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->v() != nullptr)
    {
        W.Key("v");
        W.String(std::string_view(T->v()->c_str(), T->v()->size()));
    }
    W.EndObject();
}

void Write_Tryll_VarStringSet(FTryllJsonWriter& W, const ::Tryll::VarStringSet* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->v() != nullptr)
    {
        W.Key("v");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->v()->size(); ++i)
        {
            W.String(std::string_view(T->v()->Get(i)->c_str(), T->v()->Get(i)->size()));
        }
        W.EndArray();
    }
    W.EndObject();
}

void Write_Tryll_VariableUpdate(FTryllJsonWriter& W, const ::Tryll::VariableUpdate* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->name() != nullptr)
    {
        W.Key("name");
        W.String(std::string_view(T->name()->c_str(), T->name()->size()));
    }
    W.Key("op");
    switch (T->op())
    {
    case ::Tryll::VariableOp_Assign: W.String("Assign"); break;
    case ::Tryll::VariableOp_AddToSet: W.String("AddToSet"); break;
    case ::Tryll::VariableOp_RemoveFromSet: W.String("RemoveFromSet"); break;
    case ::Tryll::VariableOp_Reset: W.String("Reset"); break;
    default: W.Int64(static_cast<std::int64_t>(T->op())); break;
    }
    if (T->value_type() != ::Tryll::VariableValue_NONE)
    {
        W.Key("value_type");
        switch (T->value_type())
        {
        case ::Tryll::VariableValue_VarInt: W.String("VarInt"); break;
        case ::Tryll::VariableValue_VarFloat: W.String("VarFloat"); break;
        case ::Tryll::VariableValue_VarString: W.String("VarString"); break;
        case ::Tryll::VariableValue_VarBool: W.String("VarBool"); break;
        case ::Tryll::VariableValue_VarStringSet: W.String("VarStringSet"); break;
        default: W.Int64(static_cast<std::int64_t>(T->value_type())); break;
        }
        W.Key("value");
        switch (T->value_type())
        {
        case ::Tryll::VariableValue_VarInt:
            Write_Tryll_VarInt(W, T->value_as_VarInt(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::VariableValue_VarFloat:
            Write_Tryll_VarFloat(W, T->value_as_VarFloat(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::VariableValue_VarString:
            Write_Tryll_VarString(W, T->value_as_VarString(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::VariableValue_VarBool:
            Write_Tryll_VarBool(W, T->value_as_VarBool(), Omitted, Prefix + "value.");
            break;
        case ::Tryll::VariableValue_VarStringSet:
            Write_Tryll_VarStringSet(W, T->value_as_VarStringSet(), Omitted, Prefix + "value.");
            break;
        default: W.BeginObject(); W.EndObject(); break;
        }
    }
    W.EndObject();
}

void Write_Tryll_VisemeEvent(FTryllJsonWriter& W, const ::Tryll::VisemeEvent* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("sample_offset");
    W.UInt64(T->sample_offset());
    W.Key("viseme");
    switch (T->viseme())
    {
    case ::Tryll::WireViseme_sil: W.String("sil"); break;
    case ::Tryll::WireViseme_PP: W.String("PP"); break;
    case ::Tryll::WireViseme_FF: W.String("FF"); break;
    case ::Tryll::WireViseme_TH: W.String("TH"); break;
    case ::Tryll::WireViseme_DD: W.String("DD"); break;
    case ::Tryll::WireViseme_kk: W.String("kk"); break;
    case ::Tryll::WireViseme_CH: W.String("CH"); break;
    case ::Tryll::WireViseme_SS: W.String("SS"); break;
    case ::Tryll::WireViseme_nn: W.String("nn"); break;
    case ::Tryll::WireViseme_RR: W.String("RR"); break;
    case ::Tryll::WireViseme_aa: W.String("aa"); break;
    case ::Tryll::WireViseme_E: W.String("E"); break;
    case ::Tryll::WireViseme_ih: W.String("ih"); break;
    case ::Tryll::WireViseme_oh: W.String("oh"); break;
    case ::Tryll::WireViseme_ou: W.String("ou"); break;
    default: W.Int64(static_cast<std::int64_t>(T->viseme())); break;
    }
    W.Key("weight");
    W.Float(T->weight());
    W.EndObject();
}

void Write_Tryll_VisemeEventList(FTryllJsonWriter& W, const ::Tryll::VisemeEventList* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    const auto Path_events = (Prefix.empty() ? std::string("events") : Prefix + "events");
    const auto Found_events = Omitted.find(Path_events);
    if (T->events() != nullptr || Found_events != Omitted.end())
    {
        W.Key("events");
        std::int64_t Count_events = 0;
        std::int64_t Bytes_events = 0;
        if (Found_events != Omitted.end())
        {
            Count_events = Found_events->second.Count;
            Bytes_events = Found_events->second.Bytes;
        }
        else
        {
            Count_events = static_cast<std::int64_t>(T->events()->size());
            Bytes_events = static_cast<std::int64_t>(T->events()->size()) * 4;
        }
        W.BeginObject();
        W.Key("omitted"); W.Bool(true);
        W.Key("count"); W.Int64(Count_events);
        W.Key("bytes"); W.Int64(Bytes_events);
        W.EndObject();
    }
    W.EndObject();
}

::flatbuffers::Offset<::Tryll::VisemeEventList> Pack_Tryll_VisemeEventList(
    flatbuffers::FlatBufferBuilder& Fbb, const ::Tryll::VisemeEventList* T,
    std::vector<FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) return 0;
    ::flatbuffers::Offset<::flatbuffers::Vector<::flatbuffers::Offset<::Tryll::VisemeEvent>>> _events;
    if (T->events() != nullptr)
    {
        const auto Path = (Prefix.empty() ? std::string("events") : Prefix + "events");
        Omitted.push_back(FTryllOmittedField{ Path, static_cast<std::int64_t>(T->events()->size()), static_cast<std::int64_t>(T->events()->size()) * 4 });
        _events = Fbb.CreateVector(static_cast<const ::flatbuffers::Offset<::Tryll::VisemeEvent>*>(nullptr), 0);
    }
    return ::Tryll::CreateVisemeEventList(Fbb, _events);
}

void Write_Tryll_VoiceProfile(FTryllJsonWriter& W, const ::Tryll::VoiceProfile* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    if (T->tts_model_name() != nullptr)
    {
        W.Key("tts_model_name");
        W.String(std::string_view(T->tts_model_name()->c_str(), T->tts_model_name()->size()));
    }
    W.Key("speaker_id");
    W.Int64(T->speaker_id());
    if (T->reference_voice() != nullptr)
    {
        W.Key("reference_voice");
        W.String(std::string_view(T->reference_voice()->c_str(), T->reference_voice()->size()));
    }
    if (T->voice_description() != nullptr)
    {
        W.Key("voice_description");
        W.String(std::string_view(T->voice_description()->c_str(), T->voice_description()->size()));
    }
    W.Key("speed");
    W.Float(T->speed());
    if (T->lang() != nullptr)
    {
        W.Key("lang");
        W.String(std::string_view(T->lang()->c_str(), T->lang()->size()));
    }
    W.Key("expressiveness");
    W.Float(T->expressiveness());
    W.Key("prepare");
    switch (T->prepare())
    {
    case ::Tryll::VoicePreparation_Eager: W.String("Eager"); break;
    case ::Tryll::VoicePreparation_Lazy: W.String("Lazy"); break;
    default: W.Int64(static_cast<std::int64_t>(T->prepare())); break;
    }
    W.Key("emit_visemes");
    W.Bool(T->emit_visemes());
    if (T->delivery_modes() != nullptr)
    {
        W.Key("delivery_modes");
        W.BeginArray();
        for (flatbuffers::uoffset_t i = 0; i < T->delivery_modes()->size(); ++i)
        {
            Write_Tryll_DeliveryMode(W, T->delivery_modes()->Get(i), Omitted, Prefix + "delivery_modes.");
        }
        W.EndArray();
    }
    if (T->default_delivery_mode() != nullptr)
    {
        W.Key("default_delivery_mode");
        W.String(std::string_view(T->default_delivery_mode()->c_str(), T->default_delivery_mode()->size()));
    }
    W.EndObject();
}

void Write_Tryll_WireAudioFormat(FTryllJsonWriter& W, const ::Tryll::WireAudioFormat* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("sample_rate");
    W.UInt64(T->sample_rate());
    W.Key("channels");
    W.Int64(T->channels());
    W.Key("bits_per_sample");
    W.Int64(T->bits_per_sample());
    W.EndObject();
}

void Write_Tryll_WireTranscriptUpdate(FTryllJsonWriter& W, const ::Tryll::WireTranscriptUpdate* T,
    const std::map<std::string, FTryllOmittedField>& Omitted, const std::string& Prefix)
{
    if (T == nullptr) { W.BeginObject(); W.EndObject(); return; }
    W.BeginObject();
    W.Key("voice_input_id");
    W.UInt64(T->voice_input_id());
    if (T->text() != nullptr)
    {
        W.Key("text");
        W.String(std::string_view(T->text()->c_str(), T->text()->size()));
    }
    W.Key("kind");
    switch (T->kind())
    {
    case ::Tryll::TranscriptUpdateKind_SpeechStart: W.String("SpeechStart"); break;
    case ::Tryll::TranscriptUpdateKind_Partial: W.String("Partial"); break;
    case ::Tryll::TranscriptUpdateKind_SegmentFinal: W.String("SegmentFinal"); break;
    case ::Tryll::TranscriptUpdateKind_UtteranceFinal: W.String("UtteranceFinal"); break;
    default: W.Int64(static_cast<std::int64_t>(T->kind())); break;
    }
    W.Key("audio_ms_consumed");
    W.UInt64(T->audio_ms_consumed());
    W.EndObject();
}

} // namespace

void WriteMessage(FTryllJsonWriter& Writer, const std::uint8_t* Data, std::size_t Size,
    const std::vector<FTryllOmittedField>& Omitted)
{
    auto Map = Index(Omitted);
    auto* Root = ::Tryll::GetMessage(Data);
    const auto* Bytes = Data;
    (void)Size;
    (void)Bytes;
    switch (Root->body_type())
    {
    case ::Tryll::MessageBody_ConnectionReady:
        Write_Tryll_ConnectionReady(Writer, Root->body_as_ConnectionReady(), Map, "");
        break;
    case ::Tryll::MessageBody_CreateSessionRequest:
        Write_Tryll_CreateSessionRequest(Writer, Root->body_as_CreateSessionRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_CreateSessionResponse:
        Write_Tryll_CreateSessionResponse(Writer, Root->body_as_CreateSessionResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_CreateAgentRequest:
        Write_Tryll_CreateAgentRequest(Writer, Root->body_as_CreateAgentRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_CreateAgentResponse:
        Write_Tryll_CreateAgentResponse(Writer, Root->body_as_CreateAgentResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_SendMessageRequest:
        Write_Tryll_SendMessageRequest(Writer, Root->body_as_SendMessageRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_DestroyAgentRequest:
        Write_Tryll_DestroyAgentRequest(Writer, Root->body_as_DestroyAgentRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_AnswerText:
        Write_Tryll_AnswerText(Writer, Root->body_as_AnswerText(), Map, "");
        break;
    case ::Tryll::MessageBody_TurnComplete:
        Write_Tryll_TurnComplete(Writer, Root->body_as_TurnComplete(), Map, "");
        break;
    case ::Tryll::MessageBody_ErrorResponse:
        Write_Tryll_ErrorResponse(Writer, Root->body_as_ErrorResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_Ack:
        Write_Tryll_Ack(Writer, Root->body_as_Ack(), Map, "");
        break;
    case ::Tryll::MessageBody_ListModelsRequest:
        Write_Tryll_ListModelsRequest(Writer, Root->body_as_ListModelsRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_ListModelsResponse:
        Write_Tryll_ListModelsResponse(Writer, Root->body_as_ListModelsResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_DownloadModelRequest:
        Write_Tryll_DownloadModelRequest(Writer, Root->body_as_DownloadModelRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_DownloadProgress:
        Write_Tryll_DownloadProgress(Writer, Root->body_as_DownloadProgress(), Map, "");
        break;
    case ::Tryll::MessageBody_DownloadComplete:
        Write_Tryll_DownloadComplete(Writer, Root->body_as_DownloadComplete(), Map, "");
        break;
    case ::Tryll::MessageBody_CreateStringStorageRequest:
        Write_Tryll_CreateStringStorageRequest(Writer, Root->body_as_CreateStringStorageRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_CreateStringStorageResponse:
        Write_Tryll_CreateStringStorageResponse(Writer, Root->body_as_CreateStringStorageResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_DestroyStringStorageRequest:
        Write_Tryll_DestroyStringStorageRequest(Writer, Root->body_as_DestroyStringStorageRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_NodeEvent:
        Write_Tryll_NodeEvent(Writer, Root->body_as_NodeEvent(), Map, "");
        break;
    case ::Tryll::MessageBody_LoadModelRequest:
        Write_Tryll_LoadModelRequest(Writer, Root->body_as_LoadModelRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_LoadModelResponse:
        Write_Tryll_LoadModelResponse(Writer, Root->body_as_LoadModelResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_UnloadModelRequest:
        Write_Tryll_UnloadModelRequest(Writer, Root->body_as_UnloadModelRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest:
        Write_Tryll_CreateEmbeddedStringStorageRequest(Writer, Root->body_as_CreateEmbeddedStringStorageRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageResponse:
        Write_Tryll_CreateEmbeddedStringStorageResponse(Writer, Root->body_as_CreateEmbeddedStringStorageResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_DestroyEmbeddedStringStorageRequest:
        Write_Tryll_DestroyEmbeddedStringStorageRequest(Writer, Root->body_as_DestroyEmbeddedStringStorageRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_ChangeAgentParamRequest:
        Write_Tryll_ChangeAgentParamRequest(Writer, Root->body_as_ChangeAgentParamRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_CreateVoiceInputRequest:
        Write_Tryll_CreateVoiceInputRequest(Writer, Root->body_as_CreateVoiceInputRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_CreateVoiceInputResponse:
        Write_Tryll_CreateVoiceInputResponse(Writer, Root->body_as_CreateVoiceInputResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_BeginUtteranceRequest:
        Write_Tryll_BeginUtteranceRequest(Writer, Root->body_as_BeginUtteranceRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_AudioBuffer:
        Write_Tryll_AudioBuffer(Writer, Root->body_as_AudioBuffer(), Map, "");
        break;
    case ::Tryll::MessageBody_EndUtteranceRequest:
        Write_Tryll_EndUtteranceRequest(Writer, Root->body_as_EndUtteranceRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_CancelUtteranceRequest:
        Write_Tryll_CancelUtteranceRequest(Writer, Root->body_as_CancelUtteranceRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_WireTranscriptUpdate:
        Write_Tryll_WireTranscriptUpdate(Writer, Root->body_as_WireTranscriptUpdate(), Map, "");
        break;
    case ::Tryll::MessageBody_DestroyVoiceInputRequest:
        Write_Tryll_DestroyVoiceInputRequest(Writer, Root->body_as_DestroyVoiceInputRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_TtsAudioFormatFrame:
        Write_Tryll_TtsAudioFormatFrame(Writer, Root->body_as_TtsAudioFormatFrame(), Map, "");
        break;
    case ::Tryll::MessageBody_TtsAudioFrame:
        Write_Tryll_TtsAudioFrame(Writer, Root->body_as_TtsAudioFrame(), Map, "");
        break;
    case ::Tryll::MessageBody_DeleteModelRequest:
        Write_Tryll_DeleteModelRequest(Writer, Root->body_as_DeleteModelRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_DebugCommandRequest:
        Write_Tryll_DebugCommandRequest(Writer, Root->body_as_DebugCommandRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_DebugCommandResponse:
        Write_Tryll_DebugCommandResponse(Writer, Root->body_as_DebugCommandResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_CancelRequest:
        Write_Tryll_CancelRequest(Writer, Root->body_as_CancelRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_ResumeAgentRequest:
        Write_Tryll_ResumeAgentRequest(Writer, Root->body_as_ResumeAgentRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_UpdateAgentVariablesRequest:
        Write_Tryll_UpdateAgentVariablesRequest(Writer, Root->body_as_UpdateAgentVariablesRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_GetAgentVariablesRequest:
        Write_Tryll_GetAgentVariablesRequest(Writer, Root->body_as_GetAgentVariablesRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_GetAgentVariablesResponse:
        Write_Tryll_GetAgentVariablesResponse(Writer, Root->body_as_GetAgentVariablesResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_PrefillAgentKvCacheRequest:
        Write_Tryll_PrefillAgentKvCacheRequest(Writer, Root->body_as_PrefillAgentKvCacheRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_PrefillAgentKvCacheResponse:
        Write_Tryll_PrefillAgentKvCacheResponse(Writer, Root->body_as_PrefillAgentKvCacheResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_EvictAgentKvCacheRequest:
        Write_Tryll_EvictAgentKvCacheRequest(Writer, Root->body_as_EvictAgentKvCacheRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_EvictAgentKvCacheResponse:
        Write_Tryll_EvictAgentKvCacheResponse(Writer, Root->body_as_EvictAgentKvCacheResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_GetAgentKvCacheStatusRequest:
        Write_Tryll_GetAgentKvCacheStatusRequest(Writer, Root->body_as_GetAgentKvCacheStatusRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_GetAgentKvCacheStatusResponse:
        Write_Tryll_GetAgentKvCacheStatusResponse(Writer, Root->body_as_GetAgentKvCacheStatusResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_AppendInteractionsRequest:
        Write_Tryll_AppendInteractionsRequest(Writer, Root->body_as_AppendInteractionsRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_AppendInteractionsResponse:
        Write_Tryll_AppendInteractionsResponse(Writer, Root->body_as_AppendInteractionsResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_RemoveInteractionsFromEndRequest:
        Write_Tryll_RemoveInteractionsFromEndRequest(Writer, Root->body_as_RemoveInteractionsFromEndRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_RemoveInteractionsFromEndResponse:
        Write_Tryll_RemoveInteractionsFromEndResponse(Writer, Root->body_as_RemoveInteractionsFromEndResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_SetInferenceThrottle:
        Write_Tryll_SetInferenceThrottle(Writer, Root->body_as_SetInferenceThrottle(), Map, "");
        break;
    case ::Tryll::MessageBody_SetSessionLocaleRequest:
        Write_Tryll_SetSessionLocaleRequest(Writer, Root->body_as_SetSessionLocaleRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_ExportDialogRequest:
        Write_Tryll_ExportDialogRequest(Writer, Root->body_as_ExportDialogRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_ExportDialogResponse:
        Write_Tryll_ExportDialogResponse(Writer, Root->body_as_ExportDialogResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_ImportDialogRequest:
        Write_Tryll_ImportDialogRequest(Writer, Root->body_as_ImportDialogRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_ImportDialogResponse:
        Write_Tryll_ImportDialogResponse(Writer, Root->body_as_ImportDialogResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_GetMemoryConsumptionRequest:
        Write_Tryll_GetMemoryConsumptionRequest(Writer, Root->body_as_GetMemoryConsumptionRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_GetMemoryConsumptionResponse:
        Write_Tryll_GetMemoryConsumptionResponse(Writer, Root->body_as_GetMemoryConsumptionResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_EnsureMutableKbRequest:
        Write_Tryll_EnsureMutableKbRequest(Writer, Root->body_as_EnsureMutableKbRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_EnsureMutableKbResponse:
        Write_Tryll_EnsureMutableKbResponse(Writer, Root->body_as_EnsureMutableKbResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_EnumerateMutableKbsRequest:
        Write_Tryll_EnumerateMutableKbsRequest(Writer, Root->body_as_EnumerateMutableKbsRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_EnumerateMutableKbsResponse:
        Write_Tryll_EnumerateMutableKbsResponse(Writer, Root->body_as_EnumerateMutableKbsResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbWriteRequest:
        Write_Tryll_MutableKbWriteRequest(Writer, Root->body_as_MutableKbWriteRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbWriteResponse:
        Write_Tryll_MutableKbWriteResponse(Writer, Root->body_as_MutableKbWriteResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbDeleteRequest:
        Write_Tryll_MutableKbDeleteRequest(Writer, Root->body_as_MutableKbDeleteRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbDeleteResponse:
        Write_Tryll_MutableKbDeleteResponse(Writer, Root->body_as_MutableKbDeleteResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbClearRequest:
        Write_Tryll_MutableKbClearRequest(Writer, Root->body_as_MutableKbClearRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbClearResponse:
        Write_Tryll_MutableKbClearResponse(Writer, Root->body_as_MutableKbClearResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbQueryRequest:
        Write_Tryll_MutableKbQueryRequest(Writer, Root->body_as_MutableKbQueryRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbQueryResponse:
        Write_Tryll_MutableKbQueryResponse(Writer, Root->body_as_MutableKbQueryResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbGetRequest:
        Write_Tryll_MutableKbGetRequest(Writer, Root->body_as_MutableKbGetRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbGetResponse:
        Write_Tryll_MutableKbGetResponse(Writer, Root->body_as_MutableKbGetResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbListRequest:
        Write_Tryll_MutableKbListRequest(Writer, Root->body_as_MutableKbListRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbListResponse:
        Write_Tryll_MutableKbListResponse(Writer, Root->body_as_MutableKbListResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbStatsRequest:
        Write_Tryll_MutableKbStatsRequest(Writer, Root->body_as_MutableKbStatsRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_MutableKbStatsResponse:
        Write_Tryll_MutableKbStatsResponse(Writer, Root->body_as_MutableKbStatsResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_ExportMutableKbRequest:
        Write_Tryll_ExportMutableKbRequest(Writer, Root->body_as_ExportMutableKbRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_ExportMutableKbResponse:
        Write_Tryll_ExportMutableKbResponse(Writer, Root->body_as_ExportMutableKbResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_ImportMutableKbRequest:
        Write_Tryll_ImportMutableKbRequest(Writer, Root->body_as_ImportMutableKbRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_ImportMutableKbResponse:
        Write_Tryll_ImportMutableKbResponse(Writer, Root->body_as_ImportMutableKbResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_SaveGameMemoryRequest:
        Write_Tryll_SaveGameMemoryRequest(Writer, Root->body_as_SaveGameMemoryRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_SaveGameMemoryResponse:
        Write_Tryll_SaveGameMemoryResponse(Writer, Root->body_as_SaveGameMemoryResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_LoadGameMemoryRequest:
        Write_Tryll_LoadGameMemoryRequest(Writer, Root->body_as_LoadGameMemoryRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_LoadGameMemoryResponse:
        Write_Tryll_LoadGameMemoryResponse(Writer, Root->body_as_LoadGameMemoryResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_SaveDialogRequest:
        Write_Tryll_SaveDialogRequest(Writer, Root->body_as_SaveDialogRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_SaveDialogResponse:
        Write_Tryll_SaveDialogResponse(Writer, Root->body_as_SaveDialogResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_LoadDialogRequest:
        Write_Tryll_LoadDialogRequest(Writer, Root->body_as_LoadDialogRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_LoadDialogResponse:
        Write_Tryll_LoadDialogResponse(Writer, Root->body_as_LoadDialogResponse(), Map, "");
        break;
    case ::Tryll::MessageBody_TtsVisemeFrame:
        Write_Tryll_TtsVisemeFrame(Writer, Root->body_as_TtsVisemeFrame(), Map, "");
        break;
    case ::Tryll::MessageBody_SetAgentVoiceRequest:
        Write_Tryll_SetAgentVoiceRequest(Writer, Root->body_as_SetAgentVoiceRequest(), Map, "");
        break;
    case ::Tryll::MessageBody_SetAgentDeliveryModeRequest:
        Write_Tryll_SetAgentDeliveryModeRequest(Writer, Root->body_as_SetAgentDeliveryModeRequest(), Map, "");
        break;
    default:
        Writer.BeginObject();
        Writer.EndObject();
        break;
    }
    (void)6;
}

void WriteLocal(FTryllJsonWriter& Writer, const std::uint8_t* Data, std::size_t Size,
    const std::vector<FTryllOmittedField>& Omitted)
{
    (void)Size;
    auto Map = Index(Omitted);
    auto* Root = ::Tryll::GetLogLocalEnvelope(Data);
    switch (Root->body_type())
    {
    case ::Tryll::LogLocalBody_LogConnectionState:
        Write_Tryll_LogConnectionState(Writer, Root->body_as_LogConnectionState(), Map, "");
        break;
    case ::Tryll::LogLocalBody_LogLocalError:
        Write_Tryll_LogLocalError(Writer, Root->body_as_LogLocalError(), Map, "");
        break;
    case ::Tryll::LogLocalBody_LogCaptureGap:
        Write_Tryll_LogCaptureGap(Writer, Root->body_as_LogCaptureGap(), Map, "");
        break;
    default:
        Writer.BeginObject();
        Writer.EndObject();
        break;
    }
    (void)6;
}

std::vector<std::uint8_t> StripMessage(const std::uint8_t* Data, std::size_t Size,
    std::vector<FTryllOmittedField>& Omitted)
{
    if (Data == nullptr || Size == 0) return {};
    auto* Root = ::Tryll::GetMessage(Data);
    flatbuffers::FlatBufferBuilder Fbb;
    switch (Root->body_type())
    {
    case ::Tryll::MessageBody_CreateStringStorageRequest:
    {
        auto Body = Pack_Tryll_CreateStringStorageRequest(Fbb, Root->body_as_CreateStringStorageRequest(), Omitted, "");
        auto Msg = ::Tryll::CreateMessage(Fbb, ::Tryll::MessageBody_CreateStringStorageRequest, Body.Union());
        ::Tryll::FinishMessageBuffer(Fbb, Msg);
        return std::vector<std::uint8_t>(Fbb.GetBufferPointer(), Fbb.GetBufferPointer() + Fbb.GetSize());
    }
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest:
    {
        auto Body = Pack_Tryll_CreateEmbeddedStringStorageRequest(Fbb, Root->body_as_CreateEmbeddedStringStorageRequest(), Omitted, "");
        auto Msg = ::Tryll::CreateMessage(Fbb, ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest, Body.Union());
        ::Tryll::FinishMessageBuffer(Fbb, Msg);
        return std::vector<std::uint8_t>(Fbb.GetBufferPointer(), Fbb.GetBufferPointer() + Fbb.GetSize());
    }
    case ::Tryll::MessageBody_AudioBuffer:
    {
        auto Body = Pack_Tryll_AudioBuffer(Fbb, Root->body_as_AudioBuffer(), Omitted, "");
        auto Msg = ::Tryll::CreateMessage(Fbb, ::Tryll::MessageBody_AudioBuffer, Body.Union());
        ::Tryll::FinishMessageBuffer(Fbb, Msg);
        return std::vector<std::uint8_t>(Fbb.GetBufferPointer(), Fbb.GetBufferPointer() + Fbb.GetSize());
    }
    case ::Tryll::MessageBody_TtsAudioFrame:
    {
        auto Body = Pack_Tryll_TtsAudioFrame(Fbb, Root->body_as_TtsAudioFrame(), Omitted, "");
        auto Msg = ::Tryll::CreateMessage(Fbb, ::Tryll::MessageBody_TtsAudioFrame, Body.Union());
        ::Tryll::FinishMessageBuffer(Fbb, Msg);
        return std::vector<std::uint8_t>(Fbb.GetBufferPointer(), Fbb.GetBufferPointer() + Fbb.GetSize());
    }
    case ::Tryll::MessageBody_ExportDialogResponse:
    {
        auto Body = Pack_Tryll_ExportDialogResponse(Fbb, Root->body_as_ExportDialogResponse(), Omitted, "");
        auto Msg = ::Tryll::CreateMessage(Fbb, ::Tryll::MessageBody_ExportDialogResponse, Body.Union());
        ::Tryll::FinishMessageBuffer(Fbb, Msg);
        return std::vector<std::uint8_t>(Fbb.GetBufferPointer(), Fbb.GetBufferPointer() + Fbb.GetSize());
    }
    case ::Tryll::MessageBody_ImportDialogRequest:
    {
        auto Body = Pack_Tryll_ImportDialogRequest(Fbb, Root->body_as_ImportDialogRequest(), Omitted, "");
        auto Msg = ::Tryll::CreateMessage(Fbb, ::Tryll::MessageBody_ImportDialogRequest, Body.Union());
        ::Tryll::FinishMessageBuffer(Fbb, Msg);
        return std::vector<std::uint8_t>(Fbb.GetBufferPointer(), Fbb.GetBufferPointer() + Fbb.GetSize());
    }
    case ::Tryll::MessageBody_ExportMutableKbResponse:
    {
        auto Body = Pack_Tryll_ExportMutableKbResponse(Fbb, Root->body_as_ExportMutableKbResponse(), Omitted, "");
        auto Msg = ::Tryll::CreateMessage(Fbb, ::Tryll::MessageBody_ExportMutableKbResponse, Body.Union());
        ::Tryll::FinishMessageBuffer(Fbb, Msg);
        return std::vector<std::uint8_t>(Fbb.GetBufferPointer(), Fbb.GetBufferPointer() + Fbb.GetSize());
    }
    case ::Tryll::MessageBody_ImportMutableKbRequest:
    {
        auto Body = Pack_Tryll_ImportMutableKbRequest(Fbb, Root->body_as_ImportMutableKbRequest(), Omitted, "");
        auto Msg = ::Tryll::CreateMessage(Fbb, ::Tryll::MessageBody_ImportMutableKbRequest, Body.Union());
        ::Tryll::FinishMessageBuffer(Fbb, Msg);
        return std::vector<std::uint8_t>(Fbb.GetBufferPointer(), Fbb.GetBufferPointer() + Fbb.GetSize());
    }
    case ::Tryll::MessageBody_TtsVisemeFrame:
    {
        auto Body = Pack_Tryll_TtsVisemeFrame(Fbb, Root->body_as_TtsVisemeFrame(), Omitted, "");
        auto Msg = ::Tryll::CreateMessage(Fbb, ::Tryll::MessageBody_TtsVisemeFrame, Body.Union());
        ::Tryll::FinishMessageBuffer(Fbb, Msg);
        return std::vector<std::uint8_t>(Fbb.GetBufferPointer(), Fbb.GetBufferPointer() + Fbb.GetSize());
    }
    default:
        return std::vector<std::uint8_t>(Data, Data + Size);
    }
}

void ReadScope(const std::uint8_t* Data, std::size_t Size, ::Tryll::MessageBody& Body,
    std::uint64_t& RequestId, std::uint64_t& AgentId, std::uint64_t& VoiceInputId)
{
    RequestId = 0;
    AgentId = 0;
    VoiceInputId = 0;
    Body = ::Tryll::MessageBody_NONE;
    if (Data == nullptr || Size == 0) return;
    auto* Root = ::Tryll::GetMessage(Data);
    Body = Root->body_type();
    switch (Body)
    {
    case ::Tryll::MessageBody_CreateSessionRequest:
    {
        const auto* Row = Root->body_as_CreateSessionRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_CreateSessionResponse:
    {
        const auto* Row = Root->body_as_CreateSessionResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_CreateAgentRequest:
    {
        const auto* Row = Root->body_as_CreateAgentRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_CreateAgentResponse:
    {
        const auto* Row = Root->body_as_CreateAgentResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_SendMessageRequest:
    {
        const auto* Row = Root->body_as_SendMessageRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_DestroyAgentRequest:
    {
        const auto* Row = Root->body_as_DestroyAgentRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_AnswerText:
    {
        const auto* Row = Root->body_as_AnswerText();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_TurnComplete:
    {
        const auto* Row = Root->body_as_TurnComplete();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_ErrorResponse:
    {
        const auto* Row = Root->body_as_ErrorResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_Ack:
    {
        const auto* Row = Root->body_as_Ack();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_ListModelsRequest:
    {
        const auto* Row = Root->body_as_ListModelsRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_ListModelsResponse:
    {
        const auto* Row = Root->body_as_ListModelsResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_DownloadModelRequest:
    {
        const auto* Row = Root->body_as_DownloadModelRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_DownloadProgress:
    {
        const auto* Row = Root->body_as_DownloadProgress();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_DownloadComplete:
    {
        const auto* Row = Root->body_as_DownloadComplete();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_CreateStringStorageRequest:
    {
        const auto* Row = Root->body_as_CreateStringStorageRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_CreateStringStorageResponse:
    {
        const auto* Row = Root->body_as_CreateStringStorageResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_DestroyStringStorageRequest:
    {
        const auto* Row = Root->body_as_DestroyStringStorageRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_NodeEvent:
    {
        const auto* Row = Root->body_as_NodeEvent();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_LoadModelRequest:
    {
        const auto* Row = Root->body_as_LoadModelRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_LoadModelResponse:
    {
        const auto* Row = Root->body_as_LoadModelResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_UnloadModelRequest:
    {
        const auto* Row = Root->body_as_UnloadModelRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest:
    {
        const auto* Row = Root->body_as_CreateEmbeddedStringStorageRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageResponse:
    {
        const auto* Row = Root->body_as_CreateEmbeddedStringStorageResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_DestroyEmbeddedStringStorageRequest:
    {
        const auto* Row = Root->body_as_DestroyEmbeddedStringStorageRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_ChangeAgentParamRequest:
    {
        const auto* Row = Root->body_as_ChangeAgentParamRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_CreateVoiceInputRequest:
    {
        const auto* Row = Root->body_as_CreateVoiceInputRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_CreateVoiceInputResponse:
    {
        const auto* Row = Root->body_as_CreateVoiceInputResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        VoiceInputId = static_cast<std::uint64_t>(Row->voice_input_id());
        break;
    }
    case ::Tryll::MessageBody_BeginUtteranceRequest:
    {
        const auto* Row = Root->body_as_BeginUtteranceRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        VoiceInputId = static_cast<std::uint64_t>(Row->voice_input_id());
        break;
    }
    case ::Tryll::MessageBody_AudioBuffer:
    {
        const auto* Row = Root->body_as_AudioBuffer();
        if (Row == nullptr) break;
        VoiceInputId = static_cast<std::uint64_t>(Row->voice_input_id());
        break;
    }
    case ::Tryll::MessageBody_EndUtteranceRequest:
    {
        const auto* Row = Root->body_as_EndUtteranceRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        VoiceInputId = static_cast<std::uint64_t>(Row->voice_input_id());
        break;
    }
    case ::Tryll::MessageBody_CancelUtteranceRequest:
    {
        const auto* Row = Root->body_as_CancelUtteranceRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        VoiceInputId = static_cast<std::uint64_t>(Row->voice_input_id());
        break;
    }
    case ::Tryll::MessageBody_WireTranscriptUpdate:
    {
        const auto* Row = Root->body_as_WireTranscriptUpdate();
        if (Row == nullptr) break;
        VoiceInputId = static_cast<std::uint64_t>(Row->voice_input_id());
        break;
    }
    case ::Tryll::MessageBody_DestroyVoiceInputRequest:
    {
        const auto* Row = Root->body_as_DestroyVoiceInputRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        VoiceInputId = static_cast<std::uint64_t>(Row->voice_input_id());
        break;
    }
    case ::Tryll::MessageBody_TtsAudioFormatFrame:
    {
        const auto* Row = Root->body_as_TtsAudioFormatFrame();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_TtsAudioFrame:
    {
        const auto* Row = Root->body_as_TtsAudioFrame();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_DeleteModelRequest:
    {
        const auto* Row = Root->body_as_DeleteModelRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_DebugCommandRequest:
    {
        const auto* Row = Root->body_as_DebugCommandRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_DebugCommandResponse:
    {
        const auto* Row = Root->body_as_DebugCommandResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_CancelRequest:
    {
        const auto* Row = Root->body_as_CancelRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_ResumeAgentRequest:
    {
        const auto* Row = Root->body_as_ResumeAgentRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_UpdateAgentVariablesRequest:
    {
        const auto* Row = Root->body_as_UpdateAgentVariablesRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_GetAgentVariablesRequest:
    {
        const auto* Row = Root->body_as_GetAgentVariablesRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_GetAgentVariablesResponse:
    {
        const auto* Row = Root->body_as_GetAgentVariablesResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_PrefillAgentKvCacheRequest:
    {
        const auto* Row = Root->body_as_PrefillAgentKvCacheRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_PrefillAgentKvCacheResponse:
    {
        const auto* Row = Root->body_as_PrefillAgentKvCacheResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_EvictAgentKvCacheRequest:
    {
        const auto* Row = Root->body_as_EvictAgentKvCacheRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_EvictAgentKvCacheResponse:
    {
        const auto* Row = Root->body_as_EvictAgentKvCacheResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_GetAgentKvCacheStatusRequest:
    {
        const auto* Row = Root->body_as_GetAgentKvCacheStatusRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_GetAgentKvCacheStatusResponse:
    {
        const auto* Row = Root->body_as_GetAgentKvCacheStatusResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_AppendInteractionsRequest:
    {
        const auto* Row = Root->body_as_AppendInteractionsRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_AppendInteractionsResponse:
    {
        const auto* Row = Root->body_as_AppendInteractionsResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_RemoveInteractionsFromEndRequest:
    {
        const auto* Row = Root->body_as_RemoveInteractionsFromEndRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_RemoveInteractionsFromEndResponse:
    {
        const auto* Row = Root->body_as_RemoveInteractionsFromEndResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_SetSessionLocaleRequest:
    {
        const auto* Row = Root->body_as_SetSessionLocaleRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_ExportDialogRequest:
    {
        const auto* Row = Root->body_as_ExportDialogRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_ExportDialogResponse:
    {
        const auto* Row = Root->body_as_ExportDialogResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_ImportDialogRequest:
    {
        const auto* Row = Root->body_as_ImportDialogRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_ImportDialogResponse:
    {
        const auto* Row = Root->body_as_ImportDialogResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_GetMemoryConsumptionRequest:
    {
        const auto* Row = Root->body_as_GetMemoryConsumptionRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_GetMemoryConsumptionResponse:
    {
        const auto* Row = Root->body_as_GetMemoryConsumptionResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_EnsureMutableKbRequest:
    {
        const auto* Row = Root->body_as_EnsureMutableKbRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_EnsureMutableKbResponse:
    {
        const auto* Row = Root->body_as_EnsureMutableKbResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_EnumerateMutableKbsRequest:
    {
        const auto* Row = Root->body_as_EnumerateMutableKbsRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_EnumerateMutableKbsResponse:
    {
        const auto* Row = Root->body_as_EnumerateMutableKbsResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbWriteRequest:
    {
        const auto* Row = Root->body_as_MutableKbWriteRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbWriteResponse:
    {
        const auto* Row = Root->body_as_MutableKbWriteResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbDeleteRequest:
    {
        const auto* Row = Root->body_as_MutableKbDeleteRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbDeleteResponse:
    {
        const auto* Row = Root->body_as_MutableKbDeleteResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbClearRequest:
    {
        const auto* Row = Root->body_as_MutableKbClearRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbClearResponse:
    {
        const auto* Row = Root->body_as_MutableKbClearResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbQueryRequest:
    {
        const auto* Row = Root->body_as_MutableKbQueryRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbQueryResponse:
    {
        const auto* Row = Root->body_as_MutableKbQueryResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbGetRequest:
    {
        const auto* Row = Root->body_as_MutableKbGetRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbGetResponse:
    {
        const auto* Row = Root->body_as_MutableKbGetResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbListRequest:
    {
        const auto* Row = Root->body_as_MutableKbListRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbListResponse:
    {
        const auto* Row = Root->body_as_MutableKbListResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbStatsRequest:
    {
        const auto* Row = Root->body_as_MutableKbStatsRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_MutableKbStatsResponse:
    {
        const auto* Row = Root->body_as_MutableKbStatsResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_ExportMutableKbRequest:
    {
        const auto* Row = Root->body_as_ExportMutableKbRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_ExportMutableKbResponse:
    {
        const auto* Row = Root->body_as_ExportMutableKbResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_ImportMutableKbRequest:
    {
        const auto* Row = Root->body_as_ImportMutableKbRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_ImportMutableKbResponse:
    {
        const auto* Row = Root->body_as_ImportMutableKbResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_SaveGameMemoryRequest:
    {
        const auto* Row = Root->body_as_SaveGameMemoryRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_SaveGameMemoryResponse:
    {
        const auto* Row = Root->body_as_SaveGameMemoryResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_LoadGameMemoryRequest:
    {
        const auto* Row = Root->body_as_LoadGameMemoryRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_LoadGameMemoryResponse:
    {
        const auto* Row = Root->body_as_LoadGameMemoryResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        break;
    }
    case ::Tryll::MessageBody_SaveDialogRequest:
    {
        const auto* Row = Root->body_as_SaveDialogRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_SaveDialogResponse:
    {
        const auto* Row = Root->body_as_SaveDialogResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_LoadDialogRequest:
    {
        const auto* Row = Root->body_as_LoadDialogRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_LoadDialogResponse:
    {
        const auto* Row = Root->body_as_LoadDialogResponse();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_TtsVisemeFrame:
    {
        const auto* Row = Root->body_as_TtsVisemeFrame();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_SetAgentVoiceRequest:
    {
        const auto* Row = Root->body_as_SetAgentVoiceRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    case ::Tryll::MessageBody_SetAgentDeliveryModeRequest:
    {
        const auto* Row = Root->body_as_SetAgentDeliveryModeRequest();
        if (Row == nullptr) break;
        RequestId = static_cast<std::uint64_t>(Row->request_id());
        AgentId = static_cast<std::uint64_t>(Row->agent_id());
        break;
    }
    default: break;
    }
    (void)Size;
}

std::string Label(::Tryll::MessageBody Body, const std::uint8_t* Data, std::size_t Size)
{
    (void)Size;
    if (Data == nullptr) return {};
    auto* Root = ::Tryll::GetMessage(Data);
    switch (Body)
    {
    case ::Tryll::MessageBody_SendMessageRequest:
        return Label_Tryll_SendMessageRequest(Root->body_as_SendMessageRequest());
    case ::Tryll::MessageBody_AnswerText:
        return Label_Tryll_AnswerText(Root->body_as_AnswerText());
    case ::Tryll::MessageBody_DownloadProgress:
        return Label_Tryll_DownloadProgress(Root->body_as_DownloadProgress());
    case ::Tryll::MessageBody_NodeEvent:
        return Label_Tryll_NodeEvent(Root->body_as_NodeEvent());
    case ::Tryll::MessageBody_ChangeAgentParamRequest:
        return Label_Tryll_ChangeAgentParamRequest(Root->body_as_ChangeAgentParamRequest());
    case ::Tryll::MessageBody_TtsAudioFrame:
        return Label_Tryll_TtsAudioFrame(Root->body_as_TtsAudioFrame());
    case ::Tryll::MessageBody_TtsVisemeFrame:
        return Label_Tryll_TtsVisemeFrame(Root->body_as_TtsVisemeFrame());
    default:
        return {};
    }
}

static const FTryllLogField kFields_Tryll_Ack[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "warning", "string", "Set when a successful call changed something the caller did not ask for. SetAgentVoice fills this when the active delivery mode is not on the new profile and the server falls back to that profile's default mode. Empty on every other ack.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_Ack = {
    "Ack", "",
    "", "",
    false, false,
    kFields_Tryll_Ack, 3,
};
static const char* kEnum_Tryll_AgentVariable_value[] = { "NONE", "VarInt", "VarFloat", "VarString", "VarBool", "VarStringSet" };
static const FTryllLogField kFields_Tryll_AgentVariable[] = {
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "value", "union", "",
        "null", kEnum_Tryll_AgentVariable_value, 6,
        "", "",
        "VariableValue", "", "",
    },
    {
        "allow_output_substitution", "scalar", "When true, this variable may appear as `__NAME__` in Generate / GenerateAndSpeak output and be replaced with its rendered value. Immutable declaration metadata (not overridable per-component). Default false — opt-in. Forbidden for StringSet; enabled strings are capped at 2048 UTF-8 bytes (see AgentVariables validation).",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_AgentVariable = {
    "AgentVariable", "",
    "", "",
    false, false,
    kFields_Tryll_AgentVariable, 3,
};
static const FTryllLogField kFields_Tryll_AnswerText[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "multiline",
        "", "", "",
    },
    {
        "is_delta", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "is_final", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "node_name", "string", "Name of the node that produced this text (multi-sender attribution).",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_AnswerText = {
    "AnswerText", "{node_name}",
    "concat", "text",
    true, true,
    kFields_Tryll_AnswerText, 6,
};
static const FTryllLogField kFields_Tryll_AppendInteractionsRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "interactions", "vector", "",
        "", nullptr, 0,
        "", "",
        "DialogInteraction", "table", "DialogInteraction",
    },
};
static const FTryllLogTable kTable_Tryll_AppendInteractionsRequest = {
    "AppendInteractionsRequest", "",
    "", "",
    false, false,
    kFields_Tryll_AppendInteractionsRequest, 3,
};
static const FTryllLogField kFields_Tryll_AppendInteractionsResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "appended_count", "scalar", "Number of interactions actually appended (after skipping all-empty pairs).",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_AppendInteractionsResponse = {
    "AppendInteractionsResponse", "",
    "", "",
    false, false,
    kFields_Tryll_AppendInteractionsResponse, 3,
};
static const FTryllLogField kFields_Tryll_AudioBuffer[] = {
    {
        "voice_input_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "pcm", "vector", "",
        "null", nullptr, 0,
        "size", "",
        "", "scalar", "",
    },
};
static const FTryllLogTable kTable_Tryll_AudioBuffer = {
    "AudioBuffer", "",
    "sum", "",
    false, false,
    kFields_Tryll_AudioBuffer, 2,
};
static const FTryllLogField kFields_Tryll_BeginUtteranceRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "voice_input_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "auto_finish_on_silence", "scalar", "",
        "true", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "max_utterance_ms", "scalar", "",
        "60000", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_BeginUtteranceRequest = {
    "BeginUtteranceRequest", "",
    "", "",
    false, false,
    kFields_Tryll_BeginUtteranceRequest, 4,
};
static const char* kEnum_Tryll_CancelRequest_mode[] = { "StopAndKeep", "StopAndDiscard" };
static const FTryllLogField kFields_Tryll_CancelRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "mode", "enum", "",
        "StopAndKeep", kEnum_Tryll_CancelRequest_mode, 2,
        "", "",
        "CancelMode", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CancelRequest = {
    "CancelRequest", "",
    "", "",
    false, false,
    kFields_Tryll_CancelRequest, 3,
};
static const FTryllLogField kFields_Tryll_CancelUtteranceRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "voice_input_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CancelUtteranceRequest = {
    "CancelUtteranceRequest", "",
    "", "",
    false, false,
    kFields_Tryll_CancelUtteranceRequest, 2,
};
static const char* kEnum_Tryll_ChangeAgentParamRequest_params[] = { "NONE", "GenerateParams", "RegexGuardrailParams", "CannedResponseParams", "ToolCallParams", "RetrieveParams", "InstructionParams", "ClassifyIntentParams", "ClassifyIntentLLMParams", "IntentToInstructionParams", "GenerateAndSpeakParams", "SpeakParams", "PauseParams", "TransformParams", "BranchParams", "WriteToMemoryParams", "SummarizeParams", "ClassifyAttributesLLMParams" };
static const FTryllLogField kFields_Tryll_ChangeAgentParamRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "node_name", "string", "Identifies the target node by name (addressing data, kept on envelope).",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "params", "union", "Whole authored typed params for the node.  Must match the node's concrete type.",
        "null", kEnum_Tryll_ChangeAgentParamRequest_params, 18,
        "", "",
        "NodeParams", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ChangeAgentParamRequest = {
    "ChangeAgentParamRequest", "{node_name} · {params_type}",
    "", "",
    true, false,
    kFields_Tryll_ChangeAgentParamRequest, 4,
};
static const FTryllLogField kFields_Tryll_ConnectionReady[] = {
    {
        "protocol_version", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "codegen_fingerprint", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "exe_path", "string", "Canonical path of the server executable (symlinks resolved). Clients use this to verify they connected to the intended build.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "build_config", "string", "\"Debug\" | \"Release\" | \"ClientDev\" | \"ClientProd\"",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "version", "string", "Tryll::kVersion string (e.g. \"1.2.3\").",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "run_id", "string", "UUID for this server process run — correlates logs and telemetry.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "managed", "scalar", "True when the server was launched in managed mode (--idle-shutdown-timeout > 0). A managed server self-exits after the grace period with zero sessions.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "idle_timeout_seconds", "scalar", "Seconds the server will wait after the last session disconnects before self-exiting.  0 when not managed (unmanaged launch).",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "mode", "string", "Deployment mode this process is running: \"tryll-dev\", \"client-dev\", or \"client-prod\". Added in protocol v24, appended so earlier fields keep their positions.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ConnectionReady = {
    "ConnectionReady", "",
    "", "",
    false, false,
    kFields_Tryll_ConnectionReady, 9,
};
static const char* kEnum_Tryll_CreateAgentRequest_kv_cache_initialization[] = { "AllocateOnly", "Prefill", "DeferAllocation" };
static const char* kEnum_Tryll_CreateAgentRequest_workload[] = { "Interactive", "Background" };
static const FTryllLogField kFields_Tryll_CreateAgentRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "graph", "table", "",
        "null", nullptr, 0,
        "", "",
        "GraphDescription", "", "",
    },
    {
        "default_model_name", "string", "Fallback model name for nodes that do not specify their own \"model_name\" param. If empty, the session has no default and every Generate node must supply its own.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "enable_diagnostics", "scalar", "When true, the server serializes workflow execution data into TurnComplete.debug_info. Intended for QA/eval pipelines. Off by default for zero overhead in production.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "maintain_dialogue_history", "scalar", "When false, the agent's dialogue history is discarded after each turn completes (stateless agent: every turn starts with the system prompt + the new turn only). Useful for technical agents — classification, routing, tool-picking — that gain nothing from prior turns and should not pay to store or project them. Default true = accumulate the full history (previous behaviour).",
        "true", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "variables", "vector", "Complete declaration of this agent's variables: every variable it will ever have, with its initial value (the value's union member fixes the type for the agent's lifetime). Post-create UpdateAgentVariablesRequest updates may only target these names (UnknownVariable otherwise).",
        "null", nullptr, 0,
        "", "",
        "AgentVariable", "table", "AgentVariable",
    },
    {
        "kv_cache_initialization", "enum", "How language-model KV contexts are materialised for this agent. Default AllocateOnly = previous behaviour (allocate, no explicit prefill).",
        "AllocateOnly", kEnum_Tryll_CreateAgentRequest_kv_cache_initialization, 3,
        "", "",
        "AgentKvCacheInitialization", "", "",
    },
    {
        "workload", "enum", "Whether a human ever waits on this agent's turns. Default Interactive = previous behaviour. Set Background for world-logic/evaluator agents so the inference scheduler paces or defers their work first under load.",
        "Interactive", kEnum_Tryll_CreateAgentRequest_workload, 2,
        "", "",
        "AgentWorkload", "", "",
    },
    {
        "voice", "table", "The agent's voice (protocol v27). Required when any node in `graph` speaks (SpeakNode, GenerateAndSpeakNode): creation fails with AgentVoiceRequired (3025) without it, and with InvalidVoiceProfile (3023) when the profile does not fit its model. Ignored, and no TTS model is loaded, when no node speaks.",
        "null", nullptr, 0,
        "", "",
        "VoiceProfile", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CreateAgentRequest = {
    "CreateAgentRequest", "",
    "", "",
    false, false,
    kFields_Tryll_CreateAgentRequest, 9,
};
static const FTryllLogField kFields_Tryll_CreateAgentResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CreateAgentResponse = {
    "CreateAgentResponse", "",
    "", "",
    false, false,
    kFields_Tryll_CreateAgentResponse, 2,
};
static const FTryllLogField kFields_Tryll_CreateEmbeddedStringStorageRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "config_path", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "strings", "vector", "",
        "null", nullptr, 0,
        "size", "",
        "", "string", "",
    },
    {
        "embedding_model", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CreateEmbeddedStringStorageRequest = {
    "CreateEmbeddedStringStorageRequest", "",
    "", "",
    false, false,
    kFields_Tryll_CreateEmbeddedStringStorageRequest, 5,
};
static const FTryllLogField kFields_Tryll_CreateEmbeddedStringStorageResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "record_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "embedding_dim", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CreateEmbeddedStringStorageResponse = {
    "CreateEmbeddedStringStorageResponse", "",
    "", "",
    false, false,
    kFields_Tryll_CreateEmbeddedStringStorageResponse, 4,
};
static const FTryllLogField kFields_Tryll_CreateSessionRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "game_name", "string", "Identifies the integration (e.g. \"test-chat\", \"qa-and-eval\", \"<partner-game>\"). Used for telemetry grouping.  Nullable — omitting it is valid for backward compatibility; the server will default to \"unknown\" when telemetry is enabled.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "locale", "string", "Optional session language as a BCP-47 tag (\"de\", \"de-DE\", \"pt-BR\"). One setting the whole voice + text stack derives from, so a game does not have to push a language into every agent and every node: • TTS   — default for each GenerateAndSpeak/Speak node's `tts_lang` (an explicitly authored `tts_lang` still wins). • STT   — language hint for multilingual recognizers (Whisper, Canary); ignored by models whose language is fixed by the weights. • Files — relative storage paths prefer a translation in a `loc/<language>/` folder beside the linked file, falling back to the linked file itself, so per-language knowledge bases, canned lines and prompts need no code. • Text  — exposed to Mustache templates as the `locale`, `language` and `language_code` keys, for authoring a \"reply in X\" directive. (Written without braces here: flatc's own code writer treats a doubled brace in a doc comment as a template key.) Empty = unset: every one of the above keeps its pre-locale behaviour. Rejected with InvalidLocale (2004) when it is not a well-formed tag. Changeable later via SetSessionLocaleRequest (57), which reaches existing agents' voice + template keys but not content already bound to a node.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "model_config", "table", "What this session needs from the model layer: which engines, and where each model should live. Replaces the four `*_engine` fields this table carried through protocol v12 — the catalog already states each model's engine, so a client declares what it needs instead of choosing an implementation per model type. Nested one level rather than held as two vectors directly here; see SessionModelConfig for why that matters.",
        "null", nullptr, 0,
        "", "",
        "SessionModelConfig", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CreateSessionRequest = {
    "CreateSessionRequest", "",
    "", "",
    false, false,
    kFields_Tryll_CreateSessionRequest, 4,
};
static const FTryllLogField kFields_Tryll_CreateSessionResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "session_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CreateSessionResponse = {
    "CreateSessionResponse", "",
    "", "",
    false, false,
    kFields_Tryll_CreateSessionResponse, 2,
};
static const char* kEnum_Tryll_CreateStringStorageRequest_kind[] = { "List", "Map", "Multimap" };
static const FTryllLogField kFields_Tryll_CreateStringStorageRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "kind", "enum", "",
        "List", kEnum_Tryll_CreateStringStorageRequest_kind, 3,
        "", "",
        "StringStorageKind", "", "",
    },
    {
        "strings", "vector", "",
        "null", nullptr, 0,
        "size", "",
        "", "string", "",
    },
    {
        "keys", "vector", "",
        "null", nullptr, 0,
        "size", "",
        "", "string", "",
    },
    {
        "file_path", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CreateStringStorageRequest = {
    "CreateStringStorageRequest", "",
    "", "",
    false, false,
    kFields_Tryll_CreateStringStorageRequest, 6,
};
static const FTryllLogField kFields_Tryll_CreateStringStorageResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CreateStringStorageResponse = {
    "CreateStringStorageResponse", "",
    "", "",
    false, false,
    kFields_Tryll_CreateStringStorageResponse, 2,
};
static const FTryllLogField kFields_Tryll_CreateVoiceInputRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "model_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "input_format", "table", "",
        "null", nullptr, 0,
        "", "",
        "WireAudioFormat", "", "",
    },
    {
        "vad_threshold", "scalar", "",
        "0.5", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "vad_min_silence_ms", "scalar", "",
        "1000", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "vad_speech_pad_ms", "scalar", "",
        "250", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "hotwords_storage_path", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "hotwords_score", "scalar", "",
        "1.5", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CreateVoiceInputRequest = {
    "CreateVoiceInputRequest", "",
    "", "",
    false, false,
    kFields_Tryll_CreateVoiceInputRequest, 8,
};
static const FTryllLogField kFields_Tryll_CreateVoiceInputResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "voice_input_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_CreateVoiceInputResponse = {
    "CreateVoiceInputResponse", "",
    "", "",
    false, false,
    kFields_Tryll_CreateVoiceInputResponse, 2,
};
static const FTryllLogField kFields_Tryll_DebugCommandRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "command", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "payload", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DebugCommandRequest = {
    "DebugCommandRequest", "",
    "", "",
    false, false,
    kFields_Tryll_DebugCommandRequest, 3,
};
static const FTryllLogField kFields_Tryll_DebugCommandResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "result", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DebugCommandResponse = {
    "DebugCommandResponse", "",
    "", "",
    false, false,
    kFields_Tryll_DebugCommandResponse, 2,
};
static const FTryllLogField kFields_Tryll_DeleteModelRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "model_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DeleteModelRequest = {
    "DeleteModelRequest", "",
    "", "",
    false, false,
    kFields_Tryll_DeleteModelRequest, 2,
};
static const FTryllLogField kFields_Tryll_DeliveryMode[] = {
    {
        "name", "string", "Mode name used by SetAgentDeliveryModeRequest: [a-z0-9_]{1,32}, unique in the profile. \"neutral\" is built in and may not be defined.",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "reference_voice", "string", "Clone families (Pocket TTS, Chatterbox, Qwen3-TTS Base): a reference clip of this character speaking in this mood, relative to the storage root. Replaces the profile's reference_voice while the mode is active.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "instruction", "string", "Maya1: sustained delivery clause appended to the voice description. A known keyword (angry, sad, whisper, ...) is rewritten into its tuned clause; anything else is appended as written.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "expressiveness", "scalar", "Chatterbox: exaggeration for this mode, 0.0-2.0. Negative = use the profile's expressiveness.",
        "-1", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "speed_scale", "scalar", "Multiplies the profile speed while the mode is active, 0.5-2.0. Ignored (must stay 1.0) on models without speed control.",
        "1", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DeliveryMode = {
    "DeliveryMode", "",
    "", "",
    false, false,
    kFields_Tryll_DeliveryMode, 5,
};
static const FTryllLogField kFields_Tryll_DestroyAgentRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DestroyAgentRequest = {
    "DestroyAgentRequest", "",
    "", "",
    false, false,
    kFields_Tryll_DestroyAgentRequest, 2,
};
static const FTryllLogField kFields_Tryll_DestroyEmbeddedStringStorageRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DestroyEmbeddedStringStorageRequest = {
    "DestroyEmbeddedStringStorageRequest", "",
    "", "",
    false, false,
    kFields_Tryll_DestroyEmbeddedStringStorageRequest, 2,
};
static const FTryllLogField kFields_Tryll_DestroyStringStorageRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DestroyStringStorageRequest = {
    "DestroyStringStorageRequest", "",
    "", "",
    false, false,
    kFields_Tryll_DestroyStringStorageRequest, 2,
};
static const FTryllLogField kFields_Tryll_DestroyVoiceInputRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "voice_input_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DestroyVoiceInputRequest = {
    "DestroyVoiceInputRequest", "",
    "", "",
    false, false,
    kFields_Tryll_DestroyVoiceInputRequest, 2,
};
static const FTryllLogField kFields_Tryll_DialogInteraction[] = {
    {
        "user_message", "string", "",
        "null", nullptr, 0,
        "", "multiline",
        "", "", "",
    },
    {
        "assistant_message", "string", "",
        "null", nullptr, 0,
        "", "multiline",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DialogInteraction = {
    "DialogInteraction", "",
    "", "",
    false, false,
    kFields_Tryll_DialogInteraction, 2,
};
static const FTryllLogField kFields_Tryll_DownloadComplete[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "model_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "success", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "error", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DownloadComplete = {
    "DownloadComplete", "",
    "", "",
    false, false,
    kFields_Tryll_DownloadComplete, 4,
};
static const FTryllLogField kFields_Tryll_DownloadModelRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "model_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DownloadModelRequest = {
    "DownloadModelRequest", "",
    "", "",
    false, false,
    kFields_Tryll_DownloadModelRequest, 2,
};
static const FTryllLogField kFields_Tryll_DownloadProgress[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "model_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "bytes_downloaded", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "total_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "percent", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_DownloadProgress = {
    "DownloadProgress", "{model_name}",
    "last", "",
    false, false,
    kFields_Tryll_DownloadProgress, 5,
};
static const FTryllLogField kFields_Tryll_EndUtteranceRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "voice_input_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_EndUtteranceRequest = {
    "EndUtteranceRequest", "",
    "", "",
    false, false,
    kFields_Tryll_EndUtteranceRequest, 2,
};
static const char* kEnum_Tryll_EngineSelection_role[] = { "Language", "Embedding", "Stt", "Tts", "Vad" };
static const char* kEnum_Tryll_EngineSelection_engine[] = { "Mock", "LlamaCpp", "OnnxGenAI", "WindowsML", "OpenVino", "TensorRtLlm", "SherpaOnnx", "OnnxRuntime" };
static const FTryllLogField kFields_Tryll_EngineSelection[] = {
    {
        "role", "enum", "",
        "Language", kEnum_Tryll_EngineSelection_role, 5,
        "", "",
        "ModelRole", "", "",
    },
    {
        "engine", "enum", "",
        "Mock", kEnum_Tryll_EngineSelection_engine, 8,
        "", "",
        "InferenceEngine", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_EngineSelection = {
    "EngineSelection", "",
    "", "",
    false, false,
    kFields_Tryll_EngineSelection, 2,
};
static const FTryllLogField kFields_Tryll_EnsureMutableKbRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "schema", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_EnsureMutableKbRequest = {
    "EnsureMutableKbRequest", "",
    "", "",
    false, false,
    kFields_Tryll_EnsureMutableKbRequest, 3,
};
static const FTryllLogField kFields_Tryll_EnsureMutableKbResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "schema", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "created", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_EnsureMutableKbResponse = {
    "EnsureMutableKbResponse", "",
    "", "",
    false, false,
    kFields_Tryll_EnsureMutableKbResponse, 4,
};
static const FTryllLogField kFields_Tryll_EnumerateMutableKbsRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_EnumerateMutableKbsRequest = {
    "EnumerateMutableKbsRequest", "",
    "", "",
    false, false,
    kFields_Tryll_EnumerateMutableKbsRequest, 1,
};
static const FTryllLogField kFields_Tryll_EnumerateMutableKbsResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "stores", "vector", "",
        "null", nullptr, 0,
        "", "",
        "MutableKbInfo", "table", "MutableKbInfo",
    },
};
static const FTryllLogTable kTable_Tryll_EnumerateMutableKbsResponse = {
    "EnumerateMutableKbsResponse", "",
    "", "",
    false, false,
    kFields_Tryll_EnumerateMutableKbsResponse, 2,
};
static const FTryllLogField kFields_Tryll_ErrorResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "code", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "message", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ErrorResponse = {
    "ErrorResponse", "",
    "", "",
    false, false,
    kFields_Tryll_ErrorResponse, 4,
};
static const FTryllLogField kFields_Tryll_EvictAgentKvCacheRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_EvictAgentKvCacheRequest = {
    "EvictAgentKvCacheRequest", "",
    "", "",
    false, false,
    kFields_Tryll_EvictAgentKvCacheRequest, 2,
};
static const char* kEnum_Tryll_EvictAgentKvCacheResponse_residency[] = { "Evicted", "Resident" };
static const char* kEnum_Tryll_EvictAgentKvCacheResponse_prefix_status[] = { "NotCurrent", "Current" };
static const FTryllLogField kFields_Tryll_EvictAgentKvCacheResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "applicable", "scalar", "",
        "true", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "residency", "enum", "",
        "Evicted", kEnum_Tryll_EvictAgentKvCacheResponse_residency, 2,
        "", "",
        "AgentKvCacheResidency", "", "",
    },
    {
        "prefix_status", "enum", "",
        "NotCurrent", kEnum_Tryll_EvictAgentKvCacheResponse_prefix_status, 2,
        "", "",
        "AgentKvCachePrefixStatus", "", "",
    },
    {
        "eligible_node_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "contexts_evicted", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "debug_info", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_EvictAgentKvCacheResponse = {
    "EvictAgentKvCacheResponse", "",
    "", "",
    false, false,
    kFields_Tryll_EvictAgentKvCacheResponse, 8,
};
static const FTryllLogField kFields_Tryll_ExportDialogRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ExportDialogRequest = {
    "ExportDialogRequest", "",
    "", "",
    false, false,
    kFields_Tryll_ExportDialogRequest, 2,
};
static const FTryllLogField kFields_Tryll_ExportDialogResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "snapshot", "table", "",
        "null", nullptr, 0,
        "", "",
        "PersistedDialogSnapshot", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ExportDialogResponse = {
    "ExportDialogResponse", "",
    "", "",
    false, false,
    kFields_Tryll_ExportDialogResponse, 3,
};
static const FTryllLogField kFields_Tryll_ExportMutableKbRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ExportMutableKbRequest = {
    "ExportMutableKbRequest", "",
    "", "",
    false, false,
    kFields_Tryll_ExportMutableKbRequest, 2,
};
static const FTryllLogField kFields_Tryll_ExportMutableKbResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "snapshot", "table", "",
        "null", nullptr, 0,
        "", "",
        "PersistedMutableKb", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ExportMutableKbResponse = {
    "ExportMutableKbResponse", "",
    "", "",
    false, false,
    kFields_Tryll_ExportMutableKbResponse, 2,
};
static const FTryllLogField kFields_Tryll_GameMemory[] = {
    {
        "format_version", "scalar", "",
        "1", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "saved_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "save_id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "stores", "vector", "",
        "null", nullptr, 0,
        "size", "",
        "PersistedMutableKb", "table", "PersistedMutableKb",
    },
};
static const FTryllLogTable kTable_Tryll_GameMemory = {
    "GameMemory", "",
    "", "",
    false, false,
    kFields_Tryll_GameMemory, 4,
};
static const FTryllLogField kFields_Tryll_GetAgentKvCacheStatusRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_GetAgentKvCacheStatusRequest = {
    "GetAgentKvCacheStatusRequest", "",
    "", "",
    false, false,
    kFields_Tryll_GetAgentKvCacheStatusRequest, 2,
};
static const char* kEnum_Tryll_GetAgentKvCacheStatusResponse_residency[] = { "Evicted", "Resident" };
static const char* kEnum_Tryll_GetAgentKvCacheStatusResponse_prefix_status[] = { "NotCurrent", "Current" };
static const FTryllLogField kFields_Tryll_GetAgentKvCacheStatusResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "applicable", "scalar", "",
        "true", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "residency", "enum", "",
        "Evicted", kEnum_Tryll_GetAgentKvCacheStatusResponse_residency, 2,
        "", "",
        "AgentKvCacheResidency", "", "",
    },
    {
        "prefix_status", "enum", "",
        "NotCurrent", kEnum_Tryll_GetAgentKvCacheStatusResponse_prefix_status, 2,
        "", "",
        "AgentKvCachePrefixStatus", "", "",
    },
    {
        "eligible_node_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "debug_info", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_GetAgentKvCacheStatusResponse = {
    "GetAgentKvCacheStatusResponse", "",
    "", "",
    false, false,
    kFields_Tryll_GetAgentKvCacheStatusResponse, 7,
};
static const FTryllLogField kFields_Tryll_GetAgentVariablesRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "names", "vector", "",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
};
static const FTryllLogTable kTable_Tryll_GetAgentVariablesRequest = {
    "GetAgentVariablesRequest", "",
    "", "",
    false, false,
    kFields_Tryll_GetAgentVariablesRequest, 3,
};
static const FTryllLogField kFields_Tryll_GetAgentVariablesResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "variables", "vector", "",
        "null", nullptr, 0,
        "", "",
        "AgentVariable", "table", "AgentVariable",
    },
};
static const FTryllLogTable kTable_Tryll_GetAgentVariablesResponse = {
    "GetAgentVariablesResponse", "",
    "", "",
    false, false,
    kFields_Tryll_GetAgentVariablesResponse, 2,
};
static const FTryllLogField kFields_Tryll_GetMemoryConsumptionRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_GetMemoryConsumptionRequest = {
    "GetMemoryConsumptionRequest", "",
    "", "",
    false, false,
    kFields_Tryll_GetMemoryConsumptionRequest, 1,
};
static const FTryllLogField kFields_Tryll_GetMemoryConsumptionResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "snapshot", "table", "",
        "null", nullptr, 0,
        "", "",
        "MemorySnapshot", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_GetMemoryConsumptionResponse = {
    "GetMemoryConsumptionResponse", "",
    "", "",
    false, false,
    kFields_Tryll_GetMemoryConsumptionResponse, 2,
};
static const FTryllLogField kFields_Tryll_GraphDescription[] = {
    {
        "nodes", "vector", "",
        "null", nullptr, 0,
        "", "",
        "NodeDescription", "table", "NodeDescription",
    },
    {
        "start_node", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_GraphDescription = {
    "GraphDescription", "",
    "", "",
    false, false,
    kFields_Tryll_GraphDescription, 2,
};
static const char* kEnum_Tryll_ImportDialogRequest_compatibility[] = { "Strict", "AllowGraphMismatch" };
static const FTryllLogField kFields_Tryll_ImportDialogRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "snapshot", "table", "",
        "", nullptr, 0,
        "", "",
        "PersistedDialogSnapshot", "", "",
    },
    {
        "compatibility", "enum", "",
        "Strict", kEnum_Tryll_ImportDialogRequest_compatibility, 2,
        "", "",
        "DialogCompatibility", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ImportDialogRequest = {
    "ImportDialogRequest", "",
    "", "",
    false, false,
    kFields_Tryll_ImportDialogRequest, 4,
};
static const FTryllLogField kFields_Tryll_ImportDialogResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "interaction_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "warnings", "vector", "",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
};
static const FTryllLogTable kTable_Tryll_ImportDialogResponse = {
    "ImportDialogResponse", "",
    "", "",
    false, false,
    kFields_Tryll_ImportDialogResponse, 4,
};
static const char* kEnum_Tryll_ImportMutableKbRequest_compatibility[] = { "Strict", "AllowSchemaDrift" };
static const FTryllLogField kFields_Tryll_ImportMutableKbRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "snapshot", "table", "",
        "", nullptr, 0,
        "", "",
        "PersistedMutableKb", "", "",
    },
    {
        "compatibility", "enum", "",
        "Strict", kEnum_Tryll_ImportMutableKbRequest_compatibility, 2,
        "", "",
        "MutableKbCompatibility", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ImportMutableKbRequest = {
    "ImportMutableKbRequest", "",
    "", "",
    false, false,
    kFields_Tryll_ImportMutableKbRequest, 3,
};
static const FTryllLogField kFields_Tryll_ImportMutableKbResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "warnings", "vector", "",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
};
static const FTryllLogTable kTable_Tryll_ImportMutableKbResponse = {
    "ImportMutableKbResponse", "",
    "", "",
    false, false,
    kFields_Tryll_ImportMutableKbResponse, 3,
};
static const FTryllLogField kFields_Tryll_ListModelsRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ListModelsRequest = {
    "ListModelsRequest", "",
    "", "",
    false, false,
    kFields_Tryll_ListModelsRequest, 1,
};
static const FTryllLogField kFields_Tryll_ListModelsResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "models", "vector", "",
        "null", nullptr, 0,
        "", "",
        "ModelInfo", "table", "ModelInfo",
    },
};
static const FTryllLogTable kTable_Tryll_ListModelsResponse = {
    "ListModelsResponse", "",
    "", "",
    false, false,
    kFields_Tryll_ListModelsResponse, 2,
};
static const char* kEnum_Tryll_LoadDialogRequest_compatibility[] = { "Strict", "AllowGraphMismatch" };
static const FTryllLogField kFields_Tryll_LoadDialogRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "path", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "compatibility", "enum", "",
        "Strict", kEnum_Tryll_LoadDialogRequest_compatibility, 2,
        "", "",
        "DialogCompatibility", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_LoadDialogRequest = {
    "LoadDialogRequest", "",
    "", "",
    false, false,
    kFields_Tryll_LoadDialogRequest, 4,
};
static const FTryllLogField kFields_Tryll_LoadDialogResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "interaction_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "warnings", "vector", "",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
};
static const FTryllLogTable kTable_Tryll_LoadDialogResponse = {
    "LoadDialogResponse", "",
    "", "",
    false, false,
    kFields_Tryll_LoadDialogResponse, 4,
};
static const char* kEnum_Tryll_LoadGameMemoryRequest_compatibility[] = { "Strict", "AllowSchemaDrift" };
static const char* kEnum_Tryll_LoadGameMemoryRequest_embed_timing[] = { "Unspecified", "Now", "Lazy" };
static const FTryllLogField kFields_Tryll_LoadGameMemoryRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "path", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "compatibility", "enum", "",
        "Strict", kEnum_Tryll_LoadGameMemoryRequest_compatibility, 2,
        "", "",
        "MutableKbCompatibility", "", "",
    },
    {
        "embed_timing", "enum", "",
        "Unspecified", kEnum_Tryll_LoadGameMemoryRequest_embed_timing, 3,
        "", "",
        "MutableKbEmbedTiming", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_LoadGameMemoryRequest = {
    "LoadGameMemoryRequest", "",
    "", "",
    false, false,
    kFields_Tryll_LoadGameMemoryRequest, 4,
};
static const FTryllLogField kFields_Tryll_LoadGameMemoryResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "store_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "warnings", "vector", "",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
};
static const FTryllLogTable kTable_Tryll_LoadGameMemoryResponse = {
    "LoadGameMemoryResponse", "",
    "", "",
    false, false,
    kFields_Tryll_LoadGameMemoryResponse, 3,
};
static const char* kEnum_Tryll_LoadModelRequest_placement[] = { "Auto", "Ram", "Vram" };
static const FTryllLogField kFields_Tryll_LoadModelRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "model_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "placement", "enum", "Where to put this model, for this load only — the top of the placement precedence chain, above the session's per-model hint. This is what lets an editor's Placement dropdown work without tearing down a long-lived shared session that other panels are using. A hint: an unsatisfiable value is downgraded, never an error. Read the result from LoadModelResponse. Only consulted when `placement_is_override` is true. Without that flag `Auto` would have to mean both \"put it wherever the host prefers\" and \"use whatever the session declared\", and those are different requests — an editor could then never move a model back to host policy once the session had been created with a Ram or Vram hint for it.",
        "Auto", kEnum_Tryll_LoadModelRequest_placement, 3,
        "", "",
        "MemoryPlacement", "", "",
    },
    {
        "placement_is_override", "scalar", "True when `placement` is a deliberate choice for this load. False (the default) means \"no opinion — use the session's per-model hint\", which is what a client that does not care about placement sends.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_LoadModelRequest = {
    "LoadModelRequest", "",
    "", "",
    false, false,
    kFields_Tryll_LoadModelRequest, 4,
};
static const char* kEnum_Tryll_LoadModelResponse_placement_resolved[] = { "Auto", "Ram", "Vram" };
static const FTryllLogField kFields_Tryll_LoadModelResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "placement_resolved", "enum", "Where the model actually ended up. **Not** an echo of the request: • `Auto` always resolves to a concrete `Ram` or `Vram` here; • an unsatisfiable `Vram` reports `Ram` (no GPU backend, or a CPU-only Sherpa ONNX Runtime build); • a model already resident under a different placement reports the resident value, because placement is fixed at load and deliberately is not part of the model cache key — first loader wins. Echoing the request instead would give a client a UI that confidently displays a device the model is not on.",
        "Auto", kEnum_Tryll_LoadModelResponse_placement_resolved, 3,
        "", "",
        "MemoryPlacement", "", "",
    },
    {
        "backend", "string", "Backend that served the load: \"vulkan\" | \"cuda\" | \"rocm\" | \"cpu\" for llama.cpp, or the ONNX Runtime provider for Sherpa. Read-only — a client never chooses this. Which GPU API is available is a host decision: it is process-wide and one-shot for llama.cpp and compile-time for Sherpa. Empty when the load did not resolve a backend.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_LoadModelResponse = {
    "LoadModelResponse", "",
    "", "",
    false, false,
    kFields_Tryll_LoadModelResponse, 3,
};
static const char* kEnum_Tryll_LogCaptureGap_reason[] = { "paused", "epoch_change", "reload_discard" };
static const FTryllLogField kFields_Tryll_LogCaptureGap[] = {
    {
        "reason", "enum", "",
        "paused", kEnum_Tryll_LogCaptureGap_reason, 3,
        "", "",
        "LogGapReason", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_LogCaptureGap = {
    "LogCaptureGap", "",
    "", "",
    false, false,
    kFields_Tryll_LogCaptureGap, 1,
};
static const char* kEnum_Tryll_LogConnectionState_state[] = { "connected", "failed", "dropped" };
static const FTryllLogField kFields_Tryll_LogConnectionState[] = {
    {
        "state", "enum", "",
        "connected", kEnum_Tryll_LogConnectionState_state, 3,
        "", "",
        "LogConnState", "", "",
    },
    {
        "reason", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_LogConnectionState = {
    "LogConnectionState", "",
    "", "",
    false, false,
    kFields_Tryll_LogConnectionState, 2,
};
static const char* kEnum_Tryll_LogLocalError_origin[] = { "transport", "unexpected_reply", "client_validation" };
static const FTryllLogField kFields_Tryll_LogLocalError[] = {
    {
        "code", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "code_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "message", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "origin", "enum", "",
        "transport", kEnum_Tryll_LogLocalError_origin, 3,
        "", "",
        "LogErrorOrigin", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_LogLocalError = {
    "LogLocalError", "",
    "", "",
    false, false,
    kFields_Tryll_LogLocalError, 4,
};
static const char* kEnum_Tryll_MemoryComponent_owner[] = { "Process", "Device", "Model", "AgentNode", "Storage", "Unattributed" };
static const char* kEnum_Tryll_MemoryComponent_tier[] = { "Measured", "Derived", "Estimated" };
static const char* kEnum_Tryll_MemoryComponent_consumer_class[] = { "Unspecified", "InteractiveVisible", "InteractiveBuffered", "Background" };
static const FTryllLogField kFields_Tryll_MemoryComponent[] = {
    {
        "kind", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "owner", "enum", "",
        "Model", kEnum_Tryll_MemoryComponent_owner, 6,
        "", "",
        "MemoryOwnerKind", "", "",
    },
    {
        "tier", "enum", "",
        "Measured", kEnum_Tryll_MemoryComponent_tier, 3,
        "", "",
        "MemoryTier", "", "",
    },
    {
        "estimate_basis", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "ram_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "vram_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "ram_bytes_high", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "device_index", "scalar", "",
        "-1", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "per_device", "vector", "",
        "null", nullptr, 0,
        "", "",
        "MemoryComponentDevice", "table", "MemoryComponentDevice",
    },
    {
        "session_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "node_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "used_tokens", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "capacity_tokens", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "resident", "scalar", "",
        "true", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "occupancy_stale", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "compute_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "consumer_class", "enum", "",
        "Unspecified", kEnum_Tryll_MemoryComponent_consumer_class, 4,
        "", "",
        "InferenceConsumerClass", "", "",
    },
    {
        "last_turn_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MemoryComponent = {
    "MemoryComponent", "",
    "", "",
    true, false,
    kFields_Tryll_MemoryComponent, 20,
};
static const FTryllLogField kFields_Tryll_MemoryComponentDevice[] = {
    {
        "device_index", "scalar", "",
        "-1", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "compute_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MemoryComponentDevice = {
    "MemoryComponentDevice", "",
    "", "",
    false, false,
    kFields_Tryll_MemoryComponentDevice, 3,
};
static const FTryllLogField kFields_Tryll_MemoryDevice[] = {
    {
        "index", "scalar", "",
        "-1", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "description", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "kind", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "total_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "free_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "self_model_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "self_context_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "self_compute_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "other_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MemoryDevice = {
    "MemoryDevice", "",
    "", "",
    false, false,
    kFields_Tryll_MemoryDevice, 10,
};
static const FTryllLogField kFields_Tryll_MemorySnapshot[] = {
    {
        "captured_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "process_working_set_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "process_private_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "process_available", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "backend_initialized", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "unattributed_ram_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "devices", "vector", "",
        "null", nullptr, 0,
        "", "",
        "MemoryDevice", "table", "MemoryDevice",
    },
    {
        "components", "vector", "",
        "null", nullptr, 0,
        "", "",
        "MemoryComponent", "table", "MemoryComponent",
    },
};
static const FTryllLogTable kTable_Tryll_MemorySnapshot = {
    "MemorySnapshot", "",
    "", "",
    false, false,
    kFields_Tryll_MemorySnapshot, 8,
};
static const char* kEnum_Tryll_ModelDependency_status[] = { "Absent", "Local", "Downloading", "Loaded", "Downloaded", "Incomplete" };
static const FTryllLogField kFields_Tryll_ModelDependency[] = {
    {
        "id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "role", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "status", "enum", "",
        "Absent", kEnum_Tryll_ModelDependency_status, 6,
        "", "",
        "ModelStatus", "", "",
    },
    {
        "size_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "folder", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "local_path", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "failure_reason", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ModelDependency = {
    "ModelDependency", "",
    "", "",
    false, false,
    kFields_Tryll_ModelDependency, 7,
};
static const char* kEnum_Tryll_ModelInfo_status[] = { "Absent", "Local", "Downloading", "Loaded", "Downloaded", "Incomplete" };
static const FTryllLogField kFields_Tryll_ModelInfo[] = {
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "status", "enum", "",
        "Absent", kEnum_Tryll_ModelInfo_status, 6,
        "", "",
        "ModelStatus", "", "",
    },
    {
        "huggingface_repo", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "size_bytes", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "model_type", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "engine", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "audience", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "tool_call_format", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context_size", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "files", "vector", "",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
    {
        "default_sampling", "table", "",
        "null", nullptr, 0,
        "", "",
        "ModelSampling", "", "",
    },
    {
        "local_path", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "tool_call_support", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "languages", "vector", "",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
    {
        "dependencies", "vector", "",
        "null", nullptr, 0,
        "", "",
        "ModelDependency", "table", "ModelDependency",
    },
    {
        "tts_capabilities", "table", "",
        "null", nullptr, 0,
        "", "",
        "TtsCapabilitiesInfo", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ModelInfo = {
    "ModelInfo", "",
    "", "",
    false, false,
    kFields_Tryll_ModelInfo, 16,
};
static const char* kEnum_Tryll_ModelPlacementHint_placement[] = { "Auto", "Ram", "Vram" };
static const char* kEnum_Tryll_ModelPlacementHint_hotwords[] = { "Default", "Off", "On" };
static const FTryllLogField kFields_Tryll_ModelPlacementHint[] = {
    {
        "model_name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "placement", "enum", "",
        "Auto", kEnum_Tryll_ModelPlacementHint_placement, 3,
        "", "",
        "MemoryPlacement", "", "",
    },
    {
        "hotwords", "enum", "Whether this session wants hotword biasing on this STT model. Overrides the catalog's `stt_hotwords_score`; `Default` follows it.",
        "Default", kEnum_Tryll_ModelPlacementHint_hotwords, 3,
        "", "",
        "HotwordBias", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ModelPlacementHint = {
    "ModelPlacementHint", "",
    "", "",
    false, false,
    kFields_Tryll_ModelPlacementHint, 3,
};
static const FTryllLogField kFields_Tryll_ModelSampling[] = {
    {
        "temperature", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "top_p", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "top_k", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "min_p", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "max_tokens", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "seed", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "repeat_penalty", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "presence_penalty", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "frequency_penalty", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ModelSampling = {
    "ModelSampling", "",
    "", "",
    false, false,
    kFields_Tryll_ModelSampling, 9,
};
static const FTryllLogField kFields_Tryll_MutableKbClearRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbClearRequest = {
    "MutableKbClearRequest", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbClearRequest, 2,
};
static const FTryllLogField kFields_Tryll_MutableKbClearResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "cleared_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbClearResponse = {
    "MutableKbClearResponse", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbClearResponse, 2,
};
static const FTryllLogField kFields_Tryll_MutableKbDeleteRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "ids", "vector", "",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbDeleteRequest = {
    "MutableKbDeleteRequest", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbDeleteRequest, 3,
};
static const FTryllLogField kFields_Tryll_MutableKbDeleteResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "deleted_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbDeleteResponse = {
    "MutableKbDeleteResponse", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbDeleteResponse, 2,
};
static const FTryllLogField kFields_Tryll_MutableKbGetRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "id", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbGetRequest = {
    "MutableKbGetRequest", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbGetRequest, 3,
};
static const FTryllLogField kFields_Tryll_MutableKbGetResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "found", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "created_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "updated_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbGetResponse = {
    "MutableKbGetResponse", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbGetResponse, 6,
};
static const FTryllLogField kFields_Tryll_MutableKbHit[] = {
    {
        "id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "distance", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbHit = {
    "MutableKbHit", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbHit, 3,
};
static const FTryllLogField kFields_Tryll_MutableKbInfo[] = {
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "schema", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "live_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "write_generation", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "materialized", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "bytes_estimate", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbInfo = {
    "MutableKbInfo", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbInfo, 6,
};
static const FTryllLogField kFields_Tryll_MutableKbListRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "offset", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "limit", "scalar", "",
        "50", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbListRequest = {
    "MutableKbListRequest", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbListRequest, 4,
};
static const FTryllLogField kFields_Tryll_MutableKbListResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "records", "vector", "",
        "null", nullptr, 0,
        "", "",
        "MutableKbRecord", "table", "MutableKbRecord",
    },
    {
        "next_offset", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "has_more", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbListResponse = {
    "MutableKbListResponse", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbListResponse, 4,
};
static const char* kEnum_Tryll_MutableKbQueryRequest_retrieval_mode[] = { "Dense", "Lexical", "Hybrid" };
static const FTryllLogField kFields_Tryll_MutableKbQueryRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "query_text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "top_k", "scalar", "",
        "4", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "retrieval_mode", "enum", "",
        "Dense", kEnum_Tryll_MutableKbQueryRequest_retrieval_mode, 3,
        "", "",
        "RetrievalMode", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbQueryRequest = {
    "MutableKbQueryRequest", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbQueryRequest, 5,
};
static const FTryllLogField kFields_Tryll_MutableKbQueryResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "hits", "vector", "",
        "null", nullptr, 0,
        "", "",
        "MutableKbHit", "table", "MutableKbHit",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbQueryResponse = {
    "MutableKbQueryResponse", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbQueryResponse, 2,
};
static const FTryllLogField kFields_Tryll_MutableKbRecord[] = {
    {
        "id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "created_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "updated_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbRecord = {
    "MutableKbRecord", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbRecord, 4,
};
static const FTryllLogField kFields_Tryll_MutableKbStatsRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbStatsRequest = {
    "MutableKbStatsRequest", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbStatsRequest, 2,
};
static const FTryllLogField kFields_Tryll_MutableKbStatsResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "live_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "tombstoned_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "slot_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "write_generation", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "bytes_estimate", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "materialized", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbStatsResponse = {
    "MutableKbStatsResponse", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbStatsResponse, 7,
};
static const FTryllLogField kFields_Tryll_MutableKbUpsert[] = {
    {
        "id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbUpsert = {
    "MutableKbUpsert", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbUpsert, 2,
};
static const FTryllLogField kFields_Tryll_MutableKbWriteRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "schema", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "records", "vector", "",
        "null", nullptr, 0,
        "", "",
        "MutableKbUpsert", "table", "MutableKbUpsert",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbWriteRequest = {
    "MutableKbWriteRequest", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbWriteRequest, 4,
};
static const FTryllLogField kFields_Tryll_MutableKbWriteResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "results", "vector", "",
        "null", nullptr, 0,
        "", "",
        "MutableKbWriteResult", "table", "MutableKbWriteResult",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbWriteResponse = {
    "MutableKbWriteResponse", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbWriteResponse, 2,
};
static const FTryllLogField kFields_Tryll_MutableKbWriteResult[] = {
    {
        "id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "inserted", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "accepted", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "error", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_MutableKbWriteResult = {
    "MutableKbWriteResult", "",
    "", "",
    false, false,
    kFields_Tryll_MutableKbWriteResult, 4,
};
static const char* kEnum_Tryll_NodeDescription_params[] = { "NONE", "GenerateParams", "RegexGuardrailParams", "CannedResponseParams", "ToolCallParams", "RetrieveParams", "InstructionParams", "ClassifyIntentParams", "ClassifyIntentLLMParams", "IntentToInstructionParams", "GenerateAndSpeakParams", "SpeakParams", "PauseParams", "TransformParams", "BranchParams", "WriteToMemoryParams", "SummarizeParams", "ClassifyAttributesLLMParams" };
static const FTryllLogField kFields_Tryll_NodeDescription[] = {
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "params", "union", "",
        "null", kEnum_Tryll_NodeDescription_params, 18,
        "", "",
        "NodeParams", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeDescription = {
    "NodeDescription", "",
    "", "",
    false, false,
    kFields_Tryll_NodeDescription, 2,
};
static const FTryllLogField kFields_Tryll_NodeEvent[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "node_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "event_type", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "kv_pairs", "vector", "",
        "null", nullptr, 0,
        "", "",
        "NodeParam", "table", "NodeParam",
    },
};
static const FTryllLogTable kTable_Tryll_NodeEvent = {
    "NodeEvent", "{event_type} · {node_name}",
    "", "",
    true, false,
    kFields_Tryll_NodeEvent, 5,
};
static const FTryllLogField kFields_Tryll_NodeParam[] = {
    {
        "key", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "value", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParam = {
    "NodeParam", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParam, 2,
};
static const char* kEnum_Tryll_NodeParams_BranchParams_test[] = { "NonEmpty", "IsTrue", "Equals", "EqualsIgnoreCase", "Contains", "Regex", "Expression" };
static const FTryllLogField kFields_Tryll_NodeParams_BranchParams[] = {
    {
        "test", "enum", "What to check: NonEmpty, IsTrue (bool Variable), Equals, EqualsIgnoreCase, Contains, Regex (against Values), or Expression (Condition).",
        "NonEmpty", kEnum_Tryll_NodeParams_BranchParams_test, 7,
        "", "",
        "BranchTest", "", "",
    },
    {
        "input", "string", "Text to test: the player's message or an earlier node's output. Empty = the player's message. Leave empty when Variable is set.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "variable", "string", "Agent variable to test instead of Input. Leave empty to test Input.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "values", "vector", "Values for Equals, EqualsIgnoreCase, Contains and Regex; the test passes if any one matches.",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
    {
        "condition", "string", "JSON condition for the Expression test, over agent variables and {\"slot\":\"<name>\"} values. Unused by other tests.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "notify_client", "scalar", "Send the game a branch_taken event with the exit taken.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "then_exit", "string", "Node to run when the test passes. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "else_exit", "string", "Node to run when the test fails or the value to test is missing. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_BranchParams = {
    "BranchParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_BranchParams, 8,
};
static const char* kEnum_Tryll_NodeParams_CannedResponseParams_selection_strategy[] = { "Random", "First", "RoundRobin" };
static const char* kEnum_Tryll_NodeParams_CannedResponseParams_send[] = { "None", "Whole", "Streamed" };
static const char* kEnum_Tryll_NodeParams_CannedResponseParams_history_role[] = { "None", "Assistant", "User", "System" };
static const FTryllLogField kFields_Tryll_NodeParams_CannedResponseParams[] = {
    {
        "string_storage", "string", "File of responses, one per line. Used instead of Lines. Both empty = the server's default responses.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "selection_strategy", "enum", "Which response comes next: Random, First (always the first), or RoundRobin (each in turn).",
        "RoundRobin", kEnum_Tryll_NodeParams_CannedResponseParams_selection_strategy, 3,
        "", "",
        "CannedResponseSelectionStrategy", "", "",
    },
    {
        "output_name", "string", "Name later nodes use to read the chosen response (in their Input, or as the slot.<name> tag). Empty = this node's name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "send", "enum", "How the response reaches the game: Whole (in one piece), Streamed, or None (kept for later nodes only).",
        "Whole", kEnum_Tryll_NodeParams_CannedResponseParams_send, 3,
        "", "",
        "SendAnswer", "", "",
    },
    {
        "history_role", "enum", "Whether later turns remember this response as an assistant message. None = forgotten after this turn.",
        "Assistant", kEnum_Tryll_NodeParams_CannedResponseParams_history_role, 4,
        "", "",
        "HistoryRole", "", "",
    },
    {
        "default_exit", "string", "Node to run after the response. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "lines", "vector", "Responses typed here; used when String Storage is empty.",
        "null", nullptr, 0,
        "", "",
        "LocalizableText", "table", "LocalizableText",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_CannedResponseParams = {
    "CannedResponseParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_CannedResponseParams, 7,
};
static const FTryllLogField kFields_Tryll_NodeParams_ClassifyAttributeChoice[] = {
    {
        "id", "string", "Value written to the output when this option wins. The model never sees it. Must be unique within the attribute.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "prompt", "string", "Description of the option the model reads, listed as \"A. <prompt>\".",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "triggers", "scalar", "Choice only. When this option is the answer, the node takes its Triggered exit. Leave off for Score attributes.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ClassifyAttributeChoice = {
    "ClassifyAttributeChoice", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ClassifyAttributeChoice, 3,
};
static const char* kEnum_Tryll_NodeParams_ClassifyAttributeDef_kind[] = { "Choice", "Boolean", "Score" };
static const char* kEnum_Tryll_NodeParams_ClassifyAttributeDef_boolean_trigger[] = { "Never", "True", "False" };
static const char* kEnum_Tryll_NodeParams_ClassifyAttributeDef_score_trigger[] = { "Never", "AtOrAbove", "Below" };
static const FTryllLogField kFields_Tryll_NodeParams_ClassifyAttributeDef[] = {
    {
        "name", "string", "Attribute name; the answer is written to <Output Name>_<Name>. Must be unique within the node.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "kind", "enum", "Choice: picks one option and writes its id. Boolean: decides one Statement, writes true or false. Score: writes a 0–1 level.",
        "Choice", kEnum_Tryll_NodeParams_ClassifyAttributeDef_kind, 3,
        "", "",
        "AttributeKind", "", "",
    },
    {
        "choices", "vector", "Choice and Score only: the options, 2–26. For Score, order them from lowest to highest.",
        "null", nullptr, 0,
        "", "",
        "ClassifyAttributeChoice", "table", "ClassifyAttributeChoice",
    },
    {
        "statement", "string", "Boolean only. The statement judged true or false for the input.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "system_prompt", "string", "Instructions for this attribute. Empty = a built-in default. Choice and Score must include the intents_block tag; Boolean must include the statement tag.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "threshold", "scalar", "Choice: minimum probability of the winner. Boolean: probability of True needed to answer true.",
        "0.5", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "margin", "scalar", "Choice only. Minimum lead of the winner over the runner-up.",
        "0.15", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "uncertain_choice", "string", "Choice only. Answer written when the winner is too weak: a choice id or #uncertain. A choice id fires Triggered if that choice does.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "boolean_trigger", "enum", "Boolean only. Which answer fires the node's Triggered exit: Never, True or False.",
        "Never", kEnum_Tryll_NodeParams_ClassifyAttributeDef_boolean_trigger, 3,
        "", "",
        "BooleanTrigger", "", "",
    },
    {
        "score_trigger", "enum", "Score only. Fire the node's Triggered exit when the score is AtOrAbove or Below Score Threshold, or Never.",
        "Never", kEnum_Tryll_NodeParams_ClassifyAttributeDef_score_trigger, 3,
        "", "",
        "ScoreTrigger", "", "",
    },
    {
        "score_threshold", "scalar", "Score only. Level (0–1) that Score Trigger compares against.",
        "0.5", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ClassifyAttributeDef = {
    "ClassifyAttributeDef", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ClassifyAttributeDef, 11,
};
static const char* kEnum_Tryll_NodeParams_ClassifyAttributesLLMParams_on_failure[] = { "Default", "Triggered" };
static const char* kEnum_Tryll_NodeParams_ClassifyAttributesLLMParams_notify_client[] = { "Disabled", "OnFound", "Always" };
static const FTryllLogField kFields_Tryll_NodeParams_ClassifyAttributesLLMParams[] = {
    {
        "model_name", "string", "Language model that scores the attributes. Empty = the workflow's Default Model Name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context_size", "scalar", "Context window for this node, in tokens. 0 = sized automatically from the attribute prompts.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "attributes", "vector", "Questions to classify, 1–16, all scored in one pass. Each writes its answer to <Output Name>_<attribute Name>.",
        "null", nullptr, 0,
        "", "",
        "ClassifyAttributeDef", "table", "ClassifyAttributeDef",
    },
    {
        "input", "string", "Text to classify: the player's message or an earlier node's output. Empty = the player's message.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "history_turns", "scalar", "How many earlier dialog turns the classifier sees. 0 = the current message only.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "temperature", "scalar", "Sharpens (below 1) or flattens (above 1) every attribute's probabilities before its thresholds. 0 = unchanged.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "on_failure", "enum", "Exit to take if scoring fails: Default (fail open) or Triggered (fail closed).",
        "Default", kEnum_Tryll_NodeParams_ClassifyAttributesLLMParams_on_failure, 2,
        "", "",
        "AttributeFailureRoute", "", "",
    },
    {
        "notify_client", "enum", "When to send the game an attributes_classified event: Disabled, OnFound (Triggered exit only), or Always.",
        "Disabled", kEnum_Tryll_NodeParams_ClassifyAttributesLLMParams_notify_client, 3,
        "", "",
        "NotifyClient", "", "",
    },
    {
        "output_name", "string", "Prefix of this node's results: each attribute writes <Output Name>_<attribute Name>. Empty = this node's name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "default_exit", "string", "Node to run when no attribute fires. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "triggered_exit", "string", "Node to run when any attribute fires (or scoring fails and On Failure is Triggered). Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context", "table", "Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit.",
        "null", nullptr, 0,
        "", "",
        "ContextOverrides", "", "",
    },
    {
        "adaptation", "table", "LoRA adapters applied to this node's model. Empty = the base model.",
        "null", nullptr, 0,
        "", "",
        "ModelAdaptation", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ClassifyAttributesLLMParams = {
    "ClassifyAttributesLLMParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ClassifyAttributesLLMParams, 13,
};
static const FTryllLogField kFields_Tryll_NodeParams_ClassifyIntentEntry[] = {
    {
        "id", "string", "Label written to the output when this intent wins. The model never sees it. Must be unique among the intents.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "prompt", "string", "Description of the intent the model reads, listed as \"A. <prompt>\", \"B. <prompt>\", … in Intents order.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ClassifyIntentEntry = {
    "ClassifyIntentEntry", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ClassifyIntentEntry, 2,
};
static const char* kEnum_Tryll_NodeParams_ClassifyIntentLLMParams_notify_client[] = { "Disabled", "OnFound", "Always" };
static const FTryllLogField kFields_Tryll_NodeParams_ClassifyIntentLLMParams[] = {
    {
        "model_name", "string", "Language model that scores the intents. Empty = the workflow's Default Model Name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context_size", "scalar", "Context window for this node, in tokens; must fit the model's trained maximum. 0 = 2048.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "system_prompt", "string", "Instructions for the classifier. Must include the intents_block tag (the lettered intent list); the letters tag (\"A, B, C\") is optional.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "intents", "vector", "The intents to choose from, 2–26. Each has an id (the label written to the output) and a description the model reads.",
        "null", nullptr, 0,
        "", "",
        "ClassifyIntentEntry", "table", "ClassifyIntentEntry",
    },
    {
        "input", "string", "Text to classify: the player's message or an earlier node's output. Empty = the player's message.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "output_name", "string", "Name later nodes use to read the result: the winning intent id, or Not Found Intent. Empty = this node's name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "history_turns", "scalar", "How many earlier dialog turns the classifier sees. 0 = the current message only.",
        "2", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "threshold", "scalar", "Minimum probability (0–1) the winning intent needs; below it = Not Found.",
        "0.5", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "margin", "scalar", "Minimum lead (0–1) of the winner over the runner-up; less = Not Found.",
        "0.15", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "temperature", "scalar", "Sharpens (below 1) or flattens (above 1) the probabilities before Threshold and Margin. 0 = unchanged.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "notify_client", "enum", "When to send the game an intent_llm_classified event: Disabled, OnFound (Found exit only), or Always.",
        "Disabled", kEnum_Tryll_NodeParams_ClassifyIntentLLMParams_notify_client, 3,
        "", "",
        "NotifyClient", "", "",
    },
    {
        "not_found_intent", "string", "Intent id that means \"none of these\": if it wins, take Not Found. Also the output on Not Found. May be #uncertain or empty.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "found_exit", "string", "Node to run when an intent wins clearly (passes Threshold and Margin). Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "not_found_exit", "string", "Node to run when no intent wins clearly. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context", "table", "Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit.",
        "null", nullptr, 0,
        "", "",
        "ContextOverrides", "", "",
    },
    {
        "adaptation", "table", "LoRA adapters applied to this node's model. Empty = the base model.",
        "null", nullptr, 0,
        "", "",
        "ModelAdaptation", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ClassifyIntentLLMParams = {
    "ClassifyIntentLLMParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ClassifyIntentLLMParams, 16,
};
static const char* kEnum_Tryll_NodeParams_ClassifyIntentParams_notify_client[] = { "Disabled", "OnFound", "Always" };
static const FTryllLogField kFields_Tryll_NodeParams_ClassifyIntentParams[] = {
    {
        "embedded_string_storage", "string", "Embedded file of example phrases, each labelled with its intent.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "input", "string", "Text to classify: the player's message or an earlier node's output. Empty = the player's message.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "metadata_field", "string", "Metadata field in that file that holds each example's intent label.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "threshold", "scalar", "Max cosine distance of the closest example (0 = identical, 2 = opposite). Farther = Not Found.",
        "0.25", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "filter", "string", "JSON filter on example metadata; only matching examples are compared. Empty = no filter.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "notify_client", "enum", "When to send the game an intent_classified event: Disabled, OnFound (Found exit only), or Always.",
        "Disabled", kEnum_Tryll_NodeParams_ClassifyIntentParams_notify_client, 3,
        "", "",
        "NotifyClient", "", "",
    },
    {
        "diagnostic_topk", "scalar", "How many closest examples to report in diagnostics. Routing always uses the closest one.",
        "5", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "found_exit", "string", "Node to run when the closest example is within Threshold. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "not_found_exit", "string", "Node to run when no example is close enough. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ClassifyIntentParams = {
    "ClassifyIntentParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ClassifyIntentParams, 9,
};
static const char* kEnum_Tryll_NodeParams_ContextOverrides_kv_cache_type[] = { "Inherit", "F16", "Q8_0", "Q4_0" };
static const FTryllLogField kFields_Tryll_NodeParams_ContextOverrides[] = {
    {
        "kv_cache_type", "enum", "Number format of the context memory (KV cache). Smaller types save memory at some quality cost. Inherit = the model's setting, else q8_0.",
        "Inherit", kEnum_Tryll_NodeParams_ContextOverrides_kv_cache_type, 4,
        "", "",
        "KvCacheType", "", "",
    },
    {
        "offload_kqv", "scalar", "Keep the context memory on the GPU. Can only turn it off, not on for a model placed in RAM. Unset = server and model placement decide.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "n_ubatch", "scalar", "Tokens processed per GPU step; sizes the working buffers. Unset = the model's setting.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "n_outputs_max", "scalar", "Leave unset. Advanced: how many positions return scores per step.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "n_batch", "scalar", "Most tokens sent to the model in one call while reading the prompt. Unset = the model's setting.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ContextOverrides = {
    "ContextOverrides", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ContextOverrides, 5,
};
static const char* kEnum_Tryll_NodeParams_GenerateAndSpeakParams_placement[] = { "BeforeUserAsUser", "BeforeUserAsSystem", "AfterUserAsUser", "AfterUserAsSystem" };
static const char* kEnum_Tryll_NodeParams_GenerateAndSpeakParams_history_role[] = { "None", "Assistant", "User", "System" };
static const FTryllLogField kFields_Tryll_NodeParams_GenerateAndSpeakParams[] = {
    {
        "model_name", "string", "Language model this node runs. Empty = the workflow's Default Model Name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context_size", "scalar", "Context window for this node, in tokens; must fit the model's trained maximum. 0 = the model's own size, else the server default.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "system_prompt", "string", "System message the model reads before the dialog. Empty = none.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "input", "string", "Text this node answers: the player's message or an earlier node's output. Empty = the player's message.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "template", "string", "Mustache text added next to the Input (where: see Placement), for example the slot.<name> tag or retrieved knowledge. Empty = none.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "placement", "enum", "Where Template goes: before or after the Input, as a system or a user message.",
        "BeforeUserAsSystem", kEnum_Tryll_NodeParams_GenerateAndSpeakParams_placement, 4,
        "", "",
        "Placement", "", "",
    },
    {
        "output_name", "string", "Name later nodes use to read this node's answer (in their Input, or as the slot.<name> tag). Empty = this node's name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "history_role", "enum", "Whether later turns remember this answer as an assistant message. None = forgotten after this turn.",
        "Assistant", kEnum_Tryll_NodeParams_GenerateAndSpeakParams_history_role, 4,
        "", "",
        "HistoryRole", "", "",
    },
    {
        "sampling", "table", "Overrides for the model's sampling settings (temperature, top-p, max tokens…). Unset values keep the model's own.",
        "null", nullptr, 0,
        "", "",
        "SamplingOverrides", "", "",
    },
    {
        "grammar", "string", "GBNF grammar the spoken answer must follow; needs a `root` rule. Empty = free text. Use it to limit speech to a fixed set of lines.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "min_sentence_chars", "scalar", "Minimum length of a spoken chunk before a sentence end cuts it (CJK characters count double). Lower = speech starts sooner, more TTS calls.",
        "12", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "first_chunk_max_chars", "scalar", "Lets the first chunk end early, at a comma, dash or space once it is this long, so speech starts sooner. 0 = off; try 48.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "default_exit", "string", "Node to run after the answer is generated and spoken. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "substitute_agent_variables", "scalar", "Replace __NAME__ markers in the answer with the values of agent variables that allow output substitution. Not case-sensitive.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "output_filter", "table", "Removes roleplay artifacts before they are spoken: \"Bob:\" prefixes, *actions*, (asides), [notes]. All off by default.",
        "null", nullptr, 0,
        "", "",
        "OutputFilterOverrides", "", "",
    },
    {
        "context", "table", "Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit.",
        "null", nullptr, 0,
        "", "",
        "ContextOverrides", "", "",
    },
    {
        "adaptation", "table", "LoRA adapters applied to this node's model. Empty = the base model.",
        "null", nullptr, 0,
        "", "",
        "ModelAdaptation", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_GenerateAndSpeakParams = {
    "GenerateAndSpeakParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_GenerateAndSpeakParams, 17,
};
static const char* kEnum_Tryll_NodeParams_GenerateParams_placement[] = { "BeforeUserAsUser", "BeforeUserAsSystem", "AfterUserAsUser", "AfterUserAsSystem" };
static const char* kEnum_Tryll_NodeParams_GenerateParams_send[] = { "None", "Whole", "Streamed" };
static const char* kEnum_Tryll_NodeParams_GenerateParams_history_role[] = { "None", "Assistant", "User", "System" };
static const FTryllLogField kFields_Tryll_NodeParams_GenerateParams[] = {
    {
        "model_name", "string", "Language model this node runs. Empty = the workflow's Default Model Name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context_size", "scalar", "Context window for this node, in tokens; must fit the model's trained maximum. 0 = the model's own size, else the server default.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "system_prompt", "string", "System message the model reads before the dialog. Empty = none.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "output_name", "string", "Name later nodes use to read this node's answer (in their Input, or as the slot.<name> tag). Empty = this node's name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "input", "string", "Text this node answers: the player's message or an earlier node's output. Empty = the player's message.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "template", "string", "Mustache text added next to the Input (where: see Placement), for example the slot.<name> tag or retrieved knowledge. Empty = none.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "placement", "enum", "Where Template goes: before or after the Input, as a system or a user message.",
        "BeforeUserAsSystem", kEnum_Tryll_NodeParams_GenerateParams_placement, 4,
        "", "",
        "Placement", "", "",
    },
    {
        "send", "enum", "How the answer reaches the game: Streamed (token by token), Whole (once, at the end), or None (kept for later nodes only).",
        "Streamed", kEnum_Tryll_NodeParams_GenerateParams_send, 3,
        "", "",
        "SendAnswer", "", "",
    },
    {
        "history_role", "enum", "Whether later turns remember this answer as an assistant message. None = forgotten after this turn.",
        "Assistant", kEnum_Tryll_NodeParams_GenerateParams_history_role, 4,
        "", "",
        "HistoryRole", "", "",
    },
    {
        "sampling", "table", "Overrides for the model's sampling settings (temperature, top-p, max tokens…). Unset values keep the model's own.",
        "null", nullptr, 0,
        "", "",
        "SamplingOverrides", "", "",
    },
    {
        "grammar", "string", "GBNF grammar the answer must follow; needs a `root` rule. Empty = free text. Use it to force a format, such as JSON or a fixed set of lines.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "default_exit", "string", "Node to run after the answer is generated. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "substitute_agent_variables", "scalar", "Replace __NAME__ markers in the answer with the values of agent variables that allow output substitution. Not case-sensitive.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "output_filter", "table", "Removes roleplay artifacts from the answer: \"Bob:\" prefixes, *actions*, (asides), [notes]. All off by default.",
        "null", nullptr, 0,
        "", "",
        "OutputFilterOverrides", "", "",
    },
    {
        "context", "table", "Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit.",
        "null", nullptr, 0,
        "", "",
        "ContextOverrides", "", "",
    },
    {
        "adaptation", "table", "LoRA adapters applied to this node's model. Empty = the base model.",
        "null", nullptr, 0,
        "", "",
        "ModelAdaptation", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_GenerateParams = {
    "GenerateParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_GenerateParams, 16,
};
static const FTryllLogField kFields_Tryll_NodeParams_InstructionParams[] = {
    {
        "instruction", "string", "Text later nodes can insert into their prompts, as the slot.<Output Name> tag or through the #instructions list.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "output_name", "string", "Name later nodes use to read the Instruction (in their Input, or as the slot.<name> tag). Empty = this node's name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "default_exit", "string", "Node to run next. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_InstructionParams = {
    "InstructionParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_InstructionParams, 3,
};
static const FTryllLogField kFields_Tryll_NodeParams_IntentToInstructionParams[] = {
    {
        "string_storage", "string", "Storage file that maps intent labels to instruction text. Used instead of Inline Keys and Instructions.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "inline_keys", "vector", "Intent labels, paired by position with Instructions. Used when String Storage is empty.",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
    {
        "output_name", "string", "Name later nodes use to read the chosen instruction (in their Input, or as the slot.<name> tag). Empty = this node's name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "default_exit", "string", "Node to run next. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "instructions", "vector", "Instruction text for each label in Inline Keys, in the same order.",
        "null", nullptr, 0,
        "", "",
        "LocalizableText", "table", "LocalizableText",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_IntentToInstructionParams = {
    "IntentToInstructionParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_IntentToInstructionParams, 5,
};
static const FTryllLogField kFields_Tryll_NodeParams_LocalizableText[] = {
    {
        "text", "string", "The text. Tick Localized to store a Text ID from the workflow text table.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_LocalizableText = {
    "LocalizableText", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_LocalizableText, 1,
};
static const FTryllLogField kFields_Tryll_NodeParams_LoraAdapter[] = {
    {
        "path", "string", "LoRA adapter file (.gguf), relative to the storage root. Must be trained for the same base model as the node's model.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "scale", "scalar", "Adapter strength. 1 = as trained, 0 = off.",
        "1", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_LoraAdapter = {
    "LoraAdapter", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_LoraAdapter, 2,
};
static const FTryllLogField kFields_Tryll_NodeParams_ModelAdaptation[] = {
    {
        "loras", "vector", "LoRA adapters applied together, up to 8, each file once. Order does not matter. Empty = none.",
        "null", nullptr, 0,
        "", "",
        "LoraAdapter", "table", "LoraAdapter",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ModelAdaptation = {
    "ModelAdaptation", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ModelAdaptation, 1,
};
static const FTryllLogField kFields_Tryll_NodeParams_OutputFilterOverrides[] = {
    {
        "strip_speaker_prefix", "scalar", "Remove a leading speaker tag such as \"Bob: \" from the answer, once per turn.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "max_prefix_bytes", "scalar", "Longest speaker tag Strip Speaker Prefix looks for, in bytes.",
        "16", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "strip_asterisk_spans", "scalar", "Remove text between asterisks, such as *smiles*.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "strip_paren_spans", "scalar", "Remove text in parentheses, such as (laughs).",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "strip_bracket_spans", "scalar", "Remove text in square brackets, such as [aside].",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "max_span_bytes", "scalar", "Longest span the strip options can remove, in bytes. Longer spans are kept as they are.",
        "64", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_OutputFilterOverrides = {
    "OutputFilterOverrides", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_OutputFilterOverrides, 6,
};
static const FTryllLogField kFields_Tryll_NodeParams_PauseParams[] = {
    {
        "default_exit", "string", "Node to run when the game resumes without choosing one. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "pause_timeout_ms", "scalar", "How long to wait for the game to resume, in ms. 0 = forever. Set a limit unless the game always resumes.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "timeout_exit", "string", "Node to run if the wait times out, such as a fallback line. Empty = end the turn as timed out. Unused when Pause Timeout Ms is 0.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_PauseParams = {
    "PauseParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_PauseParams, 3,
};
static const FTryllLogField kFields_Tryll_NodeParams_RegexGuardrailParams[] = {
    {
        "string_storage", "string", "File of regex patterns, one per line. Used instead of Inline Strings. Both empty = the server's default patterns.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "inline_strings", "vector", "Regex patterns typed here; used when String Storage is empty.",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
    {
        "input", "string", "Text checked against the patterns: the player's message or an earlier node's output. Empty = the player's message.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "triggered_exit", "string", "Node to run when the Input matches a pattern. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "not_triggered_exit", "string", "Node to run when no pattern matches. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_RegexGuardrailParams = {
    "RegexGuardrailParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_RegexGuardrailParams, 5,
};
static const char* kEnum_Tryll_NodeParams_RetrieveParams_retrieval_mode[] = { "Dense", "Lexical", "Hybrid" };
static const char* kEnum_Tryll_NodeParams_RetrieveParams_source_kind[] = { "FromFile", "FromName" };
static const FTryllLogField kFields_Tryll_NodeParams_RetrieveParams[] = {
    {
        "embedded_string_storage", "string", "Embedded knowledge file to search. Used when Source Kind is From File; leave empty with From Name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "input", "string", "Text to search with: the player's message or an earlier node's output. Empty = the player's message.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "top_k", "scalar", "How many matching chunks to return.",
        "2", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "threshold", "scalar", "Max cosine distance for a chunk to count (0 = identical, 2 = opposite). 0 = no limit. Ignored in Lexical mode.",
        "0.5", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "retrieval_mode", "enum", "Dense = match by meaning. Lexical = match by keywords (BM25). Hybrid = both, merged.",
        "Dense", kEnum_Tryll_NodeParams_RetrieveParams_retrieval_mode, 3,
        "", "",
        "RetrievalMode", "", "",
    },
    {
        "rrf_k", "scalar", "Hybrid only: how strongly the merge favours top-ranked hits. Higher = flatter. 60 is the standard value.",
        "60", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "source", "string", "Name templates use for these results: the knowledge_<source> tag. Empty = this node's name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "filter", "string", "JSON filter on chunk metadata; only matching chunks are searched. Empty = no filter.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "found_exit", "string", "Node to run when at least one chunk is found. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "not_found_exit", "string", "Node to run when nothing is found. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "storage_name", "string", "Memory store to search, by name. Used when Source Kind is From Name. May include agent variables (var.*).",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "schema", "string", "Record schema of the store. Required with From Name; with From File, checked against the file.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "source_kind", "enum", "From File = a prebuilt knowledge file. From Name = a memory store the game fills during play (Write To Memory).",
        "FromFile", kEnum_Tryll_NodeParams_RetrieveParams_source_kind, 2,
        "", "",
        "KnowledgeSourceKind", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_RetrieveParams = {
    "RetrieveParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_RetrieveParams, 13,
};
static const FTryllLogField kFields_Tryll_NodeParams_SamplingOverrides[] = {
    {
        "temperature", "scalar", "Randomness of the output: 0 = always the most likely token, 2 = very random. Unset = the model's default.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "top_p", "scalar", "Nucleus sampling: pick only from the most likely tokens that together reach this probability (0–1). Unset = the model's default.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "top_k", "scalar", "Pick only from this many most likely tokens. Unset = the model's default.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "min_p", "scalar", "Drop tokens less likely than this fraction of the top token (0–1). Unset = the model's default.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "max_tokens", "scalar", "Longest answer this node may generate, in tokens. Must be smaller than Context Size. Unset = the model's default.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "seed", "scalar", "Random seed; the same seed and input give the same output. Unset = the model's default.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "repeat_penalty", "scalar", "Penalty for repeating recent tokens; 1 = none, higher = less repetition. Unset = the model's default.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "presence_penalty", "scalar", "Penalty for any token already used, pushing toward new topics. Unset = the model's default.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "frequency_penalty", "scalar", "Penalty that grows with how often a token was already used. Unset = the model's default.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_SamplingOverrides = {
    "SamplingOverrides", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_SamplingOverrides, 9,
};
static const FTryllLogField kFields_Tryll_NodeParams_SpeakParams[] = {
    {
        "input", "string", "Text to speak: the player's message or an earlier node's output (for example a Canned Response). Empty = the player's message.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "min_sentence_chars", "scalar", "Minimum length of a spoken chunk before a sentence end cuts it (CJK characters count double). Lower = speech starts sooner, more TTS calls.",
        "12", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "default_exit", "string", "Node to run after the text is spoken. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "first_chunk_max_chars", "scalar", "Lets the first chunk end early, at a comma, dash or space once it is this long, so speech starts sooner. 0 = off; try 48.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_SpeakParams = {
    "SpeakParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_SpeakParams, 4,
};
static const FTryllLogField kFields_Tryll_NodeParams_SummarizeParams[] = {
    {
        "model_name", "string", "Language model that writes the summaries. Empty = the workflow's Default Model Name. Prefer a plain instruct model over a roleplay fine-tune.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context_size", "scalar", "Context window for this node, in tokens; must fit the model's trained maximum. 0 = 2048.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "system_prompt", "string", "Instructions for the summarizer. Empty = the built-in default (third person, named speakers, no speculation).",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "template", "string", "Request sent to the summarizer. Can use the tags previous_summary, #turns (index, user, assistant) and max_summary_tokens. Empty = built-in.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "trigger_tokens", "scalar", "Summarize when unsummarized history reaches this many tokens. Keep it below the point where the reply node trims history. 0 = off.",
        "1024", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "trigger_turns", "scalar", "Summarize every N turns. 0 = off. With both triggers off, the node does nothing.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "keep_recent_turns", "scalar", "Latest turns never summarized; they are always replayed word for word. Max 512.",
        "3", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "max_summary_tokens", "scalar", "Longest summary, in tokens. Must leave at least 64 tokens of Context Size free.",
        "256", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "sampling", "table", "Overrides for the model's sampling settings (temperature, top-p, max tokens…). Unset values keep the model's own.",
        "null", nullptr, 0,
        "", "",
        "SamplingOverrides", "", "",
    },
    {
        "notify_client", "scalar", "Send the game a summarized event after each summary.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "run_inline", "scalar", "Must stay on: summarize inside the turn that triggered it, which adds the summary time to that turn. Background mode is not available yet.",
        "true", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "default_exit", "string", "Node to run next, whether or not a summary was written. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context", "table", "Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit.",
        "null", nullptr, 0,
        "", "",
        "ContextOverrides", "", "",
    },
    {
        "adaptation", "table", "LoRA adapters applied to this node's model. Empty = the base model.",
        "null", nullptr, 0,
        "", "",
        "ModelAdaptation", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_SummarizeParams = {
    "SummarizeParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_SummarizeParams, 14,
};
static const char* kEnum_Tryll_NodeParams_ToolCallParams_mode[] = { "DetectOnly", "CallOrAnswer", "RequireCall" };
static const char* kEnum_Tryll_NodeParams_ToolCallParams_disposition[] = { "RouteOnly", "Acknowledge", "Notify", "NotifyAndAcknowledge", "Pause", "PauseAndAcknowledge", "AwaitResult" };
static const FTryllLogField kFields_Tryll_NodeParams_ToolCallParams[] = {
    {
        "model_name", "string", "Language model this node runs. Empty = the workflow's Default Model Name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context_size", "scalar", "Context window for this node, in tokens; must fit the model's trained maximum. 0 = 2048.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "system_prompt", "string", "System message the model reads before the dialog. Empty = none.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "input", "string", "Text the model reads to decide on a tool call: the player's message or an earlier node's output. Empty = the player's message.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "tool_call_format", "string", "Not used. Kept so older workflows still load.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "tools", "vector", "Tools the model may call, each with a name, description and parameters. The game carries out the call; the server only detects it.",
        "null", nullptr, 0,
        "", "",
        "ToolDefinition", "table", "ToolDefinition",
    },
    {
        "mode", "enum", "DetectOnly: call a tool or say nothing. CallOrAnswer: call a tool or answer in text. RequireCall: must call a tool.",
        "CallOrAnswer", kEnum_Tryll_NodeParams_ToolCallParams_mode, 3,
        "", "",
        "ToolCallMode", "", "",
    },
    {
        "parallel_tool_calls", "scalar", "Allow several tool calls in one turn, if the model's chat template supports it.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "sampling", "table", "Overrides for the model's sampling settings (temperature, top-p, max tokens…). Unset values keep the model's own.",
        "null", nullptr, 0,
        "", "",
        "SamplingOverrides", "", "",
    },
    {
        "tool_called_exit", "string", "Node to run when the model calls a tool. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "no_tool_called_exit", "string", "Node to run when no tool is called (under CallOrAnswer, after the text answer). Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "disposition", "enum", "After a call: whether to tell the game (Notify), wait for it (Pause, AwaitResult), and record an acknowledgement in history (…Acknowledge).",
        "Acknowledge", kEnum_Tryll_NodeParams_ToolCallParams_disposition, 7,
        "", "",
        "ToolCallDisposition", "", "",
    },
    {
        "acknowledge_text", "string", "Tool result recorded in history by the …Acknowledge dispositions. Empty = \"ok\".",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "pause_timeout_ms", "scalar", "How long a pausing Disposition waits for the game, in ms. 0 = the server default (wait forever in released builds). On timeout the turn ends.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "context", "table", "Advanced memory and batching settings for this node's model (KV cache type, GPU offload, batch sizes). Unset = inherit.",
        "null", nullptr, 0,
        "", "",
        "ContextOverrides", "", "",
    },
    {
        "adaptation", "table", "LoRA adapters applied to this node's model. Empty = the base model.",
        "null", nullptr, 0,
        "", "",
        "ModelAdaptation", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ToolCallParams = {
    "ToolCallParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ToolCallParams, 16,
};
static const FTryllLogField kFields_Tryll_NodeParams_ToolDefinition[] = {
    {
        "name", "string", "Tool name the model uses to call it, and the game receives with the call.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "description", "string", "What the tool does; the model reads this to decide when to call it.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "parameters", "vector", "Values the model must supply when it calls this tool.",
        "null", nullptr, 0,
        "", "",
        "ToolParamDefinition", "table", "ToolParamDefinition",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ToolDefinition = {
    "ToolDefinition", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ToolDefinition, 3,
};
static const FTryllLogField kFields_Tryll_NodeParams_ToolParamDefinition[] = {
    {
        "name", "string", "Parameter name the model fills in when it calls the tool.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "type", "string", "JSON Schema type of the value, such as \"string\", \"number\" or \"boolean\".",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "description", "string", "What the parameter means; the model reads this to fill it in.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_ToolParamDefinition = {
    "ToolParamDefinition", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_ToolParamDefinition, 3,
};
static const FTryllLogField kFields_Tryll_NodeParams_TransformParams[] = {
    {
        "template", "string", "Mustache text this node outputs. Can use the tags user_message, slot.<name>, knowledge_<source> and #instructions.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "output_name", "string", "Name later nodes use to read the rendered text (in their Input, or as the slot.<name> tag). Empty = this node's name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "default_exit", "string", "Node to run next. Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_TransformParams = {
    "TransformParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_TransformParams, 3,
};
static const FTryllLogField kFields_Tryll_NodeParams_WriteToMemoryParams[] = {
    {
        "storage_name", "string", "Memory store to write to, by name; it lasts for the session. May include agent variables (var.*).",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "schema", "string", "Record schema of the store. Nodes that read the same store must use the same schema.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "input", "string", "Text to store: the player's message or an earlier node's output. Empty = the player's message.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "id", "string", "Record id; writing an existing id replaces that record. May include agent variables (var.*). Empty = add a new record each time.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "written_exit", "string", "Node to run after the write, or when the text to store is empty (nothing is written). Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "failed_exit", "string", "Node to run when the store rejects the write (for example, the store is full). Empty = end the turn.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_NodeParams_WriteToMemoryParams = {
    "WriteToMemoryParams", "",
    "", "",
    false, false,
    kFields_Tryll_NodeParams_WriteToMemoryParams, 6,
};
static const char* kEnum_Tryll_PersistedComponentEntry_value[] = { "NONE", "PersistedSlot", "PersistedKnowledge", "PersistedIntention", "PersistedToolCall", "PersistedToolResult", "PersistedSummary" };
static const FTryllLogField kFields_Tryll_PersistedComponentEntry[] = {
    {
        "value", "union", "",
        "null", kEnum_Tryll_PersistedComponentEntry_value, 7,
        "", "",
        "PersistedComponent", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedComponentEntry = {
    "PersistedComponentEntry", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedComponentEntry, 1,
};
static const FTryllLogField kFields_Tryll_PersistedDialogSnapshot[] = {
    {
        "format_version", "scalar", "",
        "1", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "graph_signature", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "interactions", "vector", "",
        "null", nullptr, 0,
        "size", "",
        "PersistedInteraction", "table", "PersistedInteraction",
    },
    {
        "next_tool_call_seq", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "next_exchange_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "projection_windows", "vector", "",
        "null", nullptr, 0,
        "size", "",
        "PersistedProjectionWindow", "table", "PersistedProjectionWindow",
    },
    {
        "save_id", "string", "Client-supplied coherence stamp. Compared with game-memory save_id.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "saved_at_ms", "scalar", "Server-generated Unix epoch milliseconds at export/save time.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedDialogSnapshot = {
    "PersistedDialogSnapshot", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedDialogSnapshot, 8,
};
static const FTryllLogField kFields_Tryll_PersistedIntention[] = {
    {
        "intent", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedIntention = {
    "PersistedIntention", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedIntention, 1,
};
static const FTryllLogField kFields_Tryll_PersistedInteraction[] = {
    {
        "interrupted", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "components", "vector", "Insertion order is semantic (last-wins slots, tool replay).",
        "null", nullptr, 0,
        "", "",
        "PersistedComponentEntry", "table", "PersistedComponentEntry",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedInteraction = {
    "PersistedInteraction", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedInteraction, 2,
};
static const FTryllLogField kFields_Tryll_PersistedKbRecord[] = {
    {
        "id", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "created_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "updated_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedKbRecord = {
    "PersistedKbRecord", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedKbRecord, 4,
};
static const FTryllLogField kFields_Tryll_PersistedKnowledge[] = {
    {
        "source", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "chunks", "vector", "",
        "null", nullptr, 0,
        "", "",
        "PersistedKnowledgeChunk", "table", "PersistedKnowledgeChunk",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedKnowledge = {
    "PersistedKnowledge", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedKnowledge, 2,
};
static const FTryllLogField kFields_Tryll_PersistedKnowledgeChunk[] = {
    {
        "id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "distance", "scalar", "Cosine distance. Ignored when distance_is_nan is true.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "distance_is_nan", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "bm25_score", "scalar", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "rank_dense", "scalar", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "rank_sparse", "scalar", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedKnowledgeChunk = {
    "PersistedKnowledgeChunk", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedKnowledgeChunk, 7,
};
static const FTryllLogField kFields_Tryll_PersistedMutableKb[] = {
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "schema_name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "schema_fingerprint", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "embedding_model", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "write_generation", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "next_generated_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "records", "vector", "",
        "null", nullptr, 0,
        "size", "",
        "PersistedKbRecord", "table", "PersistedKbRecord",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedMutableKb = {
    "PersistedMutableKb", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedMutableKb, 7,
};
static const FTryllLogField kFields_Tryll_PersistedProjectionWindow[] = {
    {
        "node_name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "window_start", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedProjectionWindow = {
    "PersistedProjectionWindow", "",
    "", "",
    true, false,
    kFields_Tryll_PersistedProjectionWindow, 2,
};
static const char* kEnum_Tryll_PersistedSlot_kind[] = { "User", "Instruction", "General" };
static const char* kEnum_Tryll_PersistedSlot_history_role[] = { "None", "Assistant", "User", "System" };
static const FTryllLogField kFields_Tryll_PersistedSlot[] = {
    {
        "name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "kind", "enum", "",
        "User", kEnum_Tryll_PersistedSlot_kind, 3,
        "", "",
        "PersistedSlotKind", "", "",
    },
    {
        "history_role", "enum", "",
        "None", kEnum_Tryll_PersistedSlot_history_role, 4,
        "", "",
        "HistoryRole", "", "",
    },
    {
        "producer", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedSlot = {
    "PersistedSlot", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedSlot, 5,
};
static const FTryllLogField kFields_Tryll_PersistedStringPair[] = {
    {
        "key", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "value", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedStringPair = {
    "PersistedStringPair", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedStringPair, 2,
};
static const FTryllLogField kFields_Tryll_PersistedSummary[] = {
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "covered_through", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "producer", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedSummary = {
    "PersistedSummary", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedSummary, 3,
};
static const char* kEnum_Tryll_PersistedToolCall_disposition[] = { "RouteOnly", "Acknowledge", "Notify", "NotifyAndAcknowledge", "Pause", "PauseAndAcknowledge", "AwaitResult" };
static const FTryllLogField kFields_Tryll_PersistedToolCall[] = {
    {
        "tool_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "arguments", "vector", "",
        "null", nullptr, 0,
        "", "",
        "PersistedStringPair", "table", "PersistedStringPair",
    },
    {
        "arguments_json", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "dispatched", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "disposition", "enum", "",
        "RouteOnly", kEnum_Tryll_PersistedToolCall_disposition, 7,
        "", "",
        "ToolCallDisposition", "", "",
    },
    {
        "model_id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "exchange_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedToolCall = {
    "PersistedToolCall", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedToolCall, 8,
};
static const FTryllLogField kFields_Tryll_PersistedToolResult[] = {
    {
        "call_id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "tool_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "exchange_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PersistedToolResult = {
    "PersistedToolResult", "",
    "", "",
    false, false,
    kFields_Tryll_PersistedToolResult, 4,
};
static const FTryllLogField kFields_Tryll_PrefillAgentKvCacheRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PrefillAgentKvCacheRequest = {
    "PrefillAgentKvCacheRequest", "",
    "", "",
    false, false,
    kFields_Tryll_PrefillAgentKvCacheRequest, 2,
};
static const char* kEnum_Tryll_PrefillAgentKvCacheResponse_residency[] = { "Evicted", "Resident" };
static const char* kEnum_Tryll_PrefillAgentKvCacheResponse_prefix_status[] = { "NotCurrent", "Current" };
static const FTryllLogField kFields_Tryll_PrefillAgentKvCacheResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "applicable", "scalar", "False when the graph has no language-model nodes (operation is a no-op).",
        "true", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "residency", "enum", "",
        "Evicted", kEnum_Tryll_PrefillAgentKvCacheResponse_residency, 2,
        "", "",
        "AgentKvCacheResidency", "", "",
    },
    {
        "prefix_status", "enum", "",
        "NotCurrent", kEnum_Tryll_PrefillAgentKvCacheResponse_prefix_status, 2,
        "", "",
        "AgentKvCachePrefixStatus", "", "",
    },
    {
        "eligible_node_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "contexts_created", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "tokens_decoded", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "tokens_reused", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "debug_info", "string", "JSON lifecycle diagnostics. Non-empty only when enable_diagnostics was set on CreateAgentRequest. Never contains prompts or token text.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_PrefillAgentKvCacheResponse = {
    "PrefillAgentKvCacheResponse", "",
    "", "",
    false, false,
    kFields_Tryll_PrefillAgentKvCacheResponse, 10,
};
static const FTryllLogField kFields_Tryll_RemoveInteractionsFromEndRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_RemoveInteractionsFromEndRequest = {
    "RemoveInteractionsFromEndRequest", "",
    "", "",
    false, false,
    kFields_Tryll_RemoveInteractionsFromEndRequest, 3,
};
static const FTryllLogField kFields_Tryll_RemoveInteractionsFromEndResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "removed_count", "scalar", "Number of interactions actually removed.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_RemoveInteractionsFromEndResponse = {
    "RemoveInteractionsFromEndResponse", "",
    "", "",
    false, false,
    kFields_Tryll_RemoveInteractionsFromEndResponse, 3,
};
static const FTryllLogField kFields_Tryll_ResumeAgentRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "resume_node", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "tool_results", "vector", "",
        "null", nullptr, 0,
        "", "",
        "ToolResult", "table", "ToolResult",
    },
};
static const FTryllLogTable kTable_Tryll_ResumeAgentRequest = {
    "ResumeAgentRequest", "",
    "", "",
    false, false,
    kFields_Tryll_ResumeAgentRequest, 4,
};
static const FTryllLogField kFields_Tryll_SaveDialogRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "path", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "save_id", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_SaveDialogRequest = {
    "SaveDialogRequest", "",
    "", "",
    false, false,
    kFields_Tryll_SaveDialogRequest, 4,
};
static const FTryllLogField kFields_Tryll_SaveDialogResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "saved_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_SaveDialogResponse = {
    "SaveDialogResponse", "",
    "", "",
    false, false,
    kFields_Tryll_SaveDialogResponse, 3,
};
static const FTryllLogField kFields_Tryll_SaveGameMemoryRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "path", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "save_id", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_SaveGameMemoryRequest = {
    "SaveGameMemoryRequest", "",
    "", "",
    false, false,
    kFields_Tryll_SaveGameMemoryRequest, 3,
};
static const FTryllLogField kFields_Tryll_SaveGameMemoryResponse[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "saved_at_ms", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "store_count", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_SaveGameMemoryResponse = {
    "SaveGameMemoryResponse", "",
    "", "",
    false, false,
    kFields_Tryll_SaveGameMemoryResponse, 3,
};
static const FTryllLogField kFields_Tryll_SendMessageRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "multiline",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_SendMessageRequest = {
    "SendMessageRequest", "{text}",
    "", "",
    false, false,
    kFields_Tryll_SendMessageRequest, 3,
};
static const FTryllLogField kFields_Tryll_SessionModelConfig[] = {
    {
        "engines", "vector", "ADVISORY. Engines this session expects to touch, one entry per (role, engine) pair. May be empty, absent, incomplete, or wrong: none of that prevents a session or a load, and nothing is inferred from an empty list — in particular it is never read as Mock. Its only job is to let the server initialise those engines' backends at CreateSession instead of at first use. A model's engine always comes from the server's own catalog entry, so loading a model this list never named is normal — a developer adding a model to a live session is the motivating case. Unity and Unreal derive it from the Model Manager's manifest or omit it, and expose no engine control. C++ and Python may state it explicitly.",
        "null", nullptr, 0,
        "", "",
        "EngineSelection", "table", "EngineSelection",
    },
    {
        "placement_hints", "vector", "Sparse per-model hints. A model absent from this list means MemoryPlacement.Auto and HotwordBias.Default.",
        "null", nullptr, 0,
        "", "",
        "ModelPlacementHint", "table", "ModelPlacementHint",
    },
};
static const FTryllLogTable kTable_Tryll_SessionModelConfig = {
    "SessionModelConfig", "",
    "", "",
    false, false,
    kFields_Tryll_SessionModelConfig, 2,
};
static const FTryllLogField kFields_Tryll_SetAgentDeliveryModeRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "mode", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "intensity", "scalar", "",
        "1", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_SetAgentDeliveryModeRequest = {
    "SetAgentDeliveryModeRequest", "",
    "", "",
    false, false,
    kFields_Tryll_SetAgentDeliveryModeRequest, 4,
};
static const FTryllLogField kFields_Tryll_SetAgentVoiceRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "voice", "table", "",
        "null", nullptr, 0,
        "", "",
        "VoiceProfile", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_SetAgentVoiceRequest = {
    "SetAgentVoiceRequest", "",
    "", "",
    false, false,
    kFields_Tryll_SetAgentVoiceRequest, 3,
};
static const FTryllLogField kFields_Tryll_SetInferenceThrottle[] = {
    {
        "throttle", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_SetInferenceThrottle = {
    "SetInferenceThrottle", "",
    "", "",
    false, false,
    kFields_Tryll_SetInferenceThrottle, 1,
};
static const FTryllLogField kFields_Tryll_SetSessionLocaleRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "locale", "string", "BCP-47 tag (\"de\", \"pt-BR\"), or empty to clear the session locale.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_SetSessionLocaleRequest = {
    "SetSessionLocaleRequest", "",
    "", "",
    false, false,
    kFields_Tryll_SetSessionLocaleRequest, 2,
};
static const FTryllLogField kFields_Tryll_ToolResult[] = {
    {
        "call_id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "result", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_ToolResult = {
    "ToolResult", "",
    "", "",
    false, false,
    kFields_Tryll_ToolResult, 2,
};
static const FTryllLogField kFields_Tryll_TtsAudioFormatFrame[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "format", "table", "",
        "null", nullptr, 0,
        "", "",
        "WireAudioFormat", "", "",
    },
    {
        "node_name", "string", "Name of the first speaking node of the turn. Per-chunk attribution is TtsAudioFrame.node_name.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_TtsAudioFormatFrame = {
    "TtsAudioFormatFrame", "",
    "", "",
    true, false,
    kFields_Tryll_TtsAudioFormatFrame, 4,
};
static const FTryllLogField kFields_Tryll_TtsAudioFrame[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "pcm", "vector", "",
        "null", nullptr, 0,
        "size", "",
        "", "scalar", "",
    },
    {
        "node_name", "string", "Name of the node that produced this audio chunk (multi-sender attribution).",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_TtsAudioFrame = {
    "TtsAudioFrame", "{node_name}",
    "sum", "",
    true, false,
    kFields_Tryll_TtsAudioFrame, 4,
};
static const FTryllLogField kFields_Tryll_TtsCapabilitiesInfo[] = {
    {
        "voice_description", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "delivery_instruction", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "reference_voice", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "speed", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "language_per_call", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "voice_presets", "vector", "",
        "null", nullptr, 0,
        "", "",
        "TtsVoicePresetInfo", "table", "TtsVoicePresetInfo",
    },
    {
        "inline_tags", "vector", "",
        "null", nullptr, 0,
        "", "",
        "TtsInlineTagInfo", "table", "TtsInlineTagInfo",
    },
    {
        "expressiveness", "scalar", "Chatterbox-style exaggeration knob (VoiceProfile.expressiveness).",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "num_speakers", "scalar", "Number of speaker ids (VoiceProfile.speaker_id is 0..num_speakers-1). 0 = not advertised by the catalog.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_TtsCapabilitiesInfo = {
    "TtsCapabilitiesInfo", "",
    "", "",
    false, false,
    kFields_Tryll_TtsCapabilitiesInfo, 9,
};
static const FTryllLogField kFields_Tryll_TtsInlineTagInfo[] = {
    {
        "token", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "kind", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "verification", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_TtsInlineTagInfo = {
    "TtsInlineTagInfo", "",
    "", "",
    false, false,
    kFields_Tryll_TtsInlineTagInfo, 3,
};
static const FTryllLogField kFields_Tryll_TtsVisemeFrame[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "node_name", "string", "Name of the node that produced this viseme track (multi-sender attribution — mirrors TtsAudioFrame.node_name).",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "events", "table", "",
        "null", nullptr, 0,
        "", "",
        "VisemeEventList", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_TtsVisemeFrame = {
    "TtsVisemeFrame", "{node_name}",
    "sum", "",
    true, false,
    kFields_Tryll_TtsVisemeFrame, 4,
};
static const FTryllLogField kFields_Tryll_TtsVoicePresetInfo[] = {
    {
        "id", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "label", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "description", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_TtsVoicePresetInfo = {
    "TtsVoicePresetInfo", "",
    "", "",
    false, false,
    kFields_Tryll_TtsVoicePresetInfo, 3,
};
static const char* kEnum_Tryll_TurnComplete_status[] = { "Success", "Error", "Cancelled", "PauseTimedOut" };
static const FTryllLogField kFields_Tryll_TurnComplete[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "status", "enum", "",
        "Success", kEnum_Tryll_TurnComplete_status, 4,
        "", "",
        "TurnStatus", "", "",
    },
    {
        "debug_info", "string", "JSON-encoded workflow execution trace. Populated only when enable_diagnostics was set on the CreateAgentRequest. Empty string otherwise.",
        "null", nullptr, 0,
        "json", "",
        "", "", "",
    },
    {
        "tokens_generated", "scalar", "Total tokens sampled across all generation nodes in this turn (prompt tokens excluded).",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_TurnComplete = {
    "TurnComplete", "",
    "", "",
    false, false,
    kFields_Tryll_TurnComplete, 5,
};
static const FTryllLogField kFields_Tryll_UnloadModelRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "model_name", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_UnloadModelRequest = {
    "UnloadModelRequest", "",
    "", "",
    false, false,
    kFields_Tryll_UnloadModelRequest, 2,
};
static const FTryllLogField kFields_Tryll_UpdateAgentVariablesRequest[] = {
    {
        "request_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "agent_id", "scalar", "",
        "0", nullptr, 0,
        "", "hidden",
        "", "", "",
    },
    {
        "updates", "vector", "",
        "", nullptr, 0,
        "", "",
        "VariableUpdate", "table", "VariableUpdate",
    },
};
static const FTryllLogTable kTable_Tryll_UpdateAgentVariablesRequest = {
    "UpdateAgentVariablesRequest", "",
    "", "",
    false, false,
    kFields_Tryll_UpdateAgentVariablesRequest, 3,
};
static const FTryllLogField kFields_Tryll_VarBool[] = {
    {
        "v", "scalar", "",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_VarBool = {
    "VarBool", "",
    "", "",
    false, false,
    kFields_Tryll_VarBool, 1,
};
static const FTryllLogField kFields_Tryll_VarFloat[] = {
    {
        "v", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_VarFloat = {
    "VarFloat", "",
    "", "",
    false, false,
    kFields_Tryll_VarFloat, 1,
};
static const FTryllLogField kFields_Tryll_VarInt[] = {
    {
        "v", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_VarInt = {
    "VarInt", "",
    "", "",
    false, false,
    kFields_Tryll_VarInt, 1,
};
static const FTryllLogField kFields_Tryll_VarString[] = {
    {
        "v", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_VarString = {
    "VarString", "",
    "", "",
    false, false,
    kFields_Tryll_VarString, 1,
};
static const FTryllLogField kFields_Tryll_VarStringSet[] = {
    {
        "v", "vector", "",
        "null", nullptr, 0,
        "", "",
        "", "string", "",
    },
};
static const FTryllLogTable kTable_Tryll_VarStringSet = {
    "VarStringSet", "",
    "", "",
    false, false,
    kFields_Tryll_VarStringSet, 1,
};
static const char* kEnum_Tryll_VariableUpdate_op[] = { "Assign", "AddToSet", "RemoveFromSet", "Reset" };
static const char* kEnum_Tryll_VariableUpdate_value[] = { "NONE", "VarInt", "VarFloat", "VarString", "VarBool", "VarStringSet" };
static const FTryllLogField kFields_Tryll_VariableUpdate[] = {
    {
        "name", "string", "",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "op", "enum", "",
        "Assign", kEnum_Tryll_VariableUpdate_op, 4,
        "", "",
        "VariableOp", "", "",
    },
    {
        "value", "union", "",
        "null", kEnum_Tryll_VariableUpdate_value, 6,
        "", "",
        "VariableValue", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_VariableUpdate = {
    "VariableUpdate", "",
    "", "",
    false, false,
    kFields_Tryll_VariableUpdate, 3,
};
static const char* kEnum_Tryll_VisemeEvent_viseme[] = { "sil", "PP", "FF", "TH", "DD", "kk", "CH", "SS", "nn", "RR", "aa", "E", "ih", "oh", "ou" };
static const FTryllLogField kFields_Tryll_VisemeEvent[] = {
    {
        "sample_offset", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "viseme", "enum", "",
        "sil", kEnum_Tryll_VisemeEvent_viseme, 15,
        "", "",
        "WireViseme", "", "",
    },
    {
        "weight", "scalar", "",
        "1", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_VisemeEvent = {
    "VisemeEvent", "",
    "", "",
    false, false,
    kFields_Tryll_VisemeEvent, 3,
};
static const FTryllLogField kFields_Tryll_VisemeEventList[] = {
    {
        "events", "vector", "",
        "null", nullptr, 0,
        "size", "",
        "VisemeEvent", "table", "VisemeEvent",
    },
};
static const FTryllLogTable kTable_Tryll_VisemeEventList = {
    "VisemeEventList", "",
    "", "",
    false, false,
    kFields_Tryll_VisemeEventList, 1,
};
static const char* kEnum_Tryll_VoiceProfile_prepare[] = { "Eager", "Lazy" };
static const FTryllLogField kFields_Tryll_VoiceProfile[] = {
    {
        "tts_model_name", "string", "TTS model catalog name. Required: there is no default model. An empty string is still present on the wire and is rejected in code (3023).",
        "", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "speaker_id", "scalar", "Speaker / style index for multi-speaker models (Supertonic 3: 0-9, Orpheus and Maya1: preset index). Must be below the model's speaker count.",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "reference_voice", "string", "Reference clip for clone families (Pocket TTS, Chatterbox, Qwen3-TTS Base), relative to the storage root; locale-aware (voices/loc/<locale>/). Empty = the model's default voice (Qwen3-TTS Base requires a clip).",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "voice_description", "string", "Maya1: natural-language identity of the voice. Overrides speaker_id.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "speed", "scalar", "Speech-rate multiplier. 1.0 = native. Must stay 1.0 on models without speed control (Orpheus, Maya1, Qwen3-TTS).",
        "1", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "lang", "string", "Language for multilingual models that pick it per call. Empty = session locale, then the model default. Rejected on models with a fixed language.",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "expressiveness", "scalar", "Chatterbox exaggeration used by \"neutral\" and by modes that do not set their own, 0.0-2.0. Ignored by other families.",
        "0.5", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "prepare", "enum", "When reference clips are prepared. Default Eager.",
        "Eager", kEnum_Tryll_VoiceProfile_prepare, 2,
        "", "",
        "VoicePreparation", "", "",
    },
    {
        "emit_visemes", "scalar", "Emit a viseme track (TtsVisemeFrame) for every sentence this agent speaks, from any speaking node. Turn on for lip-synced characters.",
        "false", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "delivery_modes", "vector", "Named delivery modes. Game code switches between them with SetAgentDeliveryModeRequest.",
        "null", nullptr, 0,
        "", "",
        "DeliveryMode", "table", "DeliveryMode",
    },
    {
        "default_delivery_mode", "string", "Mode active after the voice is set. Empty = \"neutral\".",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_VoiceProfile = {
    "VoiceProfile", "",
    "", "",
    false, false,
    kFields_Tryll_VoiceProfile, 11,
};
static const FTryllLogField kFields_Tryll_WireAudioFormat[] = {
    {
        "sample_rate", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "channels", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "bits_per_sample", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_WireAudioFormat = {
    "WireAudioFormat", "",
    "", "",
    false, false,
    kFields_Tryll_WireAudioFormat, 3,
};
static const char* kEnum_Tryll_WireTranscriptUpdate_kind[] = { "SpeechStart", "Partial", "SegmentFinal", "UtteranceFinal" };
static const FTryllLogField kFields_Tryll_WireTranscriptUpdate[] = {
    {
        "voice_input_id", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "text", "string", "",
        "null", nullptr, 0,
        "", "",
        "", "", "",
    },
    {
        "kind", "enum", "",
        "SpeechStart", kEnum_Tryll_WireTranscriptUpdate_kind, 4,
        "", "",
        "TranscriptUpdateKind", "", "",
    },
    {
        "audio_ms_consumed", "scalar", "",
        "0", nullptr, 0,
        "", "",
        "", "", "",
    },
};
static const FTryllLogTable kTable_Tryll_WireTranscriptUpdate = {
    "WireTranscriptUpdate", "",
    "", "",
    false, false,
    kFields_Tryll_WireTranscriptUpdate, 4,
};
const FTryllLogTable* FindTable(const char* Name)
{
    if (Name == nullptr) return nullptr;
    static const std::pair<const char*, const FTryllLogTable*> kRows[] = {
        { kTable_Tryll_Ack.Name, &kTable_Tryll_Ack },
        { kTable_Tryll_AgentVariable.Name, &kTable_Tryll_AgentVariable },
        { kTable_Tryll_AnswerText.Name, &kTable_Tryll_AnswerText },
        { kTable_Tryll_AppendInteractionsRequest.Name, &kTable_Tryll_AppendInteractionsRequest },
        { kTable_Tryll_AppendInteractionsResponse.Name, &kTable_Tryll_AppendInteractionsResponse },
        { kTable_Tryll_AudioBuffer.Name, &kTable_Tryll_AudioBuffer },
        { kTable_Tryll_BeginUtteranceRequest.Name, &kTable_Tryll_BeginUtteranceRequest },
        { kTable_Tryll_CancelRequest.Name, &kTable_Tryll_CancelRequest },
        { kTable_Tryll_CancelUtteranceRequest.Name, &kTable_Tryll_CancelUtteranceRequest },
        { kTable_Tryll_ChangeAgentParamRequest.Name, &kTable_Tryll_ChangeAgentParamRequest },
        { kTable_Tryll_ConnectionReady.Name, &kTable_Tryll_ConnectionReady },
        { kTable_Tryll_CreateAgentRequest.Name, &kTable_Tryll_CreateAgentRequest },
        { kTable_Tryll_CreateAgentResponse.Name, &kTable_Tryll_CreateAgentResponse },
        { kTable_Tryll_CreateEmbeddedStringStorageRequest.Name, &kTable_Tryll_CreateEmbeddedStringStorageRequest },
        { kTable_Tryll_CreateEmbeddedStringStorageResponse.Name, &kTable_Tryll_CreateEmbeddedStringStorageResponse },
        { kTable_Tryll_CreateSessionRequest.Name, &kTable_Tryll_CreateSessionRequest },
        { kTable_Tryll_CreateSessionResponse.Name, &kTable_Tryll_CreateSessionResponse },
        { kTable_Tryll_CreateStringStorageRequest.Name, &kTable_Tryll_CreateStringStorageRequest },
        { kTable_Tryll_CreateStringStorageResponse.Name, &kTable_Tryll_CreateStringStorageResponse },
        { kTable_Tryll_CreateVoiceInputRequest.Name, &kTable_Tryll_CreateVoiceInputRequest },
        { kTable_Tryll_CreateVoiceInputResponse.Name, &kTable_Tryll_CreateVoiceInputResponse },
        { kTable_Tryll_DebugCommandRequest.Name, &kTable_Tryll_DebugCommandRequest },
        { kTable_Tryll_DebugCommandResponse.Name, &kTable_Tryll_DebugCommandResponse },
        { kTable_Tryll_DeleteModelRequest.Name, &kTable_Tryll_DeleteModelRequest },
        { kTable_Tryll_DeliveryMode.Name, &kTable_Tryll_DeliveryMode },
        { kTable_Tryll_DestroyAgentRequest.Name, &kTable_Tryll_DestroyAgentRequest },
        { kTable_Tryll_DestroyEmbeddedStringStorageRequest.Name, &kTable_Tryll_DestroyEmbeddedStringStorageRequest },
        { kTable_Tryll_DestroyStringStorageRequest.Name, &kTable_Tryll_DestroyStringStorageRequest },
        { kTable_Tryll_DestroyVoiceInputRequest.Name, &kTable_Tryll_DestroyVoiceInputRequest },
        { kTable_Tryll_DialogInteraction.Name, &kTable_Tryll_DialogInteraction },
        { kTable_Tryll_DownloadComplete.Name, &kTable_Tryll_DownloadComplete },
        { kTable_Tryll_DownloadModelRequest.Name, &kTable_Tryll_DownloadModelRequest },
        { kTable_Tryll_DownloadProgress.Name, &kTable_Tryll_DownloadProgress },
        { kTable_Tryll_EndUtteranceRequest.Name, &kTable_Tryll_EndUtteranceRequest },
        { kTable_Tryll_EngineSelection.Name, &kTable_Tryll_EngineSelection },
        { kTable_Tryll_EnsureMutableKbRequest.Name, &kTable_Tryll_EnsureMutableKbRequest },
        { kTable_Tryll_EnsureMutableKbResponse.Name, &kTable_Tryll_EnsureMutableKbResponse },
        { kTable_Tryll_EnumerateMutableKbsRequest.Name, &kTable_Tryll_EnumerateMutableKbsRequest },
        { kTable_Tryll_EnumerateMutableKbsResponse.Name, &kTable_Tryll_EnumerateMutableKbsResponse },
        { kTable_Tryll_ErrorResponse.Name, &kTable_Tryll_ErrorResponse },
        { kTable_Tryll_EvictAgentKvCacheRequest.Name, &kTable_Tryll_EvictAgentKvCacheRequest },
        { kTable_Tryll_EvictAgentKvCacheResponse.Name, &kTable_Tryll_EvictAgentKvCacheResponse },
        { kTable_Tryll_ExportDialogRequest.Name, &kTable_Tryll_ExportDialogRequest },
        { kTable_Tryll_ExportDialogResponse.Name, &kTable_Tryll_ExportDialogResponse },
        { kTable_Tryll_ExportMutableKbRequest.Name, &kTable_Tryll_ExportMutableKbRequest },
        { kTable_Tryll_ExportMutableKbResponse.Name, &kTable_Tryll_ExportMutableKbResponse },
        { kTable_Tryll_GameMemory.Name, &kTable_Tryll_GameMemory },
        { kTable_Tryll_GetAgentKvCacheStatusRequest.Name, &kTable_Tryll_GetAgentKvCacheStatusRequest },
        { kTable_Tryll_GetAgentKvCacheStatusResponse.Name, &kTable_Tryll_GetAgentKvCacheStatusResponse },
        { kTable_Tryll_GetAgentVariablesRequest.Name, &kTable_Tryll_GetAgentVariablesRequest },
        { kTable_Tryll_GetAgentVariablesResponse.Name, &kTable_Tryll_GetAgentVariablesResponse },
        { kTable_Tryll_GetMemoryConsumptionRequest.Name, &kTable_Tryll_GetMemoryConsumptionRequest },
        { kTable_Tryll_GetMemoryConsumptionResponse.Name, &kTable_Tryll_GetMemoryConsumptionResponse },
        { kTable_Tryll_GraphDescription.Name, &kTable_Tryll_GraphDescription },
        { kTable_Tryll_ImportDialogRequest.Name, &kTable_Tryll_ImportDialogRequest },
        { kTable_Tryll_ImportDialogResponse.Name, &kTable_Tryll_ImportDialogResponse },
        { kTable_Tryll_ImportMutableKbRequest.Name, &kTable_Tryll_ImportMutableKbRequest },
        { kTable_Tryll_ImportMutableKbResponse.Name, &kTable_Tryll_ImportMutableKbResponse },
        { kTable_Tryll_ListModelsRequest.Name, &kTable_Tryll_ListModelsRequest },
        { kTable_Tryll_ListModelsResponse.Name, &kTable_Tryll_ListModelsResponse },
        { kTable_Tryll_LoadDialogRequest.Name, &kTable_Tryll_LoadDialogRequest },
        { kTable_Tryll_LoadDialogResponse.Name, &kTable_Tryll_LoadDialogResponse },
        { kTable_Tryll_LoadGameMemoryRequest.Name, &kTable_Tryll_LoadGameMemoryRequest },
        { kTable_Tryll_LoadGameMemoryResponse.Name, &kTable_Tryll_LoadGameMemoryResponse },
        { kTable_Tryll_LoadModelRequest.Name, &kTable_Tryll_LoadModelRequest },
        { kTable_Tryll_LoadModelResponse.Name, &kTable_Tryll_LoadModelResponse },
        { kTable_Tryll_LogCaptureGap.Name, &kTable_Tryll_LogCaptureGap },
        { kTable_Tryll_LogConnectionState.Name, &kTable_Tryll_LogConnectionState },
        { kTable_Tryll_LogLocalError.Name, &kTable_Tryll_LogLocalError },
        { kTable_Tryll_MemoryComponent.Name, &kTable_Tryll_MemoryComponent },
        { kTable_Tryll_MemoryComponentDevice.Name, &kTable_Tryll_MemoryComponentDevice },
        { kTable_Tryll_MemoryDevice.Name, &kTable_Tryll_MemoryDevice },
        { kTable_Tryll_MemorySnapshot.Name, &kTable_Tryll_MemorySnapshot },
        { kTable_Tryll_ModelDependency.Name, &kTable_Tryll_ModelDependency },
        { kTable_Tryll_ModelInfo.Name, &kTable_Tryll_ModelInfo },
        { kTable_Tryll_ModelPlacementHint.Name, &kTable_Tryll_ModelPlacementHint },
        { kTable_Tryll_ModelSampling.Name, &kTable_Tryll_ModelSampling },
        { kTable_Tryll_MutableKbClearRequest.Name, &kTable_Tryll_MutableKbClearRequest },
        { kTable_Tryll_MutableKbClearResponse.Name, &kTable_Tryll_MutableKbClearResponse },
        { kTable_Tryll_MutableKbDeleteRequest.Name, &kTable_Tryll_MutableKbDeleteRequest },
        { kTable_Tryll_MutableKbDeleteResponse.Name, &kTable_Tryll_MutableKbDeleteResponse },
        { kTable_Tryll_MutableKbGetRequest.Name, &kTable_Tryll_MutableKbGetRequest },
        { kTable_Tryll_MutableKbGetResponse.Name, &kTable_Tryll_MutableKbGetResponse },
        { kTable_Tryll_MutableKbHit.Name, &kTable_Tryll_MutableKbHit },
        { kTable_Tryll_MutableKbInfo.Name, &kTable_Tryll_MutableKbInfo },
        { kTable_Tryll_MutableKbListRequest.Name, &kTable_Tryll_MutableKbListRequest },
        { kTable_Tryll_MutableKbListResponse.Name, &kTable_Tryll_MutableKbListResponse },
        { kTable_Tryll_MutableKbQueryRequest.Name, &kTable_Tryll_MutableKbQueryRequest },
        { kTable_Tryll_MutableKbQueryResponse.Name, &kTable_Tryll_MutableKbQueryResponse },
        { kTable_Tryll_MutableKbRecord.Name, &kTable_Tryll_MutableKbRecord },
        { kTable_Tryll_MutableKbStatsRequest.Name, &kTable_Tryll_MutableKbStatsRequest },
        { kTable_Tryll_MutableKbStatsResponse.Name, &kTable_Tryll_MutableKbStatsResponse },
        { kTable_Tryll_MutableKbUpsert.Name, &kTable_Tryll_MutableKbUpsert },
        { kTable_Tryll_MutableKbWriteRequest.Name, &kTable_Tryll_MutableKbWriteRequest },
        { kTable_Tryll_MutableKbWriteResponse.Name, &kTable_Tryll_MutableKbWriteResponse },
        { kTable_Tryll_MutableKbWriteResult.Name, &kTable_Tryll_MutableKbWriteResult },
        { kTable_Tryll_NodeDescription.Name, &kTable_Tryll_NodeDescription },
        { kTable_Tryll_NodeEvent.Name, &kTable_Tryll_NodeEvent },
        { kTable_Tryll_NodeParam.Name, &kTable_Tryll_NodeParam },
        { kTable_Tryll_NodeParams_BranchParams.Name, &kTable_Tryll_NodeParams_BranchParams },
        { kTable_Tryll_NodeParams_CannedResponseParams.Name, &kTable_Tryll_NodeParams_CannedResponseParams },
        { kTable_Tryll_NodeParams_ClassifyAttributeChoice.Name, &kTable_Tryll_NodeParams_ClassifyAttributeChoice },
        { kTable_Tryll_NodeParams_ClassifyAttributeDef.Name, &kTable_Tryll_NodeParams_ClassifyAttributeDef },
        { kTable_Tryll_NodeParams_ClassifyAttributesLLMParams.Name, &kTable_Tryll_NodeParams_ClassifyAttributesLLMParams },
        { kTable_Tryll_NodeParams_ClassifyIntentEntry.Name, &kTable_Tryll_NodeParams_ClassifyIntentEntry },
        { kTable_Tryll_NodeParams_ClassifyIntentLLMParams.Name, &kTable_Tryll_NodeParams_ClassifyIntentLLMParams },
        { kTable_Tryll_NodeParams_ClassifyIntentParams.Name, &kTable_Tryll_NodeParams_ClassifyIntentParams },
        { kTable_Tryll_NodeParams_ContextOverrides.Name, &kTable_Tryll_NodeParams_ContextOverrides },
        { kTable_Tryll_NodeParams_GenerateAndSpeakParams.Name, &kTable_Tryll_NodeParams_GenerateAndSpeakParams },
        { kTable_Tryll_NodeParams_GenerateParams.Name, &kTable_Tryll_NodeParams_GenerateParams },
        { kTable_Tryll_NodeParams_InstructionParams.Name, &kTable_Tryll_NodeParams_InstructionParams },
        { kTable_Tryll_NodeParams_IntentToInstructionParams.Name, &kTable_Tryll_NodeParams_IntentToInstructionParams },
        { kTable_Tryll_NodeParams_LocalizableText.Name, &kTable_Tryll_NodeParams_LocalizableText },
        { kTable_Tryll_NodeParams_LoraAdapter.Name, &kTable_Tryll_NodeParams_LoraAdapter },
        { kTable_Tryll_NodeParams_ModelAdaptation.Name, &kTable_Tryll_NodeParams_ModelAdaptation },
        { kTable_Tryll_NodeParams_OutputFilterOverrides.Name, &kTable_Tryll_NodeParams_OutputFilterOverrides },
        { kTable_Tryll_NodeParams_PauseParams.Name, &kTable_Tryll_NodeParams_PauseParams },
        { kTable_Tryll_NodeParams_RegexGuardrailParams.Name, &kTable_Tryll_NodeParams_RegexGuardrailParams },
        { kTable_Tryll_NodeParams_RetrieveParams.Name, &kTable_Tryll_NodeParams_RetrieveParams },
        { kTable_Tryll_NodeParams_SamplingOverrides.Name, &kTable_Tryll_NodeParams_SamplingOverrides },
        { kTable_Tryll_NodeParams_SpeakParams.Name, &kTable_Tryll_NodeParams_SpeakParams },
        { kTable_Tryll_NodeParams_SummarizeParams.Name, &kTable_Tryll_NodeParams_SummarizeParams },
        { kTable_Tryll_NodeParams_ToolCallParams.Name, &kTable_Tryll_NodeParams_ToolCallParams },
        { kTable_Tryll_NodeParams_ToolDefinition.Name, &kTable_Tryll_NodeParams_ToolDefinition },
        { kTable_Tryll_NodeParams_ToolParamDefinition.Name, &kTable_Tryll_NodeParams_ToolParamDefinition },
        { kTable_Tryll_NodeParams_TransformParams.Name, &kTable_Tryll_NodeParams_TransformParams },
        { kTable_Tryll_NodeParams_WriteToMemoryParams.Name, &kTable_Tryll_NodeParams_WriteToMemoryParams },
        { kTable_Tryll_PersistedComponentEntry.Name, &kTable_Tryll_PersistedComponentEntry },
        { kTable_Tryll_PersistedDialogSnapshot.Name, &kTable_Tryll_PersistedDialogSnapshot },
        { kTable_Tryll_PersistedIntention.Name, &kTable_Tryll_PersistedIntention },
        { kTable_Tryll_PersistedInteraction.Name, &kTable_Tryll_PersistedInteraction },
        { kTable_Tryll_PersistedKbRecord.Name, &kTable_Tryll_PersistedKbRecord },
        { kTable_Tryll_PersistedKnowledge.Name, &kTable_Tryll_PersistedKnowledge },
        { kTable_Tryll_PersistedKnowledgeChunk.Name, &kTable_Tryll_PersistedKnowledgeChunk },
        { kTable_Tryll_PersistedMutableKb.Name, &kTable_Tryll_PersistedMutableKb },
        { kTable_Tryll_PersistedProjectionWindow.Name, &kTable_Tryll_PersistedProjectionWindow },
        { kTable_Tryll_PersistedSlot.Name, &kTable_Tryll_PersistedSlot },
        { kTable_Tryll_PersistedStringPair.Name, &kTable_Tryll_PersistedStringPair },
        { kTable_Tryll_PersistedSummary.Name, &kTable_Tryll_PersistedSummary },
        { kTable_Tryll_PersistedToolCall.Name, &kTable_Tryll_PersistedToolCall },
        { kTable_Tryll_PersistedToolResult.Name, &kTable_Tryll_PersistedToolResult },
        { kTable_Tryll_PrefillAgentKvCacheRequest.Name, &kTable_Tryll_PrefillAgentKvCacheRequest },
        { kTable_Tryll_PrefillAgentKvCacheResponse.Name, &kTable_Tryll_PrefillAgentKvCacheResponse },
        { kTable_Tryll_RemoveInteractionsFromEndRequest.Name, &kTable_Tryll_RemoveInteractionsFromEndRequest },
        { kTable_Tryll_RemoveInteractionsFromEndResponse.Name, &kTable_Tryll_RemoveInteractionsFromEndResponse },
        { kTable_Tryll_ResumeAgentRequest.Name, &kTable_Tryll_ResumeAgentRequest },
        { kTable_Tryll_SaveDialogRequest.Name, &kTable_Tryll_SaveDialogRequest },
        { kTable_Tryll_SaveDialogResponse.Name, &kTable_Tryll_SaveDialogResponse },
        { kTable_Tryll_SaveGameMemoryRequest.Name, &kTable_Tryll_SaveGameMemoryRequest },
        { kTable_Tryll_SaveGameMemoryResponse.Name, &kTable_Tryll_SaveGameMemoryResponse },
        { kTable_Tryll_SendMessageRequest.Name, &kTable_Tryll_SendMessageRequest },
        { kTable_Tryll_SessionModelConfig.Name, &kTable_Tryll_SessionModelConfig },
        { kTable_Tryll_SetAgentDeliveryModeRequest.Name, &kTable_Tryll_SetAgentDeliveryModeRequest },
        { kTable_Tryll_SetAgentVoiceRequest.Name, &kTable_Tryll_SetAgentVoiceRequest },
        { kTable_Tryll_SetInferenceThrottle.Name, &kTable_Tryll_SetInferenceThrottle },
        { kTable_Tryll_SetSessionLocaleRequest.Name, &kTable_Tryll_SetSessionLocaleRequest },
        { kTable_Tryll_ToolResult.Name, &kTable_Tryll_ToolResult },
        { kTable_Tryll_TtsAudioFormatFrame.Name, &kTable_Tryll_TtsAudioFormatFrame },
        { kTable_Tryll_TtsAudioFrame.Name, &kTable_Tryll_TtsAudioFrame },
        { kTable_Tryll_TtsCapabilitiesInfo.Name, &kTable_Tryll_TtsCapabilitiesInfo },
        { kTable_Tryll_TtsInlineTagInfo.Name, &kTable_Tryll_TtsInlineTagInfo },
        { kTable_Tryll_TtsVisemeFrame.Name, &kTable_Tryll_TtsVisemeFrame },
        { kTable_Tryll_TtsVoicePresetInfo.Name, &kTable_Tryll_TtsVoicePresetInfo },
        { kTable_Tryll_TurnComplete.Name, &kTable_Tryll_TurnComplete },
        { kTable_Tryll_UnloadModelRequest.Name, &kTable_Tryll_UnloadModelRequest },
        { kTable_Tryll_UpdateAgentVariablesRequest.Name, &kTable_Tryll_UpdateAgentVariablesRequest },
        { kTable_Tryll_VarBool.Name, &kTable_Tryll_VarBool },
        { kTable_Tryll_VarFloat.Name, &kTable_Tryll_VarFloat },
        { kTable_Tryll_VarInt.Name, &kTable_Tryll_VarInt },
        { kTable_Tryll_VarString.Name, &kTable_Tryll_VarString },
        { kTable_Tryll_VarStringSet.Name, &kTable_Tryll_VarStringSet },
        { kTable_Tryll_VariableUpdate.Name, &kTable_Tryll_VariableUpdate },
        { kTable_Tryll_VisemeEvent.Name, &kTable_Tryll_VisemeEvent },
        { kTable_Tryll_VisemeEventList.Name, &kTable_Tryll_VisemeEventList },
        { kTable_Tryll_VoiceProfile.Name, &kTable_Tryll_VoiceProfile },
        { kTable_Tryll_WireAudioFormat.Name, &kTable_Tryll_WireAudioFormat },
        { kTable_Tryll_WireTranscriptUpdate.Name, &kTable_Tryll_WireTranscriptUpdate },
    };
    for (const auto& Row : kRows)
    {
        if (std::strcmp(Row.first, Name) == 0) return Row.second;
    }
    return nullptr;
}

} // namespace Tryll::AgentLog

