// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllModelManagerPresentation.h"

FString FTryllModelManagerPresentation::FmtDecimalBytes(int64 Bytes)
{
	if (Bytes >= 1000000000) return FString::Printf(TEXT("%.2f GB"), Bytes / 1.0e9);
	if (Bytes >= 1000000)    return FString::Printf(TEXT("%lld MB"), Bytes / 1000000);
	if (Bytes <= 0)          return TEXT("—");
	return FString::Printf(TEXT("%lld KB"), Bytes / 1000);
}

FString FTryllModelManagerPresentation::FmtSpeed(float BytesPerSec)
{
	if (BytesPerSec >= 1000000.f) return FString::Printf(TEXT("%.1f MB/s"), BytesPerSec / 1.0e6f);
	if (BytesPerSec >= 1000.f)    return FString::Printf(TEXT("%.0f KB/s"), BytesPerSec / 1000.f);
	return FString::Printf(TEXT("%.0f B/s"), BytesPerSec);
}

FString FTryllModelManagerPresentation::FmtEta(float Seconds)
{
	if (Seconds <= 0.f || !FMath::IsFinite(Seconds)) return TEXT("—");
	const int32 Total = FMath::RoundToInt(Seconds);
	const int32 H = Total / 3600;
	const int32 M = (Total % 3600) / 60;
	const int32 S = Total % 60;
	if (H > 0) return FString::Printf(TEXT("%d:%02d:%02d"), H, M, S);
	return FString::Printf(TEXT("%d:%02d"), M, S);
}

bool FTryllModelManagerPresentation::SupportsPlacement(const FString& ModelType)
{
	return !ModelType.Equals(TEXT("vad"), ESearchCase::IgnoreCase);
}

bool FTryllModelManagerPresentation::ShowsPlacement(const bool bRegistered, const FString& ModelType)
{
	return bRegistered && SupportsPlacement(ModelType);
}

bool FTryllModelManagerPresentation::MatchesSearch(
	const FString& Name, const FString& HuggingFaceRepo, const FString& SearchLower)
{
	if (SearchLower.IsEmpty())
	{
		return true;
	}
	return Name.ToLower().Contains(SearchLower) || HuggingFaceRepo.ToLower().Contains(SearchLower);
}

bool FTryllModelManagerPresentation::MatchesFilters(
	const bool bTypeOk, const bool bEngineOk, const bool bStatusOk,
	const bool bAudienceOk, const bool bRegistrationOk,
	const FString& Name, const FString& HuggingFaceRepo, const FString& SearchLower)
{
	return bTypeOk && bEngineOk && bStatusOk && bAudienceOk && bRegistrationOk
		&& MatchesSearch(Name, HuggingFaceRepo, SearchLower);
}

FString FTryllModelManagerPresentation::FormatDownloadProgress(
	const int64 Bytes, const int64 Total, const float Speed, const float RemainSeconds)
{
	return FString::Printf(
		TEXT("%s / %s · %s · ETA %s"),
		*FmtDecimalBytes(Bytes),
		*FmtDecimalBytes(Total),
		*FmtSpeed(Speed),
		*FmtEta(RemainSeconds));
}

FString FTryllModelManagerPresentation::FormatDownloadDone(const int64 Size)
{
	return FString::Printf(TEXT("%s · done"), *FmtDecimalBytes(Size));
}

FString FTryllModelManagerPresentation::FormatDownloadFailed(const FString& Error)
{
	return Error.IsEmpty() ? FString(TEXT("Failed")) : FString::Printf(TEXT("Failed — %s"), *Error);
}

bool FTryllModelManagerPresentation::ShouldApplyListGeneration(
	const int32 RequestGeneration, const int32 CurrentGeneration)
{
	return RequestGeneration == CurrentGeneration;
}

FTryllModelManagerPresentation::ECatalogType FTryllModelManagerPresentation::MapModelType(
	const FString& Type)
{
	if (Type == TEXT("embedding")) return ECatalogType::Embedding;
	if (Type == TEXT("stt")) return ECatalogType::Stt;
	if (Type == TEXT("tts")) return ECatalogType::Tts;
	if (Type == TEXT("vad")) return ECatalogType::Vad;
	return ECatalogType::Language;
}

FString FTryllModelManagerPresentation::AudienceOrUnset(const FString& Audience)
{
	return Audience.IsEmpty() ? FString(TEXT("unset")) : Audience;
}

FString FTryllModelManagerPresentation::FormatSampling(const FTryllSamplingParams& Sampling)
{
	TArray<FString> Parts;
	Parts.Add(FString::Printf(TEXT("temp %.2g"), Sampling.Temperature));
	if (Sampling.TopP < 1.0f) Parts.Add(FString::Printf(TEXT("top_p %.2g"), Sampling.TopP));
	if (Sampling.TopK > 0) Parts.Add(FString::Printf(TEXT("top_k %d"), Sampling.TopK));
	if (Sampling.MinP > 0.0f) Parts.Add(FString::Printf(TEXT("min_p %.2g"), Sampling.MinP));
	if (Sampling.MaxTokens > 0) Parts.Add(FString::Printf(TEXT("max_tokens %d"), Sampling.MaxTokens));
	if (Sampling.RepeatPenalty != 1.0f)
	{
		Parts.Add(FString::Printf(TEXT("repeat %.2g"), Sampling.RepeatPenalty));
	}
	return FString::Join(Parts, TEXT("  ·  "));
}

