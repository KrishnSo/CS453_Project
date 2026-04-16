#include <mpi.h>
#include <nlohmann/json.hpp>

#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
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
    vector<vector<int>> local_nodes;
    vector<vector<int>> ghost_nodes;
};

struct Metrics {
    int leader_rounds = 0;
    int dijkstra_iterations = 0;
    long long messages_sent = 0;
    long long bytes_sent = 0;
    double leader_time = 0.0;
    double dijkstra_time = 0.0;
};

static void fail_if(bool cond, const string& msg) {
    if (cond) throw runtime_error(msg);
}

static void log_line(ofstream& logf, const string& s) {
    logf << s << '\n';
    logf.flush();
}

Graph load_graph(const string& path) {
    ifstream f(path);
    fail_if(!f.is_open(), "Could not open graph file: " + path);

    json j;
    f >> j;

    Graph g;
    g.n = j["num_nodes"].get<int>();
    fail_if(g.n <= 0, "Graph must have at least one node");
    g.adj.assign(g.n, {});

    for (const auto& e : j["edges"]) {
        int u = e["u"].get<int>();
        int v = e["v"].get<int>();
        int w = e["w"].get<int>();
        fail_if(u < 0 || u >= g.n || v < 0 || v >= g.n, "Edge endpoint out of range");
        fail_if(w <= 0, "Edge weights must be positive");

        g.adj[u].push_back({v, w});
        g.adj[v].push_back({u, w});
    }

    return g;
}

Partition load_partition(const string& path, int expected_nodes) {
    ifstream f(path);
    fail_if(!f.is_open(), "Could not open partition file: " + path);

    json j;
    f >> j;

    Partition p;
    p.num_ranks = j["num_ranks"].get<int>();
    p.owner.assign(expected_nodes, -1);

    for (auto it = j["owner"].begin(); it != j["owner"].end(); ++it) {
        int node = stoi(it.key());
        int rank = it.value().get<int>();
        fail_if(node < 0 || node >= expected_nodes, "Owner node out of range");
        p.owner[node] = rank;
    }

    p.local_nodes.assign(p.num_ranks, {});
    for (auto it = j["local_nodes"].begin(); it != j["local_nodes"].end(); ++it) {
        int r = stoi(it.key());
        for (const auto& x : it.value()) p.local_nodes[r].push_back(x.get<int>());
    }

    p.ghost_nodes.assign(p.num_ranks, {});
    for (auto it = j["ghost_nodes"].begin(); it != j["ghost_nodes"].end(); ++it) {
        int r = stoi(it.key());
        for (const auto& x : it.value()) p.ghost_nodes[r].push_back(x.get<int>());
    }

    return p;
}

set<int> remote_neighbor_ranks(const Graph& g, const Partition& p, int rank) {
    set<int> res;
    for (int u : p.local_nodes[rank]) {
        for (const auto& e : g.adj[u]) {
            int r2 = p.owner[e.to];
            if (r2 != rank) res.insert(r2);
        }
    }
    return res;
}

