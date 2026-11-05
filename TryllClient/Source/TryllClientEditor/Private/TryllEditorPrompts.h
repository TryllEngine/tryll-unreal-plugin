// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "Misc/MessageDialog.h"

/**
 * Modal prompts and file pickers the editor plugin shows. Every
 * FMessageDialog::Open and IDesktopPlatform file dialog in TryllClientEditor
 * goes through here so window tests can record a prompt and script its answer
 * instead of blocking (or auto-answering) the -unattended editor.
 */
class ITryllEditorPrompts
{
public:
	virtual ~ITryllEditorPrompts() = default;

	virtual EAppReturnType::Type Open(EAppMsgType::Type Type, const FText& Message, const FText& Title) = 0;

	virtual bool SaveFileDialog(const void* ParentWindowHandle, const FString& Title,
		const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes,
		uint32 Flags, TArray<FString>& OutFilenames) = 0;

	virtual bool OpenFileDialog(const void* ParentWindowHandle, const FString& Title,
		const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes,
		uint32 Flags, TArray<FString>& OutFilenames) = 0;
};

class FTryllEditorPrompts
{
public:
	static EAppReturnType::Type Open(EAppMsgType::Type Type, const FText& Message,
		const FText& Title = FText::GetEmpty());

	static bool SaveFileDialog(const void* ParentWindowHandle, const FString& Title,
		const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes,
		uint32 Flags, TArray<FString>& OutFilenames);

	static bool OpenFileDialog(const void* ParentWindowHandle, const FString& Title,
		const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes,
		uint32 Flags, TArray<FString>& OutFilenames);

	/** Test-only replacement; nullptr means the real dialogs. Not owned. */
	static void SetOverrideForTests(ITryllEditorPrompts* Override);
};
