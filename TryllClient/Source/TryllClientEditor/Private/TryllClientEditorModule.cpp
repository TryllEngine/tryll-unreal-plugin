// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllClientEditorModule.h"
#include "AgentLog/AgentLogStore.h"
#include "STryllChatPanel.h"
#include "STryllModelManagerPanel.h"
#include "STryllDialogLabPanel.h"
#include "STryllAgentLogPanel.h"
#include "STryllMemoryPanel.h"
#include "STryllKnowledgePanel.h"
#include "TryllDialogLabCompareFromChat.h"
#include "TryllStoragePathCustomization.h"
#include "TryllModelNameCustomization.h"
#include "TryllSlotInputCustomization.h"
#include "TryllSlotNameCustomization.h"
#include "TryllNodeDescriptionCustomization.h"
#include "TryllLocalizablePreview.h"
#include "STryllTextEditWindow.h"
#include "TryllLogsFolder.h"
#include "Generated/Nodes/TryllClassifyAttributeChoice.h"
#include "Generated/Nodes/TryllClassifyAttributeDef.h"
#include "Generated/Nodes/TryllClassifyIntentEntry.h"
#include "Generated/Nodes/TryllToolDefinition.h"
#include "Generated/Nodes/TryllToolParamDefinition.h"
#include "TryllClassifyAttributeDefCustomization.h"
#include "TryllNestedStructCustomization.h"
#include "TryllVariableDeclCustomization.h"
#include "TryllVariableDecl.h"
#include "TryllAgentComponent.h"
#include "TryllAgentComponentCustomization.h"
#include "TryllVoiceProfile.h"
#include "TryllVoiceProfileCustomization.h"
#include "TryllVoiceProfileFactory.h"
#include "TryllRuntimeSettings.h"
#include "TryllRuntimeSettingsCustomization.h"
#include "TryllPIEModelGuard.h"
#include "TryllAgentLogReloadFlag.h"
#include "UI/TryllEditorStyle.h"
#include "Welcome/STryllHelpPanel.h"
#include "Welcome/STryllWelcomePanel.h"
#include "Welcome/TryllInstallEvent.h"
#include "Welcome/TryllWelcomeStartup.h"
#include "TryllWorkflowAssetFactory.h"
#include "TryllDialogScriptAssetFactory.h"
#include "TryllDialogSuiteAssetFactory.h"
#include "Graph/AssetTypeActions_TryllWorkflow.h"
#include "Graph/SGraphNode_Tryll.h"
#if TRYLL_DEVELOPER_TOOLS
#include "Developer/TryllDeveloperMenu.h"
#endif

#include "Framework/Docking/TabManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "EdGraphUtilities.h"
#include "Misc/CoreMisc.h"

#define LOCTEXT_NAMESPACE "TryllClientEditor"

DEFINE_LOG_CATEGORY(LogTryllEditor);

const FName FTryllClientEditorModule::ChatTabName(TEXT("TryllChat"));
const FName FTryllClientEditorModule::ModelManagerTabName(TEXT("TryllModelManager"));
const FName FTryllClientEditorModule::DialogLabTabName(TEXT("TryllDialogLab"));
const FName FTryllClientEditorModule::AgentLogTabName(TEXT("TryllAgentLog"));
const FName FTryllClientEditorModule::MemoryTabName(TEXT("TryllMemory"));
const FName FTryllClientEditorModule::KnowledgeTabName(TEXT("TryllKnowledge"));
const FName FTryllClientEditorModule::WelcomeTabName(TEXT("TryllWelcome"));
const FName FTryllClientEditorModule::HelpTabName(TEXT("TryllHelp"));

FAgentLogStore& FTryllClientEditorModule::GetAgentLogStore()
{
	FTryllClientEditorModule& Module =
		FModuleManager::LoadModuleChecked<FTryllClientEditorModule>("TryllClientEditor");
	check(Module.AgentLogStore.IsValid());
	return *Module.AgentLogStore;
}

