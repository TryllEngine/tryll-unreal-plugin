// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TryllError.h"
#include "TryllVoiceProfile.h"
#include "TryllGraphDescription.h"
#include "TryllIntentClassification.h"
#include "TryllModelManifest.h"   // ETryllMemoryPlacement
#include "TryllKvCache.h"
#include "TryllThrottle.h"
#include "TryllVariableDecl.h"
#include "TryllMemorySnapshot.h"
#include "TryllMkbSnapshot.h"
#include "TryllMutableKb.h"
#include "TryllMutableKbTypes.h"
#include "TryllModelInfo.h"
#include "TryllVoiceInput.h"
#include "TryllRuntimeSettings.h"
#include "TryllTextResolve.h"
#include "TryllToolCall.h"
#include "TryllNodeEvent.h"
#include "TryllManagedServerProcess.h"
#include "TryllSubsystem.generated.h"

class FTryllConnection;
class FTryllConnectionEvents;
struct FTryllConnEvent;
class FTryllAgent;
class FTryllVoiceInput;
class FTryllMutableKb;

// ── Delegate type aliases (avoid comma-parsing issues in UHT) ─────────────────

using FTryllOnAgentCreated    = void(TSharedPtr<FTryllAgent>, FTryllError);
using FTryllOnListModels      = void(TArray<FTryllModelInfo>, FTryllError);
using FTryllOnGetMemoryConsumption = void(FTryllMemorySnapshot, FTryllError);
using FTryllOnCreateSession = void(FTryllError);
using FTryllOnVoiceInputCreated = void(TSharedPtr<FTryllVoiceInput>, FTryllError);
using FTryllOnEnsureMutableKb = void(TSharedPtr<FTryllMutableKb>, FTryllError);
using FTryllOnEnumerateMutableKbs = void(TArray<FTryllMutableKbInfo>, FTryllError);
using FTryllOnSaveGameMemory = void(FTryllGameMemorySaveResult, FTryllError);
using FTryllOnLoadGameMemory = void(FTryllGameMemoryLoadResult, FTryllError);

/** Compact agent descriptor for Agent Log registration snapshots. */
struct FTryllRegisteredAgentSnapshot
{
	uint64  AgentId                 = 0;
	FString LabelHint;
	bool    bCreatedWithDiagnostics = false;
};

// ── Blueprint-assignable delegates ────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllConnectionChanged, bool, bConnected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllError,             const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllAgentDestroyed,    int64, AgentId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTryllCreateSession,  bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllListModels,       const TArray<FTryllModelInfo>&, Models, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllGetMemoryConsumption, const FTryllMemorySnapshot&, Snapshot, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllEnsureMutableKb, const FString&, Name, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllEnumerateMutableKbs, int32, StoreCount, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllSaveGameMemory, int32, StoreCount, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllLoadGameMemory, int32, StoreCount, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnTryllDownloadProgress,  const FString&, ModelName, int64, BytesDownloaded, int64, TotalBytes, float, Percent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTryllDownloadComplete,  const FString&, ModelName, bool, bSuccess, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllLoadModel,         const FString&, ModelName, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllUnloadModel,       const FString&, ModelName, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllSetSessionLocale, const FString&, Locale,    bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTryllLocalizedTextFailed, int64, AgentId, const FString&, AssetName, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllLocalizedTextApplied, int64, AgentId, const FString&, AssetName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTryllToolCall, int64, AgentId, const FString&, ToolName, const FString&, ArgumentsJson);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnTryllToolCallWithId,
	int64, AgentId,
	const FString&, CallId,
	const FString&, ToolName,
	const FString&, ArgumentsJson);

