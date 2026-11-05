// Copyright (c) 2026 Tryll. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Tryll
// This file is part of Tryll Engine and is licensed under the Tryll Engine License; see LICENSE.md shipped with this plugin. By using this file, you accept that License.

#include "TryllJsonWriter.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace
{
	bool IsSepUtf8(std::string_view Text)
	{
		if (Text.empty()) return false;
		for (size_t i = 0; i < Text.size();)
		{
			const auto Ch = static_cast<unsigned char>(Text[i]);
			if (Ch == ' ' || Ch == '\t' || Ch == '|' || Ch == '-' || Ch == ':' || Ch == '/')
			{
				++i;
				continue;
			}
			if (Ch == 0xC2 && i + 1 < Text.size() && static_cast<unsigned char>(Text[i + 1]) == 0xB7)
			{
				i += 2; // · U+00B7
				continue;
			}
			if (Ch == 0xE2 && i + 2 < Text.size()
				&& static_cast<unsigned char>(Text[i + 1]) == 0x80
				&& static_cast<unsigned char>(Text[i + 2]) == 0xA2)
			{
				i += 3; // • no, U+00B7 is C2 B7. U+00B7 middle dot. The spec uses " · " U+00B7.
				continue;
			}
			return false;
		}
		return true;
	}

	void AppendUtf8(std::string& Out, char32_t Cp)
	{
		if (Cp < 0x80) Out.push_back(static_cast<char>(Cp));
		else if (Cp < 0x800)
		{
			Out.push_back(static_cast<char>(0xC0 | (Cp >> 6)));
			Out.push_back(static_cast<char>(0x80 | (Cp & 0x3F)));
		}
		else if (Cp < 0x10000)
		{
			Out.push_back(static_cast<char>(0xE0 | (Cp >> 12)));
			Out.push_back(static_cast<char>(0x80 | ((Cp >> 6) & 0x3F)));
			Out.push_back(static_cast<char>(0x80 | (Cp & 0x3F)));
		}
		else
		{
			Out.push_back(static_cast<char>(0xF0 | (Cp >> 18)));
			Out.push_back(static_cast<char>(0x80 | ((Cp >> 12) & 0x3F)));
			Out.push_back(static_cast<char>(0x80 | ((Cp >> 6) & 0x3F)));
			Out.push_back(static_cast<char>(0x80 | (Cp & 0x3F)));
		}
	}

	bool DecodeUtf8(std::string_view In, size_t& Index, char32_t& Out)
	{
		if (Index >= In.size()) return false;
		const auto C0 = static_cast<unsigned char>(In[Index]);
		if (C0 < 0x80) { Out = C0; Index += 1; return true; }
		int Need = 0;
		char32_t Cp = 0;
		if ((C0 & 0xE0) == 0xC0) { Need = 1; Cp = C0 & 0x1F; }
		else if ((C0 & 0xF0) == 0xE0) { Need = 2; Cp = C0 & 0x0F; }
		else if ((C0 & 0xF8) == 0xF0) { Need = 3; Cp = C0 & 0x07; }
		else { Index += 1; Out = 0xFFFD; return true; }
		if (Index + static_cast<size_t>(Need) >= In.size()) { Index = In.size(); Out = 0xFFFD; return true; }
		for (int i = 1; i <= Need; ++i)
		{
			const auto Cx = static_cast<unsigned char>(In[Index + static_cast<size_t>(i)]);
			if ((Cx & 0xC0) != 0x80) { Index += 1; Out = 0xFFFD; return true; }
			Cp = (Cp << 6) | (Cx & 0x3F);
		}
		Index += static_cast<size_t>(Need) + 1;
		Out = Cp;
		return true;
	}

	void SkipWs(std::string_view S, size_t& I)
	{
		while (I < S.size() && (S[I] == ' ' || S[I] == '\n' || S[I] == '\r' || S[I] == '\t')) ++I;
	}

	struct FTryllJsonParse
	{
		std::string_view S;
		size_t I = 0;
		const char* Err = nullptr;

		bool Fail(const char* Reason)
		{
			Err = Reason;
			return false;
		}

		void SkipWs()
		{
			while (I < S.size() && (S[I] == ' ' || S[I] == '\n' || S[I] == '\r' || S[I] == '\t')) ++I;
		}

		bool ParseValue();
		bool ParseString();
		bool ParseLiteral();
		bool ParseNumber();
		bool ParseObject();
		bool ParseArray();
	};

	bool FTryllJsonParse::ParseString()
	{
		if (I >= S.size() || S[I] != '"') return Fail("object key");
		++I;
		while (I < S.size())
		{
			const char C = S[I++];
			if (C == '"') return true;
			if (C == '\\')
			{
				if (I >= S.size()) return Fail("bad escape");
				const char E = S[I++];
				if (E == 'u')
				{
					if (I + 4 > S.size()) return Fail("bad escape");
					I += 4;
				}
				else if (E != '"' && E != '\\' && E != '/' && E != 'b' && E != 'f' && E != 'n' && E != 'r' && E != 't')
					return Fail("bad escape");
			}
			else if (static_cast<unsigned char>(C) < 0x20)
				return Fail("control character");
		}
		return Fail("unterminated string");
	}

	bool FTryllJsonParse::ParseLiteral()
	{
		if (S.compare(I, 4, "true") == 0) { I += 4; return true; }
		if (S.compare(I, 5, "false") == 0) { I += 5; return true; }
		if (S.compare(I, 4, "null") == 0) { I += 4; return true; }
		return Fail("invalid literal");
	}

	bool FTryllJsonParse::ParseNumber()
	{
		const size_t Start = I;
		if (I < S.size() && S[I] == '-') ++I;
		if (I >= S.size() || S[I] < '0' || S[I] > '9') return Fail("invalid number");
		if (S[I] == '0') ++I;
		else while (I < S.size() && S[I] >= '0' && S[I] <= '9') ++I;
		if (I < S.size() && S[I] == '.')
		{
			++I;
			if (I >= S.size() || S[I] < '0' || S[I] > '9') return Fail("invalid number");
			while (I < S.size() && S[I] >= '0' && S[I] <= '9') ++I;
		}
		if (I < S.size() && (S[I] == 'e' || S[I] == 'E'))
		{
			++I;
			if (I < S.size() && (S[I] == '+' || S[I] == '-')) ++I;
			if (I >= S.size() || S[I] < '0' || S[I] > '9') return Fail("invalid number");
			while (I < S.size() && S[I] >= '0' && S[I] <= '9') ++I;
		}
		if (I == Start) return Fail("invalid number");
		return true;
	}

	bool FTryllJsonParse::ParseObject()
	{
		++I;
		SkipWs();
		if (I < S.size() && S[I] == '}') { ++I; return true; }
		while (true)
		{
			if (!ParseString()) return false;
			SkipWs();
			if (I >= S.size() || S[I] != ':') return Fail("object colon");
			++I;
			SkipWs();
			if (!ParseValue()) return false;
			SkipWs();
			if (I < S.size() && S[I] == ',') { ++I; SkipWs(); continue; }
			if (I < S.size() && S[I] == '}') { ++I; return true; }
			return Fail("object end");
		}
	}

	bool FTryllJsonParse::ParseArray()
	{
		++I;
		SkipWs();
		if (I < S.size() && S[I] == ']') { ++I; return true; }
		while (true)
		{
			if (!ParseValue()) return false;
			SkipWs();
			if (I < S.size() && S[I] == ',') { ++I; SkipWs(); continue; }
			if (I < S.size() && S[I] == ']') { ++I; return true; }
			return Fail("array end");
		}
	}

	bool FTryllJsonParse::ParseValue()
	{
		if (I >= S.size()) return Fail("truncated");
		const char C = S[I];
		if (C == '"') return ParseString();
		if (C == '{') return ParseObject();
		if (C == '[') return ParseArray();
		if (C == 't' || C == 'f' || C == 'n') return ParseLiteral();
		if (C == '-' || (C >= '0' && C <= '9')) return ParseNumber();
		return Fail("invalid value");
	}

}

