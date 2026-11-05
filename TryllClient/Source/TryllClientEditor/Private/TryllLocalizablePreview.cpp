// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllLocalizablePreview.h"

#include "TryllTextProvider.h"
#include "TryllTextResolve.h"
#include "TryllWorkflowAsset.h"
#include "UI/TryllEditorStyle.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailCustomization.h"
#include "IDetailGroup.h"
#include "IDetailPropertyRow.h"
#include "IPropertyTypeCustomization.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "Styling/AppStyle.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

#define LOCTEXT_NAMESPACE "TryllLocalizablePreview"

namespace
{
	/** Bumped when a string table or the loaded translations change; previews recompute. */
	uint32 GPreviewEpoch = 0;
	FDelegateHandle GObjectModifiedHandle;
	FDelegateHandle GPropertyChangedHandle;
	FDelegateHandle GRevisionHandle;

	/** A preview is also recomputed at most this often, for .locres recompiled on disk. */
	constexpr double PreviewRefreshSeconds = 2.0;
	constexpr int32 PreviewLines = 5;

	void BumpPreviewEpoch()
	{
		++GPreviewEpoch;
		if (UTryllTextProvider* Provider = UTryllTextProvider::Get())
		{
			Provider->InvalidatePreviewCache();
		}
	}

	FName TableOf(const UObject* Object)
	{
		const UTryllWorkflowAsset* Asset = Cast<UTryllWorkflowAsset>(Object);
		if (Asset == nullptr && Object != nullptr)
		{
			Asset = Object->GetTypedOuter<UTryllWorkflowAsset>();
		}
		return Asset ? Asset->TextTable : NAME_None;
	}

	bool IsTextIdProperty(const FProperty* Prop)
	{
		return Prop && Prop->IsA<FStrProperty>() && Prop->GetName().EndsWith(TEXT("TextId"));
	}

	/** "SystemPromptTextId" → "SystemPrompt". */
	FString FieldOfTextId(const FProperty* TextIdProp)
	{
		return TextIdProp->GetName().LeftChop(6);
	}

	FName LocalizedFlagName(const FString& Field)
	{
		return FName(*(FString(TEXT("b")) + Field + TEXT("Localized")));
	}

	/** True when Owner has a b<Field>Localized flag and a <Field>TextId for this string. */
	bool IsLocalizableString(const UStruct* Owner, const FProperty* Prop)
	{
		if (Owner == nullptr || Prop == nullptr || !Prop->IsA<FStrProperty>())
		{
			return false;
		}
		const FString Name = Prop->GetName();
		return CastField<FBoolProperty>(Owner->FindPropertyByName(LocalizedFlagName(Name))) != nullptr
			&& Owner->FindPropertyByName(FName(*(Name + TEXT("TextId")))) != nullptr;
	}

	bool IsTicked(const TSharedPtr<IPropertyHandle>& Flag)
	{
		bool bValue = false;
		return Flag.IsValid() && Flag->GetValue(bValue) == FPropertyAccess::Success && bValue;
	}

