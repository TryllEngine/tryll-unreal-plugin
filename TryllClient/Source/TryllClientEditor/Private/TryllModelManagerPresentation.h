// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllGraphDescription.h"
#include "TryllModelInfo.h"

/**
 * Feature-local Model Manager presentation core: decimal/download formatters,
 * filter/placement predicates and download-job captions.
 */
struct TRYLLCLIENTEDITOR_API FTryllModelManagerPresentation
{
	static FString FmtDecimalBytes(int64 Bytes);
	static FString FmtSpeed(float BytesPerSec);
	static FString FmtEta(float Seconds);
	static bool SupportsPlacement(const FString& ModelType);
	static bool ShowsPlacement(bool bRegistered, const FString& ModelType);
	static bool MatchesSearch(const FString& Name, const FString& HuggingFaceRepo, const FString& SearchLower);

	/** When EmptyMeansAll, an empty set does not constrain (Unreal). Unity passes false. */
	template <typename T>
	static bool Included(const TSet<T>& Selected, const T& Value, bool bEmptyMeansAll)
	{
		if (Selected.Num() == 0)
		{
			return bEmptyMeansAll;
		}
		return Selected.Contains(Value);
	}

	static bool MatchesFilters(
		bool bTypeOk, bool bEngineOk, bool bStatusOk, bool bAudienceOk, bool bRegistrationOk,
		const FString& Name, const FString& HuggingFaceRepo, const FString& SearchLower);

	static FString FormatDownloadProgress(int64 Bytes, int64 Total, float Speed, float RemainSeconds);
	static FString FormatDownloadDone(int64 Size);
	static FString FormatDownloadFailed(const FString& Error);
	static bool ShouldApplyListGeneration(int32 RequestGeneration, int32 CurrentGeneration);

	enum class ECatalogType : uint8 { Language, Embedding, Stt, Tts, Vad };

	struct FCatalogRow
	{
		FString Name;
		ECatalogType Type = ECatalogType::Language;
		ETryllModelStatus Status = ETryllModelStatus::Absent;
		FString Engine;
		FString Audience;
		FString ToolFormat;
		FString ToolSupport;
		TArray<FString> Files;
		int32 ContextSize = 0;
		FString Sampling;
		bool bLocalOnly = false;
		bool bIncomplete = false;
		bool bAbsent = false;
		bool bMalformed = false;
	};

	struct FDownloadJob
	{
		enum class EPhase : uint8 { Active, Completed, Failed };

		FString Name;
		EPhase Phase = EPhase::Active;
		uint64 BytesDownloaded = 0;
		uint64 TotalBytes = 0;
		float Percent = 0.f;
		FString Error;
		double LastTime = 0.0;
		uint64 LastBytes = 0;
		float SpeedBytesPerSec = 0.f;
	};

	static ECatalogType MapModelType(const FString& Type);
	static FString AudienceOrUnset(const FString& Audience);
	static FString FormatSampling(const FTryllSamplingParams& Sampling);
	static ETryllInferenceEngine ParseEngine(const FString& Engine);
	static FCatalogRow ProjectCatalog(const FTryllModelInfo& Model);
	static uint8 ResolveRegistration(bool bHasManifest, uint8 Stored);
	static bool IsMissingRegistered(uint8 Registration, ETryllModelStatus Status);
	static bool ShouldPromptVadCompanion(
		ECatalogType Type, uint8 NewRegistration, uint8 VadRegistration);
	static ETryllModelStatus StatusAfterDownloadProgress();
	static ETryllModelStatus StatusAfterDownloadComplete(bool bSuccess);
	static bool ApplyDownloadProgress(
		TArray<FDownloadJob>& Jobs,
		const FString& Name,
		uint64 Bytes,
		uint64 Total,
		float Percent,
		double Now);
	static void ApplyDownloadComplete(
		TArray<FDownloadJob>& Jobs,
		const FString& Name,
		bool bSuccess,
		const FString& Error);
	static int32 ActiveDownloadCount(const TArray<FDownloadJob>& Jobs);
	static void ClearFinishedDownloads(TArray<FDownloadJob>& Jobs);
};
