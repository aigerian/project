#include "includes.h"

#include "graphs.h"
#include "Eigen/Core"
#include "Eigen/Dense"
#include "link-grammar/link-includes.h"
#include "stemmers.h"
#include "text.h"

#include "defines.h"

typedef vector<wstring> wsentence;

wstring UTF82Unicode(const char *str);
wstring link_grammar_normalize_word(wstring word);
int SQL_init_file(string &filename);
void SQL_delete_previous_data(int id);
void SQL_write_words(int id, map<wstring, double> &weights);
void SQL_write_links(int id, oriented_graph<map<string, int>> &graph);
void SQL_write_edge_weights(int id, unoriented_graph<double> &graph);
void export_graph(unoriented_graph<double> &graph, wofstream &file);

extern Dictionary link_grammar_dict;
extern bool order_mode;
extern bool link_grammar_mode;
extern double sample_weights_threshold;
extern double test_weights_threshold;
extern int n0;
extern double lambda;
extern double epsilon;
STEM_TYPE stemmer;
const double INFTY = 1e+10;

oriented_graph<map<string, int>> construct_oriented_graph1(text &text)
{
	oriented_graph<map<string, int>> oriented_graph1;
	vector<tuple<wstring, wstring, string>> links;

	if (order_mode)
	{
        for (wsentence &sentence : text.normalized)
		{
			for (int j = 1; j < sentence.size(); ++j)
			{
                oriented_graph1.add_word(sentence[j - 1]);
                oriented_graph1.add_word(sentence[j]);
				links.push_back(tuple<wstring, wstring, string>(sentence[j - 1], sentence[j], "Order"));
			}
		}
	}

	if (link_grammar_mode)
	{
		Parse_Options opts = parse_options_create();
		if ((LANG == "kz") || (LANG == "tr"))
		{
			parse_options_set_islands_ok(opts, true);
		}
		for (int i = 0; i < text.raw.size(); ++i)
		{
			if (text.raw[i] == "")
			{
				continue;
			}
			Sentence sent = sentence_create(text.raw[i].c_str(), link_grammar_dict);
			try
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
					if (parsed == 0)
					{
						cout << "Sentence #" << i + 1 << " can't be parsed." << endl;
						sentence_delete(sent);
						continue;
					}
				}
			}
			catch (...)
			{
				cout << "Runtime error on sentence #" << i + 1 << "." << endl;
                sentence_delete(sent);
				continue;
			}
			Linkage linkage = linkage_create(0, sent, opts);
			for (int j = 0; j < linkage_get_num_words(linkage); ++j)
			{
				wstring word = link_grammar_normalize_word(UTF82Unicode(linkage_get_word(linkage, j)));
				if (word == L"")
				{
					continue;
				}
				stemmer(word);
                oriented_graph1.add_word(word);
			}

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

				links.push_back(tuple<wstring, wstring, string>(lword, rword, link_type));
			}

			linkage_delete(linkage);
			sentence_delete(sent);
		}
		parse_options_delete(opts);
	}

    oriented_graph1.assign_matrix();
    for (auto &link : links)
	{
		const wstring lword = get<0>(link);
		const wstring rword = get<1>(link);
		const string link_type = get<2>(link);
		auto &mp = oriented_graph1(lword, rword);
		if (!mp.count(link_type))
		{
			mp[link_type] = 0;
		}
		++mp[link_type];
	}
	return oriented_graph1;
}

oriented_graph<double> construct_oriented_graph2(oriented_graph<map<string, int>> &oriented_graph1, map<string, int> &W)
{
    oriented_graph<double> oriented_graph2;
    for (auto &el : oriented_graph1)
    {
        oriented_graph2.add_word(el.first);
    }
    oriented_graph2.assign_matrix();
    for (int i = 0; i < oriented_graph1.size(); ++i)
    {
        for (int j = 0; j < oriented_graph1.size(); ++j)
        {
            for (auto itm = oriented_graph1(i, j).begin(); itm != oriented_graph1(i, j).end(); ++itm)
            {
                if (!W.count(itm->first))
                {
                    W[itm->first] = 1;
                }
                oriented_graph2(i, j) += itm->second * W[itm->first];
            }
        }
    }
    return oriented_graph2;
}

