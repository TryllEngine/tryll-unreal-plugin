// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllError.h"
#include "TryllMkbSnapshot.h"
#include "TryllMutableKbTypes.h"

class FTryllConnection;

/**
 * Session-bound handle for one named mutable knowledge base.
 * Created by UTryllSubsystem / UTryllEditorSubsystem after EnsureMutableKb.
 * Operations fail with a terminal error after the owning session is torn down.
 */
class TRYLLCLIENT_API FTryllMutableKb final : public TSharedFromThis<FTryllMutableKb>
{
public:
	FTryllMutableKb(TSharedPtr<FTryllConnection> InConnection, FString InName, FString InSchema);

	[[nodiscard]] bool IsValid() const;
	[[nodiscard]] const FString& GetName() const { return Name; }
	[[nodiscard]] const FString& GetSchema() const { return Schema; }

	void Write(
		const TArray<FTryllMutableKbUpsert>& Records,
		TFunction<void(const TArray<FTryllMutableKbWriteResult>&, const FTryllError&)> OnComplete);

	void Delete(
		const TArray<FString>& Ids,
		TFunction<void(uint32 DeletedCount, const FTryllError&)> OnComplete);

	void Clear(TFunction<void(uint32 ClearedCount, const FTryllError&)> OnComplete);

	void Query(
		const FString& QueryText,
		uint32 TopK,
		TFunction<void(const TArray<FTryllMutableKbHit>&, const FTryllError&)> OnComplete);

	void Query(
		const FString& QueryText,
		uint32 TopK,
		ETryllRetrievalMode Mode,
		TFunction<void(const TArray<FTryllMutableKbHit>&, const FTryllError&)> OnComplete);

	void Get(
		const FString& Id,
		TFunction<void(bool bFound, const FTryllMutableKbRecord&, const FTryllError&)> OnComplete);

	void List(
		uint32 Offset,
		uint32 Limit,
		TFunction<void(const FTryllMutableKbListPage&, const FTryllError&)> OnComplete);

	void Stats(TFunction<void(const FTryllMutableKbStats&, const FTryllError&)> OnComplete);

	void Export(TFunction<void(const FTryllMutableKbSnapshot&, const FTryllError&)> OnComplete);

	void Import(
		const FTryllMutableKbSnapshot& Snapshot,
		ETryllMutableKbCompatibility Compatibility,
		TFunction<void(const FTryllMutableKbImportResult&, const FTryllError&)> OnComplete);

	[[nodiscard]] bool TryHandleEvent(const struct FTryllConnEvent& Event);
	void DrainPendingCallbacks(const FTryllError& TerminalError);

private:
	void FailIfInvalid(const TFunction<void(const FTryllError&)>& OnComplete) const;

	TSharedPtr<FTryllConnection> Connection;
	FString Name;
	FString Schema;
};
