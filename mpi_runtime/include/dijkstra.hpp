#pragma once

#include "graph.hpp"
#include "metrics.hpp"
#include "partition.hpp"

#include <fstream>
#include <vector>

std::vector<int> run_dijkstra(const Graph& g, const Partition& p, int rank, int source,
                              Metrics& metrics, std::ofstream& logf);
