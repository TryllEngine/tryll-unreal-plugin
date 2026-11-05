// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllRenamePropagation.h"

#include "TryllGraphDescription.h"
#include "TryllSlotInput.h"
#include "TryllSlotName.h"
#include "TryllMustacheRefs.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "Generated/Nodes/TryllClassifyAttributesLLMParams.h"
#include "UObject/UnrealType.h"
#include "Internationalization/Regex.h"

#define LOCTEXT_NAMESPACE "TryllRenamePropagation"

namespace
{
	const FRegexPattern kGrammarPattern(TEXT("^[A-Za-z_][A-Za-z0-9_]*$"));

	/** Mutable pointer to the (at most one) FTryllSlotName-typed property value on Params. */
	FTryllSlotName* FindSlotNameValue(UTryllNodeParamsBase* Params)
	{
		if (!Params) return nullptr;
		for (TFieldIterator<FStructProperty> It(Params->GetClass()); It; ++It)
			if (It->Struct == FTryllSlotName::StaticStruct())
				return It->ContainerPtrToValuePtr<FTryllSlotName>(Params);
		return nullptr;
	}

	/** Read-only counterpart of the above, for callers holding a const params object. */
	const FTryllSlotName* FindSlotNameValue(const UTryllNodeParamsBase* Params)
	{
		if (!Params) return nullptr;
		for (TFieldIterator<FStructProperty> It(Params->GetClass()); It; ++It)
			if (It->Struct == FTryllSlotName::StaticStruct())
				return It->ContainerPtrToValuePtr<FTryllSlotName>(Params);
		return nullptr;
	}

	/** Visits every FTryllSlotInput-typed property value on Params (mutable). */
	void VisitSlotInputValues(UTryllNodeParamsBase* Params, TFunctionRef<void(FTryllSlotInput&)> Visitor)
	{
		if (!Params) return;
		for (TFieldIterator<FStructProperty> It(Params->GetClass()); It; ++It)
			if (It->Struct == FTryllSlotInput::StaticStruct())
				if (FTryllSlotInput* Value = It->ContainerPtrToValuePtr<FTryllSlotInput>(Params))
					Visitor(*Value);
	}

	/**
	 * Visits every `meta=(GetOptions="GetExitTargetOptions")` string field value on Params
	 * (mutable) — the exit fields, by schema convention (see nodes.fbs `(exit)` attribute).
	 */
	void VisitExitFieldValues(UTryllNodeParamsBase* Params, TFunctionRef<void(FString&)> Visitor)
	{
		if (!Params) return;
		for (TFieldIterator<FStrProperty> It(Params->GetClass()); It; ++It)
		{
			FStrProperty* StrProp = *It;
#if WITH_EDITORONLY_DATA
			if (!StrProp->HasMetaData(TEXT("GetOptions")) ||
			    StrProp->GetMetaData(TEXT("GetOptions")) != TEXT("GetExitTargetOptions"))
				continue;
#endif
			if (FString* Value = StrProp->ContainerPtrToValuePtr<FString>(Params))
				Visitor(*Value);
		}
	}

	/**
	 * Mustache-template fields to scan for `{{slot.<name>}}` refs. By schema convention this
	 * is exactly the field named `Template` (Generate/GenerateAndSpeak/Transform) — the
	 * server only ever scans each node type's `template` field, never `system_prompt` or
	 * other multiline text.
	 */
	void VisitTemplateFieldValues(UTryllNodeParamsBase* Params, TFunctionRef<void(FString&)> Visitor)
	{
		if (!Params) return;
		if (FProperty* Prop = Params->GetClass()->FindPropertyByName(TEXT("Template")))
			if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
				if (FString* Value = StrProp->ContainerPtrToValuePtr<FString>(Params))
					Visitor(*Value);
	}

