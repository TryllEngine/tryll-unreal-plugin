// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TryllGraphDescription.h"   // ETryllInferenceEngine
#include "TryllModelManifest.generated.h"

/**
 * Per-model build selection for this project. Ordinals match the Unity client's
 * TryllModelRegistration so a shared manifest stays interchangeable.
 */
/**
 * Whether this project wants hotword biasing on an STT model. Mirrors the wire
 * HotwordBias enum.
 *
 * Unlike ETryllMemoryPlacement this is not a hint the server can quietly
 * downgrade. Biasing requires beam-search decoding, which a recognizer fixes when
 * it is constructed, so the answer decides *which* recognizer a session is served
 * — and two sessions that disagree about one model hold two of them.
 */
UENUM()
enum class ETryllHotwordBias : uint8
{
	/** Follow the server catalog: biased when that model's stt_hotwords_score is
	 *  non-zero. The zero value, so a manifest written before this field existed
	 *  keeps its pre-existing behaviour. */
	Default = 0 UMETA(DisplayName = "Default"),
	/** Never bias. Supplying a hotwords lexicon then fails with
	 *  HotwordBiasDisabled (4106) rather than biasing nothing. */
	Off = 1 UMETA(DisplayName = "Off"),
	/** Always bias, lexicon or not. Beam-search decoding is then in force for
	 *  every session on this model, costing a few percent of decode time and
	 *  changing its transcripts. */
	On = 2 UMETA(DisplayName = "On"),
};

/**
 * Where a model's weights should live. Mirrors the wire MemoryPlacement enum.
 *
 * Always a hint: a machine that cannot honour it loads the model the other way
 * and reports what it did. That matters here because this value is authored on a
 * developer's machine and shipped to players' machines — a VRAM choice that suits
 * a 24 GB card is not a promise an 8 GB card can keep.
 *
 * Which GPU API provides VRAM (CUDA / Vulkan) is deliberately not expressible:
 * that is the host's decision, not the project's.
 */
UENUM()
enum class ETryllMemoryPlacement : uint8
{
	/** Server policy decides: device memory when the machine has a usable GPU. */
	Auto = 0,
	/** System memory (RAM). */
	Ram  = 1,
	/** Device memory (VRAM). */
	Vram = 2,
};

UENUM()
enum class ETryllModelRegistration : uint8
{
	/** Not part of this project's build (the default for unlisted models). */
	None         = 0,
	/** Usable in the editor, but stripped from the packaged build. */
	Experimental = 1,
	/** Shipped in the packaged build. */
	Production   = 2,
};

