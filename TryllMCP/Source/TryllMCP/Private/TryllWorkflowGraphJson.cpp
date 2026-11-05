// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllWorkflowGraphJson.h"

#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UObjectIterator.h"

namespace
{
	const TCHAR* const OverridePrefix = TEXT("bOverride");

	/** JSON key for a UPROPERTY, matching FJsonObjectConverter's own casing rule. */
	FString KeyFor(const FProperty* Property)
	{
		return FJsonObjectConverter::StandardizeCase(Property->GetName());
	}

	/**
	 * The single FString field of a wrapper struct such as FTryllModelName, or null
	 * when the struct is anything else. Detected structurally rather than by name so
	 * that new wrappers emitted by codegen collapse without changes here.
	 */
	const FStrProperty* FindSoleStringField(const UScriptStruct* Struct)
	{
		const FStrProperty* Sole = nullptr;
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			if (Sole != nullptr)
			{
				return nullptr;
			}
			Sole = CastField<FStrProperty>(*It);
			if (Sole == nullptr)
			{
				return nullptr;
			}
		}
		return Sole;
	}

	const UScriptStruct* AsWrapperStruct(const FProperty* Property)
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		if (StructProperty == nullptr || FindSoleStringField(StructProperty->Struct) == nullptr)
		{
			return nullptr;
		}
		return StructProperty->Struct;
	}

	/**
	 * Rewrites the agent-facing JSON into the shape FJsonObjectConverter expects:
	 * plain strings standing in for wrapper structs become single-field objects.
	 * Recurses through nested structs and arrays so Sampling.* and similar are covered.
	 */
	void ExpandWrappers(const UStruct* Definition, const TSharedPtr<FJsonObject>& Object)
	{
		if (!Object.IsValid())
		{
			return;
		}

		for (TFieldIterator<FProperty> It(Definition); It; ++It)
		{
			const FProperty* Property = *It;
			const FString     Key      = KeyFor(Property);

			const TSharedPtr<FJsonValue> Value = Object->TryGetField(Key);
			if (!Value.IsValid() || Value->Type == EJson::Null)
			{
				continue;
			}

			if (const UScriptStruct* Wrapper = AsWrapperStruct(Property))
			{
				if (Value->Type == EJson::String)
				{
					const FStrProperty*        Sole = FindSoleStringField(Wrapper);
					TSharedRef<FJsonObject>    Wrapped = MakeShared<FJsonObject>();
					Wrapped->SetStringField(KeyFor(Sole), Value->AsString());
					Object->SetObjectField(Key, Wrapped);
				}
				continue;
			}

			if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
			{
				if (Value->Type == EJson::Object)
				{
					ExpandWrappers(StructProperty->Struct, Value->AsObject());
				}
			}
		}
	}

	/**
	 * Turns field presence into the codegen's bOverrideX toggles. Runs after
	 * conversion so it can walk the JSON and the populated struct together.
	 */
	void ApplyOverrideFlags(
		const UStruct* Definition, void* Container, const TSharedPtr<FJsonObject>& Object)
	{
		if (!Object.IsValid())
		{
			return;
		}

		for (TFieldIterator<FProperty> It(Definition); It; ++It)
		{
			const FProperty* Property = *It;
			const FString     Name     = Property->GetName();

			if (const FBoolProperty* Toggle = CastField<FBoolProperty>(Property))
			{
				if (Name.StartsWith(OverridePrefix, ESearchCase::CaseSensitive))
				{
					const FString Guarded =
						FJsonObjectConverter::StandardizeCase(Name.RightChop(FCString::Strlen(OverridePrefix)));
					if (Object->HasField(Guarded))
					{
						Toggle->SetPropertyValue_InContainer(Container, true);
					}
				}
				continue;
			}

			const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
			if (StructProperty == nullptr || AsWrapperStruct(Property) != nullptr)
			{
				continue;
			}

			const TSharedPtr<FJsonValue> Value = Object->TryGetField(KeyFor(Property));
			if (Value.IsValid() && Value->Type == EJson::Object)
			{
				ApplyOverrideFlags(
					StructProperty->Struct,
					StructProperty->ContainerPtrToValuePtr<void>(Container),
					Value->AsObject());
			}
		}
	}

	/** Keys of override-guarded fields, mapped to whether their toggle is on. */
	TMap<FString, bool> CollectOverrideStates(const UStruct* Definition, const void* Container)
	{
		TMap<FString, bool> States;
		for (TFieldIterator<FProperty> It(Definition); It; ++It)
		{
			const FBoolProperty* Toggle = CastField<FBoolProperty>(*It);
			if (Toggle == nullptr
				|| !Toggle->GetName().StartsWith(OverridePrefix, ESearchCase::CaseSensitive))
			{
				continue;
			}
			States.Add(
				FJsonObjectConverter::StandardizeCase(
					Toggle->GetName().RightChop(FCString::Strlen(OverridePrefix))),
				Toggle->GetPropertyValue_InContainer(Container));
		}
		return States;
	}

	/**
	 * Inverse of the two passes above: collapse wrapper structs back to strings, drop
	 * the toggles, and drop anything the caller did not actually choose — inherited
	 * fields and fields still at their default.
	 *
	 * Emitting defaults would be actively harmful, not just noisy: read output is
	 * meant to be edited and written back, and every default echoed back becomes an
	 * explicit value on the next write. It also costs the caller context for fields
	 * it never asked about.
	 *
	 * @param Defaults Matching container from the class default object, or null to
	 *   keep every field regardless of value.
	 */
	void CollapseForOutput(
		const UStruct*                 Definition,
		const void*                    Container,
		const void*                    Defaults,
		const TSharedPtr<FJsonObject>& Object)
	{
		if (!Object.IsValid())
		{
			return;
		}

		const TMap<FString, bool> OverrideStates = CollectOverrideStates(Definition, Container);

		TArray<FString> Remove;

		for (TFieldIterator<FProperty> It(Definition); It; ++It)
		{
			const FProperty* Property = *It;
			const FString     Key      = KeyFor(Property);

			if (Property->GetName().StartsWith(OverridePrefix, ESearchCase::CaseSensitive)
				&& CastField<FBoolProperty>(Property) != nullptr)
			{
				Remove.Add(Key);
				continue;
			}

			const bool* const OverrideState = OverrideStates.Find(Key);
			if (OverrideState != nullptr && !*OverrideState)
			{
				Remove.Add(Key);
				continue;
			}

			const TSharedPtr<FJsonValue> Value = Object->TryGetField(Key);
			if (!Value.IsValid())
			{
				continue;
			}

			// An explicitly overridden field stays even when it matches the default:
			// the override is a decision not to inherit, which the value alone cannot
			// express. Everything else at its default is dropped.
			const bool bIsOverridden = OverrideState != nullptr;
			if (!bIsOverridden && Defaults != nullptr
				&& Property->Identical_InContainer(Container, Defaults))
			{
				Remove.Add(Key);
				continue;
			}

			if (const UScriptStruct* Wrapper = AsWrapperStruct(Property))
			{
				if (Value->Type == EJson::Object)
				{
					const FStrProperty* Sole = FindSoleStringField(Wrapper);
					FString              Inner;
					Value->AsObject()->TryGetStringField(KeyFor(Sole), Inner);
					Object->SetStringField(Key, Inner);
				}
				continue;
			}

			if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
			{
				if (Value->Type == EJson::Object)
				{
					CollapseForOutput(
						StructProperty->Struct,
						StructProperty->ContainerPtrToValuePtr<void>(Container),
						Defaults != nullptr
							? StructProperty->ContainerPtrToValuePtr<void>(Defaults)
							: nullptr,
						Value->AsObject());
				}
			}
		}

		for (const FString& Key : Remove)
		{
			Object->RemoveField(Key);
		}
	}

	/**
	 * FJsonObjectConverter ignores keys it cannot match, so a misspelt field would be
	 * dropped while the write still reports success.
	 */
	void CollectUnknownKeys(
		const UStruct*                 Definition,
		const TSharedPtr<FJsonObject>& Object,
		const FString&                 Path,
		TArray<FString>&               OutErrors)
	{
		if (!Object.IsValid())
		{
			return;
		}

		TMap<FString, const FProperty*> Known;
		for (TFieldIterator<FProperty> It(Definition); It; ++It)
		{
			const FProperty* Property = *It;
			if (CastField<FBoolProperty>(Property) != nullptr
				&& Property->GetName().StartsWith(OverridePrefix, ESearchCase::CaseSensitive))
			{
				continue;
			}
			Known.Add(KeyFor(Property).ToLower(), Property);
		}

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Object->Values)
		{
			const FProperty* const* Property = Known.Find(Field.Key.ToLower());
			if (Property == nullptr)
			{
				TArray<FString> Valid;
				for (const TPair<FString, const FProperty*>& Entry : Known)
				{
					Valid.Add(KeyFor(Entry.Value));
				}
				OutErrors.Add(FString::Printf(
					TEXT("%s: unknown field '%s'. Valid fields: %s."),
					*Path, *Field.Key, *FString::Join(Valid, TEXT(", "))));
				continue;
			}

			if (AsWrapperStruct(*Property) != nullptr || !Field.Value.IsValid())
			{
				continue;
			}

			const FString FieldPath = FString::Printf(TEXT("%s.%s"), *Path, *Field.Key);
			if (const FStructProperty* StructProperty = CastField<FStructProperty>(*Property))
			{
				if (Field.Value->Type == EJson::Object)
				{
					CollectUnknownKeys(StructProperty->Struct, Field.Value->AsObject(), FieldPath, OutErrors);
				}
			}
			else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(*Property))
			{
				const FStructProperty* Inner = CastField<FStructProperty>(ArrayProperty->Inner);
				if (Inner != nullptr && AsWrapperStruct(Inner) == nullptr && Field.Value->Type == EJson::Array)
				{
					const TArray<TSharedPtr<FJsonValue>>& Elements = Field.Value->AsArray();
					for (int32 Index = 0; Index < Elements.Num(); ++Index)
					{
						if (Elements[Index].IsValid() && Elements[Index]->Type == EJson::Object)
						{
							CollectUnknownKeys(
								Inner->Struct, Elements[Index]->AsObject(),
								FString::Printf(TEXT("%s[%d]"), *FieldPath, Index), OutErrors);
						}
					}
				}
			}
		}
	}

	void CollectUnknownKeys(
		const TSharedPtr<FJsonObject>& Object,
		std::initializer_list<const TCHAR*> Allowed,
		const FString&                 Path,
		TArray<FString>&               OutErrors)
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Object->Values)
		{
			bool bKnown = false;
			for (const TCHAR* Key : Allowed)
			{
				bKnown |= Field.Key.Equals(Key, ESearchCase::IgnoreCase);
			}
			if (!bKnown)
			{
				TArray<FString> Valid;
				for (const TCHAR* Key : Allowed)
				{
					Valid.Add(Key);
				}
				OutErrors.Add(FString::Printf(
					TEXT("%s: unknown field '%s'. Valid fields: %s."),
					*Path, *Field.Key, *FString::Join(Valid, TEXT(", "))));
			}
		}
	}

	/** JSON Schema type for a property, in the shape the caller actually sends. */
	FString JsonTypeFor(const FProperty* Property)
	{
		// Checked before the struct case: wrappers travel as plain strings.
		if (AsWrapperStruct(Property) != nullptr)
		{
			return TEXT("string");
		}
		if (Property->IsA<FStrProperty>() || Property->IsA<FNameProperty>()
			|| Property->IsA<FTextProperty>() || Property->IsA<FEnumProperty>()
			|| Property->IsA<FByteProperty>())
		{
			return TEXT("string");
		}
		if (Property->IsA<FBoolProperty>())
		{
			return TEXT("boolean");
		}
		if (Property->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>())
		{
			return TEXT("number");
		}
		if (Property->IsA<FNumericProperty>())
		{
			return TEXT("integer");
		}
		if (Property->IsA<FArrayProperty>() || Property->IsA<FSetProperty>())
		{
			return TEXT("array");
		}
		return TEXT("object");
	}

	const UEnum* NodeTypeEnum()
	{
		return StaticEnum<ETryllNodeType>();
	}

	FString NodeTypeToString(ETryllNodeType Type)
	{
		return NodeTypeEnum()->GetNameStringByValue(static_cast<int64>(Type));
	}

	bool TryParseNodeType(const FString& Name, ETryllNodeType& OutType)
	{
		const int64 Value = NodeTypeEnum()->GetValueByNameString(Name);
		if (Value == INDEX_NONE)
		{
			return false;
		}
		OutType = static_cast<ETryllNodeType>(Value);
		return true;
	}

	FString VariableTypeToString(ETryllVariableType Type)
	{
		return StaticEnum<ETryllVariableType>()->GetNameStringByValue(static_cast<int64>(Type));
	}

	bool TryParseVariableType(const FString& Name, ETryllVariableType& OutType)
	{
		const int64 Value = StaticEnum<ETryllVariableType>()->GetValueByNameString(Name);
		if (Value == INDEX_NONE)
		{
			return false;
		}
		OutType = static_cast<ETryllVariableType>(Value);
		return true;
	}

	/** Reads "value" into whichever FTryllVariableDecl field the declared type selects. */
	void ReadVariableValue(const TSharedPtr<FJsonObject>& Object, FTryllVariableDecl& OutDecl)
	{
		const TSharedPtr<FJsonValue> Value = Object->TryGetField(TEXT("value"));
		if (!Value.IsValid())
		{
			return;
		}

		switch (OutDecl.Type)
		{
			case ETryllVariableType::Int:
				OutDecl.IntValue = static_cast<int64>(Value->AsNumber());
				break;
			case ETryllVariableType::Float:
				OutDecl.FloatValue = Value->AsNumber();
				break;
			case ETryllVariableType::String:
				OutDecl.StringValue = Value->AsString();
				break;
			case ETryllVariableType::Bool:
				OutDecl.bBoolValue = Value->AsBool();
				break;
			case ETryllVariableType::StringSet:
				for (const TSharedPtr<FJsonValue>& Element : Value->AsArray())
				{
					OutDecl.StringSetValue.Add(Element->AsString());
				}
				break;
		}
	}

	TSharedPtr<FJsonValue> WriteVariableValue(const FTryllVariableDecl& Decl)
	{
		switch (Decl.Type)
		{
			case ETryllVariableType::Int:
				return MakeShared<FJsonValueNumber>(static_cast<double>(Decl.IntValue));
			case ETryllVariableType::Float:
				return MakeShared<FJsonValueNumber>(Decl.FloatValue);
			case ETryllVariableType::String:
				return MakeShared<FJsonValueString>(Decl.StringValue);
			case ETryllVariableType::Bool:
				return MakeShared<FJsonValueBoolean>(Decl.bBoolValue);
			case ETryllVariableType::StringSet:
			{
				TArray<TSharedPtr<FJsonValue>> Elements;
				for (const FString& Element : Decl.StringSetValue)
				{
					Elements.Add(MakeShared<FJsonValueString>(Element));
				}
				return MakeShared<FJsonValueArray>(Elements);
			}
		}
		return MakeShared<FJsonValueString>(FString());
	}
}

