// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "ToolsetRegistry/UToolsetRegistry.h"
#include "TryllWorkflowToolset.h"

/**
 * Toolsets are not discovered by reflection — each one registers itself.
 * Registration is deferred to OnAllModuleLoadingPhasesComplete because
 * RegisterToolsetClass silently no-ops while the registry's editor subsystem is
 * still coming up, which it is when a PostEngineInit module starts.
 */
class FTryllMCPModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddRaw(
			this, &FTryllMCPModule::OnAllModuleLoadingPhasesComplete);
		FCoreDelegates::OnPreExit.AddRaw(this, &FTryllMCPModule::OnPreExit);
	}

	virtual void ShutdownModule() override
	{
		FCoreDelegates::OnAllModuleLoadingPhasesComplete.RemoveAll(this);
		FCoreDelegates::OnPreExit.RemoveAll(this);
	}

private:
	void OnAllModuleLoadingPhasesComplete()
	{
		UToolsetRegistry::RegisterToolsetClass(UTryllWorkflowToolset::StaticClass());
	}

	void OnPreExit()
	{
		UToolsetRegistry::UnregisterToolsetClass(UTryllWorkflowToolset::StaticClass());
	}
};

IMPLEMENT_MODULE(FTryllMCPModule, TryllMCP)
