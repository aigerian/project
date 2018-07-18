#include "includes.h"

#include <boost/program_options.hpp>
#include <cppconn/driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include "inverse_index.h"
#include "link-grammar/link-includes.h"
#include "text.h"

#include "defines.h"

namespace po = boost::program_options;

void choose_locale();
string concat(vector<string> list);
map<wstring, double> diff(text &sample, text &test, wofstream &sample_graphml_file, wofstream &test_graphml_file);
double score(map<wstring, double> &diff);
double percent(map<wstring, double> &diff);

Dictionary link_grammar_dict;
sql::Driver *SQL_driver = get_driver_instance();
sql::Connection *SQL_connection = SQL_driver->connect("localhost", "root", "");
sql::Statement *SQL = SQL_connection->createStatement();
string sample_filename;
string test_filename;
string sample_graphml_filename;
string test_graphml_filename;
string tf_idf_filename;
bool order_mode;
bool link_grammar_mode;
double sample_tf_idf_threshold;
double test_tf_idf_threshold;
double sample_weights_threshold;
double test_weights_threshold;
int n0;
double lambda;
double epsilon;

void parse_parameters(int argc, char **argv)
{
    try
    {
        int links_mode;
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("sample,s", po::value<string>(&sample_filename)->required(), "sample text filename")
            ("test,t", po::value<string>(&test_filename)->required(), "test text filename")
            ("sample_graphml", po::value<string>(&sample_graphml_filename), "where to save graph for sample text")
            ("test_graphml", po::value<string>(&test_graphml_filename), "where to save graph for test text")
            ("tf_idf_text", po::value<string>(&tf_idf_filename), "filename of text using for TF-IDF")
            ("links_mode", po::value<int>(&links_mode)->default_value(0), "0 to use order and Link Grammar, 1 to use only order, 2 to use only Link Grammar")
            ("test_tf_idf_threshold", po::value<double>(&test_tf_idf_threshold)->default_value(0.01), "TF-IDF threshold for significant words for sample text")
            ("sample_weights_threshold", po::value<double>(&sample_weights_threshold)->default_value(0), "weights threshold for significant words for sample text")
            ("test_weights_threshold", po::value<double>(&test_weights_threshold)->default_value(0), "weights threshold for significant words for test text")
            ("n0", po::value<int>(&n0)->default_value(0), "number of significant words in sample text (0 for all words)")
            ("lambda,l", po::value<double>(&lambda)->default_value(0.2), "parameter lambda for weights computing")
            ("epsilon,e", po::value<double>(&epsilon)->default_value(0.4), "parameter epsilon for percent metric")
            ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        if (vm.count("help"))
        {
            cout << desc << endl;
            exit(0);
        }
        po::notify(vm);
        order_mode = (links_mode == 0) || (links_mode == 1);
        link_grammar_mode = (links_mode == 0) || (links_mode == 2);
    }
    catch (std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        exit(1);
    }
}

int main(int argc, char **argv)
{
    parse_parameters(argc, argv);
    SQL->execute("USE texts");
    choose_locale();
    if (link_grammar_mode)
    {
        link_grammar_dict = dictionary_create_lang(LANG);
    }

    // тексты
    /* text *sample;
    text *test;
    if (tf_idf_filename != "")
    {
        ifstream tf_idf_text(tf_idf_filename);
        inverse_index inv_index(tf_idf_text);
        sample = new text(sample_filename, inv_index, sample_tf_idf_threshold, link_grammar_mode);
        test = new text(test_filename, inv_index, test_tf_idf_threshold, link_grammar_mode);
    }
    else
    {
        sample = new text(sample_filename, true, link_grammar_mode);
        test = new text(test_filename, true, link_grammar_mode);
    }
    auto diff_value = diff(*sample, *test, wofstream(sample_graphml_filename), wofstream(test_graphml_filename));
    cout << "Score = " << score(diff_value) << endl;
    cout << "Percent = " << percent(diff_value) * 100 << "%" << endl;*/

    // сниппеты
   text query_text("../texts/snippets/query.txt", inv_index, 0.01);
    int snippets_num = 0;
    for (; ifstream("../texts/snippets/snippet" + to_string(snippets_num) + ".txt"); ++snippets_num);
    vector<int> indices(snippets_num);
    vector<double> scores(snippets_num);
    vector<double> percents(snippets_num);
    for (int i = 0; i < snippets_num; ++i)
    {
        indices[i] = i;
        auto diff_value = diff(
            query_text,
            text("../texts/snippets/snippet" + to_string(i) + ".txt", inv_index, 0.01),
            wofstream("../graphml/query.graphml"),
            wofstream("../graphml/snippet" + to_string(i) + ".graphml")
        );
        scores[i] = score(diff_value);
        percents[i] = percent(diff_value, 0.1);
    }

    std::sort(indices.begin(), indices.end(), [&](int i, int j) { return scores[i] > scores[j]; });
    for (int index : indices)
    {
        cout << "Snippet " << index << ": score = " << scores[index] << ", percent = " << percents[index] * 100 << endl;
    }

    if (link_grammar_mode)
    {
        cout << endl;
        dictionary_delete(link_grammar_dict);
    }
    delete SQL;
    delete SQL_connection;

    return 0;
}