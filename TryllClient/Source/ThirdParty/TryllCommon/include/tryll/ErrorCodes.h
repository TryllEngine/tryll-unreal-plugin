// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include <cstdint>

namespace Tryll {

/// Numeric error codes sent in ErrorResponse.message body.
///
/// Ranges:
///   1xxx  connection-level
///   2xxx  session-level
///   3xxx  agent-level
///   4xxx  inference / workflow
///   5xxx  protocol / framing
///   6xxx  model download / management
///   7xxx  string storage
///   8xxx  embedded storage and mutable knowledge bases
enum class ErrorCode : std::uint32_t
{
	// Connection
	/// TCP connect failed outright.
	ConnectionFailed  = 1001,
	/// Established connection was dropped mid-session (peer close or socket error).
	ConnectionDropped = 1002,
	/// Client-side timeout waiting for a response frame; the socket itself may still be healthy.
	Timeout           = 1003,

	// Session
	/// Server is shutting down and no longer accepts requests on this session.
	ServerShuttingDown   = 2001,
	/// A non-CreateSession request arrived before CreateSession created the session.
	SessionNotReady      = 2002,
	/// A second CreateSessionRequest arrived on a connection that already has a session.
	SessionAlreadyExists = 2003,
	/// CreateSessionRequest.locale is not a well-formed BCP-47 tag.
	InvalidLocale        = 2004,
	/// An entry in CreateSessionRequest.model_config carries an ordinal that is not
	/// a valid ModelRole, InferenceEngine or MemoryPlacement value.
	///
	/// This is the *only* remaining cause. The engine set is advisory (spec §7.1):
	/// it may be empty, incomplete, or name an engine the session never uses, and
	/// none of that is an error. What cannot be tolerated is an out-of-range
	/// ordinal, because the wire value is cast to an enum and a bad cast is
	/// undefined behaviour rather than a wrong answer.
	InvalidEngineSet     = 2005,

	// Agent
	/// Request referenced an agent_id the server does not know (wrong id or already destroyed).
	InvalidAgentId         = 3001,
	/// Agent was previously destroyed; the id is no longer valid.
	AgentAlreadyDestroyed  = 3002,
	/// Server failed to compile the workflow graph into an executable plan.
	GraphCompilationFailed = 3003,
	/// Agent is still processing the previous turn and cannot accept a new message yet.
	/// This is the re-entrancy rejection.
	AgentBusy              = 3004,
	/// node_name not found in the agent's graph.
	UnknownNode            = 3005,
	/// param_name is not settable on that node type.
	ParamNotMutable        = 3006,
	/// param_name is settable but value failed validation.
	InvalidParamValue      = 3007,
	/// (exit) field on a node references a target node that doesn't exist.
	InvalidExitTarget      = 3008,
	/// node storage path escapes storage_root (absolute or `..`).
	StorageOutsideRoot     = 3009,
	/// node storage filename is neither a containable file nor a registered virtual name.
	StorageFileNotFound    = 3010,
	/// resolved storage kind does not match the consuming node.
	StorageKindMismatch    = 3011,
	/// Resume requested but the agent is not paused.
	AgentNotPaused         = 3012,
	/// UpdateAgentVariables targets a name not declared at CreateAgent.
	UnknownVariable        = 3013,
	/// Assign/AddToSet/RemoveFromSet value type does not match the declared variable type.
	VariableTypeMismatch   = 3014,
	/// Declaration name/limit violation (CreateAgent.variables).
	InvalidVariableValue   = 3015,
	/// Resume tool_results are invalid for the current paused tool-call batch.
	InvalidToolResults     = 3016,
	/// A node relies on the session locale but its model does not declare that language.
	ModelLanguageUnsupported = 3017,
	/// Export/import snapshot is malformed, missing required fields, has an
	/// invalid enum, or has inconsistent tool call/result pairing.
	DialogSnapshotInvalid    = 3018,
	/// PersistedDialogSnapshot.format_version is newer than this server, or
	/// the standalone buffer's file identifier is not "TDLG".
	DialogFormatUnsupported  = 3019,
	/// Import Strict: snapshot.graph_signature does not match the agent's
	/// create-time graph hash.
	DialogGraphMismatch      = 3020,
	/// Encoded snapshot would overflow MaxDialogSnapshotBytes / the 1 MiB frame.
	DialogSnapshotTooLarge   = 3021,
	/// Export hit an IComponent type this snapshot format does not know, or
	/// import saw an unknown PersistedComponent union arm.
	DialogUnsupportedComponent = 3022,
	/// A VoiceProfile field fails validation or the loaded TTS model cannot
	/// use it (CreateAgent, SetAgentVoice). The message names the field and,
	/// for delivery modes, the mode.
	InvalidVoiceProfile      = 3023,
	/// SetAgentDeliveryMode named a mode the agent's voice does not define.
	UnknownDeliveryMode      = 3024,
	/// A graph with a speaking node (SpeakNode, GenerateAndSpeakNode) was
	/// created without a voice, or SetAgentDeliveryMode targeted an agent
	/// that has no voice.
	AgentVoiceRequired       = 3025,

