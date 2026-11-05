// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "TryllError.h"
#include "TryllGraphDescription.h"
#include "TryllDialogInteraction.h"
#include "TryllDialogSnapshot.h"
#include "TryllKvCache.h"
#include "TryllVariableDecl.h"
#include "TryllAgentVariables.h"
#include "TryllToolCall.h"
#include "TryllNodeEvent.h"
#include "TryllVisemeEvent.h"
#include "TryllVoiceProfile.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"

class FTryllConnection;

/**
 * Plain C++ handle for a server-side agent.
 * Created by UTryllSubsystem after a successful CreateAgentResponse.
 *
 * Ownership: typically held as TSharedPtr<FTryllAgent> by UTryllAgentComponent.
 * Lifetime: deterministic — the owning component releases the shared_ptr; this
 * triggers PushDestroyAgent via UTryllSubsystem::RequestDestroyAgent().
 *
 * All callbacks are invoked on the game thread (from UTryllSubsystem::Tick).
 */
class TRYLLCLIENT_API FTryllAgent final : public TSharedFromThis<FTryllAgent>, public FGCObject
{
public:
	FTryllAgent(TSharedPtr<FTryllConnection> InConnection, std::uint64_t InAgentId,
	            const TArray<FTryllVariableDecl>& InDeclaredVariables = {});
	~FTryllAgent();

	/** Local mirror of this agent's declared per-agent Variables. */
	[[nodiscard]] FTryllAgentVariables& GetVariables() const { return *Variables; }

	// FGCObject: the params baseline holds UObjects in a non-UObject class, which
	// the garbage collector cannot see through TObjectPtr alone. Report them here
	// so the authored/mutated node params survive collection for the agent's life.
	virtual void    AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FTryllAgent"); }

	[[nodiscard]] std::uint64_t GetAgentId()  const noexcept { return AgentId; }
	[[nodiscard]] bool          IsValid()      const noexcept { return Connection.IsValid() && AgentId != 0; }
	[[nodiscard]] bool          WasCreatedWithDiagnostics() const noexcept { return bCreatedWithDiagnostics; }
	[[nodiscard]] const FString& GetDisplayName() const { return DisplayName; }

	void SetCreatedWithDiagnostics(bool bValue) { bCreatedWithDiagnostics = bValue; }

	/**
	 * Workflow text table and asset name used to resolve ticked Text IDs in
	 * ChangeParams. Set by the subsystem when the agent is created from a graph.
	 */
	void SetLocalizedTextContext(FName InTextTable, const FString& InAssetName)
	{
		TextTable = InTextTable;
		LocalizedAssetName = InAssetName;
	}
	[[nodiscard]] FName GetTextTable() const { return TextTable; }
	[[nodiscard]] const FString& GetLocalizedAssetName() const { return LocalizedAssetName; }
	/** Optional editor/display label (e.g. owning actor name). Not on the wire. */
	void SetDisplayName(FString InName) { DisplayName = MoveTemp(InName); }

	/**
	 * Send a text message to the agent.
	 * Response arrives via the OnAnswerText / OnTurnComplete callbacks.
	 */
	void SendMessage(const FString& Text);

	/**
	 * Cooperatively cancel the agent's in-flight turn. The turn ends with
	 * ETryllTurnStatus::Cancelled via the OnTurnComplete callback. No-op if idle.
	 * Mode selects whether the partial interaction is kept (default) or discarded.
	 */
	void Cancel(ETryllCancelMode Mode = ETryllCancelMode::StopAndKeep);

	/**
	 * Resume a turn that is currently paused (see the Pause node / typed
	 * OnNodeEvent event_type="paused" wiring on UTryllSubsystem).
	 * ResumeNode empty continues via the paused node's pending exit route;
	 * non-empty jumps to that node by name. Completion is invoked on the
	 * game thread: nullopt-equivalent (Error.IsOk()) on success,
	 * AgentNotPaused (3012) if the agent is not currently paused, or
	 * UnknownNode (3005) if ResumeNode does not name a node in the graph.
	 */
	void Resume(
		const FString&                       ResumeNode = FString{},
		TFunction<void(const FTryllError&)>  OnComplete = nullptr);

