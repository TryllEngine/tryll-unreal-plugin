// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "STryllTextEditWindow.h"

#include "TryllEditorUiContract.h"
#include "UI/TryllEditorStyle.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

// Named, not anonymous: UBT unity builds merge this file with others in the module.
namespace TryllTextEditWindowPrivate
{
/** The one editor window; opening another field retargets it. */
TWeakPtr<SWindow> OpenWindow;
TWeakPtr<STryllTextEditWindow> OpenEditor;
FDelegateHandle RowExtensionHandle;

FText Title(const FString& Field)
{
	return FText::FromString(FString(FTryllEditorUiContract::TextEditorTitle).Replace(TEXT("{field}"), *Field));
}

void AddRowExtension(const FOnGenerateGlobalRowExtensionArgs& Args, TArray<FPropertyRowExtensionButton>& OutExtensions)
{
	if (!Args.PropertyHandle.IsValid() || !STryllTextEditWindow::IsTryllMultilineString(Args.PropertyHandle->GetProperty()))
	{
		return;
	}
	// Weak: the row outlives neither the handle nor a details refresh.
	const TWeakPtr<IPropertyHandle> Weak = Args.PropertyHandle;
	FPropertyRowExtensionButton& Button = OutExtensions.AddDefaulted_GetRef();
	Button.Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit");
	Button.Label = FText::FromString(FTryllEditorUiContract::TextEditorOpen);
	Button.ToolTip = FText::FromString(FTryllEditorUiContract::TextEditorOpen);
	Button.UIAction = FUIAction(
		FExecuteAction::CreateLambda([Weak]()
		{
			FTryllTextFieldPath Path;
			if (const TSharedPtr<IPropertyHandle> Handle = Weak.Pin();
				Handle.IsValid() && FTryllTextFieldPath::FromHandle(Handle.ToSharedRef(), Path))
			{
				STryllTextEditWindow::Open(Path);
			}
		}),
		FCanExecuteAction::CreateLambda([Weak]()
		{
			return STryllTextEditWindow::CanOpen(Weak.Pin());
		}));
}
} // namespace TryllTextEditWindowPrivate

TFunction<void(const FTryllTextFieldPath&)> STryllTextEditWindow::DebugOpenForTests;

bool FTryllTextFieldPath::FromHandle(const TSharedRef<IPropertyHandle>& Handle, FTryllTextFieldPath& Out)
{
	TArray<UObject*> Outers;
	Handle->GetOuterObjects(Outers);
	if (Outers.Num() != 1 || Outers[0] == nullptr || !CastField<FStrProperty>(Handle->GetProperty()))
	{
		return false;
	}

	// Walk up to the nearest object: GetOuterObjects names that object, so the
	// steps start at one of its members.
	TArray<FTryllTextFieldStep> Steps;
	TSharedPtr<IPropertyHandle> Current = Handle;
	while (Current.IsValid() && Current->GetProperty() != nullptr && !Current->GetProperty()->IsA<FObjectPropertyBase>())
	{
		const TSharedPtr<IPropertyHandle> Parent = Current->GetParentHandle();
		const int32 Index = Current->GetIndexInArray();
		if (Index == INDEX_NONE)
		{
			Steps.Insert(FTryllTextFieldStep{ Current->GetProperty()->GetFName(), INDEX_NONE }, 0);
			Current = Parent;
			continue;
		}
		const FArrayProperty* Array = Parent.IsValid() ? CastField<FArrayProperty>(Parent->GetProperty()) : nullptr;
		if (Array == nullptr)
		{
			return false; // a fixed C array or a map/set element
		}
		Steps.Insert(FTryllTextFieldStep{ Array->GetFName(), Index }, 0);
		Current = Parent->GetParentHandle();
	}
	if (Steps.Num() == 0)
	{
		return false;
	}

	Out.Owner = Outers[0];
	Out.Steps = MoveTemp(Steps);
	Out.DisplayName = Handle->GetPropertyDisplayName().ToString();
	return Out.Resolve() != nullptr;
}

