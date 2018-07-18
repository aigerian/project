#include "includes.h"

#include "graphs.h"
#include <cppconn/driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include "defines.h"

string concat(vector<string> list);
string Unicode2UTF8(wstring wstr);

extern sql::Statement *SQL;

int SQL_init_file(string &filename)
{
	sql::ResultSet *result = SQL->executeQuery(concat({ "SELECT id FROM files WHERE filename='", filename, "'" }));
	if (result->rowsCount() == 0)
	{
		SQL->execute(concat({ "INSERT INTO files SET filename='", filename, "'" }));
		result = SQL->executeQuery(concat({ "SELECT id FROM files WHERE filename='", filename, "'" }));
	}
	result->next();
	return result->getInt("id");
}

void SQL_delete_previous_data(int id)
{
	SQL->execute(concat({ "DELETE FROM words WHERE file_id=", to_string(id) }));
	SQL->execute(concat({ "DELETE FROM links WHERE file_id=", to_string(id) }));
	SQL->execute(concat({ "DELETE FROM edge_weights WHERE file_id=", to_string(id) }));
}

void SQL_write_words(int id, map<wstring, double> &weights)
{
    for (auto &el : weights)
	{
		string word = Unicode2UTF8(el.first);
		string value = to_string(el.second);
		std::replace(value.begin(), value.end(), ',', '.');
		SQL->execute(concat({ "INSERT INTO words SET file_id=", to_string(id), ", word=\"", word, "\", weight=", value }));
	}
}

void SQL_write_links(int id, oriented_graph<map<string, int>> &graph)
{
    for (auto &el1 : graph)
	{
		string word1 = Unicode2UTF8(el1.first);
        for (auto &el2 : graph)
		{
			if (el1.first == el2.first)
			{
				continue;
			}
			string word2 = Unicode2UTF8(el2.first);
			auto &mp = graph(el1.second, el2.second);
            for (auto &el3 : mp)
			{
				SQL->execute(concat({ "INSERT INTO links SET file_id=", to_string(id), ", source_word=\"", word1, "\", target_word=\"", word2, "\", type='", el3.first, "', count=", to_string(el3.second) }));
			}
		}
	}
}

void SQL_write_edge_weights(int id, unoriented_graph<double> &graph)
{
    for (auto &el1 : graph)
	{
		string word1 = Unicode2UTF8(el1.first);
        for (auto &el2 : graph)
		{
			if ((el1.first == el2.first) || (graph(el1.second, el2.second) == 0))
			{
				continue;
			}
			string word2 = Unicode2UTF8(el2.first);
			string value = to_string(graph(el1.second, el2.second));
			std::replace(value.begin(), value.end(), ',', '.');
			SQL->execute(concat({ "INSERT INTO edge_weights SET file_id=", to_string(id), ", source_word=\"", word1, "\", target_word=\"", word2, "\", weight=", value }));
		}
	}
}

void export_graph(unoriented_graph<double> &graph, wofstream &file)
{
    if (!file.good())
    {
        return;
    }
	file.imbue(locale(locale::classic(), new std::codecvt_utf8<wchar_t>)); // установка локали для записи в файл
	file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	file << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns\nhttp://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">" << endl;
	file << "<key id=\"d0\" for=\"node\" attr.name=\"label\" attr.type=\"string\">" << endl;
	file << "<default>black</default>" << endl;
	file << "</key>" << endl;
	file << "<key id=\"d1\" for=\"edge\" attr.name=\"label\" attr.type=\"string\"/>" << endl;
	file << "<graph id=\"G\" edgedefault=\"undirected\">" << endl;
    for (auto &el : graph)
	{
		file << "<node id=\"n" << el.second << "\">" << endl;
		file << "<data key=\"d0\">" << el.first << "</data>" << endl;
		file << "</node>" << endl;
	}
	int k = 0;
	for (int i = 0; i < graph.size(); ++i)
	{
		for (int j = 0; j < i; ++j)
		{
			if (graph(i, j) != 0)
			{
				file << "<edge id=\"e" << k << "\" source=\"n" << i << "\" target=\"n" << j << "\">" << endl;
				file << "<data key=\"d1\">" << graph(i, j) << "</data>" << endl;
				file << "</edge>" << endl;
				++k;
			}
		}
	}
	file << "</graph>" << endl << "</graphml>";
}

