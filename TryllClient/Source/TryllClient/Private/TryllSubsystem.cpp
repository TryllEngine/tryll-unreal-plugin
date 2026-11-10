// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllSubsystem.h"
#include "TryllAgent.h"
#include "TryllMutableKb.h"
#include "TryllConnection.h"
#include "TryllClientModule.h"
#include "TryllRuntimeSettings.h"
#include "TryllPathUtil.h"
#include "Generated/TryllNodeParamsFactory.h"
#include "ClientCodec.h"
#include "Generated/TryllCalls.gen.h"
#include "TryllSessionCalls.h"
#include "TryllWeakCompletion.h"
#include "TryllObservationTypes.h"

#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

#if WITH_EDITOR
#include "Editor.h"
#include "TryllEditorBridge.h"
#endif

using Tryll::Client::CanonicalizeExePath;

// ── UGameInstanceSubsystem ────────────────────────────────────────────────────

void UTryllSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TextLocaleChangedHandle = UTryllTextProvider::OnLocaleChanged().AddUObject(
		this, &UTryllSubsystem::HandleTextLocaleChanged);

	// Connect-retry budget tracker.
	OnConnectionChanged.AddDynamic(this, &UTryllSubsystem::OnConnectRetryStateChanged);

#if WITH_EDITOR
	if (IsDelegating())
	{
		if (ITryllEditorBridge* EditorBridge = FTryllEditorBridge::Get())
		{
			PauseListenerProbeId = EditorBridge->RegisterPauseListenerProbe([this]()
			{
				return OnPaused.IsBound();
			});
			ToolCallListenerProbeId = EditorBridge->RegisterToolCallListenerProbe([this]()
			{
				return HasToolCallListener();
			});

			if (GetDefault<UTryllRuntimeSettings>()->bAutoConnect)
			{
				UE_LOG(LogTryll, Log, TEXT("UTryllSubsystem(PIE): acquiring shared editor connection (bAutoConnect=true)"));
				EditorBridge->AcquireConnection();
				bAcquiredSharedConnection = true;
			}
			else
			{
				UE_LOG(LogTryll, Warning,
					TEXT("UTryllSubsystem(PIE): bAutoConnect=false — NOT holding the shared connection; if the PIE ")
					TEXT("model guard releases its ref the shared connection drops and CreateAgent fails with no session"));
			}

			EnsureConnectionBound();

			// Join-late: this instance may subscribe after the shared connection/
			// session was already established by another consumer (the PIE model
			// guard, a second PIE session, or a tool window opened first). Replay the
			// current state to this instance's own Blueprint/component listeners — but
			// DEFER it to the next tick. Scene actors (UTryllAgentComponent, demo
			// Blueprints) subscribe to OnConnectionChanged/OnCreateSessionComplete in
			// their BeginPlay, which runs AFTER this GameInstance Initialize.
			// Broadcasting synchronously here would fire those one-shot events before
			// anyone is listening, so event-driven agent creation would silently never
			// happen — the exact failure the PIE model guard exposes: the guard
			// pre-creates the session, so this "already up" path is taken, and the
			// agent is never created. Deferring one tick lets BeginPlay subscribe
			// first, matching the timing a cold (async) connect naturally has.
			if (EditorBridge->IsConnected() || EditorBridge->IsSessionReady())
			{
				UE_LOG(LogTryll, Log,
					TEXT("UTryllSubsystem(PIE): shared connection/session already up — deferring join-late replay to next tick"));
				FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float) -> bool
				{
					ITryllEditorBridge* Ed = FTryllEditorBridge::Get();
					if (Ed)
					{
						const bool bConn = Ed->IsConnected();
						const bool bSess = Ed->IsSessionReady();
						UE_LOG(LogTryll, Log,
							TEXT("UTryllSubsystem(PIE): replaying join-late state to late subscribers (connected=%d, sessionReady=%d)"),
							bConn ? 1 : 0, bSess ? 1 : 0);
						if (bConn) { OnConnectionChanged.Broadcast(true); }
						if (bSess) { OnCreateSessionComplete.Broadcast(true); }
					}
					return false; // one-shot
				}), 0.0f);
			}
		}
		return;
	}
#endif

	Connection = MakeShared<FTryllConnection>();
	EnsureConnectionBound();

	if (GetDefault<UTryllRuntimeSettings>()->bAutoLaunchServer)
	{
		LaunchServerProcess();
	}

	if (GetDefault<UTryllRuntimeSettings>()->bAutoConnect)
	{
		Connect();
	}
}

void UTryllSubsystem::Deinitialize()
{
#if WITH_EDITOR
	if (IsDelegating())
	{
		if (ITryllEditorBridge* EditorBridge = FTryllEditorBridge::Get())
		{
			bTearingDown = true;
			if (FTryllConnection* Shared = EditorBridge->GetSharedConnection())
			{
				Shared->FailOwnedBy(this, FTryllError(
					ETryllErrorCode::ConnectionDropped, TEXT("Play session ended")));
			}
			UnbindConnection();
			if (PauseListenerProbeId != 0)
			{
				EditorBridge->UnregisterPauseListenerProbe(PauseListenerProbeId);
				PauseListenerProbeId = 0;
			}
			if (ToolCallListenerProbeId != 0)
			{
				EditorBridge->UnregisterToolCallListenerProbe(ToolCallListenerProbeId);
				ToolCallListenerProbeId = 0;
			}

			if (bAcquiredSharedConnection)
			{
				UE_LOG(LogTryll, Log, TEXT("UTryllSubsystem(PIE): releasing shared editor connection (PIE ending)"));
				EditorBridge->ReleaseConnection();
				bAcquiredSharedConnection = false;
			}
		}
		if (TextLocaleChangedHandle.IsValid())
		{
			UTryllTextProvider::OnLocaleChanged().Remove(TextLocaleChangedHandle);
			TextLocaleChangedHandle.Reset();
		}
		LocalizedAgents.ForgetAll();
		LocalizedParamKeepAlive.Reset();
		Super::Deinitialize();
		return;
	}
#endif

	DrainPendingRequests(FTryllError(ETryllErrorCode::ConnectionDropped, TEXT("Connection dropped")));
	InvalidateConnectionEpoch(/*bNotifyDisconnected=*/false);

	if (ConnectRetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ConnectRetryTickerHandle);
		ConnectRetryTickerHandle.Reset();
	}
	RemainingConnectAttempts = 0;

	UnbindConnection();
	if (Connection.IsValid())
	{
		Connection->Stop();
		Connection.Reset();
	}

	// Disconnect only — do not kill the server process.  The server self-exits
	// via its idle-shutdown timer (managed mode).  Hard-killing was removed
	// because it terminates a server that other clients may still be using.
	// Release our handle without sending SIGTERM/TerminateProc.
	ReleaseServerHandle();

	if (TextLocaleChangedHandle.IsValid())
	{
		UTryllTextProvider::OnLocaleChanged().Remove(TextLocaleChangedHandle);
		TextLocaleChangedHandle.Reset();
	}
	LocalizedAgents.ForgetAll();
	LocalizedParamKeepAlive.Reset();

	Super::Deinitialize();
}

// ── Bundled-server resolution ─────────────────────────────────────────────────

namespace
{
	const TCHAR* TryllVariantDirName(EServerBuildVariant Variant)
	{
		return Variant == EServerBuildVariant::Release ? TEXT("Release") : TEXT("Default");
	}

	// Seed the agent's node-params baseline from the authored graph so
	// GetNodeParamsBaseline returns a usable clone-set-send starting point.
	// Clone each node's params: the baseline must not alias the graph's shared
	// objects, which a caller would corrupt by mutating in place.
	void SeedAgentParamsBaseline(
		const TSharedPtr<FTryllAgent>& Agent,
		const FTryllGraphDescription&  Graph,
		UObject*                       Outer)
	{
		if (!Agent.IsValid())
		{
			return;
		}
		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (Node.Params)
			{
				Agent->SetNodeParamsBaseline(
					Node.Name, UTryllNodeParamsFactory::CloneParams(Node.Params, Outer));
			}
		}
	}
}