/**
 * Detailed tool-call event carrying the full owning FTryllToolCall value object
 * (node name and disposition included) alongside the agent id. Fired in addition
 * to — never instead of — OnToolCall / OnToolCallWithId, so a manual handler can
 * tell whether to send results for await_result or plain-resume for other
 * pausing dispositions.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnTryllToolCallDetailed,
	int64, AgentId,
	const FTryllToolCall&, Call);

/// Embedding ClassifyIntent — event_type="intent_classified". Subscribing swallows OnNodeEvent.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnTryllIntentClassified,
	int64, AgentId,
	const FTryllIntentClassification&, Classification);

/// ClassifyIntentLLM — event_type="intent_llm_classified". Subscribing swallows OnNodeEvent.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnTryllIntentLlmClassified,
	int64, AgentId,
	const FTryllIntentLlmClassification&, Classification);

/// Pause typed event — fired for NodeEvent event_type="paused" (the executor
/// paused the turn between nodes; ChangeAgentParam is allowed while paused).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnTryllPaused,
	int64, AgentId,
	const FString&, NodeName,
	const FString&, PendingExit);

/// Generic NodeEvent fallback — fired for any event_type without a typed delegate
/// (or when the typed delegate has no subscribers).  KvPairs preserves wire order.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnTryllNodeEvent,
	int64, AgentId,
	const FString&, NodeName,
	const FString&, EventType,
	const TArray<FTryllNodeEventKeyValue>&, KvPairs);

/**
 * Game-instance subsystem: one TCP session to the Tryll server.
 * Manages the socket connection on a background thread (FTryllConnection) and
 * drains the event queue on Tick(), dispatching to registered agents and delegates.
 *
 * Default (zero-wiring) path — driven by UTryllRuntimeSettings:
 *   - bAutoLaunchServer     → spawns the bundled tryll_server.exe on Initialize.
 *   - bAutoConnect          → Connect() on Initialize (connect-retry covers server startup).
 *   - bAutoCreateSession → on Connected, CreateSessionFromSettings() sends the game
 *                             name, locale and the manifest-derived engine hint.
 *   UTryllAgentComponent / UTryllVoiceInputComponent then create themselves once
 *   IsSessionReady() — they queue until OnCreateSessionComplete if they spawn earlier.
 *   So the common case is: download models in the Model Manager, add the components,
 *   press play. There is no engine to pick: each model's catalog entry names it.
 *
 * Manual path — turn those flags off to drive the session yourself:
 *   1. Connect()                              — open TCP session to ServerHost:ServerPort.
 *   2. OnConnectionChanged(bConnected=true)   — bind to continue; or poll IsConnected().
 *   3. CreateSession(GameName, Locale)     — configure explicitly (overrides settings).
 *                                               Completion arrives on OnCreateSessionComplete;
 *                                               IsSessionReady() is true thereafter.
 *   4. CreateAgent                            — attach a UTryllAgentComponent (auto-creates once
 *                                               the session is ready) or call RequestCreateAgent().
 *   5. SendMessage(Text)                      — drive one turn; chunks on OnAnswerText, the turn
 *                                               ends with OnTurnComplete(Status).
 *   6. RequestDestroyAgent / Disconnect       — tear down when done.
 *
 * All delegates fire on the game thread from Tick(); it is safe to touch UObjects
 * and UI directly from callbacks.
 */
