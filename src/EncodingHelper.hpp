#pragma once

#include <string>
#include <vector>
#include <locale>
#include <codecvt>

static char accentFilters[65536]; //look up table
static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

//IT: artigianale...
//EN: homemade
class EncodingHelper
{
public:
	static void Init()
	{
		memset(accentFilters, 0, 65536 * sizeof(char));

		//remove all type of accents
		AddFilter(L'\xe0', 6, 'a');
		AddFilter(L'\xe8', 4, 'e');
		AddFilter(L'\xec', 4, 'i');
		AddFilter(L'\xf2', 5, 'o');
		AddFilter(L'\xf9', 4, 'u');

		AddFilter(L'\xc0', 6, 'A');
		AddFilter(L'\xc8', 4, 'E');
		AddFilter(L'\xcc', 4, 'I');
		AddFilter(L'\xd2', 5, 'O');
		AddFilter(L'\xd9', 4, 'U');
	}

	static std::wstring Convert(std::string input)
	{
		return converter.from_bytes(input);
	}

	static std::string Convert(std::wstring input)
	{
		return converter.to_bytes(input);
	}

	static std::string FilterAccents(std::string toFilter)
	{
		if (toFilter.length() <= 0)
			return "";

		std::wstring input = Convert(toFilter);

		int length = input.length();
		int curLength = length;

		wchar_t* text = new wchar_t[length * 2 + 1];

		if (text)
		{
			memset(text, 0, (length * 2 + 1) * sizeof(wchar_t));
			memcpy(text, input.c_str(), length * sizeof(wchar_t));

			for (int i = 0; i < length; i++)
			{
				if (FilterChar(text + i, curLength - i - 1))
					curLength++;
			}

			std::wstring out(text, curLength);

			delete[] text;

			return converter.to_bytes(out);
		}

		return toFilter;
	}

private:
	static inline void AddFilter(wchar_t start, wchar_t count, char replace)
	{
		for (wchar_t i = start; i < start + count; i++)
			accentFilters[i] = replace;
	}

	static inline bool FilterChar(wchar_t* ptr, int endCount)
	{
		wchar_t newChar = *(accentFilters + *ptr);

		if (newChar)
		{
			*ptr = newChar;
			memcpy(ptr + 2, ptr + 1, endCount * sizeof(wchar_t));
			*(ptr + 1) = L'\'';

			return true;
		}
		return false;
	}
};