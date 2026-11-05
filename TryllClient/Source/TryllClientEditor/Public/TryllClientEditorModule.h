// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

TRYLLCLIENTEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogTryllEditor, Log, All);

class FAgentLogStore;

class FTryllClientEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static TRYLLCLIENTEDITOR_API FAgentLogStore& GetAgentLogStore();

	static const FName ChatTabName;
	static const FName ModelManagerTabName;
	static const FName DialogLabTabName;
	static const FName AgentLogTabName;
	static const FName MemoryTabName;
	static const FName KnowledgeTabName;
	static const FName WelcomeTabName;
	static const FName HelpTabName;

	/**
	 * Compare-in-Lab: open the Dialog Lab on a chat's agent and history. Routed through the module
	 * because the chat panel has no handle on the Lab panel, and the Lab tab may not exist yet —
	 * an already-open tab is seeded directly, otherwise the capture waits for its Construct.
	 */
	void OpenDialogLabFromChat(
		class UTryllAgentComponent* SourceAgent,
		const TArray<struct FTryllChatHistoryEntry>& History);

private:
	void RegisterMenus();
	void RegisterStoragePathCustomizations();
	void UnregisterStoragePathCustomizations();
	void RegisterWorkflowGraphEditor();
	void UnregisterWorkflowGraphEditor();
	TSharedRef<class SDockTab> SpawnChatTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnModelManagerTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnDialogLabTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnAgentLogTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnMemoryTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnKnowledgeTab(const class FSpawnTabArgs& Args);

	bool bStoragePathPropertyLayoutRegistered = false;

	/** Content Browser asset actions for UTryllWorkflowAsset (double-click → graph editor). */
	TSharedPtr<class FAssetTypeActions_TryllWorkflow> WorkflowAssetTypeActions;

	/** Panel factory mapping UTryllEdGraphNode → SGraphNode_Tryll. */
	TSharedPtr<class FTryllGraphNodeFactory> WorkflowGraphNodeFactory;

	/** The live Dialog Lab panel, if its tab is open. Weak: the tab owns it, this only observes. */
	TWeakPtr<class STryllDialogLabPanel> DialogLabPanel;

	/** A capture waiting for the Lab panel to be constructed, consumed exactly once. */
	TOptional<TPair<TWeakObjectPtr<class UTryllAgentComponent>,
		TArray<struct FTryllChatHistoryEntry>>> PendingChatCapture;

	TSharedPtr<class FAgentLogStore> AgentLogStore;
};
