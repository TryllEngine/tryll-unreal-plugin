// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "TryllAgentLogDocument.h"

class FAgentLogStore;

/** Editor snapshot of the journal. The document writer itself lives in TryllClient. */
class FTryllAgentLogWriteStore
{
public:
	static FString WriteStore(const FAgentLogStore& Store, FTryllAgentLogExportInfo Info);
};