unoriented_graph<double> construct_unoriented_graph(oriented_graph<double> oriented_graph)
{
	unoriented_graph<double> unoriented_graph;
    for (auto &el : oriented_graph)
    {
        unoriented_graph.add_word(el.first);
    }
    unoriented_graph.assign_matrix();
	double path_length_sum = 0;
	for (int i = 0; i < unoriented_graph.size(); ++i)
	{
		for (int j = 0; j < i; ++j)
		{
			const double link_length = oriented_graph(i, j) + oriented_graph(j, i);
			if (link_length == 0)
			{
				continue;
			}
			const double path_length = 1 / link_length;
			path_length_sum += path_length;
            unoriented_graph(i, j) = path_length;
		}
	}
	for (int i = 0; i < unoriented_graph.size(); ++i)
	{
		for (int j = 0; j < i; ++j)
		{
            unoriented_graph(i, j) /= path_length_sum;
		}
	}

	return unoriented_graph;
}

map<wstring, double> compute_weights(oriented_graph<double> &graph)
{
    map<wstring, double> weights;
    vector<int> out(graph.size());
    for (int i = 0; i < graph.size(); ++i)
    {
        for (int j = 0; j < graph.size(); ++j)
        {
            if (graph(i, j) != 0)
            {
                ++out[i];
            }
        }
    }
    Eigen::MatrixXd matrix(graph.size(), graph.size());
    Eigen::VectorXd right(graph.size());
    for (int i = 0; i < graph.size(); ++i)
    {
        for (int j = 0; j < graph.size(); ++j)
        {
            if (i != j)
            {
                if (graph(j, i) != 0)
                {
                    matrix(i, j) = -lambda / out[j];
                }
                else
                {
                    matrix(i, j) = 0;
                }
            }
            else
            {
                matrix(i, j) = 1;
            }
        }
        right(i) = (1 - lambda) / graph.size();
    }

    Eigen::VectorXd solution = matrix.partialPivLu().solve(right);

    for (auto &el : graph)
    {
        weights[el.first] = solution(el.second);
    }
    return weights;
}

double compute_text_weight(map<wstring, double> &weights)
{
    double weight = 0;
    for (auto &el : weights)
    {
        weight += el.second;
    }
    return weight;
}

vector<vector<double>> floyd_warshall_algorithm(unoriented_graph<double> &graph)
{
	vector<vector<double>> distances(graph.size());
    for (int i = 0; i < graph.size(); ++i)
    {
        distances[i].resize(i);
        for (int j = 0; j < i; ++j)
        {
            distances[i][j] = (graph(i, j) > 0) ? graph(i, j) : INFTY;
        }
    }
	for (int k = 0; k < graph.size(); ++k)
	{
		vector<vector<double>> distances_new(graph.size());
		for (int i = 0; i < graph.size(); ++i)
		{
            distances_new[i].resize(i);
			for (int j = 0; j < i; ++j)
			{
                const double d1 = (i != k) ? distances[max(i, k)][min(i, k)] : 0;
                const double d2 = (j != k) ? distances[max(j, k)][min(j, k)] : 0;
				distances_new[i][j] = min(distances[i][j], d1 + d2);
			}
		}
		distances = distances_new;
	}
	return distances;
}

map<wstring, double> compute_cc(unoriented_graph<double> &graph)
{
	map<wstring, double> cc;
	vector<vector<double>> distances = floyd_warshall_algorithm(graph);
    for (auto &el : graph)
	{
		double sum = 0;
		for (int i = 0; i < distances.size(); ++i)
		{
			if (i != el.second)
			{
				sum += distances[max(i, el.second)][min(i, el.second)];
			}
		}
		cc[el.first] = (distances.size() - 1) / sum;
	}
	return cc;
}

