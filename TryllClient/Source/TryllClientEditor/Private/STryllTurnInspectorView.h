// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#include "TryllDialogLabIntent.h"
#include "TryllDialogLabRetrieval.h"
#include "TryllTurnInspectorModels.h"

class FJsonObject;
class SScrollBox;
class SVerticalBox;

/** One RetrieveNode execution in a turn. A node re-entered within a turn yields two of these. */
struct FTryllLabRetrievalExecution
{
	int32   NodeIndex = 0;
	FString NodeName;
	FString Query;
	FString Mode = TEXT("Dense");
	double  Threshold = 0.0;
	int32   RrfK = 60;
	TArray<FTryllLabRetrievalHit> Hits;
};

/**
 * Shared per-turn diagnostics panel used by Dialog Lab and Chat. Driven by one Render() call
 * carrying a neutral FTryllTurnInspection — Lab pins/checks/diff arrive only via optional
 * FTryllTurnLabContext.
 */
class STryllTurnInspectorView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllTurnInspectorView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Draw one turn. Pass an empty TurnKey for "no selection / no run yet". Lab chrome
	 * (pins, checks, diff) appears only when Turn.Lab is set and populated.
	 */
	void Render(const FTryllTurnInspection& Turn);

private:
	TSharedRef<SWidget> BuildPinSlot(const FString& Key, const FString& Value, const FLinearColor& Accent);
	TSharedRef<SWidget> BuildSection(
		const FString& Title, const FString& Trailing, bool bInitiallyOpen,
		TSharedRef<SWidget> Body, TFunction<FString()> CopyText = nullptr);
	TSharedRef<SWidget> BuildRouteBody() const;
	TSharedRef<SWidget> BuildTimingBody() const;
	TSharedRef<SWidget> BuildParametersBody() const;
	TSharedRef<SWidget> BuildChecksBody() const;
	TSharedRef<SWidget> BuildPromptBody(
		const FString& Mine, const FTryllTurnInspection* Peer) const;
	TSharedRef<SWidget> BuildSegmentHeading(
		const struct FTryllLabPromptSegment& Segment, const FString& Suffix) const;
	TSharedRef<SWidget> BuildDiffLines(const TArray<struct FTryllLabDiffLine>& DiffLines) const;
	TSharedRef<SWidget> BuildRetrievalBody(
		const TArray<FTryllLabRetrievalExecution>& Executions) const;
	static FTryllLabRetrievalExecution MakeRetrievalExecution(
		int32 NodeIndex, const TSharedPtr<FJsonObject>& Node);
	TSharedRef<SWidget> BuildIntentBody(
		const TArray<FTryllIntentExecution>& Executions) const;
	static FTryllIntentExecution MakeIntentExecution(
		int32 NodeIndex, const TSharedPtr<FJsonObject>& Node);
	TSharedRef<SWidget> BuildToolCallsBody() const;
	TSharedRef<SWidget> BuildMuted(const FString& Text) const;
	TSharedRef<SWidget> BuildCodeBlock(const FString& Text, float MaxHeight) const;
	void AddRawJsonSection();

	TSharedPtr<SVerticalBox> PinRow;
	TSharedPtr<SScrollBox>   Body;
	TSharedPtr<class STextBlock> HintText;

	void RebuildInspector();

	TArray<TPair<int32, TSharedPtr<class FJsonObject>>> ScopedNodes(
		const TSharedPtr<class FJsonObject>& DebugInfo) const;
	TArray<struct FTryllLabPromptSegment> ScopedSegments(
		const TArray<struct FTryllLabPromptSegment>& All) const;
	TArray<struct FTryllLabPromptSegment> MatchingSegments(
		const TArray<struct FTryllLabPromptSegment>& Theirs,
		const TArray<struct FTryllLabPromptSegment>& Mine) const;
	const FTryllTurnRouteNode* ScopedRouteNode() const;

	FTryllTurnInspection Turn;

	/** Index into debug_info.nodes[] the panel is narrowed to, or -1 for the whole turn. */
	int32 NodeScope = -1;
	/** The turn the scope was chosen against; a different turn drops it. */
	FString ScopeKey;
};
