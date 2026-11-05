// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Chip showing the deployment mode the connected server reported in ConnectionReady,
 * only when it is not the default client-dev, so customers never see it. Reads the
 * editor subsystem, then any PIE game subsystem. Collapsed while disconnected or when
 * the server predates the field. Tryll Dev is tinted as a warning: telemetry is off.
 */
class TRYLLCLIENTEDITOR_API STryllServerModeBadge : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllServerModeBadge) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Text for Mode, or empty when the badge should collapse. */
	static FString Caption(bool bConnected, const FString& Mode);

private:
	FString CurrentCaption() const;
	EVisibility GetVisibility() const;
	const struct FSlateBrush* GetBrush() const;
	FSlateColor GetTextColor() const;
};
