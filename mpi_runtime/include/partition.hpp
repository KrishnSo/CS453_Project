#pragma once

#include <string>
#include <vector>

struct Partition {
    int num_ranks = 0;
    std::vector<int> owner;
    std::vector<std::vector<int>> local_nodes;
    std::vector<std::vector<int>> ghost_nodes;
};

Partition load_partition(const std::string& path, int expected_nodes, int expected_ranks);