void FTryllClientEditorModule::StartupModule()
{
	FTryllEditorStyle::RefreshHostColors();
	FTryllEditorStyle::BindEditorTheme();

	AgentLogStore = MakeShared<FAgentLogStore>();

	if (FTryllAgentLogReloadFlag::ConsumeIfCurrentProcess())
	{
		AgentLogStore->MarkHistoryDiscardedByReload();
	}

	RegisterStoragePathCustomizations();
	FTryllPIEModelGuard::Register();
#if TRYLL_DEVELOPER_TOOLS
	FTryllDeveloperMenu::ApplyStoredMode();
#endif

	// Nomad tabs are opened from Window → Tryll ▸ (RegisterMenus). Do not SetGroup on a
	// WorkspaceMenu item — that also lists them flat under Window and duplicates the submenu.
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		WelcomeTabName,
		FOnSpawnTab::CreateStatic(&STryllWelcomePanel::SpawnTab))
		.SetDisplayName(LOCTEXT("TryllWelcomeTabTitle", "Tryll Welcome"))
		.SetTooltipText(LOCTEXT("TryllWelcomeTabTooltip", "Accept the Tryll Runtime Licence, check the server, and find the docs and community."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		HelpTabName,
		FOnSpawnTab::CreateStatic(&STryllHelpPanel::SpawnTab))
		.SetDisplayName(LOCTEXT("TryllHelpTabTitle", "Tryll Help & Community"))
		.SetTooltipText(LOCTEXT("TryllHelpTabTooltip", "Versions, updates, docs, support and community links."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ChatTabName,
		FOnSpawnTab::CreateRaw(this, &FTryllClientEditorModule::SpawnChatTab))
		.SetDisplayName(LOCTEXT("TryllChatTabTitle", "Tryll Chat"))
		.SetTooltipText(LOCTEXT("TryllChatTabTooltip", "Editor chat window for testing a scene Tryll agent component."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ModelManagerTabName,
		FOnSpawnTab::CreateRaw(this, &FTryllClientEditorModule::SpawnModelManagerTab))
		.SetDisplayName(LOCTEXT("TryllModelManagerTabTitle", "Tryll Model Manager"))
		.SetTooltipText(LOCTEXT("TryllModelManagerTabTooltip", "Manage Tryll models: list, download, and select for the build."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		DialogLabTabName,
		FOnSpawnTab::CreateRaw(this, &FTryllClientEditorModule::SpawnDialogLabTab))
		.SetDisplayName(LOCTEXT("TryllDialogLabTabTitle", "Tryll Dialog Lab"))
		.SetTooltipText(LOCTEXT("TryllDialogLabTabTooltip", "Replay a dialog script across variants and repeats, and compare the spread."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		AgentLogTabName,
		FOnSpawnTab::CreateRaw(this, &FTryllClientEditorModule::SpawnAgentLogTab))
		.SetDisplayName(LOCTEXT("TryllAgentLogTabTitle", "Tryll Agent Log"))
		.SetTooltipText(LOCTEXT("TryllAgentLogTabTooltip", "Cross-agent observation timeline with per-row inspectors."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		MemoryTabName,
		FOnSpawnTab::CreateRaw(this, &FTryllClientEditorModule::SpawnMemoryTab))
		.SetDisplayName(LOCTEXT("TryllMemoryTabTitle", "Tryll Memory"))
		.SetTooltipText(LOCTEXT("TryllMemoryTabTooltip", "Live server-wide RAM/VRAM snapshot. Refresh only, no graphs."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		KnowledgeTabName,
		FOnSpawnTab::CreateRaw(this, &FTryllClientEditorModule::SpawnKnowledgeTab))
		.SetDisplayName(LOCTEXT("TryllKnowledgeTabTitle", "Tryll Knowledge"))
		.SetTooltipText(LOCTEXT("TryllKnowledgeTabTooltip", "Inspect session mutable knowledge bases. Refresh only."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTryllClientEditorModule::RegisterMenus));

	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		UTryllRuntimeSettings::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTryllRuntimeSettingsCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(
		UTryllAgentComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTryllAgentComponentCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(
		UTryllVoiceProfile::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTryllVoiceProfileCustomization::MakeInstance));
	FTryllLocalizablePreview::RegisterCustomizations();
	STryllTextEditWindow::RegisterRowExtension();
	PropertyModule.NotifyCustomizationModuleChanged();

	RegisterWorkflowGraphEditor();

	// The Welcome tab at startup (once per session, interactive editors only), and
	// the install event once the licence is accepted.
	FTryllInstallEvent::Register();
	FTryllWelcomeStartup::Register();
}

void FTryllClientEditorModule::ShutdownModule()
{
	// R9.4 only applies to in-process hot reload (agents may still be live with an
	// empty log). A full editor exit must not leave a sticky "history discarded"
	// flag — Unity keeps this in SessionState which dies with the process.
	if (IsEngineExitRequested())
	{
		FTryllAgentLogReloadFlag::Write(false);
	}
	else if (AgentLogStore.IsValid() && AgentLogStore->GetEnvelopeCount() > 0)
	{
		FTryllAgentLogReloadFlag::Write(true);
	}

	if (AgentLogStore.IsValid())
	{
		AgentLogStore->Clear();
		AgentLogStore->Detach();
		AgentLogStore.Reset();
	}

	FTryllWelcomeStartup::Unregister();
	FTryllInstallEvent::Unregister();
	FTryllEditorStyle::UnbindEditorTheme();
	FTryllPIEModelGuard::Unregister();
	UnregisterStoragePathCustomizations();
	UnregisterWorkflowGraphEditor();

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(WelcomeTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(HelpTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ChatTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ModelManagerTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DialogLabTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AgentLogTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MemoryTabName);
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(KnowledgeTabName);
	}

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(
			UTryllRuntimeSettings::StaticClass()->GetFName());
		PropertyModule.UnregisterCustomClassLayout(
			UTryllAgentComponent::StaticClass()->GetFName());
		PropertyModule.UnregisterCustomClassLayout(
			UTryllVoiceProfile::StaticClass()->GetFName());
		FTryllLocalizablePreview::UnregisterCustomizations();
		STryllTextEditWindow::UnregisterRowExtension();
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FTryllClientEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Window");
	// Section heading "Tryll" (separator) after Layout; one hover submenu under it.
	FToolMenuSection& Section = WindowMenu->FindOrAddSection(
		"Tryll",
		LOCTEXT("TryllSection", "Tryll"),
		FToolMenuInsert("WindowLayout", EToolMenuInsertType::After));
	Section.AddSubMenu(
		"Tryll",
		LOCTEXT("TryllSubMenu", "Tryll"),
		LOCTEXT("TryllSubMenuTooltip", "Tryll editor windows."),
		FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
		{
			FToolMenuSection& Windows = SubMenu->AddSection("TryllWindows");

			Windows.AddMenuEntry(
				"OpenTryllWelcome",
				LOCTEXT("OpenTryllWelcome", "Tryll Welcome"),
				LOCTEXT("OpenTryllWelcomeTooltip", "Open the Tryll Welcome window: licence, server and community links."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::WelcomeTabName);
				})));

			Windows.AddMenuEntry(
				"OpenTryllHelp",
				LOCTEXT("OpenTryllHelp", "Tryll Help & Community"),
				LOCTEXT("OpenTryllHelpTooltip", "Open the Tryll Help & Community window: versions, updates, docs and community links."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::HelpTabName);
				})));

			Windows.AddMenuEntry(
				"OpenTryllChat",
				LOCTEXT("OpenTryllChat", "Tryll Chat"),
				LOCTEXT("OpenTryllChatTooltip", "Open the Tryll editor chat window."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::ChatTabName);
				})));

			Windows.AddMenuEntry(
				"OpenTryllModelManager",
				LOCTEXT("OpenTryllModelManager", "Tryll Model Manager"),
				LOCTEXT("OpenTryllModelManagerTooltip", "Open the Tryll model manager window."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::ModelManagerTabName);
				})));

			Windows.AddMenuEntry(
				"OpenTryllDialogLab",
				LOCTEXT("OpenTryllDialogLab", "Tryll Dialog Lab"),
				LOCTEXT("OpenTryllDialogLabTooltip", "Open the Tryll Dialog Lab window."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::DialogLabTabName);
				})));

			Windows.AddMenuEntry(
				"OpenTryllAgentLog",
				LOCTEXT("OpenTryllAgentLog", "Tryll Agent Log"),
				LOCTEXT("OpenTryllAgentLogTooltip", "Open the Tryll Agent Log window."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::AgentLogTabName);
				})));

			Windows.AddMenuEntry(
				"OpenTryllMemory",
				LOCTEXT("OpenTryllMemory", "Tryll Memory"),
				LOCTEXT("OpenTryllMemoryTooltip", "Open the Tryll Memory window."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::MemoryTabName);
				})));

			Windows.AddMenuEntry(
				"OpenTryllKnowledge",
				LOCTEXT("OpenTryllKnowledge", "Tryll Knowledge"),
				LOCTEXT("OpenTryllKnowledgeTooltip", "Open the Tryll Knowledge window."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FTryllClientEditorModule::KnowledgeTabName);
				})));

			Windows.AddMenuEntry(
				"OpenTryllLogsFolder",
				LOCTEXT("OpenTryllLogsFolder", "Open Tryll Logs Folder"),
				LOCTEXT("OpenTryllLogsFolderTooltip", "Open the folder with this project's Tryll server logs and crash dumps."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&TryllLogsFolder::Open)));