	/** Resume with one result correlated to a tool-call ID. */
	void ResumeWithToolResult(
		const FTryllToolResult& ToolResult,
		const FString& ResumeNode = FString{},
		TFunction<void(const FTryllError&)> OnComplete = nullptr);

	/** Resume with the complete result batch for one paused tool-call pass. */
	void ResumeWithToolResults(
		const TArray<FTryllToolResult>& ToolResults,
		const FString& ResumeNode = FString{},
		TFunction<void(const FTryllError&)> OnComplete = nullptr);

	/**
	 * Register game-thread tool sugar. A paused batch auto-resumes only when
	 * every call has a handler and every handler reports success.
	 */
	void RegisterTool(const FString& ToolName, FTryllToolHandler Handler);
	void RegisterTool(const FString& ToolName, FTryllSimpleToolHandler Handler);

	/** Remove a previously registered tool handler. */
	bool UnregisterTool(const FString& ToolName);

	/**
	 * True while a paused tool-call batch is retained: surfaced by the current
	 * pause and not yet cleared by a successful resume Ack, turn end/cancel,
	 * destroy, or a terminal disconnect. A retained batch may already have some
	 * handlers succeeded (see RetryPausedToolBatch).
	 */
	[[nodiscard]] bool HasPausedToolBatch() const { return !PausedToolCalls.IsEmpty(); }

	/**
	 * Re-attempt the retained paused tool-call batch: handlers that already
	 * produced a result on a previous attempt are skipped (not re-run — no
	 * duplicate side effects), remaining handlers run, and a fresh Resume is
	 * sent. No-op if there is no retained batch (HasPausedToolBatch() is false).
	 * Use after OnToolHandlerError reports a failed handler or a
	 * rejected/disconnected resume.
	 */
	void RetryPausedToolBatch();

	/** Discard the retained paused tool-call batch without resuming (terminal disconnect). */
	void ClearPausedToolBatch();

	/**
	 * Name of the node this agent's turn is currently paused after, or an empty
	 * string when the turn is not paused. A non-empty value means the server is
	 * holding the turn open until Resume (or Cancel) — new messages are rejected
	 * with AgentBusy until then.
	 */
	[[nodiscard]] const FString& GetPausedNode() const { return PausedNodeName; }

	/**
	 * Set the callback invoked on the game thread when RegisterTool handler
	 * execution or its automatic result-bearing Resume fails. The turn remains
	 * paused. Mirrors the Unity client's ToolHandlerError event.
	 */
	void SetOnToolHandlerError(TFunction<void(const FTryllError&)> Callback);

	/**
	 * Apply typed node parameters to a workflow node at runtime.
	 * The agent must not be processing a turn.
	 * Completion is invoked on the game thread via the OnComplete callback.
	 *
	 * Clone-set-send idiom (C++):
	 *   auto* P = UTryllNodeParamsFactory::CloneParams(AgentComp->GetNodeParamsBaseline("gen"), this);
	 *   Cast<UTryllGenerateParams>(P)->SystemPrompt = TEXT("New prompt");
	 *   Agent->ChangeParams(TEXT("gen"), P);
	 *
	 * Ticked Localized fields are resolved through the project's text provider on a
	 * copy before send (Params is not modified). An empty Text ID, no provider, or an
	 * unknown ID fails with InvalidParamValue and nothing is sent. To send your own
	 * text for such a field, untick its b<Field>Localized flag first.
	 */
	void ChangeParams(
		const FString&               NodeName,
		UTryllNodeParamsBase*        Params,
		TFunction<void(const FTryllError&)> OnComplete = nullptr);

	/**
	 * Internal: send params whose Text IDs are already resolved (the language-change
	 * refresh). AgentBusy is an expected reply here, so it is not observed as an error.
	 */
	void ChangeLocalizedParams(
		const FString&               NodeName,
		UTryllNodeParamsBase*        Params,
		TFunction<void(const FTryllError&)> OnComplete);