namespace TryllWorkflowGraphJson
{
	TMap<ETryllNodeType, UClass*> BuildNodeParamsClassMap()
	{
		// Only the module that owns the base class is considered. Subclasses defined
		// elsewhere — the test doubles in TryllClientTests, or anything a project
		// adds — are free to return a placeholder from GetNodeType(), and several do
		// return 0, which is Generate. Scanning every subclass would let one of those
		// win the mapping for a real node type.
		const UPackage* const OwningPackage = UTryllNodeParamsBase::StaticClass()->GetOutermost();

		TMap<ETryllNodeType, UClass*> Map;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Class = *It;
			if (!Class->IsChildOf(UTryllNodeParamsBase::StaticClass())
				|| Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
				|| Class->GetOutermost() != OwningPackage)
			{
				continue;
			}

			// GetNodeType() on the CDO rather than a name convention, so the map
			// survives any renaming on the codegen side.
			if (const UTryllNodeParamsBase* Defaults = Class->GetDefaultObject<UTryllNodeParamsBase>())
			{
				Map.FindOrAdd(Defaults->GetNodeType(), Class);
			}
		}
		return Map;
	}

	FParseResult Parse(const FString& Json, UObject* ParamsOuter)
	{
		FParseResult Result;

		TSharedPtr<FJsonObject>   Root;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			Result.Errors.Add(TEXT("Graph JSON is not a valid JSON object."));
			return Result;
		}

		CollectUnknownKeys(
			Root, { TEXT("startNode"), TEXT("defaultModelName"), TEXT("variables"), TEXT("nodes") },
			TEXT("graph"), Result.Errors);

		Root->TryGetStringField(TEXT("startNode"), Result.Graph.StartNode);

		FString DefaultModel;
		if (Root->TryGetStringField(TEXT("defaultModelName"), DefaultModel))
		{
			Result.Graph.DefaultModelName.Name = DefaultModel;
		}

		const TArray<TSharedPtr<FJsonValue>>* Variables = nullptr;
		if (Root->TryGetArrayField(TEXT("variables"), Variables))
		{
			for (int32 Index = 0; Index < Variables->Num(); ++Index)
			{
				const TSharedPtr<FJsonObject>* Entry = nullptr;
				if (!(*Variables)[Index]->TryGetObject(Entry))
				{
					Result.Errors.Add(FString::Printf(TEXT("variables[%d] is not an object."), Index));
					continue;
				}

				FTryllVariableDecl Decl;
				(*Entry)->TryGetStringField(TEXT("name"), Decl.Name);

				CollectUnknownKeys(
					*Entry, { TEXT("name"), TEXT("type"), TEXT("allowOutputSubstitution"), TEXT("value") },
					FString::Printf(TEXT("variables[%d] '%s'"), Index, *Decl.Name), Result.Errors);

				FString TypeName;
				(*Entry)->TryGetStringField(TEXT("type"), TypeName);
				if (!TryParseVariableType(TypeName, Decl.Type))
				{
					Result.Errors.Add(FString::Printf(
						TEXT("variables[%d] '%s': unknown type '%s'. Expected Int, Float, String, Bool or StringSet."),
						Index, *Decl.Name, *TypeName));
					continue;
				}

				(*Entry)->TryGetBoolField(TEXT("allowOutputSubstitution"), Decl.bAllowOutputSubstitution);
				ReadVariableValue(*Entry, Decl);
				Result.Variables.Add(MoveTemp(Decl));
			}
		}

		const TMap<ETryllNodeType, UClass*> ClassMap = BuildNodeParamsClassMap();

		const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
		if (!Root->TryGetArrayField(TEXT("nodes"), Nodes))
		{
			Result.Errors.Add(TEXT("Graph JSON has no 'nodes' array."));
			return Result;
		}

		for (int32 Index = 0; Index < Nodes->Num(); ++Index)
		{
			const TSharedPtr<FJsonObject>* Entry = nullptr;
			if (!(*Nodes)[Index]->TryGetObject(Entry))
			{
				Result.Errors.Add(FString::Printf(TEXT("nodes[%d] is not an object."), Index));
				continue;
			}

			FTryllNodeDescription Node;
			(*Entry)->TryGetStringField(TEXT("name"), Node.Name);

			FString TypeName;
			(*Entry)->TryGetStringField(TEXT("type"), TypeName);

			const FString NodePath = FString::Printf(TEXT("nodes[%d] '%s'"), Index, *Node.Name);
			CollectUnknownKeys(*Entry, { TEXT("name"), TEXT("type"), TEXT("params") }, NodePath, Result.Errors);

			ETryllNodeType NodeType;
			if (!TryParseNodeType(TypeName, NodeType))
			{
				Result.Errors.Add(FString::Printf(
					TEXT("nodes[%d] '%s': unknown type '%s'. Call DescribeNodeTypes for the list."),
					Index, *Node.Name, *TypeName));
				continue;
			}

			UClass* const* ParamsClass = ClassMap.Find(NodeType);
			if (ParamsClass == nullptr || *ParamsClass == nullptr)
			{
				Result.Errors.Add(FString::Printf(
					TEXT("nodes[%d] '%s': node type '%s' has no params class registered in this build."),
					Index, *Node.Name, *TypeName));
				continue;
			}

			UTryllNodeParamsBase* Params = NewObject<UTryllNodeParamsBase>(ParamsOuter, *ParamsClass);

			const TSharedPtr<FJsonObject>* ParamsJson = nullptr;
			if ((*Entry)->TryGetObjectField(TEXT("params"), ParamsJson) && ParamsJson != nullptr)
			{
				const int32 ErrorsBefore = Result.Errors.Num();
				CollectUnknownKeys(
					*ParamsClass, *ParamsJson,
					FString::Printf(TEXT("%s (%s).params"), *NodePath, *TypeName), Result.Errors);
				if (Result.Errors.Num() != ErrorsBefore)
				{
					continue;
				}

				// Copied because both passes mutate: wrappers are expanded before
				// conversion, and presence is read back afterwards for the toggles.
				const TSharedRef<FJsonObject> Working = MakeShared<FJsonObject>(**ParamsJson);
				ExpandWrappers(*ParamsClass, Working);

				FText FailReason;
				if (!FJsonObjectConverter::JsonObjectToUStruct(
						Working, *ParamsClass, Params, 0, 0, false, &FailReason))
				{
					Result.Errors.Add(FString::Printf(
						TEXT("nodes[%d] '%s': params could not be applied — %s"),
						Index, *Node.Name, *FailReason.ToString()));
					continue;
				}

				ApplyOverrideFlags(*ParamsClass, Params, *ParamsJson);
			}

			Node.Params = Params;
			Result.Graph.Nodes.Add(MoveTemp(Node));
		}

		return Result;
	}

	FString Serialize(
		const FTryllGraphDescription& Graph, const TArray<FTryllVariableDecl>& Variables)
	{
		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetStringField(TEXT("startNode"), Graph.StartNode);
		Root->SetStringField(TEXT("defaultModelName"), Graph.DefaultModelName.Name);

		TArray<TSharedPtr<FJsonValue>> VariableValues;
		for (const FTryllVariableDecl& Decl : Variables)
		{
			const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("name"), Decl.Name);
			Entry->SetStringField(TEXT("type"), VariableTypeToString(Decl.Type));
			Entry->SetBoolField(TEXT("allowOutputSubstitution"), Decl.bAllowOutputSubstitution);
			Entry->SetField(TEXT("value"), WriteVariableValue(Decl));
			VariableValues.Add(MakeShared<FJsonValueObject>(Entry));
		}
		Root->SetArrayField(TEXT("variables"), VariableValues);

		TArray<TSharedPtr<FJsonValue>> NodeValues;
		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("name"), Node.Name);

			if (Node.Params != nullptr)
			{
				UClass* const ParamsClass = Node.Params->GetClass();
				Entry->SetStringField(TEXT("type"), NodeTypeToString(Node.Params->GetNodeType()));

				const TSharedRef<FJsonObject> ParamsJson = MakeShared<FJsonObject>();
				FJsonObjectConverter::UStructToJsonObject(ParamsClass, Node.Params, ParamsJson);
				CollapseForOutput(
					ParamsClass, Node.Params, ParamsClass->GetDefaultObject(), ParamsJson);
				Entry->SetObjectField(TEXT("params"), ParamsJson);
			}
			else
			{
				// Reachable for assets authored before this tool existed, or written
				// by the generic reflection path, which leaves Params null on success.
				Entry->SetStringField(TEXT("type"), FString());
				Entry->SetObjectField(TEXT("params"), MakeShared<FJsonObject>());
			}

			NodeValues.Add(MakeShared<FJsonValueObject>(Entry));
		}
		Root->SetArrayField(TEXT("nodes"), NodeValues);

		FString                                Out;
		const TSharedRef<TJsonWriter<>>        Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Root, Writer);
		return Out;
	}

	FString DescribeNodeTypes(const FString& NodeTypeFilter)
	{
		const TMap<ETryllNodeType, UClass*> ClassMap = BuildNodeParamsClassMap();

		TArray<TSharedPtr<FJsonValue>> Types;
		for (const TPair<ETryllNodeType, UClass*>& Pair : ClassMap)
		{
			const FString TypeName = NodeTypeToString(Pair.Key);
			if (!NodeTypeFilter.IsEmpty() && !TypeName.Equals(NodeTypeFilter, ESearchCase::IgnoreCase))
			{
				continue;
			}

			const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("type"), TypeName);
			Entry->SetStringField(
				TEXT("description"),
				NodeTypeEnum()->GetToolTipTextByIndex(NodeTypeEnum()->GetIndexByValue(
					static_cast<int64>(Pair.Key))).ToString());

			// Which fields carry a bOverrideX toggle, and are therefore inherited when
			// omitted. Their documentation is not recoverable here: codegen emits the
			// field's comment block ahead of the toggle, so UHT attaches the toggle's
			// own boilerplate instead and the real text never reaches reflection.
			// Reporting that boilerplate would tell the caller to send a boolean, so
			// these fields are marked optional and left undescribed.
			TSet<FString> GuardedFields;
			for (TFieldIterator<FProperty> It(Pair.Value); It; ++It)
			{
				const FString Name = (*It)->GetName();
				if (CastField<FBoolProperty>(*It) != nullptr
					&& Name.StartsWith(OverridePrefix, ESearchCase::CaseSensitive))
				{
					GuardedFields.Add(FJsonObjectConverter::StandardizeCase(
						Name.RightChop(FCString::Strlen(OverridePrefix))));
				}
			}

			const TSharedRef<FJsonObject> Fields = MakeShared<FJsonObject>();
			for (TFieldIterator<FProperty> It(Pair.Value); It; ++It)
			{
				const FProperty* Property = *It;
				const FString     Name     = Property->GetName();
				const FString     Key      = KeyFor(Property);

				// Toggles are an implementation detail: presence drives them.
				if (Name.StartsWith(OverridePrefix, ESearchCase::CaseSensitive))
				{
					continue;
				}

				const TSharedRef<FJsonObject> Field = MakeShared<FJsonObject>();
				Field->SetStringField(TEXT("type"), JsonTypeFor(Property));

				const FString ToolTip = Property->GetToolTipText().ToString();
				if (!ToolTip.IsEmpty())
				{
					Field->SetStringField(TEXT("description"), ToolTip);
				}

				if (GuardedFields.Contains(Key))
				{
					Field->SetBoolField(TEXT("optional"), true);
				}

				if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
				{
					const UEnum*                   Enum = EnumProperty->GetEnum();
					TArray<TSharedPtr<FJsonValue>> Values;
					// Last entry is the generated _MAX sentinel.
					for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
					{
						Values.Add(MakeShared<FJsonValueString>(Enum->GetNameStringByIndex(Index)));
					}
					Field->SetArrayField(TEXT("enum"), Values);
				}

				if (Property->HasMetaData(TEXT("ClampMin")))
				{
					Field->SetStringField(TEXT("minimum"), Property->GetMetaData(TEXT("ClampMin")));
				}
				if (Property->HasMetaData(TEXT("ClampMax")))
				{
					Field->SetStringField(TEXT("maximum"), Property->GetMetaData(TEXT("ClampMax")));
				}
				if (Property->HasMetaData(TEXT("GetOptions")))
				{
					Field->SetBoolField(TEXT("isExit"), true);
				}

				Fields->SetObjectField(Key, Field);
			}

			Entry->SetObjectField(TEXT("params"), Fields);
			Types.Add(MakeShared<FJsonValueObject>(Entry));
		}

		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetArrayField(TEXT("nodeTypes"), Types);

		FString                         Out;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
		FJsonSerializer::Serialize(Root, Writer);
		return Out;
	}
}