FString UTryllSubsystem::GetServerRootDir()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TryllClient"));
	if (!Plugin.IsValid())
	{
		return FString();
	}
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(
		Plugin->GetBaseDir(), TEXT("Binaries"), TEXT("ThirdParty"), TEXT("TryllServer")));
}

TArray<EServerBuildVariant> UTryllSubsystem::GetAvailableServerVariants()
{
	TArray<EServerBuildVariant> Available;
	const FString Root = GetServerRootDir();
	if (Root.IsEmpty())
	{
		return Available;
	}
	// Require the exe itself, not just the variant directory. The Default (Debug)
	// junction always exists on disk (it holds a tracked .gitignore), so a
	// DirectoryExists check would report Debug as available even in a Release-only
	// build that never produced Debug/tryll_server.exe -- ResolveServerExePath
	// would then prefer Debug and log "server exe not found". FileExists follows
	// the symlink, so a dangling/exe-less variant correctly counts as absent.
	for (const EServerBuildVariant Variant : { EServerBuildVariant::Default, EServerBuildVariant::Release })
	{
		if (FPaths::FileExists(FPaths::Combine(Root, TryllVariantDirName(Variant), TEXT("tryll_server.exe"))))
		{
			Available.Add(Variant);
		}
	}
	return Available;
}

FString UTryllSubsystem::ResolveServerExePath(EServerBuildVariant Preferred)
{
#if WITH_EDITOR
	const FString Root = GetServerRootDir();
	if (Root.IsEmpty())
	{
		return FString();
	}
	const TArray<EServerBuildVariant> Available = GetAvailableServerVariants();
	if (Available.Num() == 0)
	{
		return FString();
	}
	const EServerBuildVariant Chosen =
		(Available.Num() == 1 || !Available.Contains(Preferred)) ? Available[0] : Preferred;
	return FPaths::Combine(Root, TryllVariantDirName(Chosen), TEXT("tryll_server.exe"));
#else
	// Packaged: Build.cs stages the server to a flat <Project>/Server/ (see TryllClient.Build.cs).
	(void)Preferred;
	return FPaths::Combine(FPaths::ProjectDir(), TEXT("Server"), TEXT("tryll_server.exe"));
#endif
}

// ── Delegation to the shared editor subsystem ───────────────────────────────────

bool UTryllSubsystem::IsDelegating() const
{
#if WITH_EDITOR
	// Only when the editor module has registered the shared bridge — never link
	// TryllClientEditor from this module (circular DLLs fail to load on Windows).
	return FTryllEditorBridge::Get() != nullptr;
#else
	return false;
#endif
}

// ── Server process lifecycle ──────────────────────────────────────────────────

void UTryllSubsystem::LaunchServerProcess()
{
#if WITH_EDITOR
	// See docs/research/clients/server-lifetime-ownership-and-port-selection.md §4.2.
	if (IsRunningCommandlet())
	{
		UE_LOG(LogTryll, Verbose, TEXT("Commandlet; skipping Tryll server auto-launch."));
		return;
	}
	// A standalone game runs the editor binary without GEditor; it launches its own
	// server below, like a packaged game.
	if (GEditor)
	{
		ITryllEditorBridge* EditorBridge = FTryllEditorBridge::Get();
		if (!EditorBridge)
		{
			UE_LOG(LogTryll, Error, TEXT("Tryll editor bridge unavailable — cannot auto-launch the Tryll server."));
			return;
		}

		const int32 KnownPort = GetDefault<UTryllRuntimeSettings>()->ServerPort;
		FString Error;
		if (!EditorBridge->TryLaunch(
			GetDefault<UTryllRuntimeSettings>()->ServerBuildVariant, KnownPort, /*IdleShutdownTimeoutSeconds=*/60, Error))
		{
			// The editor's licence gate already opened the Welcome window; an
			// unaccepted licence is not an installation failure.
			if (Error == FTryllManagedServerProcess::ServerNeedsLicenceMessage)
			{
				UE_LOG(LogTryll, Log, TEXT("Tryll server launch waits for the licence: %s"), *Error);
			}
			else
			{
				UE_LOG(LogTryll, Error, TEXT("Failed to launch/reuse the Tryll server: %s"), *Error);
			}
			return;
		}
		ServerPort = EditorBridge->GetActualPort();
		return;
	}
	// The editor binary is an editor install: a standalone game needs the licence too.
	// Its window cannot accept it, so only log. Packaged games are never gated.
	if (FTryllManagedServerProcess::IsLaunchBlockedByLicence())
	{
		UE_LOG(LogTryll, Error, TEXT("%s"), FTryllManagedServerProcess::ServerNeedsLicenceMessage);
		return;
	}
#endif
	const FString Abs = ResolveServerExePath(
		GetDefault<UTryllRuntimeSettings>()->ServerBuildVariant);
	if (Abs.IsEmpty())
	{
		UE_LOG(LogTryll, Error,
			TEXT("No bundled Tryll server variant found under '%s'"), *GetServerRootDir());
		return;
	}
	if (!FPaths::FileExists(Abs))
	{
		UE_LOG(LogTryll, Error, TEXT("Tryll server exe not found at '%s'"), *Abs);
		return;
	}

	// Packaged builds run in managed mode too: the server self-exits after the
	// idle grace once no client is connected. The port is always auto-allocated
	// here (concurrent game instances must not collide), so retry a few times
	// on a bind collision (Server.Spawn's bOutPortConflict) before giving up.
	constexpr int32 MaxAttempts = 4;
	FString LastReason = TEXT("unknown");

	for (int32 Attempt = 1; Attempt <= MaxAttempts; ++Attempt)
	{
		// Pick an ephemeral free port so concurrent game instances don't collide.
		// Fail closed on probe failure rather than silently falling back to the
		// shared default ServerPort (9100): that would reintroduce exactly the
		// cross-process collision this free-port policy exists to prevent, and
		// on precisely the machines (port exhaustion, odd network stacks) where
		// a collision is most likely.
		int32 ListenPort = 0;
		bool  bGotPort   = false;
		{
			FSocket* ProbeSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)
				->CreateSocket(NAME_Stream, TEXT("TryllPortProbe"), false);
			if (ProbeSocket)
			{
				TSharedRef<FInternetAddr> Addr =
					ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
				Addr->SetAnyAddress();
				Addr->SetPort(0);
				if (ProbeSocket->Bind(*Addr))
				{
					int32 BoundPort = 0;
					ProbeSocket->GetAddress(*Addr);
					BoundPort = Addr->GetPort();
					if (BoundPort > 0)
					{
						ListenPort = BoundPort;
						bGotPort   = true;
					}
				}
				ProbeSocket->Close();
				ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ProbeSocket);
			}
		}
		if (!bGotPort)
		{
			UE_LOG(LogTryll, Error, TEXT("Failed to allocate a free port for the Tryll server ")
				TEXT("(port exhaustion?); refusing to silently fall back to the shared default port."));
			return;
		}

		bool    bPortConflict = false;
		FString SpawnError;
		if (Server.Spawn(Abs, ListenPort, /*IdleShutdownTimeoutSeconds=*/60,
			GetDefault<UTryllRuntimeSettings>()->ServerReadinessTimeoutSeconds,
			GetDefault<UTryllRuntimeSettings>()->LlamaCppBackendCliValue(),
			SpawnError, bPortConflict))
		{
			ServerPort = ListenPort; // update member so Connect() uses it
			return;
		}

		LastReason = SpawnError;
		if (bPortConflict && Attempt < MaxAttempts)
		{
			UE_LOG(LogTryll, Warning,
				TEXT("Port %d was taken before the server could bind (attempt %d/%d); ")
				TEXT("retrying on a new free port."),
				ListenPort, Attempt, MaxAttempts);
			continue;
		}

		UE_LOG(LogTryll, Error, TEXT("%s"), *SpawnError);
		return;
	}

	UE_LOG(LogTryll, Error,
		TEXT("Gave up starting the Tryll server after %d attempts - %s"), MaxAttempts, *LastReason);
}

