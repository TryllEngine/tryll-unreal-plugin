// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllIntentClassification.h"
#include "TryllToolCall.h"
#include "TryllNodeEvent.generated.h"

/**
 * One key/value from a NodeEvent payload. Order matches the wire.
 * Shared by the session fallback (UTryllSubsystem::OnNodeEvent) and the
 * per-agent fallback (UTryllAgentComponent::OnNodeEvent).
 */
USTRUCT(BlueprintType)
struct TRYLLCLIENT_API FTryllNodeEventKeyValue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Tryll")
	FString Key;

	UPROPERTY(BlueprintReadOnly, Category = "Tryll")
	FString Value;
};

/** Which typed NodeEvent the shared decoder recognised. */
enum class ETryllNodeEventKind : uint8
{
	ToolCall,
	IntentClassified,
	IntentLlmClassified,
	Paused,
	Unknown,
};

/**
 * Listener slots on one agent. The component's probe reports these from
 * Blueprint IsBound(); a raw FTryllAgent reports them from its callback slots.
 * ToolCallDetailed suppresses the agent's generic OnNodeEvent (C++ AgentProxy
 * parity) even though the session-level detailed event does not.
 */
enum class ETryllAgentNodeEventKind : uint8
{
	ToolCall,
	ToolCallWithId,
	ToolCallDetailed,
	IntentClassified,
	IntentLlmClassified,
	Paused,
	NodeEvent,
};

/** Owning decode of one NodeEvent. Strings outlive the inbound frame. */
struct TRYLLCLIENT_API FTryllDecodedNodeEvent
{
	ETryllNodeEventKind Kind = ETryllNodeEventKind::Unknown;
	FString NodeName;
	FString EventType;
	FTryllToolCall ToolCall;
	FTryllIntentClassification Intent;
	FTryllIntentLlmClassification IntentLlm;
	/** Pause only: the paused node's pending exit route (wire key "exit_route"). */
	FString PendingExit;
	TArray<FTryllNodeEventKeyValue> KvPairs;
};

/**
 * Parse a NodeEvent payload once. Both dispatch sites (UTryllSubsystem and
 * UTryllEditorSubsystem) call this, then broadcast their own session delegates
 * and hand the result to FTryllAgent::HandleNodeEvent.
 */
TRYLLCLIENT_API FTryllDecodedNodeEvent TryllDecodeNodeEvent(
	const FString& NodeName,
	const FString& EventType,
	const TArray<TPair<FString, FString>>& WirePairs);
