// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "TryllEditorUiContract.h"

struct FTryllMemoryColumn
{
	const TCHAR* Header = nullptr;
	float Weight = 1.f;
	bool bNumeric = false;
};

struct FTryllMemoryTableColumns
{
	static constexpr int32 KvGutterPx = 3;

	static constexpr FTryllMemoryColumn System[] = {
		{TEXT("Source"), FTryllEditorUiContract::MemorySystemWeights[0], false},
		{TEXT("Bytes"), FTryllEditorUiContract::MemorySystemWeights[1], true},
		{TEXT("Note"), FTryllEditorUiContract::MemorySystemWeights[2], false},
	};

	static constexpr FTryllMemoryColumn Devices[] = {
		{TEXT("Device"), FTryllEditorUiContract::MemoryDevicesWeights[0], false},
		{TEXT("Kind"), FTryllEditorUiContract::MemoryDevicesWeights[1], false},
		{TEXT("Ours"), FTryllEditorUiContract::MemoryDevicesWeights[2], true},
		{TEXT("Other"), FTryllEditorUiContract::MemoryDevicesWeights[3], true},
		{TEXT("Free"), FTryllEditorUiContract::MemoryDevicesWeights[4], true},
		{TEXT("Total"), FTryllEditorUiContract::MemoryDevicesWeights[5], true},
	};

	static constexpr FTryllMemoryColumn Kv[] = {
		{TEXT("Agent / node"), FTryllEditorUiContract::MemoryKvWeights[0], false},
		{TEXT("Class"), FTryllEditorUiContract::MemoryKvWeights[1], false},
		{TEXT("When"), FTryllEditorUiContract::MemoryKvWeights[2], false},
		{TEXT("Context used"), FTryllEditorUiContract::MemoryKvWeights[3], false},
		{TEXT("Reserved"), FTryllEditorUiContract::MemoryKvWeights[4], true},
	};

	static constexpr FTryllMemoryColumn Components[] = {
		{TEXT("Kind"), FTryllEditorUiContract::MemoryComponentsWeights[0], false},
		{TEXT("Name"), FTryllEditorUiContract::MemoryComponentsWeights[1], false},
		{TEXT("Confidence"), FTryllEditorUiContract::MemoryComponentsWeights[2], false},
		{TEXT("RAM"), FTryllEditorUiContract::MemoryComponentsWeights[3], true},
		{TEXT("VRAM"), FTryllEditorUiContract::MemoryComponentsWeights[4], true},
	};
};