void UTryllSubsystem::RelaunchServerForUserCatalog()
{
	// In the editor the shared launcher owns the server and Server never runs.
	// SetUserModelsCatalogPath talks to that bridge directly.
	if (!GetDefault<UTryllRuntimeSettings>()->bAutoLaunchServer || !Server.IsRunning())
	{
		return;
	}

	const bool bWasConnected = Connection.IsValid() && Connection->IsConnected();
	UE_LOG(LogTryll, Log,
		TEXT("Restarting the Tryll server so the custom models catalog change takes effect."));
	Server.TerminateAndRelease();
	LaunchServerProcess();
	if (bWasConnected && Server.IsRunning() && GetDefault<UTryllRuntimeSettings>()->bAutoConnect)
	{
		Connect();
	}
}

void UTryllSubsystem::ReleaseServerHandle()
{
	Server.ReleaseHandle();
}

bool UTryllSubsystem::CaptureAndVerifyServerIdentity(const FTryllConnEvent& Event)
{
	ServerExePath            = Event.ServerExePath;
	ServerBuildConfig        = Event.ServerBuildConfig;
	ServerMode               = Event.ServerMode;
	ServerVersion            = Event.ServerVersion;
	ServerRunId              = Event.ServerRunId;
	bServerManaged           = Event.bServerManaged;
	ServerIdleTimeoutSeconds = static_cast<int32>(Event.ServerIdleTimeoutSeconds);

	if (ServerExePath.IsEmpty())
	{
		return true; // server predates the identity fields
	}

	UE_LOG(LogTryll, Log,
		TEXT("Server identity: exe='%s' config=%s mode=%s version=%s run_id=%s managed=%d idle_timeout=%ds"),
		*ServerExePath, *ServerBuildConfig, *ServerMode, *ServerVersion, *ServerRunId,
		bServerManaged ? 1 : 0, ServerIdleTimeoutSeconds);

	// exe_path check: did we connect to the build we intended to launch?
	// Relevant when auto-launch reused a server found on the known port.
	// The server reports a *canonical* path (symlinks resolved); our bundled
	// .../Server path is usually a symlink to the dev build, so canonicalize
	// both before comparing or the symlink trips a false alarm.
	const FString Intended = ResolveServerExePath(
		GetDefault<UTryllRuntimeSettings>()->ServerBuildVariant);
	if (Intended.IsEmpty())
	{
		return true; // nothing to compare against (e.g. no bundled server payload)
	}

	const FString A = CanonicalizeExePath(Intended);
	const FString B = CanonicalizeExePath(ServerExePath);
	if (A.Equals(B, ESearchCase::IgnoreCase))
	{
		return true;
	}

	// bAutoLaunchServer means this subsystem is responsible for the server's
	// lifecycle, so a mismatch here means a stranger raced us onto the port —
	// fail closed rather than create a session against it. This is a
	// defense-in-depth backstop: UTryllServerEditorSubsystem's known-port
	// reuse probe already verifies identity before deciding to reuse a warm
	// server, so this should only trip on a genuine race. An explicit
	// external connection (bAutoLaunchServer=false) remains advisory-only —
	// the caller chose that target on purpose.
	if (GetDefault<UTryllRuntimeSettings>()->bAutoLaunchServer)
	{
		UE_LOG(LogTryll, Error,
			TEXT("Connected server exe '%s' does not match the intended exe '%s' — a different ")
			TEXT("server instance is holding the port. Disconnecting (bAutoLaunchServer is enabled, ")
			TEXT("so this connection must be ours)."),
			*ServerExePath, *Intended);
		Disconnect();
		return false;
	}

	UE_LOG(LogTryll, Warning,
		TEXT("Connected server exe '%s' does not match the intended exe '%s' — "
		     "reusing a different server instance."),
		*ServerExePath, *Intended);
	return true;
}

void UTryllSubsystem::TerminateServerProcess()
{
	Server.TerminateAndRelease();
}

// ── FTickableGameObject ───────────────────────────────────────────────────────

ETickableTickType UTryllSubsystem::GetTickableTickType() const
{
	return IsTickable() ? ETickableTickType::Always : ETickableTickType::Never;
}

bool UTryllSubsystem::IsTickable() const
{
	return Connection.IsValid();
}

TStatId UTryllSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTryllSubsystem, STATGROUP_Tickables);
}

void UTryllSubsystem::Tick(float DeltaTime)
{
	(void)DeltaTime;
	ProcessEvents();
}

// ── Connection management ─────────────────────────────────────────────────────

void UTryllSubsystem::DrainPendingRequests(const FTryllError& TerminalError)
{
	if (Connection.IsValid())
	{
		Connection->FailOwnedBy(this, TerminalError);
	}

	for (const TWeakPtr<FTryllMutableKb>& Weak : MutableKbs)
	{
		if (TSharedPtr<FTryllMutableKb> Store = Weak.Pin())
		{
			Store->DrainPendingCallbacks(TerminalError);
		}
	}
	MutableKbs.Reset();

	TArray<TWeakPtr<FTryllAgent>> AgentSnapshot;
	AgentsById.GenerateValueArray(AgentSnapshot);
	for (const TWeakPtr<FTryllAgent>& WeakAgent : AgentSnapshot)
	{
		if (TSharedPtr<FTryllAgent> Agent = WeakAgent.Pin())
		{
			Agent->DrainPendingLifecycleCallbacks(TerminalError);
			Agent->ClearPausedToolBatch();
		}
	}

	TArray<TWeakPtr<FTryllVoiceInput>> VoiceSnapshot;
	VoiceInputsById.GenerateValueArray(VoiceSnapshot);
	for (const TWeakPtr<FTryllVoiceInput>& WeakVoice : VoiceSnapshot)
	{
		if (TSharedPtr<FTryllVoiceInput> Voice = WeakVoice.Pin())
		{
			Voice->DrainPendingAcks(TerminalError);
		}
	}
}

void UTryllSubsystem::InvalidateConnectionEpoch(bool bNotifyDisconnected)
{
	for (const TPair<std::uint64_t, TWeakPtr<FTryllAgent>>& Pair : AgentsById)
	{
		if (TSharedPtr<FTryllAgent> Agent = Pair.Value.Pin())
		{
			Agent->ClearAfterDestroyRequest();
		}
	}
	for (const TPair<std::uint64_t, TWeakPtr<FTryllVoiceInput>>& Pair : VoiceInputsById)
	{
		if (TSharedPtr<FTryllVoiceInput> Voice = Pair.Value.Pin())
		{
			Voice->ClearAfterDestroyRequest();
		}
	}
	LocalizedAgents.ForgetAll();
	if (Connection.IsValid())
	{
		Connection->GetPushRouter().ClearObjects();
		Connection->ResetSessionPhase();
	}
	AgentsById.Empty();
	VoiceInputsById.Empty();
	if (bNotifyDisconnected)
	{
		OnConnectionChanged.Broadcast(false);
	}
}

void UTryllSubsystem::Connect()
{
#if WITH_EDITOR
	if (IsDelegating())
	{
		if (ITryllEditorBridge* EditorBridge = FTryllEditorBridge::Get())
		{
			if (!bAcquiredSharedConnection)
			{
				UE_LOG(LogTryll, Log, TEXT("UTryllSubsystem(PIE): Connect() — acquiring shared editor connection"));
				EditorBridge->AcquireConnection();
				bAcquiredSharedConnection = true;
			}
			EnsureConnectionBound();
		}
		return;
	}
#endif

	if (!Connection.IsValid())
	{
		Connection = MakeShared<FTryllConnection>();
	}
	EnsureConnectionBound();

	if (ConnectRetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ConnectRetryTickerHandle);
		ConnectRetryTickerHandle.Reset();
	}
	RemainingConnectAttempts = FMath::Max(1, GetDefault<UTryllRuntimeSettings>()->ConnectMaxAttempts);

	// Packaged Connect() used to leave bSessionConfigured set across a
	// Stop()+Start() socket replacement until Connected arrived.
	const bool bHadEpoch = (Connection.IsValid() && Connection->IsConnected())
		|| bSessionConfigured
		|| AgentsById.Num() > 0
		|| VoiceInputsById.Num() > 0;
	bSessionConfigured     = false;
	bCreateSessionInFlight = false;
	DrainPendingRequests(FTryllError(ETryllErrorCode::ConnectionDropped, TEXT("Connection replaced")));
	InvalidateConnectionEpoch(bHadEpoch);

	Connection->Start(ServerHost, ServerPort);
}

