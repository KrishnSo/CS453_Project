# CS453 Project Report
## NetGameSim to MPI Distributed Algorithms

Name: Krishna Somarapu (`ksomar3`)

## 1) Approach / Overall idea

The goal of this project was to build a full pipeline, not just isolated algorithm code. I used generated graph data (synthetic and NetGameSim-style JSON), enriched it with positive weights, partitioned nodes across MPI ranks, and then ran distributed leader election and shortest path computations.

A key design choice is that **nodes are not MPI ranks**. Each rank owns multiple nodes, which is closer to realistic distributed systems.

Pipeline used:

`generate/import graph -> assign weights + normalize IDs -> partition nodes -> run MPI algorithms -> collect metrics`

---

## 2) Implementation details

### 2.1 Graph generation and enrichment

`tools/graph_export/export_graph.py` supports two modes:

1. **Synthetic mode**
   - builds a connected graph (chain backbone + extra random edges)
   - assigns positive integer weights in `[1,20]`
   - records random seed

2. **NetGameSim-style import mode**
   - reads edges using `fromNode.id` / `toNode.id`
   - remaps node IDs to `0..N-1`
   - assigns seeded positive weights

Both modes output a normalized JSON format consumed by the MPI runtime.

### 2.2 Partitioning

`tools/partition/partition.py` supports:
- `mod` partition: `owner(node) = node % ranks`
- `block` partition: contiguous node ID ranges

Partition output includes:
- `owner` map
- `local_nodes`
- `ghost_nodes`
- `cut_edges`

`ghost_nodes` are remote neighbors of locally owned nodes, which matter for cross-rank communication.

### 2.3 MPI runtime structure

The runtime is modularized in `mpi_runtime/`:
- `graph.cpp` and `partition.cpp` for loading/validation
- `leader.cpp` for FloodMax election
- `dijkstra.cpp` for distributed Dijkstra baseline
- `metrics.cpp` and `logger.cpp` for reporting
- `main.cpp` for CLI and orchestration

This avoids a single monolithic source file and keeps algorithm logic separated.

### 2.4 Distributed leader election (FloodMax-style)

For each node, candidate starts as its own ID. In each round:
- each owned node computes max(candidate of self and neighbors)
- global candidate state is synchronized with `MPI_Allreduce(MPI_MAX)`
- convergence is checked by reducing a local-change flag

After convergence, all nodes should agree on the same max ID, which is the elected leader.

### 2.5 Distributed Dijkstra

From source node `s`:
- each rank tracks tentative distances for all nodes
- each rank proposes its best unsettled local node
- global best node selected via `MPI_Allreduce` with `MPI_MINLOC`
- owner rank relaxes outgoing edges
- remote relaxations are sent to other ranks
- distances synchronized with `MPI_Allreduce(MPI_MIN)` each iteration

This is a practical MPI baseline that is correct for positive weights.

---

## 3) Experimental hypothesis

I tested two hypotheses:

1. **Partition strategy effect**:
   Block partition should reduce cut edges compared to mod in many graphs, which should reduce communication overhead.

2. **Graph size effect**:
   Medium graph should require more iterations/messages than small graph.

---

## 4) Expected results

- A single agreed leader on every run.
- Correct shortest-path distances from source 0.
- Higher message volume and runtime on larger graphs.
- Partition strategy affecting cut edges and communication behavior.

---

## 5) Actual results

Main observations from runs and generated summaries:
- Leader election converged and produced consistent global leader.
- Dijkstra completed and returned valid distances for tested graphs.
- Medium graph runs generally showed higher iteration/message costs than small graph.
- Mod vs block showed different cut-edge counts and communication patterns.

Detailed logs are saved in:
- `outputs/experiment_logs/partition_compare.txt`
- `outputs/experiment_logs/scale_compare.txt`
- `outputs/summaries/partition_compare_summary.txt`
- `outputs/summaries/scale_compare_summary.txt`

---

## 6) Explanation of results

- **Why block can help**: block keeps consecutive IDs together, so if edge locality follows ID ordering, fewer cross-rank edges are cut.
- **Why scaling increases cost**: bigger graph means more settle steps and more synchronization/communication rounds.
- **Why collectives are expensive**: both algorithms use global collectives (`Allreduce`), which become dominant at larger sizes.

---

## 7) Specific decisions / insights

- I prioritized correctness and reproducibility first (seeded generation, deterministic configs, scriptable runs).
- I used two partition strategies to produce meaningful experiment comparisons.
- I kept per-rank logs and aggregate metrics so runs are auditable and easy to explain.
- I modularized MPI code to reduce complexity and improve maintainability.

---

## 8) Limitations and future improvements

Current implementation is a course baseline and can be improved by:
- reducing global synchronization cost
- using more local/asynchronous message patterns
- adding larger graph/rank scaling studies
- adding stronger performance instrumentation (per-phase communication timing)

---

## 9) Reproducibility notes

Everything is runnable from command line via:
- build commands in `README.md`
- graph/partition tools
- experiment scripts
- tests in `tests/run_tests.sh`

Seeds are stored in graph outputs, and runtime can optionally record a run seed for traceability.

## 10) Analysis and insights

The experiments support the expected behavior of a distributed MPI graph workload. For the small graph with 8 nodes and 4 ranks, both partition strategies produced the same correct leader (7) and the same shortest-path distances from source 0, which confirms that correctness did not depend on the partition strategy. However, the communication cost differed slightly. With the mod partition, the run used 53 logical messages and about 592 bytes, while the block partition used 55 logical messages and about 616 bytes. The runtimes on these very small runs are close enough that startup overhead and local machine noise can affect the exact numbers, but the message counts show that partition strategy changes communication behavior even when the graph and final answers stay the same.

The medium graph with 16 nodes and 4 ranks showed the clearest scaling effect. Leader election still converged in 4 rounds, but Dijkstra required 17 iterations instead of 9, and the total communication increased to 100 logical messages and about 1720 bytes. This matches the expected trend: once the graph becomes larger, the runtime performs more global coordination and more cross-rank relaxations, so both communication volume and runtime increase. In other words, the computation did not become expensive because arithmetic got harder; it became more expensive because the distributed algorithm had to synchronize and exchange more information.

One important insight is that communication overhead matters even in a correct implementation. The Dijkstra baseline uses a global minimum selection each iteration and then synchronizes updated distance information, so the cost grows with both graph size and iteration count. This is why the medium graph took longer even though it used the same number of ranks. The leader election phase was relatively cheap because it converged in a small fixed number of rounds, while Dijkstra dominated the total runtime because it repeated communication and reduction steps more times.

Another useful observation is that small MPI timings on a local machine are noisy. In one fallback run using the macOS/OpenMPI TCP settings, the same small graph produced the same message counts and the same correct final distances, but the measured leader-election time increased noticeably. That result reinforces an important systems lesson: on small workloads, the algorithm may be correct and stable while the timing still varies due to runtime environment, MPI transport choice, and process-launch overhead. For that reason, message counts, bytes sent, and iteration counts are often more reliable indicators of scalability trends than a single raw runtime number on a laptop.

The main design takeaway from these experiments is that partitioning and communication pattern matter more than local computation cost. The implementation is correct and reproducible, but it is still a baseline design. A more advanced version could reduce communication overhead by using less global synchronization, more asynchronous progress, or smarter graph partitioning that reduces cut edges. Even so, this project already demonstrates the central distributed-systems lesson: once a graph is partitioned across ranks, edges become communication events, and communication becomes a first-class performance cost.