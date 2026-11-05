// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllLanguages.h"

namespace
{

/// Mirrors kLanguageNames in tryll/server/workflow/src/SessionLocale.cpp, so a
/// menu label and the server's {{language}} prompt key always read the same.
const TMap<FString, FString>& LanguageNameTable()
{
	static const TMap<FString, FString> Table = {
		{ TEXT("ar"), TEXT("Arabic")     }, { TEXT("bg"), TEXT("Bulgarian")  },
		{ TEXT("bn"), TEXT("Bengali")    }, { TEXT("ca"), TEXT("Catalan")    },
		{ TEXT("cs"), TEXT("Czech")      }, { TEXT("da"), TEXT("Danish")     },
		{ TEXT("de"), TEXT("German")     }, { TEXT("el"), TEXT("Greek")      },
		{ TEXT("en"), TEXT("English")    }, { TEXT("es"), TEXT("Spanish")    },
		{ TEXT("et"), TEXT("Estonian")   }, { TEXT("fa"), TEXT("Persian")    },
		{ TEXT("fi"), TEXT("Finnish")    }, { TEXT("fr"), TEXT("French")     },
		{ TEXT("he"), TEXT("Hebrew")     }, { TEXT("hi"), TEXT("Hindi")      },
		{ TEXT("hr"), TEXT("Croatian")   }, { TEXT("hu"), TEXT("Hungarian")  },
		{ TEXT("id"), TEXT("Indonesian") }, { TEXT("it"), TEXT("Italian")    },
		{ TEXT("ja"), TEXT("Japanese")   }, { TEXT("ko"), TEXT("Korean")     },
		{ TEXT("lt"), TEXT("Lithuanian") }, { TEXT("lv"), TEXT("Latvian")    },
		{ TEXT("ms"), TEXT("Malay")      }, { TEXT("nl"), TEXT("Dutch")      },
		{ TEXT("no"), TEXT("Norwegian")  }, { TEXT("pl"), TEXT("Polish")     },
		{ TEXT("pt"), TEXT("Portuguese") }, { TEXT("ro"), TEXT("Romanian")   },
		{ TEXT("ru"), TEXT("Russian")    }, { TEXT("sk"), TEXT("Slovak")     },
		{ TEXT("sl"), TEXT("Slovenian")  }, { TEXT("sr"), TEXT("Serbian")    },
		{ TEXT("sv"), TEXT("Swedish")    }, { TEXT("ta"), TEXT("Tamil")      },
		{ TEXT("th"), TEXT("Thai")       }, { TEXT("tr"), TEXT("Turkish")    },
		{ TEXT("uk"), TEXT("Ukrainian")  }, { TEXT("ur"), TEXT("Urdu")       },
		{ TEXT("vi"), TEXT("Vietnamese") }, { TEXT("yue"), TEXT("Cantonese") },
		{ TEXT("zh"), TEXT("Chinese")    }, { TEXT("zu"), TEXT("Zulu")       },
	};
	return Table;
}

} // namespace

TArray<FString> UTryllLanguages::OfferableLanguages(const TArray<FTryllModelInfo>& Models)
{
	TSet<FString> Intersection;
	bool bSeenAnyDeclaration = false;

	for (const FTryllModelInfo& Model : Models)
	{
		if (Model.Languages.Num() == 0)
			continue;   // undeclared = no information, not "supports nothing"

		TSet<FString> Declared;
		for (const FString& Code : Model.Languages)
		{
			if (!Code.IsEmpty())
				Declared.Add(Code.ToLower());
		}

		if (!bSeenAnyDeclaration)
		{
			Intersection = MoveTemp(Declared);
			bSeenAnyDeclaration = true;
		}
		else
		{
			Intersection = Intersection.Intersect(Declared);
		}
	}

	TArray<FString> Result = Intersection.Array();
	Result.Sort();
	return Result;
}

TArray<FString> UTryllLanguages::OfferableLanguagesForModels(
	const TArray<FTryllModelInfo>& AllModels,
	const TArray<FString>&         ModelNames)
{
	if (ModelNames.Num() == 0)
		return {};

	TArray<FTryllModelInfo> Picked;
	for (const FTryllModelInfo& Model : AllModels)
	{
		if (ModelNames.Contains(Model.Name))
			Picked.Add(Model);
	}
	return OfferableLanguages(Picked);
}

bool UTryllLanguages::HasLanguageInfo(const TArray<FTryllModelInfo>& Models)
{
	for (const FTryllModelInfo& Model : Models)
	{
		if (Model.Languages.Num() > 0)
			return true;
	}
	return false;
}

bool UTryllLanguages::ModelSupportsLanguage(const FTryllModelInfo& Model, const FString& Locale)
{
	if (Model.Languages.Num() == 0)
		return true;

	const FString Code = LanguageOf(Locale);
	if (Code.IsEmpty())
		return false;

	for (const FString& Declared : Model.Languages)
	{
		if (Declared.Equals(Code, ESearchCase::IgnoreCase))
			return true;
	}
	return false;
}

FString UTryllLanguages::LanguageOf(const FString& Locale)
{
	if (Locale.IsEmpty())
		return FString{};

	int32 Cut = Locale.Len();
	for (int32 i = 0; i < Locale.Len(); ++i)
	{
		if (Locale[i] == TCHAR('-') || Locale[i] == TCHAR('_'))
		{
			Cut = i;
			break;
		}
	}
	return Locale.Left(Cut).ToLower();
}

FString UTryllLanguages::LanguageDisplayName(const FString& Code)
{
	const FString Key = LanguageOf(Code);
	if (const FString* Found = LanguageNameTable().Find(Key))
		return *Found;
	return Code;
}