void UTryllSubsystem::Disconnect()
{
#if WITH_EDITOR
	if (IsDelegating())
	{
		// Mirrors the Unity fix: never force-disconnect the shared connection out
		// from under other consumers. Release our own acquisition; the shared
		// subsystem only actually disconnects once every consumer has released.
		if (ITryllEditorBridge* EditorBridge = FTryllEditorBridge::Get())
		{
			if (bAcquiredSharedConnection)
			{
				UE_LOG(LogTryll, Log, TEXT("UTryllSubsystem(PIE): Disconnect() — releasing shared editor connection"));
				EditorBridge->ReleaseConnection();
				bAcquiredSharedConnection = false;
			}
		}
		return;
	}
#endif

	DrainPendingRequests(FTryllError(ETryllErrorCode::ConnectionDropped, TEXT("Connection dropped")));
	InvalidateConnectionEpoch(/*bNotifyDisconnected=*/false);

	RemainingConnectAttempts = 0;
	if (ConnectRetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ConnectRetryTickerHandle);
		ConnectRetryTickerHandle.Reset();
	}

	if (Connection.IsValid())
	{
		Connection->Stop();
	}
}

void UTryllSubsystem::OnConnectRetryStateChanged(bool bConnected)
{
	if (bConnected)
	{
		RemainingConnectAttempts = 0;
		// Cancel any retry scheduled by a prior failed attempt.  Without this, a
		// retry armed on the first (failed) connect keeps ticking after the
		// immediate retry succeeds and, ~ConnectRetryDelaySeconds later, calls
		// Connection->Start() on the live connection -- tearing down the socket
		// mid-request (observed as CreateAgent failing with 1002 "Connection
		// closed" when the first CreateAgent races a slow cold model load).
		if (ConnectRetryTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(ConnectRetryTickerHandle);
			ConnectRetryTickerHandle.Reset();
		}
		return;
	}

	// One attempt has just been consumed (the one whose result triggered this callback).
	--RemainingConnectAttempts;
	if (RemainingConnectAttempts <= 0 || !Connection.IsValid())
	{
		return;
	}

	const float Delay = FMath::Max(0.0f, GetDefault<UTryllRuntimeSettings>()->ConnectRetryDelaySeconds);
	UE_LOG(LogTryll, Log, TEXT("Connect failed; retrying in %.1fs (%d attempts left)"),
		Delay, RemainingConnectAttempts);

	ConnectRetryTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float) -> bool
		{
			if (Connection.IsValid())
			{
				Connection->Start(ServerHost, ServerPort);
			}
			ConnectRetryTickerHandle.Reset();
			return false;
		}),
		Delay);
}

bool UTryllSubsystem::IsConnected() const
{
#if WITH_EDITOR
	if (IsDelegating())
	{
		ITryllEditorBridge* EditorBridge = FTryllEditorBridge::Get();
		return EditorBridge && EditorBridge->IsConnected();
	}
#endif
	return Connection.IsValid() && Connection->IsConnected();
}

int64 UTryllSubsystem::GetSessionId() const
{
#if WITH_EDITOR
	if (IsDelegating())
	{
		ITryllEditorBridge* EditorBridge = FTryllEditorBridge::Get();
		return EditorBridge ? EditorBridge->GetSessionId() : 0;
	}
#endif
	return Connection.IsValid() ? static_cast<int64>(Connection->GetSessionId()) : 0;
}

bool UTryllSubsystem::IsSessionReady() const
{
#if WITH_EDITOR
	if (IsDelegating())
	{
		ITryllEditorBridge* EditorBridge = FTryllEditorBridge::Get();
		return EditorBridge && EditorBridge->IsSessionReady();
	}
#endif
	return IsConnected() && bSessionConfigured;
}

// ── Session configuration ─────────────────────────────────────────────────────

