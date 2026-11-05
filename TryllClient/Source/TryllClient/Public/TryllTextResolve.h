// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllError.h"
#include "TryllGraphDescription.h"
#include "TryllTextProvider.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"

class FTryllAgent;

/**
 * Fills ticked Text IDs before CreateAgent and ChangeParams, and again for
 * non-structural fields when the provider's language changes.
 * Always works on a copy; the authored asset is not modified.
 */
struct TRYLLCLIENT_API FTryllTextResolve
{
	struct FTextIdRef
	{
		FString NodeName;
		FString FieldPath;
		FString Id;
	};

	static bool HasTickedFields(const FTryllGraphDescription& Graph);

	/** True when Params has a ticked field. bLiveOnly ignores fields inside structural containers. */
	static bool HasTickedFields(const UTryllNodeParamsBase* Params, bool bLiveOnly = false);

	static TArray<FTextIdRef> CollectTextIds(const FTryllGraphDescription& Graph);

	/**
	 * When the graph has a ticked field, duplicate its params onto Outer and
	 * replace those strings with provider text. OutOwned is every duplicate,
	 * including when resolution fails, so the caller can root or destroy them.
	 * A graph with no ticked field is copied by value and OutOwned stays empty.
	 */
	static bool PrepareForCreate(
		const FTryllGraphDescription& Graph,
		UObject* Outer,
		UTryllTextProvider* Provider,
		FTryllGraphDescription& OutGraph,
		TArray<UTryllNodeParamsBase*>& OutOwned,
		FTryllError& OutError);

	/**
	 * Resolve the ticked fields of Params in place (pass a copy). bLiveOnly skips
	 * fields inside structural containers, which cannot change on a live agent.
	 * bOutChanged is true when any string that is sent changed.
	 * Errors name asset, node, field, table and ID (InvalidParamValue).
	 */
	static bool ResolveParams(
		UTryllNodeParamsBase* Params,
		UTryllTextProvider* Provider,
		FName Table,
		const FString& AssetName,
		const FString& NodeName,
		bool bLiveOnly,
		bool& bOutChanged,
		FTryllError& OutError);
};

/**
 * Live agents created with ticked Text IDs. On a language change each agent is
 * refreshed from its current params baseline (so runtime ChangeParams survive):
 * ticked non-structural fields are re-resolved on a clone and only nodes whose
 * text changed are sent. One failed field sends nothing for that agent.
 * An AgentBusy reply marks the agent dirty; the refresh is redone when its turn ends.
 * Applied fires once the latest refresh of an agent is fully acknowledged, or at once
 * when no text changed. A refresh started later makes the replies of an earlier one stale.
 */
struct TRYLLCLIENT_API FTryllLocalizedAgentSet
{
	using FSend = TFunction<void(
		const TSharedPtr<FTryllAgent>& Agent,
		const FString& NodeName,
		UTryllNodeParamsBase* Params,
		TFunction<void(const FTryllError&)> OnComplete)>;

	using FFail = TFunction<void(uint64 AgentId, const FString& AssetName, const FTryllError& Error)>;

	using FApplied = TFunction<void(uint64 AgentId, const FString& AssetName)>;

	FTryllLocalizedAgentSet();
	FTryllLocalizedAgentSet(const FTryllLocalizedAgentSet&) = delete;
	FTryllLocalizedAgentSet& operator=(const FTryllLocalizedAgentSet&) = delete;

	void Track(const TSharedPtr<FTryllAgent>& Agent, FName Table, const FString& AssetName);
	void Forget(uint64 AgentId);
	void ForgetAll();

	bool IsTracked(uint64 AgentId) const;
	bool HasPending(uint64 AgentId) const;
	int32 Num() const { return ById.Num(); }

	/** Refresh every tracked agent. Returns the number of node sends. */
	int32 Refresh(UTryllTextProvider* Provider, const FSend& Send, const FFail& Fail, const FApplied& Applied = FApplied());

	/** Refresh one agent from its current baseline. Returns the number of node sends. */
	int32 RefreshAgent(uint64 AgentId, UTryllTextProvider* Provider, const FSend& Send, const FFail& Fail, const FApplied& Applied = FApplied());

	/** The agent's turn ended: redo the refresh when an earlier send came back AgentBusy. */
	int32 NoteTurnEnded(uint64 AgentId, UTryllTextProvider* Provider, const FSend& Send, const FFail& Fail, const FApplied& Applied = FApplied());

	void MarkDirty(uint64 AgentId);

private:
	struct FBinding
	{
		TWeakPtr<FTryllAgent> Agent;
		FName Table;
		FString AssetName;
		bool bDirty = false;
		uint32 Generation = 0;
	};

	TMap<uint64, FBinding> ById;

	/** Send callbacks hold a weak reference to this, so a reply after teardown is ignored. */
	TSharedRef<int32> AliveToken;
};
