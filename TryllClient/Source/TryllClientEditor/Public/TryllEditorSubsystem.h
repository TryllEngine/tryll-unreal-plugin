// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "TickableEditorObject.h"
#include "TryllRuntimeSettings.h"
#include "TryllManagedServerProcess.h"
#include "TryllVoiceProfile.h"
#include "TryllGraphDescription.h"
#include "TryllModelManifest.h"   // ETryllMemoryPlacement
#include "TryllEditorBridge.h"
#include "TryllEditorStatus.h"
#include "TryllSubsystem.h"
#include "TryllEditorSubsystem.generated.h"

class FTryllConnection;
class FTryllConnectionEvents;
class FTryllAgent;
class FTryllVoiceInput;
class FTryllClientObserverHub;
class FTryllDiagnosticsDemand;
struct FTryllConnEvent;

/**
 * One shared tryll_server.exe *and* one shared connection/session/agent-map for
 * the whole editor session: PIE (via UTryllSubsystem), the chat panel, and the
 * model manager panel all route through this instead of each owning independent
 * state. Mirrors Unity's TryllClient.Instance singleton. Deliberately a
 * UEditorSubsystem rather than a UGameInstanceSubsystem (recreated every PIE
 * session) or a plain static (which Live Coding's Object Reinstancing can't
 * safely resize).
 *
 * A TCP connection can have at most one session, ever (server-enforced) — so
 * "shared connection" and "shared session" are the same decision here.
 */