UCLASS()
class TRYLLCLIENT_API UTryllSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// ── UGameInstanceSubsystem ────────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── FTickableGameObject ───────────────────────────────────────────────────

	virtual void          Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool          IsTickable() const override;
	virtual bool          IsTickableWhenPaused() const override { return false; }
	virtual bool          IsTickableInEditor()   const override { return false; }
	virtual TStatId       GetStatId()            const override;

	// ── Connection settings ───────────────────────────────────────────────────

	/** Host name or IP address of the Tryll server. Defaults to localhost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Connection",
		meta = (ToolTip = "Host name or IP address of the Tryll server (default: 127.0.0.1)."))
	FString ServerHost = TEXT("127.0.0.1");

	/** TCP port the Tryll server is listening on (default: 9100). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Connection",
		meta = (ToolTip = "TCP port the Tryll server is listening on (default: 9100).", ClampMin = "1", ClampMax = "65535"))
	int32 ServerPort = 9100;

	// ── Connection management ─────────────────────────────────────────────────

	/**
	 * Open a TCP session to ServerHost:ServerPort.
	 * Asynchronous — completion is reported via OnConnectionChanged(bConnected=true).
	 * Errors fire OnError. Calling Connect() while already connected is a no-op.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Connection")
	void Connect();

	/** Close the active session. OnConnectionChanged(bConnected=false) fires once the socket is down. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Connection")
	void Disconnect();

	/** True while the background connection is in the Connected state. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Connection")
	bool IsConnected() const;

	/** Server-assigned session id, or 0 when not connected. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Connection")
	int64 GetSessionId() const;

	// ── Session configuration ─────────────────────────────────────────────────

	/**
	 * Send CreateSessionRequest. Must be called after Connected and before CreateAgent.
	 * Result is delivered via OnCreateSessionComplete delegate.
	 *
	 * @param GameName                  Integration identifier for telemetry grouping
	 *                                  (e.g. "tryll-roleplay-demo").  Leave empty to omit;
	 *                                  the server defaults to the project name, then
	 *                                  "unknown", when telemetry is on.
	 *                                  Pass UTryllRuntimeSettings::GameName to drive this from
	 *                                  the project settings asset.
	 * @param Locale                    Session language as a BCP-47 tag (e.g. "de", "pt-BR").
	 *                                  See UTryllRuntimeSettings::Locale for everything it
	 *                                  drives. Empty uses the project setting unless
	 *                                  bUseProjectLocaleWhenEmpty is false; a malformed
	 *                                  tag is rejected with InvalidLocale (2004).
	 * @param bUseProjectLocaleWhenEmpty When true, an empty Locale adopts the text
	 *                                  provider's current language, then
	 *                                  UTryllRuntimeSettings::Locale. Set false with an
	 *                                  empty Locale to explicitly disable session language.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Session")
	/**
	 * The four engine parameters this took through protocol v12 are gone. The
	 * engine set is derived from the models registered in UTryllModelManifest —
	 * each model's catalog entry already states which engine runs it, so there is
	 * nothing for a caller to choose, and nothing that can drift out of step with
	 * the models the project references.
	 *
	 * Storage root is process-wide (set when the server launches via --storage-root),
	 * not per session — protocol v24 removed CreateSessionRequest.storage_data_folder.
	 */
	void CreateSession(
		FString GameName = TEXT(""),
		FString Locale = TEXT(""),
		bool bUseProjectLocaleWhenEmpty = true);

	/**
	 * Change the session language without tearing the session down. Pass an
	 * empty Locale to clear it.
	 *
	 * Applies to every agent, existing and future, from its next turn: TTS
	 * language and the locale template keys are read per turn, not captured at
	 * agent creation. A new voice input picks up the new STT language hint; one
	 * already open keeps its recognizer. Storage paths re-resolve on the next
	 * load, but content already bound to a node (a Retrieve KB, canned lines)
	 * does NOT swap - those params are structural, so switching authored content
	 * still means recreating agents.
	 *
	 * Not turn-atomic: a turn already running may render its prompt under the
	 * old locale and synthesize under the new one.
	 *
	 * Completion is broadcast via OnSetSessionLocaleComplete.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Session")
	void SetSessionLocale(FString Locale);

	/**
	 * Tell Tryll the text provider's language changed (custom providers call this).
	 * Sends SetSessionLocale unless CreateSession was given an explicit locale, then
	 * re-resolves Localized prompts on live agents. Also reaches the editor session.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Session")
	void NotifyTextLocaleChanged();

	/**
	 * Report how hard the server should yield the GPU back to the game (the
	 * manual inference throttle knob). Fire-and-forget: no response, no error
	 * surfaced locally. Never sent automatically — a session that never calls
	 * this runs unthrottled.
	 * @param Level  0 = full speed (default), 1 = maximum yielding. Clamped
	 *               server-side; out-of-range values do not fail here.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Session")
	void SetInferenceThrottle(float Level);

	/**
	 * Configure the session from project settings (UTryllRuntimeSettings): game name,
	 * locale, and the engine warm-up hint derived from UTryllModelManifest. Storage
	 * root is process-wide (set at server launch).
	 *
	 * This is the "use settings" path. It runs automatically once connected when
	 * UTryllRuntimeSettings::bAutoCreateSession is true; call it yourself if you
	 * connected with that flag disabled and still want the settings-driven config.
	 * To override settings, call CreateSession() with explicit parameters instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Session")
	void CreateSessionFromSettings();

	/**
	 * True once the current connection has a successfully configured session
	 * (CreateSession completed OK). Agent / VoiceInput creation must wait for
	 * this — creating against an unconfigured session is rejected by the server.
	 */
	UFUNCTION(BlueprintPure, Category = "Tryll|Session")
	bool IsSessionReady() const;

	// ── Server identity (from ConnectionReady) ─────────────────────────────────

	/** Canonical path of the connected server executable (empty if unknown). */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Server")
	FString ServerExePath;

	/** Server build configuration ("Debug" | "Release" | "ClientDev" | "ClientProd"). */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Server")
	FString ServerBuildConfig;

	/** Deployment mode ("tryll-dev" | "client-dev" | "client-prod"). */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Server")
	FString ServerMode;

	/** Server version string. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Server")
	FString ServerVersion;

	/** Per-process server run id (UUID). */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Server")
	FString ServerRunId;

	/** True when the server runs in managed (idle-self-exit) mode. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Server")
	bool bServerManaged = false;

	/** Server idle-shutdown grace period in seconds (0 when standalone). */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Server")
	int32 ServerIdleTimeoutSeconds = 0;

	// ── Agent lifecycle ───────────────────────────────────────────────────────

	/**
	 * Begin create-agent; completion is delivered via OnComplete on the game thread.
	 * @param Graph     Workflow graph description to send to the server.
	 * @param bEnableDiagnostics  When true the server populates TurnComplete.debug_info.
	 * @param bMaintainDialogueHistory  When true (default) the agent keeps its full
	 *        dialogue history; when false the history is discarded after each turn
	 *        (stateless agent — for classification/routing/tool-picking agents).
	 * @param Voice     The agent's voice (protocol v27). Required when any node speaks
	 *        (FTryllGraphDescription::UsesTts): the server fails creation with
	 *        AgentVoiceRequired (3025) without one. Unset for a silent agent.
	 */
	void RequestCreateAgent(
		const FTryllGraphDescription&        Graph,
		TFunction<FTryllOnAgentCreated>      OnComplete,
		bool                                 bEnableDiagnostics = false,
		bool                                 bMaintainDialogueHistory = true,
		const TArray<FTryllVariableDecl>&    Variables = {},
		ETryllKvCacheInitialization          KvCacheInitialization = ETryllKvCacheInitialization::AllocateOnly,
		ETryllWorkload                       Workload = ETryllWorkload::Interactive,
		const TOptional<FTryllVoiceProfileData>& Voice = {});

	/** Queue destroy for the given agent. */
	void RequestDestroyAgent(TSharedPtr<FTryllAgent> Agent);

	[[nodiscard]] TSharedPtr<FTryllAgent> FindAgent(uint64 AgentId) const;
	TArray<FTryllRegisteredAgentSnapshot> SnapshotRegisteredAgents() const;


	// ── Model management ──────────────────────────────────────────────────────

	/** List models available for the session's inference engine. */
	void RequestListModels(TFunction<FTryllOnListModels> OnComplete);

	/**
	 * Pull a server-wide RAM/VRAM snapshot. Session-scoped (rejected with
	 * SessionNotReady before CreateSession) but the numbers cover every
	 * session on this process. Never returns AgentBusy.
	 * Not a UFUNCTION — Blueprint binds OnGetMemoryConsumptionComplete after
	 * a C++ call.
	 */
	void RequestGetMemoryConsumption(TFunction<FTryllOnGetMemoryConsumption> OnComplete);

	/** Experimental: get-or-create a named session mutable knowledge base. */
	void RequestEnsureMutableKb(
		const FString& Name,
		const FString& Schema,
		TFunction<FTryllOnEnsureMutableKb> OnComplete);

	/** Experimental: list session mutable knowledge bases. */
	void RequestEnumerateMutableKbs(TFunction<FTryllOnEnumerateMutableKbs> OnComplete);

	/** Experimental: write every store in this session to a TMKB path. */
	void RequestSaveGameMemory(
		const FString& Path,
		const FString& SaveId,
		TFunction<FTryllOnSaveGameMemory> OnComplete);

	/** Experimental: load a TMKB session snapshot. EmbedTiming must be Now or Lazy. */
	void RequestLoadGameMemory(
		const FString& Path,
		ETryllMutableKbEmbedTiming EmbedTiming,
		ETryllMutableKbCompatibility Compatibility,
		TFunction<FTryllOnLoadGameMemory> OnComplete);

	/**
	 * Start downloading a model. Progress is broadcast via OnDownloadProgress
	 * (name, bytesDownloaded, totalBytes, percent 0–100); completion via
	 * OnDownloadComplete (name, bSuccess, errorMessage — empty on success).
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Models")
	void RequestDownloadModel(const FString& ModelName);

	/**
	 * Pin a model into memory on the server (LoadAndPin).
	 * Completion is broadcast via OnLoadModelComplete.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Models")
	/// `Placement` applies to this load only and overrides any per-model hint the
	/// session declared; `bIsOverride = false` (the default) means "no opinion".
	void RequestLoadModel(
		const FString& ModelName,
		ETryllMemoryPlacement Placement = ETryllMemoryPlacement::Auto,
		bool bIsOverride = false);

	/**
	 * Demote a pinned model to on-demand and evict it if no contexts hold it.
	 * Completion is broadcast via OnUnloadModelComplete.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Models")
	void RequestUnloadModel(const FString& ModelName);

	// ── StringStorage management ──────────────────────────────────────────────

	using FTryllOnStringStorage = void(FTryllError);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllCreateStringStorage, const FString&, Name, bool, bSuccess);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTryllDestroyStringStorage, const FString&, Name, bool, bSuccess);

	/**
	 * Create a named StringStorage on the server from an inline string array.
	 * Completion is broadcast via OnCreateStringStorageComplete.
	 */
	void RequestCreateStringStorage(
		const FString&         Name,
		const TArray<FString>& Strings,
		TFunction<FTryllOnStringStorage> OnComplete = nullptr);

	/**
	 * Create a named StringStorage on the server from a server-side file path.
	 * @param Kind  0=List (default), 1=Map, 2=Multimap.  When Kind is Map or Multimap
	 *              the file must be a JSON array of {id, text} objects.
	 * Completion is broadcast via OnCreateStringStorageComplete.
	 */
	void RequestCreateStringStorageFromFile(
		const FString& Name,
		const FString& FilePath,
		uint8          Kind       = 0,
		TFunction<FTryllOnStringStorage> OnComplete = nullptr);

	/**
	 * Create a named StringStorage on the server from inline key/value pairs.
	 * @param Kind  1=Map (default; keys must be unique), 2=Multimap (duplicate keys allowed).
	 * Completion is broadcast via OnCreateStringStorageComplete.
	 */
	void RequestCreateStringStorageKeyed(
		const FString&         Name,
		const TArray<FString>& Keys,
		const TArray<FString>& Values,
		uint8                  Kind      = 1,
		TFunction<FTryllOnStringStorage> OnComplete = nullptr);

	/**
	 * Destroy a named StringStorage on the server.
	 * Nodes that already hold the storage are unaffected.
	 */
	void RequestDestroyStringStorage(
		const FString& Name,
		TFunction<FTryllOnStringStorage> OnComplete = nullptr);

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllCreateStringStorage OnCreateStringStorageComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllDestroyStringStorage OnDestroyStringStorageComplete;

	// ── EmbeddedStringStorage management ─────────────────────────────────────

	/** Result returned to callbacks on successful embedded storage creation. */
	struct FEmbeddedStorageInfo
	{
		FString      Name;
		uint32       RecordCount  = 0;
		uint32       EmbeddingDim = 0;
	};

	using FTryllOnEmbeddedStorageCreated = void(FEmbeddedStorageInfo, FTryllError);
	using FTryllOnEmbeddedStorageDestroyed = void(FTryllError);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
		FOnTryllCreateEmbeddedStringStorage,
		const FString&, Name,
		int32, RecordCount,
		bool, bSuccess);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
		FOnTryllDestroyEmbeddedStringStorage,
		const FString&, Name,
		bool, bSuccess);

	/**
	 * Create an EmbeddedStringStorage from a server-side *.fig.json.
	 * Path A: config file + optional embeddingModel override.
	 */
	void RequestCreateEmbeddedStringStorage(
		const FString& Name,
		const FString& ConfigPath,
		const FString& EmbeddingModel = FString{},
		TFunction<FTryllOnEmbeddedStorageCreated> OnComplete = nullptr);

	/**
	 * Create an EmbeddedStringStorage from inline strings.
	 * Path B: requires EmbeddingModel.
	 */
	void RequestCreateEmbeddedStringStorageFromStrings(
		const FString&         Name,
		const TArray<FString>& Strings,
		const FString&         EmbeddingModel,
		TFunction<FTryllOnEmbeddedStorageCreated> OnComplete = nullptr);

	/**
	 * Destroy a named EmbeddedStringStorage on the server.
	 * Nodes that already hold the storage are unaffected.
	 */
	void RequestDestroyEmbeddedStringStorage(
		const FString& Name,
		TFunction<FTryllOnEmbeddedStorageDestroyed> OnComplete = nullptr);

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllCreateEmbeddedStringStorage OnCreateEmbeddedStringStorageComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllDestroyEmbeddedStringStorage OnDestroyEmbeddedStringStorageComplete;

	// ── VoiceInput (STT) lifecycle ────────────────────────────────────────────

	/**
	 * Create a server-side VoiceInput (STT) handle.
	 * Completion is delivered via OnComplete on the game thread with a
	 * TSharedPtr<FTryllVoiceInput> on success.
	 */
	void RequestCreateVoiceInput(
		const FTryllVoiceInputConfig&        Config,
		TFunction<FTryllOnVoiceInputCreated> OnComplete);

	/** Queue destroy for the given VoiceInput handle. */
	void RequestDestroyVoiceInput(TSharedPtr<FTryllVoiceInput> Voice);

	// ── Internal registration (called by FTryllAgent helpers) ────────────────

	void RegisterAgent(std::uint64_t AgentId, TWeakPtr<FTryllAgent> Agent);
	void UnregisterAgent(std::uint64_t AgentId);
	void RegisterVoiceInput(std::uint64_t VoiceInputId, TWeakPtr<FTryllVoiceInput> Voice);

	// ── Delegates ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllConnectionChanged OnConnectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllError OnError;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllAgentDestroyed OnAgentDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllCreateSession OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllListModels OnListModelsComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllGetMemoryConsumption OnGetMemoryConsumptionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllEnsureMutableKb OnEnsureMutableKbComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllEnumerateMutableKbs OnEnumerateMutableKbsComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllSaveGameMemory OnSaveGameMemoryComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllLoadGameMemory OnLoadGameMemoryComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllDownloadProgress OnDownloadProgress;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllDownloadComplete OnDownloadComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllLoadModel OnLoadModelComplete;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllUnloadModel OnUnloadModelComplete;

	/**
	 * Fired when SetSessionLocale completes. Locale is the requested tag (empty
	 * when clearing); bSuccess is false only for a malformed tag, in which case
	 * the previous locale is untouched.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllSetSessionLocale OnSetSessionLocaleComplete;

	/**
	 * A language change could not update a live agent's Localized prompts. Fires once
	 * per agent and language change. If a Text ID is empty or missing in the new
	 * language, nothing was sent, so the agent keeps every prompt in the previous
	 * language. If the server rejected an update, the error is the first rejection, and
	 * nodes the server accepted already run the new language. Voice and {{language}} follow the
	 * new language either way. Fix the table, or recreate the agent.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllLocalizedTextFailed OnLocalizedTextFailed;

	/**
	 * A language change reached a live agent: the server acknowledged every Localized
	 * prompt that changed, or none needed to change. Wait for this before work that must
	 * see the new prompts, such as a KV prefill. An AgentBusy reply delays it until the
	 * running turn ends.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllLocalizedTextApplied OnLocalizedTextApplied;

	/** Broadcast when a ToolCallNode emits a tool_call NodeEvent (notifying dispositions). */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllToolCall OnToolCall;

	/** Call-ID-aware tool-call event used to correlate Resume tool results. */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllToolCallWithId OnToolCallWithId;

	/** Detailed tool-call event carrying the full owning FTryllToolCall (node + disposition). */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllToolCallDetailed OnToolCallDetailed;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllIntentClassified OnIntentClassified;

	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllIntentLlmClassified OnIntentLlmClassified;

	/** Broadcast when the executor pauses a turn between nodes (Pause node, or pausing ToolCall dispositions). */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllPaused OnPaused;

	/**
	 * Broadcast for any NodeEvent whose event_type has no typed delegate
	 * (or whose typed delegate has no subscribers). Untyped fallback.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tryll|Events")
	FOnTryllNodeEvent OnNodeEvent;

	// ── Bundled-server resolution ─────────────────────────────────────────────

	/** <PluginBaseDir>/Binaries/ThirdParty/TryllServer; empty if the plugin is not found. */
	static FString GetServerRootDir();

	/** Variant subfolders (Default/Release) whose target resolves on disk. */
	static TArray<EServerBuildVariant> GetAvailableServerVariants();

	/** Server exe to launch for Preferred; see .cpp for the editor/packaged resolution. */
	static FString ResolveServerExePath(EServerBuildVariant Preferred);

	/**
	 * Restart the auto-launched packaged server after the custom catalog path
	 * changes. No-op in the editor (the shared launcher owns that process) and
	 * when no server is running.
	 */
	void RelaunchServerForUserCatalog();

	friend class FTryllConnectionEpochSpec;

	FTryllConnection* GetLiveConnection() const;
	TSharedPtr<FTryllConnection> PinLiveConnection() const;

