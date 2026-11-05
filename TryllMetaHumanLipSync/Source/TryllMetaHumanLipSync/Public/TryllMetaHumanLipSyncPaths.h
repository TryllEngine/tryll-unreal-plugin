// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#pragma once

#include "CoreMinimal.h"

TRYLLMETAHUMANLIPSYNC_API DECLARE_LOG_CATEGORY_EXTERN(LogTryllMetaHumanLipSync, Log, All);

namespace TryllMetaHumanLipSync
{
inline const TCHAR* const ModelPackagePath = TEXT("/Game/TryllMetaHumanLipSync/solar-1-2700_blend-0v65");
inline const TCHAR* const ModelObjectPath = TEXT("/Game/TryllMetaHumanLipSync/solar-1-2700_blend-0v65.solar-1-2700_blend-0v65");
inline const TCHAR* const EngineModelObjectPath = TEXT("/MetaHumanCoreTech/RealtimeAudio/solar-1-2700_blend-0v65.solar-1-2700_blend-0v65");
inline const TCHAR* const CpuRuntimeName = TEXT("NNERuntimeORTCpu");
inline const TCHAR* const FaceAnimBlueprintPackagePath = TEXT("/TryllMetaHumanLipSync/ABP_TryllMetaHumanLipSyncFace");
inline const TCHAR* const FaceAnimBlueprintClassPath = TEXT("/TryllMetaHumanLipSync/ABP_TryllMetaHumanLipSyncFace.ABP_TryllMetaHumanLipSyncFace_C");
}
