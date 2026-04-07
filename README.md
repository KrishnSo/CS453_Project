# CS453 Distributed MPI Project

## Build
mpicxx src/main.cpp -o run

## Run
mpirun -n 1 ./run
mpirun -n 2 ./run
mpirun -n 4 ./run

## Description
This project implements distributed graph algorithms using MPI.

- Graph is partitioned across MPI ranks using node_id % size
- Leader election implemented using FloodMax algorithm
- Distributed Dijkstra shortest path algorithm implemented using MPI_Allreduce

## Algorithms

### Leader Election
Each node starts with its own ID and exchanges values with neighbors.
The maximum ID propagates through the graph until all nodes agree.

### Distributed Dijkstra
Each rank computes local minimum distances.
Global minimum is selected using MPI_MINLOC.
Distances are updated and synchronized across all ranks.

## Assumptions
- Graph is connected
- Edge weights are positive
- All ranks participate in MPI collectives in the same order

## Output
- Leader node ID
- Shortest distances from source node 0