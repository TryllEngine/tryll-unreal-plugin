// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TryllGraphDescription.h"
#include "TryllError.h"
#include "TryllKvCache.h"
#include "TryllThrottle.h"
#include "TryllDialogInteraction.h"
#include "TryllDialogSnapshot.h"
#include "TryllVariableDecl.h"
#include "TryllToolCall.h"
#include "TryllIntentClassification.h"
#include "TryllNodeEvent.h"
#include "TryllVoiceProfile.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "TryllAgentComponent.generated.h"

class UTryllSubsystem;
class UTryllWorkflowAsset;
class UTryllSpeakerComponent;
class FTryllAgentComponentPendingMessageSpec;
class FTryllVoiceInputComponentForwardSpec;
class FTryllAgent;
class FTryllAgentVariables;
class UTryllAgentVariables;

// ── Delegates ─────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTryllAgentReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllAgentError,         const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTryllAgentAnswerText, const FString&, NodeName, const FString&, Text, bool, bIsFinal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllAgentAnswerFull,    const FString&, FullText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTryllAgentTurnComplete, ETryllTurnStatus, Status, const FString&, DebugInfo, int32, TokensGenerated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllParamChanged,      bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllResumed,           bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllVoiceChanged,      const FTryllError&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllDeliveryModeChanged, const FString&, Mode, const FTryllError&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllKvCachePrefilled, const FTryllKvCachePrefillResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllKvCacheEvicted, const FTryllKvCacheEvictResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllKvCacheStatus, const FTryllKvCacheStatus&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllKvCachePrefillError, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllKvCacheEvictError, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllKvCacheStatusError, const FString&, ErrorMessage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllMessageQueued, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllInteractionsAppended, int32, Count);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllAppendInteractionsError, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllInteractionsRemoved, int32, Count);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllRemoveInteractionsError, const FString&, ErrorMessage);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTryllDialogExportedNative, const FTryllDialogSnapshot&, const FTryllError&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTryllDialogImportedNative, const FTryllDialogImportResult&, const FTryllError&);

/// Per-agent NodeEvent surfaces. No AgentId pin: the component is the agent.
/// Session-level equivalents on UTryllSubsystem stay the "all agents" escape hatch.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllAgentToolCall, const FString&, ToolName, const FString&, ArgumentsJson);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTryllAgentToolCallWithId, const FString&, CallId, const FString&, ToolName, const FString&, ArgumentsJson);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllAgentToolCallDetailed, const FTryllToolCall&, Call);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllAgentIntentClassified, const FTryllIntentClassification&, Classification);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllAgentIntentLlmClassified, const FTryllIntentLlmClassification&, Classification);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllAgentPaused, const FString&, NodeName, const FString&, PendingExit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTryllAgentNodeEvent, const FString&, NodeName, const FString&, EventType, const TArray<FTryllNodeEventKeyValue>&, KvPairs);

/**
 * Actor component: manages one Tryll server-side agent.
 * Attach to any Actor; Blueprint users bind to the delegates and call SendMessage().
 *
 * The graph comes from WorkflowAsset — a designer-authored data asset in the
 * Content Browser. CreateAgent fails if no asset is assigned. To build a graph
 * in C++ without an asset, use FTryllGraphBuilder and
 * UTryllSubsystem::RequestCreateAgent.
 *
 * The component auto-creates an agent once the session is ready (if bAutoCreateOnConnect) —
 * it queues until OnCreateSessionComplete if it spawns before the session is configured.
 * It auto-destroys the agent on EndPlay.
 */
