#include "dijkstra.hpp"
#include "logger.hpp"
#include "util.hpp"

#include <mpi.h>

#include <climits>
#include <map>
#include <vector>

std::vector<int> run_dijkstra(const Graph& g, const Partition& p, int rank, int source,
                              Metrics& metrics, std::ofstream& logf) {
    fail_if(source < 0 || source >= g.n, "Source out of range");

    std::vector<int> dist(g.n, INT_MAX);
    std::vector<bool> visited(g.n, false);

    if (p.owner[source] == rank) dist[source] = 0;

    double start = MPI_Wtime();

    while (true) {
        metrics.dijkstra_iterations++;

        int local_min = INT_MAX;
        int local_node = -1;

        for (int u : p.local_nodes[rank]) {
            if (!visited[u] && dist[u] < local_min) {
                local_min = dist[u];
                local_node = u;
            }
        }

        struct {
            int val;
            int idx;
        } local_pair{local_min, local_node}, global_pair{INT_MAX, -1};

        MPI_Allreduce(&local_pair, &global_pair, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);
        metrics.messages_sent += 1;
        metrics.bytes_sent += sizeof(local_pair);

        int u = global_pair.idx;
        if (u == -1 || global_pair.val == INT_MAX) break;

        visited[u] = true;

        std::map<int, std::vector<int>> sends;
        if (p.owner[u] == rank) {
            for (const auto& e : g.adj[u]) {
                if (visited[e.to] || dist[u] == INT_MAX) continue;
                int cand = dist[u] + e.w;
                int owner_v = p.owner[e.to];

                if (owner_v == rank) {
                    if (cand < dist[e.to]) dist[e.to] = cand;
                } else {
                    sends[owner_v].push_back(e.to);
                    sends[owner_v].push_back(cand);
                }
            }
        }

        for (int r = 0; r < p.num_ranks; ++r) {
            if (r == rank) continue;
            int count = sends.count(r) ? static_cast<int>(sends[r].size()) : 0;

            MPI_Send(&count, 1, MPI_INT, r, 200, MPI_COMM_WORLD);
            metrics.messages_sent += 1;
            metrics.bytes_sent += sizeof(int);

            if (count > 0) {
                MPI_Send(sends[r].data(), count, MPI_INT, r, 201, MPI_COMM_WORLD);
                metrics.messages_sent += 1;
                metrics.bytes_sent += static_cast<long long>(count) * sizeof(int);
            }
        }

        for (int r = 0; r < p.num_ranks; ++r) {
            if (r == rank) continue;
            int count = 0;
            MPI_Recv(&count, 1, MPI_INT, r, 200, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (count > 0) {
                std::vector<int> buf(count);
                MPI_Recv(buf.data(), count, MPI_INT, r, 201, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int i = 0; i + 1 < count; i += 2) {
                    int v = buf[i];
                    int cand = buf[i + 1];
                    if (cand < dist[v]) dist[v] = cand;
                }
            }
        }

        MPI_Allreduce(MPI_IN_PLACE, dist.data(), g.n, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        metrics.messages_sent += 1;
        metrics.bytes_sent += static_cast<long long>(g.n) * sizeof(int);
    }

    metrics.dijkstra_time = MPI_Wtime() - start;
    log_line(logf, "Dijkstra done");
    return dist;
}