#if TRYLL_DEVELOPER_TOOLS
			FTryllDeveloperMenu::AddToMenu(SubMenu);
#endif
		}));
}

TSharedRef<SDockTab> FTryllClientEditorModule::SpawnAgentLogTab(const FSpawnTabArgs& /*Args*/)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(STryllAgentLogPanel)
		];
}

TSharedRef<SDockTab> FTryllClientEditorModule::SpawnMemoryTab(const FSpawnTabArgs& /*Args*/)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(STryllMemoryPanel)
		];
}

TSharedRef<SDockTab> FTryllClientEditorModule::SpawnKnowledgeTab(const FSpawnTabArgs& /*Args*/)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(STryllKnowledgePanel)
		];
}

TSharedRef<SDockTab> FTryllClientEditorModule::SpawnChatTab(const FSpawnTabArgs& /*Args*/)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(STryllChatPanel)
		];
}

TSharedRef<SDockTab> FTryllClientEditorModule::SpawnDialogLabTab(const FSpawnTabArgs& /*Args*/)
{
	TSharedRef<STryllDialogLabPanel> Panel = SNew(STryllDialogLabPanel);
	DialogLabPanel = Panel;

	// A capture that arrived before this tab existed is applied now, then discarded so reopening
	// the tab later does not resurrect a stale chat history.
	if (PendingChatCapture.IsSet())
	{
		Panel->OpenFromChat(PendingChatCapture->Key.Get(), PendingChatCapture->Value);
		PendingChatCapture.Reset();
	}

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			Panel
		];
}

