# CS453 Project Report

## System Design

The project implements a distributed graph processing system using MPI.

Pipeline:
1. Graph generation using seed-based approach
2. Graph enrichment with positive edge weights
3. Graph partitioning across MPI ranks
4. Distributed execution of algorithms

Graph data is stored in JSON format and loaded by the MPI runtime.

---

## Leader Election

Algorithm: FloodMax

- Each node starts with its own ID
- Nodes exchange candidate IDs with neighbors
- Maximum ID propagates across graph
- Convergence reached when no updates occur

This guarantees agreement on a single leader.

---

## Distributed Dijkstra

- Each rank maintains distances for its nodes
- Each iteration:
  - Rank proposes local minimum
  - Global minimum selected using MPI_Allreduce (MINLOC)
  - Edges relaxed
- Continues until all nodes processed

---

## Partitioning Strategy

Two strategies implemented:

### Mod Partition
- Node assigned by: node_id % ranks
- Simple and balanced

### Block Partition
- Nodes assigned in contiguous ranges
- Fewer edge cuts in some cases

---

## Experiments

### Experiment 1: Partition Strategy
- Compared mod vs block
- Block partition reduced cross-rank edges

### Experiment 2: Graph Size
- Small graph (8 nodes)
- Medium graph (16 nodes)
- Observed increase in:
  - message count
  - runtime

---

## Metrics

Collected during execution:

- Leader rounds
- Dijkstra iterations
- Runtime (seconds)
- Message count
- Bytes sent

---

## Observations

- Communication dominates performance
- Partition strategy impacts efficiency
- Global synchronization simplifies correctness but increases overhead

---

## Limitations

- Uses global MPI_Allreduce (not fully decentralized)
- Does not implement ghost-node optimization

---

## Conclusion

The project successfully implements a distributed system pipeline with MPI, including graph processing and distributed algorithms with measurable performance metrics.