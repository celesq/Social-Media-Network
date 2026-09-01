# Social Media Network Benchmark Suite

A reproducible benchmark and validation harness for the C implementation of
[Social Media Network](../).

The suite exercises the application's graph, post/repost, recommendation, and
community-detection functionality under deterministic workloads while measuring
runtime, memory usage, and memory safety.

It is designed to answer three questions:

1. Does the application remain correct under larger workloads?
2. How does its resource usage change as the workload scales?
3. Does the implementation remain memory-safe under representative stress cases?

---

## Overview

The Social Media Network application models:

- friendships using graph structures;
- posts and reposts using hierarchical tree structures;
- friend recommendations and graph traversal;
- repost ancestry using Lowest Common Ancestor (LCA);
- community detection using the Bron-Kerbosch maximal-clique algorithm.

The benchmark suite generates deterministic command streams that exercise these
features at scales significantly larger than the original functional tests.

The largest graph workload currently included has been validated with:

- 10,000 users;
- 1,000,000 unique undirected friendships.

The benchmark harness does not modify the application logic during execution.
It generates inputs, executes the existing binaries, validates deterministic
output sentinels, and records performance metadata.

---

## Repository assumptions

The harness is built around the current project structure.

Important implementation details include:

- `friends.h` defines the maximum graph capacity through `MAX_PEOPLE`;
- `social_media.c` creates the friendship graph using `lg_create(MAX_PEOPLE)`;
- `users.c` loads `users.db` relative to the process working directory;
- the Makefile builds the `friends`, `posts`, and `feed` executables;
- `feed` supports commands from all three application components;
- posts and reposts grow dynamically;
- friendships are stored as an undirected graph using adjacency lists.

The workload generator verifies the source assumptions it depends on before
generating benchmark data. If an incompatible source change is detected, the
benchmark fails instead of silently running against an invalid configuration.

---

## Directory structure

```text
benchmarks/
├── generate_stress.py
├── run_benchmarks.py
├── summarize_results.py
├── self_test.py
├── run_all.sh
├── run_10k_scale.sh
└── README.md
```

Generated files are written to:

```text
benchmarks/generated/
```

Benchmark results are written to:

```text
benchmarks/results/
```

These directories contain reproducible or machine-specific artifacts and should
normally not be committed.

Recommended `.gitignore` entries:

```gitignore
benchmarks/generated/
benchmarks/results/
benchmarks/__pycache__/
*.pyc
```

---

## Benchmark components

### `generate_stress.py`

Generates deterministic workloads and isolated `users.db` files.

The generator supports large graph workloads without materializing every
possible user pair in memory. This allows workloads such as 10,000 users and
1,000,000 unique friendships to be generated efficiently.

Generated usernames use the form:

```text
bench00000
bench00001
bench00002
...
```

The number of generated users depends on the selected benchmark profile.

---

### `run_benchmarks.py`

Main benchmark runner.

It is responsible for:

- generating workloads;
- compiling the application;
- running existing regression tests when their original fixtures are available;
- validating generated workloads;
- executing repeated timing runs;
- measuring peak resident memory;
- running representative Valgrind checks;
- optionally building and running sanitizer-instrumented binaries;
- recording system and source metadata.

Correctness validation always occurs before performance measurements are
accepted.

---

### `summarize_results.py`

Converts the raw JSON benchmark output into a human-readable Markdown report.

The summary includes:

- workload scale;
- command count;
- median runtime;
- p95 runtime;
- end-to-end throughput;
- peak RSS;
- memory-safety status;
- environment metadata.

Only successfully validated workloads are reported as valid benchmark evidence.

---

### `self_test.py`

Tests the benchmark infrastructure itself.

Run this before using the harness after modifying the generator or runner.

---

### `run_all.sh`

Runs the main benchmark profile and generates its Markdown summary.

---

### `run_10k_scale.sh`

Runs the dedicated large-graph scalability suite.

The current profile exercises:

```text
10,000 users /   250,000 friendships
10,000 users /   500,000 friendships
10,000 users / 1,000,000 friendships
```

