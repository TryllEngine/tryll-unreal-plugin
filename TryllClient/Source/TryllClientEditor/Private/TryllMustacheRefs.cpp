// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllMustacheRefs.h"

namespace
{
	const FString kUserMessageTag = TEXT("{{user_message}}");
	const FString kSlotPrefix     = TEXT("{{slot.");
	const FString kUserMessageBody = TEXT("user_message");
	const FString kSlotBody        = TEXT("slot.");

	struct FRef
	{
		int32   Start;
		int32   End;
		FString Name;
	};

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

	/**
	 * Scan Tmpl left to right for `{{slot.<name>}}` / `{{user_message}}` tags, also
	 * matching whitespace-padded (`{{ slot.name }}`) and triple-brace
	 * (`{{{slot.name}}}`) forms — including both at once (`{{{ slot.name }}}`). A
	 * candidate match must have a valid identifier (for `slot.`) and braces of
	 * matching count on both sides to qualify; anything else (e.g. a lone
	 * `{{#section}}`) is skipped.
	 */
	TArray<FRef> Scan(const FString& Tmpl)
	{
		TArray<FRef> Refs;
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
				FString Name;
				int32 NameEnd = BodyStart;

				if (MatchesAt(Tmpl, BodyStart, kUserMessageBody))
				{
					NameEnd = BodyStart + kUserMessageBody.Len();
					Name = kUserMessageBody;
				}
				else if (MatchesAt(Tmpl, BodyStart, kSlotBody))
				{
					const int32 IdentStart = BodyStart + kSlotBody.Len();
					int32 IdentEnd = IdentStart;
					while (IdentEnd < N && IsNameChar(Tmpl[IdentEnd]))
						++IdentEnd;
					if (IdentEnd > IdentStart)
					{
						NameEnd = IdentEnd;
						Name = Tmpl.Mid(IdentStart, IdentEnd - IdentStart);
					}
				}

				if (!Name.IsEmpty())
				{
					const int32 CloseStart = SkipSpaces(Tmpl, NameEnd);
					const FString CloseBraces = (BraceCount == 3) ? TEXT("}}}") : TEXT("}}");
					if (MatchesAt(Tmpl, CloseStart, CloseBraces))
					{
						Refs.Add({Pos, CloseStart + CloseBraces.Len(), Name});
						Pos = CloseStart + CloseBraces.Len();
						continue;
					}
				}
			}

			++Pos;
		}
		return Refs;
	}
}

namespace TryllMustacheRefs
{
	TArray<FString> ExtractRefs(const FString& Tmpl)
	{
		TArray<FString> Out;
		for (const FRef& Ref : Scan(Tmpl))
			Out.AddUnique(Ref.Name);
		return Out;
	}

	FString Rename(const FString& Tmpl, const FString& From, const FString& To)
	{
		if (From.IsEmpty())
			return Tmpl;
		TMap<FString, FString> Renames;
		Renames.Add(From, To);
		return Rename(Tmpl, Renames);
	}

	FString Rename(const FString& Tmpl, const TMap<FString, FString>& Renames)
	{
		if (Renames.Num() == 0)
			return Tmpl;

		TArray<FRef> Refs = Scan(Tmpl);
		if (Refs.Num() == 0)
			return Tmpl;

		FString Out;
		Out.Reserve(Tmpl.Len());
		int32 Cursor = 0;
		bool bChanged = false;
		for (const FRef& Ref : Refs)
		{
			Out += Tmpl.Mid(Cursor, Ref.Start - Cursor);
			if (const FString* To = Renames.Find(Ref.Name))
			{
				Out += (*To == TEXT("user_message"))
					? kUserMessageTag
					: (kSlotPrefix + *To + TEXT("}}"));
				bChanged = true;
			}
			else
			{
				Out += Tmpl.Mid(Ref.Start, Ref.End - Ref.Start);
			}
			Cursor = Ref.End;
		}
		Out += Tmpl.Mid(Cursor, Tmpl.Len() - Cursor);
		return bChanged ? Out : Tmpl;
	}
}