void FTryllClientEditorModule::OpenDialogLabFromChat(
	UTryllAgentComponent* SourceAgent, const TArray<FTryllChatHistoryEntry>& History)
{
	// Invoking an already-open tab focuses the existing panel rather than constructing a new one,
	// so the live panel is seeded directly and only an absent one goes through the pending slot.
	if (TSharedPtr<STryllDialogLabPanel> Existing = DialogLabPanel.Pin())
	{
		FGlobalTabmanager::Get()->TryInvokeTab(DialogLabTabName);
		Existing->OpenFromChat(SourceAgent, History);
		return;
	}

	PendingChatCapture = TPair<TWeakObjectPtr<UTryllAgentComponent>, TArray<FTryllChatHistoryEntry>>(
		SourceAgent, History);
	FGlobalTabmanager::Get()->TryInvokeTab(DialogLabTabName);
}

TSharedRef<SDockTab> FTryllClientEditorModule::SpawnModelManagerTab(const FSpawnTabArgs& /*Args*/)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(STryllModelManagerPanel)
		];
}

void FTryllClientEditorModule::RegisterStoragePathCustomizations()
{
	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomPropertyTypeLayout(
		FName(TEXT("TryllStoragePath")),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllStoragePathCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FName(TEXT("TryllModelName")),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllModelNameCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FName(TEXT("TryllSlotInput")),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllSlotInputCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FName(TEXT("TryllSlotName")),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllSlotNameCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FName(TEXT("TryllNodeDescription")),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllNodeDescriptionCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FTryllVariableDecl::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllVariableDeclCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FTryllClassifyAttributeDef::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllClassifyAttributeDefCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FTryllClassifyAttributeChoice::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllNestedStructCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FTryllClassifyIntentEntry::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllNestedStructCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FTryllToolDefinition::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllNestedStructCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FTryllToolParamDefinition::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllNestedStructCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FTryllDeliveryMode::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FTryllDeliveryModeCustomization::MakeInstance));
	bStoragePathPropertyLayoutRegistered = true;

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FTryllClientEditorModule::UnregisterStoragePathCustomizations()
{
	if (!bStoragePathPropertyLayoutRegistered)
	{
		return;
	}

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(FName(TEXT("TryllStoragePath")));
		PropertyModule.UnregisterCustomPropertyTypeLayout(FName(TEXT("TryllModelName")));
		PropertyModule.UnregisterCustomPropertyTypeLayout(FName(TEXT("TryllSlotInput")));
		PropertyModule.UnregisterCustomPropertyTypeLayout(FName(TEXT("TryllSlotName")));
		PropertyModule.UnregisterCustomPropertyTypeLayout(FName(TEXT("TryllNodeDescription")));
		PropertyModule.UnregisterCustomPropertyTypeLayout(FTryllVariableDecl::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FTryllClassifyAttributeDef::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FTryllClassifyAttributeChoice::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FTryllClassifyIntentEntry::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FTryllToolDefinition::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FTryllToolParamDefinition::StaticStruct()->GetFName());
		PropertyModule.UnregisterCustomPropertyTypeLayout(FTryllDeliveryMode::StaticStruct()->GetFName());
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	bStoragePathPropertyLayoutRegistered = false;
}