UCLASS(ClassGroup = (Tryll), meta = (BlueprintSpawnableComponent))
class TRYLLCLIENT_API UTryllAgentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTryllAgentComponent();

	/**
	 * How to clear a missing Workflow Asset. Shared by the CreateAgent error and the
	 * Details-panel notice, so the Output Log names the same next step the Details panel does.
	 */
	static const TCHAR* const MissingWorkflowAssetHint;

	// ── Properties ────────────────────────────────────────────────────────────

	/** Designer-authored workflow asset. Required for agent creation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Agent")
	UTryllWorkflowAsset* WorkflowAsset = nullptr;

	/**
	 * Per-instance value overrides for the workflow's declared variables
	 * (Material-Instance pattern — name/type come from the declaration; only the
	 * initial value can be overridden). Entries whose Name/Type no longer match
	 * a declared variable are skipped with a warning at agent-creation time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Variables")
	TArray<FTryllVariableOverride> VariableOverrides;

	/** Automatically create the agent once the session is ready (connected + configured). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Agent",
		meta = (ToolTip = "When true, the component creates the agent automatically once the session is ready (connected AND configured). If it spawns before then, creation is queued until the session configures. (Property name kept for compatibility; it fires on session-ready, not raw connect.)"))
	bool bAutoCreateOnConnect = true;

	/**
	 * Request the server to populate TurnComplete.debug_info with workflow execution data.
	 * Authoring UI retired (Package 2): Chat and Agent Log OR this on via transient
	 * override / diagnostics-demand lease. Kept for runtime/Blueprint API and asset
	 * compatibility; default remains false (zero overhead in production).
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Tryll|Agent", meta = (EditCondition = "false", EditConditionHides))
	bool bEnableDiagnostics = false;

	/**
	 * When true (default) the agent keeps its full dialogue history across turns.
	 * When false the history is discarded after each turn — a stateless agent,
	 * useful for technical/classification agents that should not store or
	 * project prior turns.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Agent")
	bool bMaintainDialogueHistory = true;

	/** How language-model KV contexts are materialised at agent creation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|KV Cache")
	ETryllKvCacheInitialization KvCacheInitialization = ETryllKvCacheInitialization::AllocateOnly;

	/**
	 * Whether a human ever waits on this agent's turns. Set Background for
	 * world-logic/evaluator agents so the inference scheduler paces or defers
	 * their work first under GPU pressure. Purely a scheduling hint.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Agent")
	ETryllWorkload Workload = ETryllWorkload::Interactive;

	/**
	 * Optional speaker component used to play streaming TTS audio frames emitted
	 * by the agent (e.g. from a GenerateAndSpeak node).
	 *
	 * If left unset, CreateAgent uses the UTryllSpeakerComponent on the same
	 * actor, and adds one if there is none and the workflow has a
	 * GenerateAndSpeak or Speak node. Set this explicitly when the owning actor
	 * has more than one speaker and you need to pick a specific one.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Tryll|TTS",
		meta = (ToolTip = "Optional. Auto-discovered from the same actor if left null, and added if the workflow speaks and the actor has none. Only set explicitly when the actor has multiple UTryllSpeakerComponent instances."))
	TObjectPtr<UTryllSpeakerComponent> Speaker = nullptr;

	/**
	 * The agent's voice (protocol v27): TTS model, identity, visemes and delivery
	 * modes. Every speaking node of the workflow uses it. Required when the
	 * workflow has a Speak or Generate And Speak node: creation fails with
	 * AgentVoiceRequired (3025) without one. Sent at CreateAgent; change it on a
	 * live agent with SetVoice. One workflow can serve many characters, each with
	 * its own profile.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tryll|TTS",
		meta = (ToolTip = "The agent's voice. Required when the workflow speaks (Speak / Generate And Speak). Create one with Content Browser > Tryll > Voice Profile."))
	TObjectPtr<UTryllVoiceProfile> VoiceProfile = nullptr;

	/** Override the Voice Profile's default delivery for this agent instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|TTS")
	bool bOverrideVoiceDelivery = false;

	/** Initial delivery used when bOverrideVoiceDelivery is on. Empty = neutral. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|TTS",
		meta = (EditCondition = "bOverrideVoiceDelivery"))
	FString VoiceDelivery;

	// ── Actions ───────────────────────────────────────────────────────────────

	/**
	 * Request creation of a server-side agent from WorkflowAsset.
	 * Fires OnAgentReady on success; OnError on failure (including when no
	 * WorkflowAsset is assigned). Safe to call from Blueprint Begin Play.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Agent")
	void CreateAgent();

	/** Request destruction of the current server-side agent. OnAgentDestroyed (on the subsystem) fires on completion. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Agent")
	void DestroyAgent();

	/**
	 * Send a user-turn message to the agent.
	 * Streaming tokens arrive on OnAnswerText; the turn ends with OnTurnComplete + OnAnswerFull.
	 * No-op if the agent is not yet ready (IsAgentReady() == false).
	 *
	 * The message is **queued, not dropped**, when the agent exists but cannot accept it yet. An
	 * agent refuses a send with AgentBusy while a turn is running or a KV-cache operation is in
	 * flight — and a cache operation is exactly what happens when a game prefills the agent the
	 * player has just selected. Without queueing, a message typed during that window is silently
	 * lost and the player must retype it, which is the worst available outcome. The queue is
	 * flushed automatically when the agent becomes ready, when a turn completes, and when a cache
	 * operation finishes.
	 *
	 * Only the most recent message is held: if two are sent before either reaches the agent, the
	 * newer line is the one the player meant. OnMessageQueued fires when a send is deferred.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Agent")
	void SendMessage(const FString& Message);

	/**
	 * Hands the queued message to the agent if it can accept one now. Safe and cheap to call
	 * repeatedly; returns true only when a message was actually handed over.
	 *
	 * Called automatically on agent-ready, turn-complete and cache-operation-complete. It is
	 * public because a KV-residency policy that drives prefills through FTryllAgent directly —
	 * rather than through this component — closes a busy window this component never observed, and
	 * should nudge the queue when it does.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Agent")
	bool TryFlushPendingMessage();

	/** True while a message is waiting for the agent to become available. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Agent")
	bool HasPendingMessage() const { return bHasPendingMessage && !PendingMessage.IsEmpty(); }

	/** True while a turn this component started has not yet completed. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Agent")
	bool IsTurnInFlight() const { return bTurnInFlight; }

	/**
	 * Native access to the underlying agent, for C++ callers that need per-call callbacks the
	 * Blueprint event surface cannot express — a KV-residency policy managing several agents, for
	 * instance, needs to know which prefill finished. Not a UFUNCTION: TSharedPtr is not a
	 * Blueprint type. Mirrors the public `Agent` property on the Unity component.
	 *
	 * May be null before creation completes; check IsAgentReady() first.
	 */
	const TSharedPtr<FTryllAgent>& GetAgent() const { return Agent; }

	/** The queued message, or empty. For a "waiting to send" indicator. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Agent")
	const FString& GetPendingMessage() const { return PendingMessage; }

	/**
	 * Mutate a named parameter on a workflow node at runtime.
	 * The agent must not be processing a turn.
	 * On completion (success or error), OnParamChanged fires.
	 *
	 * Use the clone-set-send idiom:
	 *   auto* P = Cast<UTryllGenerateParams>(GetNodeParamsBaseline("gen"));
	 *   auto* Mut = Cast<UTryllGenerateParams>(UTryllNodeParamsFactory::CloneParams(P, this));
	 *   Mut->SystemPrompt = TEXT("New prompt");
	 *   ChangeParams(TEXT("gen"), Mut);
	 *
	 * @param NodeName  Instance name of the target node in the graph.
	 * @param Params    Fully specified typed params for the node (clone of baseline + edits).
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Agent")
	void ChangeParams(const FString& NodeName, UTryllNodeParamsBase* Params);

	/**
	 * Resume a turn that is currently paused (see OnPaused on this component, or
	 * UTryllSubsystem::OnPaused for every agent). On completion (success or
	 * error), OnResumed fires.
	 *
	 * @param ResumeNode  Empty continues via the paused node's pending exit route;
	 *                    non-empty jumps to that node by name.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Agent")
	void Resume(const FString& ResumeNode = TEXT(""));

	/** Resume with one result correlated to a tool-call ID. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Tools")
	void ResumeWithToolResult(
		const FTryllToolResult& ToolResult,
		const FString& ResumeNode = TEXT(""));

	/** Resume with the complete result batch for one paused tool-call pass. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Tools")
	void ResumeWithToolResults(
		const TArray<FTryllToolResult>& ToolResults,
		const FString& ResumeNode = TEXT(""));

	/** Register native game-thread auto-resume sugar for a named tool. */
	void RegisterTool(const FString& ToolName, FTryllToolHandler Handler);
	void RegisterTool(const FString& ToolName, FTryllSimpleToolHandler Handler);

	/** Remove a previously registered native tool handler. */
	bool UnregisterTool(const FString& ToolName);

	/**
	 * Look up a registered tool handler without going through an agent, or nullptr when this
	 * component has none for that name. Lets an external driver that runs its own agent — the
	 * Dialog Lab — invoke the same game-side handler this component would have used, instead of
	 * simulating it with authored data.
	 *
	 * A handler that keeps per-conversation state must be told when the dialogue restarts, and
	 * such a driver's agent is not this component's, so none of the delegates here fire for it.
	 * The convention is a parameterless `UFUNCTION()` named `ResetToolState` on the handler's
	 * component, which the driver dispatches by name. The Chat panel and the Dialog Lab both run
	 * their own agent and dispatch it at each new conversation. In-game callers need nothing extra:
	 * the handler binds OnAgentReady and OnInteractionsRemoved itself.
	 *
	 * The returned pointer is owned by this component's handler map and is invalidated by
	 * RegisterTool / UnregisterTool. Call it, do not store it.
	 */
	const FTryllToolHandler* FindToolHandler(const FString& ToolName) const;

	/**
	 * True while a paused tool-call batch is retained: surfaced by the current
	 * pause and not yet cleared by a successful resume Ack, turn end/cancel,
	 * destroy, or a terminal disconnect. False when no agent is ready.
	 */
	UFUNCTION(BlueprintPure, Category = "Tryll|Tools")
	bool HasPausedToolBatch() const;

	/**
	 * Name of the node this agent's turn is paused after, or an empty string when
	 * it is not paused. Non-empty means the server is holding the turn open until
	 * Resume (or Cancel) — Send Message will keep failing with AgentBusy until one
	 * of those arrives. Covers Pause nodes as well as pausing tool calls, unlike
	 * Has Paused Tool Batch.
	 */
	UFUNCTION(BlueprintPure, Category = "Tryll|Agent")
	FString GetPausedNode() const;

	/**
	 * Re-attempt the retained paused tool-call batch: handlers that already
	 * produced a result on a previous attempt are skipped, remaining handlers
	 * run, and a fresh Resume is sent. No-op when no agent is ready or there is
	 * no retained batch.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Tools")
	void RetryPausedToolBatch();

	/**
	 * Cooperatively cancel the in-flight turn. The turn ends with
	 * ETryllTurnStatus::Cancelled via OnTurnComplete. No-op if idle or the agent
	 * is not ready. Mode selects whether the partial interaction is kept
	 * (StopAndKeep, the chat "Stop" button) or discarded (StopAndDiscard).
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Agent")
	void Cancel(ETryllCancelMode Mode = ETryllCancelMode::StopAndKeep);

	/**
	 * Change the agent's voice. With a live agent this sends SetAgentVoice
	 * (idle-only: AgentBusy 3004 during a turn; InvalidVoiceProfile 3023 when the
	 * model cannot use the profile, and the old voice stays) and assigns
	 * VoiceProfile on success. Without an agent it only assigns VoiceProfile for
	 * the next CreateAgent. OnVoiceChanged reports the result either way.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|TTS")
	void SetVoice(UTryllVoiceProfile* NewVoice);

	/**
	 * Switch the voice's delivery mode, e.g. "angry". Allowed mid-turn; applies
	 * from the next sentence. "neutral" always works; a mode the profile does not
	 * define fails with UnknownDeliveryMode (3024) and the current mode stays.
	 * Intensity (0..1) scales the mode where the model can (speed scale, Maya1
	 * clause strength, Chatterbox expressiveness). OnDeliveryModeChanged reports
	 * the result.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|TTS", meta = (AdvancedDisplay = "Intensity"))
	void SetDeliveryMode(const FString& Mode, float Intensity = 1.f);

	/** The delivery mode last applied to the live agent ("" = neutral, or the voice's default before any switch). */
	UFUNCTION(BlueprintPure, Category = "Tryll|TTS")
	FString GetDeliveryMode() const;

	/** Voice profile data with this component's delivery override applied to a copy. */
	TOptional<FTryllVoiceProfileData> GetEffectiveVoiceProfile() const;

	/**
	 * The agent component on Owner whose speaker is Speaker (or which has no
	 * speaker yet and so will auto-discover it), or null. Lip-sync components use
	 * it to read the voice profile of the agent they animate.
	 */
	static UTryllAgentComponent* FindForSpeaker(const AActor* Owner, const UTryllSpeakerComponent* Speaker);

	/** Synchronise every eligible language-model context to its reusable prefix. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|KV Cache")
	void PrefillKvCache();

	/** Release every eligible language-model backend KV context. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|KV Cache")
	void EvictKvCache();

	/** Query aggregate language-model KV-cache lifecycle state. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|KV Cache")
	void GetKvCacheStatus();

	/** Append scripted user/assistant interactions without running the graph. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Dialog")
	void AppendInteractions(const TArray<FTryllDialogInteraction>& Interactions);

	/** Remove whole interactions from the end of the dialog. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Dialog")
	void RemoveInteractionsFromEnd(int32 Count);

	/** Export the completed dialog. C++ only — snapshot is not Blueprint-exposed. */
	void ExportDialog();

	/** Replace the agent's dialog. C++ only — snapshot is not Blueprint-exposed. */
	void ImportDialog(
		const FTryllDialogSnapshot& Snapshot,
		ETryllDialogCompatibility Compatibility = ETryllDialogCompatibility::Strict);

	/**
	 * Return the authored params baseline for the named node.
	 * Call after OnAgentReady has fired; returns null if the node name is unknown.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Agent")
	UTryllNodeParamsBase* GetNodeParamsBaseline(const FString& NodeName) const;

	/**
	 * Blueprint/C++ access to this agent's declared per-agent Variables, or
	 * nullptr if no agent is ready. Use to Set/Get/AddToSet/RemoveFromSet/Reset
	 * declared variables from Blueprint or C++ — see UTryllAgentVariables. (C++
	 * code that needs the raw mirror can use the agent's own
	 * FTryllAgent::GetVariables().)
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Variables")
	UTryllAgentVariables* GetVariables();

	/**
	 * C++ access to the live variables mirror: the PIE agent when ready, otherwise
	 * an editor-preview agent attached by Tryll Chat (if any). Null when neither
	 * is active. Used by Details customization for live override apply / Flush Now.
	 */
	FTryllAgentVariables* GetLiveVariables();

	/** True when a PIE agent or editor-preview agent is available for live variable edits. */
	bool HasLiveAgent() const;

