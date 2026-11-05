// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllWorkflowVariablesValidator.h"
#include "TryllVarRefs.h"
#include "TryllSlotGraphAnalysis.h"
#include "Generated/Nodes/TryllNodeParamsBase.h"
#include "Generated/Nodes/TryllBranchParams.h"
#include "UObject/UnrealType.h"
#include "Internationalization/Regex.h"

namespace
{
	/** Find a string UPROPERTY on Params by exact field name (generic reflection, no per-type Cast dispatch). */
	template <typename FVisitor>
	void VisitStringFieldByName(const UTryllNodeParamsBase* Params, const TCHAR* FieldName, FVisitor&& Visitor)
	{
		if (!Params) return;
		for (TFieldIterator<FStrProperty> It(Params->GetClass()); It; ++It)
		{
			if (It->GetName() != FieldName) continue;
			if (const FString* Value = It->ContainerPtrToValuePtr<FString>(Params))
				Visitor(*Value);
			return;
		}
	}

	/**
	 * Visits every Filter/Condition JSON field's current value. By schema convention this
	 * is the field named "Filter" (Retrieve/ClassifyIntent) or "Condition" (Branch's
	 * Expression test) — the server only ever compiles those two fields as filter JSON.
	 */
	template <typename FVisitor>
	void VisitFilterJsonFields(const UTryllNodeParamsBase* Params, FVisitor&& Visitor)
	{
		VisitStringFieldByName(Params, TEXT("Filter"), Visitor);
		VisitStringFieldByName(Params, TEXT("Condition"), Visitor);
	}

	/**
	 * Extract every {"var": "<name>"} operand's name from a filter JSON. A lightweight
	 * regex scan (not a full JSON parse) — sufficient for validation since the filter
	 * grammar's var operand is always a two-token "var"/string-literal pair.
	 */
	TArray<FString> ExtractFilterVarRefs(const FString& FilterJson)
	{
		TArray<FString> Out;
		static const FRegexPattern Pattern(TEXT("\"var\"\\s*:\\s*\"([A-Za-z_][A-Za-z0-9_]*)\""));
		FRegexMatcher Matcher(Pattern, FilterJson);
		while (Matcher.FindNext())
		{
			Out.Add(Matcher.GetCaptureGroup(1));
		}
		return Out;
	}

	/**
	 * Extract every {"slot": "<name>"} operand's name from a filter JSON — currently only
	 * meaningful for Branch's Expression Condition (Retrieve/ClassifyIntent Filter compiles
	 * with slot capability disabled, so this never matches there).
	 */
	TArray<FString> ExtractFilterSlotRefs(const FString& FilterJson)
	{
		TArray<FString> Out;
		static const FRegexPattern Pattern(TEXT("\"slot\"\\s*:\\s*\"([A-Za-z_][A-Za-z0-9_]*)\""));
		FRegexMatcher Matcher(Pattern, FilterJson);
		while (Matcher.FindNext())
		{
			Out.Add(Matcher.GetCaptureGroup(1));
		}
		return Out;
	}

	FString BranchTestName(ETryllBranchTest Test)
	{
		switch (Test)
		{
		case ETryllBranchTest::NonEmpty:         return TEXT("NonEmpty");
		case ETryllBranchTest::IsTrue:           return TEXT("IsTrue");
		case ETryllBranchTest::Equals:           return TEXT("Equals");
		case ETryllBranchTest::EqualsIgnoreCase: return TEXT("EqualsIgnoreCase");
		case ETryllBranchTest::Contains:         return TEXT("Contains");
		case ETryllBranchTest::Regex:            return TEXT("Regex");
		case ETryllBranchTest::Expression:       return TEXT("Expression");
		default:                                 return TEXT("Unknown");
		}
	}
}

