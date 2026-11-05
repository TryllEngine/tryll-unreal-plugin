// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllStoragePathCustomization.h"
#include "TryllEditorPrompts.h"

#include "TryllClientEditorModule.h"
#include "TryllRuntimeSettings.h"
#include "Misc/MessageDialog.h"

/** Translation folder name; mirrors kLocFolder in StoragePathResolver.h. */
static const TCHAR* TryllLocFolder = TEXT("loc");
#include "TryllStoragePath.h"

#include "DetailWidgetRow.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "IDesktopPlatform.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"

#define LOCTEXT_NAMESPACE "TryllStoragePathCustomization"

namespace
{
const FLinearColor InvalidBackgroundTint(1.f, 0.35f, 0.35f, 0.4f);

FString ReadPathValue(const TSharedPtr<IPropertyHandle>& PathHandle)
{
	FString Value;
	if (PathHandle.IsValid() && PathHandle->GetValue(Value) == FPropertyAccess::Success)
	{
		return UTryllRuntimeSettings::NormalizeStorageRelativePath(Value);
	}
	return FString();
}

void WritePathValue(const TSharedPtr<IPropertyHandle>& PathHandle, const FString& Value)
{
	if (PathHandle.IsValid())
	{
		PathHandle->SetValue(UTryllRuntimeSettings::NormalizeStorageRelativePath(Value));
	}
}
} // namespace

TSharedRef<IPropertyTypeCustomization> FTryllStoragePathCustomization::MakeInstance()
{
	return MakeShared<FTryllStoragePathCustomization>();
}

void FTryllStoragePathCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	const TSharedPtr<IPropertyHandle> PathHandle =
		StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTryllStoragePath, Path));
	if (!PathHandle.IsValid())
	{
		return;
	}

	const FString StorageKind = StructPropertyHandle->GetMetaData(TEXT("TryllStorageKind"));

	// Fix paths saved relative to a parent folder (e.g. StringStorages/RAG/x).
	{
		FString RawPath;
		if (PathHandle->GetValue(RawPath) == FPropertyAccess::Success)
		{
			const FString Normalized = UTryllRuntimeSettings::NormalizeStorageRelativePath(RawPath);
			if (!Normalized.Equals(RawPath, ESearchCase::CaseSensitive))
			{
				WritePathValue(PathHandle, Normalized);
			}
		}
	}

	const FEditableTextBoxStyle& EditboxStyle =
		FAppStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>(TEXT("NormalEditableTextBox"));

	HeaderRow.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(0.f, 0.f, 2.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("NoBorder"))
				.Padding(0.f)
				// BorderBackgroundColor tints only the border background — NOT child text.
				// Do not use ColorAndOpacity here: Transparent (alpha 0) hides the edit box.
				.BorderBackgroundColor_Lambda([PathHandle]()
				{
					const FString Value = ReadPathValue(PathHandle);
					return IsInvalidPath(Value) ? InvalidBackgroundTint : FLinearColor::Transparent;
				})
				[
					SNew(SEditableTextBox)
					.Style(&EditboxStyle)
					.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.Text_Lambda([PathHandle]()
					{
						return FText::FromString(ReadPathValue(PathHandle));
					})
					.OnTextCommitted_Lambda([PathHandle](const FText& NewText, ETextCommit::Type /*CommitType*/)
					{
						const FString Normalized =
							UTryllRuntimeSettings::NormalizeStorageRelativePath(NewText.ToString());
						WritePathValue(PathHandle, OfferDefaultPath(Normalized));
					})
					.ToolTipText_Lambda([PathHandle]()
					{
						return FText::FromString(ReadPathValue(PathHandle));
					})
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("BrowseButton", "..."))
				.ToolTipText_Lambda([]()
				{
					return UTryllRuntimeSettings::GetConfiguredStorageDataFolder().IsEmpty()
						? LOCTEXT("BrowseTooltipUnset", "Set StorageDataFolder first")
						: LOCTEXT("BrowseTooltip", "Browse for a storage file under the configured storage data folder.");
				})
				.IsEnabled_Lambda([]()
				{
					return !UTryllRuntimeSettings::GetConfiguredStorageDataFolder().IsEmpty();
				})
				.OnClicked_Lambda([PathHandle, StorageKind]()
				{
					FString RelativePath;
					if (BrowseForStorageFile(StorageKind, RelativePath))
					{
						WritePathValue(PathHandle, RelativePath);
					}
					return FReply::Handled();
				})
			]
		];
}

void FTryllStoragePathCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& /*ChildBuilder*/,
	IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
	uint32 NumChildren = 0;
	if (StructPropertyHandle->GetNumChildren(NumChildren) != FPropertyAccess::Success)
	{
		return;
	}

	for (uint32 Index = 0; Index < NumChildren; ++Index)
	{
		const TSharedPtr<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(Index);
		if (ChildHandle.IsValid())
		{
			ChildHandle->MarkHiddenByCustomization();
		}
	}
}

