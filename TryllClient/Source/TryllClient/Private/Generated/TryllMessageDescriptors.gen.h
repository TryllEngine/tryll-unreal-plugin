// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/messages.fbs  +  tools/codegen/templates/protocol_observation_cpp.j2
// See: docs/dev/codegen.md
//
// One row per MessageBody member. Not a reflected type. Curated-vs-raw
// exceptions live in TryllObservationCurated.h.

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
using _TryllFpCheck_TryllMessageDescriptors_gen = void;
}

namespace Tryll {

struct FTryllMessageDescriptor
{
    ::Tryll::MessageBody Body;
    const char* Name;
    bool ClientToServer;
    const char* ScopeKey;
    ::Tryll::MessageBody Terminal;
    ::Tryll::MessageBody Progress;
    ::Tryll::MessageBody Request;
    const char* Coalesce;
    const char* CoalesceField;
};

[[nodiscard]] inline const FTryllMessageDescriptor*
MessageDescriptor(::Tryll::MessageBody body) noexcept
{
    switch (body)
    {
    case ::Tryll::MessageBody_ConnectionReady:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ConnectionReady,
            "ConnectionReady",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CreateSessionRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CreateSessionRequest,
            "CreateSessionRequest",
            true,
            "",
            ::Tryll::MessageBody_CreateSessionResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CreateSessionResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CreateSessionResponse,
            "CreateSessionResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_CreateSessionRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CreateAgentRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CreateAgentRequest,
            "CreateAgentRequest",
            true,
            "",
            ::Tryll::MessageBody_CreateAgentResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CreateAgentResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CreateAgentResponse,
            "CreateAgentResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_CreateAgentRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_SendMessageRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_SendMessageRequest,
            "SendMessageRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_TurnComplete,
            ::Tryll::MessageBody_AnswerText,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_DestroyAgentRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_DestroyAgentRequest,
            "DestroyAgentRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_AnswerText:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_AnswerText,
            "AnswerText",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_SendMessageRequest,
            "concat",
            "text",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_TurnComplete:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_TurnComplete,
            "TurnComplete",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_SendMessageRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ErrorResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ErrorResponse,
            "ErrorResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_Ack:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_Ack,
            "Ack",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ListModelsRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ListModelsRequest,
            "ListModelsRequest",
            true,
            "",
            ::Tryll::MessageBody_ListModelsResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ListModelsResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ListModelsResponse,
            "ListModelsResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_ListModelsRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_DownloadModelRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_DownloadModelRequest,
            "DownloadModelRequest",
            true,
            "",
            ::Tryll::MessageBody_DownloadComplete,
            ::Tryll::MessageBody_DownloadProgress,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_DownloadProgress:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_DownloadProgress,
            "DownloadProgress",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_DownloadModelRequest,
            "last",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_DownloadComplete:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_DownloadComplete,
            "DownloadComplete",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_DownloadModelRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CreateStringStorageRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CreateStringStorageRequest,
            "CreateStringStorageRequest",
            true,
            "",
            ::Tryll::MessageBody_CreateStringStorageResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CreateStringStorageResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CreateStringStorageResponse,
            "CreateStringStorageResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_CreateStringStorageRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_DestroyStringStorageRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_DestroyStringStorageRequest,
            "DestroyStringStorageRequest",
            true,
            "",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_NodeEvent:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_NodeEvent,
            "NodeEvent",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_LoadModelRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_LoadModelRequest,
            "LoadModelRequest",
            true,
            "",
            ::Tryll::MessageBody_LoadModelResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_LoadModelResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_LoadModelResponse,
            "LoadModelResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_LoadModelRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_UnloadModelRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_UnloadModelRequest,
            "UnloadModelRequest",
            true,
            "",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest,
            "CreateEmbeddedStringStorageRequest",
            true,
            "",
            ::Tryll::MessageBody_CreateEmbeddedStringStorageResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CreateEmbeddedStringStorageResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CreateEmbeddedStringStorageResponse,
            "CreateEmbeddedStringStorageResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_CreateEmbeddedStringStorageRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_DestroyEmbeddedStringStorageRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_DestroyEmbeddedStringStorageRequest,
            "DestroyEmbeddedStringStorageRequest",
            true,
            "",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ChangeAgentParamRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ChangeAgentParamRequest,
            "ChangeAgentParamRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CreateVoiceInputRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CreateVoiceInputRequest,
            "CreateVoiceInputRequest",
            true,
            "",
            ::Tryll::MessageBody_CreateVoiceInputResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CreateVoiceInputResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CreateVoiceInputResponse,
            "CreateVoiceInputResponse",
            false,
            "voice_input_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_CreateVoiceInputRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_BeginUtteranceRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_BeginUtteranceRequest,
            "BeginUtteranceRequest",
            true,
            "voice_input_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_AudioBuffer:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_AudioBuffer,
            "AudioBuffer",
            true,
            "voice_input_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "sum",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_EndUtteranceRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_EndUtteranceRequest,
            "EndUtteranceRequest",
            true,
            "voice_input_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CancelUtteranceRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CancelUtteranceRequest,
            "CancelUtteranceRequest",
            true,
            "voice_input_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_WireTranscriptUpdate:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_WireTranscriptUpdate,
            "WireTranscriptUpdate",
            false,
            "voice_input_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_DestroyVoiceInputRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_DestroyVoiceInputRequest,
            "DestroyVoiceInputRequest",
            true,
            "voice_input_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_TtsAudioFormatFrame:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_TtsAudioFormatFrame,
            "TtsAudioFormatFrame",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_TtsAudioFrame:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_TtsAudioFrame,
            "TtsAudioFrame",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "sum",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_DeleteModelRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_DeleteModelRequest,
            "DeleteModelRequest",
            true,
            "",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_DebugCommandRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_DebugCommandRequest,
            "DebugCommandRequest",
            true,
            "",
            ::Tryll::MessageBody_DebugCommandResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_DebugCommandResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_DebugCommandResponse,
            "DebugCommandResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_DebugCommandRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_CancelRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_CancelRequest,
            "CancelRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ResumeAgentRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ResumeAgentRequest,
            "ResumeAgentRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_UpdateAgentVariablesRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_UpdateAgentVariablesRequest,
            "UpdateAgentVariablesRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_GetAgentVariablesRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_GetAgentVariablesRequest,
            "GetAgentVariablesRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_GetAgentVariablesResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_GetAgentVariablesResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_GetAgentVariablesResponse,
            "GetAgentVariablesResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_GetAgentVariablesRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_PrefillAgentKvCacheRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_PrefillAgentKvCacheRequest,
            "PrefillAgentKvCacheRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_PrefillAgentKvCacheResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_PrefillAgentKvCacheResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_PrefillAgentKvCacheResponse,
            "PrefillAgentKvCacheResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_PrefillAgentKvCacheRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_EvictAgentKvCacheRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_EvictAgentKvCacheRequest,
            "EvictAgentKvCacheRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_EvictAgentKvCacheResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_EvictAgentKvCacheResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_EvictAgentKvCacheResponse,
            "EvictAgentKvCacheResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_EvictAgentKvCacheRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_GetAgentKvCacheStatusRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_GetAgentKvCacheStatusRequest,
            "GetAgentKvCacheStatusRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_GetAgentKvCacheStatusResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_GetAgentKvCacheStatusResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_GetAgentKvCacheStatusResponse,
            "GetAgentKvCacheStatusResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_GetAgentKvCacheStatusRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_AppendInteractionsRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_AppendInteractionsRequest,
            "AppendInteractionsRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_AppendInteractionsResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_AppendInteractionsResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_AppendInteractionsResponse,
            "AppendInteractionsResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_AppendInteractionsRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_RemoveInteractionsFromEndRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_RemoveInteractionsFromEndRequest,
            "RemoveInteractionsFromEndRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_RemoveInteractionsFromEndResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_RemoveInteractionsFromEndResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_RemoveInteractionsFromEndResponse,
            "RemoveInteractionsFromEndResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_RemoveInteractionsFromEndRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_SetInferenceThrottle:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_SetInferenceThrottle,
            "SetInferenceThrottle",
            true,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_SetSessionLocaleRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_SetSessionLocaleRequest,
            "SetSessionLocaleRequest",
            true,
            "",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ExportDialogRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ExportDialogRequest,
            "ExportDialogRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_ExportDialogResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ExportDialogResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ExportDialogResponse,
            "ExportDialogResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_ExportDialogRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ImportDialogRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ImportDialogRequest,
            "ImportDialogRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_ImportDialogResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ImportDialogResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ImportDialogResponse,
            "ImportDialogResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_ImportDialogRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_GetMemoryConsumptionRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_GetMemoryConsumptionRequest,
            "GetMemoryConsumptionRequest",
            true,
            "",
            ::Tryll::MessageBody_GetMemoryConsumptionResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_GetMemoryConsumptionResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_GetMemoryConsumptionResponse,
            "GetMemoryConsumptionResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_GetMemoryConsumptionRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_EnsureMutableKbRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_EnsureMutableKbRequest,
            "EnsureMutableKbRequest",
            true,
            "",
            ::Tryll::MessageBody_EnsureMutableKbResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_EnsureMutableKbResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_EnsureMutableKbResponse,
            "EnsureMutableKbResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_EnsureMutableKbRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_EnumerateMutableKbsRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_EnumerateMutableKbsRequest,
            "EnumerateMutableKbsRequest",
            true,
            "",
            ::Tryll::MessageBody_EnumerateMutableKbsResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_EnumerateMutableKbsResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_EnumerateMutableKbsResponse,
            "EnumerateMutableKbsResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_EnumerateMutableKbsRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbWriteRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbWriteRequest,
            "MutableKbWriteRequest",
            true,
            "",
            ::Tryll::MessageBody_MutableKbWriteResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbWriteResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbWriteResponse,
            "MutableKbWriteResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_MutableKbWriteRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbDeleteRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbDeleteRequest,
            "MutableKbDeleteRequest",
            true,
            "",
            ::Tryll::MessageBody_MutableKbDeleteResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbDeleteResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbDeleteResponse,
            "MutableKbDeleteResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_MutableKbDeleteRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbClearRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbClearRequest,
            "MutableKbClearRequest",
            true,
            "",
            ::Tryll::MessageBody_MutableKbClearResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbClearResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbClearResponse,
            "MutableKbClearResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_MutableKbClearRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbQueryRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbQueryRequest,
            "MutableKbQueryRequest",
            true,
            "",
            ::Tryll::MessageBody_MutableKbQueryResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbQueryResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbQueryResponse,
            "MutableKbQueryResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_MutableKbQueryRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbGetRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbGetRequest,
            "MutableKbGetRequest",
            true,
            "",
            ::Tryll::MessageBody_MutableKbGetResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbGetResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbGetResponse,
            "MutableKbGetResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_MutableKbGetRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbListRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbListRequest,
            "MutableKbListRequest",
            true,
            "",
            ::Tryll::MessageBody_MutableKbListResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbListResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbListResponse,
            "MutableKbListResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_MutableKbListRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbStatsRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbStatsRequest,
            "MutableKbStatsRequest",
            true,
            "",
            ::Tryll::MessageBody_MutableKbStatsResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_MutableKbStatsResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_MutableKbStatsResponse,
            "MutableKbStatsResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_MutableKbStatsRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ExportMutableKbRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ExportMutableKbRequest,
            "ExportMutableKbRequest",
            true,
            "",
            ::Tryll::MessageBody_ExportMutableKbResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ExportMutableKbResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ExportMutableKbResponse,
            "ExportMutableKbResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_ExportMutableKbRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ImportMutableKbRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ImportMutableKbRequest,
            "ImportMutableKbRequest",
            true,
            "",
            ::Tryll::MessageBody_ImportMutableKbResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_ImportMutableKbResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_ImportMutableKbResponse,
            "ImportMutableKbResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_ImportMutableKbRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_SaveGameMemoryRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_SaveGameMemoryRequest,
            "SaveGameMemoryRequest",
            true,
            "",
            ::Tryll::MessageBody_SaveGameMemoryResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_SaveGameMemoryResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_SaveGameMemoryResponse,
            "SaveGameMemoryResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_SaveGameMemoryRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_LoadGameMemoryRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_LoadGameMemoryRequest,
            "LoadGameMemoryRequest",
            true,
            "",
            ::Tryll::MessageBody_LoadGameMemoryResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_LoadGameMemoryResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_LoadGameMemoryResponse,
            "LoadGameMemoryResponse",
            false,
            "",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_LoadGameMemoryRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_SaveDialogRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_SaveDialogRequest,
            "SaveDialogRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_SaveDialogResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_SaveDialogResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_SaveDialogResponse,
            "SaveDialogResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_SaveDialogRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_LoadDialogRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_LoadDialogRequest,
            "LoadDialogRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_LoadDialogResponse,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_LoadDialogResponse:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_LoadDialogResponse,
            "LoadDialogResponse",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_LoadDialogRequest,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_TtsVisemeFrame:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_TtsVisemeFrame,
            "TtsVisemeFrame",
            false,
            "agent_id",
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "sum",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_SetAgentVoiceRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_SetAgentVoiceRequest,
            "SetAgentVoiceRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    case ::Tryll::MessageBody_SetAgentDeliveryModeRequest:
    {
        static const FTryllMessageDescriptor kRow = {
            ::Tryll::MessageBody_SetAgentDeliveryModeRequest,
            "SetAgentDeliveryModeRequest",
            true,
            "agent_id",
            ::Tryll::MessageBody_Ack,
            ::Tryll::MessageBody_NONE,
            ::Tryll::MessageBody_NONE,
            "",
            "",
        };
        return &kRow;
    }
    default:
        return nullptr;
    }
}

/// A unary request whose fields name an agent or a voice input.
/// Unary means it has a terminal and no progress frame, so SendMessage is not
/// included. CreateAgent is not included either: it has no scope key.
/// Reply rows for these join the outbound request instead of standing alone.
[[nodiscard]] inline bool IsScopedAck(::Tryll::MessageBody body) noexcept
{
    const FTryllMessageDescriptor* row = MessageDescriptor(body);
    return row
        && row->Terminal != ::Tryll::MessageBody_NONE
        && row->Progress == ::Tryll::MessageBody_NONE
        && row->ScopeKey != nullptr
        && row->ScopeKey[0] != '\0';
}

/// An agent-scoped request marked (turn: "standalone"). It opens its own
/// Agent Log turn instead of joining an in-flight send.
[[nodiscard]] inline bool IsStandaloneTurnRequest(::Tryll::MessageBody body) noexcept
{
    switch (body)
    {
    case ::Tryll::MessageBody_ChangeAgentParamRequest:
    case ::Tryll::MessageBody_PrefillAgentKvCacheRequest:
    case ::Tryll::MessageBody_EvictAgentKvCacheRequest:
    case ::Tryll::MessageBody_GetAgentKvCacheStatusRequest:
    case ::Tryll::MessageBody_AppendInteractionsRequest:
    case ::Tryll::MessageBody_RemoveInteractionsFromEndRequest:
    case ::Tryll::MessageBody_ExportDialogRequest:
    case ::Tryll::MessageBody_ImportDialogRequest:
    case ::Tryll::MessageBody_SaveDialogRequest:
    case ::Tryll::MessageBody_LoadDialogRequest:
    case ::Tryll::MessageBody_SetAgentVoiceRequest:
        return true;
    default:
        return false;
    }
}

} // namespace Tryll
