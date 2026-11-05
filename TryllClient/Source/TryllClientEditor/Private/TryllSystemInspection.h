// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllObservationTypes.h"

/** Neutral system / gap-row inspector input (R12.5 / R12.8). */
struct FTryllSystemInspection
{
	FString Title;
	FString Hint;

	FString EventLabel;
	FString Detail;
	FString BeforeAfter;
	FString AckSummary;

	uint64    RequestId = 0;
	uint64    AgentId   = 0;
	FDateTime Timestamp;

	bool bIsCaptureGap = false;

	FString AbsenceNote;
};

struct TRYLLCLIENTEDITOR_API FTryllSystemInspectionFactory
{
	static FTryllSystemInspection Empty(const FString& Hint = TEXT("select a row"));

	static FTryllSystemInspection FromAgentLogRow(const FTryllObservationEnvelope& Envelope);

	static FTryllSystemInspection FromCaptureGap(const FTryllObservationEnvelope& Envelope);
};