void FTryllClientEditorModule::RegisterWorkflowGraphEditor()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	const EAssetTypeCategories::Type TryllCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName(TEXT("Tryll")), LOCTEXT("TryllAssetCategory", "Tryll"));
	UTryllWorkflowAssetFactory::SetMenuCategory(TryllCategory);
	UTryllDialogScriptAssetFactory::SetMenuCategory(TryllCategory);
	UTryllDialogSuiteAssetFactory::SetMenuCategory(TryllCategory);
	UTryllVoiceProfileFactory::SetMenuCategory(TryllCategory);

	WorkflowAssetTypeActions = MakeShared<FAssetTypeActions_TryllWorkflow>(TryllCategory);
	AssetTools.RegisterAssetTypeActions(WorkflowAssetTypeActions.ToSharedRef());

	WorkflowGraphNodeFactory = MakeShared<FTryllGraphNodeFactory>();
	FEdGraphUtilities::RegisterVisualNodeFactory(WorkflowGraphNodeFactory);
}

void FTryllClientEditorModule::UnregisterWorkflowGraphEditor()
{
	if (WorkflowGraphNodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(WorkflowGraphNodeFactory);
		WorkflowGraphNodeFactory.Reset();
	}

	if (WorkflowAssetTypeActions.IsValid())
	{
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			AssetTools.UnregisterAssetTypeActions(WorkflowAssetTypeActions.ToSharedRef());
		}
		WorkflowAssetTypeActions.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTryllClientEditorModule, TryllClientEditor)
