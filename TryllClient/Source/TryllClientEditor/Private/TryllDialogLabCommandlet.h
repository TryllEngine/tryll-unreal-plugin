// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "TryllDialogLabCommandlet.generated.h"

/**
 * Headless Dialog Lab for CI (Unity: TryllDialogLabBatch.Run):
 *
 *   UnrealEditor-Cmd.exe <project>.uproject -run=TryllDialogLab -tryllLabConfig=<run.json>
 *     -nullrhi -unattended
 *
 * Exit code 0 = thresholds met, 1 = a threshold missed, 2 = the run itself failed. Results go to
 * the config's output_dir as <run_id>.jsonl and <run_id>.summary.json.
 */
UCLASS()
class UTryllDialogLabCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UTryllDialogLabCommandlet();

	virtual int32 Main(const FString& Params) override;
};
