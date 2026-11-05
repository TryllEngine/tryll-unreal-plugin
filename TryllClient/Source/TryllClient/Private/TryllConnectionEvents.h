// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.
//
// Broadcast-class results for one connection. Caller-only results, including
// GetMemoryConsumption, are not members: they stay on the caller's completion.

#pragma once

#include "TryllEditorBridge.h"

class TRYLLCLIENT_API FTryllConnectionEvents
{
public:
#define TRYLL_CONNECTION_EVENT(Member, DelegateType, ForwardMethod) DelegateType Member;
#include "TryllConnectionEvents.inl"
};