private:
	void ProcessEvents();
	void EnsureConnectionBound();
	void UnbindConnection();
	bool HandleConnectionState(const FTryllConnEvent& Event);
	void HandleSessionNodeEvent(uint64 AgentId, const FTryllDecodedNodeEvent& Event);
	void ForwardConnectionChanged(bool bConnected);
	void ForwardError(const FString& ErrorMessage);
	void ForwardAgentDestroyed(uint64 AgentId);
	void ForwardCreateSession(bool bSuccess);
	void ForwardSetSessionLocale(const FString& Locale, bool bSuccess);
	void ForwardListModels(const TArray<FTryllModelInfo>& Models, bool bSuccess);
	void ForwardDownloadProgress(const FString& ModelName, uint64 BytesDownloaded, uint64 TotalBytes, float Percent);
	void ForwardDownloadComplete(const FString& ModelName, bool bSuccess, const FString& ErrorMessage);
	void ForwardLoadModel(const FString& ModelName, bool bSuccess);
	void ForwardUnloadModel(const FString& ModelName, bool bSuccess);
	void ForwardCreateStringStorage(const FString& Name, bool bSuccess);
	void ForwardDestroyStringStorage(const FString& Name, bool bSuccess);
	void ForwardCreateEmbedded(const FString& Name, int32 RecordCount, bool bSuccess);
	void ForwardDestroyEmbedded(const FString& Name, bool bSuccess);
	void DrainPendingRequests(const FTryllError& TerminalError);
	void InvalidateConnectionEpoch(bool bNotifyDisconnected);

	void LaunchServerProcess();
	void TerminateServerProcess();
	/** Release the OS handle without killing the process (normal teardown). */
	void ReleaseServerHandle();
	/**
	 * Record + verify the server identity advertised in ConnectionReady. When
	 * bAutoLaunchServer is enabled this subsystem owns the server's lifecycle,
	 * so a mismatch is fail-closed: logs an error and disconnects. Otherwise
	 * (an explicit external connection) a mismatch is advisory-only.
	 * Returns false if the mismatch caused a fail-closed disconnect — the
	 * caller must not proceed with CreateSession in that case.
	 */
	bool CaptureAndVerifyServerIdentity(const struct FTryllConnEvent& Event);

	/**
	 * True when a shared UTryllEditorSubsystem exists to route through instead of
	 * this instance's own Connection (editor process, GEditor available — PIE and
	 * editor tool windows). False for packaged/player builds and for commandlets
	 * running with GEditor == nullptr, both of which keep the full local
	 * implementation below as their only implementation. Resolved fresh on every
	 * call, never cached, matching LaunchServerProcess()'s existing discipline.
	 */
	bool IsDelegating() const;

	/// True after this instance has called the shared subsystem's AcquireConnection()
	/// without a matching ReleaseConnection() yet. Only meaningful while IsDelegating().
	bool bAcquiredSharedConnection = false;

	/// Id from ITryllEditorBridge::RegisterPauseListenerProbe while delegating. 0 = none.
	int32 PauseListenerProbeId = 0;
	/// Id from ITryllEditorBridge::RegisterToolCallListenerProbe while delegating. 0 = none.
	int32 ToolCallListenerProbeId = 0;

	/// Any of OnToolCall / OnToolCallWithId / OnToolCallDetailed is bound. Such a
	/// listener can answer a paused tool-call batch with ResumeWithToolResult(s).
	bool HasToolCallListener() const;

	int32 SessionListenerId = 0;
	TWeakPtr<FTryllConnection> BoundConnection;

	UFUNCTION()
	void OnConnectRetryStateChanged(bool bConnected);

	int32                       RemainingConnectAttempts = 0;
	FTSTicker::FDelegateHandle  ConnectRetryTickerHandle;

	TSharedPtr<FTryllConnection> Connection;

	/// True between a successful CreateSession and disconnect / reconfigure-failure.
	bool bSessionConfigured = false;

	/// True between sending a CreateSessionRequest and its terminal response.
	/// Guards the one-shot CreateSession (protocol v4) against duplicate sends
	/// (e.g. auto-create-on-connect plus a listener calling CreateSession).
	bool bCreateSessionInFlight = false;

	/// Set before PIE FailOwnedBy. Completions then fire this subsystem's
	/// delegates and leave the shared connection events alone.
	bool bTearingDown = false;

	// Packaged games and standalone games (no GEditor); editor/PIE routes through
	// the shared UTryllEditorSubsystem instead (see LaunchServerProcess()).
	FTryllManagedServerProcess Server;

	/** Resolved param copies of CreateAgent requests in flight. Released on the reply. */
	UPROPERTY()
	TArray<TObjectPtr<UTryllNodeParamsBase>> LocalizedParamKeepAlive;

	FTryllLocalizedAgentSet LocalizedAgents;
	FDelegateHandle TextLocaleChangedHandle;

	void HandleTextLocaleChanged();
	FTryllLocalizedAgentSet::FSend MakeLocalizedSend();
	FTryllLocalizedAgentSet::FFail MakeLocalizedFail();
	FTryllLocalizedAgentSet::FApplied MakeLocalizedApplied();
	void BindLocalizedTurn(const TSharedPtr<FTryllAgent>& Agent, uint64 AgentId);
	void ReleaseLocalizedOwned(const TArray<UTryllNodeParamsBase*>& Owned);


	TArray<TWeakPtr<FTryllMutableKb>> MutableKbs;

	// Live agents indexed by agent_id.
	TMap<std::uint64_t, TWeakPtr<FTryllAgent>> AgentsById;

	// Live VoiceInput handles indexed by voice_input_id.
	TMap<std::uint64_t, TWeakPtr<FTryllVoiceInput>> VoiceInputsById;
};
