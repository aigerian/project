#pragma once
#include <string>
#include <Windows.h>
#include "snowball-turkish.h"

using std::string;
using std::wstring;

class turkish_stem
{
public:
	void operator () (wstring &word_)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, &word_[0], -1, nullptr, 0, nullptr, nullptr);
		char *word_char = new char[size];
		WideCharToMultiByte(CP_UTF8, 0, &word_[0], -1, word_char, size, nullptr, nullptr);
		sb_symbol *word_sb_symbol = new sb_symbol[size];
		strncpy((char *)word_sb_symbol, word_char, size);
		delete[] word_char;
		sb_stemmer *stem = sb_stemmer_new("turkish", NULL);
		const sb_symbol *word_stemmed = sb_stemmer_stem(stem, word_sb_symbol, size - 1);
		size = strlen((char *)word_stemmed) + 1;
		word_char = new char[size];
		strncpy(word_char, (char *)word_stemmed, size);
		sb_stemmer_delete(stem);
		size = MultiByteToWideChar(CP_UTF8, 0, &word_char[0], strlen(word_char), nullptr, 0);
		wstring wstr;
		wstr.resize(size);
		MultiByteToWideChar(CP_UTF8, 0, &word_char[0], strlen(word_char), &wstr[0], size);
		delete[] word_char;
		word_ = wstr;
	}
};