UCLASS()
class TRYLLCLIENTEDITOR_API UTryllEditorSubsystem : public UEditorSubsystem, public FTickableEditorObject, public ITryllEditorBridge
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── FTickableEditorObject ─────────────────────────────────────────────────

	virtual void    Tick(float DeltaTime) override;
	virtual bool    IsTickable() const override;
	virtual TStatId GetStatId() const override;

	// ── Server process management (pre-existing) ─────────────────────────────

	bool IsRunning();

	/** Port of the last successfully launched-or-reused server (0 if none). */
	virtual int32 GetActualPort() const override { return ActualPort; }

	/**
	 * Reuses a live handle, reuses a warm server found on KnownPort (caller
	 * verifies identity once ConnectionReady arrives), or spawns a new one.
	 * Fails with FTryllEditorUiContract::ServerNeedsLicence while the licence
	 * gate blocks (see FTryllManagedServerProcess::ShouldBlockLaunch), and then
	 * opens the Tryll Welcome tab once.
	 */
	virtual bool TryLaunch(EServerBuildVariant Variant, int32 KnownPort, int32 IdleShutdownTimeoutSeconds, FString& OutError) override;

	/** Drop an open shared connection and connect to the server TryLaunch just started. */
	virtual void AdoptRelaunchedServer() override;

	/** Release the OS handle without killing the process (managed self-exit). */
	void ReleaseHandle();

	// ── Connection lifetime (refcounted) ─────────────────────────────────────

	/** Ensures the server process and connection are up; increments the refcount. */
	virtual void AcquireConnection() override;

	/** Decrements the refcount; disconnects only once every consumer has released. */
	virtual void ReleaseConnection() override;

	// ── Connection management ─────────────────────────────────────────────────

	void    Connect();
	void    Disconnect();
	virtual bool  IsConnected() const override;
	virtual int64 GetSessionId() const override;

	/**
	 * True while ConnectMaxAttempts still has budget after the latest ConnectFailed
	 * (a retry is scheduled or about to be). False on success, after the budget is
	 * exhausted, or when Connect was never started (e.g. launch failure). Used by
	 * the PIE model guard so accept-late cold starts don't abort on the first miss.
	 */
	bool HasConnectRetriesRemaining() const { return RemainingConnectAttempts > 0; }

	// ── Session configuration ─────────────────────────────────────────────────

	void CreateSession(
		FString GameName = TEXT(""),
		FString Locale = TEXT(""),
		bool bUseProjectLocaleWhenEmpty = true);


	void SetSessionLocale(FString Locale);
	void SetInferenceThrottle(float Level);

	void CreateSessionFromSettings();

	virtual bool IsSessionReady() const override;

	/** Shared chrome phase. Launching is Connecting detail, not a branchable phase. */
	ETryllOwnerPhase GetOwnerPhase() const { return OwnerPhase; }
	const FString& GetOwnerDetail() const { return OwnerDetail; }

	FTryllOnOwnerPhaseChangedNative OnOwnerPhaseChangedNative;

	// ── Server identity (from ConnectionReady) ─────────────────────────────────

	FString ServerExePath;
	FString ServerBuildConfig;
	FString ServerMode;
	FString ServerVersion;
	FString ServerRunId;
	bool    bServerManaged = false;
	int32   ServerIdleTimeoutSeconds = 0;

	// ── Agent lifecycle ───────────────────────────────────────────────────────

	void RequestCreateAgent(
		const FTryllGraphDescription&        Graph,
		TFunction<FTryllOnAgentCreated>      OnComplete,
		bool                                 bEnableDiagnostics = false,
		bool                                 bMaintainDialogueHistory = true,
		const TArray<FTryllVariableDecl>&    Variables = {},
		ETryllKvCacheInitialization          KvCacheInitialization = ETryllKvCacheInitialization::AllocateOnly,
		ETryllWorkload                       Workload = ETryllWorkload::Interactive,
		const TOptional<FTryllVoiceProfileData>& Voice = {});

	void RequestDestroyAgent(TSharedPtr<FTryllAgent> Agent);

	[[nodiscard]] TSharedPtr<FTryllAgent> FindAgent(uint64 AgentId) const;
	TArray<FTryllRegisteredAgentSnapshot> SnapshotRegisteredAgents() const;

	/** Agent Log capture — nullptr when the shared connection is down. */
	FTryllClientObserverHub* GetConnectionObserverHub();
	const TSharedRef<FTryllDiagnosticsDemand>* GetConnectionDiagnosticsDemand() const;
	/** Shared-pointer handle. GetSharedConnection returns the raw pointer. */
	TSharedPtr<FTryllConnection> GetSharedConnectionHandle() const { return Connection; }
	virtual FTryllConnectionEvents* GetConnectionEvents() override;

	// ── Model management ──────────────────────────────────────────────────────

	void RequestListModels(TFunction<FTryllOnListModels> OnComplete);
	void RequestGetMemoryConsumption(TFunction<FTryllOnGetMemoryConsumption> OnComplete);
	void RequestEnsureMutableKb(
		const FString& Name,
		const FString& Schema,
		TFunction<FTryllOnEnsureMutableKb> OnComplete);
	void RequestEnumerateMutableKbs(TFunction<FTryllOnEnumerateMutableKbs> OnComplete);
	void RequestSaveGameMemory(
		const FString& Path,
		const FString& SaveId,
		TFunction<FTryllOnSaveGameMemory> OnComplete);
	void RequestLoadGameMemory(
		const FString& Path,
		ETryllMutableKbEmbedTiming EmbedTiming,
		ETryllMutableKbCompatibility Compatibility,
		TFunction<FTryllOnLoadGameMemory> OnComplete);
	void RequestDownloadModel(const FString& ModelName);
	void RequestLoadModel(
		const FString& ModelName,
		ETryllMemoryPlacement Placement = ETryllMemoryPlacement::Auto,
		bool bIsOverride = false);
	void RequestUnloadModel(const FString& ModelName);

	/** Delete a catalog model. Completion broadcasts OnDeleteModelCompleteNative. */
	void RequestDeleteModel(const FString& ModelName);

	// ── StringStorage management ──────────────────────────────────────────────

	void RequestCreateStringStorage(
		const FString&         Name,
		const TArray<FString>& Strings,
		TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete = nullptr);

	void RequestCreateStringStorageFromFile(
		const FString& Name,
		const FString& FilePath,
		uint8          Kind       = 0,
		TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete = nullptr);

	void RequestCreateStringStorageKeyed(
		const FString&         Name,
		const TArray<FString>& Keys,
		const TArray<FString>& Values,
		uint8                  Kind      = 1,
		TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete = nullptr);

	void RequestDestroyStringStorage(
		const FString& Name,
		TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete = nullptr);

	// ── EmbeddedStringStorage management ─────────────────────────────────────

	void RequestCreateEmbeddedStringStorage(
		const FString& Name,
		const FString& ConfigPath,
		const FString& EmbeddingModel = FString{},
		TFunction<UTryllSubsystem::FTryllOnEmbeddedStorageCreated> OnComplete = nullptr);

	void RequestCreateEmbeddedStringStorageFromStrings(
		const FString&         Name,
		const TArray<FString>& Strings,
		const FString&         EmbeddingModel,
		TFunction<UTryllSubsystem::FTryllOnEmbeddedStorageCreated> OnComplete = nullptr);

	void RequestDestroyEmbeddedStringStorage(
		const FString& Name,
		TFunction<UTryllSubsystem::FTryllOnEmbeddedStorageDestroyed> OnComplete = nullptr);

	// ── VoiceInput (STT) lifecycle ────────────────────────────────────────────

	void RequestCreateVoiceInput(
		const FTryllVoiceInputConfig&        Config,
		TFunction<FTryllOnVoiceInputCreated> OnComplete);

	void RequestDestroyVoiceInput(TSharedPtr<FTryllVoiceInput> Voice);

	// ── Internal registration (called by FTryllAgent helpers) ────────────────

	void RegisterAgent(std::uint64_t AgentId, TWeakPtr<FTryllAgent> Agent);
	void UnregisterAgent(std::uint64_t AgentId);
	void RegisterVoiceInput(std::uint64_t VoiceInputId, TWeakPtr<FTryllVoiceInput> Voice);

	// ── Native delegates ──────────────────────────────────────────────────────

	FTryllOnConnectionChangedNative                    OnConnectionChangedNative;
	FTryllOnErrorNative                                OnErrorNative;
	/** A language change could not update a live editor agent's Localized prompts (AgentId, asset, message). */
	TMulticastDelegate<void(uint64, const FString&, const FString&)> OnLocalizedTextFailedNative;
	FTryllOnAgentDestroyedNative                       OnAgentDestroyedNative;
	FTryllOnCreateSessionCompleteNative                OnCreateSessionCompleteNative;
	FTryllOnSetSessionLocaleCompleteNative             OnSetSessionLocaleCompleteNative;
	FTryllOnListModelsCompleteNative                   OnListModelsCompleteNative;
	FTryllOnGetMemoryConsumptionCompleteNative          OnGetMemoryConsumptionCompleteNative;
	FTryllOnDownloadProgressNative                     OnDownloadProgressNative;
	FTryllOnDownloadCompleteNative                     OnDownloadCompleteNative;
	FTryllOnLoadModelCompleteNative                    OnLoadModelCompleteNative;
	FTryllOnUnloadModelCompleteNative                  OnUnloadModelCompleteNative;
	FTryllOnDeleteModelCompleteNative                  OnDeleteModelCompleteNative;
	FTryllOnCreateStringStorageCompleteNative          OnCreateStringStorageCompleteNative;
	FTryllOnDestroyStringStorageCompleteNative         OnDestroyStringStorageCompleteNative;
	FTryllOnCreateEmbeddedStringStorageCompleteNative  OnCreateEmbeddedStringStorageCompleteNative;
	FTryllOnDestroyEmbeddedStringStorageCompleteNative OnDestroyEmbeddedStringStorageCompleteNative;
	FTryllOnToolCallNative                             OnToolCallNative;
	FTryllOnToolCallWithIdNative                       OnToolCallWithIdNative;
	FTryllOnToolCallDetailedNative                     OnToolCallDetailedNative;
	FTryllOnIntentClassifiedNative                     OnIntentClassifiedNative;
	FTryllOnIntentLlmClassifiedNative                  OnIntentLlmClassifiedNative;
	FTryllOnPausedNative                               OnPausedNative;
	FTryllOnPausedForwardNative                        OnPausedForwardNative;
	FTryllOnNodeEventNative                            OnNodeEventNative;

	virtual int32 RegisterPauseListenerProbe(TFunction<bool()> Probe) override;
	virtual void UnregisterPauseListenerProbe(int32 ProbeId) override;
	virtual bool HasSessionPauseListener() const override;
	virtual int32 RegisterToolCallListenerProbe(TFunction<bool()> Probe) override;
	virtual void UnregisterToolCallListenerProbe(int32 ProbeId) override;
	virtual bool HasSessionToolCallListener() const override;

	friend class FTryllConnectionEpochSpec;
	friend class FTryllAcquireDeadSocketSpec;
	friend class FTryllSubsystemDrainReentrancySpec;
	friend class FTryllEditorConnectionRecoverySpec;
	friend struct FTryllRecoveryIntegrationHarness;
	friend class FTryllModelManagerRetrySpec;
	friend class FTryllJoinLateReplaySpec;
	friend struct FTryllEditorWindowTestAccess;
	friend struct FTryllLicenceGateTestAccess;

