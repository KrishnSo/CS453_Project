# Experiments

## Experiment 1: `run_partition_compare.sh`

Uses one synthetic graph (`configs/small.json` → `outputs/graphs/graph_small.json`), builds **mod** and **block** partitions for 4 ranks, runs the MPI runtime with both, and logs to `outputs/experiment_logs/partition_compare.txt`. A short summary is written to `outputs/summaries/partition_compare_summary.txt`.

**Hypothesis:** block partitioning often reduces `cut_edges` versus mod on ID-ordered graphs, which lowers cross-rank traffic.

## Experiment 2: `run_scale_compare.sh`

Runs the same pipeline on **small** and **medium** configs with mod partitioning. Logs to `outputs/experiment_logs/scale_compare.txt` and summarizes to `outputs/summaries/scale_compare_summary.txt`.

**Hypothesis:** larger graphs increase Dijkstra iterations and message volume.

## Prerequisites

Build the runtime first:

```bash
cmake -S mpi_runtime -B build && cmake --build build
```

On some macOS setups you may need:

```bash
export OMP_NUM_THREADS=1
mpirun --mca btl tcp,self --mca pml ob1 -n 4 ./build/ngs_mpi ...
```