#if WITH_EDITOR
	/**
	 * Transient weak link used by STryllChatPanel so Details edits affect the
	 * editor-owned chat agent without PIE, and so RegisterTool handlers already
	 * on this component are copied onto that preview agent. Does not serialize;
	 * Chat remains the sole owner of the TSharedPtr.
	 */
	void AttachEditorPreviewAgent(const TSharedPtr<FTryllAgent>& Preview);
	void DetachEditorPreviewAgent();
#endif

	// ── Queries ───────────────────────────────────────────────────────────────

	/** True after the server has confirmed agent creation (OnAgentReady has fired). */
	UFUNCTION(BlueprintPure, Category = "Tryll|Agent")
	bool IsAgentReady() const { return bAgentReady; }

	/** Server-assigned agent id, or 0 if the agent has not been created yet. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Agent")
	int64 GetAgentId() const { return static_cast<int64>(AgentIdNumeric); }

	// ── Delegates ─────────────────────────────────────────────────────────────

	/** Fired once after the server confirms the agent has been created. */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentReady OnAgentReady;

	/**
	 * Fired for each streaming AnswerText chunk.
	 *   - Text    = the chunk (delta or accumulated, per server config).
	 *   - bIsFinal = true on the last chunk before TurnComplete.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentAnswerText OnAnswerText;

	/** Fired after TurnComplete with the full accumulated response text. */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentAnswerFull OnAnswerFull;

	/** Fired when TurnComplete arrives, carrying the turn outcome status. */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentTurnComplete OnTurnComplete;

	/** Fired on any agent-level error (send failure, server error response). */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentError OnError;

	/**
	 * Fired when a ChangeParam request completes.
	 *   - bSuccess    = true on Ack; false on any server error.
	 *   - ErrorMessage = empty on success; human-readable description on failure.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllParamChanged OnParamChanged;

	/**
	 * Fired when a Resume request completes.
	 *   - bSuccess    = true on Ack; false on any server error (e.g. AgentNotPaused, UnknownNode).
	 *   - ErrorMessage = empty on success; human-readable description on failure.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllResumed OnResumed;

	/** Fired when SetVoice completes. Error.Code is 0 on success (InvalidVoiceProfile 3023, AgentBusy 3004, ... on failure). */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllVoiceChanged OnVoiceChanged;

	/** Fired when SetDeliveryMode completes, with the requested mode. Error.Code is 0 on success (UnknownDeliveryMode 3024, AgentVoiceRequired 3025, ... on failure). */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllDeliveryModeChanged OnDeliveryModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|KV Cache")
	FOnTryllKvCachePrefilled OnKvCachePrefilled;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|KV Cache")
	FOnTryllKvCacheEvicted OnKvCacheEvicted;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|KV Cache")
	FOnTryllKvCacheStatus OnKvCacheStatus;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|KV Cache")
	FOnTryllKvCachePrefillError OnKvCachePrefillError;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|KV Cache")
	FOnTryllKvCacheEvictError OnKvCacheEvictError;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|KV Cache")
	FOnTryllKvCacheStatusError OnKvCacheStatusError;

	/**
	 * Fired when SendMessage could not hand the message over immediately and queued it instead.
	 * Carries the queued text, so a game can show that it is waiting rather than leaving the
	 * player wondering whether their line registered.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Agent")
	FOnTryllMessageQueued OnMessageQueued;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Dialog")
	FOnTryllInteractionsAppended OnInteractionsAppended;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Dialog")
	FOnTryllAppendInteractionsError OnAppendInteractionsError;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Dialog")
	FOnTryllInteractionsRemoved OnInteractionsRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Dialog")
	FOnTryllRemoveInteractionsError OnRemoveInteractionsError;

	FOnTryllDialogExportedNative OnDialogExported;
	FOnTryllDialogImportedNative OnDialogImported;

	/** Tool call for this agent only (tool name, arguments JSON). */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentToolCall OnToolCall;

	/** Call-id-aware tool call for this agent. Use the id when resuming with a result. */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentToolCallWithId OnToolCallWithId;

	/**
	 * Detailed tool call for this agent (node name and disposition included).
	 * Fired in addition to OnToolCall / OnToolCallWithId. Binding it also
	 * suppresses this component's OnNodeEvent for that event.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentToolCallDetailed OnToolCallDetailed;

	/** Embedding ClassifyIntent result for this agent. No agent id: this component is the agent. */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentIntentClassified OnIntentClassified;

	/** ClassifyIntentLLM result for this agent. */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentIntentLlmClassified OnIntentLlmClassified;

	/**
	 * This agent's turn paused between nodes. Call Resume, or RegisterTool a
	 * handler for every tool in the batch. Binding this suppresses OnNodeEvent
	 * for the pause. UTryllSubsystem::OnPaused remains the all-agents surface.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentPaused OnPaused;

	/**
	 * Generic NodeEvent for this agent, for event types with no typed listener
	 * bound on this component. A typed listener here does not affect
	 * UTryllSubsystem::OnNodeEvent, and the reverse.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentNodeEvent OnNodeEvent;

#if WITH_EDITOR
	/** A workflow that speaks needs a VoiceProfile (the server's AgentVoiceRequired, 3025). */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleConnectionChanged(bool bConnected);

	UFUNCTION()
	void HandleCreateSessionComplete(bool bSuccess);

	friend class FTryllAgentComponentPendingMessageSpec;
	friend class FTryllVoiceInputComponentForwardSpec;
	friend class FTryllAgentNodeEventSpec;
	friend class FTryllAgentToolCallResumeSpec;
	friend class FTryllAgentComponentCreateInFlightSpec;
	friend class FTryllAgentVoiceSpec;

	/**
	 * Completes the CreateAgent request tagged Generation. A response whose generation is
	 * no longer current (DestroyAgent, EndPlay or a disconnect ran while it was pending)
	 * is dropped and its server agent destroyed, so it never fires OnAgentReady.
	 */
	void HandleCreateAgentResponse(uint32 Generation, TSharedPtr<FTryllAgent> CreatedAgent, const FTryllError& Err);
	/** Orphans any pending CreateAgent response and allows a new request. */
	void InvalidateCreateState();

	void ResolveSpeaker(const FTryllGraphDescription& Graph);
	/** The voice CreateAgent sends: VoiceProfile's data, or unset (logging an error when Graph speaks). */
	TOptional<FTryllVoiceProfileData> ResolveCreateVoice(const FTryllGraphDescription& Graph) const;
	void BindAgentCallbacks();
	void HandleInboundError(const FTryllError& Err);
	void ClearSendQueue();

	/**
	 * Merge a workflow's declared variables with this component's enabled value
	 * overrides (Material-Instance pattern: an override may only replace the
	 * initial value of a declaration with the same name AND type). Overrides
	 * that don't match any declared variable, or whose type disagrees, are
	 * skipped with a warning (stale — e.g. after the declaration changed).
	 */
	static TArray<FTryllVariableDecl> MergeVariableOverrides(
		const TArray<FTryllVariableDecl>& Declared, const TArray<FTryllVariableOverride>& Overrides);

	UTryllSubsystem*        TryllSubsystem  = nullptr;
	TSharedPtr<FTryllAgent> Agent;

