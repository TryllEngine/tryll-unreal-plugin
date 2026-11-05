// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllSlotNameCustomization.h"

#include "TryllSlotName.h"
#include "TryllGraphDescription.h"
#include "TryllSlotGraphAnalysis.h"
#include "TryllRenamePropagation.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "TryllClientEditorModule.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SEditableTextBox.h"

#define LOCTEXT_NAMESPACE "TryllSlotNameCustomization"

TSharedRef<IPropertyTypeCustomization> FTryllSlotNameCustomization::MakeInstance()
{
	return MakeShared<FTryllSlotNameCustomization>();
}

FString FTryllSlotNameCustomization::CurrentOverride() const
{
	FString Value;
	if (ValueHandle.IsValid()) ValueHandle->GetValue(Value);
	return Value;
}

void FTryllSlotNameCustomization::CommitRename(const FString& RequestedOverride)
{
	if (!ValueHandle.IsValid()) return;

	const FString OldOverride = CurrentOverride();
	FString NewOverride = RequestedOverride.TrimStartAndEnd();
	if (NewOverride == OldOverride)
		return;

	UTryllNodeParamsBase* Params = OwningParams.Get();
	const FTryllGraphDescription* Graph = Params ? UTryllNodeParamsBase::FindEnclosingGraph(Params) : nullptr;
	const FString OwnerNodeName = Params ? Params->GetOwnerNodeName() : FString();

	if (!Graph || OwnerNodeName.IsEmpty())
	{
		ValueHandle->SetValue(NewOverride);
		return;
	}

	if (!NewOverride.IsEmpty())
	{
		if (!TryllRenamePropagation::IsValidGrammar(NewOverride) || TryllRenamePropagation::IsReserved(NewOverride))
		{
			UE_LOG(LogTryllEditor, Warning,
			       TEXT("[Tryll] Invalid slot name '%s': must match [A-Za-z_][A-Za-z0-9_]* and not be the reserved '%s'. Ignored."),
			       *NewOverride, *TryllRenamePropagation::ReservedUserMessage);
			return;
		}

		// Not uniquified: the server permits two nodes to legally share an effective output
		// slot (a non-blocking double-write warning only — see CompiledGraph::Compile), so
		// forcing uniqueness here would silently reject an author's intentional shared-slot
		// graph. Node-name uniqueness is unrelated and stays handled separately. Surface the
		// same warning here instead.
		for (const FTryllNodeDescription& Node : Graph->Nodes)
		{
			if (Node.Name.IsEmpty() || Node.Name == OwnerNodeName) continue;
			if (TryllSlotGraphAnalysis::GetEffectiveWriteSlot(Node) == NewOverride)
			{
				UE_LOG(LogTryllEditor, Warning,
				       TEXT("[Tryll] Slot '%s' is also written by node '%s' — legal, but a downstream reader may see whichever write happened last this turn."),
				       *NewOverride, *Node.Name);
				break;
			}
		}
	}

	const FString OldEffective = OldOverride.IsEmpty() ? OwnerNodeName : OldOverride;
	const FString NewEffective = NewOverride.IsEmpty() ? OwnerNodeName : NewOverride;

	FScopedTransaction Transaction(LOCTEXT("RenameSlot", "Rename Tryll Slot"));

	ValueHandle->SetValue(NewOverride);
	if (OldEffective != NewEffective)
	{
		// Graph was retrieved as const for lookups above; PropagateSlotRename needs the
		// mutable graph, which is the same underlying object — re-resolve non-const.
		if (FTryllGraphDescription* MutableGraph = const_cast<FTryllGraphDescription*>(Graph))
			TryllRenamePropagation::PropagateOwnedSlotRename(
				*MutableGraph, Params, OldEffective, NewEffective);
	}

	if (Params) Params->MarkPackageDirty();
}

void FTryllSlotNameCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	ValueHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllSlotName, Value));
	if (!ValueHandle.IsValid()) return;

	TArray<UObject*> OuterObjects;
	StructPropertyHandle->GetOuterObjects(OuterObjects);
	if (OuterObjects.Num() > 0)
		OwningParams = Cast<UTryllNodeParamsBase>(OuterObjects[0]);

	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(220.f)
	[
		SNew(SEditableTextBox)
		.Text_Lambda([this] { return FText::FromString(CurrentOverride()); })
		.HintText(LOCTEXT("SlotNameHint", "(use node name)"))
		.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type CommitType)
		{
			if (CommitType == ETextCommit::OnEnter || CommitType == ETextCommit::OnUserMovedFocus)
				CommitRename(NewText.ToString());
		})
	];
}

void FTryllSlotNameCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& /*ChildBuilder*/,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);
	for (uint32 Index = 0; Index < NumChildren; ++Index)
	{
		if (const TSharedPtr<IPropertyHandle> Child = StructPropertyHandle->GetChildHandle(Index))
			Child->MarkHiddenByCustomization();
	}
}

#undef LOCTEXT_NAMESPACE