	/**
	 * Filter/Condition JSON fields to scan for `{"slot": "<name>"}` operands. By schema
	 * convention this is the field named `Filter` (Retrieve/ClassifyIntent) or `Condition`
	 * (Branch's Expression test) — the server only ever compiles those two fields as
	 * filter JSON.
	 */
	void VisitFilterJsonFieldValues(UTryllNodeParamsBase* Params, TFunctionRef<void(FString&)> Visitor)
	{
		if (!Params) return;
		for (const TCHAR* FieldName : { TEXT("Filter"), TEXT("Condition") })
		{
			if (FProperty* Prop = Params->GetClass()->FindPropertyByName(FieldName))
				if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
					if (FString* Value = StrProp->ContainerPtrToValuePtr<FString>(Params))
						Visitor(*Value);
		}
	}

	/**
	 * Rewrites every `{"slot": "<from>"}` operand in FilterJson to reference `to` instead.
	 * A lightweight regex scan/rewrite (not a full JSON parse) — sufficient since the
	 * filter grammar's slot operand is always a two-token "slot"/string-literal pair.
	 */
	FString RewriteFilterSlotRefs(const FString& FilterJson, const TMap<FString, FString>& Renames)
	{
		if (FilterJson.IsEmpty() || Renames.Num() == 0) return FilterJson;

		static const FRegexPattern Pattern(TEXT("\"slot\"\\s*:\\s*\"([A-Za-z_][A-Za-z0-9_]*)\""));
		FRegexMatcher Matcher(Pattern, FilterJson);

		FString Result;
		int32 Cursor = 0;
		bool bChanged = false;
		while (Matcher.FindNext())
		{
			const int32 MatchStart = Matcher.GetMatchBeginning();
			const int32 MatchEnd   = Matcher.GetMatchEnding();
			Result += FilterJson.Mid(Cursor, MatchStart - Cursor);

			const FString Captured = Matcher.GetCaptureGroup(1);
			const FString* To = Renames.Find(Captured);
			if (To && *To != Captured)
			{
				const int32 CapStart = Matcher.GetCaptureGroupBeginning(1);
				const int32 CapEnd   = Matcher.GetCaptureGroupEnding(1);
				Result += FilterJson.Mid(MatchStart, CapStart - MatchStart);
				Result += *To;
				Result += FilterJson.Mid(CapEnd, MatchEnd - CapEnd);
				bChanged = true;
			}
			else
			{
				Result += FilterJson.Mid(MatchStart, MatchEnd - MatchStart);
			}
			Cursor = MatchEnd;
		}
		Result += FilterJson.Mid(Cursor);
		return bChanged ? Result : FilterJson;
	}
}

namespace TryllRenamePropagation
{
	const FString ReservedUserMessage = TEXT("user_message");

	bool IsValidGrammar(const FString& Name)
	{
		if (Name.IsEmpty()) return false;
		FRegexMatcher Matcher(kGrammarPattern, Name);
		return Matcher.FindNext();
	}

	bool IsReserved(const FString& Name)
	{
		return Name == ReservedUserMessage;
	}

	FString Uniquify(const FString& BaseName, const TSet<FString>& ExistingNames)
	{
		if (BaseName.IsEmpty() || !ExistingNames.Contains(BaseName))
			return BaseName;

		int32 I = 2;
		FString Candidate;
		do
		{
			Candidate = BaseName + FString::FromInt(I);
			++I;
		} while (ExistingNames.Contains(Candidate));
		return Candidate;
	}

	FNodeRenameOutcome ResolveNodeRename(const FString& RequestedName, const TSet<FString>& ExistingNames)
	{
		FNodeRenameOutcome Outcome;
		const FString Requested = RequestedName.TrimStartAndEnd();

		if (Requested.IsEmpty())
		{
			Outcome.Error = LOCTEXT("RenameEmpty", "Node name cannot be empty.");
			return Outcome;
		}
		if (!IsValidGrammar(Requested))
		{
			Outcome.Error = LOCTEXT("RenameGrammar",
				"Node name must be letters, digits and underscores only, and cannot start with a digit.");
			return Outcome;
		}
		if (IsReserved(Requested))
		{
			Outcome.Error = FText::Format(
				LOCTEXT("RenameReserved", "'{0}' is reserved for the implicit input slot."),
				FText::FromString(ReservedUserMessage));
			return Outcome;
		}

		Outcome.bValid = true;
		Outcome.Name = Uniquify(Requested, ExistingNames);
		return Outcome;
	}

