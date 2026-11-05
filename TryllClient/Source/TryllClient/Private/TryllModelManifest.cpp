// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllModelManifest.h"

namespace TryllModelManifestPrivate
{
	UTryllModelManifest* GActiveOverride = nullptr;
}

UTryllModelManifest* UTryllModelManifest::GetMutableActive()
{
	if (TryllModelManifestPrivate::GActiveOverride)
	{
		return TryllModelManifestPrivate::GActiveOverride;
	}
	return GetMutableDefault<UTryllModelManifest>();
}

void UTryllModelManifest::SetActiveOverrideForTests(UTryllModelManifest* Override)
{
	TryllModelManifestPrivate::GActiveOverride = Override;
}

bool UTryllModelManifest::SaveToConfig()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		return false;
	}
	return TryUpdateDefaultConfigFile();
}