#if WITH_EDITOR
	/// Weak preview agent owned by STryllChatPanel (editor Test Chat). Not serialized.
	TWeakPtr<FTryllAgent> EditorPreviewAgent;
#endif

	// Lazily-created Blueprint wrapper returned by GetVariables(); kept as a
	// UPROPERTY so it is not garbage-collected while a Blueprint holds it.
	UPROPERTY(Transient)
	UTryllAgentVariables* VariablesObject = nullptr;

	std::uint64_t AgentIdNumeric   = 0;
	bool          bAgentReady      = false;

	/// Typed but not yet accepted by the agent — see SendMessage.
	FString       PendingMessage;
	/// Whether PendingMessage holds anything.
	bool          bHasPendingMessage = false;
	/// Handed to the agent but not yet confirmed by a turn, so it can be re-queued.
	FString       InFlightMessage;
	/// A turn this component started and has not seen complete. Conservative: it tracks only the
	/// busy windows this component can see, and the error path re-queues if the server refuses.
	bool          bTurnInFlight    = false;
	/// CreateAgent was requested before the session was ready; create on configure-complete.
	bool          bCreateQueued    = false;
	/// A CreateAgent request is waiting for its response; further CreateAgent calls are no-ops.
	bool          bCreateInFlight  = false;
	/// Tags each CreateAgent request; bumped to orphan a pending response.
	uint32        CreateGeneration = 0;
	FString       AccumulatedText;
	TMap<FString, FTryllToolHandler> ToolHandlers;
};