	FString HighlightMustache(const FString& Text)
	{
		FString Out;
		int32 Start = 0;
		while (Start < Text.Len())
		{
			const int32 Open = Text.Find(TEXT("{{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
			if (Open == INDEX_NONE)
			{
				Out += Text.Mid(Start);
				break;
			}
			Out += Text.Mid(Start, Open - Start);
			const int32 Close = Text.Find(TEXT("}}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open + 2);
			const int32 End = Close == INDEX_NONE ? Text.Len() : Close + 2;
			Out += TEXT("«");
			Out += Text.Mid(Open, End - Open);
			Out += TEXT("»");
			Start = End;
		}
		return Out;
	}

	int32 LineCount(const FString& Text)
	{
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, false);
		return Lines.Num();
	}

	FString FirstLines(const FString& Text, int32 MaxLines)
	{
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, false);
		if (Lines.Num() <= MaxLines)
		{
			return Text;
		}
		FString Out;
		for (int32 Index = 0; Index < MaxLines; ++Index)
		{
			if (Index > 0)
			{
				Out += TEXT("\n");
			}
			Out += Lines[Index];
		}
		return Out + FString::Printf(TEXT("\n… (%d more lines)"), Lines.Num() - MaxLines);
	}

	void Notify(const FString& Message)
	{
		FNotificationInfo Info(FText::FromString(Message));
		Info.ExpireDuration = 5.f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	/** One preview row's state. The preview is cached, never recomputed per frame. */
	struct FPreviewState
	{
		TSharedPtr<IPropertyHandle> TextId;
		TSharedPtr<IPropertyHandle> Flag;
		TWeakObjectPtr<UObject> Owner;
		/** Chosen preview language. Empty = the provider's current language. Never the editor culture. */
		FString Locale;
		bool bShowAll = false;

		FString CachedId;
		FName CachedTable;
		FString CachedLocale;
		uint32 CachedEpoch = MAX_uint32;
		double CachedAt = -1000.0;
		FTryllTextPreview Cached;

		FName Table() const { return TableOf(Owner.Get()); }

		FString Id() const
		{
			FString Value;
			if (TextId.IsValid())
			{
				TextId->GetValue(Value);
			}
			return Value;
		}

		FString EffectiveLocale() const
		{
			return Locale.IsEmpty() ? UTryllTextProvider::CurrentProviderLocale() : Locale;
		}

		const FTryllTextPreview& Get()
		{
			const FString CurrentId = Id();
			const FName CurrentTable = Table();
			const FString CurrentLocale = EffectiveLocale();
			const double Now = FPlatformTime::Seconds();
			if (CurrentId != CachedId || CurrentTable != CachedTable || CurrentLocale != CachedLocale
				|| CachedEpoch != GPreviewEpoch || Now - CachedAt > PreviewRefreshSeconds)
			{
				Cached = FTryllTextPreview::Evaluate(UTryllTextProvider::Get(), CurrentTable, CurrentId, CurrentLocale);
				CachedId = CurrentId;
				CachedTable = CurrentTable;
				CachedLocale = CurrentLocale;
				CachedEpoch = GPreviewEpoch;
				CachedAt = Now;
			}
			return Cached;
		}
	};

	FSlateColor StatusColor(const FTryllTextPreview& Preview)
	{
		switch (Preview.Kind)
		{
			case FTryllTextPreview::EKind::Found:
				return Preview.MissingLocales.Num() > 0 ? FTryllEditorStyle::ColWarn : FTryllEditorStyle::ColOk;
			case FTryllTextPreview::EKind::MissingHere:
			case FTryllTextPreview::EKind::EmptyId:
				return FTryllEditorStyle::ColErr;
			default:
				return FSlateColor::UseSubduedForeground();
		}
	}

	FString StatusText(const FTryllTextPreview& Preview, const FString& Locale)
	{
		switch (Preview.Kind)
		{
			case FTryllTextPreview::EKind::Found:
				return Preview.MissingLocales.Num() > 0
					? FString::Printf(TEXT("Found (%s). Missing in: %s"), *Locale, *FString::Join(Preview.MissingLocales, TEXT(", ")))
					: FString::Printf(TEXT("Found (%s)"), *Locale);
			case FTryllTextPreview::EKind::MissingHere:
				return Preview.bIsFallback ? Preview.Message + TEXT(". Fallback shown.") : Preview.Message;
			default:
				return Preview.Message;
		}
	}

	TSharedRef<SWidget> MakeKeyPicker(TSharedRef<FPreviewState> State)
	{
		TSharedRef<TArray<TSharedPtr<FString>>> All = MakeShared<TArray<TSharedPtr<FString>>>();
		TSharedRef<TArray<TSharedPtr<FString>>> Shown = MakeShared<TArray<TSharedPtr<FString>>>();
		// Read the table every time the menu opens, so new keys show up.
		if (UTryllTextProvider* Provider = UTryllTextProvider::Get())
		{
			for (const FString& Key : Provider->ListIds(State->Table()))
			{
				All->Add(MakeShared<FString>(Key));
			}
		}
		*Shown = *All;

		if (All->Num() == 0)
		{
			return SNew(SBox).Padding(8.f)
			[
				SNew(STextBlock).Text(FText::FromString(State->Table().IsNone()
					? FString(TEXT("Pick the workflow's Text Table first."))
					: FString::Printf(TEXT("No keys in table '%s'. Type a key in Text Id."), *State->Table().ToString())))
			];
		}

		TSharedRef<SListView<TSharedPtr<FString>>> List = SNew(SListView<TSharedPtr<FString>>)
			.ListItemsSource(&Shown.Get())
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow_Lambda([](TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& Owner)
			{
				return SNew(STableRow<TSharedPtr<FString>>, Owner)
				[
					SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()))
				];
			})
			.OnMouseButtonClick_Lambda([State](TSharedPtr<FString> Item)
			{
				if (Item.IsValid() && State->TextId.IsValid())
				{
					State->TextId->SetValue(*Item);
				}
				FSlateApplication::Get().DismissAllMenus();
			});

		return SNew(SBox).WidthOverride(320.f).MaxDesiredHeight(360.f).Padding(4.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SSearchBox)
				.OnTextChanged_Lambda([All, Shown, List](const FText& Filter)
				{
					const FString Needle = Filter.ToString();
					Shown->Reset();
					for (const TSharedPtr<FString>& Key : *All)
					{
						if (Needle.IsEmpty() || Key->Contains(Needle))
						{
							Shown->Add(Key);
						}
					}
					List->RequestListRefresh();
				})
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				List
			]
		];
	}

	TSharedRef<SWidget> MakePreviewWidget(TSharedRef<FPreviewState> State)
	{
		TSharedRef<TArray<TSharedPtr<FString>>> Locales = MakeShared<TArray<TSharedPtr<FString>>>();
		auto ReloadLocales = [Locales]()
		{
			Locales->Reset();
			if (UTryllTextProvider* Provider = UTryllTextProvider::Get())
			{
				for (const FString& Locale : Provider->ListLocales())
				{
					Locales->Add(MakeShared<FString>(Locale));
				}
			}
		};
		ReloadLocales();

		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SComboButton)
					.ToolTipText(LOCTEXT("PickKeyTip", "Pick a key from the workflow's text table"))
					.ButtonContent()
					[
						SNew(STextBlock).Text(LOCTEXT("PickKey", "Pick key"))
					]
					.OnGetMenuContent_Lambda([State]() { return MakeKeyPicker(State); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Open", "Open"))
					.ToolTipText(LOCTEXT("OpenTip", "Open the text table in its editor"))
					.OnClicked_Lambda([State]()
					{
						UTryllTextProvider* Provider = UTryllTextProvider::Get();
						FString Message;
						if (Provider == nullptr)
						{
							Notify(TEXT("No text provider is set (Project Settings > Tryll Client > Text Provider)."));
						}
						else if (State->Table().IsNone())
						{
							Notify(TEXT("This workflow has no Text Table."));
						}
						else if (!Provider->OpenInEditor(State->Table(), State->Id(), Message))
						{
							Notify(Message);
						}
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f, 0.f, 0.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(LOCTEXT("PreviewIn", "Preview in"))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)
				[
					// Only the preview's language. The editor culture is never changed and no
					// language change is broadcast to live agents.
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&Locales.Get())
					.OnComboBoxOpening_Lambda(ReloadLocales)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
					{
						return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : FString()));
					})
					.OnSelectionChanged_Lambda([State](TSharedPtr<FString> Item, ESelectInfo::Type)
					{
						if (Item.IsValid())
						{
							State->Locale = *Item;
						}
					})
					.Content()
					[
						SNew(STextBlock).Text_Lambda([State]() { return FText::FromString(State->EffectiveLocale()); })
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.ColorAndOpacity_Lambda([State]() { return StatusColor(State->Get()); })
				.Text_Lambda([State]()
				{
					return FText::FromString(StatusText(State->Get(), State->EffectiveLocale()));
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Visibility_Lambda([State]()
				{
					return State->Get().Text.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
				})
				.Text_Lambda([State]()
				{
					const FTryllTextPreview& Preview = State->Get();
					const FString Body = State->bShowAll ? Preview.Text : FirstLines(Preview.Text, PreviewLines);
					return FText::FromString(HighlightMustache(Body));
				})
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.Visibility_Lambda([State]()
				{
					return LineCount(State->Get().Text) > PreviewLines ? EVisibility::Visible : EVisibility::Collapsed;
				})
				.Text_Lambda([State]()
				{
					return State->bShowAll ? LOCTEXT("ShowLess", "Show less") : LOCTEXT("ShowAll", "Show all");
				})
				.OnClicked_Lambda([State]()
				{
					State->bShowAll = !State->bShowAll;
					return FReply::Handled();
				})
			];
	}

	TAttribute<EVisibility> VisibleWhenTicked(TSharedPtr<IPropertyHandle> Flag)
	{
		return TAttribute<EVisibility>::CreateLambda([Flag]()
		{
			return IsTicked(Flag) ? EVisibility::Visible : EVisibility::Collapsed;
		});
	}

	TAttribute<EVisibility> VisibleWhenNotTicked(TSharedPtr<IPropertyHandle> Flag)
	{
		return TAttribute<EVisibility>::CreateLambda([Flag]()
		{
			return IsTicked(Flag) ? EVisibility::Collapsed : EVisibility::Visible;
		});
	}

	TSharedRef<FPreviewState> MakeState(
		TSharedRef<IPropertyHandle> TextId, TSharedPtr<IPropertyHandle> Flag, UObject* Owner)
	{
		TSharedRef<FPreviewState> State = MakeShared<FPreviewState>();
		State->TextId = TextId;
		State->Flag = Flag;
		State->Owner = Owner;
		return State;
	}

	void AddHandlePreview(
		IDetailChildrenBuilder& ChildBuilder, TSharedRef<IPropertyHandle> TextId,
		TSharedPtr<IPropertyHandle> Flag, UObject* Owner)
	{
		const FString Field = FieldOfTextId(TextId->GetProperty());
		ChildBuilder.AddCustomRow(LOCTEXT("PreviewFilter", "Text preview"))
			.Visibility(VisibleWhenTicked(Flag))
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::Format(LOCTEXT("PreviewOf", "{0} preview"), FText::FromString(FName::NameToDisplayString(Field, false))))
			]
			.ValueContent()
			.MinDesiredWidth(280.f)
			[
				MakePreviewWidget(MakeState(TextId, Flag, Owner))
			];
	}

	UObject* FirstOuter(const TSharedRef<IPropertyHandle>& Handle)
	{
		TArray<UObject*> Outers;
		Handle->GetOuterObjects(Outers);
		return Outers.Num() > 0 ? Outers[0] : nullptr;
	}
}

