// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllKvResidencyPolicy.h"

#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "TryllAgent.h"
#include "TryllAgentComponent.h"
#include "TryllClientModule.h"
#include "TryllError.h"

UTryllKvResidencyPolicy::UTryllKvResidencyPolicy()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Cheap: the tick nudges the send queue and, at most once per debounce window, issues one
	// prefill plus any evictions. Everything else early-returns.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UTryllKvResidencyPolicy::Register(UTryllAgentComponent* Agent, bool bStartsResident)
{
	if (!Agent || Agents.Contains(Agent))
	{
		return;
	}
	Agents.Add(Agent);
	if (bStartsResident)
	{
		Resident.Add(Agent);
	}
}

void UTryllKvResidencyPolicy::RegisterAll(bool bStartResident)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		TArray<UTryllAgentComponent*> Found;
		It->GetComponents<UTryllAgentComponent>(Found);
		for (UTryllAgentComponent* Agent : Found)
		{
			Register(Agent, bStartResident);
		}
	}
}

void UTryllKvResidencyPolicy::Unregister(UTryllAgentComponent* Agent)
{
	if (!Agent)
	{
		return;
	}
	Agents.Remove(Agent);
	Recency.Remove(Agent);
	Resident.Remove(Agent);
	InFlight.Remove(Agent);
	if (PendingActive == Agent)
	{
		PendingActive = nullptr;
	}
}

void UTryllKvResidencyPolicy::SetActiveAgent(UTryllAgentComponent* Agent)
{
	if (!Agent)
	{
		return;
	}

	// Recency is maintained even while disabled, so enabling mid-session has a meaningful hot set to
	// work from instead of evicting whatever happens to be first in the list.
	Recency.Remove(Agent);
	Recency.Insert(Agent, 0);

	PendingActive = Agent;
	PendingSince  = NowSeconds();
}

UTryllAgentComponent* UTryllKvResidencyPolicy::GetActiveAgent() const
{
	return Recency.Num() > 0 ? Recency[0] : nullptr;
}

void UTryllKvResidencyPolicy::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const double Now = NowSeconds();

	// Runs regardless of bEnabled: a send can be refused for reasons unrelated to this policy, and a
	// swallowed message is the worst outcome available.
	for (UTryllAgentComponent* Agent : Agents)
	{
		if (Agent)
		{
			Agent->TryFlushPendingMessage();
		}
	}

	if (!bEnabled || Agents.Num() == 0 || Now < RetryAfter)
	{
		return;
	}

	if (PendingActive && (Now - PendingSince) >= PrefillDebounceSeconds)
	{
		UTryllAgentComponent* Target = PendingActive;
		PendingActive = nullptr;
		EnsureResident(Target);
		EvictCold();
	}
}

void UTryllKvResidencyPolicy::EnsureResident(UTryllAgentComponent* Agent)
{
	if (!Agent || !Agent->IsAgentReady())
	{
		return;
	}
	if (Resident.Contains(Agent) || InFlight.Contains(Agent))
	{
		return;
	}

	InFlight.Add(Agent);

	TWeakObjectPtr<UTryllKvResidencyPolicy> WeakThis(this);
	TWeakObjectPtr<UTryllAgentComponent>    WeakAgent(Agent);

	Agent->GetAgent()->PrefillKvCache(
		[WeakThis, WeakAgent](const FTryllKvCachePrefillResult& /*Result*/, const FTryllError& Error)
		{
			UTryllKvResidencyPolicy* Self = WeakThis.Get();
			UTryllAgentComponent*    Comp = WeakAgent.Get();
			if (!Self)
			{
				return;
			}

			Self->InFlight.Remove(Comp);
			if (Comp)
			{
				Comp->TryFlushPendingMessage();
			}

			if (Error.IsOk())
			{
				Self->Resident.Add(Comp);
				return;
			}
			if (Error.Code == static_cast<int32>(ETryllErrorCode::AgentBusy))
			{
				Self->Defer();
				return;
			}

			// Anything else is not actionable here: the next send restores the context anyway, so
			// report it and let that turn pay the cost rather than retrying blindly.
			UE_LOG(LogTryll, Warning,
				TEXT("UTryllKvResidencyPolicy: prefill failed: %s"), *Error.Message);
		});
}

void UTryllKvResidencyPolicy::EvictCold()
{
	for (int32 i = HotSetSize; i < Recency.Num(); ++i)
	{
		UTryllAgentComponent* Agent = Recency[i];
		if (!Agent || !Agent->IsAgentReady())
		{
			continue;
		}
		if (!Resident.Contains(Agent) || InFlight.Contains(Agent))
		{
			continue;
		}

		// A queued message is about to need this cache — evicting now would make the player wait for a
		// restore of exactly what we just threw away.
		if (Agent->HasPendingMessage())
		{
			continue;
		}

		InFlight.Add(Agent);

		TWeakObjectPtr<UTryllKvResidencyPolicy> WeakThis(this);
		TWeakObjectPtr<UTryllAgentComponent>    WeakAgent(Agent);

		Agent->GetAgent()->EvictKvCache(
			[WeakThis, WeakAgent](const FTryllKvCacheEvictResult& /*Result*/, const FTryllError& Error)
			{
				UTryllKvResidencyPolicy* Self = WeakThis.Get();
				if (!Self)
				{
					return;
				}

				Self->InFlight.Remove(WeakAgent.Get());

				if (Error.IsOk())
				{
					Self->Resident.Remove(WeakAgent.Get());
					return;
				}
				if (Error.Code == static_cast<int32>(ETryllErrorCode::AgentBusy))
				{
					Self->Defer();
					return;
				}
				UE_LOG(LogTryll, Warning,
					TEXT("UTryllKvResidencyPolicy: evict failed: %s"), *Error.Message);
			});
	}
}

void UTryllKvResidencyPolicy::Defer()
{
	RetryAfter = NowSeconds() + BusyRetrySeconds;
	if (!PendingActive && Recency.Num() > 0)
	{
		PendingActive = Recency[0];
		PendingSince  = NowSeconds();
	}
}

double UTryllKvResidencyPolicy::NowSeconds() const
{
	return FPlatformTime::Seconds();
}

void UTryllKvResidencyPolicy::RestoreAll()
{
	for (UTryllAgentComponent* Agent : Agents)
	{
		EnsureResident(Agent);
	}
}
