#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def mod_owner(node_id, ranks):
    return node_id % ranks


def block_owner(node_id, n, ranks):
    base = n // ranks
    rem = n % ranks
    start = 0
    for r in range(ranks):
        size = base + (1 if r < rem else 0)
        if start <= node_id < start + size:
            return r
        start += size
    raise RuntimeError("block_owner failed")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("graph", type=str)
    parser.add_argument("--ranks", type=int, required=True)
    parser.add_argument("--strategy", choices=["mod", "block"], default="mod")
    parser.add_argument("--out", type=str, required=True)
    args = parser.parse_args()

    with open(args.graph, "r") as f:
        graph = json.load(f)

    n = graph["num_nodes"]
    owner = {}

    for i in range(n):
        owner[str(i)] = (
            mod_owner(i, args.ranks)
            if args.strategy == "mod"
            else block_owner(i, n, args.ranks)
        )

    for i in range(n):
        r = owner[str(i)]
        if r < 0 or r >= args.ranks:
            raise SystemExit(f"invalid owner rank {r} for node {i}")

    local_nodes = {str(r): [] for r in range(args.ranks)}
    ghost_nodes = {str(r): set() for r in range(args.ranks)}
    cut_edges = 0

    for i in range(n):
        local_nodes[str(owner[str(i)])].append(i)

    for e in graph["edges"]:
        u = e["u"]
        v = e["v"]
        ru = owner[str(u)]
        rv = owner[str(v)]
        if ru != rv:
            cut_edges += 1
            ghost_nodes[str(ru)].add(v)
            ghost_nodes[str(rv)].add(u)

    part = {
        "num_ranks": args.ranks,
        "strategy": args.strategy,
        "owner": owner,
        "local_nodes": local_nodes,
        "ghost_nodes": {k: sorted(list(v)) for k, v in ghost_nodes.items()},
        "cut_edges": cut_edges,
    }

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w") as f:
        json.dump(part, f, indent=2)

    print(f"Wrote partition to {out_path}")
    print(f"strategy={args.strategy} cut_edges={cut_edges}")


if __name__ == "__main__":
    main()