void filter_cc_threshold(map<wstring, double> &cc, map<wstring, double> &weights, double threshold)
{
    for (auto it = cc.begin(); it != cc.end();)
    {
        if (weights[it->first] < threshold)
        {
            it = cc.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void filter_cc_n0(map<wstring, double> &cc, map<wstring, double> &weights, int n0)
{
    vector<wstring> words;
    words.reserve(weights.size());
    for (auto &el : weights)
    {
        words.push_back(el.first);
    }
    std::sort(words.begin(), words.end(),
        [&](wstring word1, wstring word2)
        {
            return (weights[word1] > weights[word2]);
        });
    auto cc_temp = cc;
    cc.clear();
    for (auto it = words.begin(); (it != words.end()) && (n0 > 0); ++it, --n0)
    {
        cc[*it] = cc_temp[*it];
    }
}

/*void filter_cc_common_words(map<wstring, double> &cc1, map<wstring, double> &cc2)
{
    for (auto it = cc1.begin(); it != cc1.end();)
    {
        if (!cc2.count(it->first))
        {
            it = cc1.erase(it);
        }
        else
        {
            ++it;
        }
    }
}*/

map<wstring, double> compute_diff(map<wstring, double> &sample_cc, map<wstring, double> &test_cc)
{
    map<wstring, double> diff;
    for (auto &el : sample_cc)
    {
        diff[el.first] = std::abs(el.second - test_cc[el.first]) / el.second;
    }
    return diff;
}

map<wstring, double> diff(text &sample, text &test, wofstream &sample_graphml_file, wofstream &test_graphml_file)
{
    int SQL_sample_file_id = SQL_init_file(sample.filename);
    int SQL_test_file_id = SQL_init_file(test.filename);
    SQL_delete_previous_data(SQL_sample_file_id);
    SQL_delete_previous_data(SQL_test_file_id);

    auto sample_oriented_graph1 = construct_oriented_graph1(sample); // построение ориентированного графа
    auto test_oriented_graph1 = construct_oriented_graph1(test);
    SQL_write_links(SQL_sample_file_id, sample_oriented_graph1);
    SQL_write_links(SQL_test_file_id, test_oriented_graph1);

    auto sample_oriented_graph2 = construct_oriented_graph2(sample_oriented_graph1, map<string, int>());
    auto test_oriented_graph2 = construct_oriented_graph2(test_oriented_graph1, map<string, int>());

    auto sample_weights = compute_weights(sample_oriented_graph2);
    auto test_weights = compute_weights(test_oriented_graph2);
    SQL_write_words(SQL_sample_file_id, sample_weights);
    SQL_write_words(SQL_test_file_id, test_weights);

    //double sample_weight = compute_text_weight(sample_weights); // вычисление весов текстов
    //double test_weight = compute_text_weight(test_weights);

    auto sample_unoriented_graph = construct_unoriented_graph(sample_oriented_graph2); // построение неориентированного графа
    auto test_unoriented_graph = construct_unoriented_graph(test_oriented_graph2);
    SQL_write_edge_weights(SQL_sample_file_id, sample_unoriented_graph);
    SQL_write_edge_weights(SQL_test_file_id, test_unoriented_graph);
    export_graph(sample_unoriented_graph, sample_graphml_file);
    export_graph(test_unoriented_graph, test_graphml_file);

    auto sample_cc = compute_cc(sample_unoriented_graph); // вычисление cc
    auto test_cc = compute_cc(test_unoriented_graph);

    filter_cc_threshold(sample_cc, sample_weights, sample_weights_threshold); // фильтраци€ по порогу sample
    filter_cc_threshold(sample_cc, test_weights, test_weights_threshold); // фильтраци€ по порогу test
    if (n0 != 0)
    {
        filter_cc_n0(sample_cc, sample_weights, n0); // фильтраци€ по n0
    }
    //filter_cc_common_words(sample_cc, test_cc); // удал€ем слова, которые присутствуют только в одном тексте

    return compute_diff(sample_cc, test_cc);
}

map<wstring, bool> compute_sim(map<wstring, double> &diff)
{
    double diff_avg = 0;
    for (auto &el : diff)
    {
        diff_avg += el.second;
    }
    diff_avg /= diff.size();
    map<wstring, bool> sim;
    for (auto &el : diff)
    {
        sim[el.first] = (diff_avg < 0.5) ? (el.second < 0.5) : (el.second < diff_avg);
    }
    return sim;
}

double compute_score(map<wstring, bool> &sim)
{
    double score = 0;
    for (auto &el : sim)
    {
        score += (int)el.second;
    }
    score /= sim.size();
    return score;
}

double score(map<wstring, double> &diff)
{
    return compute_score(compute_sim(diff)); // вычисление окончательной оценки score (стр. 24)
}

double percent(map<wstring, double> &diff)
{
    const int K = diff.size();
    int N = 0;
    for (auto &el : diff)
    {
        if (el.second <= epsilon)
        {
            ++N;
        }
    }
    return N / (double)K;
}