#include <mpi.h>
#include <iostream>
#include <vector>
#include <limits>

using namespace std;

struct Node {
    int id;
    vector<pair<int,int>> neighbors;
};

vector<Node> create_graph() {
    vector<Node> g(6);
    for(int i = 0; i < 6; i++) g[i].id = i;

    g[0].neighbors = {{1,1}, {2,4}};
    g[1].neighbors = {{0,1}, {3,2}};
    g[2].neighbors = {{0,4}, {3,1}};
    g[3].neighbors = {{1,2}, {2,1}, {4,3}};
    g[4].neighbors = {{3,3}, {5,2}};
    g[5].neighbors = {{4,2}};

    return g;
}

vector<Node> get_local_nodes(vector<Node>& graph, int rank, int size) {
    vector<Node> local;
    for(auto &n : graph) {
        if(n.id % size == rank) {
            local.push_back(n);
        }
    }
    return local;
}

void leader_election(vector<Node>& graph, vector<Node>& local_nodes, int rank) {
    vector<int> leader(6);
    for(int i = 0; i < 6; i++) leader[i] = i;

    for(int round = 0; round < 5; round++) {
        vector<int> new_leader = leader;

        for(auto &n : local_nodes) {
            for(auto &e : n.neighbors) {
                int neighbor = e.first;
                new_leader[n.id] = max(new_leader[n.id], leader[neighbor]);
            }
        }

        MPI_Allreduce(new_leader.data(), leader.data(), 6,
                      MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    }

    if(rank == 0) {
        cout << "Leader: " << leader[0] << endl;
    }
}

void dijkstra(vector<Node>& graph, vector<Node>& local_nodes, int rank) {
    const int INF = 1e9;
    vector<int> dist(6, INF);
    vector<bool> visited(6, false);

    dist[0] = 0;

    for(int i = 0; i < 6; i++) {

        int local_min = INF;
        int local_node = -1;

        for(auto &n : local_nodes) {
            if(!visited[n.id] && dist[n.id] < local_min) {
                local_min = dist[n.id];
                local_node = n.id;
            }
        }

        struct {
            int dist;
            int node;
        } local_pair = {local_min, local_node}, global_pair;

        MPI_Allreduce(&local_pair, &global_pair, 1,
                      MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);

        int u = global_pair.node;
        if(u == -1) break;

        visited[u] = true;

        for(auto &e : graph[u].neighbors) {
            int v = e.first;
            int w = e.second;

            if(dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
            }
        }

        MPI_Allreduce(MPI_IN_PLACE, dist.data(), 6,
                      MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    }

    if(rank == 0) {
        cout << "Distances:\n";
        for(int i = 0; i < 6; i++) {
            cout << i << " -> " << dist[i] << endl;
        }
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    auto graph = create_graph();
    auto local_nodes = get_local_nodes(graph, rank, size);

    if(rank == 0) cout << "Running Leader Election...\n";
    leader_election(graph, local_nodes, rank);

    if(rank == 0) cout << "\nRunning Dijkstra...\n";
    dijkstra(graph, local_nodes, rank);

    MPI_Finalize();
    return 0;
}