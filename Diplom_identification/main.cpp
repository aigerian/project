#include "includes.h"

#include "link-grammar/link-includes.h"
#include "text.h"

#include "defines.h"

Dictionary link_grammar_dict;

void choose_locale();
double score_mu0(text &sample, text &test);
double score_mu1(text &sample, text &test);

int main()
{
    choose_locale();
    link_grammar_dict = dictionary_create_lang(LANG);
    
    text query_text("../texts/snippets/query.txt", false, true);
    int snippets_num = 0;
    for (; ifstream("../texts/snippets/snippet" + to_string(snippets_num) + ".txt"); ++snippets_num);
    vector<int> indices(snippets_num);
    vector<double> scores(snippets_num);
    for (int i = 0; i < snippets_num; ++i)
    {
        indices[i] = i;
        scores[i] = score_mu0(query_text,
            text("../texts/snippets/snippet" + to_string(i) + ".txt", false, true)
        );
    }
    dictionary_delete(link_grammar_dict);
    std::sort(indices.begin(), indices.end(), [&](int i, int j) { return scores[i] > scores[j]; });
    for (int index : indices)
    {
        cout << "Snippet " << index << ": " << scores[index] << endl;
    }

    return 0;
}