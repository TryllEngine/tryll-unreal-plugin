// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllModelInfo.generated.h"

/**
 * Status of a model as reported by the server.
 * Mirrors the FlatBuffers ModelStatus enum in messages.fbs — ordinal values must stay in sync.
 */
UENUM(BlueprintType)
enum class ETryllModelStatus : uint8
{
	/** Known in catalog but not on disk and not being downloaded. */
	Absent      = 0,
	/** User-provided path; file on disk. */
	Local       = 1,
	/** Transfer currently in progress. */
	Downloading = 2,
	/** Currently loaded in memory. */
	Loaded      = 3,
	/** On disk from HuggingFace download; can be loaded. */
	Downloaded  = 4,
	/** Parent files are present but a required shared asset is absent or invalid. */
	Incomplete  = 5,
};

/** Editor-facing status of one shared-asset dependency of a parent model. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllModelDependency
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString Id;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString Role;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") ETryllModelStatus Status = ETryllModelStatus::Absent;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") int64 SizeBytes = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString Folder;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString LocalPath;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString FailureReason;
};

/** Catalog-advertised speaker/description preset. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllTtsVoicePreset
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString Id;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString Label;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString Description;
};

/** Model-native inline control token declared for authoring. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllTtsInlineTag
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString Token;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString Kind;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") FString Verification;
};

/** Structured TTS capabilities advertised by the catalog. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllTtsCapabilities
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") bool bVoiceDescription = false;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") bool bDeliveryInstruction = false;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") bool bReferenceVoice = false;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") bool bSpeed = false;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") bool bLanguagePerCall = false;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") TArray<FTryllTtsVoicePreset> VoicePresets;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") TArray<FTryllTtsInlineTag> InlineTags;
	/** Chatterbox-style emotion scalar: the voice's Expressiveness and a delivery mode's Expressiveness apply. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") bool bExpressiveness = false;
	/** Number of speaker ids the model accepts (Speaker Id must be below it); 0 = not declared. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") int32 NumSpeakers = 0;
};

/** Recommended sampling parameters for a model, mirroring the server's default_sampling. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllSamplingParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") float Temperature = 0.7f;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") float TopP = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") int32 TopK = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") float MinP = 0.05f;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") int32 MaxTokens = 2048;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") int64 Seed = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") float RepeatPenalty = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") float PresencePenalty = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models") float FrequencyPenalty = 0.0f;
};

/** Summary information for one model returned by ListModels. */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllModelInfo
{
	GENERATED_BODY()

	/** Catalog name of the model (matches the name in models.json). */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models",
		meta = (ToolTip = "Catalog name of the model as registered in models.json."))
	FString Name;

	/** Current status on the server (absent / local / downloading / downloaded / loaded). */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models",
		meta = (ToolTip = "Current status of the model on the server."))
	ETryllModelStatus Status = ETryllModelStatus::Absent;

	/** HuggingFace repo id (e.g. "Qwen/Qwen2.5-0.5B-Instruct-GGUF") or empty for local-only models. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models",
		meta = (ToolTip = "HuggingFace repository id the model can be downloaded from; empty for local-only catalog entries."))
	FString HuggingFaceRepo;

	/** On-disk size in bytes when the model has been downloaded. Zero if Status == Absent. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models",
		meta = (ToolTip = "On-disk size in bytes of the downloaded model; zero when the model is not on disk."))
	int64 SizeBytes = 0;

	/** Model type: "language" | "embedding" | "stt" | "tts" | "vad". */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	FString ModelType;

	/** Resolved inference engine, e.g. "llama-cpp" | "sherpa-onnx". */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	FString Engine;

	/** Intended audience tier: "public" | "experimental" | "internal" | "". */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	FString Audience;

	/** Tool-call prompt/parse format: "chatml" | "llama3" | "mistral" | "generic" | "". */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	FString ToolCallFormat;

	/**
	 * Advisory catalog-declared tool-calling capability for language models:
	 * "supported" | "unsupported". Empty for non-language models (and when
	 * talking to older servers). The authoritative check happens
	 * server-side at agent creation from the model's own chat template.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models",
		meta = (ToolTip = "Advisory tool-calling capability declared by the model catalog: supported or unsupported."))
	FString ToolCallSupport;

	/** Variant context window in tokens; 0 means the engine default. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	int32 ContextSize = 0;

	/** Files downloaded/resolved for the configured engine's variant. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	TArray<FString> Files;

	/**
	 * Languages this model can handle, as lowercase BCP-47 primary language subtags ("en",
	 * "de", ...). Read this to build a language menu instead of hardcoding one -
	 * it is how you discover that voice cloning is English-only, or which STT
	 * models cover the locale you want.
	 *
	 * Empty means *undeclared*, not "supports nothing": treat it as "no
	 * capability information" and keep offering the model.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models",
		meta = (ToolTip = "BCP-47 primary language subtags this model declares. Empty means undeclared, not unsupported."))
	TArray<FString> Languages;

	/** Resolved on-disk directory for the model; empty when not on disk. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	FString LocalPath;

	/** True when the catalog supplies explicit default sampling parameters. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	bool bHasDefaultSampling = false;

	/** Recommended sampling parameters; valid only when bHasDefaultSampling. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	FTryllSamplingParams DefaultSampling;

	/** Shared-asset dependencies of this model. Empty when the variant has none. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	TArray<FTryllModelDependency> Dependencies;

	/** True when the catalog advertised structured TTS capabilities. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	bool bHasTtsCapabilities = false;

	/** Catalog TTS capabilities; valid only when bHasTtsCapabilities. */
	UPROPERTY(BlueprintReadOnly, Category = "Tryll|Models")
	FTryllTtsCapabilities TtsCapabilities;
};
