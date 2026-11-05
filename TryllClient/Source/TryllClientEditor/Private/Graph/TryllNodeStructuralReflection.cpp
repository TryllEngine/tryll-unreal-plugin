// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Graph/TryllNodeStructuralReflection.h"
#include "UObject/UnrealType.h"

namespace
{
	const FName StructuralMetaKey(TEXT("TryllStructural"));
}

bool FTryllNodeStructuralReflection::IsStructural(const FProperty* Prop)
{
#if WITH_EDITOR
	return Prop != nullptr && Prop->HasMetaData(StructuralMetaKey);
#else
	return false;
#endif
}

TArray<FProperty*> FTryllNodeStructuralReflection::GetStructuralProperties(const UClass* ParamClass)
{
	TArray<FProperty*> Props;
	if (!ParamClass)
	{
		return Props;
	}

#if WITH_EDITOR
	for (TFieldIterator<FProperty> It(ParamClass); It; ++It)
	{
		FProperty* Prop = *It;
		if (IsStructural(Prop))
		{
			Props.Add(Prop);
		}
	}
#endif

	return Props;
}