void FTryllLocalizablePreview::AddPreviews(IDetailLayoutBuilder& DetailBuilder, UObject* ParamsObject)
{
	if (ParamsObject == nullptr)
	{
		return;
	}
	UClass* Class = ParamsObject->GetClass();
	IDetailCategoryBuilder* Category = nullptr;
	for (TFieldIterator<FProperty> It(Class); It; ++It)
	{
		FProperty* Prop = *It;
		if (!IsLocalizableString(Class, Prop))
		{
			continue;
		}
		const FString Field = Prop->GetName();
		TSharedPtr<IPropertyHandle> Flag = DetailBuilder.GetProperty(LocalizedFlagName(Field), Class);

		// The inline text box is not what is sent while Localized is ticked. Hide it;
		// an optional field keeps its plain bOverride inline toggle when shown.
		if (IDetailPropertyRow* Inline = DetailBuilder.EditDefaultProperty(DetailBuilder.GetProperty(Prop->GetFName(), Class)))
		{
			Inline->Visibility(VisibleWhenNotTicked(Flag));
		}

		TSharedRef<IPropertyHandle> TextId = DetailBuilder.GetProperty(FName(*(Field + TEXT("TextId"))), Class);
		if (Category == nullptr)
		{
			Category = &DetailBuilder.EditCategory(TEXT("Localization"), LOCTEXT("LocalizationCategory", "Localization"));
		}
		Category->AddCustomRow(LOCTEXT("PreviewFilter", "Text preview"))
			.Visibility(VisibleWhenTicked(Flag))
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(FText::Format(LOCTEXT("PreviewOf", "{0} preview"), FText::FromString(FName::NameToDisplayString(Field, false))))
			]
			.ValueContent()
			.MinDesiredWidth(280.f)
			[
				MakePreviewWidget(MakeState(TextId, Flag, ParamsObject))
			];
	}
}

