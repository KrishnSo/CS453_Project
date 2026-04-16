#pragma once

#include <string>
#include <vector>

struct Edge {
    int to{};
    int w{};
};

struct Graph {
    int n = 0;
    std::vector<std::vector<Edge>> adj;
};

Graph load_graph(const std::string& path);
