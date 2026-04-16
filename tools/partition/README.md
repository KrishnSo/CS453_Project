# Partition tool

Reads a normalized graph JSON and assigns each node to exactly one MPI rank.

## Strategies

- **mod**: `owner(node) = node % num_ranks` — simple, often spreads endpoints of low-ID edges across ranks.
- **block**: contiguous ID ranges per rank — often fewer cut edges on graphs where locality follows ID order.

## Output fields

- `owner`: map from node id string to rank id
- `local_nodes`: nodes owned by each rank
- `ghost_nodes`: remote neighbors of each rank’s owned nodes (needed for cross-partition edges)
- `cut_edges`: number of undirected edges whose endpoints lie on different ranks

## Example

```bash
./tools/partition/run.sh outputs/graphs/graph_small.json --ranks 4 --strategy mod --out outputs/partitions/part_small_r4_mod.json
```
