#include "includes.h"

#include "stemmers.h"

#include "defines.h"

typedef vector<wstring> wsentence;

extern STEM_TYPE stemmer;

string concat(vector<string> list)
{
    string str;
    for (string &elem : list)
    {
        str += elem;
    }
    return str;
}

string Unicode2UTF8(wstring wstr)
{
    const int size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, nullptr, 0, nullptr, nullptr) - 1;
    string str;
    str.resize(size);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, &str[0], size, nullptr, nullptr);
    return str;
}

wstring UTF82Unicode(string str)
{
    const int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], -1, nullptr, 0) - 1;
    wstring wstr;
    wstr.resize(size);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], -1, &wstr[0], size);
    return wstr;
}

wstring UTF82Unicode(const char *str)
{
    return UTF82Unicode(string(str));
}

vector<string> split_line(string &line)
{
    vector<string> result;
    vector<char> end_characters = { '.', '?', '!' };
    int begin = 0;
    for (int i = 0; i <= line.size(); ++i)
    {
        if ((i == line.size()) ||
            ((line[i] == ' ') && (i > 0) && (std::find(end_characters.begin(), end_characters.end(), line[i - 1]) != end_characters.end()))
            )
        {
            string sentence = line.substr(begin, i - begin);
            if (sentence.find_first_not_of(' ') != string::npos)
            {
                result.push_back(sentence);
            }
            begin = i + 1;
        }
    }
    return result;
}

wsentence split_sentence(wstring &sentence)
{
    wsentence words;
    int begin = 0;
    for (int i = 0; i <= sentence.size(); ++i)
    {
        if ((i == sentence.size()) || (sentence[i] == ' '))
        {
            wstring word = sentence.substr(begin, i - begin);
            word.erase(0, word.find_first_not_of(L"(?\"«"));
            word.erase(word.find_last_not_of(L".,!;%:?)?\"»…") + 1);
            if (word != L"")
            {
                stemmer(word);
                std::transform(word.begin(), word.end(), word.begin(), std::towlower);
                words.push_back(word);
            }
            begin = i + 1;
        }
    }
    return words;
}

wstring link_grammar_normalize_word(wstring word)
{
    if ((word[0] == L'[') && (word[word.size() - 1] == L']'))
    {
        word = word.substr(1, word.size() - 2);
    }
    if ((!std::isalpha(word[0], locale(""))) ||
        (word.find(L"LEFT-WALL") != string::npos) ||
        (word.find(L"RIGHT-WALL") != string::npos))
    {
        return L"";
    }
    auto position = word.find_first_of(L".[");
    if (position != wstring::npos)
    {
        word = word.substr(0, position);
    }
    std::transform(word.begin(), word.end(), word.begin(), std::towlower);
    return word;
}