/** One registered model: its name, type, and build tier. */
USTRUCT()
struct FTryllModelManifestEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category = "Models")
	FString Name;

	/** "language" | "embedding" | "stt" | "tts" | "vad" — for type-filtered pickers. */
	UPROPERTY(EditAnywhere, Config, Category = "Models")
	FString ModelType;

	UPROPERTY(EditAnywhere, Config, Category = "Models")
	ETryllModelRegistration Registration = ETryllModelRegistration::None;

	/**
	 * Engine that runs this model, cached from the catalog when it was registered.
	 *
	 * Cached rather than read live because the session's engine set is built from
	 * this manifest at CreateSession, before any ListModels round trip could
	 * answer — an editor that could only build a session while connected would be
	 * worse than none. Defaults to LlamaCpp, not Mock: Mock is the enum's zero
	 * value and a production server refuses it.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Models")
	ETryllInferenceEngine Engine = ETryllInferenceEngine::LlamaCpp;

	/** Whether this project wants hotword biasing on this STT model. See
	 *  ETryllHotwordBias. Unlike Placement this is not a downgradable hint: it
	 *  selects which recognizer a session is served. */
	UPROPERTY(config, EditAnywhere, Category = "Tryll")
	ETryllHotwordBias Hotwords = ETryllHotwordBias::Default;

	/** Where this model should live. See ETryllMemoryPlacement. */
	UPROPERTY(EditAnywhere, Config, Category = "Models")
	ETryllMemoryPlacement Placement = ETryllMemoryPlacement::Auto;

	/**
	 * BCP-47 primary language subtags this model declares, cached from the catalog.
	 *
	 * The one piece of catalog metadata cached here rather than read live.
	 * Everything else can wait for a ListModels round trip, but the settings
	 * language dropdown and the model-picker warnings have to work with no server
	 * running - an editor that can only validate languages while connected would
	 * be worse than no validation at all. Written by the Model Manager from the
	 * live FTryllModelInfo at the moment the model is registered.
	 *
	 * Empty means undeclared, not "supports nothing".
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Models")
	TArray<FString> Languages;
};

/**
 * Project-wide manifest of which models ship in the build. Stores a thin
 * name -> { type, registration, languages } list; the rest of the catalog stays
 * server-owned and is read via ListModels. The Model Manager window writes it;
 * component pickers and a future packaging step read it.
 *
 * Persisted to <YourProject>/Config/DefaultGame.ini under
 * [/Script/TryllClient.TryllModelManifest].
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Tryll Model Manifest"))
class TRYLLCLIENT_API UTryllModelManifest : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** Models the project has registered. Unlisted models are None. */
	UPROPERTY(EditAnywhere, Config, Category = "Models")
	TArray<FTryllModelManifestEntry> Entries;

	/** Registration for a model, or None if unlisted. */
	ETryllModelRegistration Get(const FString& ModelName) const
	{
		for (const FTryllModelManifestEntry& E : Entries)
			if (E.Name == ModelName) return E.Registration;
		return ETryllModelRegistration::None;
	}

	/**
	 * Set (or clear) a model's registration. Setting None removes the entry so the
	 * manifest only holds models the project actually selects. Does not persist;
	 * callers invoke TryUpdateDefaultConfigFile() to write DefaultGame.ini.
	 */
	/// Returns true when this call actually changed the manifest, so a caller knows
	/// whether to write DefaultGame.ini.
	///
	/// Reported from here rather than compared by the caller, and that is the point.
	/// Callers used to decide for themselves — the Model Manager's catalog refresh skipped
	/// the whole call whenever cached *languages* matched the server, which meant a stale
	/// cached *engine* was never corrected. Every manifest written before the engine field
	/// existed therefore kept the enum default and silently pre-warmed nothing. Only this
	/// method knows what it caches, so only this method can say whether a write mattered;
	/// adding a field later cannot reintroduce the same bug.
	bool Set(const FString& ModelName, const FString& ModelType, ETryllModelRegistration Registration,
	         const TArray<FString>* Languages = nullptr,
	         const ETryllInferenceEngine* Engine = nullptr,
	         const ETryllMemoryPlacement* Placement = nullptr,
	         const ETryllHotwordBias* Hotwords = nullptr)
	{
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			if (Entries[i].Name != ModelName) continue;
			if (Registration == ETryllModelRegistration::None)
			{
				Entries.RemoveAt(i);
				return true;
			}

			bool bChanged = Entries[i].ModelType != ModelType
			             || Entries[i].Registration != Registration;
			Entries[i].ModelType    = ModelType;
			Entries[i].Registration = Registration;
			// Null means "caller had no catalog data to hand" (an offline
			// re-registration): keep what we already cached rather than
			// erasing it and silently disabling the language warnings.
			if (Languages)
			{
				if (Entries[i].Languages != *Languages) bChanged = true;
				Entries[i].Languages = *Languages;
			}
			// Same rule as Languages: null means "no new information", so an
			// offline re-registration keeps the cached engine rather than
			// silently resetting it. A stale engine no longer stops a load — the
			// server resolves it from its own catalog — but it does cost the
			// pre-warm, which is the only thing the engine hint buys.
			if (Engine)
			{
				if (Entries[i].Engine != *Engine) bChanged = true;
				Entries[i].Engine = *Engine;
			}
			if (Hotwords)
			{
				if (Entries[i].Hotwords != *Hotwords) bChanged = true;
				Entries[i].Hotwords = *Hotwords;
			}
			if (Placement)
			{
				if (Entries[i].Placement != *Placement) bChanged = true;
				Entries[i].Placement = *Placement;
			}
			return bChanged;
		}

		if (Registration == ETryllModelRegistration::None) return false;

		FTryllModelManifestEntry Entry;
		Entry.Name         = ModelName;
		Entry.ModelType    = ModelType;
		Entry.Registration = Registration;
		if (Languages) Entry.Languages = *Languages;
		if (Engine)    Entry.Engine    = *Engine;
		if (Placement) Entry.Placement = *Placement;
		if (Hotwords) Entry.Hotwords = *Hotwords;
		Entries.Add(MoveTemp(Entry));
		return true;
	}

	/** Cached languages for a registered model; empty when unlisted or undeclared. */
	TArray<FString> LanguagesOf(const FString& ModelName) const
	{
		for (const FTryllModelManifestEntry& E : Entries)
			if (E.Name == ModelName) return E.Languages;
		return {};
	}

	/**
	 * Every language any registered model declares, sorted - the source for the
	 * settings language dropdown.
	 *
	 * A **union**, deliberately, not an intersection: a project may register an
	 * English-only model for one character and a 31-language voice for everyone
	 * else, and intersecting would collapse the menu to English. Per-model
	 * mismatches are surfaced where they are actionable instead - on the model
	 * picker (inspector warning) and at agent creation (server error).
	 *
	 * Empty ModelTypes means "all types".
	 */
	TArray<FString> DeclaredLanguages(const TArray<FString>& ModelTypes = {}) const
	{
		TSet<FString> Seen;
		for (const FTryllModelManifestEntry& E : Entries)
		{
			if (ModelTypes.Num() > 0 && !ModelTypes.Contains(E.ModelType)) continue;
			for (const FString& Code : E.Languages)
				if (!Code.IsEmpty()) Seen.Add(Code);
		}
		TArray<FString> Out = Seen.Array();
		Out.Sort();
		return Out;
	}

	/**
	 * Registered models of the given types that do NOT declare Language, ignoring
	 * those that declared nothing. Drives the "N voice models cannot do German"
	 * summary in project settings.
	 */
	TArray<FString> ModelsMissingLanguage(const FString& Language,
	                                      const TArray<FString>& ModelTypes = {}) const
	{
		TArray<FString> Out;
		if (Language.IsEmpty()) return Out;

		for (const FTryllModelManifestEntry& E : Entries)
		{
			if (ModelTypes.Num() > 0 && !ModelTypes.Contains(E.ModelType)) continue;
			if (E.Languages.Num() == 0) continue;   // undeclared = unknown, not missing

			bool bFound = false;
			for (const FString& Code : E.Languages)
				if (Code.Equals(Language, ESearchCase::IgnoreCase)) { bFound = true; break; }
			if (!bFound) Out.Add(E.Name);
		}
		return Out;
	}

	/** Names of registered models of a given type, for type-filtered pickers. */
	TArray<FString> ModelsOfType(const FString& ModelType) const
	{
		TArray<FString> Out;
		for (const FTryllModelManifestEntry& E : Entries)
			if (E.ModelType == ModelType) Out.Add(E.Name);
		return Out;
	}

	/**
	 * The manifest every reader and writer uses: the project CDO, or a test's
	 * in-memory override. Use these instead of GetDefault / GetMutableDefault so
	 * editor-window tests never read or write the host project's DefaultGame.ini.
	 */
	static const UTryllModelManifest* GetActive() { return GetMutableActive(); }
	static UTryllModelManifest* GetMutableActive();

	/** Test-only. The override must stay rooted until it is cleared with nullptr. */
	static void SetActiveOverrideForTests(UTryllModelManifest* Override);

	/** Write DefaultGame.ini. No-op (false) for a test override, which has nowhere to go. */
	bool SaveToConfig();
};