FString* FTryllTextFieldPath::Resolve() const
{
	UObject* Object = Owner.Get();
	if (Object == nullptr || Steps.Num() == 0)
	{
		return nullptr;
	}

	const UStruct* Struct = Object->GetClass();
	void* Container = Object;
	for (int32 StepIndex = 0; StepIndex < Steps.Num(); ++StepIndex)
	{
		const FTryllTextFieldStep& Step = Steps[StepIndex];
		FProperty* Property = FindFProperty<FProperty>(Struct, Step.Name);
		if (Property == nullptr)
		{
			return nullptr;
		}

		void* Value = Property->ContainerPtrToValuePtr<void>(Container);
		const FProperty* ValueProperty = Property;
		if (Step.ArrayIndex != INDEX_NONE)
		{
			const FArrayProperty* Array = CastField<FArrayProperty>(Property);
			if (Array == nullptr)
			{
				return nullptr;
			}
			FScriptArrayHelper Helper(Array, Value);
			if (!Helper.IsValidIndex(Step.ArrayIndex))
			{
				return nullptr;
			}
			Value = Helper.GetRawPtr(Step.ArrayIndex);
			ValueProperty = Array->Inner;
		}

		if (StepIndex == Steps.Num() - 1)
		{
			return CastField<FStrProperty>(ValueProperty) ? static_cast<FString*>(Value) : nullptr;
		}
		const FStructProperty* StructProperty = CastField<FStructProperty>(ValueProperty);
		if (StructProperty == nullptr)
		{
			return nullptr;
		}
		Struct = StructProperty->Struct;
		Container = Value;
	}
	return nullptr;
}

bool STryllTextEditWindow::IsTryllMultilineString(const FProperty* Property)
{
	if (Property == nullptr || !Property->IsA<FStrProperty>() || !Property->GetBoolMetaData(TEXT("MultiLine")))
	{
		return false;
	}
	const UStruct* Owner = Property->GetOwnerStruct();
	return Owner != nullptr && Owner->GetPackage()->GetFName() == FName(TEXT("/Script/TryllClient"));
}

bool STryllTextEditWindow::CanOpen(const TSharedPtr<IPropertyHandle>& Handle)
{
	if (!Handle.IsValid() || !Handle->IsValidHandle() || !Handle->IsEditable())
	{
		return false;
	}
	FString Value;
	return Handle->GetValue(Value) == FPropertyAccess::Success;
}

void STryllTextEditWindow::Open(const FTryllTextFieldPath& Path)
{
	using namespace TryllTextEditWindowPrivate;
	if (DebugOpenForTests)
	{
		DebugOpenForTests(Path);
		return;
	}
	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	const TSharedRef<STryllTextEditWindow> Editor = SNew(STryllTextEditWindow, Path);
	if (const TSharedPtr<SWindow> Existing = OpenWindow.Pin())
	{
		if (const TSharedPtr<STryllTextEditWindow> Previous = OpenEditor.Pin())
		{
			Previous->Commit();
		}
		OpenEditor = Editor;
		Existing->SetTitle(Title(Path.DisplayName));
		Existing->SetContent(Editor);
		Existing->BringToFront();
		FSlateApplication::Get().SetKeyboardFocus(Editor->TextBox, EFocusCause::SetDirectly);
		return;
	}

	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(Title(Path.DisplayName))
		.ClientSize(FVector2D(720.0f, 560.0f))
		.MinWidth(360.0f)
		.MinHeight(240.0f)
		.SupportsMaximize(true)
		.SupportsMinimize(false)
		[
			Editor
		];
	// Closing the window does not always blur the box first; keep what was typed.
	Window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([](const TSharedRef<SWindow>&)
	{
		if (const TSharedPtr<STryllTextEditWindow> Current = OpenEditor.Pin())
		{
			Current->Commit();
		}
	}));
	OpenWindow = Window;
	OpenEditor = Editor;

	const TSharedPtr<SWindow> Parent = FGlobalTabmanager::Get()->GetRootWindow();
	if (Parent.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(Window, Parent.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}
	FSlateApplication::Get().SetKeyboardFocus(Editor->TextBox, EFocusCause::SetDirectly);
}

