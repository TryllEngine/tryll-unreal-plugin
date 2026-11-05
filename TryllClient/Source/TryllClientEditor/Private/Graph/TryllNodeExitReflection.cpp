// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Graph/TryllNodeExitReflection.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "UObject/UnrealType.h"

namespace
{
	const FName ExitMetaKey(TEXT("TryllExit"));
}

TArray<FTryllNodeExit> FTryllNodeExitReflection::GetExitsForClass(const UClass* ParamClass)
{
	TArray<FTryllNodeExit> Exits;
	if (!ParamClass)
	{
		return Exits;
	}

#if WITH_EDITOR
	for (TFieldIterator<FProperty> It(ParamClass); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop->HasMetaData(ExitMetaKey))
		{
			continue;
		}
		if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
		{
			FTryllNodeExit Exit;
			Exit.ExitName = Prop->GetMetaData(ExitMetaKey);
			Exit.Prop = StrProp;
			Exits.Add(Exit);
		}
	}
	// TFieldIterator yields these exits in schema declaration order
	// (e.g. tool_called then no_tool_called), which is the order we want pins to appear.
#endif

	return Exits;
}

TArray<FTryllNodeExit> FTryllNodeExitReflection::GetExits(const UTryllNodeParamsBase* Params)
{
	return GetExitsForClass(Params ? Params->GetClass() : nullptr);
}

FString FTryllNodeExitReflection::ReadTarget(const UTryllNodeParamsBase* Params, const FTryllNodeExit& Exit)
{
	if (!Params || !Exit.Prop)
	{
		return FString();
	}
	return Exit.Prop->GetPropertyValue_InContainer(Params);
}

void FTryllNodeExitReflection::WriteTarget(UTryllNodeParamsBase* Params, const FTryllNodeExit& Exit, const FString& Target)
{
	if (!Params || !Exit.Prop)
	{
		return;
	}
	Exit.Prop->SetPropertyValue_InContainer(Params, Target);
}