/*void export_two_graphs(graph<double> &graph1, graph<double> &graph2, wofstream &file)
{
	graph<tuple<double, double>> united_graph;
	united_graph.vertex_of_word = graph1.vertex_of_word;
	int vertex_num = graph1.vertex_of_word.size();
    for (auto &el : graph2)
	{
		if (!united_graph.vertex_of_word.count(el.first))
		{
			united_graph.vertex_of_word[el.first] = vertex_num;
			++vertex_num;
		}
	}
	united_graph.matrix.assign(vertex_num, vector<tuple<double, double>>(vertex_num, tuple<double, double>(0, 0)));
    for (auto &el1 : graph1)
	{
        for (auto &el2 : graph1)
		{
			const int vertex1 = united_graph.vertex_of_word[el1.first];
			const int vertex2 = united_graph.vertex_of_word[el2.first];
			get<0>(united_graph.matrix[vertex1][vertex2]) = graph1.matrix[el1.second][el2.second];
			get<0>(united_graph.matrix[vertex2][vertex1]) = graph1.matrix[el1.second][el2.second];
		}
	}
    for (auto &el1 : graph2)
    {
        for (auto &el2 : graph2)
		{
			const int vertex1 = united_graph.vertex_of_word[el1.first];
			const int vertex2 = united_graph.vertex_of_word[el2.first];
			get<1>(united_graph.matrix[vertex1][vertex2]) = graph2.matrix[el1.second][el2.second];
			get<1>(united_graph.matrix[vertex2][vertex1]) = graph2.matrix[el1.second][el2.second];
		}
	}

	file.imbue(locale(locale::classic(), new std::codecvt_utf8<wchar_t>)); // установка локали для записи в файл
	file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	file << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns\nhttp://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">" << endl;
	file << "<key id=\"d0\" for=\"node\" attr.name=\"label\" attr.type=\"string\">" << endl;
	file << "<default>black</default>" << endl;
	file << "</key>" << endl;
	file << "<key id=\"d1\" for=\"edge\" attr.name=\"label\" attr.type=\"string\"/>" << endl;
	file << "<key id=\"d2\" for=\"edge\" attr.name=\"color\" attr.type=\"string\"/>" << endl;
	file << "<graph id=\"G\" edgedefault=\"undirected\">" << endl;
    for (auto &el : united_graph)
	{
		file << "<node id=\"n" << el.second << "\">" << endl;
		file << "<data key=\"d0\">" << el.first << "</data>" << endl;
		file << "</node>" << endl;
	}
	int k = 0;
	for (int i = 0; i < united_graph.vertex_of_word.size(); ++i)
	{
		for (int j = 0; j < i; ++j)
		{
			if (get<0>(united_graph.matrix[i][j]) != 0)
			{
				file << "<edge id=\"e" << k << "\" source=\"n" << i << "\" target=\"n" << j << "\">" << endl;
				file << "<data key=\"d1\">" << get<0>(united_graph.matrix[i][j]) << "</data>" << endl;
				file << "</edge>" << endl;
				++k;
			}
			if (get<1>(united_graph.matrix[i][j]) != 0)
			{
				file << "<edge id=\"e" << k << "\" source=\"n" << i << "\" target=\"n" << j << "\">" << endl;
				file << "<data key=\"d1\">" << get<1>(united_graph.matrix[i][j]) << "</data>" << endl;
				file << "<data key=\"d2\">0,255,0</data>" << endl;
				file << "</edge>" << endl;
				++k;
			}
		}
	}
	file << "</graph>" << endl << "</graphml>";
}*/