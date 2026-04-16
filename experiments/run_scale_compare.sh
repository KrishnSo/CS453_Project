#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

./tools/graph_export/run.sh configs/small.json outputs/graph_small.json
./tools/partition/run.sh outputs/graph_small.json --ranks 4 --strategy mod --out outputs/part_small_r4_mod.json
mpirun -n 4 ./build/ngs_mpi --graph outputs/graph_small.json --part outputs/part_small_r4_mod.json --algo both --source 0

./tools/graph_export/run.sh configs/medium.json outputs/graph_medium.json
./tools/partition/run.sh outputs/graph_medium.json --ranks 4 --strategy mod --out outputs/part_medium_r4_mod.json
mpirun -n 4 ./build/ngs_mpi --graph outputs/graph_medium.json --part outputs/part_medium_r4_mod.json --algo both --source 0