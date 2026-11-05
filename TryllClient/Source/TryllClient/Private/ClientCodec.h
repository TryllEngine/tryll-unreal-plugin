// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

// Generated codec declarations for non-manual client→server messages (Unreal flavor).
// Manual carve-outs and convenience wrappers are declared below.
#include "Generated/ClientCodec.gen.h"

#include "CoreMinimal.h"
#include "TryllGraphDescription.h"
#include "TryllVariableDecl.h"
#include "TryllDialogInteraction.h"
#include "TryllDialogSnapshot.h"
#include "TryllMkbSnapshot.h"
#include "TryllError.h"

namespace Tryll::Client {

// ── Manual carve-outs ─────────────────────────────────────────────────────────

/**
 * Build a CreateAgentRequest FlatBuffers payload with the full graph description.
 * Voice (protocol v27) is the agent's voice; null leaves CreateAgentRequest.voice
 * absent, which the server accepts only for a graph with no speaking node.
 */
[[nodiscard]] TRYLLCLIENT_API TArray<uint8>
BuildCreateAgentRequest(uint64 RequestId, const FTryllGraphDescription& Graph, bool bEnableDiagnostics,
                        bool bMaintainDialogueHistory = true, const TArray<FTryllVariableDecl>& Variables = {},
                        ::Tryll::AgentKvCacheInitialization KvCacheInitialization =
	                        ::Tryll::AgentKvCacheInitialization_AllocateOnly,
                        ::Tryll::AgentWorkload Workload = ::Tryll::AgentWorkload_Interactive,
                        const ::Tryll::VoiceProfileT* Voice = nullptr);

/** Build a batched UpdateAgentVariablesRequest FlatBuffers payload (deferred-flush from FTryllAgentVariables). */
[[nodiscard]] TArray<uint8>
BuildUpdateAgentVariablesRequest(uint64 RequestId, uint64 AgentId, const TArray<FTryllVariableUpdate>& Updates);

/** Build a ResumeAgentRequest payload with an optional nested ToolResult batch. */
[[nodiscard]] TArray<uint8>
BuildResumeAgentRequest(
	uint64 RequestId,
	uint64 AgentId,
	const FString& ResumeNode,
	const TArray<TPair<FString, FString>>& ToolResults = {});

/** Build an AppendInteractionsRequest FlatBuffers payload (manual: nested [DialogInteraction]). */
[[nodiscard]] TArray<uint8>
BuildAppendInteractionsRequest(
	uint64 RequestId,
	uint64 AgentId,
	const TArray<FTryllDialogInteraction>& Interactions);

/** Build an ImportDialogRequest FlatBuffers payload (manual: nested PersistedDialogSnapshot). */
[[nodiscard]] TArray<uint8>
BuildImportDialogRequest(
	uint64 RequestId,
	uint64 AgentId,
	const FTryllDialogSnapshot& Snapshot,
	ETryllDialogCompatibility Compatibility,
	FTryllError& OutError);

[[nodiscard]] bool PackPersistedDialogSnapshot(
	flatbuffers::FlatBufferBuilder& Fbb,
	const FTryllDialogSnapshot& Snapshot,
	flatbuffers::Offset<::Tryll::PersistedDialogSnapshot>& OutOffset,
	FTryllError& OutError);

[[nodiscard]] bool UnpackPersistedDialogSnapshot(
	const ::Tryll::PersistedDialogSnapshot& Table,
	FTryllDialogSnapshot& OutSnapshot,
	FTryllError& OutError);

[[nodiscard]] bool PackPersistedMutableKb(
	flatbuffers::FlatBufferBuilder& Fbb,
	const FTryllMutableKbSnapshot& Snapshot,
	flatbuffers::Offset<::Tryll::PersistedMutableKb>& OutOffset,
	FTryllError& OutError);

[[nodiscard]] bool UnpackPersistedMutableKb(
	const ::Tryll::PersistedMutableKb& Table,
	FTryllMutableKbSnapshot& OutSnapshot,
	FTryllError& OutError);

[[nodiscard]] TArray<uint8>
BuildImportMutableKbRequest(
	uint64 RequestId,
	const FTryllMutableKbSnapshot& Snapshot,
	ETryllMutableKbCompatibility Compatibility,
	FTryllError& OutError);

/**
 * Build a ChangeAgentParamRequest FlatBuffers payload using the typed NodeParams union.
 * Call from the game thread; Params->Pack() is safe here and result is thread-portable bytes.
 */
[[nodiscard]] TArray<uint8>
BuildChangeAgentParamsRequest(
	uint64                RequestId,
	uint64                AgentId,
	const FString&        NodeName,
	UTryllNodeParamsBase* Params);

/** Build an AudioBuffer FlatBuffers payload (fire-and-forget; no request_id). */
[[nodiscard]] TRYLLCLIENT_API TArray<uint8>
BuildAudioBuffer(uint64 VoiceInputId, const TArray<uint8>& Pcm);

// ── Dual-mode StringStorage convenience wrappers ──────────────────────────────

/** Build a CreateStringStorageRequest FlatBuffers payload (inline strings; kind=List). */
[[nodiscard]] TArray<uint8>
BuildCreateStringStorageRequest(uint64 RequestId, const FString& Name, const TArray<FString>& Strings);

/** Build a CreateStringStorageRequest FlatBuffers payload (inline key/value pairs; kind=Map or Multimap). */
[[nodiscard]] TArray<uint8>
BuildCreateKeyedStringStorageRequest(
	uint64                         RequestId,
	const FString&                 Name,
	const TArray<FString>&         Keys,
	const TArray<FString>&         Values,
	::Tryll::StringStorageKind     Kind = ::Tryll::StringStorageKind_Map);

/** Build a CreateStringStorageRequest FlatBuffers payload (server-side file; kind=List). */
[[nodiscard]] TArray<uint8>
BuildCreateStringStorageFromFileRequest(uint64 RequestId, const FString& Name, const FString& FilePath);

/** Build a CreateStringStorageRequest FlatBuffers payload (server-side file; explicit kind). */
[[nodiscard]] TArray<uint8>
BuildCreateStringStorageFromFileRequest(
	uint64                     RequestId,
	const FString&             Name,
	const FString&             FilePath,
	::Tryll::StringStorageKind Kind);

// ── Dual-mode EmbeddedStringStorage convenience wrappers ─────────────────────

/** Build a CreateEmbeddedStringStorageRequest (Path A: config file). */
[[nodiscard]] TArray<uint8>
BuildCreateEmbeddedStringStorageRequest(
	uint64         RequestId,
	const FString& Name,
	const FString& ConfigPath,
	const FString& EmbeddingModel);

/** Build a CreateEmbeddedStringStorageRequest (Path B: inline strings). */
[[nodiscard]] TArray<uint8>
BuildCreateEmbeddedStringStorageFromStringsRequest(
	uint64                RequestId,
	const FString&        Name,
	const TArray<FString>& Strings,
	const FString&        EmbeddingModel);

// ── VoiceInput flat-format convenience wrapper ────────────────────────────────

/** Build a CreateVoiceInputRequest FlatBuffers payload (flat audio format fields). */
[[nodiscard]] TArray<uint8>
BuildCreateVoiceInputRequest(
	uint64         RequestId,
	const FString& ModelName,
	uint32         SampleRate,
	uint16         Channels,
	uint16         BitsPerSample,
	float          VadThreshold,
	uint32         VadMinSilenceMs,
	uint32         VadSpeechPadMs,
	const FString& HotwordsStoragePath = FString{},
	float          HotwordsScore       = 1.5f);

} // namespace Tryll::Client