namespace TryllWorkflowVariablesValidator
{
	TArray<FTryllWorkflowIssue> Validate(
		const FTryllGraphDescription& Graph, const TArray<FTryllVariableDecl>& DeclaredVariables)
	{
		TArray<FTryllWorkflowIssue> Issues;
		const auto AddIssue = [&](FTryllWorkflowIssue::ESeverity Sev, FString Msg)
		{
			Issues.Add({ Sev, MoveTemp(Msg) });
		};

		TSet<FString> DeclaredNames;
		TMap<FString, FString> EnabledFoldToCanonical;
		for (const FTryllVariableDecl& D : DeclaredVariables)
		{
			if (!D.Name.IsEmpty())
			{
				DeclaredNames.Add(D.Name);
			}
			if (!D.bAllowOutputSubstitution)
				continue;

			if (D.Type == ETryllVariableType::StringSet)
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					FString::Printf(
						TEXT("Variable '%s' cannot enable output substitution on a StringSet."),
						*D.Name));
				continue;
			}
			if (D.Name.Contains(TEXT("__")) || D.Name.EndsWith(TEXT("_")))
			{
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					FString::Printf(
						TEXT("Variable '%s' allows output substitution, so its name must not contain '__' or end with '_'."),
						*D.Name));
			}
			if (D.Type == ETryllVariableType::String)
			{
				const FTCHARToUTF8 Utf8(*D.StringValue);
				const int32 Utf8Bytes = Utf8.Length();
				if (Utf8Bytes > 2048)
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						FString::Printf(
							TEXT("Variable '%s' allows output substitution, so its string value must be at most 2048 UTF-8 bytes (got %d)."),
							*D.Name, Utf8Bytes));
				}
			}
			if (!D.Name.IsEmpty())
			{
				const FString Fold = D.Name.ToLower();
				if (const FString* Prior = EnabledFoldToCanonical.Find(Fold))
				{
					AddIssue(FTryllWorkflowIssue::ESeverity::Error,
						FString::Printf(
							TEXT("Variables '%s' and '%s' both allow output substitution and collide under ASCII case-folding."),
							**Prior, *D.Name));
				}
				else
				{
					EnabledFoldToCanonical.Add(Fold, D.Name);
				}
			}
		}

		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			if (!Node.Params || Node.Name.IsEmpty())
				continue;

			VisitStringFieldByName(Node.Params, TEXT("Template"), [&](const FString& Tmpl)
			{
				if (Tmpl.IsEmpty()) return;
				for (const FString& VarName : TryllVarRefs::ExtractRefs(Tmpl))
				{
					if (!DeclaredNames.Contains(VarName))
					{
						AddIssue(FTryllWorkflowIssue::ESeverity::Error,
							FString::Printf(TEXT("Node '%s' template references undeclared variable 'var.%s'."),
								*Node.Name, *VarName));
					}
				}
			});

			VisitFilterJsonFields(Node.Params, [&](const FString& FilterJson)
			{
				if (FilterJson.IsEmpty()) return;
				for (const FString& VarName : ExtractFilterVarRefs(FilterJson))
				{
					if (!DeclaredNames.Contains(VarName))
					{
						AddIssue(FTryllWorkflowIssue::ESeverity::Error,
							FString::Printf(TEXT("Node '%s' filter references undeclared variable '{\"var\": \"%s\"}'."),
								*Node.Name, *VarName));
					}
				}

				for (const FString& SlotName : ExtractFilterSlotRefs(FilterJson))
				{
					const TArray<FString> Upstream = TryllSlotGraphAnalysis::GetUpstreamSlots(Graph, Node.Name);
					if (!Upstream.Contains(SlotName))
					{
						AddIssue(FTryllWorkflowIssue::ESeverity::Error,
							FString::Printf(TEXT("Node '%s' filter references slot '%s', which is not producible on any path reaching this node."),
								*Node.Name, *SlotName));
					}
					else if (!TryllSlotGraphAnalysis::IsMustAvailable(Graph, Node.Name, SlotName))
					{
						AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
							FString::Printf(TEXT("Node '%s' filter references slot '%s', which is only produced on some branches reaching this node (may be empty at runtime)."),
								*Node.Name, *SlotName));
					}
				}
			});
		}

		// ── Branch cross-field coherence checks ────────────────────────────
		// Mirrors the server's ValidateBranchParamShape plus a couple of client-only
		// authoring hints. Condition *grammar* is never validated client-side.
		for (const FTryllNodeDescription& Node : Graph.Nodes)
		{
			const auto* Branch = Cast<UTryllBranchParams>(Node.Params);
			if (!Branch || Node.Name.IsEmpty())
				continue;

			const bool bHasInput      = !Branch->Input.Value.IsEmpty();
			const bool bHasVariable   = !Branch->Variable.IsEmpty();
			const bool bIsExpression  = Branch->Test == ETryllBranchTest::Expression;
			const bool bValuesEmpty   = Branch->Values.Num() == 0;
			const bool bIsTextTest    =
				Branch->Test == ETryllBranchTest::Equals ||
				Branch->Test == ETryllBranchTest::EqualsIgnoreCase ||
				Branch->Test == ETryllBranchTest::Contains ||
				Branch->Test == ETryllBranchTest::Regex;

			if (bIsTextTest && bValuesEmpty)
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					FString::Printf(TEXT("Node '%s': test=%s requires a non-empty 'values' list."),
						*Node.Name, *BranchTestName(Branch->Test)));

			if (bIsExpression && Branch->Condition.IsEmpty())
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					FString::Printf(TEXT("Node '%s': test=Expression requires a non-empty 'condition'."), *Node.Name));

			if (!bIsExpression && bHasInput && bHasVariable)
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					FString::Printf(TEXT("Node '%s': set either 'input' or 'variable', not both."), *Node.Name));

			if (bHasVariable && !DeclaredNames.Contains(Branch->Variable))
				AddIssue(FTryllWorkflowIssue::ESeverity::Error,
					FString::Printf(TEXT("Node '%s' references undeclared variable '%s'."), *Node.Name, *Branch->Variable));

			if (!bIsExpression && !Branch->Condition.IsEmpty())
				AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
					FString::Printf(TEXT("Node '%s': 'condition' is ignored under test=%s (only used by Expression)."),
						*Node.Name, *BranchTestName(Branch->Test)));

			const bool bValuesIgnored =
				Branch->Test == ETryllBranchTest::IsTrue ||
				Branch->Test == ETryllBranchTest::NonEmpty ||
				bIsExpression;
			if (bValuesIgnored && !bValuesEmpty)
				AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
					FString::Printf(TEXT("Node '%s': 'values' is ignored under test=%s."),
						*Node.Name, *BranchTestName(Branch->Test)));

			if (Branch->ThenExit == Branch->ElseExit)
				AddIssue(FTryllWorkflowIssue::ESeverity::Warning,
					FString::Printf(TEXT("Node '%s': then_exit and else_exit both target '%s'; the branch never changes routing."),
						*Node.Name, Branch->ThenExit.IsEmpty() ? TEXT("END") : *Branch->ThenExit));

			// Regex-compiles check intentionally omitted here: FRegexPattern/FRegexMatcher
			// expose no parse-error API, so there is no reliable way to detect an invalid
			// pattern client-side in Unreal (unlike Unity's .NET Regex, which throws). See
			// the Unity TryllWorkflowValidator.cs mirror of this check for the equivalent.
		}

		return Issues;
	}
}
