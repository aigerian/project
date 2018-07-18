#pragma once
#include "includes.h"

typedef vector<wstring> wsentence;
wstring UTF82Unicode(string str);
vector<string> split_line(string &line);
wsentence split_sentence(wstring &sentence);

class inverse_index
{
    map<wstring, double> inv_index;
public:
    int docs_num;

    inverse_index(ifstream &tf_idf_file)
    {
        vector<wsentence> tf_idf_text;
        string line;
        while (getline(tf_idf_file, line))
        {
            tf_idf_text.push_back(split_sentence(UTF82Unicode(line)));
        }
        docs_num = tf_idf_text.size();

        map<wstring, set<int>> inv_index_set;
        for (int doc_num = 0; doc_num < tf_idf_text.size(); ++doc_num)
        {
            for (wstring &word : tf_idf_text[doc_num])
            {
                if (!(inv_index_set.count(word)))
                {
                    inv_index_set[word] = set<int>();
                }
                inv_index_set[word].insert(doc_num);
            }
        }

        for (auto &el : inv_index_set)
        {
            inv_index[el.first] = el.second.size();
        }
    }

    double operator [] (wstring &key)
    {
        return inv_index[key];
    }
};