private:
	enum class EPortProbeResult
	{
		/** Nothing answered — the port is free to bind. */
		Empty,
		/** A Tryll server answered and its exe_path matches ours (or predates identity fields). */
		ReusableServer,
		/** Something answered but is not our server — do not reuse it. */
		Rejected,
	};

	/**
	 * Connect briefly to Port, read the first frame, and verify it is a
	 * ConnectionReady whose exe_path canonically matches IntendedExe. A server
	 * that predates the identity fields (empty exe_path) is accepted, since it
	 * cannot be verified either way. This is what makes known-port reuse
	 * fail-closed: a stale/foreign process on the fixed editor port is rejected
	 * instead of silently adopted.
	 */
	static EPortProbeResult ProbeAndVerify(int32 Port, const FString& IntendedExe, FString& OutReason);

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
	bool CaptureAndVerifyServerIdentity(const FTryllConnEvent& Event);

	void OnConnectRetryStateChanged(bool bConnected);

	FTryllManagedServerProcess Server;
	int32 ActualPort = 0;
	FString LastLlamaCppBackend;
	FString LastSaveRoot;
	FString LastStorageRoot;
	FString LastProjectName;
	FString LastProjectDir;
	FString LastUserModelsCatalog;
	FString LastMode;
	/// Test-only extra CLI args forwarded to the next Spawn (fault injection).
	FString DebugExtraCliArgsForTests;
	/// Test-only launch isolation forwarded to the next Spawn (see FTryllManagedServerProcess).
	FString DebugStorageRootForTests;
	FString DebugSaveRootForTests;
	bool bDebugNoUserCatalogsForTests = false;

	/// Isolated tests increment the lease without launching a server.
	bool DebugInhibitLaunchForTests = false;

	// ── Licence gate ──────────────────────────────────────────────────────────

	/** TryLaunch, then Connect. AcquireConnection and an accepted licence share it. */
	void LaunchAndConnect();

	/** The live licence gate, or the test override of "non-interactive". */
	bool IsLicenceBlockingLaunch() const;

	/** Open (or focus) the Tryll Welcome tab on the next tick. Pending opens merge into one. */
	void RequestWelcomeAfterBlockedLaunch();

	/**
	 * FTryllLicence::OnChanged. Withdrawn (the gate now blocks): stop the server.
	 * Accepted while consumers wait on a blocked launch: launch and connect now.
	 */
	void HandleLicenceChanged();

	/** Drop the connection so consumers see it, and kill the server this editor spawned. */
	void StopServerForWithdrawnLicence();

	/// The latest TryLaunch was stopped by the licence gate.
	bool bLaunchBlockedByLicence = false;
	bool bWelcomeOpenPending = false;
	FDelegateHandle LicenceChangedHandle;

	/// Test-only: replaces IsNonInteractiveRun() in the gate.
	TOptional<bool> DebugNonInteractiveForTests;
	/// Test-only: called instead of opening the Welcome tab.
	TFunction<void()> DebugOpenWelcomeForTests;
	/// Test-only: replaces everything TryLaunch does after the licence gate.
	TFunction<bool(FString& OutError)> DebugLaunchAfterGateForTests;

	int32 DebugConnectionRefCountForTests() const { return ConnectionRefCount; }

	int32 ConnectionRefCount = 0;

	struct FPauseListenerProbe
	{
		int32 Id = 0;
		TFunction<bool()> IsBound;
	};
	TArray<FPauseListenerProbe> PauseListenerProbes;
	int32 NextPauseListenerProbeId = 1;
	TArray<FPauseListenerProbe> ToolCallListenerProbes;
	int32 NextToolCallListenerProbeId = 1;

	TSharedPtr<FTryllConnection> Connection;
	TWeakPtr<FTryllConnection> BoundConnection;
	int32 SessionListenerId = 0;
	bool bPumpingEvents = false;
	bool bReleaseConnectionAfterPump = false;