	/**
	 * Replace the agent's voice (protocol v27 SetAgentVoice). Idle-only: AgentBusy
	 * (3004) while a turn runs, unless it is paused before resume. A profile the
	 * model cannot use fails with InvalidVoiceProfile (3023) and the old voice
	 * stays. The active delivery mode survives when the new profile defines a mode
	 * of the same name; otherwise the new profile's default mode applies.
	 * On success GetVoice() returns the new profile.
	 */
	void SetVoice(
		const FTryllVoiceProfileData& Voice,
		TFunction<void(const FTryllError&)> OnComplete = nullptr);

	/**
	 * Switch the voice's delivery mode (protocol v27 SetAgentDeliveryMode). Allowed
	 * mid-turn; applies from the next sentence. "neutral" is always accepted; an
	 * unknown mode fails with UnknownDeliveryMode (3024) and the current mode stays;
	 * an agent without a voice fails with AgentVoiceRequired (3025). Intensity is
	 * clamped to 0..1 by the server; models that cannot use it ignore it.
	 */
	void SetDeliveryMode(
		const FString& Mode,
		float Intensity = 1.f,
		TFunction<void(const FTryllError&)> OnComplete = nullptr);

	/** The voice this agent was created with or last set successfully; unset when it has none. */
	[[nodiscard]] const TOptional<FTryllVoiceProfileData>& GetVoice() const { return Voice; }

	/** Delivery mode last set successfully, or the voice's default ("" = neutral). */
	[[nodiscard]] const FString& GetDeliveryMode() const { return DeliveryMode; }

	/** Record the voice sent at creation (called by the subsystem). */
	void SetCreatedVoice(const TOptional<FTryllVoiceProfileData>& InVoice);

	/** Synchronise every eligible language-model context to its reusable prefix. */
	void PrefillKvCache(
		TFunction<void(const FTryllKvCachePrefillResult&, const FTryllError&)> OnComplete = nullptr);

	/** Release backend KV contexts for every eligible language-model node. */
	void EvictKvCache(
		TFunction<void(const FTryllKvCacheEvictResult&, const FTryllError&)> OnComplete = nullptr);

	/** Query aggregate language-model KV-cache lifecycle state. */
	void GetKvCacheStatus(
		TFunction<void(const FTryllKvCacheStatus&, const FTryllError&)> OnComplete = nullptr);

	/** Append scripted user/assistant interactions without running the graph. Idle-only. */
	void AppendInteractions(
		const TArray<FTryllDialogInteraction>& Interactions,
		TFunction<void(uint32 Count, const FTryllError&)> OnComplete = nullptr);

	/** Remove whole interactions from the end of the dialog. count=0 is a no-op. Idle-only. */
	void RemoveInteractionsFromEnd(
		int32 Count,
		TFunction<void(uint32 RemovedCount, const FTryllError&)> OnComplete = nullptr);

	/** Export the completed dialog as an owned snapshot. Idle-only. Does not write a file. */
	void ExportDialog(
		TFunction<void(const FTryllDialogSnapshot&, const FTryllError&)> OnComplete = nullptr);

	/** Replace the agent's dialog with Snapshot. Idle-only. Never appends. */
	void ImportDialog(
		const FTryllDialogSnapshot& Snapshot,
		ETryllDialogCompatibility Compatibility = ETryllDialogCompatibility::Strict,
		TFunction<void(const FTryllDialogImportResult&, const FTryllError&)> OnComplete = nullptr);

	/**
	 * Return a deep copy of the last authored params for the named node.
	 * Use this as the starting point for a clone-set-send mutation; structural
	 * fields are automatically propagated from the baseline.
	 */
	UTryllNodeParamsBase* GetNodeParamsBaseline(const FString& NodeName) const;

	/** Store/update the authored params baseline for a node (called by the subsystem). */
	void SetNodeParamsBaseline(const FString& NodeName, UTryllNodeParamsBase* Params);

	/** Node names that have a params baseline. */
	TArray<FString> GetNodeParamsBaselineNames() const;

	/** Called by UTryllSubsystem when the destroy Ack arrives; clears the local agent id. */
	void ClearAfterDestroyRequest();

