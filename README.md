# CS453 Distributed MPI Project (NetGameSim → MPI)

This project implements a full pipeline: **synthetic or NetGameSim-style graphs** → **weighted JSON** → **partition with ghost nodes** → **MPI FloodMax leader election** and **distributed Dijkstra**, with metrics and experiments.

Upstream: [NetGameSim](https://github.com/0x1DOCD00D/NetGameSim) (course walkthrough: [YouTube](https://www.youtube.com/watch?v=6fdazJBkdjA&t=2658s)).

## Requirements

- macOS or Linux
- **OpenMPI** (e.g. `brew install open-mpi`)
- **CMake** 3.10+
- **Python 3**
- C++17 compiler

## Project layout

- `netgamesim/` — upstream NetGameSim (clone or submodule; keep intact)
- `tools/graph_export/` — graph generation / NetGameSim JSON import
- `tools/partition/` — partitioners (`mod`, `block`) with owner + ghost metadata
- `mpi_runtime/` — C++ MPI runtime (`ngs_mpi`)
- `configs/` — JSON configs for graphs and import helper
- `outputs/` — graphs, partitions, logs, experiment captures, summaries
- `experiments/` — reproducible experiment scripts
- `REPORT.md` — design and results
- `student.txt` — student identity line

## Build (command line)

```bash
cd ~/Desktop/CS453_Spring2026
cmake -S mpi_runtime -B build
cmake --build build
```

## Synthetic graph workflow

```bash
./tools/graph_export/run.sh configs/small.json outputs/graphs/graph_small.json
./tools/partition/run.sh outputs/graphs/graph_small.json --ranks 4 --strategy mod --out outputs/partitions/part_small_r4_mod.json
mpirun -n 4 ./build/ngs_mpi --graph outputs/graphs/graph_small.json --part outputs/partitions/part_small_r4_mod.json --algo both --source 0
```

## NetGameSim import workflow

```bash
python3 tools/graph_export/export_graph.py \
  --raw-netgamesim outputs/raw/raw_netgamesim_graph.json \
  --seed 12345 \
  --out outputs/graphs/graph_from_netgamesim.json
./tools/partition/run.sh outputs/graphs/graph_from_netgamesim.json --ranks 4 --strategy mod --out outputs/partitions/part_netgamesim_r4_mod.json
mpirun -n 4 ./build/ngs_mpi --graph outputs/graphs/graph_from_netgamesim.json --part outputs/partitions/part_netgamesim_r4_mod.json --algo both --source 0
```

Or use the helper config:

```bash
./tools/graph_export/run.sh configs/netgamesim_import.json outputs/graphs/graph_from_netgamesim.json import
```

## MPI runtime CLI

```bash
./build/ngs_mpi --graph <graph.json> --part <part.json> [--algo leader|dijkstra|both] [--source N] [--rounds R] [--seed S]
```

- `--rounds` — leader-election cap (default `0` = run until convergence)
- `--seed` — optional; echoed to logs for reproducibility

## Experiments

```bash
./experiments/run_partition_compare.sh
./experiments/run_scale_compare.sh
```

Logs: `outputs/experiment_logs/` · Summaries: `outputs/summaries/`

## Tests

```bash
./tests/run_tests.sh
# or: python3 -m unittest discover -s tests -v
```

Includes unit tests for export/partition and a reference Dijkstra check; optional MPI smoke test if `build/ngs_mpi` and `mpirun` are available.

## Logging

Per-rank logs: `outputs/logs/runtime_rank<R>.log` (append mode).

## Assumptions (correctness)

- Graph is **connected**
- Edge weights are **strictly positive** (Dijkstra)
- Node IDs are **0 … N−1** in normalized JSON
- `num_ranks` in the partition file **matches** `mpirun -n`

## macOS / OpenMPI note

If `mpirun` fails with interface or PMIx errors, try:

```bash
mpirun --mca btl tcp,self --mca pml ob1 -n 4 ./build/ngs_mpi ...
```

## Author

See `student.txt`.
