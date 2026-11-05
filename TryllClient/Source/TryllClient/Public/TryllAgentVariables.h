// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.
//
// Local mirror of one agent's declared per-agent Variables, with deferred-flush
// staging. Mirrors Tryll::Client::AgentVariables (C++ core client,
// tryll/clients/cpp/include/tryll/AgentVariables.h), tryll_client.variables.AgentVariables
// (Python), and TryllAgentVariables (Unity, tryll/clients/unity/Runtime/TryllAgentVariables.cs):
// seeded from the declaration passed to CreateAgent, mutations validated locally
// (unknown name / type mismatch return an FTryllError immediately — no wire
// round-trip) and staged.
//
// Staged writes are coalesced per name (last value wins for scalars; a pure
// Reset is preserved; set edits accumulate a minimal add/remove delta, or a
// whole-set assign, whichever is smaller) so rapid repeated writes never grow
// the batch unboundedly. The batch is flushed as one UpdateAgentVariablesRequest
// immediately before the agent's next SendMessage/Resume/ChangeParams call
// (never before Cancel), and retained in-flight until acknowledged: a transient
// AgentBusy re-stages behind newer writes, a hard rejection re-syncs the
// affected names from the server, and either way the error is surfaced through
// the agent's OnError delegate (auto-flush) or the caller's completion callback.

#pragma once

#include "CoreMinimal.h"
#include "TryllError.h"
#include "TryllVariableDecl.h"
#include "UObject/Object.h"
#include "TryllAgentVariables.generated.h"

class FTryllAgent;

/**
 * Local mirror of one agent's declared per-agent Variables (C++ surface).
 * Obtained via FTryllAgent::GetVariables(). Blueprint code uses the
 * UTryllAgentVariables wrapper (UTryllAgentComponent::GetVariables()) instead.
 */
class TRYLLCLIENT_API FTryllAgentVariables
{
public:
	FTryllAgentVariables(FTryllAgent* InAgent, const TArray<FTryllVariableDecl>& Declared);

	// ── Introspection ──────────────────────────────────────────────────────

	[[nodiscard]] bool IsDeclared(const FString& Name) const { return DeclaredTypes.Contains(Name); }
	[[nodiscard]] TOptional<ETryllVariableType> DeclaredType(const FString& Name) const;

	/** True when one or more mutations are staged and not yet flushed. */
	[[nodiscard]] bool IsDirty() const { return PendingOrder.Num() > 0; }

	/** Copy of every declared variable's current local value, keyed by name. */
	[[nodiscard]] TMap<FString, FTryllVariableDecl> Snapshot() const { return Values; }

	[[nodiscard]] int64           GetInt(const FString& Name) const;
	[[nodiscard]] double          GetFloat(const FString& Name) const;
	[[nodiscard]] FString         GetString(const FString& Name) const;
	[[nodiscard]] bool            GetBool(const FString& Name) const;
	[[nodiscard]] TArray<FString> GetStringSet(const FString& Name) const;

	// ── Mutation (local pre-validation; no wire round-trip on failure) ─────

	FTryllError SetInt(const FString& Name, int64 Value);
	FTryllError SetFloat(const FString& Name, double Value);
	FTryllError SetString(const FString& Name, const FString& Value);
	FTryllError SetBool(const FString& Name, bool bValue);
	FTryllError SetStringSet(const FString& Name, const TArray<FString>& Values);

	/** Idempotent — a no-op success if Element is already present. */
	FTryllError AddToSet(const FString& Name, const FString& Element);
	/** Idempotent — a no-op success if Element is already absent. */
	FTryllError RemoveFromSet(const FString& Name, const FString& Element);
	/** Restores the CreateAgent-time initial value for Name. */
	FTryllError Reset(const FString& Name);

	// ── Flush / sync ─────────────────────────────────────────────────────

	/**
	 * Send every staged mutation as one batched UpdateAgentVariablesRequest.
	 * Called automatically at the top of FTryllAgent::SendMessage/Resume/
	 * ChangeParams — deliberately NOT before Cancel. No-op (invokes OnComplete
	 * with Ok) when nothing is staged.
	 */
	void FlushIfDirty(
		TFunction<void(const FTryllError&)> OnComplete = nullptr,
		uint64 CarrierRequestId = 0);

	/** Refresh the local mirror from the server's authoritative current values.
	 *  Names with a locally newer (staged or in-flight) write are skipped.
	 *  Empty Names = every declared variable. */
	void SyncFromServerAsync(TArray<FString> Names = {}, TFunction<void(const FTryllError&)> OnComplete = nullptr);

	/**
	 * Test hook: when set, FlushIfDirty invokes this instead of the live agent
	 * wire send. Signature is (WireBatch, OnComplete).
	 */
	TFunction<void(TArray<FTryllVariableUpdate>, TFunction<void(const FTryllError&)>)> TestSendUpdate;

private:
	FTryllAgent* Agent = nullptr;