	/** Fail in-flight KV-cache lifecycle callbacks before the agent handle is cleared. */
	void DrainPendingLifecycleCallbacks(const FTryllError& TerminalError);

	// ── Callback setters ──────────────────────────────────────────────────────

	/**
	 * Streaming text callback. Matches the current AnswerText wire message:
	 *   - NodeName  = the workflow node that produced this chunk (multi-sender attribution;
	 *                 ignore it if the graph only ever has one text-producing node).
	 *   - bIsDelta  = true  → Text is an incremental token/chunk.
	 *   - bIsDelta  = false → Text is the full response accumulated so far.
	 *   - bIsFinal  = true  → This is the last AnswerText before TurnComplete for this node.
	 */
	void SetOnAnswerText(TFunction<void(const FString& NodeName, const FString& Text, bool bIsDelta, bool bIsFinal)> Callback);

	/** Called once per turn after all AnswerText chunks, carrying the turn outcome.
	 *  TokensGenerated: server-reported total tokens sampled across all generation
	 *  nodes in this turn (prompt tokens excluded). */
	void SetOnTurnComplete(TFunction<void(ETryllTurnStatus Status, const FString& DebugInfo, int32 TokensGenerated)> Callback);

	/**
	 * Runs before the public turn-complete callback, so a localized param update
	 * that came back AgentBusy goes out before gameplay can start the next turn.
	 * Independent of SetOnTurnComplete, which gameplay replaces.
	 */
	void SetLocalizedTurnEnded(TFunction<void()> Callback);

	/** Called on any send-side error (connection loss, server error response, etc.). */
	void SetOnError(TFunction<void(const FTryllError& Error)> Callback);

	/** Invoke the OnError callback (if set). Used by FTryllAgentVariables to
	 *  surface an auto-flush rejection on the agent's normal error path. */
	void DispatchError(const FTryllError& Error) const;

	/**
	 * TTS audio format callback. Fires once per turn, before any OnTtsAudio.
	 * Parameters: (NodeName, SampleRate, Channels, BitsPerSample). NodeName is
	 * the producing node's name (multi-sender attribution — mirrors
	 * OnAnswerText). Only mono int16 PCM is currently emitted by the server.
	 */
	void SetOnTtsAudioFormat(TFunction<void(const FString& NodeName, uint32 SampleRate, uint16 Channels, uint16 BitsPerSample)> Callback);

	/**
	 * Streaming TTS audio callback. Fires for each TtsAudioFrame within a turn.
	 * NodeName is the producing node's name (multi-sender attribution). The
	 * view references the FTryllConnEvent storage and is valid only for the
	 * duration of the callback. End-of-stream is signalled by OnTurnComplete —
	 * TtsAudioFrame intentionally carries no is_final field.
	 */
	void SetOnTtsAudio(TFunction<void(const FString& NodeName, TArrayView<const int16> Pcm)> Callback);

	/**
	 * Viseme track callback. Fires once per synthesized sentence when the
	 * agent's voice has emit_visemes (FTryllVoiceProfileData::bEmitVisemes) on,
	 * after that sentence's audio. Offsets are turn-absolute samples at the rate from OnTtsAudioFormat;
	 * frames of a turn continue from one another. The view is valid only for the
	 * duration of the callback.
	 */
	void SetOnTtsVisemes(TFunction<void(const FString& NodeName, TArrayView<const FTryllVisemeEvent> Events)> Callback);

	/**
	 * Per-agent NodeEvent callbacks. Names match AgentProxy. No agent id: the
	 * handle is the agent. An empty callback clears the slot.
	 * SetOnToolCallEvent is the owning detailed callback (node + disposition).
	 */
	void SetOnToolCall(TFunction<void(const FString& ToolName, const FString& ArgumentsJson)> Callback);
	void SetOnToolCallWithId(TFunction<void(const FString& CallId, const FString& ToolName, const FString& ArgumentsJson)> Callback);
	void SetOnToolCallEvent(TFunction<void(const FTryllToolCall& Call)> Callback);
	void SetOnIntentClassified(TFunction<void(const FTryllIntentClassification& Classification)> Callback);
	void SetOnIntentLlmClassified(TFunction<void(const FTryllIntentLlmClassification& Classification)> Callback);
	void SetOnPaused(TFunction<void(const FString& NodeName, const FString& PendingExit)> Callback);
	void SetOnNodeEvent(TFunction<void(const FString& NodeName, const FString& EventType, const TArray<FTryllNodeEventKeyValue>& KvPairs)> Callback);

