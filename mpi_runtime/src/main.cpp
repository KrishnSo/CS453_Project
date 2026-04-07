#include <mpi.h>
#include <nlohmann/json.hpp>

#include <climits>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace std;

struct Edge {
    int to;
    int w;
};

struct Graph {
    int n = 0;
    vector<vector<Edge>> adj;
};

struct Partition {
    int num_ranks = 0;
    vector<int> owner;
};

struct Metrics {
    int leader_rounds = 0;
    int dijkstra_iterations = 0;
    long long message_count = 0;
    long long bytes_sent = 0;
    double leader_time = 0.0;
    double dijkstra_time = 0.0;
};

static void fail_if(bool cond, const string& message) {
    if (cond) {
        throw runtime_error(message);
    }
}

Graph load_graph(const string& path) {
    ifstream f(path);
    fail_if(!f.is_open(), "Could not open graph file: " + path);

    json j;
    f >> j;

    fail_if(!j.contains("num_nodes"), "Graph JSON missing num_nodes");
    fail_if(!j.contains("edges"), "Graph JSON missing edges");

    Graph g;
    g.n = j["num_nodes"].get<int>();
    fail_if(g.n <= 0, "Graph must have at least one node");

    g.adj.assign(g.n, {});

    for (const auto& e : j["edges"]) {
        fail_if(!e.contains("u") || !e.contains("v") || !e.contains("w"),
                "Edge missing u/v/w fields");

        int u = e["u"].get<int>();
        int v = e["v"].get<int>();
        int w = e["w"].get<int>();

        fail_if(u < 0 || u >= g.n || v < 0 || v >= g.n,
                "Edge endpoint out of range");
        fail_if(w <= 0, "Dijkstra requires positive edge weights");

        g.adj[u].push_back({v, w});
        g.adj[v].push_back({u, w});
    }

    return g;
}

Partition load_partition(const string& path) {
    ifstream f(path);
    fail_if(!f.is_open(), "Could not open partition file: " + path);

    json j;
    f >> j;

    fail_if(!j.contains("num_ranks"), "Partition JSON missing num_ranks");
    fail_if(!j.contains("owner"), "Partition JSON missing owner");

    Partition p;
    p.num_ranks = j["num_ranks"].get<int>();

    const auto& owner_json = j["owner"];
    fail_if(!owner_json.is_object(), "Partition owner must be a JSON object");

    int n = static_cast<int>(owner_json.size());
    p.owner.assign(n, -1);

    for (auto it = owner_json.begin(); it != owner_json.end(); ++it) {
        int node = stoi(it.key());
        int rank = it.value().get<int>();
        fail_if(node < 0 || node >= n, "Partition node id out of range");
        fail_if(rank < 0 || rank >= p.num_ranks, "Partition rank id out of range");
        p.owner[node] = rank;
    }

    for (int i = 0; i < n; ++i) {
        fail_if(p.owner[i] == -1, "Partition missing owner for node " + to_string(i));
    }

    return p;
}

vector<int> owned_nodes(const Partition& p, int rank) {
    vector<int> nodes;
    for (int i = 0; i < static_cast<int>(p.owner.size()); ++i) {
        if (p.owner[i] == rank) {
            nodes.push_back(i);
        }
    }
    return nodes;
}

