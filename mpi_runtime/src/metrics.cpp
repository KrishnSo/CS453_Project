#include "metrics.hpp"

#include <sstream>
#include <string>

std::string format_metrics_summary(const Metrics& m) {
    std::ostringstream o;
    o << "leader_rounds=" << m.leader_rounds << " dijkstra_iterations=" << m.dijkstra_iterations
      << " messages_sent=" << m.messages_sent << " bytes_sent=" << m.bytes_sent
      << " leader_time_s=" << m.leader_time << " dijkstra_time_s=" << m.dijkstra_time;
    return o.str();
}
