// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

// AUTO-GENERATED — do not edit by hand.
// Source of truth: tryll/schema/*.fbs  +  tools/codegen/templates/unreal_schema_enums.h.j2
// See: docs/dev/codegen.md
#pragma once

#include "CoreMinimal.h"

/** Schema enums saved in node params, with Unreal value names, for checking the handwritten UENUMs against. */
struct FTryllSchemaEnum
{
    const TCHAR* Name;
    TArray<TPair<FString, int64>> Values;
};

inline TArray<FTryllSchemaEnum> TryllSchemaEnums()
{
    return {
        {TEXT("ETryllAttributeFailureRoute"), {
            {TEXT("Default"), 0},
            {TEXT("Triggered"), 1},
        }},
        {TEXT("ETryllAttributeKind"), {
            {TEXT("Choice"), 0},
            {TEXT("Boolean"), 1},
            {TEXT("Score"), 2},
        }},
        {TEXT("ETryllBooleanTrigger"), {
            {TEXT("Never"), 0},
            {TEXT("True_"), 1},
            {TEXT("False_"), 2},
        }},
        {TEXT("ETryllBranchTest"), {
            {TEXT("NonEmpty"), 0},
            {TEXT("IsTrue"), 1},
            {TEXT("Equals"), 2},
            {TEXT("EqualsIgnoreCase"), 3},
            {TEXT("Contains"), 4},
            {TEXT("Regex"), 5},
            {TEXT("Expression"), 6},
        }},
        {TEXT("ETryllCannedResponseSelectionStrategy"), {
            {TEXT("Random"), 0},
            {TEXT("First"), 1},
            {TEXT("RoundRobin"), 2},
        }},
        {TEXT("ETryllHistoryRole"), {
            {TEXT("None"), 0},
            {TEXT("Assistant"), 1},
            {TEXT("User"), 2},
            {TEXT("System"), 3},
        }},
        {TEXT("ETryllKnowledgeSourceKind"), {
            {TEXT("FromFile"), 0},
            {TEXT("FromName"), 1},
        }},
        {TEXT("ETryllKvCacheType"), {
            {TEXT("Inherit"), 0},
            {TEXT("F16"), 1},
            {TEXT("Q8_0"), 2},
            {TEXT("Q4_0"), 3},
        }},
        {TEXT("ETryllNotifyClient"), {
            {TEXT("Disabled"), 0},
            {TEXT("OnFound"), 1},
            {TEXT("Always"), 2},
        }},
        {TEXT("ETryllPlacement"), {
            {TEXT("BeforeUserAsUser"), 0},
            {TEXT("BeforeUserAsSystem"), 1},
            {TEXT("AfterUserAsUser"), 2},
            {TEXT("AfterUserAsSystem"), 3},
        }},
        {TEXT("ETryllRetrievalMode"), {
            {TEXT("Dense"), 0},
            {TEXT("Lexical"), 1},
            {TEXT("Hybrid"), 2},
        }},
        {TEXT("ETryllScoreTrigger"), {
            {TEXT("Never"), 0},
            {TEXT("AtOrAbove"), 1},
            {TEXT("Below"), 2},
        }},
        {TEXT("ETryllSendAnswer"), {
            {TEXT("None"), 0},
            {TEXT("Whole"), 1},
            {TEXT("Streamed"), 2},
        }},
        {TEXT("ETryllToolCallDisposition"), {
            {TEXT("RouteOnly"), 0},
            {TEXT("Acknowledge"), 1},
            {TEXT("Notify"), 2},
            {TEXT("NotifyAndAcknowledge"), 3},
            {TEXT("Pause"), 4},
            {TEXT("PauseAndAcknowledge"), 5},
            {TEXT("AwaitResult"), 6},
        }},
        {TEXT("ETryllToolCallMode"), {
            {TEXT("DetectOnly"), 0},
            {TEXT("CallOrAnswer"), 1},
            {TEXT("RequireCall"), 2},
        }},
    };
}
