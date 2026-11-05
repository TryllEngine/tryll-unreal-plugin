// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllNodeDescriptionCustomization.h"
#include "TryllLocalizablePreview.h"

#include "TryllGraphDescription.h"
#include "TryllWorkflowAsset.h"
#include "TryllRenamePropagation.h"
#include "TryllClientEditorModule.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TryllNodeDescriptionCustomization"

namespace
{
	FTryllGraphDescription* FindMutableGraph(UObject* Obj)
	{
		if (UTryllWorkflowAsset* Asset = Cast<UTryllWorkflowAsset>(Obj))
			return &Asset->Graph;
		return nullptr;
	}
}

TSharedRef<IPropertyTypeCustomization> FTryllNodeDescriptionCustomization::MakeInstance()
{
	return MakeShared<FTryllNodeDescriptionCustomization>();
}

FString FTryllNodeDescriptionCustomization::CurrentName() const
{
	FString Value;
	if (NameHandle.IsValid()) NameHandle->GetValue(Value);
	return Value;
}

void FTryllNodeDescriptionCustomization::CommitRename(const FString& RequestedName)
{
	if (!NameHandle.IsValid()) return;

	const FString OldName = CurrentName();
	const FString Requested = RequestedName.TrimStartAndEnd();
	if (Requested == OldName)
		return;

	TArray<UObject*> OuterObjects;
	NameHandle->GetOuterObjects(OuterObjects);
	FTryllGraphDescription* Graph = OuterObjects.Num() > 0 ? FindMutableGraph(OuterObjects[0]) : nullptr;

	if (!Graph)
	{
		// No graph to check against or propagate into: still refuse a name the server would
		// reject, rather than writing it through unvalidated.
		const TryllRenamePropagation::FNodeRenameOutcome Bare =
			TryllRenamePropagation::ResolveNodeRename(Requested, TSet<FString>());
		if (!Bare.bValid)
		{
			UE_LOG(LogTryllEditor, Warning, TEXT("[Tryll] Node rename to '%s' rejected: %s"),
			       *Requested, *Bare.Error.ToString());
			return;
		}
		NameHandle->SetValue(Bare.Name);
		return;
	}

	FTryllNodeDescription* RenamedNode = nullptr;
	TSet<FString> ExistingNames;
	for (FTryllNodeDescription& Node : Graph->Nodes)
	{
		if (Node.Name.IsEmpty()) continue;
		if (Node.Name == OldName && !RenamedNode)
			RenamedNode = &Node;
		else
			ExistingNames.Add(Node.Name);
	}

	const TryllRenamePropagation::FNodeRenameOutcome Outcome =
		TryllRenamePropagation::ResolveNodeRename(Requested, ExistingNames);
	if (!Outcome.bValid)
	{
		UE_LOG(LogTryllEditor, Warning, TEXT("[Tryll] Node rename to '%s' rejected: %s"),
		       *Requested, *Outcome.Error.ToString());
		return;
	}

	const FString NewName = Outcome.Name;
	const bool bRewriteSlot = RenamedNode && TryllRenamePropagation::EffectiveSlotIsNodeName(*RenamedNode);

	FScopedTransaction Transaction(LOCTEXT("RenameNode", "Rename Tryll Node"));

	NameHandle->SetValue(NewName);
	TryllRenamePropagation::PropagateNodeRename(*Graph, OldName, NewName);
	if (bRewriteSlot)
		TryllRenamePropagation::PropagateOwnedSlotRename(
			*Graph, RenamedNode ? RenamedNode->Params : nullptr, OldName, NewName);

	for (UObject* Obj : OuterObjects)
		if (Obj) Obj->MarkPackageDirty();
}

void FTryllNodeDescriptionCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	NameHandle   = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllNodeDescription, Name));
	ParamsHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllNodeDescription, Params));
	if (!NameHandle.IsValid()) return;

	HeaderRow.NameContent()
	[
		SNew(STextBlock).Text(LOCTEXT("NodeNameLabel", "Node Name"))
	]
	.ValueContent()
	.MinDesiredWidth(220.f)
	[
		SNew(SEditableTextBox)
		.Text_Lambda([this] { return FText::FromString(CurrentName()); })
		.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type CommitType)
		{
			if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
				CommitRename(NewText.ToString());
		})
	];
}

void FTryllNodeDescriptionCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (ParamsHandle.IsValid())
	{
		// The params fields are drawn here (not by the default object row), so a new
		// params class picked in the header needs a rebuild.
		TWeakPtr<IPropertyUtilities> Utilities = CustomizationUtils.GetPropertyUtilities();
		ParamsHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([Utilities]()
		{
			if (TSharedPtr<IPropertyUtilities> Pinned = Utilities.Pin())
			{
				Pinned->ForceRefresh();
			}
		}));
		FTryllLocalizablePreview::AddParamsObject(ChildBuilder, ParamsHandle.ToSharedRef());
	}
}

#undef LOCTEXT_NAMESPACE