void STryllTextEditWindow::RegisterRowExtension()
{
	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	TryllTextEditWindowPrivate::RowExtensionHandle =
		PropertyModule.GetGlobalRowExtensionDelegate().AddStatic(&TryllTextEditWindowPrivate::AddRowExtension);
}

void STryllTextEditWindow::UnregisterRowExtension()
{
	using namespace TryllTextEditWindowPrivate;
	if (RowExtensionHandle.IsValid() && FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor")
			.GetGlobalRowExtensionDelegate().Remove(RowExtensionHandle);
	}
	RowExtensionHandle.Reset();
	if (const TSharedPtr<SWindow> Window = OpenWindow.Pin())
	{
		Window->RequestDestroyWindow();
	}
}

void STryllTextEditWindow::Construct(const FArguments& /*InArgs*/, const FTryllTextFieldPath& InPath)
{
	Path = InPath;
	const UObject* Owner = Path.Owner.Get();
	const FString OwnerName = Owner ? FPackageName::GetShortName(Owner->GetPackage()->GetName()) : FString();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(12.0f, 10.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Path.DisplayName))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(OwnerName))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Dim))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Visibility(this, &STryllTextEditWindow::GoneVisibility)
				.Text(FText::FromString(FTryllEditorUiContract::TextEditorGone))
				.ColorAndOpacity(FSlateColor(FTryllEditorStyle::Txt))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SAssignNew(TextBox, SMultiLineEditableTextBox)
				.Visibility(this, &STryllTextEditWindow::TextVisibility)
				.Text(this, &STryllTextEditWindow::GetFieldText)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.AutoWrapText(true)
				.AlwaysShowScrollbars(false)
				.SelectAllTextWhenFocused(false)
				.ClearKeyboardFocusOnCommit(false)
				.SelectAllTextOnCommit(false)
				.OnTextChanged_Lambda([this](const FText& Text)
				{
					Pending = Text.ToString();
				})
				.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
				{
					Pending = Text.ToString();
					Commit();
				})
			]
		]
	];
}

void STryllTextEditWindow::Commit()
{
	if (!Pending.IsSet())
	{
		return;
	}
	const FString Next = Pending.GetValue();
	Pending.Reset();

	FString* Value = Path.Resolve();
	UObject* Owner = Path.Owner.Get();
	if (Value == nullptr || Owner == nullptr || *Value == Next)
	{
		return;
	}

	FProperty* Member = FindFProperty<FProperty>(Owner->GetClass(), Path.Steps[0].Name);
	const FScopedTransaction Transaction(TryllTextEditWindowPrivate::Title(Path.DisplayName));
	Owner->PreEditChange(Member); // Modify(): records the undo step and dirties the package
	*Value = Next;
	FPropertyChangedEvent Event(Member, EPropertyChangeType::ValueSet);
	Owner->PostEditChangeProperty(Event);
}

FText STryllTextEditWindow::GetFieldText() const
{
	const FString* Value = Path.Resolve();
	return Value ? FText::FromString(*Value) : FText::GetEmpty();
}

EVisibility STryllTextEditWindow::GoneVisibility() const
{
	return Path.Resolve() ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility STryllTextEditWindow::TextVisibility() const
{
	return Path.Resolve() ? EVisibility::Visible : EVisibility::Collapsed;
}

FString STryllTextEditWindow::DebugTextForTests() const
{
	return GetFieldText().ToString();
}

bool STryllTextEditWindow::DebugGoneForTests() const
{
	return GoneVisibility() == EVisibility::Visible;
}

void STryllTextEditWindow::DebugTypeForTests(const FString& Text)
{
	Pending = Text;
	Commit();
}