FTryllJsonWriter::FTryllJsonWriter(bool bIndentIn)
	: bIndent(bIndentIn)
{
}

void FTryllJsonWriter::BeginObject() { Begin('{'); }
void FTryllJsonWriter::EndObject() { End('}'); }
void FTryllJsonWriter::BeginArray() { Begin('['); }
void FTryllJsonWriter::EndArray() { End(']'); }

void FTryllJsonWriter::Key(std::string_view Name)
{
	Lead(false);
	WriteString(Name);
	Text.push_back(':');
	if (bIndent) Text.push_back(' ');
}

void FTryllJsonWriter::Null()
{
	Lead(true);
	Text.append("null");
}

void FTryllJsonWriter::Bool(bool bValue)
{
	Lead(true);
	Text.append(bValue ? "true" : "false");
}

void FTryllJsonWriter::Int64(std::int64_t Value)
{
	char Buf[32];
	std::snprintf(Buf, sizeof(Buf), "%lld", static_cast<long long>(Value));
	RawNumber(Buf);
}

void FTryllJsonWriter::UInt64(std::uint64_t Value)
{
	char Buf[32];
	std::snprintf(Buf, sizeof(Buf), "%llu", static_cast<unsigned long long>(Value));
	RawNumber(Buf);
}

void FTryllJsonWriter::Float(float Value)
{
	Double(static_cast<double>(Value));
}

void FTryllJsonWriter::Double(double Value)
{
	if (std::isnan(Value)) { String("NaN"); return; }
	if (std::isinf(Value)) { String(Value > 0 ? "Infinity" : "-Infinity"); return; }
	if (Value == 0.0)
	{
		std::uint64_t Bits = 0;
		std::memcpy(&Bits, &Value, sizeof(Bits));
		if ((Bits >> 63) != 0) { RawNumber("-0.0"); return; }
	}
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.17g", Value);
	RawNumber(Buf);
}

