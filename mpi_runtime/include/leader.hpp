#pragma once

#include "graph.hpp"
#include "metrics.hpp"
#include "partition.hpp"

#include <fstream>
#include <vector>

int run_leader(const Graph& g, const Partition& p, int rank, Metrics& metrics, std::ofstream& logf,
               int max_rounds);