	/**
	 * When set, replaces the "callback slot is non-null" listener check.
	 * UTryllAgentComponent installs forwarders that are always non-null and a
	 * probe that reports the Blueprint delegate's real IsBound() state, so
	 * generic-fallback suppression and the unresumable-pause warning see the
	 * listeners the game actually bound.
	 */
	void SetListenerProbe(TFunction<bool(ETryllAgentNodeEventKind)> Probe);

	// ── Dispatch (called by UTryllSubsystem / UTryllEditorSubsystem on Tick) ──

	void HandleAnswerText(const FString& NodeName, const FString& Text, bool bIsDelta, bool bIsFinal);
	void HandleTurnComplete(ETryllTurnStatus Status, const FString& DebugInfo, int32 TokensGenerated);
	void HandleError(const FTryllError& Error);
	void HandleTtsAudioFormat(const FString& NodeName, uint32 SampleRate, uint16 Channels, uint16 BitsPerSample);
	void HandleTtsAudio(const FString& NodeName, TArrayView<const int16> Pcm);
	void HandleTtsVisemes(const FString& NodeName, TArrayView<const FTryllVisemeEvent> Events);

	/**
	 * Agent-scope half of NodeEvent dispatch. The caller has already broadcast
	 * session-level delegates. Records tool calls, fires the per-agent callbacks,
	 * then (for a pause) runs RegisterTool auto-resume.
	 *
	 * @param bSessionListenerBound  true when a session-scope listener for this
	 *        event's kind is bound: OnPaused for a pause, any of OnToolCall /
	 *        OnToolCallWithId / OnToolCallDetailed for a tool call (subsystem
	 *        Blueprint delegates, or in PIE a direct editor listener / registered
	 *        listener probe). Used only to decide whether a pause is resumable.
	 *        Agent-scope listeners are counted here as well.
	 */
	void HandleNodeEvent(const FTryllDecodedNodeEvent& Event, bool bSessionListenerBound);

	// ── Internal wire calls (called by FTryllAgentVariables) ─────────────────

	/** Send a batched UpdateAgentVariablesRequest (deferred-flush from FTryllAgentVariables::FlushIfDirty). */
	void SendUpdateVariables(
		TArray<FTryllVariableUpdate> Updates,
		TFunction<void(const FTryllError&)> OnComplete,
		uint64 CarrierRequestId = 0);

	/** Send a GetAgentVariablesRequest. Empty Names = every declared variable. */
	void SendGetVariables(TArray<FString> Names,
		TFunction<void(const TArray<FTryllVariableDecl>&, const FTryllError&)> OnComplete);

private:
	bool HasNodeEventListener(ETryllAgentNodeEventKind Kind) const;
	void DispatchGenericNodeEvent(const FTryllDecodedNodeEvent& Event);
	void HandleToolCall(
		const FString& CallId,
		const FString& NodeName,
		const FString& ToolName,
		const FString& ArgumentsJson,
		const FString& Disposition);
	/**
	 * @param bExternalListenerBound  true when any listener (session OnPaused,
	 *        this agent's OnPaused, or this agent's generic OnNodeEvent) can
	 *        resume the pause. When false and no registered tool handler covers
	 *        the batch, nothing can resume the turn and a warning is logged.
	 */
	void HandlePaused(const FString& NodeName, bool bExternalListenerBound);

	void ResumeInternal(
		const FString& ResumeNode,
		TArray<TPair<FString, FString>> ToolResults,
		TFunction<void(const FTryllError&)> OnComplete);