void FTryllJsonWriter::String(std::string_view Value)
{
	Lead(true);
	WriteString(Value);
}

void FTryllJsonWriter::Raw(std::string_view Json)
{
	Lead(true);
	Text.append(Json);
}

bool FTryllJsonWriter::TryParseJson(std::string_view InText, std::string& OutReason)
{
	OutReason.clear();
	if (InText.empty()) { OutReason = "empty"; return false; }
	FTryllJsonParse Parse{ InText, 0, nullptr };
	Parse.SkipWs();
	if (!Parse.ParseValue()) { OutReason = Parse.Err ? Parse.Err : "invalid"; return false; }
	Parse.SkipWs();
	if (Parse.I != InText.size()) { OutReason = "trailing data"; return false; }
	return true;
}


void FTryllJsonWriter::Begin(char Open)
{
	Lead(true);
	Text.push_back(Open);
	Frames.push_back(FFrame{ Open == '[', 0 });
}

void FTryllJsonWriter::End(char Close)
{
	const bool bEmpty = Frames.empty() || Frames.back().Count == 0;
	if (!Frames.empty()) Frames.pop_back();
	if (bIndent && !bEmpty)
	{
		Text.push_back('\n');
		Text.append(static_cast<size_t>(Depth()) * 2, ' ');
	}
	Text.push_back(Close);
}

void FTryllJsonWriter::Lead(bool bValue)
{
	if (Frames.empty()) return;
	FFrame& Frame = Frames.back();
	if (bValue && !Frame.bArray) return;
	if (Frame.Count++ > 0)
	{
		Text.push_back(',');
		if (bIndent)
		{
			Text.push_back('\n');
			Text.append(static_cast<size_t>(Depth()) * 2, ' ');
		}
	}
	else if (bIndent)
	{
		Text.push_back('\n');
		Text.append(static_cast<size_t>(Depth()) * 2, ' ');
	}
}

void FTryllJsonWriter::RawNumber(std::string_view Number)
{
	Lead(true);
	Text.append(Number);
}

void FTryllJsonWriter::WriteString(std::string_view Value)
{
	Text.push_back('"');
	size_t Index = 0;
	while (Index < Value.size())
	{
		char32_t Cp = 0;
		DecodeUtf8(Value, Index, Cp);
		switch (Cp)
		{
		case '"': Text.append("\\\""); break;
		case '\\': Text.append("\\\\"); break;
		case '\n': Text.append("\\n"); break;
		case '\r': Text.append("\\r"); break;
		case '\t': Text.append("\\t"); break;
		default:
			if (Cp < 0x20)
			{
				char Buf[8];
				std::snprintf(Buf, sizeof(Buf), "\\u%04x", static_cast<unsigned>(Cp));
				Text.append(Buf);
			}
			else AppendUtf8(Text, Cp);
			break;
		}
	}
	Text.push_back('"');
}

std::string TryllFormatLogLabel(
	std::string_view Template,
	const std::vector<std::pair<std::string, std::string>>& Refs)
{
	struct Part { bool bVal; std::string Text; };
	std::vector<Part> Parts;
	size_t Cursor = 0;
	while (Cursor < Template.size())
	{
		const auto Open = Template.find('{', Cursor);
		if (Open == std::string_view::npos)
		{
			Parts.push_back({ false, std::string(Template.substr(Cursor)) });
			break;
		}
		if (Open > Cursor) Parts.push_back({ false, std::string(Template.substr(Cursor, Open - Cursor)) });
		const auto Close = Template.find('}', Open + 1);
		if (Close == std::string_view::npos)
		{
			Parts.push_back({ false, std::string(Template.substr(Open)) });
			break;
		}
		const auto Key = Template.substr(Open + 1, Close - Open - 1);
		std::string Found;
		for (const auto& Ref : Refs)
			if (Ref.first == Key) Found = Ref.second;
		Parts.push_back({ true, std::move(Found) });
		Cursor = Close + 1;
	}
	auto Content = [&](int Index, int Step) -> bool
	{
		for (int i = Index + Step; i >= 0 && i < static_cast<int>(Parts.size()); i += Step)
		{
			if (Parts[static_cast<size_t>(i)].bVal) return !Parts[static_cast<size_t>(i)].Text.empty();
			if (!IsSepUtf8(Parts[static_cast<size_t>(i)].Text)) return !Parts[static_cast<size_t>(i)].Text.empty();
		}
		return false;
	};
	std::string Out;
	for (int i = 0; i < static_cast<int>(Parts.size()); ++i)
	{
		const auto& Part = Parts[static_cast<size_t>(i)];
		if (Part.bVal)
		{
			if (!Part.Text.empty()) Out.append(Part.Text);
			continue;
		}
		if (IsSepUtf8(Part.Text))
		{
			if (Content(i, -1) && Content(i, 1)) Out.append(Part.Text);
			continue;
		}
		Out.append(Part.Text);
	}
	return Out;
}