void UTryllSubsystem::CreateSession(
	FString GameName,
	FString Locale,
	bool bUseProjectLocaleWhenEmpty)
{
	// A locale the caller chose is not overridden by later text-provider language changes.
	const bool bExplicitLocale = !Locale.IsEmpty();
	if (Locale.IsEmpty() && bUseProjectLocaleWhenEmpty)
		Locale = UTryllTextProvider::ResolveSessionLocale();

	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	if (!Live.IsValid())
	{
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	// CreateSession is one-shot (protocol v4). The guard lives on the connection
	// so the editor auto-create and a delegating game cannot both send.
	if (Live->IsSessionConfigured())
	{
		Live->GetEvents().OnCreateSessionComplete.Broadcast(true);
		return;
	}
	if (!Live->TryBeginCreateSession())
	{
		return;
	}
	Live->SetSessionLocaleExplicit(bExplicitLocale);

	bCreateSessionInFlight = true;

	::Tryll::SessionModelConfigT ModelConfig;
	for (const TPair<uint8, uint8>& Pair : UTryllRuntimeSettings::BuildEngineDeclarations())
	{
		auto Entry = std::make_unique<::Tryll::EngineSelectionT>();
		Entry->role = static_cast<::Tryll::ModelRole>(Pair.Key);
		Entry->engine = static_cast<::Tryll::InferenceEngine>(Pair.Value);
		ModelConfig.engines.push_back(std::move(Entry));
	}
	TMap<FString, ::Tryll::ModelPlacementHintT*> HintsByModel;
	auto HintFor = [&ModelConfig, &HintsByModel](const FString& Model) -> ::Tryll::ModelPlacementHintT*
	{
		if (::Tryll::ModelPlacementHintT** Found = HintsByModel.Find(Model))
		{
			return *Found;
		}
		auto Entry = std::make_unique<::Tryll::ModelPlacementHintT>();
		Entry->model_name = TCHAR_TO_UTF8(*Model);
		::Tryll::ModelPlacementHintT* Raw = Entry.get();
		ModelConfig.placement_hints.push_back(std::move(Entry));
		HintsByModel.Add(Model, Raw);
		return Raw;
	};
	for (const TPair<FString, uint8>& Hint : UTryllRuntimeSettings::BuildPlacementHints())
	{
		HintFor(Hint.Key)->placement = static_cast<::Tryll::MemoryPlacement>(Hint.Value);
	}
	for (const TPair<FString, uint8>& Hint : UTryllRuntimeSettings::BuildHotwordHints())
	{
		HintFor(Hint.Key)->hotwords = static_cast<::Tryll::HotwordBias>(Hint.Value);
	}

	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::Calls::CreateSession(
		*Live,
		this,
		GameName,
		Locale,
		ModelConfig,
		[WeakThis](TUniquePtr<::Tryll::CreateSessionResponseT> Resp, const FTryllError& Error)
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			const bool bAlready = Error.Code == static_cast<int32>(ETryllErrorCode::SessionAlreadyExists);
			if (Error.IsOk() || bAlready)
			{
				if (FTryllConnection* Conn = Self->GetLiveConnection())
				{
					if (Resp)
					{
						Conn->SetSessionId(Resp->session_id);
					}
					Conn->MarkSessionReady();
					Conn->GetEvents().OnCreateSessionComplete.Broadcast(true);
				}
				Self->bCreateSessionInFlight = false;
				return;
			}
			UE_LOG(LogTryll, Warning, TEXT("UTryllSubsystem: CreateSession error: %s"), *Error.Message);
			Self->bCreateSessionInFlight = false;
			if (Self->bTearingDown)
			{
				// The in-flight create set the shared phase to "configuring".
				// Drop that claim so the editor can retry. A session that is
				// already ready is left alone: abandon only moves phase 1 to 0.
				if (FTryllConnection* Conn = Self->GetLiveConnection())
				{
					Conn->AbandonCreateSession();
				}
				Self->OnCreateSessionComplete.Broadcast(false);
				Self->OnError.Broadcast(Error.Message);
			}
			else if (FTryllConnection* Conn = Self->GetLiveConnection())
			{
				Conn->AbandonCreateSession();
				Conn->GetEvents().OnCreateSessionComplete.Broadcast(false);
				Conn->GetEvents().OnError.Broadcast(Error.Message);
			}
		},
		[WeakThis](uint64 RequestId)
		{
			if (UTryllSubsystem* Self = WeakThis.Get(); Self && (Self->GetLiveConnection() != nullptr))
			{
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_CreateSessionRequest;
				Note.RequestBody = ::Tryll::MessageBody_CreateSessionRequest;
				Note.RequestId = RequestId;
				Self->GetLiveConnection()->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void UTryllSubsystem::SetSessionLocale(FString Locale)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::SetSessionLocale(
		Live.Get(), this, Locale,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown)
			{
				return nullptr;
			}
			return Self->GetLiveConnection() ? &Self->GetLiveConnection()->GetEvents() : nullptr;
		},
		[WeakThis](const FString& SentLocale, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnSetSessionLocaleComplete.Broadcast(SentLocale, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

void UTryllSubsystem::CreateSessionFromSettings()
{
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	// Empty locale + the project-locale path: the text provider's language wins,
	// then Settings->Locale. An explicit CreateSession locale still wins.
	CreateSession(
		Settings->GameName,
		TEXT(""),
		/*bUseProjectLocaleWhenEmpty=*/true);
}

void UTryllSubsystem::NotifyTextLocaleChanged()
{
	UTryllTextProvider::BroadcastLocaleChanged();
}

void UTryllSubsystem::HandleTextLocaleChanged()
{
	UTryllTextProvider* Provider = UTryllTextProvider::Get();
	if (Provider == nullptr)
	{
		return;
	}
#if WITH_EDITOR
	if (IsDelegating())
	{
		// PIE creates agents on the shared connection and tracks them here.
		// The editor session owns SetSessionLocale.
		LocalizedAgents.Refresh(Provider, MakeLocalizedSend(), MakeLocalizedFail(), MakeLocalizedApplied());
		return;
	}
#endif
	if (IsSessionReady())
	{
		const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
		FString Locale;
		if (Live.IsValid() && UTryllTextProvider::LocaleForLanguageChange(Live->IsSessionLocaleExplicit(), Locale))
		{
			SetSessionLocale(Locale);
		}
	}
	LocalizedAgents.Refresh(Provider, MakeLocalizedSend(), MakeLocalizedFail(), MakeLocalizedApplied());
}

FTryllLocalizedAgentSet::FSend UTryllSubsystem::MakeLocalizedSend()
{
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	return [WeakThis](const TSharedPtr<FTryllAgent>& Agent, const FString& NodeName,
		UTryllNodeParamsBase* Params, TFunction<void(const FTryllError&)> OnComplete)
	{
		if (WeakThis.IsValid() && Agent.IsValid())
		{
			Agent->ChangeLocalizedParams(NodeName, Params, MoveTemp(OnComplete));
		}
		else if (OnComplete)
		{
			OnComplete(FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent not ready")));
		}
	};
}

FTryllLocalizedAgentSet::FFail UTryllSubsystem::MakeLocalizedFail()
{
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	return [WeakThis](uint64 AgentId, const FString& AssetName, const FTryllError& Error)
	{
		if (UTryllSubsystem* Self = WeakThis.Get())
		{
			Self->OnLocalizedTextFailed.Broadcast(static_cast<int64>(AgentId), AssetName, Error.Message);
		}
	};
}

FTryllLocalizedAgentSet::FApplied UTryllSubsystem::MakeLocalizedApplied()
{
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	return [WeakThis](uint64 AgentId, const FString& AssetName)
	{
		if (UTryllSubsystem* Self = WeakThis.Get())
		{
			Self->OnLocalizedTextApplied.Broadcast(static_cast<int64>(AgentId), AssetName);
		}
	};
}

void UTryllSubsystem::BindLocalizedTurn(const TSharedPtr<FTryllAgent>& Agent, uint64 AgentId)
{
	if (!Agent.IsValid())
	{
		return;
	}
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Agent->SetLocalizedTurnEnded([WeakThis, AgentId]()
	{
		if (UTryllSubsystem* Self = WeakThis.Get())
		{
			Self->LocalizedAgents.NoteTurnEnded(
				AgentId, UTryllTextProvider::Get(), Self->MakeLocalizedSend(), Self->MakeLocalizedFail(), Self->MakeLocalizedApplied());
		}
	});
}

void UTryllSubsystem::ReleaseLocalizedOwned(const TArray<UTryllNodeParamsBase*>& Owned)
{
	for (UTryllNodeParamsBase* Params : Owned)
	{
		LocalizedParamKeepAlive.Remove(Params);
		if (Params)
		{
			Params->MarkAsGarbage();
		}
	}
}

// ── Agent lifecycle ───────────────────────────────────────────────────────────

void UTryllSubsystem::RequestCreateAgent(
	const FTryllGraphDescription&   Graph,
	TFunction<FTryllOnAgentCreated> OnComplete,
	bool                            bEnableDiagnostics,
	bool                            bMaintainDialogueHistory,
	const TArray<FTryllVariableDecl>& Variables,
	ETryllKvCacheInitialization     KvCacheInitialization,
	ETryllWorkload                  Workload,
	const TOptional<FTryllVoiceProfileData>& Voice)
{

	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	if (!Live.IsValid())
	{
		if (OnComplete)
		{
			OnComplete(nullptr, FTryllError(ETryllErrorCode::ConnectionFailed, TEXT("No connection")));
		}
		return;
	}

	bEnableDiagnostics = bEnableDiagnostics || Live->GetDiagnosticsDemand()->IsActive();

	// The language the prompts are resolved in. A switch before the reply is caught on Track.
	const FString LocaleAtResolve = UTryllTextProvider::CurrentProviderLocale();
	FTryllGraphDescription Resolved;
	TArray<UTryllNodeParamsBase*> Owned;
	FTryllError ResolveError;
	if (!FTryllTextResolve::PrepareForCreate(
			Graph, this, UTryllTextProvider::Get(), Resolved, Owned, ResolveError))
	{
		ReleaseLocalizedOwned(Owned);
		if (OnComplete)
		{
			OnComplete(nullptr, ResolveError);
		}
		return;
	}
	for (UTryllNodeParamsBase* Params : Owned)
	{
		LocalizedParamKeepAlive.Add(Params);
	}
	const bool bTrackLocalized = Owned.Num() > 0;

	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::Calls::CreateAgent(
		*Live,
		this,
		[Resolved, bEnableDiagnostics, bMaintainDialogueHistory, Variables, KvCacheInitialization, Workload, Voice](uint64 Id)
		{
			::Tryll::VoiceProfileT WireVoice;
			if (Voice.IsSet())
			{
				Voice->ToWire(WireVoice);
			}
			return Tryll::Client::BuildCreateAgentRequest(
				Id,
				Resolved,
				bEnableDiagnostics,
				bMaintainDialogueHistory,
				Variables,
				static_cast<::Tryll::AgentKvCacheInitialization>(KvCacheInitialization),
				static_cast<::Tryll::AgentWorkload>(Workload),
				Voice.IsSet() ? &WireVoice : nullptr);
		},
		[WeakThis, OnComplete = MoveTemp(OnComplete), Variables, Resolved, Owned, bEnableDiagnostics, bTrackLocalized, LocaleAtResolve, Voice](
			TUniquePtr<::Tryll::CreateAgentResponseT> Resp,
			const FTryllError& Error)
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Error.IsOk() || !Resp || !Self || !(Self->GetLiveConnection() != nullptr))
			{
				if (Self)
				{
					Self->ReleaseLocalizedOwned(Owned);
				}
				if (OnComplete)
				{
					OnComplete(
						nullptr,
						Error.IsOk()
							? FTryllError(ETryllErrorCode::ConnectionFailed, TEXT("CreateAgent failed"))
							: Error);
				}
				return;
			}
			const uint64 AgentId = Resp->agent_id;
			TSharedPtr<FTryllAgent> Agent = MakeShared<FTryllAgent>(Self->GetLiveConnection()->AsShared(), AgentId, Variables);
			Agent->SetCreatedWithDiagnostics(bEnableDiagnostics);
			Agent->SetCreatedVoice(Voice);
			Self->RegisterAgent(AgentId, Agent);
			Agent->SetLocalizedTextContext(Resolved.TextTable, Resolved.LocalizedAssetName);
			SeedAgentParamsBaseline(Agent, Resolved, Self);
			// The baseline holds its own clones; the resolved copies are done.
			Self->ReleaseLocalizedOwned(Owned);
			if (bTrackLocalized)
			{
				Self->LocalizedAgents.Track(Agent, Resolved.TextTable, Resolved.LocalizedAssetName);
				Self->BindLocalizedTurn(Agent, AgentId);
				if (UTryllTextProvider* Provider = UTryllTextProvider::Get();
					Provider && Provider->GetCurrentLocale() != LocaleAtResolve)
				{
					// The language changed while the agent was being created.
					Self->LocalizedAgents.RefreshAgent(
						AgentId, Provider, Self->MakeLocalizedSend(), Self->MakeLocalizedFail(), Self->MakeLocalizedApplied());
				}
			}
			if (OnComplete)
			{
				OnComplete(Agent, Error);
			}
		},
		{});
}

void UTryllSubsystem::SetInferenceThrottle(float Level)
{

	Tryll::Client::FTryllSessionCalls::SetInferenceThrottle(PinLiveConnection().Get(), Level);
}

void UTryllSubsystem::RequestDestroyAgent(TSharedPtr<FTryllAgent> Agent)
{

	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	if (!Live.IsValid() || !Agent.IsValid())
	{
		return;
	}
	const std::uint64_t Aid = Agent->GetAgentId();
	if (Aid == 0)
	{
		return;
	}
	Agent->DrainPendingLifecycleCallbacks(FTryllError(ETryllErrorCode::InvalidAgentId, TEXT("Agent destroyed")));
	Agent->ClearAfterDestroyRequest();
	LocalizedAgents.Forget(Aid);
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DestroyAgent(
		Live.Get(), this, Aid,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](uint64 Id)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->UnregisterAgent(Id);
			}
		},
		[WeakThis](int64 Id)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnAgentDestroyed.Broadcast(Id);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

// ── Model management ──────────────────────────────────────────────────────────

void UTryllSubsystem::RequestListModels(TFunction<FTryllOnListModels> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::ListModels(
		Live.Get(), this,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		MoveTemp(OnComplete),
		[WeakThis](const TArray<FTryllModelInfo>& Models, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnListModelsComplete.Broadcast(Models, bOk);
			}
		});
}

void UTryllSubsystem::RequestGetMemoryConsumption(TFunction<FTryllOnGetMemoryConsumption> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::GetMemoryConsumption(
		Live.Get(), this, MoveTemp(OnComplete),
		[WeakThis](const FTryllMemorySnapshot& Snap, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnGetMemoryConsumptionComplete.Broadcast(Snap, bOk);
			}
		});
}

void UTryllSubsystem::RequestEnsureMutableKb(
	const FString& Name,
	const FString& Schema,
	TFunction<FTryllOnEnsureMutableKb> OnComplete)
{

	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::EnsureMutableKb(
		Live.Get(), this, Name, Schema, MoveTemp(OnComplete),
		[WeakThis]() -> TSharedPtr<FTryllConnection>
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return Self->GetLiveConnection()->AsShared();
		},
		{},
		[WeakThis](const FString& StoreName, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnEnsureMutableKbComplete.Broadcast(StoreName, bOk);
			}
		});
}

