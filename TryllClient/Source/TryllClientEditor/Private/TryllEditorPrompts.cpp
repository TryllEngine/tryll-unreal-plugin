// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllEditorPrompts.h"

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"

namespace TryllEditorPromptsPrivate
{
	ITryllEditorPrompts* GOverride = nullptr;
}

EAppReturnType::Type FTryllEditorPrompts::Open(
	EAppMsgType::Type Type, const FText& Message, const FText& Title)
{
	if (TryllEditorPromptsPrivate::GOverride)
	{
		return TryllEditorPromptsPrivate::GOverride->Open(Type, Message, Title);
	}
	return Title.IsEmpty()
		? FMessageDialog::Open(Type, Message)
		: FMessageDialog::Open(Type, Message, Title);
}

bool FTryllEditorPrompts::SaveFileDialog(const void* ParentWindowHandle, const FString& Title,
	const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes,
	uint32 Flags, TArray<FString>& OutFilenames)
{
	if (TryllEditorPromptsPrivate::GOverride)
	{
		return TryllEditorPromptsPrivate::GOverride->SaveFileDialog(
			ParentWindowHandle, Title, DefaultPath, DefaultFile, FileTypes, Flags, OutFilenames);
	}
	IDesktopPlatform* Platform = FDesktopPlatformModule::Get();
	return Platform && Platform->SaveFileDialog(
		ParentWindowHandle, Title, DefaultPath, DefaultFile, FileTypes, Flags, OutFilenames);
}

bool FTryllEditorPrompts::OpenFileDialog(const void* ParentWindowHandle, const FString& Title,
	const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes,
	uint32 Flags, TArray<FString>& OutFilenames)
{
	if (TryllEditorPromptsPrivate::GOverride)
	{
		return TryllEditorPromptsPrivate::GOverride->OpenFileDialog(
			ParentWindowHandle, Title, DefaultPath, DefaultFile, FileTypes, Flags, OutFilenames);
	}
	IDesktopPlatform* Platform = FDesktopPlatformModule::Get();
	return Platform && Platform->OpenFileDialog(
		ParentWindowHandle, Title, DefaultPath, DefaultFile, FileTypes, Flags, OutFilenames);
}

void FTryllEditorPrompts::SetOverrideForTests(ITryllEditorPrompts* Override)
{
	TryllEditorPromptsPrivate::GOverride = Override;
}