void FTryllLocalizablePreview::AddParamsObject(IDetailChildrenBuilder& ChildBuilder, TSharedRef<IPropertyHandle> ParamsHandle)
{
	UObject* Params = nullptr;
	ParamsHandle->GetValue(Params);

	// Resolve every editable field handle first; if the property tree does not expose
	// them by name, fall back to the default drawing rather than showing nothing.
	TArray<TPair<FProperty*, TSharedRef<IPropertyHandle>>> Fields;
	if (Params != nullptr)
	{
		for (TFieldIterator<FProperty> It(Params->GetClass()); It; ++It)
		{
			FProperty* Prop = *It;
			if (!Prop->HasAnyPropertyFlags(CPF_Edit) || Prop->HasMetaData(TEXT("InlineEditConditionToggle")))
			{
				continue;
			}
			TSharedPtr<IPropertyHandle> Child = ParamsHandle->GetChildHandle(Prop->GetFName());
			if (Child.IsValid())
			{
				Fields.Emplace(Prop, Child.ToSharedRef());
			}
		}
	}
	if (Params != nullptr && Fields.Num() == 0)
	{
		ChildBuilder.AddProperty(ParamsHandle);
		return;
	}

	IDetailGroup& Group = ChildBuilder.AddGroup(ParamsHandle->GetProperty()->GetFName(), ParamsHandle->GetPropertyDisplayName());
	Group.HeaderRow()
		.NameContent()
		[
			ParamsHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			ParamsHandle->CreatePropertyValueWidget()
		];
	Group.ToggleExpansion(true);

	const UClass* Class = Params ? Params->GetClass() : nullptr;
	for (const TPair<FProperty*, TSharedRef<IPropertyHandle>>& Field : Fields)
	{
		FProperty* Prop = Field.Key;
		IDetailPropertyRow& Row = Group.AddPropertyRow(Field.Value);
		if (IsLocalizableString(Class, Prop))
		{
			// Same rule as the graph editor: the inline text is not sent while Localized is ticked.
			Row.Visibility(VisibleWhenNotTicked(ParamsHandle->GetChildHandle(LocalizedFlagName(Prop->GetName()))));
		}
		if (IsTextIdProperty(Prop))
		{
			const FString Name = FieldOfTextId(Prop);
			TSharedPtr<IPropertyHandle> Flag = ParamsHandle->GetChildHandle(LocalizedFlagName(Name));
			Group.AddWidgetRow()
				.Visibility(VisibleWhenTicked(Flag))
				.NameContent()
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFont())
					.Text(FText::Format(LOCTEXT("PreviewOf", "{0} preview"), FText::FromString(FName::NameToDisplayString(Name, false))))
				]
				.ValueContent()
				.MinDesiredWidth(280.f)
				[
					MakePreviewWidget(MakeState(Field.Value, Flag, Params))
				];
		}
	}
}

