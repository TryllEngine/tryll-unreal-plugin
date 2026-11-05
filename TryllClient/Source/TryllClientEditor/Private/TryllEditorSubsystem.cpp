// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllEditorSubsystem.h"

#include "ClientWire.h"
#include "TryllClientModule.h"
#include "TryllEditorBridge.h"
#include "TryllAgent.h"
#include "TryllMutableKb.h"
#include "TryllConnection.h"
#include "TryllPathUtil.h"
#include "TryllVoiceInput.h"
#include "ClientCodec.h"
#include "Generated/TryllCalls.gen.h"
#include "TryllSessionCalls.h"
#include "TryllMemorySnapshot.h"
#include "TryllObservationTypes.h"
#include "Generated/TryllNodeParamsFactory.h"

#include "messages_generated.h"

#include "TryllRuntimeSettings.h"
#include "TryllTextProvider.h"
#include "TryllEditorUiContract.h"
#include "TryllClientEditorModule.h"
#include "TryllLicence.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

#include <flatbuffers/verifier.h>

using Tryll::Client::CanonicalizeExePath;

namespace
{
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

	// ConnectionDropped / ConnectionFailed are already broadcast by the
	// connection event. Request completions still run, without a second OnError.
	void BroadcastServerError(UTryllEditorSubsystem& Self, const FTryllError& Error)
	{
		if (Error.Code == static_cast<int32>(ETryllErrorCode::ConnectionDropped)
			|| Error.Code == static_cast<int32>(ETryllErrorCode::ConnectionFailed))
		{
			return;
		}
		if (FTryllConnection* Conn = Self.GetSharedConnection())
		{
			Conn->GetEvents().OnError.Broadcast(Error.Message);
		}
		else
		{
			Self.OnErrorNative.Broadcast(Error.Message);
		}
	}

	::Tryll::SessionModelConfigT BuildSessionModelConfig()
	{
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
		return ModelConfig;
	}

	FString CopyUtf8(const std::string& Text)
	{
		return UTF8_TO_TCHAR(Text.c_str());
	}
}

void UTryllEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FTryllEditorBridge::Register(this);
	TextLocaleChangedHandle = UTryllTextProvider::OnLocaleChanged().AddUObject(
		this, &UTryllEditorSubsystem::HandleTextLocaleChanged);
	// Tests flip the in-memory licence store; the live editor server must not follow them.
	LicenceChangedHandle = FTryllLicence::OnChanged().AddWeakLambda(this, [this]()
	{
		if (!FTryllLicence::IsUsingInMemoryStoreForTests())
		{
			HandleLicenceChanged();
		}
	});
}

// ── Server process management (pre-existing) ───────────────────────────────────

bool UTryllEditorSubsystem::IsRunning()
{
	return Server.IsRunning();
}

UTryllEditorSubsystem::EPortProbeResult UTryllEditorSubsystem::ProbeAndVerify(
	int32 Port, const FString& IntendedExe, FString& OutReason)
{
	OutReason.Reset();

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return EPortProbeResult::Empty;
	}

	FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("TryllServerProbe"), false);
	if (!Socket)
	{
		return EPortProbeResult::Empty;
	}
	ON_SCOPE_EXIT
	{
		Socket->Close();
		SocketSubsystem->DestroySocket(Socket);
	};

	Socket->SetNonBlocking(true);

	bool bIsValidIp = false;
	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	Addr->SetIp(TEXT("127.0.0.1"), bIsValidIp);
	Addr->SetPort(Port);

	Socket->Connect(*Addr);
	const bool bConnected = Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromMilliseconds(200))
		&& Socket->GetConnectionState() == SCS_Connected;
	if (!bConnected)
	{
		return EPortProbeResult::Empty;
	}

	// Switch to blocking for the framed read, but bound the wait explicitly
	// first so a listener that never sends anything (some unrelated service
	// squatting on the port) cannot hang the probe.
	Socket->SetNonBlocking(false);
	if (!Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromMilliseconds(1000)))
	{
		OutReason = TEXT("connected but did not send any data (likely not a Tryll server)");
		return EPortProbeResult::Rejected;
	}

	TArray<uint8> Frame;
	if (!Tryll::Client::ReadFrame(Socket, Frame))
	{
		OutReason = TEXT("connected but did not send a valid frame (likely not a Tryll server)");
		return EPortProbeResult::Rejected;
	}

	// The frame may come from an arbitrary process squatting on the port —
	// verify before trusting it as a FlatBuffers message (matches the real
	// connect path's ConnectionReady handling in TryllConnection.cpp).
	flatbuffers::Verifier Verifier(Frame.GetData(), static_cast<size_t>(Frame.Num()));
	if (!::Tryll::VerifyMessageBuffer(Verifier))
	{
		OutReason = TEXT("connected but sent an invalid/non-FlatBuffers frame (likely not a Tryll server)");
		return EPortProbeResult::Rejected;
	}

	const auto* Msg = ::Tryll::GetMessage(Frame.GetData());
	if (!Msg || Msg->body_type() != ::Tryll::MessageBody_ConnectionReady)
	{
		OutReason = TEXT("connected but the first frame was not ConnectionReady");
		return EPortProbeResult::Rejected;
	}

	const auto* Ready = Msg->body_as<::Tryll::ConnectionReady>();
	FString ServerExePath;
	if (Ready && Ready->exe_path())
	{
		ServerExePath = UTF8_TO_TCHAR(Ready->exe_path()->c_str());
	}

	if (ServerExePath.IsEmpty())
	{
		// Server predates the identity fields — cannot verify, but it spoke
		// the Tryll protocol correctly, so treat it as reusable.
		return EPortProbeResult::ReusableServer;
	}

	const FString Intended = CanonicalizeExePath(IntendedExe);
	const FString Actual   = CanonicalizeExePath(ServerExePath);
	if (!Intended.Equals(Actual, ESearchCase::IgnoreCase))
	{
		OutReason = FString::Printf(TEXT("identity mismatch (expected '%s', found '%s')"), *Intended, *Actual);
		return EPortProbeResult::Rejected;
	}

	return EPortProbeResult::ReusableServer;
}

namespace
{
const TCHAR* LlamaCppBackendConfigSection = TEXT("TryllEditorServer");

FString PortScopedConfigKey(const TCHAR* Prefix, int32 Port)
{
	return FString::Printf(TEXT("%s.%d"), Prefix, Port);
}

FString RememberedLlamaCppBackendForPort(int32 Port)
{
	FString Stored;
	GConfig->GetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastLlamaCppBackend"), Port),
		Stored,
		GEditorPerProjectIni);
	return Stored;
}

void RememberLlamaCppBackend(FString& SessionValue, int32 Port, const FString& Backend)
{
	SessionValue = Backend;
	GConfig->SetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastLlamaCppBackend"), Port),
		*Backend,
		GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

FString RememberedSaveRootForPort(int32 Port)
{
	FString Stored;
	GConfig->GetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastSaveRoot"), Port),
		Stored,
		GEditorPerProjectIni);
	return Stored;
}

void RememberSaveRoot(FString& SessionValue, int32 Port, const FString& SaveRoot)
{
	SessionValue = SaveRoot;
	GConfig->SetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastSaveRoot"), Port),
		*SaveRoot,
		GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

bool LlamaCppBackendMatchesReuse(const FString& Remembered, const FString& Wanted)
{
	return Remembered.Equals(Wanted, ESearchCase::CaseSensitive)
		|| (Remembered.IsEmpty() && Wanted.Equals(TEXT("vulkan"), ESearchCase::IgnoreCase));
}

bool SaveRootMatchesReuse(const FString& Remembered, const FString& Wanted)
{
	return Remembered.Equals(Wanted, ESearchCase::CaseSensitive);
}

FString RememberedUserModelsCatalogForPort(int32 Port)
{
	FString Stored;
	GConfig->GetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastUserModelsCatalog"), Port),
		Stored,
		GEditorPerProjectIni);
	return Stored;
}

void RememberUserModelsCatalog(FString& SessionValue, int32 Port, const FString& CatalogPath)
{
	SessionValue = CatalogPath;
	GConfig->SetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastUserModelsCatalog"), Port),
		*CatalogPath,
		GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

FString RememberedStorageRootForPort(int32 Port)
{
	FString Stored;
	GConfig->GetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastStorageRoot"), Port),
		Stored,
		GEditorPerProjectIni);
	return Stored;
}

void RememberStorageRoot(FString& SessionValue, int32 Port, const FString& StorageRoot)
{
	SessionValue = StorageRoot;
	GConfig->SetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastStorageRoot"), Port),
		*StorageRoot,
		GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

FString RememberedProjectNameForPort(int32 Port)
{
	FString Stored;
	GConfig->GetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastProjectName"), Port),
		Stored,
		GEditorPerProjectIni);
	return Stored;
}

FString LaunchModeForReuse()
{
	return FTryllManagedServerProcess::ResolveLaunchMode();
}

