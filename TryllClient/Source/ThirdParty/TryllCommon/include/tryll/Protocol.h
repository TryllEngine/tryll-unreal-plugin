// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Tryll {

/// Current wire protocol version, sent in ConnectionReady.
///
/// Version history:
///   v1: initial protocol (GraphDescription carried a separate
///       `routes: [ExitRoute]` table; node wiring was external to typed params).
///   v2: workflow wiring moved onto typed node params via `(exit)`-attributed
///       string fields (e.g. GenerateParams.default_exit).  The ExitRoute table
///       and GraphDescription.routes were removed; clients must populate the
///       per-node exit fields instead.
///   v3: CreateStringStorageRequest/DestroyStringStorageRequest re-interpreted
///       as Pin/Unpin: inline blobs register under a virtual name (pinned until
///       Destroy); file_path is now relative to storage_root and is stored under
///       the canonical absolute path (idempotent re-pin).  File paths in
///       CreateStringStorageRequest are relative to storage_root (not absolute).
///       CreateEmbeddedStringStorageRequest likewise uses RegisterVirtual.
///   v4: Connection/Session split.  SessionReady (which carried session_id at
///       connect) is renamed ConnectionReady and no longer carries a session_id —
///       it is a connection-level hello (protocol_version + codegen_fingerprint).
///       ConfigureSessionRequest/Response are renamed CreateSessionRequest/
///       CreateSessionResponse; CreateSession is now mandatory and one-shot, and
///       CreateSessionResponse carries the server-allocated session_id.  Every
///       other request before CreateSession is rejected with SessionNotReady; a
///       second CreateSession with SessionAlreadyExists.
///   v5: Per-agent Variables.  Three MessageBody union members were appended
///       (UpdateAgentVariablesRequest = 43, GetAgentVariablesRequest = 44,
///       GetAgentVariablesResponse = 45) and CreateAgentRequest gained a trailing
///       `variables: [AgentVariable]` field.  A v4 server cannot decode the new
///       requests, so the version is bumped to fail incompatible peers at the
///       ConnectionReady handshake rather than on the first variable request.
///   v6: Agent KV-cache lifecycle.  Six MessageBody union members were appended
///       (Prefill/Evict/GetAgentKvCacheStatus request/response pairs = 46–51)
///       and CreateAgentRequest gained `kv_cache_initialization`.  A v5 server
///       cannot decode the new requests, so the version is bumped to fail
///       incompatible peers at ConnectionReady.
///   v7: Dialog mutation.  Four MessageBody union members were appended
///       (AppendInteractions / RemoveInteractionsFromEnd request/response
///       pairs = 52–55).  A v6 server cannot decode the new requests, so the
///       version is bumped to fail incompatible peers at ConnectionReady.
///   v8: ResumeAgentRequest gained an optional `tool_results: [ToolResult]`
///       vector so a paused tool-call turn can receive model-visible results
///       atomically before graph execution continues.
///   v9: ToolCallParams replaced notify_client / pause_after_tool_call /
///       history_policy with a single ToolCallDisposition enum (wire layout
///       change; no peer compatibility).
///   v10: RetrieveParams gained retrieval_mode and rrf_k inserted before
///       source/filter/found_exit/not_found_exit (field-layout break). A v9
///       peer cannot safely decode RetrieveParams; reject at ConnectionReady.
///   v11: Manual inference throttle. One MessageBody union member was appended
///       (SetInferenceThrottle = 56, fire-and-forget) and CreateAgentRequest
///       gained a trailing `workload: AgentWorkload` field. A v10 server
///       cannot decode the new message, so the version is bumped to fail
///       incompatible peers at ConnectionReady.
///   v12: Session locale + pause deadlines.  Two unrelated changes carrying one
///       version number: both landed after v11 and neither reached a release on
///       its own, so the v12 contract is the union of the two.  Either half
///       alone would have forced the bump; incompatible peers are failed at
///       ConnectionReady.
///       Session locale: CreateSessionRequest gained a trailing `locale`
///       (BCP-47) field, ModelInfo a trailing `languages` vector, and one
///       MessageBody union member was appended (SetSessionLocaleRequest = 57)
///       for changing the language mid-session. The union member alone forces
///       the bump — a v11 server cannot decode it — and the two additive
///       fields would have warranted one anyway: a v11 server *silently
///       ignores* the locale, so the game would get English speech and
///       English-resolved storage paths with no error to explain why.
///       Pause deadlines: TurnStatus gained PauseTimedOut = 3; PauseParams
///       gained pause_timeout_ms + timeout_exit and ToolCallParams gained
///       pause_timeout_ms (all structural).  A v11 client cannot interpret the
///       new terminal status and would read an abandoned turn as an unknown
///       outcome.  Behaviour is unchanged until a deadline is set: every new
///       field defaults to 0 = infinite.
///   v13: Engine selection and model placement.  Two changes under one version,
///       following the v12 precedent: each would force the bump alone, and
///       neither reaches a release on its own.
///       Engine set: CreateSessionRequest lost inference_engine / stt_engine /
///       tts_engine / embedding_engine and gained `model_config`
///       (SessionModelConfig: role-qualified engines plus sparse per-model
///       placement hints).  The catalog already states each model's engine, so a
///       client declares which engines it needs rather than choosing an
///       implementation per model type, and `Mock` leaves the wire entirely.
///       This is a field-layout break; a v12 peer cannot decode it.
///       Placement: LoadModelRequest gained `placement`, LoadModelResponse
///       gained `placement_resolved` + `backend`.  The additive half would
///       warrant a bump on its own: a v12 server *silently ignores* a placement
///       hint, so a game asking for system RAM gets a model in VRAM (or the
///       reverse) with no error to explain the memory blowup or the
///       order-of-magnitude slowdown — the same reasoning already written into
///       the v12 locale entry.
///       The vectors live inside SessionModelConfig rather than on
///       CreateSessionRequest itself for a codegen reason: the codec templates
///       handle a single sub-table but not a vector of tables, for which they
///       emit uncompilable code.  See the schema comment on SessionModelConfig.
///   v14: Dialog snapshot export/import.  Four MessageBody union members were
///       appended (ExportDialogRequest/Response = 58/59, ImportDialogRequest/
///       Response = 60/61).  The nested PersistedDialogSnapshot is a separate
///       FlatBuffers root (file identifier "TDLG"); its format_version is
///       independent of this protocol number.  A v13 server cannot decode the
///       new requests, so the version is bumped to fail incompatible peers at
///       ConnectionReady.
///   v15: TTS voice controls and shared-asset model metadata.  Three additive
///       changes under one version: GenerateAndSpeakParams and SpeakParams gain
///       tts_voice_description + tts_delivery_instruction; ModelStatus gains
///       Incomplete = 5; ModelInfo gains dependencies and tts_capabilities.
///       A v14 peer cannot safely interpret Incomplete or the new ModelInfo
///       tables, and a v14 server silently ignores the new node fields, so
///       incompatible peers are failed at ConnectionReady.
///   v16: ClassifyIntent / ClassifyIntentLLM `notify_client` changed from bool
///       to NotifyClient { Disabled, OnFound, Always }. ClassifyIntentLLM now
///       emits event_type "intent_llm_classified" (embedding keeps
///       "intent_classified") with a full <label>.prob vector. A v15 peer
///       cannot decode the enum field or the new event_type, so incompatible
///       peers are failed at ConnectionReady.
///   v17: GetMemoryConsumptionRequest/Response (union 62/63). A v16 server
///       cannot decode the new request, so incompatible peers are failed at
///       ConnectionReady.
///   v18: ModelPlacementHint gains `hotwords` (HotwordBias), and
///       CreateVoiceInputRequest's `vad_min_silence_ms` default changes from
///       500 to 1000.
///
///       Both halves are silent across a version gap, which is why this is a
///       bump and not a compatible extension. FlatBuffers omits a field left at
///       its default, so the *reader's* schema supplies the value: a v17 client
///       that omits vad_min_silence_ms means 500 and a v18 server reads 1000,
///       and the reverse for a v18 client against a v17 server. Nothing errors —
///       the VAD simply segments differently than the caller asked, which shows
///       up as changed transcripts rather than as a failure. A v17 server also
///       ignores HotwordBias entirely, so a project that set Off would get
///       biasing anyway.
///   v19: ClassifyIntentLLMParams field-layout break. `system_prompt` moved
///       ahead of the deprecated CSV pair, and `intents` moved from the end
///       of the table to sit after `intents_prompt`. Vtable slots are
///       positional, so a v18 peer would decode subsequent fields into the
///       wrong slots. Reject at ConnectionReady.
///   v20: Mutable knowledge bases. MessageBody gains ensure/enumerate/write/
///       query/export/import/save/load pairs; RetrieveParams gains
///       KnowledgeSourceKind + named-store fields; WriteToMemoryParams is
///       appended to NodeParams; dialog snapshots gain save_id / saved_at_ms.
///       A v19 server cannot decode the new requests or node params.
///   v21: Viseme lip-sync track. `GenerateAndSpeakParams.emit_visemes` and
///       `TtsVisemeFrame` on the wire. A v20 peer cannot decode the new frames.
///   v22: Rolling conversation summarization. `SummarizeParams` was appended
///       to the NodeParams union (after WriteToMemoryParams) and
///       `PersistedSummary` to the standalone dialog-snapshot component union.
///       A v21 server cannot construct the new node, and a v21 client cannot
///       decode a folded snapshot, so reject incompatible peers at ConnectionReady.
///   v23: BeginUtteranceRequest drops agent_id. The server no longer injects
///       a turn from a voice utterance — the client sends the transcript
///       itself. Vtable slots after the removed field shift, so a v22 peer
///       would misread auto_finish_on_silence. Reject at ConnectionReady.
///   v24: The storage root becomes process-wide. CreateSessionRequest loses
///       storage_data_folder, so every later field shifts, and ConnectionReady
///       gains a trailing `mode` string. A v23 peer would read the session
///       fields into the wrong slots. Reject at ConnectionReady.
///   v25: GenerateAndSpeakParams and SpeakParams gain `first_chunk_max_chars`
///       (appended; default 0 = off). The field is additive, but a v24 server
///       silently ignores it, so a client that set it would get whole-sentence
///       first chunks with no error — the v15 reasoning. Reject at ConnectionReady.
///   v26: ClassifyAttributesLLMParams is appended to the NodeParams union
///       after SummarizeParams. A v25 server cannot construct the new node,
///       so reject incompatible peers at ConnectionReady. Same version:
///       ClassifyIntentLLMParams drops the deprecated intents_ids /
///       intents_prompt pair, gains output_name after input, and moves
///       temperature from the end of the table to after margin, so every
///       later field shifts vtable slots.
///   v27: The voice moves from the TTS nodes to the agent. SpeakParams and
///       GenerateAndSpeakParams lose every TTS field (model, speaker, speed,
///       lang, reference voice, description, delivery, emit_visemes), so later
///       fields shift vtable slots. CreateAgentRequest gains `voice`
///       (VoiceProfile, voice.fbs); SetAgentVoiceRequest and
///       SetAgentDeliveryModeRequest are appended to MessageBody (95, 96);
///       TtsCapabilitiesInfo gains expressiveness and num_speakers. The audio
///       format is announced once per agent turn. Reject at ConnectionReady.
///   v28: GenerateParams, ToolCallParams, ClassifyIntentLLMParams,
///       ClassifyAttributesLLMParams, GenerateAndSpeakParams and SummarizeParams
///       gain a trailing `adaptation` table (ModelAdaptation: `loras`, a list
///       of LoraAdapter = storage-relative path + scale; control vectors will
///       join it). Additive, but a v27 server would silently run the base
///       model — the v15 reasoning. Reject at ConnectionReady.
///   v29: CannedResponseParams gains a trailing `lines` list and
///       IntentToInstructionParams gains a trailing `instructions` list
///       (LocalizableText). The old `inline_strings` slot stays in the schema
///       but new peers do not write it. A v28 server would ignore `lines` /
///       `instructions` and see an empty inline list — the v15 reasoning.
///       Reject at ConnectionReady.
inline constexpr std::uint32_t ProtocolVersion = 29;