Each workload contains unique undirected friendships and includes a deterministic
correctness sentinel.

---

## Prerequisites

Linux is recommended because the harness uses GNU timing utilities and Valgrind.

On Ubuntu/Debian:

```bash
sudo apt update
sudo apt install build-essential python3 valgrind time
```

No external Python packages are required.

Check that the project builds normally before running benchmarks:

```bash
make clean
make -j
```

---

## Validate the benchmark harness

From the repository root:

```bash
python3 benchmarks/self_test.py
```

A successful run should end with:

```text
OK
```

This validates the workload-generation logic independently of the Social Media
Network executable.

---

## Quick smoke test

The `quick` profile provides a fast verification that the benchmark pipeline is
working correctly.

```bash
python3 benchmarks/run_benchmarks.py \
  --repo . \
  --profile quick \
  --repeats 2 \
  --output benchmarks/results/quick_results.json
```

Generate the report:

```bash
python3 benchmarks/summarize_results.py \
  benchmarks/results/quick_results.json \
  --output benchmarks/results/quick_summary.md
```

Inspect it with:

```bash
cat benchmarks/results/quick_summary.md
```

All generated workloads should report `passed` before proceeding to larger
profiles.

---

## Full benchmark suite

Run the standard benchmark profile with:

```bash
python3 benchmarks/run_benchmarks.py \
  --repo . \
  --profile cv \
  --repeats 5 \
  --output benchmarks/results/benchmark_results.json
```

Then generate the summary:

```bash
python3 benchmarks/summarize_results.py \
  benchmarks/results/benchmark_results.json \
  --output benchmarks/results/benchmark_summary.md
```

Alternatively:

```bash
./benchmarks/run_all.sh .
```

The full profile covers several independent workload categories:

- graph creation and friendship operations;
- friendship churn;
- posts, reposts, and likes;
- mixed graph/content workloads;
- controlled Bron-Kerbosch clique workloads;
- deep repost hierarchies and LCA queries.

---

## 10K-user scalability test

For dedicated large-graph testing:

```bash
./benchmarks/run_10k_scale.sh .
```

This executes three graph workloads:

| Users | Unique friendships |
|---:|---:|
| 10,000 | 250,000 |
| 10,000 | 500,000 |
| 10,000 | 1,000,000 |

Each workload is executed five times after correctness validation.

Results are written to:

```text
benchmarks/results/benchmark_10k_scale_results.json
benchmarks/results/benchmark_10k_scale_summary.md
```

---

## Correctness validation

Generated workloads are not considered successful merely because the executable
terminates.

Each workload contains a deterministic output sentinel that checks part of the
resulting application state.

Examples include:

- expected friendship degree after graph construction;
- expected state after friendship add/remove churn;
- expected like counts;
- expected repost ancestry;
- expected clique output.

A workload must pass its sentinel before its timing results are accepted.

This prevents crashes, partial execution, or silently incorrect state from being
reported as successful scalability results.

---

## Synthetic `users.db`

The application loads `users.db` from its current working directory.

To avoid modifying the repository's original dataset, benchmark workloads run
inside isolated generated directories containing their own deterministic
`users.db`.

For example:

```text
benchmarks/generated/<workload>/data/users.db
```

This ensures stress tests:

- do not overwrite repository data;
- do not depend on course-specific usernames;
- are reproducible across machines.

Existing `.in` / `.ref` regression fixtures are handled separately.

If the original `users.db` required by those fixtures is unavailable, the
regression stage is explicitly reported as skipped rather than being executed
against incompatible synthetic users.

---

## Performance measurements

Timing is performed only after correctness validation.

For each workload, the harness records several metrics.

### Wall-clock runtime

The same validated command stream is executed repeatedly.

The report records:

- median;
- minimum;
- p95.

The median is used as the primary runtime measurement because it is less
sensitive to individual scheduling or system-load outliers.

### Throughput

Throughput is calculated as:

```text
total input commands / median wall-clock runtime
```

This represents end-to-end application throughput.

It includes:

- command parsing;
- graph/content operations;
- memory allocation;
- output generation;
- program initialization;
- teardown.

It should therefore not be interpreted as isolated algorithm or query latency.

### Peak RSS

When GNU `/usr/bin/time` is available, the harness records maximum resident set
size for the workload.

This approximates the maximum amount of physical memory occupied by the process
during execution.

---

## Memory safety

Performance and memory-safety runs are intentionally separated.

Instrumentation substantially changes execution time, so Valgrind and sanitizer
runs are never used as performance measurements.

### Valgrind

Representative workloads can be executed under Valgrind to detect:

- memory leaks;
- invalid reads;
- invalid writes;
- incorrect memory management.

### AddressSanitizer / UndefinedBehaviorSanitizer

Where supported by the project's build configuration, the runner can build
instrumented executables using ASan and UBSan.

These checks target issues such as:

- out-of-bounds memory access;
- use-after-free;
- invalid pointer operations;
- undefined integer or memory behavior.

A sanitizer build failure is reported separately from a sanitizer runtime
failure.

---

## Result reproducibility

The raw JSON report records metadata about the benchmark environment, including
where available:

- Git commit;
- CPU;
- operating system/platform;
- compiler;
- Python version;
- benchmark profile;
- repetition count;
- source hashes;
- generated input hashes.

Generated workloads are deterministic, which allows the same inputs to be
recreated on another machine.

Timing values are inherently hardware-dependent, but workload correctness and
scale remain reproducible.

---

## Friendship-count semantics

A friendship represents one unique undirected graph edge.

For example:

```text
1,000,000 friendships
```

means exactly 1,000,000 unique relationships between pairs of users.

Because the application uses adjacency lists for an undirected graph, each
friendship may internally appear in both users' adjacency lists.

The benchmark reports logical friendships, not internal adjacency-list entries.

---

## Bron-Kerbosch workloads

The application uses Bron-Kerbosch for maximal-clique detection.

Clique enumeration has exponential worst-case complexity, so these benchmarks
are deliberately isolated from maximum-scale graph-ingestion tests.

The suite uses controlled clique sizes to verify correctness and observe
algorithm behavior without conflating clique enumeration with graph-storage
scalability.

In particular, the benchmark does **not** execute `common-group` over the
10,000-user / 1,000,000-edge stress graph.

---

## Repost hierarchy workloads

Reposts form hierarchical structures and are tested independently from the
friendship graph.

Dedicated workloads construct increasingly deep repost chains and validate
Lowest Common Ancestor queries against deterministic expected results.

This allows tree traversal behavior to be tested without mixing it with graph
ingestion performance.

---

## Interpreting results

A few rules are important when reading benchmark reports:

1. **Generated input is not proof of successful execution.**  
   Only workloads marked `passed` completed their correctness sentinel.

2. **Throughput is end-to-end throughput.**  
   It is not isolated graph-operation latency.

3. **Results from different workloads should not be combined.**  
   For example, the maximum friendship count and maximum content count may come
   from separate application executions.

4. **Mixed workloads represent simultaneous scale.**  
   Use the `mixed` workload when evaluating behavior with graph and content
   operations active in the same process.

5. **Performance results are machine-dependent.**  
   Compare workloads from the same environment when evaluating scaling trends.

6. **Clique enumeration is fundamentally different from graph ingestion.**  
   Maximum-edge graph tests should not be interpreted as Bron-Kerbosch
   scalability results.

---

## Example validated scalability result

On an Intel Core Ultra 9 285H system, the current implementation successfully
completed the largest dedicated graph workload:

```text
10,000 users
1,000,000 unique friendships
1,000,001 commands
5 repetitions
23.35 s median runtime
124.4 MB peak RSS
```

This result is provided as a reference execution, not as a hardware-independent
performance guarantee.

To reproduce the workload:

```bash
./benchmarks/run_10k_scale.sh .
```

---

## Cleaning generated data

Benchmark inputs and results can be removed safely:

```bash
rm -rf benchmarks/generated
rm -rf benchmarks/results
```

They will be recreated automatically on the next run.