	// Inference / workflow
	/// A generation or workflow node raised a non-recoverable inference error.
	InferenceFailed  = 4001,
	/// A generation or workflow node exceeded its per-node time budget.
	InferenceTimeout = 4002,
	/// Workflow exceeded the maximum number of node transitions per turn.
	MaxStepsExceeded = 4003,
	/// Explicit PrefillKvCache / create-with-Prefill failed (allocation, format, or decode).
	KvCacheOperationFailed = 4004,
	/// Selected inference backend cannot provide the requested KV-cache lifecycle op.
	KvCacheManagementUnsupported = 4005,

	// Protocol / framing
	/// Received frame could not be decoded against the FlatBuffers schema.
	MalformedMessage        = 5001,
	/// Received frame carries a message type the server does not recognize.
	UnknownMessageType      = 5002,
	/// Outgoing or incoming frame exceeds the negotiated maximum size.
	FrameTooLarge           = 5003,
	/// Client and server speak incompatible versions of the wire protocol.
	ProtocolVersionMismatch = 5004,
	/// Debug-command channel not in this build (Production) or disabled in config.
	DebugCommandsUnavailable = 5005,
	/// DebugCommandRequest.command is not a recognised verb.
	UnknownDebugCommand      = 5006,

	// Download / model management
	/// Model download transport failed (HTTP error, interrupted transfer, checksum mismatch).
	DownloadFailed             = 6001,
	/// Model is not registered in the catalog, or has no HuggingFace repo/files configured.
	DownloadNotAvailable       = 6002,
	/// Target disk does not have enough free space for the model.
	InsufficientDiskSpace      = 6003,
	/// A download for this model is already in progress on the server.
	DownloadAlreadyActive      = 6004,
	/// Model not in catalog or engine not registered.
	ModelResolutionFailed      = 6005,
	// 6006 retired (ModelAutoDownloadFailed) — auto model downloading was removed;
	// do not reuse this number, see feat-010-remove-auto-model-downloading.
	/// RESERVED AND CURRENTLY UNUSED. Would report a model whose variant declares
	/// a hard device requirement the host cannot meet. No catalog model declares
	/// one, so the check is deliberately not built — the number is reserved so the
	/// taxonomy has a documented home for it, and so nobody assigns 6007 to
	/// something else. Client placement requests are *hints* and cannot error:
	/// they are downgraded and the resolved value is reported instead.
	/// See engine-selection-and-model-placement-spec.md section 7.4.
	ModelPlacementUnavailable  = 6007,
	/// A referenced model needs an engine that is not registered in this server
	/// build. The message names the model, its role, and the engine it needs.
	///
	/// This is a statement about the *server binary*, not about client bookkeeping.
	/// Through the first cut of protocol v13 the same number meant "your session
	/// did not declare this engine" (`EngineNotDeclared`), which put the blame on a
	/// client field that is now advisory — and named a cause the reader could not
	/// act on. The guard that belongs earlier is the build-time one (spec §7.10).
	EngineNotAvailable         = 6008,
	/// This deployment mode does not download or delete models. Client Prod
	/// ships every model with the game and rejects both requests.
	ModelManagementDisabled    = 6009,

