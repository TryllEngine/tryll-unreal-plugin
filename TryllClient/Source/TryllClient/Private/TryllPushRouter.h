// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.
//
// Routes pushes for one connection. Agent and voice maps live here so whichever
// subsystem drains the queue delivers to the object that owns the id. Session
// listeners (one per subsystem) receive tool calls, intents, NodeEvent, and pauses.

#pragma once

#include "CoreMinimal.h"
#include "TryllNodeEvent.h"

class FTryllAgent;
class FTryllVoiceInput;
struct FTryllConnEvent;

struct FTryllSessionPushListener
{
	TFunction<void(uint64 /*AgentId*/, const FTryllDecodedNodeEvent&)> OnNodeEvent;
	TFunction<bool()> HasPauseListener;
	/// Session OnToolCall / OnToolCallWithId / OnToolCallDetailed. A listener that
	/// saw the batch can resume the following pause with ResumeWithToolResult(s).
	TFunction<bool()> HasToolCallListener;
};

class TRYLLCLIENT_API FTryllPushRouter
{
public:
	void RegisterAgent(uint64 AgentId, TWeakPtr<FTryllAgent> Agent);
	void UnregisterAgent(uint64 AgentId);
	void RegisterVoiceInput(uint64 VoiceInputId, TWeakPtr<FTryllVoiceInput> Voice);
	void UnregisterVoiceInput(uint64 VoiceInputId);

	TSharedPtr<FTryllAgent> FindAgent(uint64 AgentId) const;
	void SnapshotAgents(TArray<TWeakPtr<FTryllAgent>>& Out) const;
	void SnapshotAgentPairs(TArray<TPair<uint64, TWeakPtr<FTryllAgent>>>& Out) const;
	void SnapshotVoices(TArray<TWeakPtr<FTryllVoiceInput>>& Out) const;
	int32 AgentCount() const { return Agents.Num(); }
	int32 VoiceCount() const { return Voices.Num(); }
	void ClearObjects();

	int32 AddSessionListener(FTryllSessionPushListener Listener);
	void RemoveSessionListener(int32 ListenerId);

	void Route(const FTryllConnEvent& Event);

private:
	struct FListenerSlot
	{
		int32 Id = 0;
		FTryllSessionPushListener Listener;
	};

	TMap<uint64, TWeakPtr<FTryllAgent>> Agents;
	TMap<uint64, TWeakPtr<FTryllVoiceInput>> Voices;
	TArray<FListenerSlot> Listeners;
	int32 NextListenerId = 1;
};