	/**
	 * Run registered handlers for the retained PausedToolCalls batch and, on
	 * full success, send the resume. Skips calls that already have a cached
	 * result from a previous attempt so retry never re-runs a handler that
	 * already succeeded. The batch itself is only cleared once the resume Ack
	 * reports success.
	 */
	/** @return true when the batch fully resolved and a resume was sent. */
	bool RunPausedBatch();

	/** Report a tool-handler/resume failure via OnToolHandlerError, then OnError. */
	void ReportToolHandlerError(const FTryllError& Error);

	TSharedPtr<FTryllConnection> Connection;
	std::uint64_t                AgentId = 0;
	bool                         bCreatedWithDiagnostics = false;
	FString                      DisplayName;

	TFunction<void(const FString&, const FString&, bool, bool)>          OnAnswerText;
	TFunction<void(ETryllTurnStatus, const FString&, int32)>             OnTurnComplete;
	TFunction<void()>                                                    LocalizedTurnEnded;
	TFunction<void(const FTryllError&)>                                  OnError;
	TFunction<void(const FTryllError&)>                                  OnToolHandlerError;
	TFunction<void(const FString&, uint32, uint16, uint16)>              OnTtsAudioFormat;
	TFunction<void(const FString&, TArrayView<const int16>)>             OnTtsAudio;
	TFunction<void(const FString&, TArrayView<const FTryllVisemeEvent>)> OnTtsVisemes;

	TFunction<void(const FString&, const FString&)>                      OnToolCall;
	TFunction<void(const FString&, const FString&, const FString&)>      OnToolCallWithId;
	TFunction<void(const FTryllToolCall&)>                               OnToolCallEvent;
	TFunction<void(const FTryllIntentClassification&)>                   OnIntentClassified;
	TFunction<void(const FTryllIntentLlmClassification&)>                OnIntentLlmClassified;
	TFunction<void(const FString&, const FString&)>                      OnPaused;
	TFunction<void(const FString&, const FString&, const TArray<FTryllNodeEventKeyValue>&)> OnNodeEvent;
	TFunction<bool(ETryllAgentNodeEventKind)>                            ListenerProbe;

	TMap<FString, FTryllToolHandler> ToolHandlers;
	TArray<FTryllToolCall> PendingToolCalls;
	// True once any call in PendingToolCalls reached a tool-call listener (agent or
	// session scope). That listener can answer with ResumeWithToolResult(s), so the
	// pause that surfaces the batch is resumable without an OnPaused listener.
	// Consumed by that pause.
	bool bPendingBatchObserved = false;

	// Retained paused-call batch: the calls surfaced by the current pause, kept
	// alive through handler execution and the resume Ack (not just until handlers
	// start running). Cleared only on a successful resume Ack, turn end/cancel,
	// destroy, or a terminal disconnect — never merely because a handler ran.
	TArray<FTryllToolCall> PausedToolCalls;
	// Results already produced by a handler for this batch, keyed by call id. Lets
	// RetryPausedToolBatch skip handlers that already succeeded instead of re-running
	// them (and their side effects) after a partial failure.
	TMap<FString, FTryllToolResult> PausedToolResults;

	// Node the current turn is parked after, or empty when not paused. Mirrors the
	// server's own pause state from the "paused" NodeEvent; cleared on a successful
	// resume Ack, turn end/cancel, or destroy. Diagnostic: it exists so a hung turn
	// can say which node it is waiting at.
	FString PausedNodeName;

	// Authored params baseline per node name.
	TMap<FString, TObjectPtr<UTryllNodeParamsBase>>      ParamsBaseline;

	// Workflow text table + asset name for resolving ticked Text IDs in ChangeParams.
	FName   TextTable;
	FString LocalizedAssetName;

	void SendChangeParams(
		const FString& NodeName,
		UTryllNodeParamsBase* Params,
		TFunction<void(const FTryllError&)> OnComplete,
		bool bBusyIsExpected);

	// Local mirror of this agent's declared per-agent Variables.
	TUniquePtr<FTryllAgentVariables> Variables;

	// The agent's voice as last accepted by the server, and its delivery mode.
	TOptional<FTryllVoiceProfileData> Voice;
	FString DeliveryMode;
};
