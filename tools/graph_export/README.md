# Graph export

Produces normalized JSON graphs for the MPI runtime.

## Modes

### Synthetic (`export_graph.py --synthetic`)

Builds a connected graph with a spanning path plus random extra edges, assigns uniform random weights in `[1, 20]`, and records the seed.

### NetGameSim import (`export_graph.py --raw-netgamesim <file>`)

Loads JSON that uses `fromNode.id` / `toNode.id` edges (NetGameSim-style). Node IDs are remapped to `0..N-1`. Weights are assigned from the given seed.

## Output format

- `num_nodes`, `nodes` (each `id` in `0..N-1`), `edges` (`u`, `v`, `w`)
- `seed`, `source_format` (`synthetic` or `netgamesim_json`)

## Wrapper

```bash
./tools/graph_export/run.sh configs/small.json outputs/graphs/graph_small.json
./tools/graph_export/run.sh configs/netgamesim_import.json outputs/graphs/graph_from_netgamesim.json import
```

Direct Python:

```bash
python3 tools/graph_export/export_graph.py --raw-netgamesim outputs/raw/raw_netgamesim_graph.json --seed 12345 --out outputs/graphs/graph_from_netgamesim.json
```
