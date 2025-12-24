# Benchmarking jlq

This document describes a repeatable workflow to generate datasets and measure `jlq` throughput.

This is intended for **local performance evaluation** and is not run as part of unit tests.

## Goals (from PRD)

- Process a 10GiB JSONL file in < 5 seconds on NVMe (environment-dependent).
- Keep memory usage bounded (scratch buffer sized to max line + padding).

## 1) Generate benchmark data

Use the deterministic generator script:

```bash
./scripts/gen_jsonl.py \
  --out /mnt/nvme/jlq_10g.jsonl \
  --lines 20000000 \
  --seed 1 \
  --path network.http.status \
  --type number \
  --value 500 \
  --match-rate 0.01 \
  --missing-rate 0.10 \
  --malformed-rate 0.001 \
  --crlf-rate 0.10
```

Notes:
- Keep `--seed` constant for comparable results.
- Prefer writing to NVMe for PRD-style targets.
- For max-line-length policy testing (64MiB), use `--oversize-rate` on a smaller file.

## 2) Build jlq (recommended presets)

Use a Release build for benchmarking:

```bash
cmake --preset release
cmake --build --preset release-build
```

## 3) Run throughput benchmarks

### 3.1 Using the included benchmark runner

Redirecting stdout is recommended to avoid measuring terminal I/O.

```bash
./scripts/bench.py \
  --jlq ./build/release/bin/jlq \
  --file /mnt/nvme/jlq_10g.jsonl \
  --path network.http.status \
  --type number \
  --value 500 \
  --runs 7 --warmups 1
```

The script reports median wall time and computed throughput (GiB/s).

### 3.2 Using hyperfine (optional)

If `hyperfine` is installed:

```bash
hyperfine --warmup 1 --runs 7 \
  './build/release/bin/jlq /mnt/nvme/jlq_10g.jsonl --path network.http.status --type number --value 500 > /dev/null'
```

## 4) Cold vs warm cache

Two modes are common:

- **Warm cache**: repeat runs without cache dropping; better for CPU/parse throughput comparisons.
- **Cold-ish**: best-effort I/O measurement. This often requires privileged cache drop:
  - `sync; echo 3 | sudo tee /proc/sys/vm/drop_caches`
  - Not always available/allowed (especially in containers).

Always report which mode you used.

## 5) Record results (template)

Include the following in benchmark notes:

- Hardware: CPU model, core count, RAM
- Storage: NVMe/SATA, filesystem
- Build: preset, compiler version, CMake version
- Dataset generator args (including seed)
- Command line used
- Median time, computed throughput (GiB/s)
- Whether stdout was redirected to `/dev/null`

## 6) Optional deeper profiling (Linux)

### perf stat

```bash
perf stat -d \
  ./build/release/bin/jlq /mnt/nvme/jlq_10g.jsonl --path network.http.status --type number --value 500 > /dev/null
```

Use `perf record` / flamegraphs only after correctness is stable.
