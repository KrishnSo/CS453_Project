#!/usr/bin/env python3
import argparse
import json
import random
from collections import deque
from pathlib import Path


def normalize_edges(edges, directed=False):
    seen = set()
    out = []
    for u, v, w in edges:
        if u == v:
            continue
        key = (u, v) if directed else tuple(sorted((u, v)))
        if key in seen:
            continue
        seen.add(key)
        out.append({"u": u, "v": v, "w": int(w)})
    return out


def connected(n, edges):
    adj = [[] for _ in range(n)]
    for e in edges:
        u, v = e["u"], e["v"]
        adj[u].append(v)
        adj[v].append(u)

    q = deque([0])
    vis = [False] * n
    vis[0] = True
    while q:
        u = q.popleft()
        for v in adj[u]:
            if not vis[v]:
                vis[v] = True
                q.append(v)
    return all(vis)


def build_synthetic_graph(n, extra_edges, seed):
    rng = random.Random(seed)
    raw = []

    # guaranteed connected backbone
    for i in range(n - 1):
        raw.append((i, i + 1, rng.randint(1, 20)))

    # extra random edges
    attempts = 0
    while extra_edges > 0 and attempts < n * n * 10:
        u = rng.randrange(n)
        v = rng.randrange(n)
        attempts += 1
        if u == v:
            continue
        raw.append((u, v, rng.randint(1, 20)))
        extra_edges -= 1

    edges = normalize_edges(raw, directed=False)
    if not connected(n, edges):
        raise RuntimeError("Synthetic graph unexpectedly not connected")
    return {
        "seed": seed,
        "num_nodes": n,
        "nodes": [{"id": i} for i in range(n)],
        "edges": edges,
    }


def build_from_netgamesim(raw_path, seed):
    with open(raw_path, "r") as f:
        data = json.load(f)

    # Flexible extraction because raw NetGameSim structure may vary
    raw_nodes = data.get("nodes", [])
    raw_edges = data.get("edges", [])

    node_ids = set()
    for node in raw_nodes:
        if isinstance(node, dict) and "id" in node:
            node_ids.add(int(node["id"]))

    raw = []
    rng = random.Random(seed)

    for edge in raw_edges:
        # Prefer fromNode/toNode IDs as discussed in class threads
        if "fromNode" in edge and "toNode" in edge:
            u = int(edge["fromNode"]["id"])
            v = int(edge["toNode"]["id"])
        else:
            # fallback if a different raw structure appears
            u = int(edge["u"])
            v = int(edge["v"])

        node_ids.add(u)
        node_ids.add(v)
        w = rng.randint(1, 20)
        raw.append((u, v, w))

    # Remap node ids to 0..N-1
    sorted_ids = sorted(node_ids)
    remap = {old: new for new, old in enumerate(sorted_ids)}

    edges = normalize_edges(
        [(remap[u], remap[v], w) for (u, v, w) in raw],
        directed=False,
    )

    n = len(sorted_ids)
    if n == 0:
        raise RuntimeError("No nodes found in NetGameSim JSON")
    if not connected(n, edges):
        raise RuntimeError("Normalized graph is not connected")

    return {
        "seed": seed,
        "num_nodes": n,
        "nodes": [{"id": i} for i in range(n)],
        "edges": edges,
        "source_format": "netgamesim_json",
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--synthetic", action="store_true")
    parser.add_argument("--raw-netgamesim", type=str, default="")
    parser.add_argument("--nodes", type=int, default=8)
    parser.add_argument("--extra-edges", type=int, default=6)
    parser.add_argument("--seed", type=int, required=True)
    parser.add_argument("--out", type=str, required=True)
    args = parser.parse_args()

    if args.synthetic:
        graph = build_synthetic_graph(args.nodes, args.extra_edges, args.seed)
    elif args.raw_netgamesim:
        graph = build_from_netgamesim(args.raw_netgamesim, args.seed)
    else:
        raise SystemExit("Use either --synthetic or --raw-netgamesim <file>")

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w") as f:
        json.dump(graph, f, indent=2)

    print(f"Wrote graph to {out_path}")
    print(f"seed={graph['seed']} num_nodes={graph['num_nodes']} edges={len(graph['edges'])}")


if __name__ == "__main__":
    main()