	TMap<FString, ETryllVariableType> DeclaredTypes;
	TMap<FString, FTryllVariableDecl> Initial;
	TMap<FString, FTryllVariableDecl> Values;

	// Per-name coalesced staging entry. Mode is empty (pure set delta),
	// "assign" (whole-value replacement), or "reset".
	struct FPendingEntry
	{
		FString      Mode;
		TSet<FString> Adds;
		TSet<FString> Removes;
	};
	TArray<FString>              PendingOrder;
	TMap<FString, FPendingEntry> PendingByName;
	// Per-name count of outstanding in-flight batches that still include the
	// name. Each FlushIfDirty captures its own batch snapshot in the response
	// callback so overlapping flushes cannot overwrite each other's recovery
	// state. MutationSeq advances on every staged write so an older batch's
	// AgentBusy/hard-error recovery never re-applies a superseded value.
	TMap<FString, int32>         InFlightRefCounts;
	TMap<FString, int32>         MutationSeq;

	FTryllError RequireType(const FString& Name, ETryllVariableType Expected) const;
	static FTryllError UnknownVariableError(const FString& Name);
	static TArray<FString> NormalizeSet(const TArray<FString>& In);

	FPendingEntry& TouchPending(const FString& Name);
	void MarkWholesale(const FString& Name, const TCHAR* Mode);
	TArray<FTryllVariableUpdate> BuildWireBatch() const;
	bool IsBatchCurrent(const FString& Name, const TMap<FString, int32>& BatchSeq) const;
	void HandleFlushResult(
		const FTryllError& Error,
		TArray<FString> BatchOrder,
		TMap<FString, FPendingEntry> BatchByName,
		TMap<FString, int32> BatchSeq);
};

/**
 * Blueprint-facing wrapper around FTryllAgentVariables. Returned by
 * UTryllAgentComponent::GetVariables(); forwards typed setters/getters to the
 * agent's C++ mirror. Reads are synchronous (served from the local mirror);
 * writes are staged and flushed automatically before the next agent
 * interaction. Setters return an FTryllError (Ok on success); an unknown name
 * or type mismatch is rejected locally with no wire round-trip.
 */
UCLASS(BlueprintType)
class TRYLLCLIENT_API UTryllAgentVariables : public UObject
{
	GENERATED_BODY()

public:
	/** Bind this wrapper to an agent. Safe to re-bind (e.g. after the agent is recreated). */
	void Initialize(TWeakPtr<FTryllAgent> InAgent) { Agent = MoveTemp(InAgent); }

	UFUNCTION(BlueprintCallable, Category = "Tryll|Variables")
	FTryllError SetIntVariable(const FString& Name, int64 Value);

	UFUNCTION(BlueprintCallable, Category = "Tryll|Variables")
	FTryllError SetFloatVariable(const FString& Name, float Value);

	UFUNCTION(BlueprintCallable, Category = "Tryll|Variables")
	FTryllError SetStringVariable(const FString& Name, const FString& Value);

	UFUNCTION(BlueprintCallable, Category = "Tryll|Variables")
	FTryllError SetBoolVariable(const FString& Name, bool bValue);

	UFUNCTION(BlueprintCallable, Category = "Tryll|Variables")
	FTryllError SetStringSetVariable(const FString& Name, const TArray<FString>& Values);

	UFUNCTION(BlueprintCallable, Category = "Tryll|Variables")
	FTryllError AddToSetVariable(const FString& Name, const FString& Element);

	UFUNCTION(BlueprintCallable, Category = "Tryll|Variables")
	FTryllError RemoveFromSetVariable(const FString& Name, const FString& Element);

	UFUNCTION(BlueprintCallable, Category = "Tryll|Variables")
	FTryllError ResetVariable(const FString& Name);

	/** True when one or more mutations are staged and not yet flushed to the server. */
	UFUNCTION(BlueprintPure, Category = "Tryll|Variables")
	bool IsDirty() const;

	/**
	 * Send every staged mutation as one batched UpdateAgentVariablesRequest.
	 * Normally automatic before Send/Resume/ChangeParams; use explicitly for
	 * debugging (Flush Now in Details). No-op success when nothing is staged.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|Variables")
	void FlushIfDirty();

	UFUNCTION(BlueprintPure, Category = "Tryll|Variables")
	int64 GetIntVariable(const FString& Name, bool& bFound) const;

	UFUNCTION(BlueprintPure, Category = "Tryll|Variables")
	float GetFloatVariable(const FString& Name, bool& bFound) const;

	UFUNCTION(BlueprintPure, Category = "Tryll|Variables")
	FString GetStringVariable(const FString& Name, bool& bFound) const;

	UFUNCTION(BlueprintPure, Category = "Tryll|Variables")
	bool GetBoolVariable(const FString& Name, bool& bFound) const;

	UFUNCTION(BlueprintPure, Category = "Tryll|Variables")
	TArray<FString> GetStringSetVariable(const FString& Name, bool& bFound) const;

private:
	TWeakPtr<FTryllAgent> Agent;
};
