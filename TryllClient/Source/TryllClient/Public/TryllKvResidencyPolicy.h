// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TryllKvResidencyPolicy.generated.h"

class UTryllAgentComponent;

/**
 * Keeps only the agents currently in use holding language-model KV cache, evicting the rest to
 * reclaim VRAM.
 *
 * **This is a memory-for-latency trade, and it should be opt-in.** With every agent's contexts
 * resident, switching between agents is nearly free. Evicted, the first message to an agent also pays
 * context allocation and a full prefill — on the order of a second or two — in exchange for a large
 * VRAM saving. That is the right trade on a GPU that would otherwise not fit and the wrong one on a
 * GPU with headroom, which is why bEnabled exists rather than this being unconditional. Drive it from
 * a graphics-quality or "low VRAM" setting.
 *
 * **Why this is in the plugin rather than in your game.** Nothing here is game-specific, and every
 * part of it is a mistake waiting to be made: forgetting the debounce turns scrubbing through a
 * character list into one uncancellable prefill per entry; forgetting the hot set punishes the most
 * common interaction pattern there is; and forgetting that cache operations are idle-only produces
 * intermittent AgentBusy failures that look like server bugs.
 *
 * **Correctness never depends on this.** The server transparently restores an evicted context before
 * the next send, so a badly tuned policy costs time to first token, not behaviour.
 *
 * **Usage.** Add to any Actor, call Register for each agent that should participate (or RegisterAll),
 * and call SetActiveAgent whenever the player's focus moves. Agents that must always stay warm — a
 * background summariser that runs after every turn, say — should simply not be registered: evicting
 * one guarantees a restore on its next turn, which is strictly worse than never evicting it.
 *
 * For creation-time behaviour, pair this with UTryllAgentComponent::KvCacheInitialization =
 * DeferAllocation on the agents it manages. That is where most of the saving comes from: a context
 * costs its full context_size the moment it is created, so never allocating it is worth more than
 * evicting it later.
 */
UCLASS(ClassGroup = (Tryll), meta = (BlueprintSpawnableComponent),
	   HideCategories = (Activation, Collision, Cooking, Rendering, Replication))
class TRYLLCLIENT_API UTryllKvResidencyPolicy : public UActorComponent
{
	GENERATED_BODY()

public:
	UTryllKvResidencyPolicy();

	/**
	 * Whether the policy evicts at all. When false nothing is evicted, so toggling at runtime is safe;
	 * call RestoreAll afterwards to warm what was evicted.
	 *
	 * Agents created with DeferAllocation stay that way — creation mode is structural. Turning the
	 * policy off mid-session restores what it evicted; it cannot undo an allocation that never
	 * happened.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|KV Cache")
	bool bEnabled = true;

	/**
	 * How many agents keep their contexts resident, including the active one.
	 *
	 * Two is the useful default: "go back to the one I was just using" is the commonest pattern, and a
	 * hot set of one punishes it with a full restore every time. One maximises the saving.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|KV Cache",
			  meta = (ClampMin = "1", ClampMax = "8"))
	int32 HotSetSize = 2;

	/**
	 * Seconds an agent must stay active before its prefill starts.
	 *
	 * Not cosmetic: a prefill cannot be cancelled once started, so the only way to avoid paying for
	 * agents the player merely passed over is not to start.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|KV Cache",
			  meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float PrefillDebounceSeconds = 0.35f;

	/** Seconds to back off after an operation was refused with AgentBusy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tryll|KV Cache",
			  meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float BusyRetrySeconds = 0.5f;

	/**
	 * Registers an agent. bStartsResident should be false when the agent was created with
	 * DeferAllocation (nothing is allocated yet) and true otherwise. Re-registering is a no-op.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|KV Cache")
	void Register(UTryllAgentComponent* Agent, bool bStartsResident);

	/** Registers every agent component in the world. Convenience for simple setups. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|KV Cache")
	void RegisterAll(bool bStartResident);

	/** Stops managing an agent. Does not evict or restore it. */
	UFUNCTION(BlueprintCallable, Category = "Tryll|KV Cache")
	void Unregister(UTryllAgentComponent* Agent);

	/**
	 * Tells the policy which agent the player is now interacting with. Safe to call on every
	 * selection, including repeats — the debounce and the resident set absorb the noise.
	 *
	 * Named SetActiveAgent rather than SetActive because UActorComponent already has a SetActive,
	 * and a same-named override would both hide the base call and be rejected by UHT.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|KV Cache")
	void SetActiveAgent(UTryllAgentComponent* Agent);

	/** The agent most recently passed to SetActiveAgent, or null. */
	UFUNCTION(BlueprintPure, Category = "Tryll|KV Cache")
	UTryllAgentComponent* GetActiveAgent() const;

	/**
	 * Restores every registered agent. Call after setting bEnabled to false so nobody is left paying
	 * restore costs for a policy that is no longer running.
	 */
	UFUNCTION(BlueprintCallable, Category = "Tryll|KV Cache")
	void RestoreAll();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

private:
	void EnsureResident(UTryllAgentComponent* Agent);
	void EvictCold();
	void Defer();

	UPROPERTY(Transient)
	TArray<UTryllAgentComponent*> Agents;

	/** Most-recently-active first. The first HotSetSize entries stay resident. */
	UPROPERTY(Transient)
	TArray<UTryllAgentComponent*> Recency;

	/** Agents believed to hold contexts, so repeat work is skipped. */
	UPROPERTY(Transient)
	TSet<UTryllAgentComponent*> Resident;

	/** Agents with an operation in flight, so a second is not issued. */
	UPROPERTY(Transient)
	TSet<UTryllAgentComponent*> InFlight;

	UPROPERTY(Transient)
	UTryllAgentComponent* PendingActive = nullptr;

	double PendingSince = 0.0;
	double RetryAfter   = 0.0;

	/** Unscaled wall clock; pause and time dilation must not stall prefill/evict. */
	double NowSeconds() const;
};
