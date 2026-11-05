// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "AgentLog/AgentLogConfig.h"

#include "Misc/ConfigCacheIni.h"

FAgentLogConfig FAgentLogConfig::Load()
{
	FAgentLogConfig Config;

	GConfig->GetInt(
		IniSection,
		TEXT("MaxCompletedTurnsPerAgent"),
		Config.MaxCompletedTurnsPerAgent,
		GEditorPerProjectIni);
	GConfig->GetInt(
		IniSection,
		TEXT("MaxObservationsPerAgent"),
		Config.MaxObservationsPerAgent,
		GEditorPerProjectIni);
	GConfig->GetInt(
		IniSection,
		TEXT("MaxRawSessionObservations"),
		Config.MaxRawSessionObservations,
		GEditorPerProjectIni);
	GConfig->GetFloat(IniSection, TEXT("AgentListWidth"), Config.AgentListWidth, GEditorPerProjectIni);
	GConfig->GetFloat(IniSection, TEXT("TimelineWidth"), Config.TimelineWidth, GEditorPerProjectIni);
	GConfig->GetFloat(IniSection, TEXT("InspectorWidth"), Config.InspectorWidth, GEditorPerProjectIni);
	GConfig->GetBool(IniSection, TEXT("bInspectorVisible"), Config.bInspectorVisible, GEditorPerProjectIni);
	GConfig->GetBool(IniSection, TEXT("bCoalesceAnswerText"), Config.bCoalesceAnswerText, GEditorPerProjectIni);
	GConfig->GetBool(
		IniSection, TEXT("bCollapseCompletedTurns"), Config.bCollapseCompletedTurns, GEditorPerProjectIni);

	Config.MaxCompletedTurnsPerAgent = FMath::Max(Config.MaxCompletedTurnsPerAgent, 1);
	Config.MaxObservationsPerAgent   = FMath::Max(Config.MaxObservationsPerAgent, 1);
	Config.MaxRawSessionObservations = FMath::Max(Config.MaxRawSessionObservations, 1);

	return Config;
}

void FAgentLogConfig::Save() const
{
	GConfig->SetInt(
		IniSection,
		TEXT("MaxCompletedTurnsPerAgent"),
		MaxCompletedTurnsPerAgent,
		GEditorPerProjectIni);
	GConfig->SetInt(
		IniSection,
		TEXT("MaxObservationsPerAgent"),
		MaxObservationsPerAgent,
		GEditorPerProjectIni);
	GConfig->SetInt(
		IniSection,
		TEXT("MaxRawSessionObservations"),
		MaxRawSessionObservations,
		GEditorPerProjectIni);
	GConfig->SetFloat(IniSection, TEXT("AgentListWidth"), AgentListWidth, GEditorPerProjectIni);
	GConfig->SetFloat(IniSection, TEXT("TimelineWidth"), TimelineWidth, GEditorPerProjectIni);
	GConfig->SetFloat(IniSection, TEXT("InspectorWidth"), InspectorWidth, GEditorPerProjectIni);
	GConfig->SetBool(IniSection, TEXT("bInspectorVisible"), bInspectorVisible, GEditorPerProjectIni);
	GConfig->SetBool(IniSection, TEXT("bCoalesceAnswerText"), bCoalesceAnswerText, GEditorPerProjectIni);
	GConfig->SetBool(
		IniSection, TEXT("bCollapseCompletedTurns"), bCollapseCompletedTurns, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}
