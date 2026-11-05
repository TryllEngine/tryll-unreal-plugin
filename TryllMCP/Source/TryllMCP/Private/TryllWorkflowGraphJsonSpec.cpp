// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.
//
// Covers the agent-facing graph JSON layer used by the MCP workflow toolset.
// The cases here are the ones a generic reflection client gets wrong: a supplied
// value that never takes effect because its bOverrideX toggle stayed off, and a
// node whose params object silently ends up null.

#if WITH_AUTOMATION_TESTS

#include "TryllWorkflowGraphJson.h"

#include "Generated/Nodes/TryllGenerateParams.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"

namespace
{
	TSharedPtr<FJsonObject> ParseJson(const FString& Json)
	{
		TSharedPtr<FJsonObject>   Object;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		FJsonSerializer::Deserialize(Reader, Object);
		return Object;
	}

	/** First node's "params" object from serialized graph JSON. */
	TSharedPtr<FJsonObject> FirstNodeParams(const FString& Json)
	{
		const TSharedPtr<FJsonObject> Root = ParseJson(Json);
		if (!Root.IsValid())
		{
			return nullptr;
		}

		const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
		if (!Root->TryGetArrayField(TEXT("nodes"), Nodes) || Nodes->IsEmpty())
		{
			return nullptr;
		}

		const TSharedPtr<FJsonObject>* Params = nullptr;
		if (!(*Nodes)[0]->AsObject()->TryGetObjectField(TEXT("params"), Params))
		{
			return nullptr;
		}
		return *Params;
	}