void FTryllLocalizablePreview::AddPreviews(IDetailChildrenBuilder& ChildBuilder, TSharedRef<IPropertyHandle> ObjectOrStruct)
{
	UObject* Owner = FirstOuter(ObjectOrStruct);
	uint32 Count = 0;
	ObjectOrStruct->GetNumChildren(Count);
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		TSharedPtr<IPropertyHandle> Child = ObjectOrStruct->GetChildHandle(Index);
		if (!Child.IsValid())
		{
			continue;
		}
		IDetailPropertyRow& Row = ChildBuilder.AddProperty(Child.ToSharedRef());
		const FProperty* Prop = Child->GetProperty();
		const UStruct* OwnerStruct = Prop ? Prop->GetOwnerStruct() : nullptr;
		if (IsLocalizableString(OwnerStruct, Prop))
		{
			Row.Visibility(VisibleWhenNotTicked(ObjectOrStruct->GetChildHandle(LocalizedFlagName(Prop->GetName()))));
		}
		if (IsTextIdProperty(Prop))
		{
			const FString Field = FieldOfTextId(Prop);
			AddHandlePreview(ChildBuilder, Child.ToSharedRef(), ObjectOrStruct->GetChildHandle(LocalizedFlagName(Field)), Owner);
		}
	}
}

