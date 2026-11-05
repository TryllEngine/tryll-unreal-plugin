// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FTryllObservationEnvelope;

/** Raw JSON for one journaled frame, shown under the Agent Log inspector. */
class STryllAgentLogPayloadView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllAgentLogPayloadView) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void Clear();
	void SetSource(const FTryllObservationEnvelope& Envelope);
};
