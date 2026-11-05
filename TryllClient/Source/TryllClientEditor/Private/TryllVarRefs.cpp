// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllVarRefs.h"

namespace TryllVarRefs
{
// Nested in TryllVarRefs, not at file scope: TryllMustacheRefs.cpp has same-signature
// IsNameChar / MatchesAt / SkipSpaces in its own anonymous namespace, and the two
// would redefine each other in a unity blob (td-042). The scanners stay independent
// on purpose (see TryllVarRefs.h), so these are not shared.
namespace
{
	const FString kVarBody = TEXT("var.");

	bool IsNameChar(TCHAR C)
	{
		return (C >= TEXT('a') && C <= TEXT('z')) ||
		       (C >= TEXT('A') && C <= TEXT('Z')) ||
		       (C >= TEXT('0') && C <= TEXT('9')) ||
		       C == TEXT('_');
	}

	bool MatchesAt(const FString& S, int32 Pos, const FString& Literal)
	{
		if (Pos + Literal.Len() > S.Len())
			return false;
		for (int32 I = 0; I < Literal.Len(); ++I)
			if (S[Pos + I] != Literal[I])
				return false;
		return true;
	}

	int32 SkipSpaces(const FString& S, int32 Pos)
	{
		const int32 N = S.Len();
		while (Pos < N && FChar::IsWhitespace(S[Pos]))
			++Pos;
		return Pos;
	}
}

	TArray<FString> ExtractRefs(const FString& Tmpl)
	{
		TArray<FString> Out;
		const int32 N = Tmpl.Len();
		int32 Pos = 0;
		while (Pos < N)
		{
			int32 BraceCount = 0;
			if (MatchesAt(Tmpl, Pos, TEXT("{{{")))
				BraceCount = 3;
			else if (MatchesAt(Tmpl, Pos, TEXT("{{")))
				BraceCount = 2;

			if (BraceCount > 0)
			{
				const int32 BodyStart = SkipSpaces(Tmpl, Pos + BraceCount);
				if (MatchesAt(Tmpl, BodyStart, kVarBody))
				{
					const int32 IdentStart = BodyStart + kVarBody.Len();
					int32 IdentEnd = IdentStart;
					while (IdentEnd < N && IsNameChar(Tmpl[IdentEnd]))
						++IdentEnd;

					if (IdentEnd > IdentStart)
					{
						const int32 CloseStart = SkipSpaces(Tmpl, IdentEnd);
						const FString CloseBraces = (BraceCount == 3) ? TEXT("}}}") : TEXT("}}");
						if (MatchesAt(Tmpl, CloseStart, CloseBraces))
						{
							Out.AddUnique(Tmpl.Mid(IdentStart, IdentEnd - IdentStart));
							Pos = CloseStart + CloseBraces.Len();
							continue;
						}
					}
				}
			}

			++Pos;
		}
		return Out;
	}
}