void FTryllLocalizablePreview::OpenCheckTexts(UTryllWorkflowAsset* Asset)
{
	if (Asset == nullptr)
	{
		return;
	}

	UTryllTextProvider* Provider = UTryllTextProvider::Get();
	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	if (Provider == nullptr || !Provider->SupportsEditorPreview())
	{
		const FString Name = Provider ? Provider->GetClass()->GetName() : TEXT("none");
		Body->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Text(FText::FromString(FString::Printf(TEXT("No preview — %s has no editor support."), *Name)))
		];
	}
	else
	{
		// Reads each culture's compiled translations once (cached by the provider);
		// the editor culture is not switched per cell.
		Provider->InvalidatePreviewCache();
		const TArray<FString> Locales = Provider->ListLocales();
		const TArray<FTryllTextResolve::FTextIdRef> Ids = FTryllTextResolve::CollectTextIds(Asset->Graph);
		if (Ids.Num() == 0)
		{
			Body->AddSlot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("None", "This workflow has no Localized fields."))
			];
		}
		for (const FTryllTextResolve::FTextIdRef& Entry : Ids)
		{
			Body->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Font(FAppStyle::GetFontStyle("BoldFont"))
				.Text(FText::FromString(Entry.NodeName + TEXT(" · ") + Entry.FieldPath + TEXT("  ") + Entry.Id))
			];
			if (Entry.Id.IsEmpty())
			{
				Body->AddSlot().AutoHeight()
				[
					SNew(STextBlock).ColorAndOpacity(FTryllEditorStyle::ColErr).Text(LOCTEXT("Empty", "Enter a Text ID"))
				];
				continue;
			}
			for (const FString& Locale : Locales)
			{
				FString Text;
				const bool bFound = Provider->GetTextForPreview(Asset->TextTable, Entry.Id, Locale, Text);
				Body->AddSlot().AutoHeight()
				[
					SNew(STextBlock)
					.ColorAndOpacity(bFound ? FSlateColor::UseForeground() : FSlateColor(FTryllEditorStyle::ColErr))
					.Text(FText::FromString(bFound
						? Locale + TEXT(": ") + FirstLines(Text, 1)
						: Locale + TEXT(": missing")))
				];
			}
		}
	}

	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("CheckTitle", "Check texts"))
		.ClientSize(FVector2D(640.f, 480.f))
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				Body
			]
		];
	FSlateApplication::Get().AddWindow(Window);
}

