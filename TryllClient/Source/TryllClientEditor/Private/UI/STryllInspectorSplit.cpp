// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "UI/STryllInspectorSplit.h"

#include "TryllInspectorSplitMath.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"

void STryllInspectorSplit::Construct(const FArguments& InArgs)
{
	FixedSide = InArgs._FixedSide;
	PrimaryMin = InArgs._PrimaryMin;
	SecondaryMin = InArgs._SecondaryMin;
	StoredWidth = InArgs._StoredWidth;
	OnStoredWidthChanged = InArgs._OnStoredWidthChanged;
	IsFixedPaneVisible = InArgs._IsFixedPaneVisible;

	const TSharedRef<SWidget> Primary = InArgs._PrimaryContent.Widget;
	const TSharedRef<SWidget> Secondary = InArgs._SecondaryContent.Widget;

	const TSharedRef<SWidget> FixedChild =
		FixedSide == EFixedSide::First ? Primary : Secondary;
	const TSharedRef<SWidget> FlexChild =
		FixedSide == EFixedSide::First ? Secondary : Primary;

	const TSharedRef<SWidget> FixedBox =
		SNew(SBox)
		.MinDesiredWidth_Lambda([this]
		{
			return IsFixedVisible() ? GetFixedMin() : 0.0f;
		})
		.WidthOverride_Lambda([this]
		{
			return IsFixedVisible() ? GetClampedFixedWidth() : FTryllInspectorSplitMath::CollapseWidth;
		})
		.Clipping(EWidgetClipping::ClipToBounds)
		[
			FixedChild
		];

	const TSharedRef<SWidget> FlexBox =
		SNew(SBox)
		.MinDesiredWidth_Lambda([this] { return GetFlexMin(); })
		[
			FlexChild
		];

	if (FixedSide == EFixedSide::First)
	{
		SAssignNew(Splitter, SSplitter)
			.Orientation(Orient_Horizontal)
			+ SSplitter::Slot()
				.SizeRule(SSplitter::SizeToContent)
				.Resizable(true)
				.OnSlotResized_Lambda([this](float NewWidth)
				{
					PersistResizedWidth(NewWidth);
				})
				[
					FixedBox
				]
			+ SSplitter::Slot().Value(1.0f)
				[
					FlexBox
				];
	}
	else
	{
		SAssignNew(Splitter, SSplitter)
			.Orientation(Orient_Horizontal)
			+ SSplitter::Slot().Value(1.0f)
				[
					FlexBox
				]
			+ SSplitter::Slot()
				.SizeRule(SSplitter::SizeToContent)
				.Resizable(true)
				.OnSlotResized_Lambda([this](float NewWidth)
				{
					PersistResizedWidth(NewWidth);
				})
				[
					FixedBox
				];
	}

	ChildSlot
	[
		Splitter.ToSharedRef()
	];
}

float STryllInspectorSplit::GetSplitWidth() const
{
	if (!Splitter.IsValid())
	{
		return 0.0f;
	}
	return Splitter->GetCachedGeometry().GetLocalSize().X;
}

float STryllInspectorSplit::GetFixedMin() const
{
	return FixedSide == EFixedSide::First ? PrimaryMin.Get() : SecondaryMin.Get();
}

float STryllInspectorSplit::GetFlexMin() const
{
	return FixedSide == EFixedSide::First ? SecondaryMin.Get() : PrimaryMin.Get();
}

float STryllInspectorSplit::GetClampedFixedWidth() const
{
	return FTryllInspectorSplitMath::ClampFixedPane(
		GetSplitWidth(),
		StoredWidth.Get(),
		GetFixedMin(),
		GetFlexMin(),
		FTryllInspectorSplitMath::DefaultHandleWidth);
}

bool STryllInspectorSplit::IsFixedVisible() const
{
	return IsFixedPaneVisible.Get(true);
}

void STryllInspectorSplit::PersistResizedWidth(const float NewWidth)
{
	if (!IsFixedVisible()
		|| !FTryllInspectorSplitMath::ShouldPersistWidth(NewWidth, GetFixedMin()))
	{
		return;
	}
	const float Clamped = FTryllInspectorSplitMath::ClampFixedPane(
		GetSplitWidth(),
		NewWidth,
		GetFixedMin(),
		GetFlexMin(),
		FTryllInspectorSplitMath::DefaultHandleWidth);
	OnStoredWidthChanged.ExecuteIfBound(Clamped);
}
