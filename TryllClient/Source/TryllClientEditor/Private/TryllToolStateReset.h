// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

class UTryllAgentComponent;

namespace TryllToolStateReset
{
	/**
	 * Tell the tool handlers on Component's actor that a new conversation started. Editor drivers
	 * (Chat, Dialog Lab) run their own agent, not the component's, so none of the component's
	 * delegates fire for them.
	 *
	 * Dispatched by name: every parameterless UFUNCTION named ResetToolState on the owning actor's
	 * components receives it. A stateless handler declares none.
	 */
	void ResetOnOwner(const UTryllAgentComponent* Component);
}