bool FTryllStoragePathCustomization::IsInvalidPath(const FString& Value)
{
	if (Value.IsEmpty())
	{
		return false;
	}

	if (!FPaths::IsRelative(Value) || Value.StartsWith(TEXT("/")) || Value.StartsWith(TEXT("\\")))
	{
		return true;
	}

	return Value.Contains(TEXT(".."));
}

FString FTryllStoragePathCustomization::BuildFileDialogFilter(const FString& StorageKind)
{
	if (StorageKind.Equals(TEXT("embedded"), ESearchCase::IgnoreCase))
	{
		return TEXT("Embedded storage config (*.json)|*.json|All files (*.*)|*.*");
	}
	if (StorageKind.Equals(TEXT("voice"), ESearchCase::IgnoreCase))
	{
		return TEXT("Voice clips (*.wav)|*.wav|All files (*.*)|*.*");
	}

	return TEXT("Plain storage files (*.txt;*.json;*.mm.json)|*.txt;*.json;*.mm.json|All files (*.*)|*.*");
}

bool FTryllStoragePathCustomization::BrowseForStorageFile(
	const FString& StorageKind,
	FString& OutRelativePath)
{
	const FString Root = UTryllRuntimeSettings::GetStorageRootAbsolute();
	const FString StartDir = FPaths::DirectoryExists(Root)
		? Root
		: FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

	TArray<FString> OutFiles;
	const bool bOpened = FTryllEditorPrompts::OpenFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Select storage file"),
		StartDir,
		TEXT(""),
		BuildFileDialogFilter(StorageKind),
		EFileDialogFlags::None,
		OutFiles);

	if (!bOpened || OutFiles.IsEmpty())
	{
		return false;
	}

	FString RelativePath;
	if (!UTryllRuntimeSettings::MakePathRelativeToStorageRoot(OutFiles[0], RelativePath))
	{
		UE_LOG(LogTryllEditor, Warning,
			TEXT("[Tryll] Selected file '%s' is outside the storage root '%s'. "
			     "Set TryllRuntimeSettings.StorageDataFolder to the folder that "
			     "contains your storage files. File not stored."),
			*OutFiles[0], *Root);
		return false;
	}

	OutRelativePath = OfferDefaultPath(
		UTryllRuntimeSettings::NormalizeStorageRelativePath(RelativePath));
	return true;
}

/**
 * Language when RelativePath sits inside a `loc/<language>/` folder, else empty.
 * OutDefaultPath receives the path with the `loc/<language>` pair removed - the
 * file this is a translation of.
 */
static FString LocalizedFolderLanguage(const FString& RelativePath, FString& OutDefaultPath)
{
	OutDefaultPath.Reset();

	TArray<FString> Parts;
	RelativePath.ParseIntoArray(Parts, TEXT("/"), /*InCullEmpty=*/false);

	// Need at least "loc/<lang>/<file>".
	for (int32 i = 0; i + 2 < Parts.Num(); ++i)
	{
		if (Parts[i] != TryllLocFolder) continue;

		const FString Language = Parts[i + 1];
		if (Language.IsEmpty()) continue;

		TArray<FString> Rebuilt;
		Rebuilt.Reserve(Parts.Num() - 2);
		for (int32 j = 0; j < Parts.Num(); ++j)
			if (j != i && j != i + 1) Rebuilt.Add(Parts[j]);

		OutDefaultPath = FString::Join(Rebuilt, TEXT("/"));
		return Language;
	}
	return FString();
}

FString FTryllStoragePathCustomization::OfferDefaultPath(const FString& RelativePath)
{
	FString DefaultPath;
	const FString Language = LocalizedFolderLanguage(RelativePath, DefaultPath);
	if (Language.IsEmpty() || DefaultPath.IsEmpty())
	{
		return RelativePath;
	}

	// Asks rather than rewriting silently: pinning one field to one language is
	// occasionally deliberate (a character who always speaks French), and a
	// dialog that defaults to the right answer costs one keypress while a silent
	// rewrite would be unexplainable.
	const EAppReturnType::Type Answer = FTryllEditorPrompts::Open(EAppMsgType::YesNo, FText::Format(
		LOCTEXT("LinkDefaultStorageFile",
			"'{0}' is a translation inside 'loc/{1}/'.\n\n"
			"Link the default file instead?\n\n{2}\n\n"
			"Tryll looks in 'loc/<language>/' beside the linked file automatically, so "
			"linking the default picks up this '{1}' translation when the session runs in "
			"that language - and falls back to the default elsewhere.\n\n"
			"Linking the translation directly would use '{1}' in every language."),
		FText::FromString(RelativePath),
		FText::FromString(Language),
		FText::FromString(DefaultPath)));

	return Answer == EAppReturnType::Yes ? DefaultPath : RelativePath;
}

#undef LOCTEXT_NAMESPACE
