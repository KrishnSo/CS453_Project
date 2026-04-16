#include "graph.hpp"
#include "util.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

using json = nlohmann::json;

Graph load_graph(const std::string& path) {
    std::ifstream f(path);
    fail_if(!f.is_open(), "Could not open graph file: " + path);

    json j;
    f >> j;

    Graph g;
    g.n = j["num_nodes"].get<int>();
    fail_if(g.n <= 0, "Graph must have at least one node");
    g.adj.assign(g.n, {});

    for (const auto& e : j["edges"]) {
        int u = e["u"].get<int>();
        int v = e["v"].get<int>();
        int w = e["w"].get<int>();
        fail_if(u < 0 || u >= g.n || v < 0 || v >= g.n, "Edge endpoint out of range");
        fail_if(w <= 0, "Edge weights must be positive");

        g.adj[u].push_back({v, w});
        g.adj[v].push_back({u, w});
    }

    return g;
}