int run_leader_election(const Graph& g,
                        const Partition& p,
                        int rank,
                        Metrics& metrics) {
    vector<int> candidate(g.n);
    for (int i = 0; i < g.n; ++i) {
        candidate[i] = i;
    }

    double start = MPI_Wtime();

    bool global_changed = true;
    while (global_changed) {
        ++metrics.leader_rounds;

        vector<int> next = candidate;
        bool local_changed = false;

        for (int u = 0; u < g.n; ++u) {
            if (p.owner[u] != rank) {
                continue;
            }
            int best = candidate[u];
            for (const auto& e : g.adj[u]) {
                best = max(best, candidate[e.to]);
            }
            if (best != candidate[u]) {
                next[u] = best;
                local_changed = true;
            }
        }

        // Simple global sync baseline. Count this collective as one logical message per rank.
        metrics.message_count += 1;
        metrics.bytes_sent += static_cast<long long>(g.n) * sizeof(int);

        MPI_Allreduce(next.data(), candidate.data(), g.n, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

        int local_flag = local_changed ? 1 : 0;
        int global_flag = 0;
        MPI_Allreduce(&local_flag, &global_flag, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        global_changed = (global_flag != 0);
    }

    metrics.leader_time = MPI_Wtime() - start;

    int local_leader = -1;
    for (int u = 0; u < g.n; ++u) {
        if (p.owner[u] == rank) {
            local_leader = max(local_leader, candidate[u]);
        }
    }

    int global_leader = -1;
    MPI_Allreduce(&local_leader, &global_leader, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    return global_leader;
}

vector<int> run_dijkstra(const Graph& g,
                         const Partition& p,
                         int rank,
                         int source,
                         Metrics& metrics) {
    vector<int> dist(g.n, INT_MAX);
    vector<bool> visited(g.n, false);

    fail_if(source < 0 || source >= g.n, "Source node out of range");

    if (rank == p.owner[source]) {
        dist[source] = 0;
    }

    double start = MPI_Wtime();

    for (int iter = 0; iter < g.n; ++iter) {
        ++metrics.dijkstra_iterations;

        int local_min = INT_MAX;
        int local_node = -1;

        for (int i = 0; i < g.n; ++i) {
            if (p.owner[i] == rank && !visited[i] && dist[i] < local_min) {
                local_min = dist[i];
                local_node = i;
            }
        }

        struct {
            int val;
            int idx;
        } local_pair{local_min, local_node}, global_pair{INT_MAX, -1};

        metrics.message_count += 1;
        metrics.bytes_sent += sizeof(local_pair);

        MPI_Allreduce(&local_pair, &global_pair, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);

        int u = global_pair.idx;
        if (u == -1 || global_pair.val == INT_MAX) {
            break;
        }

        visited[u] = true;

        if (p.owner[u] == rank) {
            for (const auto& e : g.adj[u]) {
                if (!visited[e.to] && dist[u] != INT_MAX) {
                    long long cand = static_cast<long long>(dist[u]) + e.w;
                    if (cand < dist[e.to]) {
                        dist[e.to] = static_cast<int>(cand);
                    }
                }
            }
        }

        metrics.message_count += 1;
        metrics.bytes_sent += static_cast<long long>(g.n) * sizeof(int);

        // Global sync baseline for correctness.
        MPI_Allreduce(MPI_IN_PLACE, dist.data(), g.n, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    }

    metrics.dijkstra_time = MPI_Wtime() - start;
    return dist;
}

void print_usage(int rank) {
    if (rank == 0) {
        cout << "Usage:\n"
             << "  ./ngs_mpi --graph <graph.json> --part <part.json> "
             << "[--algo leader|dijkstra|both] [--source N]\n";
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int world_size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    try {
        string graph_file;
        string part_file;
        string algo = "both";
        int source = 0;

        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--graph" && i + 1 < argc) {
                graph_file = argv[++i];
            } else if (arg == "--part" && i + 1 < argc) {
                part_file = argv[++i];
            } else if (arg == "--algo" && i + 1 < argc) {
                algo = argv[++i];
            } else if (arg == "--source" && i + 1 < argc) {
                source = stoi(argv[++i]);
            } else {
                print_usage(rank);
                MPI_Finalize();
                return 1;
            }
        }

        if (graph_file.empty() || part_file.empty()) {
            print_usage(rank);
            MPI_Finalize();
            return 1;
        }

        Graph g = load_graph(graph_file);
        Partition p = load_partition(part_file);

        fail_if(static_cast<int>(p.owner.size()) != g.n,
                "Partition size does not match graph node count");
        fail_if(p.num_ranks != world_size,
                "Partition num_ranks does not match MPI world size");

        Metrics metrics;

        if (rank == 0) {
            cout << "Graph loaded: " << g.n << " nodes\n";
            cout << "Partition loaded: " << p.num_ranks << " ranks\n";
        }

        MPI_Barrier(MPI_COMM_WORLD);

        if (algo == "leader" || algo == "both") {
            if (rank == 0) {
                cout << "\nRunning Leader Election...\n";
            }

            int leader = run_leader_election(g, p, rank, metrics);

            MPI_Barrier(MPI_COMM_WORLD);

            if (rank == 0) {
                cout << "Leader: " << leader << "\n";
                cout << "Leader rounds: " << metrics.leader_rounds << "\n";
                cout << "Leader time: " << metrics.leader_time << " seconds\n";
            }
        }

        if (algo == "dijkstra" || algo == "both") {
            if (rank == 0) {
                cout << "\nRunning Dijkstra...\n";
            }

            vector<int> dist = run_dijkstra(g, p, rank, source, metrics);

            MPI_Barrier(MPI_COMM_WORLD);

            if (rank == 0) {
                cout << "Distances from source " << source << ":\n";
                for (int i = 0; i < g.n; ++i) {
                    cout << i << " -> " << dist[i] << "\n";
                }
                cout << "Dijkstra iterations: " << metrics.dijkstra_iterations << "\n";
                cout << "Dijkstra time: " << metrics.dijkstra_time << " seconds\n";
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);

        if (rank == 0) {
            cout << "\nMetrics summary:\n";
            cout << "Messages sent (logical count): " << metrics.message_count << "\n";
            cout << "Bytes sent (approx): " << metrics.bytes_sent << "\n";
        }
    } catch (const exception& ex) {
        if (rank == 0) {
            cerr << "ERROR: " << ex.what() << "\n";
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    MPI_Finalize();
    return 0;
}