// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SSplitter;

DECLARE_DELEGATE_OneParam(FOnTryllSplitStoredWidth, float);

/**
 * Two-pane split that owns §4.2: fixed side, persist, collapse-guard,
 * clamp against the flex min, and width-0 collapse (never hide the child).
 */
class STryllInspectorSplit : public SCompoundWidget
{
public:
	enum class EFixedSide : uint8
	{
		First,
		Second,
	};

	SLATE_BEGIN_ARGS(STryllInspectorSplit)
		: _FixedSide(EFixedSide::Second)
		, _PrimaryMin(0.0f)
		, _SecondaryMin(0.0f)
		, _StoredWidth(0.0f)
		, _IsFixedPaneVisible(true)
	{
	}
		SLATE_ARGUMENT(EFixedSide, FixedSide)
		SLATE_ATTRIBUTE(float, PrimaryMin)
		SLATE_ATTRIBUTE(float, SecondaryMin)
		SLATE_ATTRIBUTE(float, StoredWidth)
		SLATE_EVENT(FOnTryllSplitStoredWidth, OnStoredWidthChanged)
		SLATE_ATTRIBUTE(bool, IsFixedPaneVisible)
		SLATE_NAMED_SLOT(FArguments, PrimaryContent)
		SLATE_NAMED_SLOT(FArguments, SecondaryContent)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	float GetSplitWidth() const;

private:
	float GetFixedMin() const;
	float GetFlexMin() const;
	float GetClampedFixedWidth() const;
	bool IsFixedVisible() const;
	void PersistResizedWidth(float NewWidth);

	EFixedSide FixedSide = EFixedSide::Second;
	TAttribute<float> PrimaryMin;
	TAttribute<float> SecondaryMin;
	TAttribute<float> StoredWidth;
	FOnTryllSplitStoredWidth OnStoredWidthChanged;
	TAttribute<bool> IsFixedPaneVisible;
	TSharedPtr<SSplitter> Splitter;
};
