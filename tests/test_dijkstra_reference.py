"""Reference Dijkstra to validate exported graphs (single-process)."""
import heapq
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def dijkstra(adj, n, source):
    dist = [10**18] * n
    dist[source] = 0
    pq = [(0, source)]
    while pq:
        d, u = heapq.heappop(pq)
        if d != dist[u]:
            continue
        for v, w in adj[u]:
            nd = d + w
            if nd < dist[v]:
                dist[v] = nd
                heapq.heappush(pq, (nd, v))
    return dist


class TestDijkstraReference(unittest.TestCase):
    def test_small_graph_matches_known_seed(self):
        g = json.loads((ROOT / "outputs/graphs/graph_small.json").read_text())
        n = g["num_nodes"]
        adj = [[] for _ in range(n)]
        for e in g["edges"]:
            u, v, w = e["u"], e["v"], e["w"]
            adj[u].append((v, w))
            adj[v].append((u, w))
        d = dijkstra(adj, n, 0)
        self.assertEqual(d, [0, 14, 15, 21, 33, 31, 23, 11])


if __name__ == "__main__":
    unittest.main()