/// Max FlatBuffers payload bytes (excluding the 4-byte length prefix).
inline constexpr std::size_t MaxFramePayloadBytes = 1048576; // 1 MB

/// Conservative cap on a standalone dialog-snapshot buffer so wrapping it in
/// ExportDialogResponse / ImportDialogRequest still fits MaxFramePayloadBytes.
/// Never truncate — oversized snapshots fail with DialogSnapshotTooLarge.
inline constexpr std::size_t MaxDialogSnapshotBytes = MaxFramePayloadBytes - 2048;

/// Conservative cap on a standalone mutable-KB snapshot so wrapping it in
/// ExportMutableKbResponse / ImportMutableKbRequest still fits the frame.
inline constexpr std::size_t MaxMutableKbSnapshotBytes = MaxFramePayloadBytes - 2048;

/// Defensive cap on a whole-session GameMemory file. This is not a wire frame;
/// several stores can exceed MaxMutableKbSnapshotBytes in aggregate.
inline constexpr std::size_t MaxGameMemorySnapshotBytes = 64u * 1024u * 1024u;

/// Max PCM bytes carried by a single TtsAudioFrame.  Leaves a conservative
/// margin under MaxFramePayloadBytes for FlatBuffers table/envelope overhead
/// (~a few hundred bytes) so a maximally-sized PCM slice still passes
/// IsValidPayloadLength.  High-rate TTS models (e.g. Supertonic 3 at 44.1 kHz)
/// can emit sentence-sized chunks well above the frame cap; senders must split
/// PCM into slices of at most this many bytes.
inline constexpr std::size_t MaxTtsPcmBytesPerFrame = MaxFramePayloadBytes - 512;