	// StringStorage
	/// Requested StringStorage name is empty, malformed, or not found on destroy.
	InvalidStringStorageName   = 7001,
	/// StringStorage with this name is already registered for the session.
	StringStorageAlreadyExists = 7002,
	/// StringStorage content was rejected (unreadable file, bad format, empty array, etc.).
	InvalidStringStorageData   = 7003,
	/// Virtual (inline) name collides with a real path or an existing entry.
	StorageNameInUse           = 7004,

	// EmbeddedStringStorage
	InvalidEmbeddedStorageName   = 8001,
	EmbeddedStorageAlreadyExists = 8002,
	EmbeddedStorageBuildFailed   = 8003,

	// Mutable knowledge bases
	/// Export/import/save/load refused because an agent is not strictly idle.
	StorageBusy              = 8101,
	/// Named store exists under a different schema than the caller asked for.
	SchemaMismatch           = 8102,
	/// Live-record cap reached; the overflowing insert was rejected.
	MaxRecordsExceeded       = 8103,
	/// Unknown catalog schema name, or schema path failed to load.
	UnknownSchema            = 8104,
	/// Snapshot format_version is newer, identifier is not TMKB, or the
	/// buffer failed verification.
	MutableKbSnapshotInvalid = 8105,
	/// Encoded snapshot would overflow MaxMutableKbSnapshotBytes (wire) or
	/// MaxGameMemorySnapshotBytes (file).
	StorageSnapshotTooLarge  = 8106,
	/// LoadGameMemory embed_timing was Unspecified (omission is an error).
	InvalidEmbedTiming       = 8107,
	/// Named store does not exist (get/query/export without ensure).
	UnknownMutableKb         = 8108,
	/// SaveGameMemory / SaveDialog refused because server `saves.enabled` is false.
	SavesDisabled            = 8109,

	// VoiceInput / STT
	/// Server failed to load the STT model requested by CreateVoiceInput.
	SttModelLoadFailed     = 4100,
	/// Declared audio format is not supported (only 16-bit mono PCM is accepted).
	AudioFormatUnsupported = 4101,
	/// Request referenced a voice_input_id the server does not know.
	VoiceInputNotFound     = 4102,
	/// BeginUtterance issued while an utterance is already active on this handle.
	UtteranceInProgress    = 4103,
	/// End/Cancel issued with no utterance open.
	NoActiveUtterance      = 4104,
	/// Utterance exceeded max_utterance_ms before being committed.
	UtteranceTimeout       = 4105,
	/// A voice input supplied a hotwords lexicon for a model this session set
	/// HotwordBias.Off on. Refused rather than accepted-and-ignored: a lexicon
	/// that silently does nothing is the failure mode this whole feature spent
	/// months in.
	HotwordBiasDisabled    = 4106,
	/// CreateVoiceInputRequest.hotwords_score is not a finite, non-negative
	/// number. It reaches the recognizer's beam search as a per-token bonus, so
	/// a NaN poisons every hypothesis comparison and an infinity makes the biased
	/// path win unconditionally — both surface as garbled transcripts with
	/// nothing in the log, which is why this is refused rather than clamped.
	InvalidHotwordsScore   = 4107,
	/// Client-side only; the server never sends it. The voice input component found
	/// no usable capture device: none present, the configured device is not
	/// connected, or the capture stream failed to open. The message names the cause;
	/// the component's microphone status says the same.
	MicrophoneUnavailable  = 4108,
	/// Client-side only; the server never sends it. The voice input component is
	/// (re)creating its voice input, because a microphone appeared or the device's
	/// sample rate changed, so this BeginUtterance was not started. Try again shortly.
	VoiceInputStarting     = 4109,
};

} // namespace Tryll
