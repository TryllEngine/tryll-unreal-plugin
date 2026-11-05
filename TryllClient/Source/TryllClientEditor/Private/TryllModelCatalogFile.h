// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllModelInfo.h"

/**
 * Reads the bundled server catalog
 * (<Plugin>/Binaries/ThirdParty/TryllServer/<Variant>/data/models.json) plus
 * the optional project catalog from Project Settings, and optional user
 * drop-ins under data/models.d/*.json, so editor UI can answer
 * catalog questions with no server running and nothing registered yet.
 *
 * Merge rules match the server: base/earlier wins on duplicate model names; a
 * malformed user file is skipped and the rest still load.
 *
 * A *complement* to the two live sources, not a replacement:
 *   - ListModels             - authoritative, needs a connected server.
 *   - UTryllModelManifest    - what this project registered, cached at
 *                              registration time.
 *   - This                   - everything the shipped + user catalogs *could*
 *                              offer, registered or not, downloaded or not.
 *
 * Used for the "how many models can serve this language" counts in the language
 * picker, where the useful number is the catalog's rather than the project's: it
 * answers "could I register something for Japanese?" rather than "did I already?".
 *
 * Results are cached and re-parsed only when any layer's timestamp changes -
 * details panels repaint constantly and must not re-read JSON each time.
 *
 * Unreal counterpart to Unity's TryllModelCatalogFile.
 */
class TRYLLCLIENTEDITOR_API FTryllModelCatalogFile
{
public:
	/**
	 * Number of catalog models declaring each language, keyed by lowercase ISO
	 * 639-1 code and counted per *model* (not per variant).
	 *
	 * Empty when the catalog cannot be found or parsed - callers must degrade to
	 * showing no counts rather than reporting zeros, which would read as
	 * "nothing supports this".
	 */
	static const TMap<FString, int32>& CountsByLanguage();

	/** Total models in the merged catalog; 0 when it could not be read. */
	static int32 ModelCount();

	/** True when a catalog was found and parsed. */
	static bool IsAvailable();

	/**
	 * Catalog tts_capabilities of a TTS model (first variant that declares them),
	 * or null when the model is unknown or declares none. Drives the voice-profile
	 * Details panel with no server running. The pointer is valid until the next call.
	 */
	static const FTryllTtsCapabilities* FindTtsCapabilities(const FString& ModelName);

	/**
	 * Catalog licence_url of the Engine variant of a model, else of its first variant
	 * that declares one. Empty when the catalog is unavailable or the model declares
	 * none (typical for user-catalog entries). ListModels does not carry licences.
	 */
	static FString FindLicenceUrl(const FString& ModelName, const FString& Engine);

private:
	static void Refresh();
	static FString LicenceKey(const FString& ModelName, const FString& Engine);

	static TArray<FString>         CachedPaths;
	static TArray<FDateTime>       CachedStamps;
	static TMap<FString, int32>    Counts;
	static TMap<FString, FTryllTtsCapabilities> TtsCaps;
	// Keyed by LicenceKey(Name, Engine), plus LicenceKey(Name, "") for the first
	// variant that declares one - the fallback when the engine has no entry.
	static TMap<FString, FString> LicenceUrls;
	static int32                   TotalModels;
	static bool                    bParsed;
};
