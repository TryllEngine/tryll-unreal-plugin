// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TryllGraphDescription.h"
#include "TryllRuntimeSettings.generated.h"

class UTryllTextProvider;

/** Bundled server build to launch — the Default / Release subfolder of TryllServer/. */
UENUM()
enum class EServerBuildVariant : uint8
{
	Default,
	Release,
};

/**
 * llama.cpp GPU plugin preference passed to an auto-launched server as
 * --llama-backend. Vulkan is the portable default. Auto prefers CUDA on
 * NVIDIA when the CUDA files were staged, otherwise Vulkan.
 * Ignored when bAutoLaunchServer is false.
 */
UENUM()
enum class ETryllLlamaCppBackend : uint8
{
	Vulkan,
	Auto,
};

/** Detail an editor server sends. Off is command-line only and is not a project setting. */
UENUM()
enum class ETryllTelemetryLevel : uint8
{
	Basic UMETA(DisplayName = "Basic"),
	Turns UMETA(DisplayName = "Turns"),
	Full  UMETA(DisplayName = "Full"),
};

/**
 * Project-level settings for the Tryll Client plugin.
 * Edit in Project Settings → Plugins → Tryll Client.
 * Persisted to <YourProject>/Config/DefaultGame.ini under
 * [/Script/TryllClient.TryllRuntimeSettings].
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Tryll Client"))
class TRYLLCLIENT_API UTryllRuntimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** Spawn the bundled tryll_server.exe on Initialize. Set false to run a server yourself. */
	UPROPERTY(EditAnywhere, Config, Category = "Server")
	bool bAutoLaunchServer = true;

	/**
	 * Connect to the server automatically once the subsystem initializes. The
	 * built-in connect-retry (ConnectMaxAttempts / ConnectRetryDelaySeconds) covers
	 * the brief window while the launched server comes up. Turn off to call
	 * Connect() yourself (e.g. from a loading screen).
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Server")
	bool bAutoConnect = true;

	/**
	 * Which bundled server build to launch. Only honored when both Default and Release
	 * variants are present; otherwise the single present variant is always used. The editor
	 * hides this unless both resolve.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Server")
	EServerBuildVariant ServerBuildVariant = EServerBuildVariant::Default;

	/**
	 * llama.cpp GPU plugin for an auto-launched server. Vulkan (default) is
	 * portable and skips CUDA DLLs in a packaged build. Auto prefers CUDA on
	 * NVIDIA when those DLLs were staged. Passed as --llama-backend; ignored
	 * when bAutoLaunchServer is false.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Server")
	ETryllLlamaCppBackend LlamaCppBackend = ETryllLlamaCppBackend::Vulkan;

	/** CLI value for --llama-backend ("vulkan" or "auto"). */
	FString LlamaCppBackendCliValue() const
	{
		return LlamaCppBackend == ETryllLlamaCppBackend::Auto ? TEXT("auto") : TEXT("vulkan");
	}

	/**
	 * Known port for the shared editor server. Every in-editor launcher (PIE, the
	 * chat panel, the model manager panel) probes this port before spawning and
	 * reuses whatever it finds there, so one warm server survives across PIE
	 * sessions and multiple open tool windows instead of each spawning its own.
	 * Packaged/player builds ignore this and always pick a free ephemeral port.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Server", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 ServerPort = 9100;

	/**
	 * Seconds the launcher waits *synchronously* for a freshly spawned server's port
	 * to start accepting before it stops blocking the game thread and hands off to the
	 * asynchronous connect path. Kept short deliberately: a cold start (DLL paging +
	 * first-run antivirus scan of the freshly built exe) can take much longer, and the
	 * server is *warming*, not dead — so on timeout the launcher no longer kills it; the
	 * connection worker keeps retrying (see ConnectMaxAttempts) until the server is ready.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Server", meta = (ClampMin = "0.0"))
	double ServerReadinessTimeoutSeconds = 5.0;

	/**
	 * Editor-only. When true (default), the Play-In-Editor model guard verifies that every
	 * registered required model is downloaded before Play starts — it briefly vetoes Play,
	 * connects to the server, downloads anything missing (with a progress toast), then resumes
	 * Play automatically. Turn this off to enter Play immediately with no pre-flight check
	 * (e.g. when you provision models yourself, or to bypass the guard while debugging). No
	 * effect in packaged builds, where the guard does not exist.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Editor")
	bool bEnablePlayModeModelGuard = true;

	/**
	 * Configure the session automatically once connected, using the engine fields
	 * below. Mirrors bAutoLaunchServer, but for session setup: leave on for the
	 * simple "it just works" path; turn off to configure the session yourself
	 * (e.g. during a loading screen, or to run multiple sessions with different
	 * engines).
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Session")
	bool bAutoCreateSession = true;

	// There are deliberately no engine properties here any more. Through protocol
	// v12 this carried four (LLM / STT / TTS / embedding), three defaulting to
	// Mock — a question a game developer cannot reasonably answer, whose wrong
	// answer shipped mock engines to production, and which the server can answer
	// itself: the model catalog already states which engine runs each model.
	//
	// The engine set is derived from the models registered in UTryllModelManifest.

	/**
	 * The (role, engine) ordinal pairs this project expects to use, derived from the
	 * registered models in UTryllModelManifest and deduped.
	 *
	 * Advisory. It lets the server initialise those engines' backends at
	 * CreateSession rather than at first use; a model's engine always comes from the
	 * server's catalog, so nothing here gates a load. Empty when nothing is
	 * registered, which is legal and is exactly what a fresh project sends.
	 *
	 * Derived rather than configured, so it cannot drift from the models the project
	 * actually references — and there is deliberately no engine control anywhere in
	 * the editor, because which C++ library runs a model is not a question a game
	 * developer can answer.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Models")
	static TArray<int32> BuildEngineDeclarationsFlat();

	/** Non-Blueprint form: (ModelRole, InferenceEngine) ordinal pairs. */
	static TArray<TPair<uint8, uint8>> BuildEngineDeclarations();

	/**
	 * Per-model placement hints from the manifest, omitting models left on Auto —
	 * the hint list is sparse by design.
	 */
	static TArray<TPair<FString, uint8>> BuildPlacementHints();

	/**
	 * Per-model hotword-bias hints from the manifest, omitting models left on
	 * Default — sparse for the same reason placement is, and because Default and
	 * "absent" mean the same thing to the server.
	 */
	static TArray<TPair<FString, uint8>> BuildHotwordHints();

	/**
	 * Integration identifier sent to the server on every session for telemetry grouping.
	 * Use a short, stable, lowercase slug — e.g. "my-game" or "tryll-roleplay-demo".
	 * Leave empty to omit (server defaults to "unknown" when telemetry is enabled).
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Telemetry")
	FString GameName;

	/**
	 * Detail the editor server sends. Basic is the default. Turns and Full are for
	 * debugging. Passed as --telemetry-level on the next client-dev launch. The running
	 * server keeps its current level until that launch. Packaged players do not send the flag.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Telemetry", meta = (
		DisplayName = "Telemetry Level",
		ToolTip = "Basic (default): hardware, session, graph shape, and lifetime stats. No conversation text. Turns: adds per-turn timings and counts. No conversation text. Full: adds prompts, outputs, and error text. The running server keeps its current level until the next launch."))
	ETryllTelemetryLevel TelemetryLevel = ETryllTelemetryLevel::Basic;

	/** Hidden 32-hex project lineage id. Not shown in settings. */
	UPROPERTY(Config)
	FString ProjectId;

	/**
	 * Process-wide storage root for knowledge bases, voices, guardrails, and models.
	 * Must sit inside the project and outside Content/ — Unreal packs Content/ into
	 * .pak archives the server cannot read. Default is TryllStorage next to the .uproject.
	 *
	 * Editor: relative to FPaths::ProjectDir(). Packaged: always
	 * <packaged project dir>/TryllStorage/ (staged NonUFS by TryllClient.Build.cs).
	 * An absolute path is kept as-is. Empty omits --storage-root and the server uses
	 * its mode default (<project-dir>/TryllStorage in client-dev).
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Storage")
	FString StorageDataFolder = TEXT("TryllStorage");

	/**
	 * Folder an auto-launched sidecar passes as --save-root for SaveGameMemory /
	 * SaveDialog relative paths. Empty in the editor omits the flag (relative
	 * saves stay under the session storage root). Empty in a packaged build uses
	 * ProjectSavedDir().
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Storage")
	FString SaveDataFolder;

	/**
	 * Optional project-relative path to a custom models catalog JSON. Any file
	 * name. Empty means this project adds no extra models.
	 *
	 * Stored relative to the project directory, for example
	 * "Content/Tryll/my-models.json". An auto-launched server receives the
	 * resolved file as --user-models-path. A path outside the project is
	 * rejected. Leave empty when you are not adding models.
	 *
	 * The first auto-launch runs from UTryllSubsystem::Initialize, before
	 * BeginPlay. Set the path from your GameInstance Init, or from Project
	 * Settings, so that first process sees the file. SetUserModelsCatalogPath
	 * while a server is already running restarts it; an open session is dropped.
	 *
	 * A packaged build copies the catalog saved here to
	 * Server/data/user-catalog/ and rewrites relative chat_template_file values
	 * so they sit beside that copy. A different file chosen from code in a
	 * packaged game is used only when it is already a loose file inside the
	 * project directory.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Models",
		meta = (RelativeToGameDir, FilePathFilter = "json"))
	FString UserModelsCatalogPath;

	/**
	 * Session language as a BCP-47 tag ("de", "de-DE", "pt-BR"). One setting the whole
	 * voice + text stack derives from, so a language menu does not have to reach into
	 * every agent:
	 *   - TTS   : the default for the agent voice's Lang (a voice profile that sets
	 *             its own Lang still wins).
	 *   - STT   : language hint for multilingual recognizers (Whisper, Canary).
	 *   - Files : node storage paths prefer loc/<locale>/ siblings when they exist,
	 *             so localized knowledge bases and canned lines are a folder layout
	 *             rather than code.
	 *   - Text  : templates can read the `language` key to write a "reply in X"
	 *             instruction.
	 *
	 * Empty leaves every one of those at its pre-locale behaviour. This is the
	 * value used at session start; to change language later call
	 * UTryllSubsystem::SetSessionLocale, which reaches existing agents' voice and template
	 * keys but not content already bound to a node.
	 * FInternationalization::Get().GetCurrentCulture()->GetName() is the usual source.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Localization",
		meta = (GetOptions = "GetLocaleOptions"))
	FString Locale = TEXT("en");

	/**
	 * Resolves Text IDs on Localized prompt fields. None (the default): Localized
	 * fields cannot be used and Locale above is the session language.
	 * Set (for example to TryllStringTableTextProvider, which reads Unreal string
	 * tables and follows the game culture): its current language becomes the
	 * session language, ahead of Locale, unless CreateSession is given a locale.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Localization",
		meta = (ToolTip = "Resolves Text IDs on Localized prompt fields. None = no Localized prompts; the Locale field is the session language. When set, the provider's language wins over Locale unless CreateSession is given a locale."))
	TSubclassOf<UTryllTextProvider> TextProviderClass;

	/**
	 * Choices for the Locale dropdown: the languages the project's *registered*
	 * models declare (UTryllModelManifest), never a hardcoded list - so the menu
	 * can only ever offer something the project can actually load. Plus an empty
	 * entry, because "no session language" is a meaningful choice rather than a
	 * missing value.
	 *
	 * A **union** across registered models, not an intersection: registering an
	 * English-only model for one character must not collapse the whole menu to
	 * English. Per-model mismatches surface on the model picker and at agent
	 * creation, where they are actionable.
	 *
	 * **Fallback, not the primary path.** In the editor, FTryllRuntimeSettingsCustomization
	 * replaces this row with a combo that annotates each entry with a catalog
	 * model count ("German (de) - 7 models") while still storing the bare code.
	 * GetOptions cannot do that: it writes the *selected string* into the
	 * property, so an annotated entry would be stored as the locale. This stays
	 * for any surface that renders the property without that customization.
	 */
	UFUNCTION()
	TArray<FString> GetLocaleOptions() const;


	/**
	 * Absolute storage root for the next sidecar spawn / editor file picking.
	 * Packaged builds always return ProjectDir/TryllStorage (where Build.cs stages it).
	 * Empty when StorageDataFolder is unset in the editor.
	 */
	FString ResolveStorageDataFolderForServer() const;

	/** Absolute --save-root for the next sidecar spawn, or empty to omit the flag. */
	FString ResolveSaveDataFolderForLaunch() const;

	/**
	 * Telemetry / project identity: configured GameName, else FApp::GetProjectName().
	 * Passed as --project-name and used as the empty game_name fallback on the server.
	 */
	FString ResolveProjectName() const;

	/** Generate and persist a project id in the default config when it is missing. */
	FString EnsureProjectId();

	/** True when ConfiguredFolder resolves under the project's Content/ directory. */
	static bool IsStorageUnderContent(const FString& ConfiguredFolder);

	/**
	 * Move the configured storage folder to <Project>/TryllStorage and update the
	 * setting. Used by the settings "Move to TryllStorage" action when the path is
	 * under Content/. Returns false and leaves the setting unchanged on failure.
	 */
	static bool MoveStorageToTryllStorage(FString& OutError);

	/**
	 * Store RelativePath as this project's custom models catalog and save it.
	 * Empty clears the setting. An absolute path is kept only when it sits
	 * inside the project, and is stored relative. Returns false and leaves the
	 * setting unchanged when the path is outside the project.
	 *
	 * When auto-launch is on and a server is already running, the process is
	 * restarted and an open session is dropped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll")
	static bool SetUserModelsCatalogPath(const FString& RelativePath, UPARAM(ref) FString& OutError);

	/**
	 * After a launch input changed, restart a server this plugin already launched
	 * and move an open connection to it. No-op when auto-launch is off or nothing has
	 * launched yet (the next launch reads the new input). Reason prefixes the warning
	 * logged when the restart fails.
	 */
	static void RelaunchRunningServer(const FString& Reason);

	/**
	 * Collapse Configured against ProjectDir into a project-relative path.
	 * Empty is valid. Fails when the path resolves outside the project.
	 */
	static bool TryNormalizeUserModelsCatalogPath(
		const FString& ProjectDir,
		const FString& Configured,
		FString& OutRelative,
		FString& OutError);

	/**
	 * Absolute catalog file for the next sidecar spawn, or empty when unset or
	 * outside the project. Editor: the project file. Packaged: the copy staged
	 * at Server/data/user-catalog/ when that file exists.
	 */
	FString ResolveUserModelsCatalogForLaunch() const;

	/**
	 * Project-relative configured folder → absolute path. Relative paths resolve against
	 * FPaths::ProjectDir() in both editor and packaged builds (not the exe BaseDir).
	 */
	static FString ResolveStorageDataFolderForServer(const FString& ConfiguredFolder);

	/** Configured StorageDataFolder from DefaultGame.ini (preferred) or the settings CDO. */
	static FString GetConfiguredStorageDataFolder();

	/** Absolute storage root for editor file picking; project dir when unset. */
	static FString GetStorageRootAbsolute();

	/** Picked absolute file → path relative to the resolved storage root (forward slashes). */
	static bool MakePathRelativeToStorageRoot(const FString& AbsoluteFilePath, FString& OutRelativePath);

	/**
	 * Strips accidental parent-folder prefixes (e.g. "TryllStorage/RAG/x" when the
	 * storage root is already TryllStorage). Paths are relative to storage_root.
	 */
	static FString NormalizeStorageRelativePath(const FString& RelativePath);

	/**
	 * Max number of attempts when establishing a session via UTryllSubsystem::Connect.
	 * 1 disables retry. The first call counts as attempt 1.
	 *
	 * Sized as a cold-start backstop: after the launcher hands off a still-warming
	 * server (see ServerReadinessTimeoutSeconds), the worker retries connect+hello
	 * until the server is ready. ConnectMaxAttempts x (ConnectionReadyTimeoutSeconds +
	 * ConnectRetryDelaySeconds) must comfortably exceed a realistic cold start; it then
	 * bounds the failure (a clear error) instead of retrying forever.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Connection", meta = (ClampMin = "1"))
	int32 ConnectMaxAttempts = 20;

	/** Constant delay between connect attempts. */
	UPROPERTY(EditAnywhere, Config, Category = "Connection", meta = (ClampMin = "0.0"))
	float ConnectRetryDelaySeconds = 1.0f;

	/**
	 * Per-attempt bound on how long the connection worker waits for the server's
	 * ConnectionReady hello after the TCP connect succeeds. Without this the read
	 * was unbounded: a server that accepts the socket but has not finished starting
	 * (cold-start "ConnectionReady-late") would hang the worker forever. On timeout
	 * the worker reports ConnectFailed, which consumes one attempt and re-drives the
	 * connect via the retry budget above.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Connection", meta = (ClampMin = "0.1"))
	double ConnectionReadyTimeoutSeconds = 10.0;
};
