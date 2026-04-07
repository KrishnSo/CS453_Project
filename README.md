
# CS453 Distributed MPI Project

## Overview
This project implements a distributed graph processing pipeline using MPI.

Pipeline:
- Graph generation using a seed-based exporter
- Graph enrichment with positive edge weights
- Graph partitioning across MPI ranks
- Distributed leader election using FloodMax
- Distributed shortest path computation using Dijkstra

The project follows the required end-to-end workflow:
1. Generate a connected weighted graph
2. Partition the graph across ranks
3. Run distributed MPI algorithms
4. Collect metrics and compare experiments

## Project Structure

CS453_Spring2026/
├── netgamesim/                  # Upstream NetGameSim repository
├── tools/
│   ├── graph_export/
│   │   ├── export_graph.py      # Graph generation/export script
│   │   └── run.sh               # Wrapper script for export
│   └── partition/
│       ├── partition.py         # Partitioning tool
│       └── run.sh               # Wrapper script for partitioning
├── mpi_runtime/
│   ├── include/
│   │   └── nlohmann/json.hpp    # JSON header
│   ├── src/
│   │   └── main.cpp             # MPI runtime
│   └── CMakeLists.txt           # MPI runtime build file
├── configs/
│   ├── small.json               # Small graph config
│   └── medium.json              # Medium graph config
├── outputs/
│   ├── graph_small.json
│   ├── graph_medium.json
│   ├── part_small_r4_mod.json
│   ├── part_small_r4_block.json
│   ├── part_medium_r4_mod.json
│   └── logs/
├── archive/
│   └── prototype_main.cpp
├── README.md
└── REPORT.md

## Requirements
- macOS or Linux
- OpenMPI
- CMake
- Python 3
- C++17 compiler

## Installation

### Install OpenMPI
```bash
brew install open-mpi
````

### Install CMake

```bash
brew install cmake
```

## Build Instructions

Build the MPI runtime from the command line:

```bash
cmake -S mpi_runtime -B build
cmake --build build
```

## Graph Generation

Generate reproducible graphs using the provided configs:

### Small graph

```bash
./tools/graph_export/run.sh configs/small.json outputs/graph_small.json
```

### Medium graph

```bash
./tools/graph_export/run.sh configs/medium.json outputs/graph_medium.json
```

The graph export step:

* generates a connected graph
* assigns positive weights
* saves the random seed
* writes output as normalized JSON

## Partitioning

Partition graphs across MPI ranks using one of two strategies.

### Mod partition

```bash
./tools/partition/run.sh outputs/graph_small.json --ranks 4 --strategy mod --out outputs/part_small_r4_mod.json
```

### Block partition

```bash
./tools/partition/run.sh outputs/graph_small.json --ranks 4 --strategy block --out outputs/part_small_r4_block.json
```

### Medium graph partition

```bash
./tools/partition/run.sh outputs/graph_medium.json --ranks 4 --strategy mod --out outputs/part_medium_r4_mod.json
```

The partitioning tool:

* assigns each node to exactly one rank
* emits an ownership map
* records local nodes per rank
* computes cut edges

## Running the MPI Runtime

### Run both algorithms on small graph with mod partition

```bash
mpirun -n 4 ./build/ngs_mpi --graph outputs/graph_small.json --part outputs/part_small_r4_mod.json --algo both --source 0
```

### Run both algorithms on small graph with block partition

```bash
mpirun -n 4 ./build/ngs_mpi --graph outputs/graph_small.json --part outputs/part_small_r4_block.json --algo both --source 0
```

### Run both algorithms on medium graph with mod partition

```bash
mpirun -n 4 ./build/ngs_mpi --graph outputs/graph_medium.json --part outputs/part_medium_r4_mod.json --algo both --source 0
```

### Run only leader election

```bash
mpirun -n 4 ./build/ngs_mpi --graph outputs/graph_small.json --part outputs/part_small_r4_mod.json --algo leader --source 0
```

### Run only Dijkstra

```bash
mpirun -n 4 ./build/ngs_mpi --graph outputs/graph_small.json --part outputs/part_small_r4_mod.json --algo dijkstra --source 0
```

## Algorithms

### Leader Election

Leader election is implemented using a FloodMax-style approach.

* each node begins with its own ID
* nodes propagate candidate IDs
* the global maximum node ID becomes the leader
* convergence is measured in rounds

### Distributed Dijkstra

Dijkstra is implemented as a distributed baseline using MPI collectives.

* each rank proposes its local minimum unsettled node
* global minimum is chosen using MPI_Allreduce with MPI_MINLOC
* edges are relaxed from the chosen node
* distances are synchronized across ranks

## Metrics Collected

The runtime reports:

* leader election rounds
* leader election runtime
* Dijkstra iterations
* Dijkstra runtime
* logical message count
* approximate bytes sent

## Experiments

### Experiment 1: Partition Strategy Comparison

Compared:

* mod partition
* block partition

Observation:

* block partition produced fewer cut edges on the small graph
* fewer cut edges generally reduce communication overhead

### Experiment 2: Graph Size Scaling

Compared:

* small graph (8 nodes)
* medium graph (16 nodes)

Observation:

* larger graph increased iterations, message count, and bytes sent
* runtime also increased with graph size

## Example Output

```text
Graph loaded: 8 nodes
Partition loaded: 4 ranks

Running Leader Election...
Leader: 7
Leader rounds: 4
Leader time: 1.8e-05 seconds

Running Dijkstra...
Distances from source 0:
0 -> 0
1 -> 14
2 -> 15
3 -> 21
4 -> 33
5 -> 31
6 -> 23
7 -> 11
Dijkstra iterations: 8
Dijkstra time: 0.00197 seconds

Metrics summary:
Messages sent (logical count): 20
Bytes sent (approx): 448
```

## Assumptions

* graph is connected
* edge weights are positive
* node IDs are in the range 0 to N-1
* partition size matches the graph node count
* number of MPI ranks matches the partition file

## Notes on NetGameSim

The project includes the NetGameSim repository as the upstream graph-generation platform.
The current workflow uses a graph export tool to produce normalized JSON graphs for the MPI runtime.

## Reproducibility

Reproducibility is supported through:

* fixed config files
* explicit random seeds
* saved generated graph outputs
* saved partition outputs
* command-line build and run instructions

## Author

Krishna Somarapu
CS453 Spring 2026

```
```