void UTryllSubsystem::RequestEnumerateMutableKbs(TFunction<FTryllOnEnumerateMutableKbs> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::EnumerateMutableKbs(
		Live.Get(), this, MoveTemp(OnComplete),
		[WeakThis](int32 Count, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnEnumerateMutableKbsComplete.Broadcast(Count, bOk);
			}
		});
}

void UTryllSubsystem::RequestSaveGameMemory(
	const FString& Path,
	const FString& SaveId,
	TFunction<FTryllOnSaveGameMemory> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::SaveGameMemory(
		Live.Get(), this, Path, SaveId, MoveTemp(OnComplete),
		[WeakThis](int32 Count, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnSaveGameMemoryComplete.Broadcast(Count, bOk);
			}
		});
}

void UTryllSubsystem::RequestLoadGameMemory(
	const FString& Path,
	ETryllMutableKbEmbedTiming EmbedTiming,
	ETryllMutableKbCompatibility Compatibility,
	TFunction<FTryllOnLoadGameMemory> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::LoadGameMemory(
		Live.Get(), this, Path, EmbedTiming, Compatibility, MoveTemp(OnComplete),
		[WeakThis](int32 Count, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnLoadGameMemoryComplete.Broadcast(Count, bOk);
			}
		});
}

void UTryllSubsystem::RequestDownloadModel(const FString& ModelName)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DownloadModel(
		Live.Get(), this, ModelName,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](const FString& Name, uint64 Bytes, uint64 Total, float Percent)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnDownloadProgress.Broadcast(
					Name, static_cast<int64>(Bytes), static_cast<int64>(Total), Percent);
			}
		},
		[WeakThis](const FString& Name, bool bOk, const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnDownloadComplete.Broadcast(Name, bOk, Message);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

void UTryllSubsystem::RequestLoadModel(
	const FString& ModelName, ETryllMemoryPlacement Placement, bool bIsOverride)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::LoadModel(
		Live.Get(), this, ModelName, Placement, bIsOverride,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](const FString& Name, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnLoadModelComplete.Broadcast(Name, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

void UTryllSubsystem::RequestUnloadModel(const FString& ModelName)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::UnloadModel(
		Live.Get(), this, ModelName,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](const FString& Name, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnUnloadModelComplete.Broadcast(Name, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

// ── Internal registration ─────────────────────────────────────────────────────

void UTryllSubsystem::RegisterAgent(std::uint64_t AgentId, TWeakPtr<FTryllAgent> Agent)
{
	if (AgentId != 0)
	{
		AgentsById.Add(AgentId, Agent);
		if (FTryllConnection* Live = GetLiveConnection())
		{
			Live->GetPushRouter().RegisterAgent(AgentId, Agent);
		}
	}
}

void UTryllSubsystem::RegisterVoiceInput(std::uint64_t VoiceInputId, TWeakPtr<FTryllVoiceInput> Voice)
{
	if (VoiceInputId != 0)
	{
		VoiceInputsById.Add(VoiceInputId, Voice);
		if (FTryllConnection* Live = GetLiveConnection())
		{
			Live->GetPushRouter().RegisterVoiceInput(VoiceInputId, Voice);
		}
	}
}

void UTryllSubsystem::UnregisterAgent(std::uint64_t AgentId)
{
	AgentsById.Remove(AgentId);
	if (FTryllConnection* Live = GetLiveConnection())
	{
		Live->GetPushRouter().UnregisterAgent(AgentId);
	}
}

TSharedPtr<FTryllAgent> UTryllSubsystem::FindAgent(uint64 AgentId) const
{
	if (const TWeakPtr<FTryllAgent>* Found = AgentsById.Find(AgentId))
	{
		return Found->Pin();
	}
	return nullptr;
}

TArray<FTryllRegisteredAgentSnapshot> UTryllSubsystem::SnapshotRegisteredAgents() const
{
	TArray<FTryllRegisteredAgentSnapshot> Out;
	Out.Reserve(AgentsById.Num());
	for (const TPair<std::uint64_t, TWeakPtr<FTryllAgent>>& Pair : AgentsById)
	{
		const TSharedPtr<FTryllAgent> Agent = Pair.Value.Pin();
		if (!Agent.IsValid())
		{
			continue;
		}
		FTryllRegisteredAgentSnapshot Snapshot;
		Snapshot.AgentId                 = Pair.Key;
		Snapshot.LabelHint               = Agent->GetDisplayName();
		Snapshot.bCreatedWithDiagnostics = Agent->WasCreatedWithDiagnostics();
		Out.Add(MoveTemp(Snapshot));
	}
	return Out;
}

// ── Event dispatch (game thread) ──────────────────────────────────────────────

FTryllConnection* UTryllSubsystem::GetLiveConnection() const
{
#if WITH_EDITOR
	if (IsDelegating())
	{
		if (ITryllEditorBridge* Bridge = FTryllEditorBridge::Get())
		{
			return Bridge->GetSharedConnection();
		}
		return nullptr;
	}
#endif
	return Connection.Get();
}

TSharedPtr<FTryllConnection> UTryllSubsystem::PinLiveConnection() const
{
	if (FTryllConnection* Raw = GetLiveConnection())
	{
		return Raw->AsShared();
	}
	return nullptr;
}

void UTryllSubsystem::UnbindConnection()
{
	if (TSharedPtr<FTryllConnection> Old = BoundConnection.Pin())
	{
		FTryllConnectionEvents& Events = Old->GetEvents();
#define TRYLL_CONNECTION_EVENT(Member, DelegateType, ForwardMethod) Events.Member.RemoveAll(this);
#include "TryllConnectionEvents.inl"
		if (SessionListenerId != 0)
		{
			Old->GetPushRouter().RemoveSessionListener(SessionListenerId);
			SessionListenerId = 0;
		}
	}
	BoundConnection.Reset();
}

void UTryllSubsystem::EnsureConnectionBound()
{
	FTryllConnection* Live = GetLiveConnection();
	if (BoundConnection.Pin().Get() == Live)
	{
		return;
	}
	UnbindConnection();
	if (!Live)
	{
		return;
	}
	BoundConnection = Live->AsShared();
	FTryllConnectionEvents& Events = Live->GetEvents();
#define TRYLL_CONNECTION_EVENT(Member, DelegateType, ForwardMethod) \
	Events.Member.AddUObject(this, &UTryllSubsystem::ForwardMethod);
#include "TryllConnectionEvents.inl"
	SessionListenerId = Live->GetPushRouter().AddSessionListener(FTryllSessionPushListener{
		[this](uint64 AgentId, const FTryllDecodedNodeEvent& Event)
		{
			HandleSessionNodeEvent(AgentId, Event);
		},
		[this]()
		{
			return OnPaused.IsBound();
		},
		[this]()
		{
			return HasToolCallListener();
		}
	});
	if (!IsDelegating())
	{
		TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
		Live->SetStateHook([WeakThis](const FTryllConnEvent& Event) -> bool
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				return Self->HandleConnectionState(Event);
			}
			return true;
		});
	}
}

bool UTryllSubsystem::HandleConnectionState(const FTryllConnEvent& Event)
{
	switch (Event.Type)
	{
	case ETryllConnEventType::Connected:
		UE_LOG(LogTryll, Log, TEXT("UTryllSubsystem: connection ready (no session yet — call CreateSession)"));
		// Do not clear bCreateSessionInFlight here. IsConnected() is true
		// on the socket thread before this event is pumped, so a join-late
		// CreateSession can already be in flight on this connection.
		if (!CaptureAndVerifyServerIdentity(Event))
		{
			return false;
		}
		// Auto-create BEFORE the public broadcast so this subsystem's
		// CreateSession claims the one-shot first.
		if (GetDefault<UTryllRuntimeSettings>()->bAutoCreateSession)
		{
			CreateSessionFromSettings();
		}
		return true;

	case ETryllConnEventType::ConnectFailed:
		UE_LOG(LogTryll, Warning, TEXT("UTryllSubsystem: connect failed: %s"), *Event.Message);
		bSessionConfigured     = false;
		bCreateSessionInFlight = false;
		if (Connection.IsValid())
		{
			Connection->ResetSessionPhase();
		}
		DrainPendingRequests(FTryllError(ETryllErrorCode::ConnectionFailed, Event.Message));
		return true;

	case ETryllConnEventType::Disconnected:
		UE_LOG(LogTryll, Log, TEXT("UTryllSubsystem: disconnected"));
		bSessionConfigured     = false;
		bCreateSessionInFlight = false;
		DrainPendingRequests(FTryllError(ETryllErrorCode::ConnectionDropped,
			Event.Message.IsEmpty() ? FString(TEXT("Connection dropped")) : Event.Message));
		InvalidateConnectionEpoch(/*bNotifyDisconnected=*/false);
		return true;

	default:
		return true;
	}
}

void UTryllSubsystem::HandleSessionNodeEvent(uint64 AgentId, const FTryllDecodedNodeEvent& Decoded)
{
	bool bDispatchedTyped = false;
	const int64 Id = static_cast<int64>(AgentId);
	switch (Decoded.Kind)
	{
	case ETryllNodeEventKind::ToolCall:
		if (OnToolCallDetailed.IsBound())
		{
			OnToolCallDetailed.Broadcast(Id, Decoded.ToolCall);
		}
		if (OnToolCallWithId.IsBound())
		{
			OnToolCallWithId.Broadcast(
				Id,
				Decoded.ToolCall.CallId,
				Decoded.ToolCall.ToolName,
				Decoded.ToolCall.ArgumentsJson);
			bDispatchedTyped = true;
		}
		if (OnToolCall.IsBound())
		{
			OnToolCall.Broadcast(Id, Decoded.ToolCall.ToolName, Decoded.ToolCall.ArgumentsJson);
			bDispatchedTyped = true;
		}
		break;

	case ETryllNodeEventKind::IntentClassified:
		if (OnIntentClassified.IsBound())
		{
			OnIntentClassified.Broadcast(Id, Decoded.Intent);
			bDispatchedTyped = true;
		}
		break;

	case ETryllNodeEventKind::IntentLlmClassified:
		if (OnIntentLlmClassified.IsBound())
		{
			OnIntentLlmClassified.Broadcast(Id, Decoded.IntentLlm);
			bDispatchedTyped = true;
		}
		break;

	case ETryllNodeEventKind::Paused:
		if (OnPaused.IsBound())
		{
			OnPaused.Broadcast(Id, Decoded.NodeName, Decoded.PendingExit);
			bDispatchedTyped = true;
		}
		break;

	default:
		break;
	}

	if (!bDispatchedTyped && OnNodeEvent.IsBound())
	{
		OnNodeEvent.Broadcast(Id, Decoded.NodeName, Decoded.EventType, Decoded.KvPairs);
	}
}

void UTryllSubsystem::ForwardConnectionChanged(bool bConnected)
{
	OnConnectionChanged.Broadcast(bConnected);
}

void UTryllSubsystem::ForwardError(const FString& ErrorMessage)
{
	OnError.Broadcast(ErrorMessage);
}

void UTryllSubsystem::ForwardAgentDestroyed(uint64 AgentId)
{
	OnAgentDestroyed.Broadcast(static_cast<int64>(AgentId));
}

void UTryllSubsystem::ForwardCreateSession(bool bSuccess)
{
	bSessionConfigured = bSuccess;
	bCreateSessionInFlight = false;
	OnCreateSessionComplete.Broadcast(bSuccess);
}

void UTryllSubsystem::ForwardSetSessionLocale(const FString& Locale, bool bSuccess)
{
	OnSetSessionLocaleComplete.Broadcast(Locale, bSuccess);
}

void UTryllSubsystem::ForwardListModels(const TArray<FTryllModelInfo>& Models, bool bSuccess)
{
	OnListModelsComplete.Broadcast(Models, bSuccess);
}

void UTryllSubsystem::ForwardDownloadProgress(const FString& ModelName, uint64 BytesDownloaded, uint64 TotalBytes, float Percent)
{
	OnDownloadProgress.Broadcast(
		ModelName,
		static_cast<int64>(BytesDownloaded),
		static_cast<int64>(TotalBytes),
		Percent);
}

void UTryllSubsystem::ForwardDownloadComplete(const FString& ModelName, bool bSuccess, const FString& ErrorMessage)
{
	OnDownloadComplete.Broadcast(ModelName, bSuccess, ErrorMessage);
}

void UTryllSubsystem::ForwardLoadModel(const FString& ModelName, bool bSuccess)
{
	OnLoadModelComplete.Broadcast(ModelName, bSuccess);
}

void UTryllSubsystem::ForwardUnloadModel(const FString& ModelName, bool bSuccess)
{
	OnUnloadModelComplete.Broadcast(ModelName, bSuccess);
}

void UTryllSubsystem::ForwardCreateStringStorage(const FString& Name, bool bSuccess)
{
	OnCreateStringStorageComplete.Broadcast(Name, bSuccess);
}

void UTryllSubsystem::ForwardDestroyStringStorage(const FString& Name, bool bSuccess)
{
	OnDestroyStringStorageComplete.Broadcast(Name, bSuccess);
}

void UTryllSubsystem::ForwardCreateEmbedded(const FString& Name, int32 RecordCount, bool bSuccess)
{
	OnCreateEmbeddedStringStorageComplete.Broadcast(Name, RecordCount, bSuccess);
}

void UTryllSubsystem::ForwardDestroyEmbedded(const FString& Name, bool bSuccess)
{
	OnDestroyEmbeddedStringStorageComplete.Broadcast(Name, bSuccess);
}

void UTryllSubsystem::ProcessEvents()
{
	// A completion can release the last reference. Hold the connection until
	// PumpEvents returns.
	const TSharedPtr<FTryllConnection> Pinned = Connection;
	if (!Pinned.IsValid())
	{
		return;
	}
	EnsureConnectionBound();
	Pinned->PumpEvents();
}

void UTryllSubsystem::RequestCreateStringStorage(
	const FString& Name,
	const TArray<FString>& Strings,
	TFunction<FTryllOnStringStorage> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::CreateStringStorage(
		Live.Get(), this, Name, static_cast<uint8>(::Tryll::StringStorageKind_List), Strings, TArray<FString>{}, FString{},
		MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](const FString& StorageName, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnCreateStringStorageComplete.Broadcast(StorageName, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

void UTryllSubsystem::RequestCreateStringStorageFromFile(
	const FString& Name,
	const FString& FilePath,
	uint8          Kind,
	TFunction<FTryllOnStringStorage> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::CreateStringStorage(
		Live.Get(), this, Name, Kind, TArray<FString>{}, TArray<FString>{}, FilePath,
		MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](const FString& StorageName, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnCreateStringStorageComplete.Broadcast(StorageName, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

void UTryllSubsystem::RequestCreateStringStorageKeyed(
	const FString&         Name,
	const TArray<FString>& Keys,
	const TArray<FString>& Values,
	uint8                  Kind,
	TFunction<FTryllOnStringStorage> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::CreateStringStorage(
		Live.Get(), this, Name, Kind, Values, Keys, FString{},
		MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](const FString& StorageName, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnCreateStringStorageComplete.Broadcast(StorageName, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

void UTryllSubsystem::RequestDestroyStringStorage(
	const FString& Name,
	TFunction<FTryllOnStringStorage> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DestroyStringStorage(
		Live.Get(), this, Name, MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](const FString& StorageName, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnDestroyStringStorageComplete.Broadcast(StorageName, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

void UTryllSubsystem::RequestCreateEmbeddedStringStorage(
	const FString& Name,
	const FString& ConfigPath,
	const FString& EmbeddingModel,
	TFunction<FTryllOnEmbeddedStorageCreated> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::CreateEmbeddedStringStorage(
		Live.Get(), this, Name, ConfigPath, TArray<FString>{}, EmbeddingModel, MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](const FString& StorageName, int32 RecordCount, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnCreateEmbeddedStringStorageComplete.Broadcast(StorageName, RecordCount, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

void UTryllSubsystem::RequestCreateEmbeddedStringStorageFromStrings(
	const FString& Name,
	const TArray<FString>& Strings,
	const FString& EmbeddingModel,
	TFunction<FTryllOnEmbeddedStorageCreated> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::CreateEmbeddedStringStorage(
		Live.Get(), this, Name, FString{}, Strings, EmbeddingModel, MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](const FString& StorageName, int32 RecordCount, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnCreateEmbeddedStringStorageComplete.Broadcast(StorageName, RecordCount, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

void UTryllSubsystem::RequestDestroyEmbeddedStringStorage(
	const FString& Name,
	TFunction<FTryllOnEmbeddedStorageDestroyed> OnComplete)
{
	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DestroyEmbeddedStringStorage(
		Live.Get(), this, Name, MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](const FString& StorageName, bool bOk)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnDestroyEmbeddedStringStorageComplete.Broadcast(StorageName, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
}

// ── VoiceInput (STT) lifecycle ────────────────────────────────────────────────

void UTryllSubsystem::RequestCreateVoiceInput(
	const FTryllVoiceInputConfig&        Config,
	TFunction<FTryllOnVoiceInputCreated> OnComplete)
{

	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	if (!Live.IsValid())
	{
		if (OnComplete)
		{
			OnComplete(nullptr, FTryllError(ETryllErrorCode::ConnectionFailed, TEXT("No connection")));
		}
		return;
	}

	::Tryll::WireAudioFormatT Format;
	Format.sample_rate = Config.InputFormat.SampleRate;
	Format.channels = static_cast<uint16>(Config.InputFormat.Channels);
	Format.bits_per_sample = static_cast<uint16>(Config.InputFormat.BitsPerSample);
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::Calls::CreateVoiceInput(
		*Live,
		this,
		Config.ModelName,
		Format,
		Config.VadThreshold,
		Config.VadMinSilenceMs,
		Config.VadSpeechPadMs,
		Config.HotwordsStoragePath,
		Config.HotwordsScore,
		[WeakThis, OnComplete = MoveTemp(OnComplete)](
			TUniquePtr<::Tryll::CreateVoiceInputResponseT> Resp,
			const FTryllError& Error)
		{
			TSharedPtr<FTryllVoiceInput> Voice;
			if (Error.IsOk() && Resp)
			{
				if (UTryllSubsystem* Self = WeakThis.Get())
				{
					if ((Self->GetLiveConnection() != nullptr))
					{
						Voice = MakeShared<FTryllVoiceInput>(Self->GetLiveConnection()->AsShared(), Resp->voice_input_id);
						Self->RegisterVoiceInput(Resp->voice_input_id, Voice);
					}
				}
			}
			if (OnComplete)
			{
				OnComplete(Voice, Error);
			}
		},
		{});
}

void UTryllSubsystem::RequestDestroyVoiceInput(TSharedPtr<FTryllVoiceInput> Voice)
{

	const TSharedPtr<FTryllConnection> Live = PinLiveConnection();
	if (!Live.IsValid() || !Voice.IsValid())
	{
		return;
	}
	const std::uint64_t Vid = Voice->GetVoiceInputId();
	if (Vid == 0)
	{
		return;
	}
	TWeakObjectPtr<UTryllSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DestroyVoiceInput(
		Live.Get(), this, Vid,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllSubsystem* Self = WeakThis.Get();
			if (!Self || Self->bTearingDown || !Self->GetLiveConnection())
			{
				return nullptr;
			}
			return &Self->GetLiveConnection()->GetEvents();
		},
		[WeakThis](uint64 Id)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->VoiceInputsById.Remove(Id);
				if (FTryllConnection* Conn = Self->GetLiveConnection())
				{
					Conn->GetPushRouter().UnregisterVoiceInput(Id);
				}
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllSubsystem* Self = WeakThis.Get())
			{
				Self->OnError.Broadcast(Message);
			}
		});
	Voice->ClearAfterDestroyRequest();
}

bool UTryllSubsystem::HasToolCallListener() const
{
	return OnToolCall.IsBound() || OnToolCallWithId.IsBound() || OnToolCallDetailed.IsBound();
}
