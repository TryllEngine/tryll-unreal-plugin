// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.
//
// One implementation of the session calls both subsystems expose. A completion
// runs before any error broadcast. Shared connection events are used when
// Events() returns them; otherwise the local callback runs (PIE teardown).

#pragma once

#include "TryllSubsystem.h"

class FTryllConnection;
class FTryllConnectionEvents;

namespace Tryll::Client {

class TRYLLCLIENT_API FTryllSessionCalls
{
public:
	static void SetInferenceThrottle(FTryllConnection* Connection, float Level);

	static void SetSessionLocale(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& Locale,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<void(const FString&, bool)> LocalComplete,
		TFunction<void(const FString&)> LocalError);

	static void ListModels(
		FTryllConnection* Connection,
		const void* Owner,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<FTryllOnListModels> OnComplete,
		TFunction<void(const TArray<FTryllModelInfo>&, bool)> LocalComplete);

	static void GetMemoryConsumption(
		FTryllConnection* Connection,
		const void* Owner,
		TFunction<FTryllOnGetMemoryConsumption> OnComplete,
		TFunction<void(const FTryllMemorySnapshot&, bool)> LocalComplete);

	static void EnumerateMutableKbs(
		FTryllConnection* Connection,
		const void* Owner,
		TFunction<FTryllOnEnumerateMutableKbs> OnComplete,
		TFunction<void(int32, bool)> LocalCount);

	static void SaveGameMemory(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& Path,
		const FString& SaveId,
		TFunction<FTryllOnSaveGameMemory> OnComplete,
		TFunction<void(int32, bool)> LocalCount);

	static void LoadGameMemory(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& Path,
		ETryllMutableKbEmbedTiming EmbedTiming,
		ETryllMutableKbCompatibility Compatibility,
		TFunction<FTryllOnLoadGameMemory> OnComplete,
		TFunction<void(int32, bool)> LocalCount);

	static void LoadModel(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& ModelName,
		ETryllMemoryPlacement Placement,
		bool bIsOverride,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<void(const FString&, bool)> LocalComplete,
		TFunction<void(const FString&)> LocalError);

	static void UnloadModel(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& ModelName,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<void(const FString&, bool)> LocalComplete,
		TFunction<void(const FString&)> LocalError);

	static void DeleteModel(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& ModelName,
		TFunction<void(const FString&, bool)> LocalComplete,
		TFunction<void(const FString&)> LocalError);

	static void CreateStringStorage(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& Name,
		uint8 Kind,
		const TArray<FString>& Values,
		const TArray<FString>& Keys,
		const FString& FilePath,
		TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<void(const FString&, bool)> LocalComplete,
		TFunction<void(const FString&)> LocalError);

	static void DestroyStringStorage(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& Name,
		TFunction<UTryllSubsystem::FTryllOnStringStorage> OnComplete,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<void(const FString&, bool)> LocalComplete,
		TFunction<void(const FString&)> LocalError);

	static void CreateEmbeddedStringStorage(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& Name,
		const FString& ConfigPath,
		const TArray<FString>& Strings,
		const FString& EmbeddingModel,
		TFunction<UTryllSubsystem::FTryllOnEmbeddedStorageCreated> OnComplete,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<void(const FString&, int32, bool)> LocalComplete,
		TFunction<void(const FString&)> LocalError);

	static void DestroyAgent(
		FTryllConnection* Connection,
		const void* Owner,
		uint64 AgentId,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<void(uint64)> OnDestroyed,
		TFunction<void(int64)> LocalDestroyed,
		TFunction<void(const FString&)> LocalError);

	static void DestroyVoiceInput(
		FTryllConnection* Connection,
		const void* Owner,
		uint64 VoiceInputId,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<void(uint64)> OnDestroyed,
		TFunction<void(const FString&)> LocalError);

	static void DownloadModel(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& ModelName,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<void(const FString&, uint64, uint64, float)> LocalProgress,
		TFunction<void(const FString&, bool, const FString&)> LocalComplete,
		TFunction<void(const FString&)> LocalError);

	static void EnsureMutableKb(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& Name,
		const FString& Schema,
		TFunction<FTryllOnEnsureMutableKb> OnComplete,
		TFunction<TSharedPtr<FTryllConnection>()> LiveConnection,
		TFunction<void(const TSharedPtr<FTryllMutableKb>&)> OnStore,
		TFunction<void(const FString&, bool)> LocalComplete);

	static void DestroyEmbeddedStringStorage(
		FTryllConnection* Connection,
		const void* Owner,
		const FString& Name,
		TFunction<UTryllSubsystem::FTryllOnEmbeddedStorageDestroyed> OnComplete,
		TFunction<FTryllConnectionEvents*()> Events,
		TFunction<void(const FString&, bool)> LocalComplete,
		TFunction<void(const FString&)> LocalError);
};

}
