#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

LOGDIR="outputs/experiment_logs"
mkdir -p "$LOGDIR" outputs/summaries

OUT="$LOGDIR/partition_compare.txt"
{
  echo "=== Experiment 1: mod vs block partition (same graph) ==="
  ./tools/graph_export/run.sh configs/small.json outputs/graphs/graph_small.json
  ./tools/partition/run.sh outputs/graphs/graph_small.json --ranks 4 --strategy mod --out outputs/partitions/part_small_r4_mod.json
  ./tools/partition/run.sh outputs/graphs/graph_small.json --ranks 4 --strategy block --out outputs/partitions/part_small_r4_block.json

  echo "--- mod partition run ---"
  mpirun -n 4 ./build/ngs_mpi --graph outputs/graphs/graph_small.json --part outputs/partitions/part_small_r4_mod.json --algo both --source 0

  echo "--- block partition run ---"
  mpirun -n 4 ./build/ngs_mpi --graph outputs/graphs/graph_small.json --part outputs/partitions/part_small_r4_block.json --algo both --source 0
} 2>&1 | tee "$OUT"

python3 experiments/summarize_results.py partition "$OUT" outputs/summaries/partition_compare_summary.txt
