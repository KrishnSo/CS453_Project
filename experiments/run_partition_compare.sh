#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

./tools/graph_export/run.sh configs/small.json outputs/graph_small.json
./tools/partition/run.sh outputs/graph_small.json --ranks 4 --strategy mod --out outputs/part_small_r4_mod.json
./tools/partition/run.sh outputs/graph_small.json --ranks 4 --strategy block --out outputs/part_small_r4_block.json

mpirun -n 4 ./build/ngs_mpi --graph outputs/graph_small.json --part outputs/part_small_r4_mod.json --algo both --source 0
mpirun -n 4 ./build/ngs_mpi --graph outputs/graph_small.json --part outputs/part_small_r4_block.json --algo both --source 0