FString RememberedModeForPort(int32 Port)
{
	FString Stored;
	GConfig->GetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastMode"), Port),
		Stored,
		GEditorPerProjectIni);
	return Stored;
}

void RememberMode(FString& SessionValue, int32 Port, const FString& Mode)
{
	SessionValue = Mode;
	GConfig->SetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastMode"), Port),
		*Mode,
		GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

void RememberProjectName(FString& SessionValue, int32 Port, const FString& ProjectName)
{
	SessionValue = ProjectName;
	GConfig->SetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastProjectName"), Port),
		*ProjectName,
		GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

FString RememberedProjectDirForPort(int32 Port)
{
	FString Stored;
	GConfig->GetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastProjectDir"), Port),
		Stored,
		GEditorPerProjectIni);
	return Stored;
}

void RememberProjectDir(FString& SessionValue, int32 Port, const FString& ProjectDir)
{
	SessionValue = ProjectDir;
	GConfig->SetString(
		LlamaCppBackendConfigSection,
		*PortScopedConfigKey(TEXT("LastProjectDir"), Port),
		*ProjectDir,
		GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

/** Path actually passed to the server: empty when unset or the file is missing. */
FString UserModelsCatalogForLaunch()
{
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	if (!Settings)
	{
		return FString();
	}
	const FString Path = Settings->ResolveUserModelsCatalogForLaunch();
	if (Path.IsEmpty() || !FPaths::FileExists(Path))
	{
		return FString();
	}
	return Path;
}

FString StorageRootForLaunch()
{
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	return Settings ? Settings->ResolveStorageDataFolderForServer() : FString();
}

FString ProjectNameForLaunch()
{
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	return Settings ? Settings->ResolveProjectName() : FString();
}

FString ProjectDirForLaunch()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
}

bool TcpAccepts(int32 Port)
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return false;
	}
	FSocket* Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("TryllOwnedProbe"), false);
	if (!Socket)
	{
		return false;
	}
	bool bIsValidIp = false;
	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	Addr->SetIp(TEXT("127.0.0.1"), bIsValidIp);
	Addr->SetPort(Port);
	Socket->SetNonBlocking(true);
	Socket->Connect(*Addr);
	const bool bConnected =
		Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromMilliseconds(200))
		&& Socket->GetConnectionState() == SCS_Connected;
	Socket->Close();
	SocketSubsystem->DestroySocket(Socket);
	return bConnected;
}

bool OwnedProcessIsUsable(FTryllManagedServerProcess& Server, int32 Port)
{
	if (!Server.IsRunning())
	{
		return false;
	}
	if (!Server.IsObservedReady())
	{
		return true;
	}
	if (TcpAccepts(Port))
	{
		return true;
	}
	FPlatformProcess::Sleep(0.25f);
	if (TcpAccepts(Port))
	{
		return true;
	}
	UE_LOG(LogTryll, Warning,
		TEXT("TryllEditorSubsystem: owned server is alive but not accepting on port %d — ")
		TEXT("treating it as a hung idle-exit and relaunching."),
		Port);
	Server.TerminateAndRelease();
	return false;
}
} // namespace

bool UTryllEditorSubsystem::TryLaunch(
	EServerBuildVariant Variant, int32 KnownPort, int32 IdleShutdownTimeoutSeconds, FString& OutError)
{
	OutError.Reset();

	// Nothing starts or is reused until the licence is accepted. Accepting it
	// resumes this launch for consumers still waiting (HandleLicenceChanged).
	if (IsLicenceBlockingLaunch())
	{
		bLaunchBlockedByLicence = true;
		OutError = FTryllEditorUiContract::ServerNeedsLicence;
		RequestWelcomeAfterBlockedLaunch();
		return false;
	}
	bLaunchBlockedByLicence = false;
	if (DebugLaunchAfterGateForTests)
	{
		return DebugLaunchAfterGateForTests(OutError);
	}

	if (const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>())
	{
		if (UTryllRuntimeSettings::IsStorageUnderContent(Settings->StorageDataFolder))
		{
			OutError = TEXT("Storage data folder must be outside Content/. Move it to TryllStorage, then launch again.");
			return false;
		}
	}

	const FString WantedBackend = GetDefault<UTryllRuntimeSettings>()->LlamaCppBackendCliValue();
	const FString WantedSaveRoot = GetDefault<UTryllRuntimeSettings>()->ResolveSaveDataFolderForLaunch();
	const FString WantedStorageRoot = StorageRootForLaunch();
	const FString WantedProjectName = ProjectNameForLaunch();
	const FString WantedProjectDir = ProjectDirForLaunch();
	const FString WantedUserCatalog = UserModelsCatalogForLaunch();
	if (Server.IsRunning())
	{
		if (!LlamaCppBackendMatchesReuse(LastLlamaCppBackend, WantedBackend)
			|| !SaveRootMatchesReuse(LastSaveRoot, WantedSaveRoot)
			|| !SaveRootMatchesReuse(LastStorageRoot, WantedStorageRoot)
			|| !SaveRootMatchesReuse(LastProjectName, WantedProjectName)
			|| !SaveRootMatchesReuse(LastProjectDir, WantedProjectDir)
			|| !SaveRootMatchesReuse(LastUserModelsCatalog, WantedUserCatalog)
			|| !SaveRootMatchesReuse(LastMode, LaunchModeForReuse()))
		{
			// Setting changed while our handle is live — drop it and treat the
			// still-running process as a stranger so we spawn with the new flag.
			Server.ReleaseHandle();
		}
		else if (OwnedProcessIsUsable(Server, ActualPort > 0 ? ActualPort : KnownPort))
		{
			return true;
		}
	}

	const FString Abs = UTryllSubsystem::ResolveServerExePath(Variant);
	if (Abs.IsEmpty() || !FPaths::FileExists(Abs))
	{
		OutError = FString::Printf(TEXT("bundled server exe not found (resolved to '%s')."), *Abs);
		return false;
	}

	// Probe first so a still-warm server (previous PIE session, another tool
	// window) is reused instead of spawning a duplicate that fails to bind.
	// Fail closed: only reuse when the probe confirms a matching exe_path (or
	// the server predates identity fields). A mismatched/unverifiable listener
	// is treated as a stranger — spawn our own server on a free ephemeral port
	// instead of the known port, which is occupied and not ours to bind.
	FString ProbeReason;
	bool bKnownPortHasStranger = false;
	switch (ProbeAndVerify(KnownPort, Abs, ProbeReason))
	{
	case EPortProbeResult::ReusableServer:
	{
		const FString Remembered = RememberedLlamaCppBackendForPort(KnownPort);
		const FString RememberedSave = RememberedSaveRootForPort(KnownPort);
		const FString RememberedStorage = RememberedStorageRootForPort(KnownPort);
		const FString RememberedName = RememberedProjectNameForPort(KnownPort);
		const FString RememberedDir = RememberedProjectDirForPort(KnownPort);
		const FString RememberedCatalog = RememberedUserModelsCatalogForPort(KnownPort);
		if (LlamaCppBackendMatchesReuse(Remembered, WantedBackend)
			&& SaveRootMatchesReuse(RememberedSave, WantedSaveRoot)
			&& SaveRootMatchesReuse(RememberedStorage, WantedStorageRoot)
			&& SaveRootMatchesReuse(RememberedName, WantedProjectName)
			&& SaveRootMatchesReuse(RememberedDir, WantedProjectDir)
			&& SaveRootMatchesReuse(RememberedCatalog, WantedUserCatalog)
			&& SaveRootMatchesReuse(RememberedModeForPort(KnownPort), LaunchModeForReuse()))
		{
			UE_LOG(LogTryll, Log,
				TEXT("TryllEditorSubsystem: port %d already in use by a verified Tryll server — reusing."),
				KnownPort);
			ActualPort = KnownPort;
			LastLlamaCppBackend = WantedBackend;
			LastSaveRoot = WantedSaveRoot;
			LastStorageRoot = WantedStorageRoot;
			LastProjectName = WantedProjectName;
			LastProjectDir = WantedProjectDir;
			LastUserModelsCatalog = WantedUserCatalog;
			LastMode = LaunchModeForReuse();
			return true;
		}
		UE_LOG(LogTryll, Warning,
			TEXT("TryllEditorSubsystem: port %d is a verified Tryll server started with backend '%s', ")
			TEXT("save-root '%s', storage '%s', project '%s' / '%s', catalog '%s' but this project wants ")
			TEXT("'%s' / '%s' / '%s' / '%s' / '%s' / '%s' — will not reuse it."),
			KnownPort,
			Remembered.IsEmpty() ? TEXT("unknown") : *Remembered,
			*RememberedSave,
			*RememberedStorage,
			*RememberedName,
			*RememberedDir,
			*RememberedCatalog,
			*WantedBackend,
			*WantedSaveRoot,
			*WantedStorageRoot,
			*WantedProjectName,
			*WantedProjectDir,
			*WantedUserCatalog);
		bKnownPortHasStranger = true;
		break;
	}

	case EPortProbeResult::Rejected:
		UE_LOG(LogTryll, Warning,
			TEXT("TryllEditorSubsystem: port %d is occupied by a different process (%s); ")
			TEXT("will not reuse it — spawning our own server on a free port instead."),
			KnownPort, *ProbeReason);
		bKnownPortHasStranger = true;
		break;

	case EPortProbeResult::Empty:
	default:
		break; // nothing there — proceed to spawn on the known port as usual
	}

	// Auto-allocated ports (the stranger-fallback case) can be re-picked on a bind
	// collision, so retry a few times; a direct bind to the known, fixed port gets
	// exactly one attempt — report the conflict instead of retrying blindly.
	const bool  bAutoPort    = bKnownPortHasStranger;
	const int32 MaxAttempts  = bAutoPort ? 4 : 1;
	FString     LastReason   = TEXT("unknown");

	for (int32 Attempt = 1; Attempt <= MaxAttempts; ++Attempt)
	{
		int32 ListenPort = KnownPort;
		if (bKnownPortHasStranger)
		{
			ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
			if (SocketSubsystem)
			{
				FSocket* ProbeSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("TryllPortProbe"), false);
				if (ProbeSocket)
				{
					TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
					Addr->SetAnyAddress();
					Addr->SetPort(0);
					if (ProbeSocket->Bind(*Addr))
					{
						ProbeSocket->GetAddress(*Addr);
						const int32 BoundPort = Addr->GetPort();
						if (BoundPort > 0)
						{
							ListenPort = BoundPort;
						}
					}
					ProbeSocket->Close();
					SocketSubsystem->DestroySocket(ProbeSocket);
				}
			}
			// If the free-port probe itself failed, ListenPort stays KnownPort as a
			// last resort — the spawn will likely fail to bind, surfacing a clear error.
		}

		bool    bPortConflict = false;
		FString SpawnError;
		Server.DebugExtraCliArgs = DebugExtraCliArgsForTests;
		Server.DebugStorageRoot = DebugStorageRootForTests;
		Server.DebugSaveRoot = DebugSaveRootForTests;
		Server.bDebugNoUserCatalogs = bDebugNoUserCatalogsForTests;
		if (Server.Spawn(Abs, ListenPort, IdleShutdownTimeoutSeconds,
			GetDefault<UTryllRuntimeSettings>()->ServerReadinessTimeoutSeconds,
			WantedBackend, SpawnError, bPortConflict))
		{
			ActualPort = ListenPort;
			RememberLlamaCppBackend(LastLlamaCppBackend, ListenPort, WantedBackend);
			RememberSaveRoot(LastSaveRoot, ListenPort, WantedSaveRoot);
			RememberStorageRoot(LastStorageRoot, ListenPort, WantedStorageRoot);
			RememberProjectName(LastProjectName, ListenPort, WantedProjectName);
			RememberMode(LastMode, ListenPort, LaunchModeForReuse());
			RememberProjectDir(LastProjectDir, ListenPort, WantedProjectDir);
			RememberUserModelsCatalog(LastUserModelsCatalog, ListenPort, WantedUserCatalog);
			return true;
		}

		LastReason = SpawnError;
		if (bPortConflict && bAutoPort && Attempt < MaxAttempts)
		{
			UE_LOG(LogTryll, Warning,
				TEXT("TryllEditorSubsystem: port %d was taken before the server could bind ")
				TEXT("(attempt %d/%d); retrying on a new free port."),
				ListenPort, Attempt, MaxAttempts);
			continue;
		}

		OutError = SpawnError;
		return false;
	}

	OutError = FString::Printf(
		TEXT("gave up starting the Tryll server after %d attempts - %s"), MaxAttempts, *LastReason);
	return false;
}

