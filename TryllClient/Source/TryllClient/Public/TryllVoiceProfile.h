// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TryllModelName.h"
#include "TryllStoragePath.h"
#include "TryllVoiceProfile.generated.h"

namespace Tryll { struct VoiceProfileT; }
struct FTryllWorkflowIssue;
struct FTryllTtsCapabilities;

/**
 * When the server prepares a voice's reference clips (speaker embeddings,
 * decoded audio). Mirrors Tryll::VoicePreparation in voice.fbs.
 */
UENUM(BlueprintType)
enum class ETryllVoicePreparation : uint8
{
	/** Every clip (base and all modes) when the voice is set: no hitch at the first sentence in a new mode. */
	Eager = 0,
	/** Each clip on first use. For experiments. */
	Lazy = 1,
};

/**
 * One named delivery (mood or manner of speaking) of a voice. What each field
 * does depends on the TTS family; a field the voice's model cannot use is
 * rejected with InvalidVoiceProfile (3023) when the voice is set. A mode with no
 * fields set behaves like the built-in "neutral" mode, so every profile can
 * define the same names and game code stays portable across models.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllDeliveryMode
{
	GENERATED_BODY()

	/** Name passed to Set Delivery Mode: lowercase letters, digits and '_', 1-32 characters, unique in the profile. "neutral" is built in and cannot be defined. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice")
	FString Name;

	/** Clone families (Pocket TTS, Chatterbox, Qwen3-TTS Base): a clip of this character speaking in this mood, relative to the storage root. Replaces the profile's reference voice while the mode is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (TryllStorageKind = "voice"))
	FTryllStoragePath ReferenceVoice;

	/** Maya1: sustained delivery clause appended to the voice description. A known keyword (angry, sad, whisper, ...) becomes its tuned clause; anything else is appended as written. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (MultiLine = "true"))
	FString Instruction;

	/** Chatterbox: exaggeration for this mode, 0.0-2.0. Negative uses the profile's Expressiveness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (ClampMin = "-1.0", ClampMax = "2.0", UIMin = "-1.0", UIMax = "2.0"))
	float Expressiveness = -1.f;

	/** Multiplies the profile speed while the mode is active. Must stay 1.0 on models without speed control. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (ClampMin = "0.5", ClampMax = "2.0", UIMin = "0.5", UIMax = "2.0"))
	float SpeedScale = 1.f;
};

/**
 * The voice of one agent (protocol v27): the TTS model, the base identity, the
 * speech-pipeline settings and the named delivery modes. Every speaking node of
 * the agent (Speak, Generate And Speak) uses it. Hand-mirrors VoiceProfile in
 * voice.fbs; ToWire converts it for CreateAgent / SetAgentVoice.
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllVoiceProfileData
{
	GENERATED_BODY()

	/** TTS model catalog name. Required: there is no default model. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (TryllModelType = "tts"))
	FTryllModelName TtsModelName;

	/** Speaker or style index for multi-speaker models (Supertonic 3: 0-9; Orpheus and Maya1: preset index). Must be below the model's speaker count. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (ClampMin = "0", UIMin = "0"))
	int32 SpeakerId = 0;

	/** Clone families (Pocket TTS, Chatterbox, Qwen3-TTS Base): reference clip relative to the storage root; locale-aware (voices/loc/<locale>/). Empty = the model's default voice (Qwen3-TTS Base requires a clip). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (TryllStorageKind = "voice"))
	FTryllStoragePath ReferenceVoice;

	/** Maya1: natural-language identity of the voice. Overrides Speaker Id. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (MultiLine = "true"))
	FString VoiceDescription;

	/** Speech-rate multiplier, 1.0 = native. Must stay 1.0 on models without speed control (Orpheus, Maya1, Qwen3-TTS). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (ClampMin = "0.1", ClampMax = "3.0", UIMin = "0.1", UIMax = "3.0"))
	float Speed = 1.f;

	/** Language for multilingual models that pick it per call. Empty = session locale, then the model default. Rejected on models with a fixed language. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice")
	FString Lang;

	/** Chatterbox exaggeration used by "neutral" and by modes that do not set their own, 0.0-2.0. Ignored by other families. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "2.0"))
	float Expressiveness = 0.5f;

	/** When the server prepares the reference clips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", AdvancedDisplay)
	ETryllVoicePreparation Prepare = ETryllVoicePreparation::Eager;

	/** Emit a viseme track for every sentence this agent speaks, from any speaking node. Turn on for lip-synced characters (Tryll Lip Sync, the MetaHuman Viseme solver). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice")
	bool bEmitVisemes = false;

	/** Named delivery modes. Game code switches between them with Set Delivery Mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice", meta = (TitleProperty = "Name"))
	TArray<FTryllDeliveryMode> DeliveryModes;

	/** Mode active after the voice is set. Empty = "neutral". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|Voice")
	FString DefaultDeliveryMode;

	/** Fill the FlatBuffers object-API mirror. Storage paths are normalized like node file pickers. */
	void ToWire(::Tryll::VoiceProfileT& Out) const;

	/** Names of the defined delivery modes. */
	TArray<FString> GetModeNames() const;

	/** True when Name is "neutral" or one of the defined modes. */
	bool HasMode(const FString& Name) const;

	/**
	 * Local checks the server would also make, minus anything that needs the
	 * model's capabilities: a model name, mode names, the default mode, ranges,
	 * text limits and the reference clips (path shape and, when
	 * bCheckFilesExist and the storage folder is configured, that the file exists).
	 * Context names the profile in each message.
	 */
	void Validate(const FString& Context, TArray<FTryllWorkflowIssue>& OutIssues, bool bCheckFilesExist = true) const;

	/**
	 * The capability half of the server's check (InvalidVoiceProfile, 3023): a field
	 * the model cannot use is an error. Caps usually come from the catalog
	 * (ListModels or the editor's catalog file); the server checks the loaded
	 * model's runtime capabilities.
	 */
	void ValidateAgainst(const FTryllTtsCapabilities& Caps, const FString& Context, TArray<FTryllWorkflowIssue>& OutIssues) const;

	/** The built-in mode that is always available. */
	static const TCHAR* NeutralMode;

	/** True for a valid delivery-mode name: [a-z0-9_]{1,32}. */
	static bool IsValidModeName(const FString& Name);
};

/**
 * A character's voice as an asset: assign it on UTryllAgentComponent::VoiceProfile.
 * One profile can serve many agents, and one workflow can be voiced by many
 * profiles. Content Browser: Tryll > Voice Profile.
 */
UCLASS(BlueprintType)
class TRYLLCLIENT_API UTryllVoiceProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice", meta = (ShowOnlyInnerProperties))
	FTryllVoiceProfileData Voice;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
	virtual void PreSave(FObjectPreSaveContext ObjectSaveContext) override;
#endif
};
