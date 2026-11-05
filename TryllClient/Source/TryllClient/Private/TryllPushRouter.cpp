// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllPushRouter.h"

#include "TryllAgent.h"
#include "TryllConnection.h"
#include "TryllVoiceInput.h"

void FTryllPushRouter::RegisterAgent(uint64 AgentId, TWeakPtr<FTryllAgent> Agent)
{
	if (AgentId != 0)
	{
		Agents.Add(AgentId, Agent);
	}
}

void FTryllPushRouter::UnregisterAgent(uint64 AgentId)
{
	Agents.Remove(AgentId);
}

void FTryllPushRouter::RegisterVoiceInput(uint64 VoiceInputId, TWeakPtr<FTryllVoiceInput> Voice)
{
	if (VoiceInputId != 0)
	{
		Voices.Add(VoiceInputId, Voice);
	}
}

void FTryllPushRouter::UnregisterVoiceInput(uint64 VoiceInputId)
{
	Voices.Remove(VoiceInputId);
}

TSharedPtr<FTryllAgent> FTryllPushRouter::FindAgent(uint64 AgentId) const
{
	if (const TWeakPtr<FTryllAgent>* Found = Agents.Find(AgentId))
	{
		return Found->Pin();
	}
	return nullptr;
}

void FTryllPushRouter::SnapshotAgents(TArray<TWeakPtr<FTryllAgent>>& Out) const
{
	Agents.GenerateValueArray(Out);
}

void FTryllPushRouter::SnapshotVoices(TArray<TWeakPtr<FTryllVoiceInput>>& Out) const
{
	Voices.GenerateValueArray(Out);
}

void FTryllPushRouter::SnapshotAgentPairs(TArray<TPair<uint64, TWeakPtr<FTryllAgent>>>& Out) const
{
	Out.Reserve(Agents.Num());
	for (const TPair<uint64, TWeakPtr<FTryllAgent>>& Pair : Agents)
	{
		Out.Add(Pair);
	}
}

void FTryllPushRouter::ClearObjects()
{
	Agents.Empty();
	Voices.Empty();
}

int32 FTryllPushRouter::AddSessionListener(FTryllSessionPushListener Listener)
{
	FListenerSlot Slot;
	Slot.Id = NextListenerId++;
	Slot.Listener = MoveTemp(Listener);
	const int32 Id = Slot.Id;
	Listeners.Add(MoveTemp(Slot));
	return Id;
}

void FTryllPushRouter::RemoveSessionListener(int32 ListenerId)
{
	Listeners.RemoveAll([ListenerId](const FListenerSlot& Slot)
	{
		return Slot.Id == ListenerId;
	});
}

void FTryllPushRouter::Route(const FTryllConnEvent& Event)
{
	switch (Event.Type)
	{
	case ETryllConnEventType::AnswerText:
		if (const TSharedPtr<FTryllAgent> Agent = FindAgent(Event.AgentId))
		{
			Agent->HandleAnswerText(Event.Message, Event.Text, Event.bFlag, Event.bFlag2);
		}
		break;

	case ETryllConnEventType::TurnComplete:
		if (const TSharedPtr<FTryllAgent> Agent = FindAgent(Event.AgentId))
		{
			Agent->HandleTurnComplete(
				static_cast<ETryllTurnStatus>(Event.TurnStatus),
				Event.Text,
				static_cast<int32>(Event.TokensGenerated));
		}
		break;

	case ETryllConnEventType::TtsAudioFormat:
		if (const TSharedPtr<FTryllAgent> Agent = FindAgent(Event.AgentId))
		{
			Agent->HandleTtsAudioFormat(Event.Message, Event.TtsSampleRate, Event.TtsChannels, Event.TtsBitsPerSample);
		}
		break;

	case ETryllConnEventType::TtsAudio:
		if (const TSharedPtr<FTryllAgent> Agent = FindAgent(Event.AgentId))
		{
			Agent->HandleTtsAudio(Event.Message, TArrayView<const int16>(Event.TtsPcm.GetData(), Event.TtsPcm.Num()));
		}
		break;

	case ETryllConnEventType::TtsViseme:
		if (const TSharedPtr<FTryllAgent> Agent = FindAgent(Event.AgentId))
		{
			Agent->HandleTtsVisemes(
				Event.Message,
				TArrayView<const FTryllVisemeEvent>(Event.TtsVisemes.GetData(), Event.TtsVisemes.Num()));
		}
		break;

	case ETryllConnEventType::TranscriptUpdate:
		if (const TWeakPtr<FTryllVoiceInput>* Found = Voices.Find(Event.VoiceInputId))
		{
			if (const TSharedPtr<FTryllVoiceInput> Voice = Found->Pin())
			{
				Voice->HandleTranscriptUpdate(
					Event.Text,
					static_cast<ETryllTranscriptUpdateKind>(Event.TranscriptKind),
					static_cast<int32>(Event.AudioMsConsumed));
			}
		}
		break;

	case ETryllConnEventType::NodeEvent:
	{
		const FTryllDecodedNodeEvent Decoded = TryllDecodeNodeEvent(Event.Text, Event.Message, Event.KvPairs);
		bool bPauseListener = false;
		bool bToolCallListener = false;
		for (const FListenerSlot& Slot : Listeners)
		{
			if (Slot.Listener.HasPauseListener && Slot.Listener.HasPauseListener())
			{
				bPauseListener = true;
			}
			if (Slot.Listener.HasToolCallListener && Slot.Listener.HasToolCallListener())
			{
				bToolCallListener = true;
			}
		}
		for (const FListenerSlot& Slot : Listeners)
		{
			if (Slot.Listener.OnNodeEvent)
			{
				Slot.Listener.OnNodeEvent(Event.AgentId, Decoded);
			}
		}
		if (const TSharedPtr<FTryllAgent> Agent = FindAgent(Event.AgentId))
		{
			// Pause and tool-call listeners answer different events. Passing the
			// pause flag into a tool-call batch would mark a later Pause node as
			// resumable, and the reverse would hide a real tool-call listener.
			const bool bSessionListener =
				Decoded.Kind == ETryllNodeEventKind::Paused ? bPauseListener
				: Decoded.Kind == ETryllNodeEventKind::ToolCall ? bToolCallListener
				: false;
			Agent->HandleNodeEvent(Decoded, bSessionListener);
		}
		break;
	}

	default:
		break;
	}
}