void UTryllEditorSubsystem::ReleaseHandle()
{
	Server.ReleaseHandle();
}

void UTryllEditorSubsystem::DrainPendingRequests(const FTryllError& TerminalError)
{
	if (Connection.IsValid())
	{
		Connection->FailRegistered(TerminalError);
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
	if (Connection.IsValid())
	{
		Connection->GetPushRouter().SnapshotAgents(AgentSnapshot);
	}
	else
	{
		AgentsById.GenerateValueArray(AgentSnapshot);
	}
	for (const TWeakPtr<FTryllAgent>& WeakAgent : AgentSnapshot)
	{
		if (TSharedPtr<FTryllAgent> Agent = WeakAgent.Pin())
		{
			Agent->DrainPendingLifecycleCallbacks(TerminalError);
			Agent->ClearPausedToolBatch();
		}
	}

	TArray<TWeakPtr<FTryllVoiceInput>> VoiceSnapshot;
	if (Connection.IsValid())
	{
		Connection->GetPushRouter().SnapshotVoices(VoiceSnapshot);
	}
	else
	{
		VoiceInputsById.GenerateValueArray(VoiceSnapshot);
	}
	for (const TWeakPtr<FTryllVoiceInput>& WeakVoice : VoiceSnapshot)
	{
		if (TSharedPtr<FTryllVoiceInput> Voice = WeakVoice.Pin())
		{
			Voice->DrainPendingAcks(TerminalError);
		}
	}
}

void UTryllEditorSubsystem::InvalidateConnectionEpoch(bool bNotifyDisconnected)
{
	TArray<TWeakPtr<FTryllAgent>> Agents;
	TArray<TWeakPtr<FTryllVoiceInput>> Voices;
	if (Connection.IsValid())
	{
		Connection->GetPushRouter().SnapshotAgents(Agents);
		Connection->GetPushRouter().SnapshotVoices(Voices);
		Connection->GetPushRouter().ClearObjects();
		Connection->ResetSessionPhase();
	}
	else
	{
		AgentsById.GenerateValueArray(Agents);
		VoiceInputsById.GenerateValueArray(Voices);
	}
	for (const TWeakPtr<FTryllAgent>& WeakAgent : Agents)
	{
		if (TSharedPtr<FTryllAgent> Agent = WeakAgent.Pin())
		{
			Agent->ClearAfterDestroyRequest();
		}
	}
	for (const TWeakPtr<FTryllVoiceInput>& WeakVoice : Voices)
	{
		if (TSharedPtr<FTryllVoiceInput> Voice = WeakVoice.Pin())
		{
			Voice->ClearAfterDestroyRequest();
		}
	}
	LocalizedAgents.ForgetAll();
	AgentsById.Empty();
	VoiceInputsById.Empty();
	if (bNotifyDisconnected)
	{
		if (Connection.IsValid())
		{
			Connection->GetEvents().OnConnectionChanged.Broadcast(false);
		}
		else
		{
			OnConnectionChangedNative.Broadcast(false);
		}
	}
}

void UTryllEditorSubsystem::Deinitialize()
{
	if (TextLocaleChangedHandle.IsValid())
	{
		UTryllTextProvider::OnLocaleChanged().Remove(TextLocaleChangedHandle);
		TextLocaleChangedHandle.Reset();
	}
	if (LicenceChangedHandle.IsValid())
	{
		FTryllLicence::OnChanged().Remove(LicenceChangedHandle);
		LicenceChangedHandle.Reset();
	}
	LocalizedAgents.ForgetAll();
	LocalizedParamKeepAlive.Reset();
	FTryllEditorBridge::Unregister(this);

	DrainPendingRequests(FTryllError(ETryllErrorCode::ConnectionDropped, TEXT("Connection dropped")));
	InvalidateConnectionEpoch(/*bNotifyDisconnected=*/false);

	if (ConnectRetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ConnectRetryTickerHandle);
		ConnectRetryTickerHandle.Reset();
	}
	RemainingConnectAttempts = 0;

	if (Connection.IsValid())
	{
		Connection->Stop();
		Connection.Reset();
	}
	ConnectionRefCount = 0;

	// Fires once on editor shutdown — not per PIE stop or tool-window close,
	// since other consumers may still be relying on this warm server.
	ReleaseHandle();
	Super::Deinitialize();
}

// ── FTickableEditorObject ───────────────────────────────────────────────────────

bool UTryllEditorSubsystem::IsTickable() const
{
	return Connection.IsValid();
}

TStatId UTryllEditorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTryllEditorSubsystem, STATGROUP_Tickables);
}

void UTryllEditorSubsystem::Tick(float DeltaTime)
{
	(void)DeltaTime;
	ProcessEvents();
}

// ── Connection lifetime (refcounted) ────────────────────────────────────────────

void UTryllEditorSubsystem::AcquireConnection()
{
	++ConnectionRefCount;
	UE_LOG(LogTryll, Log,
		TEXT("UTryllEditorSubsystem: AcquireConnection — refcount now %d (%s)"),
		ConnectionRefCount,
		Connection.IsValid() ? TEXT("reusing live connection") : TEXT("no connection yet — will launch/connect"));

	if (DebugInhibitLaunchForTests)
	{
		RefreshOwnerPhase();
		return;
	}

	// Gate launch/connect on a live socket, NOT on pointer validity. A dropped
	// transport leaves Connection non-null; treating that as "already connected"
	// wedges later consumers (Chat Error retry while PIE still holds a ref).
	if (Connection.IsValid() && Connection->IsConnected())
	{
		RefreshOwnerPhase();
		return;
	}

	// A second consumer joining a cold-start attempt must not reset the shared
	// retry budget or replace the in-flight worker.
	if (Connection.IsValid() && HasConnectRetriesRemaining())
	{
		RefreshOwnerPhase();
		return;
	}

	LaunchAndConnect();
}