int run_leader(const Graph& g, const Partition& p, int rank, Metrics& metrics, ofstream& logf) {
    vector<int> candidate(g.n);
    for (int i = 0; i < g.n; ++i) candidate[i] = i;

    double start = MPI_Wtime();
    set<int> nbr_ranks = remote_neighbor_ranks(g, p, rank);
    bool global_changed = true;

    while (global_changed) {
        metrics.leader_rounds++;

        vector<int> next = candidate;
        bool local_changed = false;

        for (int u : p.local_nodes[rank]) {
            int best = candidate[u];
            for (const auto& e : g.adj[u]) {
                best = max(best, candidate[e.to]);
            }
            if (best != candidate[u]) {
                next[u] = best;
                local_changed = true;
            }
        }

        for (int other : nbr_ranks) {
            vector<int> recvbuf(g.n, -1);
            MPI_Sendrecv(
                next.data(), g.n, MPI_INT, other, 100,
                recvbuf.data(), g.n, MPI_INT, other, 100,
                MPI_COMM_WORLD, MPI_STATUS_IGNORE
            );
            metrics.messages_sent += 1;
            metrics.bytes_sent += static_cast<long long>(g.n) * sizeof(int);

            for (int ghost : p.ghost_nodes[rank]) {
                next[ghost] = max(next[ghost], recvbuf[ghost]);
            }
        }

        for (int u : p.local_nodes[rank]) candidate[u] = next[u];

        MPI_Allreduce(MPI_IN_PLACE, candidate.data(), g.n, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        metrics.messages_sent += 1;
        metrics.bytes_sent += static_cast<long long>(g.n) * sizeof(int);

        int changed_flag = local_changed ? 1 : 0;
        int global_flag = 0;
        MPI_Allreduce(&changed_flag, &global_flag, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        global_changed = (global_flag != 0);
    }

    metrics.leader_time = MPI_Wtime() - start;

    int local_leader = -1;
    for (int u : p.local_nodes[rank]) local_leader = max(local_leader, candidate[u]);

    int global_leader = -1;
    MPI_Allreduce(&local_leader, &global_leader, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    log_line(logf, "Leader done, leader=" + to_string(global_leader));
    return global_leader;
}

vector<int> run_dijkstra(const Graph& g, const Partition& p, int rank, int source,
                         Metrics& metrics, ofstream& logf) {
    fail_if(source < 0 || source >= g.n, "Source out of range");

    vector<int> dist(g.n, INT_MAX);
    vector<bool> visited(g.n, false);

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

        map<int, vector<int>> sends;
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
                vector<int> buf(count);
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

static void usage(int rank) {
    if (rank == 0) {
        cout << "Usage:\n"
             << "./ngs_mpi --graph <graph.json> --part <part.json> "
             << "[--algo leader|dijkstra|both] [--source N]\n";
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0, world_size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    try {
        string graph_file, part_file, algo = "both";
        int source = 0;

        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--graph" && i + 1 < argc) graph_file = argv[++i];
            else if (arg == "--part" && i + 1 < argc) part_file = argv[++i];
            else if (arg == "--algo" && i + 1 < argc) algo = argv[++i];
            else if (arg == "--source" && i + 1 < argc) source = stoi(argv[++i]);
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
        Partition p = load_partition(part_file, g.n);
        fail_if(p.num_ranks != world_size, "Partition rank count does not match MPI world size");

        system("mkdir -p outputs/logs");
        ofstream logf("outputs/logs/runtime_rank" + to_string(rank) + ".log", ios::app);

        Metrics metrics;

        if (rank == 0) {
            cout << "Graph loaded: " << g.n << " nodes\n";
            cout << "Partition loaded: " << p.num_ranks << " ranks\n";
        }

        if (algo == "leader" || algo == "both") {
            if (rank == 0) cout << "\nRunning Leader Election...\n";
            int leader = run_leader(g, p, rank, metrics, logf);
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == 0) {
                cout << "Leader: " << leader << "\n";
                cout << "Leader rounds: " << metrics.leader_rounds << "\n";
                cout << "Leader time: " << metrics.leader_time << " seconds\n";
            }
        }

        if (algo == "dijkstra" || algo == "both") {
            if (rank == 0) cout << "\nRunning Dijkstra...\n";
            vector<int> dist = run_dijkstra(g, p, rank, source, metrics, logf);
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == 0) {
                cout << "Distances from source " << source << ":\n";
                for (int i = 0; i < g.n; ++i) cout << i << " -> " << dist[i] << "\n";
                cout << "Dijkstra iterations: " << metrics.dijkstra_iterations << "\n";
                cout << "Dijkstra time: " << metrics.dijkstra_time << " seconds\n";
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == 0) {
            cout << "\nMetrics summary:\n";
            cout << "Messages sent (logical count): " << metrics.messages_sent << "\n";
            cout << "Bytes sent (approx): " << metrics.bytes_sent << "\n";
        }
    } catch (const exception& ex) {
        if (rank == 0) cerr << "ERROR: " << ex.what() << "\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    MPI_Finalize();
    return 0;
}