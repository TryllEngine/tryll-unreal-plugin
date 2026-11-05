// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

/**
 * Completion captures a UObject as TWeakObjectPtr. If the object is gone the
 * completion does nothing. Fn is invoked as Fn(Live, Args...).
 */
template<typename TObject, typename Functor>
auto MakeWeakCompletion(TObject* Object, Functor&& Fn)
{
	TWeakObjectPtr<TObject> Weak(Object);
	return [Weak, Fn = Forward<Functor>(Fn)](auto&&... Args)
	{
		if (TObject* Live = Weak.Get())
		{
			Fn(Live, Forward<decltype(Args)>(Args)...);
		}
	};
}

/** Same idea for a non-UObject shared engine object (agent, voice input, KB). */
template<typename TObject, typename Functor>
auto MakeWeakSharedCompletion(TSharedPtr<TObject> Object, Functor&& Fn)
{
	TWeakPtr<TObject> Weak(Object);
	return [Weak, Fn = Forward<Functor>(Fn)](auto&&... Args)
	{
		if (TSharedPtr<TObject> Live = Weak.Pin())
		{
			Fn(Live, Forward<decltype(Args)>(Args)...);
		}
	};
}
