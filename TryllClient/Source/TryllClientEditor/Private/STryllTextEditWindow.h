// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class IPropertyHandle;
class SMultiLineEditableTextBox;

/** One step from an object to a string field: a member, or one element of an array member. */
struct FTryllTextFieldStep
{
	FName Name;
	int32 ArrayIndex = INDEX_NONE;
};

/**
 * Where a string field lives. Resolved again on every read, by member name, so
 * a details refresh, a selection change, or a hot reload does not leave a
 * dangling pointer: the field is either found again or reported gone.
 */
struct FTryllTextFieldPath
{
	TWeakObjectPtr<UObject> Owner;
	TArray<FTryllTextFieldStep> Steps;
	FString DisplayName;

	/** False for a handle the window cannot follow: several objects, a map, a fixed array. */
	static bool FromHandle(const TSharedRef<IPropertyHandle>& Handle, FTryllTextFieldPath& Out);

	/** The field's storage now, or null when the owner or the element is gone. */
	FString* Resolve() const;
};

/**
 * A large, wrapped editor for one multiline Tryll string field (a system
 * prompt, a template), opened by the edit button at the right of the field's
 * details row. The text is written back when the box loses focus or the window
 * closes, as one undo step, like the details panel's own text box. One window:
 * opening another field retargets it. Follows docs/dev/editor-window-checklist.md:
 * theme roles only, copy from FTryllEditorUiContract, and a debug surface for
 * headless tests. Mirrors Unity's TryllTextEditWindow.
 */
class STryllTextEditWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STryllTextEditWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const FTryllTextFieldPath& InPath);

	/** True for a multiline string declared by a Tryll type. */
	static bool IsTryllMultilineString(const FProperty* Property);
	static bool CanOpen(const TSharedPtr<IPropertyHandle>& Handle);
	static void Open(const FTryllTextFieldPath& Path);

	/** Adds the edit button to every Tryll multiline string row in every details view. */
	static void RegisterRowExtension();
	static void UnregisterRowExtension();

	/** Test seam: called instead of opening the window. */
	static TFunction<void(const FTryllTextFieldPath&)> DebugOpenForTests;

	/** Write the typed text to the field, if it changed, as one undo step. */
	void Commit();

	FString DebugTextForTests() const;
	bool DebugGoneForTests() const;
	/** Type the whole text and leave the box, as a person would. */
	void DebugTypeForTests(const FString& Text);

private:
	FText GetFieldText() const;
	EVisibility GoneVisibility() const;
	EVisibility TextVisibility() const;

	FTryllTextFieldPath Path;
	TSharedPtr<SMultiLineEditableTextBox> TextBox;
	TOptional<FString> Pending;
};
