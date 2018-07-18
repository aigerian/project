#pragma once
#include "includes.h"

#include "inverse_index.h"

typedef vector<wstring> wsentence;

vector<string> split_line(string &line);
wsentence split_sentence(wstring &sentence);
wstring UTF82Unicode(string str);

class text
{
public:
    string filename;
    vector<wsentence> normalized;
    vector<string> raw;

    text(string filename_, bool write_normalized = true, bool write_raw = true)
    {
        filename = filename_;
        ifstream file(filename_);
        if (!file.good())
        {
            cerr << "Could not read file " << filename << endl;
            exit(1);
        }
        string line;
        while (getline(file, line))
        {
            vector<string> sentences = split_line(line);
            for (string &sentence : sentences)
            {
                if (write_normalized)
                {
                    normalized.push_back(split_sentence(UTF82Unicode(sentence)));
                }
                if (write_raw)
                {
                    raw.push_back(sentence);
                }
            }
        }
    }

private:
    map<wstring, double> get_tf_idf(inverse_index &inv_index)
    {
        map<wstring, int> number_of_occurence;
        int words_total = 0;
        for (wsentence &sentence : normalized)
        {
            for (wstring &word : sentence)
            {
                if (!(number_of_occurence.count(word)))
                {
                    number_of_occurence[word] = 0;
                }
                ++number_of_occurence[word];
                ++words_total;
            }
        }

        map<wstring, double> tf_idf;
        for (wsentence &sentence : normalized)
        {
            for (wstring &word : sentence)
            {
                if (tf_idf.count(word))
                {
                    continue;
                }
                const double tf = (double)number_of_occurence[word] / words_total;
                const double idf = std::log((double)(inv_index.docs_num + 1) / (inv_index[word] + 1));
                tf_idf[word] = tf * idf;
            }
        }

        return tf_idf;
    }

public:
    void tf_idf_filter(inverse_index &inv_index, double threshold)
    {
        map<wstring, double> tf_idf = get_tf_idf(inv_index);

        /*for (auto it1 = normalized.begin(); it1 != normalized.end();)
        {
            for (auto it2 = it1->begin(); it2 != it1->end();)
            {
                if (tf_idf[*it2] < threshold)
                {
                    it2 = it1->erase(it2);
                }
                else
                {
                    ++it2;
                }
            }
            if (it1->size() == 0)
            {
                it1 = normalized.erase(it1);
            }
            else
            {
                ++it1;
            }
            return;
        }*/

        vector<wsentence> normalized_new(normalized.size());

        for (int i = 0; i < normalized.size(); ++i)
        {
            for (wstring &word : normalized[i])
            {
                if (tf_idf[word] > threshold)
                {
                    normalized_new[i].push_back(word);
                }
            }
        }

        normalized = normalized_new;
    }

    text(string filename_, inverse_index &inv_index, double threshold, bool write_raw = true)
    {
        new(this) text(filename_, true, write_raw);
        tf_idf_filter(inv_index, threshold);
    }
};