namespace
{
	class FTryllWorkflowTextDetails : public IDetailCustomization
	{
	public:
		static TSharedRef<IDetailCustomization> MakeInstance()
		{
			return MakeShared<FTryllWorkflowTextDetails>();
		}

		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			TArray<TWeakObjectPtr<UObject>> Objects;
			DetailBuilder.GetObjectsBeingCustomized(Objects);
			TWeakObjectPtr<UTryllWorkflowAsset> Asset = Objects.Num() > 0 ? Cast<UTryllWorkflowAsset>(Objects[0].Get()) : nullptr;
			IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Tryll|Workflow"));
			Category.AddCustomRow(LOCTEXT("CheckFilter", "Check texts"))
				.WholeRowContent()
				[
					SNew(SButton)
					.Text(LOCTEXT("CheckTexts", "Check texts"))
					.OnClicked_Lambda([Asset]()
					{
						FTryllLocalizablePreview::OpenCheckTexts(Asset.Get());
						return FReply::Handled();
					})
				];
		}
	};

	class FTryllClassifyIntentEntryPreview : public IPropertyTypeCustomization
	{
	public:
		static TSharedRef<IPropertyTypeCustomization> MakeInstance()
		{
			return MakeShared<FTryllClassifyIntentEntryPreview>();
		}

		virtual void CustomizeHeader(
			TSharedRef<IPropertyHandle> StructPropertyHandle,
			FDetailWidgetRow& HeaderRow,
			IPropertyTypeCustomizationUtils& /*CustomizationUtils*/) override
		{
			HeaderRow
				.NameContent()[StructPropertyHandle->CreatePropertyNameWidget()]
				.ValueContent()[StructPropertyHandle->CreatePropertyValueWidget()];
		}

		virtual void CustomizeChildren(
			TSharedRef<IPropertyHandle> StructPropertyHandle,
			IDetailChildrenBuilder& ChildBuilder,
			IPropertyTypeCustomizationUtils& /*CustomizationUtils*/) override
		{
			FTryllLocalizablePreview::AddPreviews(ChildBuilder, StructPropertyHandle);
		}
	};
}

void FTryllLocalizablePreview::RegisterCustomizations()
{
	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		UTryllWorkflowAsset::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTryllWorkflowTextDetails::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FName(TEXT("TryllClassifyIntentEntry")),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTryllClassifyIntentEntryPreview::MakeInstance));

	// Editing a string table, or newly loaded translations, refreshes open previews.
	GObjectModifiedHandle = FCoreUObjectDelegates::OnObjectModified.AddLambda([](UObject* Object)
	{
		if (Object && Object->IsA<UStringTable>())
		{
			BumpPreviewEpoch();
		}
	});
	GPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddLambda(
		[](UObject* Object, FPropertyChangedEvent& /*Event*/)
		{
			if (Object && Object->IsA<UStringTable>())
			{
				BumpPreviewEpoch();
			}
		});
	GRevisionHandle = FTextLocalizationManager::Get().OnTextRevisionChangedEvent.AddLambda([]()
	{
		BumpPreviewEpoch();
	});
}

void FTryllLocalizablePreview::UnregisterCustomizations()
{
	FCoreUObjectDelegates::OnObjectModified.Remove(GObjectModifiedHandle);
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(GPropertyChangedHandle);
	if (GRevisionHandle.IsValid() && !IsEngineExitRequested())
	{
		FTextLocalizationManager::Get().OnTextRevisionChangedEvent.Remove(GRevisionHandle);
	}
	GObjectModifiedHandle.Reset();
	GPropertyChangedHandle.Reset();
	GRevisionHandle.Reset();

	if (!FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		return;
	}
	FPropertyEditorModule& PropertyModule =
		FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout(UTryllWorkflowAsset::StaticClass()->GetFName());
	PropertyModule.UnregisterCustomPropertyTypeLayout(FName(TEXT("TryllClassifyIntentEntry")));
}

#undef LOCTEXT_NAMESPACE