/// Size of the length prefix in bytes.
inline constexpr std::size_t LengthPrefixBytes = 4;

/// Process exit code the server returns when it cannot bind its listen port
/// because the address is already in use (EADDRINUSE).  Distinct from the
/// generic fatal exit code (1) and the license-gate code (2) so that a client
/// launcher (ManagedServer) can detect a port collision and retry on a fresh
/// free port rather than surfacing an opaque "process exited" error.
inline constexpr int ServerExitPortInUse = 3;

/// Process exit code the server returns when a command-line argument is
/// malformed (e.g. a non-numeric or negative --idle-shutdown-timeout).
/// Distinct from the generic fatal (1), license-gate (2), and port-in-use (3)
/// codes so a launcher can tell a usage error apart from a runtime failure
/// instead of seeing an opaque crash.
inline constexpr int ServerExitInvalidArgs = 4;

/// Decode a 4-byte little-endian length prefix.
[[nodiscard]] inline std::uint32_t ReadLengthPrefixLE(
	const std::array<std::uint8_t, LengthPrefixBytes>& bytes) noexcept
{
	return static_cast<std::uint32_t>(bytes[0])
		| (static_cast<std::uint32_t>(bytes[1]) << 8)
		| (static_cast<std::uint32_t>(bytes[2]) << 16)
		| (static_cast<std::uint32_t>(bytes[3]) << 24);
}

/// Encode a uint32 as a 4-byte little-endian length prefix.
inline void WriteLengthPrefixLE(
	std::uint32_t length,
	std::array<std::uint8_t, LengthPrefixBytes>& out) noexcept
{
	out[0] = static_cast<std::uint8_t>(length & 0xFF);
	out[1] = static_cast<std::uint8_t>((length >> 8) & 0xFF);
	out[2] = static_cast<std::uint8_t>((length >> 16) & 0xFF);
	out[3] = static_cast<std::uint8_t>((length >> 24) & 0xFF);
}

/// Returns true if length is within the allowed payload range.
[[nodiscard]] inline bool IsValidPayloadLength(std::uint32_t length) noexcept
{
	return length > 0 && length <= static_cast<std::uint32_t>(MaxFramePayloadBytes);
}

} // namespace Tryll
