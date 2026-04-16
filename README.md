# CS453 Spring 2026 Project
## NetGameSim to MPI Distributed Algorithms

Name: Krishna Somarapu (`ksomar3`)

This project takes generated network graphs and runs two distributed algorithms using MPI:
1. Leader election (using the FloodMax style)
2. Shortest paths from a source (using the distributed Dijkstra baseline)

The full workflow is as:

`Graph generation/import -> graph enrichment (positive weights + IDs) -> partition across MPI ranks -> distributed algorithm run -> logs + metrics + experiment summary`

---

## Upstream attribution

This project builds on NetGameSim from Professor Grechanik:
- The initial NetGameSim repo: <https://github.com/0x1DOCD00D/NetGameSim>
- I referred the Walkthrough video: <https://www.youtube.com/watch?v=6fdazJBkdjA&t=2658s>

---

## Requirements

- macOS or Linux
- OpenMPI
- CMake (3.10+)
- Python 3
- C++17 compiler

If needed on macOS - I use mac so this helps:
```bash
brew install open-mpi cmake
```

---

## Repository layout

```
CS453_Spring2026/
├── netgamesim/
├── configs/
├── tools/
│   ├── graph_export/
│   └── partition/
├── mpi_runtime/
├── experiments/
├── outputs/
├── tests/
├── README.md
├── REPORT.md
└── student.txt
```

---

## Build instructions

Download the file and move it to Desktop for easier access
Basically clone the repository and build from the root directory:
Name the file CS453_Spring2026

```bash
cd ~/Desktop/CS453_Spring2026
cmake -S mpi_runtime -B build
cmake --build build
```

This builds the runtime executable at `build/ngs_mpi`.

---

## 1) Generate graph (synthetic path)

Small graph:
```bash
./tools/graph_export/run.sh configs/small.json outputs/graphs/graph_small.json
```

Medium graph:
```bash
./tools/graph_export/run.sh configs/medium.json outputs/graphs/graph_medium.json
```

What this does is:
- creates a connected graph
- assigns positive edge weights
- stores random seed in output JSON
- normalizes node IDs to `0..N-1`

---

## 2) Generate graph (The NetGameSim style import path)

```bash
python3 tools/graph_export/export_graph.py \
  --raw-netgamesim outputs/raw/raw_netgamesim_graph.json \
  --seed 12345 \
  --out outputs/graphs/graph_from_netgamesim.json
```

Or using config wrapper:
```bash
./tools/graph_export/run.sh configs/netgamesim_import.json outputs/graphs/graph_from_netgamesim.json import
```

---

## 3) Partition graph across ranks

Mod partition:
```bash
./tools/partition/run.sh outputs/graphs/graph_small.json --ranks 4 --strategy mod --out outputs/partitions/part_small_r4_mod.json
```

Block partition:
```bash
./tools/partition/run.sh outputs/graphs/graph_small.json --ranks 4 --strategy block --out outputs/partitions/part_small_r4_block.json
```

Partition output includes:
- `owner` map
- `local_nodes` per rank
- `ghost_nodes` per rank
- `cut_edges`

---

## 4) Run MPI runtime

Run both algorithms:
```bash
mpirun -n 4 ./build/ngs_mpi --graph outputs/graphs/graph_small.json --part outputs/partitions/part_small_r4_mod.json --algo both --source 0
```

Leader election only:
```bash
mpirun -n 4 ./build/ngs_mpi --graph outputs/graphs/graph_small.json --part outputs/partitions/part_small_r4_mod.json --algo leader --rounds 200
```

Dijkstra only:
```bash
mpirun -n 4 ./build/ngs_mpi --graph outputs/graphs/graph_small.json --part outputs/partitions/part_small_r4_mod.json --algo dijkstra --source 0
```

Optional args:
- `--rounds R` max rounds for leader election (`0` = until convergence)
- `--seed S` records run seed in logs for reproducibility

---

## 5) Run experiments

Experiment 1 (partition strategy comparison):
```bash
./experiments/run_partition_compare.sh
```

Experiment 2 (small vs medium scaling):
```bash
./experiments/run_scale_compare.sh
```

Outputs:
- raw experiment logs: `outputs/experiment_logs/`
- summaries: `outputs/summaries/`

---

## Tests

Run all tests:
```bash
./tests/run_tests.sh
```

This runs:
- export tool tests
- partition tool tests
- reference Dijkstra correctness test
- MPI smoke test (when `mpirun` and build are available)

---

## Logging and metrics

Per rank runtime logs are as follows:
- `outputs/logs/runtime_rank0.log`
- `outputs/logs/runtime_rank1.log`
- ...

Runtime metrics would include:
- leader rounds
- dijkstra iterations
- runtime per algorithm
- total message count (logical)
- approximate bytes sent

---

## Reproducibility

All graph generation is seed based. The seed used for each graph is stored in the output JSON, allowing exact reproduction of experiments. Runtime seeds can also be passed via CLI.

---

## Assumptions 

- Graph is connected
- Edge weights are strictly positive
- Node IDs are normalized to `0..N-1`
- Partition rank count matches `mpirun -n`

---

## You can use this as a very quick example for test or use cases

```bash
cd ~/Desktop/CS453_Spring2026
cmake -S mpi_runtime -B build && cmake --build build
./tools/graph_export/run.sh configs/small.json outputs/graphs/graph_small.json
./tools/partition/run.sh outputs/graphs/graph_small.json --ranks 4 --strategy mod --out outputs/partitions/part_small_r4_mod.json
mpirun -n 4 ./build/ngs_mpi --graph outputs/graphs/graph_small.json --part outputs/partitions/part_small_r4_mod.json --algo both --source 0
```

If the openmpi complains on mac network interface then please use:
```bash
mpirun --mca btl tcp,self --mca pml ob1 -n 4 ./build/ngs_mpi ...
```
