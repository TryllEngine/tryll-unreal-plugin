// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllToolStateReset.h"

#include "TryllAgentComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/Class.h"

void TryllToolStateReset::ResetOnOwner(const UTryllAgentComponent* Component)
{
	const AActor* Owner = Component != nullptr ? Component->GetOwner() : nullptr;
	if (Owner == nullptr)
	{
		return;
	}

	static const FName ResetFunctionName(TEXT("ResetToolState"));
	for (UActorComponent* Sibling : Owner->GetComponents())
	{
		if (Sibling == nullptr)
		{
			continue;
		}
		UFunction* Reset = Sibling->FindFunction(ResetFunctionName);
		// NumParms guard: a same-named function that takes arguments would read them off a
		// stack frame we never wrote. Dispatch by name only ever means the nullary one.
		if (Reset != nullptr && Reset->NumParms == 0)
		{
			Sibling->ProcessEvent(Reset, nullptr);
		}
	}
}