void UTryllEditorSubsystem::LaunchAndConnect()
{
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	const int32 KnownPort = Settings->ServerPort;
	FString Error;
	bLaunchingServer = true;
	RefreshOwnerPhase();
	if (!TryLaunch(Settings->ServerBuildVariant, KnownPort, /*IdleShutdownTimeoutSeconds=*/60, Error))
	{
		UE_LOG(LogTryll, Error, TEXT("Failed to launch/reuse the Tryll server: %s"), *Error);
		// Surface the launch failure to subscribers (e.g. the PIE model guard) so it
		// becomes a clear error instead of an indefinite wait — this branch previously
		// returned silently, leaving a guard that only advances on delegate callbacks
		// stuck forever. Mirror ProcessEvents' ConnectFailed order: error, then the bool.
		// The ConnectionRefCount taken above stays balanced: the subscriber's failure
		// handler releases its hold, and Disconnect() is a safe no-op with no Connection.
		bLaunchingServer = false;
		LastOwnerError = Error;
		OnErrorNative.Broadcast(Error);
		OnConnectionChangedNative.Broadcast(false);
		RefreshOwnerPhase();
		return;
	}

	bLaunchingServer = false;
	Connect();
}

// ── Licence gate ────────────────────────────────────────────────────────────────

bool UTryllEditorSubsystem::IsLicenceBlockingLaunch() const
{
	if (!DebugNonInteractiveForTests.IsSet())
	{
		return FTryllManagedServerProcess::IsLaunchBlockedByLicence();
	}
	return FTryllManagedServerProcess::ShouldBlockLaunch(
		FTryllManagedServerProcess::ResolveLaunchMode(),
		FTryllLicence::IsAccepted(),
		DebugNonInteractiveForTests.GetValue());
}

void UTryllEditorSubsystem::RequestWelcomeAfterBlockedLaunch()
{
	if (DebugOpenWelcomeForTests)
	{
		DebugOpenWelcomeForTests();
		return;
	}
	// Deferred: a blocked launch often runs inside another tab's spawn (a panel
	// acquiring its connection), where invoking a tab is not safe.
	if (bWelcomeOpenPending || !FSlateApplication::IsInitialized())
	{
		return;
	}
	bWelcomeOpenPending = true;
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float) -> bool
		{
			bWelcomeOpenPending = false;
			if (FSlateApplication::IsInitialized()
				&& FGlobalTabmanager::Get()->HasTabSpawner(FTryllClientEditorModule::WelcomeTabName))
			{
				FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::WelcomeTabName);
			}
			return false;
		}),
		0.0f);
}

void UTryllEditorSubsystem::HandleLicenceChanged()
{
	if (IsLicenceBlockingLaunch())
	{
		StopServerForWithdrawnLicence();
		return;
	}
	// Windows that were waiting on a blocked launch connect without being reopened.
	if (bLaunchBlockedByLicence && ConnectionRefCount > 0 && !IsConnected() && !DebugInhibitLaunchForTests)
	{
		UE_LOG(LogTryll, Log, TEXT("UTryllEditorSubsystem: licence accepted — starting the Tryll server."));
		LaunchAndConnect();
	}
}

void UTryllEditorSubsystem::StopServerForWithdrawnLicence()
{
	if (ConnectionRefCount > 0)
	{
		bLaunchBlockedByLicence = true;
	}
	if (!Server.IsRunning() && !Connection.IsValid() && ActualPort <= 0)
	{
		return;
	}
	UE_LOG(LogTryll, Log, TEXT("UTryllEditorSubsystem: licence withdrawn — stopping the Tryll server."));

	// A consumer can release the last lease from a callback below, and that
	// Disconnect drops Connection while its delegates still broadcast. Hold the
	// connection until this returns, as ProcessEvents does.
	const TSharedPtr<FTryllConnection> Pinned = Connection;

	// Connect's order: bind, fail what is pending, then end the epoch with a
	// broadcast so every consumer (panels, PIE) sees the disconnect. The reason
	// goes first, so a consumer that fails on the disconnect shows it.
	const bool bWasConnected = IsConnected();
	if (Connection.IsValid())
	{
		EnsureConnectionBound();
	}
	const FString Reason = FTryllEditorUiContract::ServerNeedsLicence;
	DrainPendingRequests(FTryllError(ETryllErrorCode::ConnectionDropped, Reason));
	if (ConnectionRefCount > 0)
	{
		if (Pinned.IsValid())
		{
			Pinned->GetEvents().OnError.Broadcast(Reason);
		}
		else
		{
			OnErrorNative.Broadcast(Reason);
		}
	}
	InvalidateConnectionEpoch(bWasConnected);
	Disconnect();

	// A server this editor spawned stops now. A reused warm server has no handle
	// here; with no client left it exits after its idle timeout.
	Server.TerminateAndRelease();
	ActualPort = 0;

	if (ConnectionRefCount > 0)
	{
		LastOwnerError = Reason;
	}
	RefreshOwnerPhase();
}

void UTryllEditorSubsystem::ReleaseConnection()
{
	if (ConnectionRefCount == 0)
	{
		UE_LOG(LogTryll, Log, TEXT("UTryllEditorSubsystem: ReleaseConnection — already at 0 (no-op)"));
		return;
	}
	--ConnectionRefCount;
	UE_LOG(LogTryll, Log, TEXT("UTryllEditorSubsystem: ReleaseConnection — refcount now %d%s"),
		ConnectionRefCount,
		ConnectionRefCount == 0 ? TEXT(" — dropping to 0, disconnecting") : TEXT(""));
	if (ConnectionRefCount == 0)
	{
		Disconnect();
	}
	RefreshOwnerPhase();
}

// ── Connection management ───────────────────────────────────────────────────────

void UTryllEditorSubsystem::AdoptRelaunchedServer()
{
	if (ActualPort <= 0 || !IsConnected())
	{
		return;
	}
	Disconnect();
	Connect();
}

void UTryllEditorSubsystem::Connect()
{
	// Starting a (re)connection means no session exists yet on it. Clear the
	// session flags now so IsSessionReady() can't report a stale session from a
	// prior connection during the window between the socket coming up and the
	// Connected event being processed — this subsystem outlives many
	// connect/disconnect cycles, so the flags would otherwise leak across them
	// (observed: a fast reused server + a scene UTryllAgentComponent's BeginPlay
	// CreateAgent racing in that window, rejected by the server with "No session").
	const bool bHadEpoch = (Connection.IsValid() && Connection->IsConnected())
		|| bSessionConfigured
		|| (Connection.IsValid() && Connection->GetPushRouter().AgentCount() > 0)
		|| (Connection.IsValid() && Connection->GetPushRouter().VoiceCount() > 0);
	bSessionConfigured     = false;
	bCreateSessionInFlight = false;
	if (Connection.IsValid())
	{
		Connection->ResetSessionPhase();
		// Bind before the epoch broadcast, or the prior connection's disconnect
		// lands on the events object with nobody forwarding it to native listeners.
		EnsureConnectionBound();
	}
	DrainPendingRequests(FTryllError(ETryllErrorCode::ConnectionDropped, TEXT("Connection replaced")));
	InvalidateConnectionEpoch(bHadEpoch);

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

	const int32 ConnectPort = ActualPort > 0 ? ActualPort : GetDefault<UTryllRuntimeSettings>()->ServerPort;
	UE_LOG(LogTryll, Log,
		TEXT("UTryllEditorSubsystem: Connect — starting worker to 127.0.0.1:%d (retry budget %d)"),
		ConnectPort, RemainingConnectAttempts);
	Connection->Start(TEXT("127.0.0.1"), ConnectPort);
	RefreshOwnerPhase();
}

void UTryllEditorSubsystem::Disconnect()
{
	UE_LOG(LogTryll, Log,
		TEXT("UTryllEditorSubsystem: Disconnect — tearing down shared connection (session id was %lld)"),
		GetSessionId());

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
		// Flush observer envelopes (for example a DestroyAgent note just before Stop)
		// before dropping the connection. ProcessEvents early-outs once Connection is
		// null, so without this pump Agent Log never sees the destroy and the agent
		// never flips to "destroyed" in the list (Unity still shows it).
		Connection->GetObserverHub().Pump(256);
		Connection->Stop();
		// Reset so AcquireConnection()'s Connection.IsValid() gate reflects "have a
		// live connection" — otherwise a later reopen would see a stale pointer and
		// never reconnect. A play session that still shares this connection releases
		// its own ref and does not reach this last-ref teardown.
		Connection.Reset();
	}
	LastOwnerError.Reset();
	RefreshOwnerPhase();
}

