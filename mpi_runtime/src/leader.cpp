#include "leader.hpp"
#include "logger.hpp"
#include "util.hpp"

#include <mpi.h>

#include <algorithm>
#include <vector>

int run_leader(const Graph& g, const Partition& p, int rank, Metrics& metrics, std::ofstream& logf,
               int max_rounds) {
    std::vector<int> candidate(g.n);
    for (int i = 0; i < g.n; ++i) candidate[i] = i;

    double start = MPI_Wtime();
    bool global_changed = true;

    while (global_changed) {
        if (max_rounds > 0 && metrics.leader_rounds >= max_rounds) break;
        metrics.leader_rounds++;

        std::vector<int> next = candidate;
        bool local_changed = false;

        for (int u : p.local_nodes[rank]) {
            int best = candidate[u];
            for (const auto& e : g.adj[u]) {
                best = std::max(best, candidate[e.to]);
            }
            if (best != candidate[u]) {
                next[u] = best;
                local_changed = true;
            }
        }

        for (int u : p.local_nodes[rank]) candidate[u] = next[u];

        MPI_Allreduce(MPI_IN_PLACE, candidate.data(), g.n, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        metrics.messages_sent += 1;
        metrics.bytes_sent += static_cast<long long>(g.n) * sizeof(int);

        int changed_flag = local_changed ? 1 : 0;
        int global_flag = 0;
        MPI_Allreduce(&changed_flag, &global_flag, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        metrics.messages_sent += 1;
        metrics.bytes_sent += sizeof(int);
        global_changed = (global_flag != 0);
    }

    metrics.leader_time = MPI_Wtime() - start;

    int cmin = *std::min_element(candidate.begin(), candidate.end());
    int cmax = *std::max_element(candidate.begin(), candidate.end());
    int gmin = 0;
    int gmax = 0;
    MPI_Allreduce(&cmin, &gmin, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    MPI_Allreduce(&cmax, &gmax, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    metrics.messages_sent += 2;
    metrics.bytes_sent += 2 * sizeof(int);

    fail_if(gmin != gmax, "Leader election: candidate values disagree across nodes (not converged)");
    int global_leader = gmax;

    log_line(logf, "Leader done, leader=" + std::to_string(global_leader) + ", agreement_ok=1");
    return global_leader;
}
