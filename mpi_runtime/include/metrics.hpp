#pragma once

#include <string>

struct Metrics {
    int leader_rounds = 0;
    int dijkstra_iterations = 0;
    long long messages_sent = 0;
    long long bytes_sent = 0;
    double leader_time = 0.0;
    double dijkstra_time = 0.0;
};

std::string format_metrics_summary(const Metrics& m);
