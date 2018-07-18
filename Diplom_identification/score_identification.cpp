#include "includes.h"

#include "link-grammar/link-includes.h"
#include "stemmers.h"
#include "text.h"

#include "defines.h"

typedef vector<wstring> wsentence;

string concat(vector<string> list);
//wstring UTF82Unicode(string str);
wstring UTF82Unicode(const char *str);
vector<string> split_line(string &line);
//wsentence split_sentence(wstring &sentence);
wstring link_grammar_normalize_word(wstring word);

extern Dictionary link_grammar_dict;
STEM_TYPE stemmer;

bool sentence_clever_parse(Sentence &sent, Parse_Options &opts)
{
    if (sentence_parse(sent, opts) == 0)
    {
        parse_options_set_min_null_count(opts, 1);
        parse_options_set_max_null_count(opts, sentence_length(sent));
        parse_options_set_disjunct_cost(opts, 1.9);
        int parsed = sentence_parse(sent, opts);
        parse_options_set_min_null_count(opts, 0);
        parse_options_set_max_null_count(opts, 0);
        parse_options_set_disjunct_cost(opts, 2.7);
        return (parsed != 0);
    }
    return true;
}

set<tuple<wstring, wstring, string>> extract_links(Linkage &linkage)
{
    // 0 - order and LINK
    // 1 - only order
    // 2 - only LINK
    const bool order_mode = (LINKS_MODE == 0) || (LINKS_MODE == 1);
    const bool link_mode = (LINKS_MODE == 0) || (LINKS_MODE == 2);

    set<tuple<wstring, wstring, string>> links;

    if (order_mode)
    {
        int j;
        wstring prev_word;
        for (j = 0; (prev_word == L"") && (j < linkage_get_num_words(linkage)); ++j)
        {
            prev_word = link_grammar_normalize_word(UTF82Unicode(linkage_get_word(linkage, j)));
        }
        for (; j < linkage_get_num_words(linkage); ++j)
        {
            wstring next_word = link_grammar_normalize_word(UTF82Unicode(linkage_get_word(linkage, j)));
            if (next_word == L"")
            {
                continue;
            }
            links.insert(tuple<wstring, wstring, string>(next_word, prev_word, "Order"));
            prev_word = next_word;
        }
    }

    if (link_mode)
    {
        for (int j = 0; j < linkage_get_num_links(linkage); ++j)
        {
            wstring lword = link_grammar_normalize_word(UTF82Unicode(linkage_get_word(linkage, linkage_get_link_lword(linkage, j))));
            if (lword == L"")
            {
                continue;
            }
            stemmer(lword);

            wstring rword = link_grammar_normalize_word(UTF82Unicode(linkage_get_word(linkage, linkage_get_link_rword(linkage, j))));
            if (rword == L"")
            {
                continue;
            }
            stemmer(rword);

            string link_type(linkage_get_link_label(linkage, j));

            links.insert(tuple<wstring, wstring, string>(lword, rword, link_type));
        }
    }

    return links;
}

double closeness_measure(string &sentence1, string &sentence2, bool measure_mu)
{
    Parse_Options opts = parse_options_create();
    if ((LANG == "kz") || (LANG == "tr"))
    {
        parse_options_set_islands_ok(opts, true);
    }
    Sentence sent1 = sentence_create(sentence1.c_str(), link_grammar_dict);
    Sentence sent2 = sentence_create(sentence2.c_str(), link_grammar_dict);
    try
    {
        if ((sentence_clever_parse(sent1, opts) == 0) || (sentence_clever_parse(sent2, opts) == 0))
        {
            cout << "Sentence can't be parsed." << endl;
            sentence_delete(sent1);
            sentence_delete(sent2);
            return 0;
        }
    }
    catch (...)
    {
        cout << "Runtime error on sentence." << endl;
        sentence_delete(sent1);
        sentence_delete(sent2);
        return 0;
    }
    Linkage linkage1 = linkage_create(0, sent1, opts);
    Linkage linkage2 = linkage_create(0, sent2, opts);
    set<tuple<wstring, wstring, string>> links1 = extract_links(linkage1);
    set<tuple<wstring, wstring, string>> links2 = extract_links(linkage2);
    linkage_delete(linkage1);
    linkage_delete(linkage2);
    sentence_delete(sent1);
    sentence_delete(sent2);
    parse_options_delete(opts);
    set<tuple<wstring, wstring, string>> common_links;
    std::set_intersection(links1.begin(), links1.end(),
        links2.begin(), links2.end(),
        std::inserter(common_links, common_links.begin()));
    return (measure_mu == 0) ?
        (double)common_links.size() / max(links1.size(), links2.size()) :
        2. * common_links.size() / (links1.size() + links2.size());
}

double score_identification(text &sample, text &test, bool measure_mu)
{
    vector<double> scores;
    scores.reserve(sample.raw.size());
    for (string &sentence1 : sample.raw)
    {
        double max_score = 0;
        for (string &sentence2 : test.raw)
        {
            max_score = max(max_score, closeness_measure(sentence1, sentence2, measure_mu));
        }
        scores.push_back(max_score);
    }
    double total_score = 0;
    for (double score : scores)
    {
        total_score += score;
    }
    return total_score / scores.size();
}

double score_mu0(text &sample, text &test)
{
    return score_identification(sample, test, 0);
}

double score_mu1(text &sample, text &test)
{
    return score_identification(sample, test, 1);
}