public:
	virtual FTryllConnection* GetSharedConnection() override { return Connection.Get(); }

private:

	FDelegateHandle ConnectRetryNativeHandle;
	int32                      RemainingConnectAttempts = 0;
	FTSTicker::FDelegateHandle ConnectRetryTickerHandle;

	/// True between a successful CreateSession and disconnect / reconfigure-failure.
	bool bSessionConfigured = false;

	/// True between sending a CreateSessionRequest and its terminal response.
	bool bCreateSessionInFlight = false;

	void RefreshOwnerPhase();
	void SetLastOwnerError(const FString& Error);

	ETryllOwnerPhase OwnerPhase = ETryllOwnerPhase::Idle;
	FString OwnerDetail;
	FString LastOwnerError;
	bool bLaunchingServer = false;

	/** Resolved param copies of CreateAgent requests in flight. Released on the reply. */
	UPROPERTY()
	TArray<TObjectPtr<UTryllNodeParamsBase>> LocalizedParamKeepAlive;

	FTryllLocalizedAgentSet LocalizedAgents;
	FDelegateHandle TextLocaleChangedHandle;

	void HandleTextLocaleChanged();
	FTryllLocalizedAgentSet::FSend MakeLocalizedSend();
	FTryllLocalizedAgentSet::FFail MakeLocalizedFail();
	void BindLocalizedTurn(const TSharedPtr<FTryllAgent>& Agent, uint64 AgentId);
	void ReleaseLocalizedOwned(const TArray<UTryllNodeParamsBase*>& Owned);

	TArray<TWeakPtr<FTryllMutableKb>> MutableKbs;

	// Live agents indexed by agent_id.
	TMap<std::uint64_t, TWeakPtr<FTryllAgent>> AgentsById;

	// Live VoiceInput handles indexed by voice_input_id.
	TMap<std::uint64_t, TWeakPtr<FTryllVoiceInput>> VoiceInputsById;
};
