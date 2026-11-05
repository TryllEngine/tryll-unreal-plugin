// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "Graph/TryllWorkflowGraphCustomizations.h"
#include "Graph/TryllWorkflowAssetEditor.h"
#include "Graph/TryllEdGraphNode.h"

#include "TryllWorkflowAsset.h"
#include "TryllLocalizablePreview.h"
#include "TryllGraphDescription.h"
#include "TryllRenamePropagation.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "HAL/PlatformProcess.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TryllWorkflowGraphCustomizations"

// ── Node-params: group categories + hide exit dropdowns ──────────────────────────

TSharedRef<IDetailCustomization> FTryllNodeParamsGraphCustomization::MakeInstance()
{
	return MakeShared<FTryllNodeParamsGraphCustomization>();
}

void FTryllNodeParamsGraphCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() == 0 || !Objects[0].IsValid())
	{
		return;
	}

	UClass* Class = Objects[0]->GetClass();
	for (TFieldIterator<FProperty> It(Class); It; ++It)
	{
		FProperty* Prop = *It;
		if (Prop->HasMetaData(TEXT("TryllExit")))
		{
			DetailBuilder.HideProperty(Prop->GetFName(), Class);
		}
	}

	FTryllLocalizablePreview::AddPreviews(DetailBuilder, Objects[0].Get());

	// Generated params put every field in a `group` Category (nodes.fbs). Name and help live
	// in the asset editor's header bar, so the panel is one flat run of group categories.
	DetailBuilder.HideCategory(TEXT("Hidden"));
	DetailBuilder.HideCategory(TEXT("Exits"));
	DetailBuilder.EditCategory(TEXT("Advanced")).InitiallyCollapsed(true);
	DetailBuilder.SortCategories([](const TMap<FName, IDetailCategoryBuilder*>& Categories)
	{
		static const FName Order[] = {
			TEXT("Input"), TEXT("Model"), TEXT("Prompt"), TEXT("Storage"),
			TEXT("Behavior"), TEXT("Output"), TEXT("Advanced"), TEXT("Exits"),
		};
		for (const TPair<FName, IDetailCategoryBuilder*>& Pair : Categories)
		{
			int32 Rank = UE_ARRAY_COUNT(Order);
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(Order); ++Index)
			{
				if (Pair.Key == Order[Index])
				{
					Rank = Index;
					break;
				}
			}
			Pair.Value->SetSortOrder(Rank);
		}
	});
}

// ── Workflow asset: DefaultModelName + Variables (Entry node selection) ────────

TSharedRef<IDetailCustomization> FTryllWorkflowAssetGraphCustomization::MakeInstance()
{
	return MakeShared<FTryllWorkflowAssetGraphCustomization>();
}

void FTryllWorkflowAssetGraphCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedRef<IPropertyHandle> GraphHandle =
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTryllWorkflowAsset, Graph));

	// Hide the whole Graph struct; re-surface only the default model. Variables
	// live at asset level (not inside Graph) — expose them next to DefaultModelName
	// so Entry-node selection authors declarations without leaving the graph editor.
	DetailBuilder.HideProperty(GraphHandle);

	TSharedPtr<IPropertyHandle> DefaultModelHandle =
		GraphHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllGraphDescription, DefaultModelName));
	TSharedRef<IPropertyHandle> VariablesHandle =
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTryllWorkflowAsset, Variables));

	IDetailCategoryBuilder& Category =
		DetailBuilder.EditCategory(TEXT("Workflow"), LOCTEXT("WorkflowCategory", "Workflow"));

	if (DefaultModelHandle.IsValid())
	{
		Category.AddProperty(DefaultModelHandle);
	}
	Category.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTryllWorkflowAsset, TextTableAsset)));
	Category.AddProperty(DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UTryllWorkflowAsset, TextTable)));
	Category.AddProperty(VariablesHandle);

	TArray<TWeakObjectPtr<UObject>> WorkflowObjects;
	DetailBuilder.GetObjectsBeingCustomized(WorkflowObjects);
	TWeakObjectPtr<UTryllWorkflowAsset> WorkflowAsset = WorkflowObjects.Num() > 0
		? Cast<UTryllWorkflowAsset>(WorkflowObjects[0].Get()) : nullptr;
	Category.AddCustomRow(LOCTEXT("CheckFilter", "Check texts"))
		.WholeRowContent()
		[
			SNew(SButton)
			.Text(LOCTEXT("CheckTexts", "Check texts"))
			.OnClicked_Lambda([WorkflowAsset]()
			{
				FTryllLocalizablePreview::OpenCheckTexts(WorkflowAsset.Get());
				return FReply::Handled();
			})
		];
}

#undef LOCTEXT_NAMESPACE
