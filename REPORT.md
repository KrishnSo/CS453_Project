# CS453 Project Report

## Design Overview
The system represents a graph distributed across MPI ranks. Nodes are assigned using a modulo partitioning strategy (node_id % number_of_ranks).

Each rank maintains its local nodes and communicates updates using MPI collective operations.

## Leader Election
We implemented the FloodMax algorithm.

- Each node starts with its own ID
- Nodes exchange candidate IDs with neighbors
- The maximum ID propagates across the graph
- All nodes converge to the same leader

MPI_Allreduce is used to synchronize leader updates across ranks.

## Distributed Dijkstra
We implemented a distributed version of Dijkstra’s algorithm.

- Each rank maintains local distances
- A global minimum node is selected using MPI_MINLOC
- Distances are updated based on edge relaxation
- MPI_Allreduce is used to synchronize distances

## Experiments
The system was tested with:
- 1 MPI rank
- 2 MPI ranks
- 4 MPI ranks

All runs produced consistent results.

## Observations
- Increasing the number of ranks distributes computation
- Communication overhead increases due to MPI collectives
- Correctness is preserved across partitions

## Assumptions
- The graph is connected
- All edge weights are positive
- All MPI ranks execute collectives in the same order