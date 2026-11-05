// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.
//
// Weak capture that still works when a test constructs the object on the stack.
// Production agents, voices, and knowledge bases are TSharedPtr-owned, so the
// weak pointer expires when they do. A stack object has no shared owner;
// AsShared() would assert, and the synchronous drain tests need the callback.

#pragma once

#include "Templates/SharedPointer.h"

template<typename T>
struct TTryllPinned
{
	TSharedPtr<T> Held;
	T* Ptr = nullptr;

	explicit operator bool() const { return Ptr != nullptr; }
	T* operator->() const { return Ptr; }
};

template<typename T>
struct TTryllLiveRef
{
	TWeakPtr<T> Weak;
	T* Raw = nullptr;

	TTryllPinned<T> Pin() const
	{
		if (TSharedPtr<T> Pinned = Weak.Pin())
		{
			return TTryllPinned<T>{ Pinned, Pinned.Get() };
		}
		return TTryllPinned<T>{ nullptr, Raw };
	}
};

template<typename T>
TTryllLiveRef<T> CaptureTryllLive(T* Self)
{
	TTryllLiveRef<T> Out;
	if (Self && Self->DoesSharedInstanceExist())
	{
		Out.Weak = Self->AsShared();
	}
	else
	{
		Out.Raw = Self;
	}
	return Out;
}
