// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllDialogLabDialogues.h"

#include "TryllDialogScriptAsset.h"
#include "TryllDialogSuiteAsset.h"

bool FTryllDialogLabDialogues::IsAcceptedSource(const UObject* Source)
{
	return Cast<UTryllDialogScriptAsset>(Source) != nullptr
		|| Cast<UTryllDialogSuiteAsset>(Source) != nullptr;
}

bool FTryllDialogLabDialogues::TryResolve(const UObject* Source,
	TArray<const UTryllDialogScriptAsset*>& OutScripts, FString& OutError)
{
	OutScripts.Reset();
	OutError.Reset();
	if (Source == nullptr)
	{
		OutError = TEXT("Assign a Dialog Script or Dialog Suite.");
		return false;
	}
	if (const UTryllDialogScriptAsset* Script = Cast<UTryllDialogScriptAsset>(Source))
	{
		OutScripts.Add(Script);
		return true;
	}
	if (const UTryllDialogSuiteAsset* Suite = Cast<UTryllDialogSuiteAsset>(Source))
	{
		for (const TObjectPtr<UTryllDialogScriptAsset>& Entry : Suite->Dialogues)
		{
			if (Entry != nullptr)
			{
				OutScripts.Add(Entry.Get());
			}
		}
		if (OutScripts.Num() == 0)
		{
			OutError = FString::Printf(TEXT("Dialog Suite '%s' has no dialog scripts."), *Suite->GetName());
			return false;
		}
		return true;
	}
	OutError = FString::Printf(TEXT("'%s' is a %s; drop a Dialog Script or Dialog Suite."),
		*Source->GetName(), *Source->GetClass()->GetName());
	return false;
}

FString FTryllDialogLabDialogues::Prefix(const int32 Index, const FString& Name, const int32 Count)
{
	return Count <= 1 ? FString() : FString::Printf(TEXT("Dialogue %d '%s': "), Index + 1, *Name);
}
