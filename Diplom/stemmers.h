#pragma once
#import "Lemmatizer.tlb"
#include "stemming/english_stem.h"
#include "kazakh_stem.h"
#include "turkish_stem.h"

template <typename LemmatizerType> class dialing_stem
{
	LEMMATIZERLib::ILemmatizerPtr piLemmatizer;
public:
	dialing_stem()
	{
		::CoInitialize(NULL);
		piLemmatizer.CreateInstance(__uuidof(LemmatizerType));
		piLemmatizer->LoadDictionariesRegistry();
	}
	void operator () (wstring &word)
	{
		LEMMATIZERLib::IParadigmCollectionPtr piParadigmCollection = piLemmatizer->CreateParadigmCollectionFromForm(word.c_str(), FALSE, FALSE);
		if (piParadigmCollection->Count > 0)
		{
			word = _bstr_t(piParadigmCollection->Item[0]->Norm);
		}
		std::transform(word.begin(), word.end(), word.begin(), std::towlower);
		piParadigmCollection = 0;
	}
	~dialing_stem()
	{
		piLemmatizer = 0;
		::CoUninitialize();
	}
};

typedef dialing_stem<LEMMATIZERLib::LemmatizerRussian> russian_stem;
typedef stemming::english_stem<> english_stem;

class null_stem
{
public:
	void operator () (wstring &word_)
	{
	}
};