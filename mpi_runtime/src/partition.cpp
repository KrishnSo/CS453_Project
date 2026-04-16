#include "partition.hpp"
#include "util.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <string>

using json = nlohmann::json;

Partition load_partition(const std::string& path, int expected_nodes, int expected_ranks) {
    std::ifstream f(path);
    fail_if(!f.is_open(), "Could not open partition file: " + path);

    json j;
    f >> j;

    Partition p;
    p.num_ranks = j["num_ranks"].get<int>();
    fail_if(p.num_ranks != expected_ranks,
            "Partition num_ranks does not match MPI world size (expected " + std::to_string(expected_ranks) +
                ", got " + std::to_string(p.num_ranks) + ")");
    fail_if(p.num_ranks <= 0, "Invalid num_ranks");

    p.owner.assign(expected_nodes, -1);

    for (auto it = j["owner"].begin(); it != j["owner"].end(); ++it) {
        int node = std::stoi(it.key());
        int rank = it.value().get<int>();
        fail_if(node < 0 || node >= expected_nodes, "Owner node out of range");
        fail_if(rank < 0 || rank >= p.num_ranks, "Owner rank out of range for node " + std::to_string(node));
        fail_if(p.owner[node] != -1, "Duplicate owner entry for node " + std::to_string(node));
        p.owner[node] = rank;
    }

    for (int i = 0; i < expected_nodes; ++i) {
        fail_if(p.owner[i] < 0, "Missing owner assignment for node " + std::to_string(i));
    }

    p.local_nodes.assign(p.num_ranks, {});
    for (auto it = j["local_nodes"].begin(); it != j["local_nodes"].end(); ++it) {
        int r = std::stoi(it.key());
        fail_if(r < 0 || r >= p.num_ranks, "local_nodes key out of range");
        for (const auto& x : it.value()) p.local_nodes[r].push_back(x.get<int>());
    }

    p.ghost_nodes.assign(p.num_ranks, {});
    for (auto it = j["ghost_nodes"].begin(); it != j["ghost_nodes"].end(); ++it) {
        int r = std::stoi(it.key());
        fail_if(r < 0 || r >= p.num_ranks, "ghost_nodes key out of range");
        for (const auto& x : it.value()) p.ghost_nodes[r].push_back(x.get<int>());
    }

    return p;
}