	bool EffectiveSlotIsNodeName(const UTryllNodeParamsBase* Params)
	{
		if (!Params) return false;
		const FTryllSlotName* SlotName = FindSlotNameValue(Params);
		return SlotName && SlotName->Value.IsEmpty();
	}

	bool EffectiveSlotIsNodeName(const FTryllNodeDescription& Node)
	{
		return EffectiveSlotIsNodeName(Node.Params);
	}

	void PropagateNodeRename(FTryllGraphDescription& Graph, const FString& OldName, const FString& NewName)
	{
		if (OldName.IsEmpty() || OldName == NewName) return;

		if (Graph.StartNode == OldName)
			Graph.StartNode = NewName;

		for (FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (!Node.Params) continue;
			VisitExitFieldValues(Node.Params, [&](FString& Value)
			{
				if (Value == OldName) Value = NewName;
			});
		}
	}

	// Applies every pair against the original reference. A value written by one
	// pair is not eligible for another pair in the same call.
	void ApplySlotRenames(FTryllGraphDescription& Graph, const TMap<FString, FString>& Renames)
	{
		if (Renames.Num() == 0) return;

		for (FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (!Node.Params) continue;

			VisitSlotInputValues(Node.Params, [&](FTryllSlotInput& Input)
			{
				if (const FString* NewSlot = Renames.Find(Input.Value))
					Input.Value = *NewSlot;
			});

			VisitTemplateFieldValues(Node.Params, [&](FString& Tmpl)
			{
				if (Tmpl.IsEmpty()) return;
				const FString Rewritten = TryllMustacheRefs::Rename(Tmpl, Renames);
				if (Rewritten != Tmpl) Tmpl = Rewritten;
			});

			VisitFilterJsonFieldValues(Node.Params, [&](FString& FilterJson)
			{
				if (FilterJson.IsEmpty()) return;
				const FString Rewritten = RewriteFilterSlotRefs(FilterJson, Renames);
				if (Rewritten != FilterJson) FilterJson = Rewritten;
			});
		}
	}

	void PropagateOwnedSlotRename(
		FTryllGraphDescription& Graph,
		const UTryllNodeParamsBase* Params,
		const FString& OldPrefix,
		const FString& NewPrefix)
	{
		if (const UTryllClassifyAttributesLLMParams* Attributes =
			Cast<UTryllClassifyAttributesLLMParams>(Params))
		{
			TMap<FString, FString> Renames;
			for (const FTryllClassifyAttributeDef& Attribute : Attributes->Attributes)
			{
				if (Attribute.Name.IsEmpty()) continue;
				const FString OldSlot = OldPrefix + TEXT("_") + Attribute.Name;
				const FString NewSlot = NewPrefix + TEXT("_") + Attribute.Name;
				if (OldSlot == NewSlot) continue;
				Renames.Add(OldSlot, NewSlot);
			}
			ApplySlotRenames(Graph, Renames);
			return;
		}
		PropagateSlotRename(Graph, OldPrefix, NewPrefix);
	}

	void PropagateAttributeRename(
		FTryllGraphDescription& Graph,
		const UTryllClassifyAttributesLLMParams* Params,
		const FString& OwnerNodeName,
		const FString& OldAttributeName,
		const FString& NewAttributeName)
	{
		if (!Params || OldAttributeName.IsEmpty() || NewAttributeName.IsEmpty()
			|| OldAttributeName == NewAttributeName)
			return;
		const FString Prefix = Params->OutputName.Value.IsEmpty()
			? OwnerNodeName : Params->OutputName.Value;
		if (Prefix.IsEmpty()) return;
		PropagateSlotRename(
			Graph,
			Prefix + TEXT("_") + OldAttributeName,
			Prefix + TEXT("_") + NewAttributeName);
	}

	void PropagateSlotRename(FTryllGraphDescription& Graph, const FString& OldSlot, const FString& NewSlot)
	{
		if (OldSlot.IsEmpty() || OldSlot == NewSlot) return;
		TMap<FString, FString> Renames;
		Renames.Add(OldSlot, NewSlot);
		ApplySlotRenames(Graph, Renames);
	}
}

#undef LOCTEXT_NAMESPACE