void UTryllEditorSubsystem::OnConnectRetryStateChanged(bool bConnected)
{
	if (bConnected)
	{
		RemainingConnectAttempts = 0;
		if (ConnectRetryTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(ConnectRetryTickerHandle);
			ConnectRetryTickerHandle.Reset();
		}
		return;
	}

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
			if (Connection.IsValid() && !Connection->IsConnected())
			{
				Connection->Start(TEXT("127.0.0.1"), ActualPort > 0 ? ActualPort : GetDefault<UTryllRuntimeSettings>()->ServerPort);
			}
			ConnectRetryTickerHandle.Reset();
			return false;
		}),
		Delay);
}

bool UTryllEditorSubsystem::IsConnected() const
{
	return Connection.IsValid() && Connection->IsConnected();
}

int64 UTryllEditorSubsystem::GetSessionId() const
{
	return Connection.IsValid() ? static_cast<int64>(Connection->GetSessionId()) : 0;
}

bool UTryllEditorSubsystem::IsSessionReady() const
{
	const bool bReady = bSessionConfigured
		|| (Connection.IsValid() && Connection->IsSessionConfigured());
	return IsConnected() && bReady;
}

void UTryllEditorSubsystem::SetLastOwnerError(const FString& Error)
{
	LastOwnerError = Error;
	RefreshOwnerPhase();
}

void UTryllEditorSubsystem::RefreshOwnerPhase()
{
	ETryllOwnerPhase Next = ETryllOwnerPhase::Idle;
	FString Detail;

	if (IsSessionReady())
	{
		Next = ETryllOwnerPhase::Ready;
	}
	else if (bCreateSessionInFlight || (Connection.IsValid() && Connection->IsCreateSessionInFlight()))
	{
		Next = ETryllOwnerPhase::Configuring;
		Detail = TEXT("Configuring session…");
	}
	else if (bLaunchingServer || (ConnectionRefCount > 0 && !IsConnected() && HasConnectRetriesRemaining()))
	{
		Next = ETryllOwnerPhase::Connecting;
		if (bLaunchingServer)
		{
			Detail = TEXT("Launching server…");
		}
		else if (!LastOwnerError.IsEmpty())
		{
			Detail = TEXT("Waiting for server…");
		}
		else
		{
			Detail = FTryllEditorUiContract::StatusConnecting;
		}
	}
	else if (!LastOwnerError.IsEmpty() && !HasConnectRetriesRemaining() && ConnectionRefCount > 0)
	{
		// Terminal connect or CreateSession failure wins over a live socket.
		Next = ETryllOwnerPhase::Error;
		Detail = LastOwnerError;
	}
	else if (ConnectionRefCount > 0 && !IsConnected())
	{
		Next = ETryllOwnerPhase::Connecting;
		Detail = FTryllEditorUiContract::StatusConnecting;
	}
	else if (IsConnected() && !bSessionConfigured)
	{
		Next = ETryllOwnerPhase::Configuring;
		Detail = TEXT("Configuring session…");
	}
	else if (ConnectionRefCount == 0)
	{
		// After every consumer released, chrome returns to Idle.
		Next = ETryllOwnerPhase::Idle;
	}

	if (Next != OwnerPhase || Detail != OwnerDetail)
	{
		OwnerPhase = Next;
		OwnerDetail = Detail;
		OnOwnerPhaseChangedNative.Broadcast(OwnerPhase, OwnerDetail);
	}
}

// ── Session configuration ───────────────────────────────────────────────────────

void UTryllEditorSubsystem::CreateSession(
	FString GameName,
	FString Locale,
	bool bUseProjectLocaleWhenEmpty)
{
	// A locale the caller chose is not overridden by later text-provider language changes.
	const bool bExplicitLocale = !Locale.IsEmpty();
	if (Locale.IsEmpty() && bUseProjectLocaleWhenEmpty)
		Locale = UTryllTextProvider::ResolveSessionLocale();

	if (!Connection.IsValid())
	{
		OnCreateSessionCompleteNative.Broadcast(false);
		return;
	}
	// Completions broadcast on the connection. Bind first so a synchronous
	// reply (and the native listeners tests register) actually run.
	EnsureConnectionBound();

	if (bSessionConfigured || Connection->IsSessionConfigured())
	{
		Connection->GetEvents().OnCreateSessionComplete.Broadcast(true);
		return;
	}
	if (bCreateSessionInFlight || Connection->IsCreateSessionInFlight() || !Connection->TryBeginCreateSession())
	{
		return;
	}
	Connection->SetSessionLocaleExplicit(bExplicitLocale);

	bCreateSessionInFlight = true;
	RefreshOwnerPhase();
	// Both hint kinds, not just placement: the Model Manager that authors the
	// hotword choice runs in this session, so dropping it here would make its own
	// control inert in Chat, the Model Manager and PIE — everywhere it is set.
	::Tryll::SessionModelConfigT ModelConfig = BuildSessionModelConfig();
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::Calls::CreateSession(
		*Connection,
		this,
		GameName,
		Locale,
		ModelConfig,
		[WeakThis](TUniquePtr<::Tryll::CreateSessionResponseT> Resp, const FTryllError& Error)
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self)
			{
				return;
			}
			const bool bAlready = Error.Code == static_cast<int32>(ETryllErrorCode::SessionAlreadyExists);
			if (Error.IsOk() || bAlready)
			{
				if (Resp && Self->Connection.IsValid())
				{
					Self->Connection->SetSessionId(Resp->session_id);
				}
				if (Self->Connection.IsValid())
				{
					Self->Connection->MarkSessionReady();
				}
				Self->bSessionConfigured = true;
				Self->bCreateSessionInFlight = false;
				Self->LastOwnerError.Reset();
				if (bAlready)
				{
					UE_LOG(LogTryll, Log,
						TEXT("UTryllEditorSubsystem: CreateSession already exists — treating as ready"));
				}
				else
				{
					UE_LOG(LogTryll, Log, TEXT("UTryllEditorSubsystem: CreateSession OK — session id=%lld"), Self->GetSessionId());
				}
				if (Self->Connection.IsValid())
				{
					Self->Connection->GetEvents().OnCreateSessionComplete.Broadcast(true);
				}
				return;
			}
			UE_LOG(LogTryll, Warning, TEXT("UTryllEditorSubsystem: CreateSession error: %s"), *Error.Message);
			if (Self->Connection.IsValid())
			{
				Self->Connection->AbandonCreateSession();
			}
			Self->bSessionConfigured = false;
			Self->bCreateSessionInFlight = false;
			Self->LastOwnerError = Error.Message;
			if (Self->Connection.IsValid())
			{
				Self->Connection->GetEvents().OnCreateSessionComplete.Broadcast(false);
			}
			BroadcastServerError(*Self, Error);
		},
		[WeakThis](uint64 RequestId)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get(); Self && Self->Connection.IsValid())
			{
				FTryllObservationNote Note;
				Note.Direction = ETryllObservationDirection::Outbound;
				Note.Body = ::Tryll::MessageBody_CreateSessionRequest;
				Note.RequestBody = ::Tryll::MessageBody_CreateSessionRequest;
				Note.RequestId = RequestId;
				Self->Connection->GetObserverHub().AnnotateNote(Note);
			}
		});
}

