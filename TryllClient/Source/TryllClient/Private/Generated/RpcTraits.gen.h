// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/messages.fbs  +  tools/codegen/templates/protocol_rpc_traits_cpp.j2
// See: docs/dev/codegen.md
//
// Request → declared terminal / progress. Ack is a real MessageBody value.
// A request with no (rpc:) cannot occur: codegen rejects the schema.

#pragma once

#include "messages_generated.h"

#include "generated/CodegenFingerprint.gen.h"

#include <string_view>
namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_RpcTraits_gen = void;
}

namespace Tryll {

[[nodiscard]] inline ::Tryll::MessageBody
RpcTerminalOf(::Tryll::MessageBody request) noexcept
{
    switch (request)
    {
    case ::Tryll::MessageBody_CreateSessionRequest:
        return ::Tryll::MessageBody_CreateSessionResponse;
    case ::Tryll::MessageBody_CreateAgentRequest:
        return ::Tryll::MessageBody_CreateAgentResponse;
    case ::Tryll::MessageBody_SendMessageRequest:
        return ::Tryll::MessageBody_TurnComplete;
    case ::Tryll::MessageBody_DestroyAgentRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_ListModelsRequest:
        return ::Tryll::MessageBody_ListModelsResponse;
    case ::Tryll::MessageBody_DownloadModelRequest:
        return ::Tryll::MessageBody_DownloadComplete;
    case ::Tryll::MessageBody_CreateStringStorageRequest:
        return ::Tryll::MessageBody_CreateStringStorageResponse;
    case ::Tryll::MessageBody_DestroyStringStorageRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_LoadModelRequest:
        return ::Tryll::MessageBody_LoadModelResponse;
    case ::Tryll::MessageBody_UnloadModelRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest:
        return ::Tryll::MessageBody_CreateEmbeddedStringStorageResponse;
    case ::Tryll::MessageBody_DestroyEmbeddedStringStorageRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_ChangeAgentParamRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_CreateVoiceInputRequest:
        return ::Tryll::MessageBody_CreateVoiceInputResponse;
    case ::Tryll::MessageBody_BeginUtteranceRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_EndUtteranceRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_CancelUtteranceRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_DestroyVoiceInputRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_DeleteModelRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_DebugCommandRequest:
        return ::Tryll::MessageBody_DebugCommandResponse;
    case ::Tryll::MessageBody_CancelRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_ResumeAgentRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_UpdateAgentVariablesRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_GetAgentVariablesRequest:
        return ::Tryll::MessageBody_GetAgentVariablesResponse;
    case ::Tryll::MessageBody_PrefillAgentKvCacheRequest:
        return ::Tryll::MessageBody_PrefillAgentKvCacheResponse;
    case ::Tryll::MessageBody_EvictAgentKvCacheRequest:
        return ::Tryll::MessageBody_EvictAgentKvCacheResponse;
    case ::Tryll::MessageBody_GetAgentKvCacheStatusRequest:
        return ::Tryll::MessageBody_GetAgentKvCacheStatusResponse;
    case ::Tryll::MessageBody_AppendInteractionsRequest:
        return ::Tryll::MessageBody_AppendInteractionsResponse;
    case ::Tryll::MessageBody_RemoveInteractionsFromEndRequest:
        return ::Tryll::MessageBody_RemoveInteractionsFromEndResponse;
    case ::Tryll::MessageBody_SetSessionLocaleRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_ExportDialogRequest:
        return ::Tryll::MessageBody_ExportDialogResponse;
    case ::Tryll::MessageBody_ImportDialogRequest:
        return ::Tryll::MessageBody_ImportDialogResponse;
    case ::Tryll::MessageBody_GetMemoryConsumptionRequest:
        return ::Tryll::MessageBody_GetMemoryConsumptionResponse;
    case ::Tryll::MessageBody_EnsureMutableKbRequest:
        return ::Tryll::MessageBody_EnsureMutableKbResponse;
    case ::Tryll::MessageBody_EnumerateMutableKbsRequest:
        return ::Tryll::MessageBody_EnumerateMutableKbsResponse;
    case ::Tryll::MessageBody_MutableKbWriteRequest:
        return ::Tryll::MessageBody_MutableKbWriteResponse;
    case ::Tryll::MessageBody_MutableKbDeleteRequest:
        return ::Tryll::MessageBody_MutableKbDeleteResponse;
    case ::Tryll::MessageBody_MutableKbClearRequest:
        return ::Tryll::MessageBody_MutableKbClearResponse;
    case ::Tryll::MessageBody_MutableKbQueryRequest:
        return ::Tryll::MessageBody_MutableKbQueryResponse;
    case ::Tryll::MessageBody_MutableKbGetRequest:
        return ::Tryll::MessageBody_MutableKbGetResponse;
    case ::Tryll::MessageBody_MutableKbListRequest:
        return ::Tryll::MessageBody_MutableKbListResponse;
    case ::Tryll::MessageBody_MutableKbStatsRequest:
        return ::Tryll::MessageBody_MutableKbStatsResponse;
    case ::Tryll::MessageBody_ExportMutableKbRequest:
        return ::Tryll::MessageBody_ExportMutableKbResponse;
    case ::Tryll::MessageBody_ImportMutableKbRequest:
        return ::Tryll::MessageBody_ImportMutableKbResponse;
    case ::Tryll::MessageBody_SaveGameMemoryRequest:
        return ::Tryll::MessageBody_SaveGameMemoryResponse;
    case ::Tryll::MessageBody_LoadGameMemoryRequest:
        return ::Tryll::MessageBody_LoadGameMemoryResponse;
    case ::Tryll::MessageBody_SaveDialogRequest:
        return ::Tryll::MessageBody_SaveDialogResponse;
    case ::Tryll::MessageBody_LoadDialogRequest:
        return ::Tryll::MessageBody_LoadDialogResponse;
    case ::Tryll::MessageBody_SetAgentVoiceRequest:
        return ::Tryll::MessageBody_Ack;
    case ::Tryll::MessageBody_SetAgentDeliveryModeRequest:
        return ::Tryll::MessageBody_Ack;
    default:
        return ::Tryll::MessageBody_NONE;
    }
}

[[nodiscard]] inline ::Tryll::MessageBody
RpcProgressOf(::Tryll::MessageBody request) noexcept
{
    switch (request)
    {
    case ::Tryll::MessageBody_SendMessageRequest:
        return ::Tryll::MessageBody_AnswerText;
    case ::Tryll::MessageBody_DownloadModelRequest:
        return ::Tryll::MessageBody_DownloadProgress;
    default:
        return ::Tryll::MessageBody_NONE;
    }
}

[[nodiscard]] inline bool
RpcIsUnary(::Tryll::MessageBody request) noexcept
{
    switch (request)
    {
    case ::Tryll::MessageBody_CreateSessionRequest:
    case ::Tryll::MessageBody_CreateAgentRequest:
    case ::Tryll::MessageBody_DestroyAgentRequest:
    case ::Tryll::MessageBody_ListModelsRequest:
    case ::Tryll::MessageBody_CreateStringStorageRequest:
    case ::Tryll::MessageBody_DestroyStringStorageRequest:
    case ::Tryll::MessageBody_LoadModelRequest:
    case ::Tryll::MessageBody_UnloadModelRequest:
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest:
    case ::Tryll::MessageBody_DestroyEmbeddedStringStorageRequest:
    case ::Tryll::MessageBody_ChangeAgentParamRequest:
    case ::Tryll::MessageBody_CreateVoiceInputRequest:
    case ::Tryll::MessageBody_BeginUtteranceRequest:
    case ::Tryll::MessageBody_EndUtteranceRequest:
    case ::Tryll::MessageBody_CancelUtteranceRequest:
    case ::Tryll::MessageBody_DestroyVoiceInputRequest:
    case ::Tryll::MessageBody_DeleteModelRequest:
    case ::Tryll::MessageBody_DebugCommandRequest:
    case ::Tryll::MessageBody_CancelRequest:
    case ::Tryll::MessageBody_ResumeAgentRequest:
    case ::Tryll::MessageBody_UpdateAgentVariablesRequest:
    case ::Tryll::MessageBody_GetAgentVariablesRequest:
    case ::Tryll::MessageBody_PrefillAgentKvCacheRequest:
    case ::Tryll::MessageBody_EvictAgentKvCacheRequest:
    case ::Tryll::MessageBody_GetAgentKvCacheStatusRequest:
    case ::Tryll::MessageBody_AppendInteractionsRequest:
    case ::Tryll::MessageBody_RemoveInteractionsFromEndRequest:
    case ::Tryll::MessageBody_SetSessionLocaleRequest:
    case ::Tryll::MessageBody_ExportDialogRequest:
    case ::Tryll::MessageBody_ImportDialogRequest:
    case ::Tryll::MessageBody_GetMemoryConsumptionRequest:
    case ::Tryll::MessageBody_EnsureMutableKbRequest:
    case ::Tryll::MessageBody_EnumerateMutableKbsRequest:
    case ::Tryll::MessageBody_MutableKbWriteRequest:
    case ::Tryll::MessageBody_MutableKbDeleteRequest:
    case ::Tryll::MessageBody_MutableKbClearRequest:
    case ::Tryll::MessageBody_MutableKbQueryRequest:
    case ::Tryll::MessageBody_MutableKbGetRequest:
    case ::Tryll::MessageBody_MutableKbListRequest:
    case ::Tryll::MessageBody_MutableKbStatsRequest:
    case ::Tryll::MessageBody_ExportMutableKbRequest:
    case ::Tryll::MessageBody_ImportMutableKbRequest:
    case ::Tryll::MessageBody_SaveGameMemoryRequest:
    case ::Tryll::MessageBody_LoadGameMemoryRequest:
    case ::Tryll::MessageBody_SaveDialogRequest:
    case ::Tryll::MessageBody_LoadDialogRequest:
    case ::Tryll::MessageBody_SetAgentVoiceRequest:
    case ::Tryll::MessageBody_SetAgentDeliveryModeRequest:
        return true;
    default:
        return false;
    }
}

/// A streaming frame that carries a request id but is not that request's
/// terminal or progress reply.
[[nodiscard]] inline bool
IsRequestScopedPush(::Tryll::MessageBody body) noexcept
{
    switch (body)
    {
    case ::Tryll::MessageBody_TtsAudioFormatFrame:
    case ::Tryll::MessageBody_TtsAudioFrame:
    case ::Tryll::MessageBody_TtsVisemeFrame:
        return true;
    default:
        return false;
    }
}

} // namespace Tryll
