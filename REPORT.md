# CS453 Project Report

## Approach / overall idea

We treat **graph nodes as logical entities** and **MPI ranks as partitions** that own subsets of nodes. Cross-partition edges require explicit inter-rank messaging (or equivalent global synchronization). The pipeline is: **generate/import → enrich with weights → partition → MPI algorithms → metrics**.

## Implementation details

### Graph export

- **Synthetic mode:** builds a connected graph (spanning path plus random extra edges), assigns uniform positive weights, records `seed` and `source_format`.
- **NetGameSim import:** reads `fromNode.id` / `toNode.id` edges, remaps to `0..N-1`, assigns weights from a seeded RNG.

### Partitioning

- **mod:** `node % ranks` — simple balance.
- **block:** contiguous ID ranges — often fewer cut edges when ID correlates with locality.

Each rank receives `owner`, `local_nodes`, and `ghost_nodes` (remote endpoints of cut edges).

### Leader election (FloodMax-style)

Each node initializes its candidate to its node ID. Each round, each owned node sets its candidate to the maximum of its own and neighbors’ candidates (from the replicated global candidate vector). We use **`MPI_Allreduce(MPI_MAX)`** on the full candidate vector so every rank shares a consistent view of all nodes, avoiding pairwise ordering deadlocks while preserving synchronous round semantics. Convergence is detected when **no owned node changes** in a round (`MPI_Allreduce` on a change flag). Final **agreement** is checked via global min/max on the candidate array.

### Distributed Dijkstra

Baseline **parallel Dijkstra**: each rank proposes its best local unsettled distance (`MPI_Allreduce` with `MPI_MINLOC` on the `(distance, node)` pair). The owning rank relaxes outgoing edges; updates to remote nodes are sent with point-to-point messages. Distances are reconciled with **`MPI_Allreduce(MPI_MIN)`** on the full distance vector each iteration.

### Ghost nodes

In the partition JSON, **ghost nodes** list remote neighbors of locally owned nodes. The runtime uses this metadata for documentation and consistency with the assignment; the current leader election uses global Allreduce for candidate sync; Dijkstra uses explicit messages for remote relaxations.

## Experimental hypothesis

1. **Partitioning:** For the same graph, **block** partitioning often reduces **cut edges** versus **mod**, which should reduce cross-rank messages.
2. **Scaling:** A **larger** graph (medium vs small) should increase Dijkstra iterations and total communication volume.

## Expected results

- Mod vs block: different `cut_edges` counts; block may show lower cross-rank traffic on small graphs with ID locality.
- Small vs medium: higher iteration counts and message counts on the medium graph.

## Actual results

Run the experiment scripts and inspect:

- `outputs/experiment_logs/partition_compare.txt`
- `outputs/experiment_logs/scale_compare.txt`
- `outputs/summaries/partition_compare_summary.txt`
- `outputs/summaries/scale_compare_summary.txt`

(Exact numbers depend on your machine and OpenMPI build.)

## Explanation of results

### Partition strategy

- **Mod** spreads consecutive IDs across ranks; many edges connect nearby IDs, so more edges are cut.
- **Block** keeps consecutive IDs together, reducing cuts when edges are local in ID space.

### Communication costs

- **Leader election:** dominated by per-round `MPI_Allreduce` on a length-`N` vector plus a small change flag.
- **Dijkstra:** each iteration uses `MPI_MINLOC` + point-to-point updates + `MPI_Allreduce` on distances — collectives dominate as `N` grows or ranks increase.

### Small vs medium graphs

- More nodes increase Dijkstra iterations (up to one settlement per node) and generally increase message volume.

## Specific decisions / insights

- **Global Allreduce for FloodMax candidate state** trades decentralized purity for **deadlock-free correctness** and simpler grading.
- **MPI_MINLOC** for global frontier selection matches the course’s suggested MPI baseline.
- **Positive weights** are required for Dijkstra; **connectivity** is required for a unique leader and well-defined shortest paths.

## Limitations

- Algorithms are not fully decentralized; collectives are used for correctness and simplicity.
- Distance accumulation uses `int`; extremely large weights/paths could overflow (not exercised in our configs).
