#pragma once
#include "includes.h"

template <typename T> class oriented_graph;
template <typename T> class unoriented_graph;

template <typename T> class oriented_graph
{
    map<wstring, int> vertex_of_word;
    vector<vector<T>> matrix;

public:
    map<wstring, int>::iterator begin()
    {
        return vertex_of_word.begin();
    }
    map<wstring, int>::iterator end()
    {
        return vertex_of_word.end();
    }
    int size()
    {
        return vertex_of_word.size();
    }
    void add_word(wstring word)
    {
        if (!vertex_of_word.count(word))
        {
            vertex_of_word[word] = vertex_of_word.size();
        }
    }
    void assign_matrix()
    {
        matrix.assign(vertex_of_word.size(), vector<T>(vertex_of_word.size()));
    }
    T & operator () (int i, int j)
    {
        return matrix[i][j];
    }
    T & operator () (wstring word1, wstring word2)
    {
        return matrix[vertex_of_word[word1]][vertex_of_word[word2]];
    }
};

template <typename T> class unoriented_graph
{
    map<wstring, int> vertex_of_word;
    vector<vector<T>> matrix;

public:
    map<wstring, int>::iterator begin()
    {
        return vertex_of_word.begin();
    }
    map<wstring, int>::iterator end()
    {
        return vertex_of_word.end();
    }
    int size()
    {
        return vertex_of_word.size();
    }
    void add_word(wstring word)
    {
        if (!vertex_of_word.count(word))
        {
            vertex_of_word[word] = vertex_of_word.size();
        }
    }
    void assign_matrix()
    {
        matrix.resize(vertex_of_word.size());
        for (int i = 0; i < matrix.size(); ++i)
        {
            matrix[i].resize(i);
        }
    }
    T & operator () (int i, int j)
    {
        return matrix[max(i, j)][min(i, j)];
    }
    T & operator () (wstring word1, wstring word2)
    {
        return matrix[max(vertex_of_word[word1], vertex_of_word[word2])][min(vertex_of_word[word1], vertex_of_word[word2])];
    }
};