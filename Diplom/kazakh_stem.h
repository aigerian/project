#pragma once
#include <map>
#include <string>
#include <vector>

using std::map;
using std::wstring;
using std::vector;

class kazakh_stem
{
	wstring word;
	vector<wstring> P1;
	vector<wstring> P2;
	vector<wstring> P3;
	vector<wstring> P4;
	vector<wstring> suffixes;
	vector<wstring> exceptions;
	map<wchar_t, wchar_t> synharmonisms;

public:
	kazakh_stem()
	{
        locale::global(locale("kazakh_Kazakhstan.1251"));
		P1 = { L"лар", L"лер", L"дар", L"дер", L"тар", L"тер", L"лары", L"лері", L"дары", L"дері", L"тары", L"тері" };
		P2 = { L"ңыз", L"ңіз", L"мыз", L"міз", L"і", L"сы", L"сі", L"ы", L"м" };
		P3 = { L"ңыз", L"ңiз", L"мын", L"мін", L"бын", L"бін", L"пын", L"пін", L"сың", L"сің", L"сыз", L"сіз", L"мыз", L"міз", L"быз", L"біз", L"пыз", L"піз", L"ды", L"дi", L"м", L"ты", L"тi", L"ң", L"қ", L"к" };
		P4 = { L"ның", L"нің", L"дың", L"дің", L"тың", L"тің", L"нан", L"нен", L"тан", L"тен", L"дан", L"ден", L"ін", L"ын", L"мен", L"бен", L"пен", L"ға", L"ге", L"қа", L"ке", L"на", L"не", L"ны", L"ні", L"да", L"де", L"та", L"те", L"ді", L"ды", L"ты", L"ті" };
		suffixes = { L"ылған", L"атын", L"етін", L"ілген", L"итын", L"итін", L"ылмаған", L"ілмеген", L"ған", L"ген" };
		exceptions = { L"қатер", L"сымсыз", L"жеке", L"фиктивті", L"автоматты", L"компьютер", L"қайта", L"уақыты", L"пәрмен", L"қате", L"үлгі", L"қатар", L"коды", L"тест", L"пайда", L"байт", L"қалқа", L"қатты", L"пішін", L"қауіпсіз", L"кілт", L"мониторинк", L"орта", L"қызмет", L"атрибут", L"жұмыс", L"аудит", L"құрылғы", L"сәулет", L"қосынды", L"атын", L"стандарт", L"топ", L"сүнгі", L"константа", L"қысқа", L"хат", L"диалог", L"литер", L"макропәрмен", L"жады", L"такт", L"бет", L"микроассемблер", L"ассемблер", L"шарт", L"негіз", L"негізгі", L"нұсқа", L"вандер", L"сан", L"әдіс", L"кластер", L"буын", L"арна", L"мән", L"қолданбалы", L"машина", L"желі", L"карта", L"ағын", L"шартсыз", L"өріс", L"дана", L"үдеріс", L"спулер", L"саясат", L"қаржы", L"неліктен", L"жайлы" };
        exceptions.resize(2 * exceptions.size());
        for (auto it1 = exceptions.begin(), it2 = it1 + exceptions.size() / 2; it2 != exceptions.end(); ++it1, ++it2)
        {
            wstring word = *it1;
            word[0] = std::towupper(word[0]);
            *it2 = word;
        }
		synharmonisms[L'ғ'] = L'қ';
		synharmonisms[L'г'] = L'к';
		synharmonisms[L'б'] = L'п';
	}

private:
	bool find_part(vector<wstring> &parts)
	{
		if (std::find(exceptions.begin(), exceptions.end(), word) != exceptions.end())
		{
			return false;
		}
        for (wstring &part : parts)
		{
			const int position = word.rfind(part);
			if (position != wstring::npos)
			{
				if (position == word.size() - part.size())
				{
					word = word.substr(0, position);
					return true;
				}
			}
		}
		return false;
	}

	void process_suffixes()
	{
		bool find_suffixes = find_part(suffixes);
		if (find_suffixes)
		{
			word += L"у";
		}
	}

	void process_synharmonisms()
	{
        for (auto &el : synharmonisms)
		{
			const int position = word.rfind(el.first);
			if (position != wstring::npos)
			{
				if (position == word.size() - 1)
				{
					word = word.substr(0, position);
					word += el.second;
					return;
				}
			}
		}
	}

	void analyze_after_cases()
	{
		find_part(P2);
		find_part(P1);
		process_suffixes();
		process_synharmonisms();
	}

	void analyze_after_pronouns()
	{
		find_part(P4);
		find_part(P2);
		find_part(P1);
		process_suffixes();
		process_synharmonisms();
	}

	wstring analyze()
	{
		bool find_cases = find_part(P4);
		if (find_cases)
		{
			analyze_after_cases();
			return word;
		}

		bool find_pronouns = find_part(P3);
		if (find_pronouns)
		{
			analyze_after_pronouns();
			return word;
		}

		find_part(P2);
		find_part(P1);
		process_suffixes();
		process_synharmonisms();

		return word;
	}

public:

	void operator () (wstring &word_)
	{
		word = word_;
		word_ = analyze();
	}
};