void UTryllEditorSubsystem::SetSessionLocale(FString Locale)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::SetSessionLocale(
		Connection.Get(), this, Locale,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		[WeakThis](const FString& SentLocale, bool bOk)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnSetSessionLocaleCompleteNative.Broadcast(SentLocale, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

void UTryllEditorSubsystem::SetInferenceThrottle(float Level)
{
	Tryll::Client::FTryllSessionCalls::SetInferenceThrottle(Connection.Get(), Level);
}

void UTryllEditorSubsystem::CreateSessionFromSettings()
{
	const UTryllRuntimeSettings* Settings = GetDefault<UTryllRuntimeSettings>();
	// Empty locale: the text provider's language, then Settings->Locale. Not explicit,
	// so a later language change still moves the session language.
	CreateSession(Settings->GameName, FString(), /*bUseProjectLocaleWhenEmpty=*/true);
}

void UTryllEditorSubsystem::HandleTextLocaleChanged()
{
	UTryllTextProvider* Provider = UTryllTextProvider::Get();
	if (Provider == nullptr)
	{
		return;
	}
	FString Locale;
	if (IsSessionReady() && Connection.IsValid()
		&& UTryllTextProvider::LocaleForLanguageChange(Connection->IsSessionLocaleExplicit(), Locale))
	{
		SetSessionLocale(Locale);
	}
	LocalizedAgents.Refresh(Provider, MakeLocalizedSend(), MakeLocalizedFail());
}

FTryllLocalizedAgentSet::FSend UTryllEditorSubsystem::MakeLocalizedSend()
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
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

FTryllLocalizedAgentSet::FFail UTryllEditorSubsystem::MakeLocalizedFail()
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	return [WeakThis](uint64 AgentId, const FString& AssetName, const FTryllError& Error)
	{
		if (UTryllEditorSubsystem* Self = WeakThis.Get())
		{
			Self->OnLocalizedTextFailedNative.Broadcast(AgentId, AssetName, Error.Message);
		}
	};
}

void UTryllEditorSubsystem::BindLocalizedTurn(const TSharedPtr<FTryllAgent>& Agent, uint64 AgentId)
{
	if (!Agent.IsValid())
	{
		return;
	}
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Agent->SetLocalizedTurnEnded([WeakThis, AgentId]()
	{
		if (UTryllEditorSubsystem* Self = WeakThis.Get())
		{
			Self->LocalizedAgents.NoteTurnEnded(
				AgentId, UTryllTextProvider::Get(), Self->MakeLocalizedSend(), Self->MakeLocalizedFail());
		}
	});
}

void UTryllEditorSubsystem::ReleaseLocalizedOwned(const TArray<UTryllNodeParamsBase*>& Owned)
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

// ── Agent lifecycle ──────────────────────────────────────────────────────────────

void UTryllEditorSubsystem::RequestCreateAgent(
	const FTryllGraphDescription&   Graph,
	TFunction<FTryllOnAgentCreated> OnComplete,
	bool                            bEnableDiagnostics,
	bool                            bMaintainDialogueHistory,
	const TArray<FTryllVariableDecl>& Variables,
	ETryllKvCacheInitialization     KvCacheInitialization,
	ETryllWorkload                  Workload,
	const TOptional<FTryllVoiceProfileData>& Voice)
{
	if (!Connection.IsValid())
	{
		OnErrorNative.Broadcast(TEXT("No connection"));
		if (OnComplete)
		{
			OnComplete(nullptr, FTryllError(ETryllErrorCode::ConnectionFailed, TEXT("No connection")));
		}
		return;
	}

	bEnableDiagnostics = bEnableDiagnostics || Connection->GetDiagnosticsDemand()->IsActive();

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

	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::Calls::CreateAgent(
		*Connection,
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
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Error.IsOk() || !Resp || !Self || !Self->Connection.IsValid())
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
			TSharedPtr<FTryllAgent> Agent = MakeShared<FTryllAgent>(Self->Connection, AgentId, Variables);
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
						AgentId, Provider, Self->MakeLocalizedSend(), Self->MakeLocalizedFail());
				}
			}
			if (OnComplete)
			{
				OnComplete(Agent, Error);
			}
		},
		{});
}

void UTryllEditorSubsystem::RequestDestroyAgent(TSharedPtr<FTryllAgent> Agent)
{
	if (!Connection.IsValid() || !Agent.IsValid())
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
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DestroyAgent(
		Connection.Get(), this, Aid,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		[WeakThis](uint64 Id)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->UnregisterAgent(Id);
			}
		},
		{},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

// ── Model management ─────────────────────────────────────────────────────────────

void UTryllEditorSubsystem::RequestListModels(TFunction<FTryllOnListModels> OnComplete)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::ListModels(
		Connection.Get(), this,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		MoveTemp(OnComplete),
		{});
}

void UTryllEditorSubsystem::RequestGetMemoryConsumption(TFunction<FTryllOnGetMemoryConsumption> OnComplete)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::GetMemoryConsumption(
		Connection.Get(), this, MoveTemp(OnComplete),
		[WeakThis](const FTryllMemorySnapshot& Snap, bool bOk)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnGetMemoryConsumptionCompleteNative.Broadcast(Snap, bOk);
			}
		});
}

void UTryllEditorSubsystem::RequestEnsureMutableKb(
	const FString& Name,
	const FString& Schema,
	TFunction<FTryllOnEnsureMutableKb> OnComplete)
{
	if (!Connection.IsValid())
	{
		OnErrorNative.Broadcast(TEXT("No connection"));
	}
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::EnsureMutableKb(
		Connection.Get(), this, Name, Schema, MoveTemp(OnComplete),
		[WeakThis]() -> TSharedPtr<FTryllConnection>
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return Self->Connection;
		},
		[WeakThis](const TSharedPtr<FTryllMutableKb>& Store)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->MutableKbs.Add(Store);
			}
		},
		{});
}

void UTryllEditorSubsystem::RequestEnumerateMutableKbs(TFunction<FTryllOnEnumerateMutableKbs> OnComplete)
{
	Tryll::Client::FTryllSessionCalls::EnumerateMutableKbs(Connection.Get(), this, MoveTemp(OnComplete), {});
}

void UTryllEditorSubsystem::RequestSaveGameMemory(
	const FString& Path,
	const FString& SaveId,
	TFunction<FTryllOnSaveGameMemory> OnComplete)
{
	Tryll::Client::FTryllSessionCalls::SaveGameMemory(
		Connection.Get(), this, Path, SaveId, MoveTemp(OnComplete), {});
}

void UTryllEditorSubsystem::RequestLoadGameMemory(
	const FString& Path,
	ETryllMutableKbEmbedTiming EmbedTiming,
	ETryllMutableKbCompatibility Compatibility,
	TFunction<FTryllOnLoadGameMemory> OnComplete)
{
	Tryll::Client::FTryllSessionCalls::LoadGameMemory(
		Connection.Get(), this, Path, EmbedTiming, Compatibility, MoveTemp(OnComplete), {});
}

