// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/messages.fbs  +  tools/codegen/templates/protocol_request_id_cpp.j2
// See: docs/dev/codegen.md
//
// Provides Tryll::ExtractRequestId(const ::Tryll::Message*) — a single
// generated switch that covers every MessageBody member whose schema carries a
// request_id field AND participates in pending-lambda correlation.  Pure
// Notifications (SessionReady, NodeEvent, WireTranscriptUpdate) and
// FireAndForget messages (AudioBuffer) are excluded and return 0 via default.
//
// Delete the hand-written copy in FbbHelpers.h and any duplicates; include
// this header instead.

#pragma once

#include <flatbuffers/flatbuffers.h>
#include "messages_generated.h"

#include <cstdint>

#include "generated/CodegenFingerprint.gen.h"

#include <string_view>
namespace {
static_assert(
    std::string_view("b5fd85e0d11e") == std::string_view(::Tryll::Generated::kCodegenFingerprint),
    "Tryll codegen inconsistent: this file was generated against a different fingerprint than "
    "CodegenFingerprint.gen.h. You likely ran a partial codegen subcommand. "
    "Run cmake --build in tryll/ or tryll_codegen.py all.");
using _TryllFpCheck_RequestId_gen = void;
}

namespace Tryll {

/// Return the \c request_id carried by \p msg, or 0 for messages that carry
/// no request_id (SessionReady, NodeEvent, WireTranscriptUpdate, AudioBuffer).
///
/// ErrorResponse is handled separately by callers before reaching this helper;
/// do not pass it here (the result is still correct, but context is lost).
///
/// Precondition: the buffer has already passed \c VerifyMessageBuffer.
[[nodiscard]] inline std::uint64_t
ExtractRequestId(const ::Tryll::Message* msg) noexcept
{
    if (!msg) return 0;
    switch (msg->body_type())
    {
    case ::Tryll::MessageBody_CreateSessionRequest:
        return msg->body_as<::Tryll::CreateSessionRequest>()->request_id();
    case ::Tryll::MessageBody_CreateSessionResponse:
        return msg->body_as<::Tryll::CreateSessionResponse>()->request_id();
    case ::Tryll::MessageBody_CreateAgentRequest:
        return msg->body_as<::Tryll::CreateAgentRequest>()->request_id();
    case ::Tryll::MessageBody_CreateAgentResponse:
        return msg->body_as<::Tryll::CreateAgentResponse>()->request_id();
    case ::Tryll::MessageBody_SendMessageRequest:
        return msg->body_as<::Tryll::SendMessageRequest>()->request_id();
    case ::Tryll::MessageBody_DestroyAgentRequest:
        return msg->body_as<::Tryll::DestroyAgentRequest>()->request_id();
    case ::Tryll::MessageBody_AnswerText:
        return msg->body_as<::Tryll::AnswerText>()->request_id();
    case ::Tryll::MessageBody_TurnComplete:
        return msg->body_as<::Tryll::TurnComplete>()->request_id();
    case ::Tryll::MessageBody_ErrorResponse:
        return msg->body_as<::Tryll::ErrorResponse>()->request_id();
    case ::Tryll::MessageBody_Ack:
        return msg->body_as<::Tryll::Ack>()->request_id();
    case ::Tryll::MessageBody_ListModelsRequest:
        return msg->body_as<::Tryll::ListModelsRequest>()->request_id();
    case ::Tryll::MessageBody_ListModelsResponse:
        return msg->body_as<::Tryll::ListModelsResponse>()->request_id();
    case ::Tryll::MessageBody_DownloadModelRequest:
        return msg->body_as<::Tryll::DownloadModelRequest>()->request_id();
    case ::Tryll::MessageBody_DownloadProgress:
        return msg->body_as<::Tryll::DownloadProgress>()->request_id();
    case ::Tryll::MessageBody_DownloadComplete:
        return msg->body_as<::Tryll::DownloadComplete>()->request_id();
    case ::Tryll::MessageBody_CreateStringStorageRequest:
        return msg->body_as<::Tryll::CreateStringStorageRequest>()->request_id();
    case ::Tryll::MessageBody_CreateStringStorageResponse:
        return msg->body_as<::Tryll::CreateStringStorageResponse>()->request_id();
    case ::Tryll::MessageBody_DestroyStringStorageRequest:
        return msg->body_as<::Tryll::DestroyStringStorageRequest>()->request_id();
    case ::Tryll::MessageBody_LoadModelRequest:
        return msg->body_as<::Tryll::LoadModelRequest>()->request_id();
    case ::Tryll::MessageBody_LoadModelResponse:
        return msg->body_as<::Tryll::LoadModelResponse>()->request_id();
    case ::Tryll::MessageBody_UnloadModelRequest:
        return msg->body_as<::Tryll::UnloadModelRequest>()->request_id();
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest:
        return msg->body_as<::Tryll::CreateEmbeddedStringStorageRequest>()->request_id();
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageResponse:
        return msg->body_as<::Tryll::CreateEmbeddedStringStorageResponse>()->request_id();
    case ::Tryll::MessageBody_DestroyEmbeddedStringStorageRequest:
        return msg->body_as<::Tryll::DestroyEmbeddedStringStorageRequest>()->request_id();
    case ::Tryll::MessageBody_ChangeAgentParamRequest:
        return msg->body_as<::Tryll::ChangeAgentParamRequest>()->request_id();
    case ::Tryll::MessageBody_CreateVoiceInputRequest:
        return msg->body_as<::Tryll::CreateVoiceInputRequest>()->request_id();
    case ::Tryll::MessageBody_CreateVoiceInputResponse:
        return msg->body_as<::Tryll::CreateVoiceInputResponse>()->request_id();
    case ::Tryll::MessageBody_BeginUtteranceRequest:
        return msg->body_as<::Tryll::BeginUtteranceRequest>()->request_id();
    case ::Tryll::MessageBody_EndUtteranceRequest:
        return msg->body_as<::Tryll::EndUtteranceRequest>()->request_id();
    case ::Tryll::MessageBody_CancelUtteranceRequest:
        return msg->body_as<::Tryll::CancelUtteranceRequest>()->request_id();
    case ::Tryll::MessageBody_DestroyVoiceInputRequest:
        return msg->body_as<::Tryll::DestroyVoiceInputRequest>()->request_id();
    case ::Tryll::MessageBody_TtsAudioFormatFrame:
        return msg->body_as<::Tryll::TtsAudioFormatFrame>()->request_id();
    case ::Tryll::MessageBody_TtsAudioFrame:
        return msg->body_as<::Tryll::TtsAudioFrame>()->request_id();
    case ::Tryll::MessageBody_DeleteModelRequest:
        return msg->body_as<::Tryll::DeleteModelRequest>()->request_id();
    case ::Tryll::MessageBody_DebugCommandRequest:
        return msg->body_as<::Tryll::DebugCommandRequest>()->request_id();
    case ::Tryll::MessageBody_DebugCommandResponse:
        return msg->body_as<::Tryll::DebugCommandResponse>()->request_id();
    case ::Tryll::MessageBody_CancelRequest:
        return msg->body_as<::Tryll::CancelRequest>()->request_id();
    case ::Tryll::MessageBody_ResumeAgentRequest:
        return msg->body_as<::Tryll::ResumeAgentRequest>()->request_id();
    case ::Tryll::MessageBody_UpdateAgentVariablesRequest:
        return msg->body_as<::Tryll::UpdateAgentVariablesRequest>()->request_id();
    case ::Tryll::MessageBody_GetAgentVariablesRequest:
        return msg->body_as<::Tryll::GetAgentVariablesRequest>()->request_id();
    case ::Tryll::MessageBody_GetAgentVariablesResponse:
        return msg->body_as<::Tryll::GetAgentVariablesResponse>()->request_id();
    case ::Tryll::MessageBody_PrefillAgentKvCacheRequest:
        return msg->body_as<::Tryll::PrefillAgentKvCacheRequest>()->request_id();
    case ::Tryll::MessageBody_PrefillAgentKvCacheResponse:
        return msg->body_as<::Tryll::PrefillAgentKvCacheResponse>()->request_id();
    case ::Tryll::MessageBody_EvictAgentKvCacheRequest:
        return msg->body_as<::Tryll::EvictAgentKvCacheRequest>()->request_id();
    case ::Tryll::MessageBody_EvictAgentKvCacheResponse:
        return msg->body_as<::Tryll::EvictAgentKvCacheResponse>()->request_id();
    case ::Tryll::MessageBody_GetAgentKvCacheStatusRequest:
        return msg->body_as<::Tryll::GetAgentKvCacheStatusRequest>()->request_id();
    case ::Tryll::MessageBody_GetAgentKvCacheStatusResponse:
        return msg->body_as<::Tryll::GetAgentKvCacheStatusResponse>()->request_id();
    case ::Tryll::MessageBody_AppendInteractionsRequest:
        return msg->body_as<::Tryll::AppendInteractionsRequest>()->request_id();
    case ::Tryll::MessageBody_AppendInteractionsResponse:
        return msg->body_as<::Tryll::AppendInteractionsResponse>()->request_id();
    case ::Tryll::MessageBody_RemoveInteractionsFromEndRequest:
        return msg->body_as<::Tryll::RemoveInteractionsFromEndRequest>()->request_id();
    case ::Tryll::MessageBody_RemoveInteractionsFromEndResponse:
        return msg->body_as<::Tryll::RemoveInteractionsFromEndResponse>()->request_id();
    case ::Tryll::MessageBody_SetSessionLocaleRequest:
        return msg->body_as<::Tryll::SetSessionLocaleRequest>()->request_id();
    case ::Tryll::MessageBody_ExportDialogRequest:
        return msg->body_as<::Tryll::ExportDialogRequest>()->request_id();
    case ::Tryll::MessageBody_ExportDialogResponse:
        return msg->body_as<::Tryll::ExportDialogResponse>()->request_id();
    case ::Tryll::MessageBody_ImportDialogRequest:
        return msg->body_as<::Tryll::ImportDialogRequest>()->request_id();
    case ::Tryll::MessageBody_ImportDialogResponse:
        return msg->body_as<::Tryll::ImportDialogResponse>()->request_id();
    case ::Tryll::MessageBody_GetMemoryConsumptionRequest:
        return msg->body_as<::Tryll::GetMemoryConsumptionRequest>()->request_id();
    case ::Tryll::MessageBody_GetMemoryConsumptionResponse:
        return msg->body_as<::Tryll::GetMemoryConsumptionResponse>()->request_id();
    case ::Tryll::MessageBody_EnsureMutableKbRequest:
        return msg->body_as<::Tryll::EnsureMutableKbRequest>()->request_id();
    case ::Tryll::MessageBody_EnsureMutableKbResponse:
        return msg->body_as<::Tryll::EnsureMutableKbResponse>()->request_id();
    case ::Tryll::MessageBody_EnumerateMutableKbsRequest:
        return msg->body_as<::Tryll::EnumerateMutableKbsRequest>()->request_id();
    case ::Tryll::MessageBody_EnumerateMutableKbsResponse:
        return msg->body_as<::Tryll::EnumerateMutableKbsResponse>()->request_id();
    case ::Tryll::MessageBody_MutableKbWriteRequest:
        return msg->body_as<::Tryll::MutableKbWriteRequest>()->request_id();
    case ::Tryll::MessageBody_MutableKbWriteResponse:
        return msg->body_as<::Tryll::MutableKbWriteResponse>()->request_id();
    case ::Tryll::MessageBody_MutableKbDeleteRequest:
        return msg->body_as<::Tryll::MutableKbDeleteRequest>()->request_id();
    case ::Tryll::MessageBody_MutableKbDeleteResponse:
        return msg->body_as<::Tryll::MutableKbDeleteResponse>()->request_id();
    case ::Tryll::MessageBody_MutableKbClearRequest:
        return msg->body_as<::Tryll::MutableKbClearRequest>()->request_id();
    case ::Tryll::MessageBody_MutableKbClearResponse:
        return msg->body_as<::Tryll::MutableKbClearResponse>()->request_id();
    case ::Tryll::MessageBody_MutableKbQueryRequest:
        return msg->body_as<::Tryll::MutableKbQueryRequest>()->request_id();
    case ::Tryll::MessageBody_MutableKbQueryResponse:
        return msg->body_as<::Tryll::MutableKbQueryResponse>()->request_id();
    case ::Tryll::MessageBody_MutableKbGetRequest:
        return msg->body_as<::Tryll::MutableKbGetRequest>()->request_id();
    case ::Tryll::MessageBody_MutableKbGetResponse:
        return msg->body_as<::Tryll::MutableKbGetResponse>()->request_id();
    case ::Tryll::MessageBody_MutableKbListRequest:
        return msg->body_as<::Tryll::MutableKbListRequest>()->request_id();
    case ::Tryll::MessageBody_MutableKbListResponse:
        return msg->body_as<::Tryll::MutableKbListResponse>()->request_id();
    case ::Tryll::MessageBody_MutableKbStatsRequest:
        return msg->body_as<::Tryll::MutableKbStatsRequest>()->request_id();
    case ::Tryll::MessageBody_MutableKbStatsResponse:
        return msg->body_as<::Tryll::MutableKbStatsResponse>()->request_id();
    case ::Tryll::MessageBody_ExportMutableKbRequest:
        return msg->body_as<::Tryll::ExportMutableKbRequest>()->request_id();
    case ::Tryll::MessageBody_ExportMutableKbResponse:
        return msg->body_as<::Tryll::ExportMutableKbResponse>()->request_id();
    case ::Tryll::MessageBody_ImportMutableKbRequest:
        return msg->body_as<::Tryll::ImportMutableKbRequest>()->request_id();
    case ::Tryll::MessageBody_ImportMutableKbResponse:
        return msg->body_as<::Tryll::ImportMutableKbResponse>()->request_id();
    case ::Tryll::MessageBody_SaveGameMemoryRequest:
        return msg->body_as<::Tryll::SaveGameMemoryRequest>()->request_id();
    case ::Tryll::MessageBody_SaveGameMemoryResponse:
        return msg->body_as<::Tryll::SaveGameMemoryResponse>()->request_id();
    case ::Tryll::MessageBody_LoadGameMemoryRequest:
        return msg->body_as<::Tryll::LoadGameMemoryRequest>()->request_id();
    case ::Tryll::MessageBody_LoadGameMemoryResponse:
        return msg->body_as<::Tryll::LoadGameMemoryResponse>()->request_id();
    case ::Tryll::MessageBody_SaveDialogRequest:
        return msg->body_as<::Tryll::SaveDialogRequest>()->request_id();
    case ::Tryll::MessageBody_SaveDialogResponse:
        return msg->body_as<::Tryll::SaveDialogResponse>()->request_id();
    case ::Tryll::MessageBody_LoadDialogRequest:
        return msg->body_as<::Tryll::LoadDialogRequest>()->request_id();
    case ::Tryll::MessageBody_LoadDialogResponse:
        return msg->body_as<::Tryll::LoadDialogResponse>()->request_id();
    case ::Tryll::MessageBody_TtsVisemeFrame:
        return msg->body_as<::Tryll::TtsVisemeFrame>()->request_id();
    case ::Tryll::MessageBody_SetAgentVoiceRequest:
        return msg->body_as<::Tryll::SetAgentVoiceRequest>()->request_id();
    case ::Tryll::MessageBody_SetAgentDeliveryModeRequest:
        return msg->body_as<::Tryll::SetAgentDeliveryModeRequest>()->request_id();
    default:
        return 0;
    }
}

/// True for client-to-server messages that carry no request_id and expect no reply.
/// The pre-session gate drops these instead of answering with ErrorResponse.
[[nodiscard]] inline constexpr bool
IsFireAndForget(::Tryll::MessageBody body) noexcept
{
    switch (body)
    {
    case ::Tryll::MessageBody_AudioBuffer:
    case ::Tryll::MessageBody_SetInferenceThrottle:
        return true;
    default:
        return false;
    }
}

} // namespace Tryll