ETryllInferenceEngine FTryllModelManagerPresentation::ParseEngine(const FString& Engine)
{
	if (Engine == TEXT("mock")) return ETryllInferenceEngine::Mock;
	if (Engine == TEXT("llama-cpp")) return ETryllInferenceEngine::LlamaCpp;
	if (Engine == TEXT("onnx-genai")) return ETryllInferenceEngine::OnnxGenAI;
	if (Engine == TEXT("windows-ml")) return ETryllInferenceEngine::WindowsML;
	if (Engine == TEXT("openvino")) return ETryllInferenceEngine::OpenVino;
	if (Engine == TEXT("tensorrt-llm")) return ETryllInferenceEngine::TensorRtLlm;
	if (Engine == TEXT("sherpa-onnx")) return ETryllInferenceEngine::SherpaOnnx;
	if (Engine == TEXT("onnx-runtime")) return ETryllInferenceEngine::OnnxRuntime;
	return ETryllInferenceEngine::LlamaCpp;
}

FTryllModelManagerPresentation::FCatalogRow FTryllModelManagerPresentation::ProjectCatalog(
	const FTryllModelInfo& Model)
{
	FCatalogRow Row;
	Row.Name = Model.Name;
	Row.Type = MapModelType(Model.ModelType);
	Row.Status = Model.Status;
	Row.Engine = Model.Engine;
	Row.Audience = AudienceOrUnset(Model.Audience);
	Row.ToolFormat = Model.ToolCallFormat;
	Row.ToolSupport = Model.ToolCallSupport;
	Row.Files = Model.Files;
	Row.ContextSize = Model.ContextSize;
	Row.Sampling = Model.bHasDefaultSampling ? FormatSampling(Model.DefaultSampling) : FString();
	Row.bLocalOnly = Model.HuggingFaceRepo.IsEmpty();
	Row.bIncomplete = Model.Status == ETryllModelStatus::Incomplete;
	Row.bAbsent = Model.Status == ETryllModelStatus::Absent;
	Row.bMalformed = Model.Name.IsEmpty();
	return Row;
}

uint8 FTryllModelManagerPresentation::ResolveRegistration(const bool bHasManifest, const uint8 Stored)
{
	return bHasManifest ? Stored : 0;
}

bool FTryllModelManagerPresentation::IsMissingRegistered(
	const uint8 Registration, const ETryllModelStatus Status)
{
	return Registration != 0 && Status == ETryllModelStatus::Absent;
}

bool FTryllModelManagerPresentation::ShouldPromptVadCompanion(
	const ECatalogType Type, const uint8 NewRegistration, const uint8 VadRegistration)
{
	return Type == ECatalogType::Stt && NewRegistration != 0 && VadRegistration == 0;
}

ETryllModelStatus FTryllModelManagerPresentation::StatusAfterDownloadProgress()
{
	return ETryllModelStatus::Downloading;
}

ETryllModelStatus FTryllModelManagerPresentation::StatusAfterDownloadComplete(const bool bSuccess)
{
	return bSuccess ? ETryllModelStatus::Downloaded : ETryllModelStatus::Absent;
}

bool FTryllModelManagerPresentation::ApplyDownloadProgress(
	TArray<FDownloadJob>& Jobs,
	const FString& Name,
	const uint64 Bytes,
	const uint64 Total,
	const float Percent,
	const double Now)
{
	int32 Idx = Jobs.IndexOfByPredicate([&](const FDownloadJob& J) { return J.Name == Name; });
	const bool bIsNew = Idx == INDEX_NONE;
	if (bIsNew)
	{
		FDownloadJob Job;
		Job.Name = Name;
		Job.LastTime = Now;
		Job.LastBytes = Bytes;
		Jobs.Insert(Job, 0);
		Idx = 0;
	}

	FDownloadJob& J = Jobs[Idx];
	const double Dt = Now - J.LastTime;
	if (!bIsNew && Dt > 0.05 && Bytes >= J.LastBytes)
	{
		const float Inst = static_cast<float>((Bytes - J.LastBytes) / Dt);
		J.SpeedBytesPerSec = J.SpeedBytesPerSec > 0.f
			? J.SpeedBytesPerSec + (Inst - J.SpeedBytesPerSec) * 0.3f
			: Inst;
		J.LastTime = Now;
		J.LastBytes = Bytes;
	}
	J.Phase = FDownloadJob::EPhase::Active;
	J.BytesDownloaded = Bytes;
	J.TotalBytes = Total;
	J.Percent = Percent;
	return bIsNew;
}

void FTryllModelManagerPresentation::ApplyDownloadComplete(
	TArray<FDownloadJob>& Jobs,
	const FString& Name,
	const bool bSuccess,
	const FString& Error)
{
	int32 Idx = Jobs.IndexOfByPredicate([&](const FDownloadJob& J) { return J.Name == Name; });
	if (Idx == INDEX_NONE)
	{
		FDownloadJob Job;
		Job.Name = Name;
		Jobs.Insert(Job, 0);
		Idx = 0;
	}
	FDownloadJob& J = Jobs[Idx];
	J.Phase = bSuccess ? FDownloadJob::EPhase::Completed : FDownloadJob::EPhase::Failed;
	J.Error = Error;
	J.SpeedBytesPerSec = 0.f;
	if (bSuccess)
	{
		J.Percent = 100.f;
	}
}

int32 FTryllModelManagerPresentation::ActiveDownloadCount(const TArray<FDownloadJob>& Jobs)
{
	int32 N = 0;
	for (const FDownloadJob& J : Jobs)
	{
		if (J.Phase == FDownloadJob::EPhase::Active)
		{
			++N;
		}
	}
	return N;
}

void FTryllModelManagerPresentation::ClearFinishedDownloads(TArray<FDownloadJob>& Jobs)
{
	Jobs.RemoveAll([](const FDownloadJob& J)
	{
		return J.Phase != FDownloadJob::EPhase::Active;
	});
}
