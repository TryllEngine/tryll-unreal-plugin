// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "TryllObservationTypes.h"
#include "TryllVariableDecl.h"

struct FTryllConnEvent;

struct TRYLLCLIENT_API FTryllObservationSnapshot
{
	static FTryllObservationEnvelope FromNote(const FTryllObservationNote& Note, uint64 EventId, uint64 Epoch);
	static FTryllObservationEnvelope FromEvent(const FTryllConnEvent& Evt, uint64 EventId, uint64 Epoch);
};

TRYLLCLIENT_API FString TryllSummarizeVariableUpdates(const TArray<FTryllVariableUpdate>& Updates);
