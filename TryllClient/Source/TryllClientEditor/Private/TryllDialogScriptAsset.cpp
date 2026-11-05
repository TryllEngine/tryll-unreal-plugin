// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogScriptAsset.h"

#include "TryllDialogLabScriptData.h"
#include "Serialization/CustomVersion.h"

const FGuid FTryllDialogScriptVersion::GUID(0x6E2F1C4B, 0x91D24A37, 0xB8E05C13, 0x4D7A9F26);

// Registers the version so packages record it; the editor module loads at PostEngineInit,
// before any Dialog Script package is opened.
static FCustomVersionRegistration GRegisterTryllDialogScriptVersion(
	FTryllDialogScriptVersion::GUID,
	FTryllDialogScriptVersion::LatestVersion,
	TEXT("TryllDialogScriptVersion"));

void UTryllDialogScriptAsset::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FTryllDialogScriptVersion::GUID);
	Super::Serialize(Ar);
}

void UTryllDialogScriptAsset::PostLoad()
{
	Super::PostLoad();
	if (GetLinkerCustomVersion(FTryllDialogScriptVersion::GUID) < FTryllDialogScriptVersion::TypedChecks
		&& MigrateLegacyChecks())
	{
		// Saved by ResavePackages -OnlySaveDirtyPackages, or the next time it is saved by hand.
		MarkPackageDirty();
	}
}

bool UTryllDialogScriptAsset::MigrateLegacyChecks()
{
	bool bChanged = false;
	for (int32 Index = 0; Index < Steps.Num(); ++Index)
	{
		FTryllDialogScriptStep& Step = Steps[Index];
		for (const FTryllLabCheckSpec& Spec : FTryllLabCheckSpec::FromLegacyStep(Step, Index >= SeedTurnCount))
		{
			if (UTryllDialogCheck* Check = Spec.ToCheck(this))
			{
				Step.Checks.Add(Check);
				bChanged = true;
			}
		}
		bChanged |= Step.MustContain.Num() > 0 || Step.MustNotContain.Num() > 0
			|| Step.RegexForbid.Num() > 0 || Step.MaxWords > 0
			|| Step.IntentChecks.Num() > 0 || Step.AttributeChecks.Num() > 0;
		Step.MustContain.Reset();
		Step.MustNotContain.Reset();
		Step.RegexForbid.Reset();
		Step.MaxWords = 0;
		Step.IntentChecks.Reset();
		Step.AttributeChecks.Reset();
	}
	for (const FTryllLabCheckSpec& Spec : FTryllLabCheckSpec::FromLegacyGlobal(GlobalRegexForbid))
	{
		if (UTryllDialogCheck* Check = Spec.ToCheck(this))
		{
			GlobalChecks.Add(Check);
			bChanged = true;
		}
	}
	GlobalRegexForbid.Reset();
	return bChanged;
}
