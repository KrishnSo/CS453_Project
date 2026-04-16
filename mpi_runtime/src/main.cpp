#include "dijkstra.hpp"
#include "graph.hpp"
#include "leader.hpp"
#include "logger.hpp"
#include "metrics.hpp"
#include "partition.hpp"
#include "util.hpp"

#include <mpi.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void usage(int rank) {
    if (rank == 0) {
        std::cout << "Usage:\n"
                  << "./ngs_mpi --graph <graph.json> --part <part.json> "
                  << "[--algo leader|dijkstra|both] [--source N] [--rounds R] [--seed S]\n"
                  << "  --rounds  max leader-election rounds (0 = until convergence)\n"
                  << "  --seed    recorded in logs for reproducibility (optional)\n";
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0, world_size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    try {
        std::string graph_file, part_file, algo = "both";
        int source = 0;
        int max_leader_rounds = 0;
        long long run_seed = -1;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--graph" && i + 1 < argc) graph_file = argv[++i];
            else if (arg == "--part" && i + 1 < argc) part_file = argv[++i];
            else if (arg == "--algo" && i + 1 < argc) algo = argv[++i];
            else if (arg == "--source" && i + 1 < argc) source = std::stoi(argv[++i]);
            else if (arg == "--rounds" && i + 1 < argc) max_leader_rounds = std::stoi(argv[++i]);
            else if (arg == "--seed" && i + 1 < argc) run_seed = std::stoll(argv[++i]);
            else {
                usage(rank);
                MPI_Finalize();
                return 1;
            }
        }

        if (graph_file.empty() || part_file.empty()) {
            usage(rank);
            MPI_Finalize();
            return 1;
        }

        Graph g = load_graph(graph_file);
        Partition p = load_partition(part_file, g.n, world_size);

        ensure_runtime_log_dir();
        std::ofstream logf(default_runtime_log_path(rank), std::ios::app);

        Metrics metrics;

        if (rank == 0) {
            std::cout << "Graph loaded: " << g.n << " nodes\n";
            std::cout << "Partition loaded: " << p.num_ranks << " ranks\n";
            if (run_seed >= 0) std::cout << "Run seed (recorded): " << run_seed << "\n";
        }
        if (run_seed >= 0) {
            log_line(logf, "run_seed=" + std::to_string(run_seed));
        }

        if (algo == "leader" || algo == "both") {
            if (rank == 0) std::cout << "\nRunning Leader Election...\n";
            int leader = run_leader(g, p, rank, metrics, logf, max_leader_rounds);
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == 0) {
                std::cout << "Leader: " << leader << "\n";
                std::cout << "Leader rounds: " << metrics.leader_rounds << "\n";
                std::cout << "Leader time: " << metrics.leader_time << " seconds\n";
            }
        }

        if (algo == "dijkstra" || algo == "both") {
            if (rank == 0) std::cout << "\nRunning Dijkstra...\n";
            std::vector<int> dist = run_dijkstra(g, p, rank, source, metrics, logf);
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == 0) {
                std::cout << "Distances from source " << source << ":\n";
                for (int i = 0; i < g.n; ++i) std::cout << i << " -> " << dist[i] << "\n";
                std::cout << "Dijkstra iterations: " << metrics.dijkstra_iterations << "\n";
                std::cout << "Dijkstra time: " << metrics.dijkstra_time << " seconds\n";
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            std::cout << "\nMetrics summary:\n";
            std::cout << format_metrics_summary(metrics) << "\n";
            std::cout << "Messages sent (logical count): " << metrics.messages_sent << "\n";
            std::cout << "Bytes sent (approx): " << metrics.bytes_sent << "\n";
        }
    } catch (const std::exception& ex) {
        if (rank == 0) std::cerr << "ERROR: " << ex.what() << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    MPI_Finalize();
    return 0;
}