void UTryllEditorSubsystem::RequestDownloadModel(const FString& ModelName)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DownloadModel(
		Connection.Get(), this, ModelName,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		[WeakThis](const FString& Name, uint64 Bytes, uint64 Total, float Percent)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnDownloadProgressNative.Broadcast(Name, Bytes, Total, Percent);
			}
		},
		[WeakThis](const FString& Name, bool bOk, const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnDownloadCompleteNative.Broadcast(Name, bOk, Message);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

void UTryllEditorSubsystem::RequestLoadModel(
	const FString& ModelName, ETryllMemoryPlacement Placement, bool bIsOverride)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::LoadModel(
		Connection.Get(), this, ModelName, Placement, bIsOverride,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		[WeakThis](const FString& Name, bool bOk)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnLoadModelCompleteNative.Broadcast(Name, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

void UTryllEditorSubsystem::RequestUnloadModel(const FString& ModelName)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::UnloadModel(
		Connection.Get(), this, ModelName,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		[WeakThis](const FString& Name, bool bOk)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnUnloadModelCompleteNative.Broadcast(Name, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

void UTryllEditorSubsystem::RequestDeleteModel(const FString& ModelName)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DeleteModel(
		Connection.Get(), this, ModelName,
		[WeakThis](const FString& Name, bool bOk)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnDeleteModelCompleteNative.Broadcast(Name, bOk);
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

// ── Internal registration ────────────────────────────────────────────────────────

void UTryllEditorSubsystem::RegisterAgent(std::uint64_t AgentId, TWeakPtr<FTryllAgent> Agent)
{
	if (AgentId != 0)
	{
		AgentsById.Add(AgentId, Agent);
		if (Connection.IsValid())
		{
			Connection->GetPushRouter().RegisterAgent(AgentId, Agent);
		}
	}
}

void UTryllEditorSubsystem::RegisterVoiceInput(std::uint64_t VoiceInputId, TWeakPtr<FTryllVoiceInput> Voice)
{
	if (VoiceInputId != 0)
	{
		VoiceInputsById.Add(VoiceInputId, Voice);
		if (Connection.IsValid())
		{
			Connection->GetPushRouter().RegisterVoiceInput(VoiceInputId, Voice);
		}
	}
}

void UTryllEditorSubsystem::UnregisterAgent(std::uint64_t AgentId)
{
	AgentsById.Remove(AgentId);
	if (Connection.IsValid())
	{
		Connection->GetPushRouter().UnregisterAgent(AgentId);
	}
}

TSharedPtr<FTryllAgent> UTryllEditorSubsystem::FindAgent(uint64 AgentId) const
{
	if (Connection.IsValid())
	{
		return Connection->GetPushRouter().FindAgent(AgentId);
	}
	if (const TWeakPtr<FTryllAgent>* Found = AgentsById.Find(AgentId))
	{
		return Found->Pin();
	}
	return nullptr;
}

TArray<FTryllRegisteredAgentSnapshot> UTryllEditorSubsystem::SnapshotRegisteredAgents() const
{
	TArray<TPair<uint64, TWeakPtr<FTryllAgent>>> Pairs;
	if (Connection.IsValid())
	{
		Connection->GetPushRouter().SnapshotAgentPairs(Pairs);
	}
	else
	{
		for (const TPair<std::uint64_t, TWeakPtr<FTryllAgent>>& Pair : AgentsById)
		{
			Pairs.Add(TPair<uint64, TWeakPtr<FTryllAgent>>(Pair.Key, Pair.Value));
		}
	}
	TArray<FTryllRegisteredAgentSnapshot> Out;
	Out.Reserve(Pairs.Num());
	for (const TPair<uint64, TWeakPtr<FTryllAgent>>& Pair : Pairs)
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

FTryllClientObserverHub* UTryllEditorSubsystem::GetConnectionObserverHub()
{
	return Connection.IsValid() ? &Connection->GetObserverHub() : nullptr;
}

const TSharedRef<FTryllDiagnosticsDemand>* UTryllEditorSubsystem::GetConnectionDiagnosticsDemand() const
{
	return Connection.IsValid() ? &Connection->GetDiagnosticsDemand() : nullptr;
}

// ── Event dispatch (editor tick) ──────────────────────────────────────────────────

bool UTryllEditorSubsystem::CaptureAndVerifyServerIdentity(const FTryllConnEvent& Event)
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

	const FString Intended = UTryllSubsystem::ResolveServerExePath(
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

	// The editor subsystem always owns the shared server's lifecycle (it launches
	// or reuses it), so a mismatch means a stranger raced us onto the port — fail
	// closed rather than create a session against it. Defense-in-depth backstop:
	// ProbeAndVerify already rejects a mismatched listener before deciding to
	// reuse a warm server, so this should only trip on a genuine race.
	UE_LOG(LogTryll, Error,
		TEXT("Connected server exe '%s' does not match the intended exe '%s' — a different ")
		TEXT("server instance is holding the port. Disconnecting."),
		*ServerExePath, *Intended);
	Disconnect();
	return false;
}

void UTryllEditorSubsystem::ProcessEvents()
{
	// A completion can Disconnect and drop the last reference. Hold the
	// connection until PumpEvents returns.
	const TSharedPtr<FTryllConnection> Pinned = Connection;
	if (!Pinned.IsValid())
	{
		return;
	}
	EnsureConnectionBound();
	Pinned->PumpEvents();
}

FTryllConnectionEvents* UTryllEditorSubsystem::GetConnectionEvents()
{
	return Connection.IsValid() ? &Connection->GetEvents() : nullptr;
}

void UTryllEditorSubsystem::UnbindConnection()
{
	if (FTryllConnection* Old = BoundConnection.Pin().Get())
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

void UTryllEditorSubsystem::EnsureConnectionBound()
{
	if (BoundConnection.Pin().Get() == Connection.Get())
	{
		return;
	}
	UnbindConnection();
	if (!Connection.IsValid())
	{
		return;
	}
	BoundConnection = Connection;
	FTryllConnectionEvents& Events = Connection->GetEvents();
#define TRYLL_CONNECTION_EVENT(Member, DelegateType, ForwardMethod) \
	Events.Member.AddUObject(this, &UTryllEditorSubsystem::ForwardMethod);
#include "TryllConnectionEvents.inl"
	SessionListenerId = Connection->GetPushRouter().AddSessionListener(FTryllSessionPushListener{
		[this](uint64 AgentId, const FTryllDecodedNodeEvent& Event)
		{
			HandleSessionNodeEvent(AgentId, Event);
		},
		[this]()
		{
			return HasSessionPauseListener();
		},
		[this]()
		{
			return HasSessionToolCallListener();
		}
	});
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Connection->SetStateHook([WeakThis](const FTryllConnEvent& Event) -> bool
	{
		if (UTryllEditorSubsystem* Self = WeakThis.Get())
		{
			return Self->HandleConnectionState(Event);
		}
		return true;
	});
}

bool UTryllEditorSubsystem::HandleConnectionState(const FTryllConnEvent& Event)
{
	switch (Event.Type)
	{
	case ETryllConnEventType::Connected:
		UE_LOG(LogTryll, Log, TEXT("UTryllEditorSubsystem: connection ready (no session yet — call CreateSession)"));
		// Do not clear bCreateSessionInFlight here. The socket thread sets
		// IsConnected() before this event is pumped, so a restored editor
		// window's join-late Start() can already have sent the one-shot
		// CreateSession. Resetting the guard would send a second request
		// and fail every consumer with SessionAlreadyExists.
		if (!CaptureAndVerifyServerIdentity(Event))
		{
			return false;
		}
		if (GetDefault<UTryllRuntimeSettings>()->bAutoCreateSession)
		{
			CreateSessionFromSettings();
		}
		LastOwnerError.Reset();
		OnConnectRetryStateChanged(true);
		return true;

	case ETryllConnEventType::ConnectFailed:
		UE_LOG(LogTryll, Warning, TEXT("UTryllEditorSubsystem: connect failed: %s"), *Event.Message);
		bSessionConfigured     = false;
		bCreateSessionInFlight = false;
		if (Connection.IsValid())
		{
			Connection->ResetSessionPhase();
		}
		DrainPendingRequests(FTryllError(ETryllErrorCode::ConnectionFailed, Event.Message));
		// Drive the retry budget before the public broadcast so subscribers
		// (notably the PIE model guard) can tell a retryable accept-late failure
		// from a terminal one via HasConnectRetriesRemaining().
		LastOwnerError = Event.Message;
		OnConnectRetryStateChanged(false);
		return true;

	case ETryllConnEventType::Disconnected:
		UE_LOG(LogTryll, Log, TEXT("UTryllEditorSubsystem: disconnected"));
		bSessionConfigured     = false;
		bCreateSessionInFlight = false;
		LastOwnerError = Event.Message.IsEmpty() ? FString(TEXT("Connection dropped")) : Event.Message;
		DrainPendingRequests(FTryllError(ETryllErrorCode::ConnectionDropped, LastOwnerError));
		InvalidateConnectionEpoch(/*bNotifyDisconnected=*/false);
		return true;

	default:
		return true;
	}
}

void UTryllEditorSubsystem::HandleSessionNodeEvent(uint64 AgentId, const FTryllDecodedNodeEvent& Decoded)
{
	bool bDispatchedTyped = false;
	switch (Decoded.Kind)
	{
	case ETryllNodeEventKind::ToolCall:
		if (OnToolCallDetailedNative.IsBound())
		{
			OnToolCallDetailedNative.Broadcast(AgentId, Decoded.ToolCall);
		}
		if (OnToolCallWithIdNative.IsBound())
		{
			OnToolCallWithIdNative.Broadcast(
				AgentId,
				Decoded.ToolCall.CallId,
				Decoded.ToolCall.ToolName,
				Decoded.ToolCall.ArgumentsJson);
			bDispatchedTyped = true;
		}
		if (OnToolCallNative.IsBound())
		{
			OnToolCallNative.Broadcast(AgentId, Decoded.ToolCall.ToolName, Decoded.ToolCall.ArgumentsJson);
			bDispatchedTyped = true;
		}
		break;

	case ETryllNodeEventKind::IntentClassified:
		if (OnIntentClassifiedNative.IsBound())
		{
			OnIntentClassifiedNative.Broadcast(AgentId, Decoded.Intent);
			bDispatchedTyped = true;
		}
		break;

	case ETryllNodeEventKind::IntentLlmClassified:
		if (OnIntentLlmClassifiedNative.IsBound())
		{
			OnIntentLlmClassifiedNative.Broadcast(AgentId, Decoded.IntentLlm);
			bDispatchedTyped = true;
		}
		break;

	case ETryllNodeEventKind::Paused:
		if (OnPausedNative.IsBound())
		{
			OnPausedNative.Broadcast(AgentId, Decoded.NodeName, Decoded.PendingExit);
		}
		if (HasSessionPauseListener())
		{
			bDispatchedTyped = true;
		}
		break;

	default:
		break;
	}

	if (!bDispatchedTyped && OnNodeEventNative.IsBound())
	{
		OnNodeEventNative.Broadcast(AgentId, Decoded.NodeName, Decoded.EventType, Decoded.KvPairs);
	}
}

void UTryllEditorSubsystem::ForwardConnectionChanged(bool bConnected)
{
	OnConnectionChangedNative.Broadcast(bConnected);
	RefreshOwnerPhase();
}

void UTryllEditorSubsystem::ForwardError(const FString& ErrorMessage)
{
	OnErrorNative.Broadcast(ErrorMessage);
}

void UTryllEditorSubsystem::ForwardAgentDestroyed(uint64 AgentId)
{
	OnAgentDestroyedNative.Broadcast(AgentId);
}

void UTryllEditorSubsystem::ForwardCreateSession(bool bSuccess)
{
	bSessionConfigured = bSuccess;
	bCreateSessionInFlight = false;
	if (bSuccess)
	{
		LastOwnerError.Reset();
	}
	OnCreateSessionCompleteNative.Broadcast(bSuccess);
	RefreshOwnerPhase();
}

void UTryllEditorSubsystem::ForwardSetSessionLocale(const FString& Locale, bool bSuccess)
{
	OnSetSessionLocaleCompleteNative.Broadcast(Locale, bSuccess);
}

void UTryllEditorSubsystem::ForwardListModels(const TArray<FTryllModelInfo>& Models, bool bSuccess)
{
	OnListModelsCompleteNative.Broadcast(Models, bSuccess);
}

void UTryllEditorSubsystem::ForwardDownloadProgress(const FString& ModelName, uint64 BytesDownloaded, uint64 TotalBytes, float Percent)
{
	OnDownloadProgressNative.Broadcast(ModelName, BytesDownloaded, TotalBytes, Percent);
}

void UTryllEditorSubsystem::ForwardDownloadComplete(const FString& ModelName, bool bSuccess, const FString& ErrorMessage)
{
	OnDownloadCompleteNative.Broadcast(ModelName, bSuccess, ErrorMessage);
}

void UTryllEditorSubsystem::ForwardLoadModel(const FString& ModelName, bool bSuccess)
{
	OnLoadModelCompleteNative.Broadcast(ModelName, bSuccess);
}

void UTryllEditorSubsystem::ForwardUnloadModel(const FString& ModelName, bool bSuccess)
{
	OnUnloadModelCompleteNative.Broadcast(ModelName, bSuccess);
}

void UTryllEditorSubsystem::ForwardCreateStringStorage(const FString& Name, bool bSuccess)
{
	OnCreateStringStorageCompleteNative.Broadcast(Name, bSuccess);
}

void UTryllEditorSubsystem::ForwardDestroyStringStorage(const FString& Name, bool bSuccess)
{
	OnDestroyStringStorageCompleteNative.Broadcast(Name, bSuccess);
}

void UTryllEditorSubsystem::ForwardCreateEmbedded(const FString& Name, int32 RecordCount, bool bSuccess)
{
	OnCreateEmbeddedStringStorageCompleteNative.Broadcast(Name, RecordCount, bSuccess);
}

void UTryllEditorSubsystem::ForwardDestroyEmbedded(const FString& Name, bool bSuccess)
{
	OnDestroyEmbeddedStringStorageCompleteNative.Broadcast(Name, bSuccess);
}

void UTryllEditorSubsystem::RequestCreateStringStorage(
	const FString& Name,
	const TArray<FString>& Strings,
	TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::CreateStringStorage(
		Connection.Get(), this, Name, static_cast<uint8>(::Tryll::StringStorageKind_List), Strings, TArray<FString>{}, FString{},
		MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		{},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

void UTryllEditorSubsystem::RequestCreateStringStorageFromFile(
	const FString& Name,
	const FString& FilePath,
	uint8          Kind,
	TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::CreateStringStorage(
		Connection.Get(), this, Name, Kind, TArray<FString>{}, TArray<FString>{}, FilePath, MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		{},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

void UTryllEditorSubsystem::RequestCreateStringStorageKeyed(
	const FString&         Name,
	const TArray<FString>& Keys,
	const TArray<FString>& Values,
	uint8                  Kind,
	TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::CreateStringStorage(
		Connection.Get(), this, Name, Kind, Values, Keys, FString{},
		MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		{},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

void UTryllEditorSubsystem::RequestDestroyStringStorage(
	const FString& Name,
	TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DestroyStringStorage(
		Connection.Get(), this, Name, MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		{},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

void UTryllEditorSubsystem::RequestCreateEmbeddedStringStorage(
	const FString& Name,
	const FString& ConfigPath,
	const FString& EmbeddingModel,
	TFunction<UTryllSubsystem::FTryllOnEmbeddedStorageCreated> OnComplete)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::CreateEmbeddedStringStorage(
		Connection.Get(), this, Name, ConfigPath, TArray<FString>{}, EmbeddingModel, MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		{},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

void UTryllEditorSubsystem::RequestCreateEmbeddedStringStorageFromStrings(
	const FString& Name,
	const TArray<FString>& Strings,
	const FString& EmbeddingModel,
	TFunction<UTryllSubsystem::FTryllOnEmbeddedStorageCreated> OnComplete)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::CreateEmbeddedStringStorage(
		Connection.Get(), this, Name, FString{}, Strings, EmbeddingModel, MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		{},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

void UTryllEditorSubsystem::RequestDestroyEmbeddedStringStorage(
	const FString& Name,
	TFunction<UTryllSubsystem::FTryllOnEmbeddedStorageDestroyed> OnComplete)
{
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DestroyEmbeddedStringStorage(
		Connection.Get(), this, Name, MoveTemp(OnComplete),
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		{},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->OnErrorNative.Broadcast(Message);
			}
		});
}

// ── VoiceInput (STT) lifecycle ───────────────────────────────────────────────────

void UTryllEditorSubsystem::RequestCreateVoiceInput(
	const FTryllVoiceInputConfig&        Config,
	TFunction<FTryllOnVoiceInputCreated> OnComplete)
{
	if (!Connection.IsValid())
	{
		OnErrorNative.Broadcast(TEXT("No connection"));
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
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::Calls::CreateVoiceInput(
		*Connection,
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
			FTryllError Result = Error;
			if (Error.IsOk() && Resp)
			{
				if (UTryllEditorSubsystem* Self = WeakThis.Get())
				{
					if (Self->Connection.IsValid())
					{
						Voice = MakeShared<FTryllVoiceInput>(Self->Connection, Resp->voice_input_id);
						if (Resp->voice_input_id != 0)
						{
							Self->RegisterVoiceInput(Resp->voice_input_id, Voice);
						}
					}
				}
			}
			else if (Error.IsOk())
			{
				Result = FTryllError(ETryllErrorCode::ConnectionFailed, TEXT("CreateVoiceInput failed"));
			}
			if (OnComplete)
			{
				OnComplete(Voice, Result);
			}
		},
		{});
}

void UTryllEditorSubsystem::RequestDestroyVoiceInput(TSharedPtr<FTryllVoiceInput> Voice)
{
	if (!Connection.IsValid() || !Voice.IsValid())
	{
		return;
	}
	const std::uint64_t Vid = Voice->GetVoiceInputId();
	if (Vid == 0)
	{
		return;
	}
	TWeakObjectPtr<UTryllEditorSubsystem> WeakThis(this);
	Tryll::Client::FTryllSessionCalls::DestroyVoiceInput(
		Connection.Get(), this, Vid,
		[WeakThis]() -> FTryllConnectionEvents*
		{
			UTryllEditorSubsystem* Self = WeakThis.Get();
			if (!Self || !Self->Connection.IsValid())
			{
				return nullptr;
			}
			return &Self->Connection->GetEvents();
		},
		[WeakThis](uint64 Id)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				Self->VoiceInputsById.Remove(Id);
				if (Self->Connection.IsValid())
				{
					Self->Connection->GetPushRouter().UnregisterVoiceInput(Id);
				}
			}
		},
		[WeakThis](const FString& Message)
		{
			if (UTryllEditorSubsystem* Self = WeakThis.Get())
			{
				UE_LOG(LogTryll, Warning, TEXT("UTryllEditorSubsystem: DestroyVoiceInput error: %s"), *Message);
				Self->OnErrorNative.Broadcast(Message);
			}
		});
	Voice->ClearAfterDestroyRequest();
}

int32 UTryllEditorSubsystem::RegisterPauseListenerProbe(TFunction<bool()> Probe)
{
	FPauseListenerProbe Entry;
	Entry.Id = NextPauseListenerProbeId++;
	Entry.IsBound = MoveTemp(Probe);
	const int32 Id = Entry.Id;
	PauseListenerProbes.Add(MoveTemp(Entry));
	return Id;
}

void UTryllEditorSubsystem::UnregisterPauseListenerProbe(int32 ProbeId)
{
	PauseListenerProbes.RemoveAll([ProbeId](const FPauseListenerProbe& Entry)
	{
		return Entry.Id == ProbeId;
	});
}

bool UTryllEditorSubsystem::HasSessionPauseListener() const
{
	// OnPausedNative is direct listeners only. The PIE forward channel is excluded
	// on purpose: UTryllSubsystem always binds it.
	if (OnPausedNative.IsBound())
	{
		return true;
	}
	for (const FPauseListenerProbe& Probe : PauseListenerProbes)
	{
		if (Probe.IsBound && Probe.IsBound())
		{
			return true;
		}
	}
	return false;
}

int32 UTryllEditorSubsystem::RegisterToolCallListenerProbe(TFunction<bool()> Probe)
{
	FPauseListenerProbe Entry;
	Entry.Id = NextToolCallListenerProbeId++;
	Entry.IsBound = MoveTemp(Probe);
	const int32 Id = Entry.Id;
	ToolCallListenerProbes.Add(MoveTemp(Entry));
	return Id;
}

void UTryllEditorSubsystem::UnregisterToolCallListenerProbe(int32 ProbeId)
{
	ToolCallListenerProbes.RemoveAll([ProbeId](const FPauseListenerProbe& Entry)
	{
		return Entry.Id == ProbeId;
	});
}

bool UTryllEditorSubsystem::HasSessionToolCallListener() const
{
	// Probes only: the OnToolCall*Native delegates are the PIE forward channels
	// and every delegating game subsystem subscribes to them.
	for (const FPauseListenerProbe& Probe : ToolCallListenerProbes)
	{
		if (Probe.IsBound && Probe.IsBound())
		{
			return true;
		}
	}
	return false;
}
