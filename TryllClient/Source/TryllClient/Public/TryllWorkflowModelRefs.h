// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllGraphDescription.h"
#include "TryllModelManifest.h"

struct FTryllVoiceProfileData;

/**
 * Collects model-name references from a workflow graph and compares them against
 * the project's TryllModelManifest. Used by UTryllWorkflowAsset::IsDataValid /
 * PreSave (cook) so a packaged build cannot silently omit a referenced model.
 *
 * Covers workflow assets and voice profiles (UTryllVoiceProfile, protocol v27).
 */
struct TRYLLCLIENT_API FTryllWorkflowModelRef
{
	/** Empty for graph-level DefaultModelName. */
	FString NodeName;
	FString FieldName;
	FString ModelName;
	/** "language" | "embedding" | "stt" | "tts" | "vad" */
	FString ModelType;
	/** True for a reference read from a voice profile rather than a graph. */
	bool bFromVoiceProfile = false;
};

struct TRYLLCLIENT_API FTryllWorkflowModelViolation
{
	FTryllWorkflowModelRef Ref;
	ETryllModelRegistration Registration = ETryllModelRegistration::None;

	FString ToString(const FString& AssetLabel) const;
};

struct TRYLLCLIENT_API FTryllWorkflowModelRefs
{
	/** Every non-empty FTryllModelName field tagged meta=(TryllModelType=...) that is active (bOverride* true, or no override toggle). */
	static TArray<FTryllWorkflowModelRef> Collect(const FTryllGraphDescription& Graph);

	/** The voice profile's model reference (TtsModelName), through the same tagged-field scan. Empty when unset. */
	static TArray<FTryllWorkflowModelRef> CollectVoice(const FTryllVoiceProfileData& Voice);

	/** Voice-profile refs whose registration is not Production. */
	static TArray<FTryllWorkflowModelViolation> FindNonProductionVoiceRefs(
		const FTryllVoiceProfileData& Voice,
		const UTryllModelManifest* Manifest);

	/** Refs whose registration is not Production (including unlisted → None). */
	static TArray<FTryllWorkflowModelViolation> FindNonProductionRefs(
		const FTryllGraphDescription& Graph,
		const UTryllModelManifest* Manifest);

	/**
	 * True when any STT model is referenced but the manifest has no Production VAD.
	 * Offline STT needs the server's default_vad_model staged into the build.
	 */
	static bool NeedsProductionVad(
		const TArray<FTryllWorkflowModelRef>& Refs,
		const UTryllModelManifest* Manifest,
		FString& OutReason);
};
