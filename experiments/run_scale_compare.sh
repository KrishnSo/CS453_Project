#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

LOGDIR="outputs/experiment_logs"
mkdir -p "$LOGDIR" outputs/summaries

OUT="$LOGDIR/scale_compare.txt"
{
  echo "=== Experiment 2: small vs medium graph (mod partition, 4 ranks) ==="

  ./tools/graph_export/run.sh configs/small.json outputs/graphs/graph_small.json
  ./tools/partition/run.sh outputs/graphs/graph_small.json --ranks 4 --strategy mod --out outputs/partitions/part_small_r4_mod.json
  echo "--- small graph ---"
  mpirun -n 4 ./build/ngs_mpi --graph outputs/graphs/graph_small.json --part outputs/partitions/part_small_r4_mod.json --algo both --source 0

  ./tools/graph_export/run.sh configs/medium.json outputs/graphs/graph_medium.json
  ./tools/partition/run.sh outputs/graphs/graph_medium.json --ranks 4 --strategy mod --out outputs/partitions/part_medium_r4_mod.json
  echo "--- medium graph ---"
  mpirun -n 4 ./build/ngs_mpi --graph outputs/graphs/graph_medium.json --part outputs/partitions/part_medium_r4_mod.json --algo both --source 0
} 2>&1 | tee "$OUT"

python3 experiments/summarize_results.py scale "$OUT" outputs/summaries/scale_compare_summary.txt