	const TCHAR* const kMerchantGraph = TEXT(R"({
		"startNode": "gen",
		"defaultModelName": "qwen2.5-0.5b-instruct",
		"variables": [
			{ "name": "gold", "type": "Int", "allowOutputSubstitution": true, "value": 120 },
			{ "name": "stock", "type": "StringSet", "value": ["fuel cell", "rope"] }
		],
		"nodes": [
			{ "name": "gen", "type": "Generate",
			  "params": { "systemPrompt": "You are a merchant.", "defaultExit": "" } }
		]
	})");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTryllWorkflowGraphJsonParseTest,
	"Tryll.Tests.Unit.MCP.WorkflowGraphJson.Parse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTryllWorkflowGraphJsonParseTest::RunTest(const FString& /*Parameters*/)
{
	const TryllWorkflowGraphJson::FParseResult Parsed =
		TryllWorkflowGraphJson::Parse(kMerchantGraph, GetTransientPackage());

	TestTrue(TEXT("graph parses without errors"), Parsed.IsValid());
	TestEqual(TEXT("start node"), Parsed.Graph.StartNode, FString(TEXT("gen")));
	TestEqual(TEXT("default model"),
		Parsed.Graph.DefaultModelName.Name, FString(TEXT("qwen2.5-0.5b-instruct")));
	TestEqual(TEXT("node count"), Parsed.Graph.Nodes.Num(), 1);

	if (Parsed.Graph.Nodes.IsEmpty())
	{
		return false;
	}

	// The generic reflection path returns success while leaving this null, which
	// is the failure this whole layer exists to make impossible.
	UTryllGenerateParams* Params = Cast<UTryllGenerateParams>(Parsed.Graph.Nodes[0].Params);
	if (Params == nullptr)
	{
		AddError(TEXT("node params were not instantiated as UTryllGenerateParams"));
		return false;
	}

	TestEqual(TEXT("system prompt applied"),
		Params->SystemPrompt, FString(TEXT("You are a merchant.")));
	TestTrue(TEXT("supplying a value turns its override toggle on"),
		Params->bOverrideSystemPrompt);
	TestFalse(TEXT("an omitted value leaves its override toggle off"),
		Params->bOverrideModelName);

	TestEqual(TEXT("variable count"), Parsed.Variables.Num(), 2);
	if (Parsed.Variables.Num() == 2)
	{
		TestEqual(TEXT("int variable value"), Parsed.Variables[0].IntValue, static_cast<int64>(120));
		TestTrue(TEXT("substitution flag"), Parsed.Variables[0].bAllowOutputSubstitution);
		TestEqual(TEXT("string set size"), Parsed.Variables[1].StringSetValue.Num(), 2);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTryllWorkflowGraphJsonRoundTripTest,
	"Tryll.Tests.Unit.MCP.WorkflowGraphJson.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTryllWorkflowGraphJsonRoundTripTest::RunTest(const FString& /*Parameters*/)
{
	const TryllWorkflowGraphJson::FParseResult First =
		TryllWorkflowGraphJson::Parse(kMerchantGraph, GetTransientPackage());
	TestTrue(TEXT("first parse succeeds"), First.IsValid());

	const FString Serialized = TryllWorkflowGraphJson::Serialize(First.Graph, First.Variables);

	const TryllWorkflowGraphJson::FParseResult Second =
		TryllWorkflowGraphJson::Parse(Serialized, GetTransientPackage());
	TestTrue(TEXT("re-parsing the serialized form succeeds"), Second.IsValid());
	TestEqual(TEXT("node count survives"), Second.Graph.Nodes.Num(), First.Graph.Nodes.Num());
	TestEqual(TEXT("variable count survives"), Second.Variables.Num(), First.Variables.Num());
	TestEqual(TEXT("start node survives"), Second.Graph.StartNode, First.Graph.StartNode);
	TestEqual(TEXT("default model survives"),
		Second.Graph.DefaultModelName.Name, First.Graph.DefaultModelName.Name);

	if (!Second.Graph.Nodes.IsEmpty())
	{
		const UTryllGenerateParams* Params =
			Cast<UTryllGenerateParams>(Second.Graph.Nodes[0].Params);
		if (Params == nullptr)
		{
			AddError(TEXT("round-tripped node lost its params object"));
			return false;
		}
		TestEqual(TEXT("system prompt survives"),
			Params->SystemPrompt, FString(TEXT("You are a merchant.")));
	}

	// Inherited fields must not reappear as explicit values, or the next write
	// would silently pin every default the model happened to read back.
	const TSharedPtr<FJsonObject> Params = FirstNodeParams(Serialized);
	if (Params.IsValid())
	{
		TestTrue(TEXT("overridden field is emitted"), Params->HasField(TEXT("systemPrompt")));
		TestFalse(TEXT("inherited field is omitted"), Params->HasField(TEXT("modelName")));
		TestFalse(TEXT("override toggles are not part of the surface"),
			Params->HasField(TEXT("bOverrideSystemPrompt")));

		// Unguarded fields have no inherit/override distinction, so the only signal
		// that the caller never chose one is that it still holds the default.
		TestFalse(TEXT("a field left at its default is omitted"),
			Params->HasField(TEXT("contextSize")));
		TestFalse(TEXT("an all-default nested struct is omitted"),
			Params->HasField(TEXT("outputFilter")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTryllWorkflowGraphJsonRejectsUnknownNodeTypeTest,
	"Tryll.Tests.Unit.MCP.WorkflowGraphJson.RejectsUnknownNodeType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTryllWorkflowGraphJsonRejectsUnknownNodeTypeTest::RunTest(const FString& /*Parameters*/)
{
	const TCHAR* const Json = TEXT(R"({
		"startNode": "a",
		"nodes": [ { "name": "a", "type": "Generat", "params": {} } ]
	})");

	const TryllWorkflowGraphJson::FParseResult Parsed =
		TryllWorkflowGraphJson::Parse(Json, GetTransientPackage());

	TestFalse(TEXT("a misspelt node type is rejected"), Parsed.IsValid());
	TestEqual(TEXT("no node is produced"), Parsed.Graph.Nodes.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTryllWorkflowGraphJsonRejectsUnknownFieldsTest,
	"Tryll.Tests.Unit.MCP.WorkflowGraphJson.RejectsUnknownFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTryllWorkflowGraphJsonRejectsUnknownFieldsTest::RunTest(const FString& /*Parameters*/)
{
	const auto ErrorsMention = [](const TryllWorkflowGraphJson::FParseResult& Parsed, const TCHAR* Text)
	{
		return Parsed.Errors.ContainsByPredicate(
			[Text](const FString& Error) { return Error.Contains(Text); });
	};

	const TryllWorkflowGraphJson::FParseResult Param = TryllWorkflowGraphJson::Parse(TEXT(R"({
		"startNode": "guard",
		"nodes": [ { "name": "guard", "type": "RegexGuardrail", "params": { "passExit": "x" } } ]
	})"), GetTransientPackage());
	TestFalse(TEXT("a misspelt param is rejected"), Param.IsValid());
	TestTrue(TEXT("the error names the misspelt param"), ErrorsMention(Param, TEXT("'passExit'")));
	TestTrue(TEXT("the error lists the real exit fields"), ErrorsMention(Param, TEXT("triggeredExit")));

	const TryllWorkflowGraphJson::FParseResult Nested = TryllWorkflowGraphJson::Parse(TEXT(R"({
		"startNode": "gen",
		"nodes": [ { "name": "gen", "type": "Generate", "params": { "sampling": { "temprature": 0.2 } } } ]
	})"), GetTransientPackage());
	TestFalse(TEXT("a misspelt nested field is rejected"), Nested.IsValid());
	TestTrue(TEXT("the error names the nested field"), ErrorsMention(Nested, TEXT("'temprature'")));

	const TryllWorkflowGraphJson::FParseResult Root = TryllWorkflowGraphJson::Parse(TEXT(R"({
		"start": "gen",
		"nodes": [ { "name": "gen", "type": "Generate", "parms": {} } ]
	})"), GetTransientPackage());
	TestTrue(TEXT("a misspelt graph field is rejected"), ErrorsMention(Root, TEXT("'start'")));
	TestTrue(TEXT("a misspelt node field is rejected"), ErrorsMention(Root, TEXT("'parms'")));

	const TryllWorkflowGraphJson::FParseResult Toggle = TryllWorkflowGraphJson::Parse(TEXT(R"({
		"startNode": "gen",
		"nodes": [ { "name": "gen", "type": "Generate", "params": { "bOverrideSystemPrompt": true } } ]
	})"), GetTransientPackage());
	TestFalse(TEXT("override toggles are not accepted as fields"), Toggle.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTryllWorkflowGraphJsonWrapperStringsTest,
	"Tryll.Tests.Unit.MCP.WorkflowGraphJson.WrapperStructsAreStrings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTryllWorkflowGraphJsonWrapperStringsTest::RunTest(const FString& /*Parameters*/)
{
	const TCHAR* const Json = TEXT(R"({
		"startNode": "gen",
		"nodes": [
			{ "name": "gen", "type": "Generate",
			  "params": { "modelName": "gemma-3-1b-it", "outputName": "reply" } }
		]
	})");

	const TryllWorkflowGraphJson::FParseResult Parsed =
		TryllWorkflowGraphJson::Parse(Json, GetTransientPackage());
	TestTrue(TEXT("plain strings are accepted for wrapper structs"), Parsed.IsValid());

	if (Parsed.Graph.Nodes.IsEmpty())
	{
		return false;
	}

	const UTryllGenerateParams* Params = Cast<UTryllGenerateParams>(Parsed.Graph.Nodes[0].Params);
	if (Params == nullptr)
	{
		AddError(TEXT("node params were not instantiated"));
		return false;
	}

	TestEqual(TEXT("model name unwrapped into FTryllModelName"),
		Params->ModelName.Name, FString(TEXT("gemma-3-1b-it")));
	TestTrue(TEXT("wrapper field also drives its override toggle"), Params->bOverrideModelName);

	const TSharedPtr<FJsonObject> Emitted =
		FirstNodeParams(TryllWorkflowGraphJson::Serialize(Parsed.Graph, Parsed.Variables));
	if (Emitted.IsValid())
	{
		FString ModelName;
		TestTrue(TEXT("model name comes back out as a plain string"),
			Emitted->TryGetStringField(TEXT("modelName"), ModelName));
		TestEqual(TEXT("round-tripped model name"), ModelName, FString(TEXT("gemma-3-1b-it")));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
