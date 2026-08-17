---
name: codegen-perf-iteration
description: Autonomous codegen quality and performance loop. Use when optimizing ta_codegen C output, investigating performance regressions, or iterating toward parity with the C reference library. Triggers on "perf iteration", "performance loop", "benchmark loop", "codegen performance", or "optimize codegen".
---

# Codegen Performance Iteration

Autonomously evolve ta-lib's codegen output toward performance parity with the C reference library through a generate, build, test, benchmark, analyze, fix loop.

## The Core Loop

```
GENERATE → BUILD → TEST → BENCHMARK → ANALYZE → PLAN → FIX → TEST → BENCHMARK → COMMIT/REVERT → repeat
```

### GENERATE
```bash
cd ta_codegen/generator
cargo run --release -- generate --backend=c
cargo run --release -- generate-servers --backend=c
cargo run --release -- generate-bench --backend=c
```
If the codegen panics, fix the parser/backend issue first. Don't iterate on broken generation.

### BUILD
```bash
cargo run --release -- build --backend=c
```
Also rebuild cmake + ta_bench_direct if needed:
```bash
cmake --build cmake-build --target ta_bench_direct
cp cmake-build/bin/ta_bench_direct bin/
```

### TEST (correctness gate)
```bash
cd bin && ./ta_regtest --codegen --language=c
```
**Every function must pass.** If ANY fails, stop and fix before benchmarking.

### BENCHMARK

**Read the spread before the ratio.** Both tools print a `+-` column; at low
`--iters` the same functions have read 0.57x and 1.00x on consecutive runs.
A number without a narrow spread beside it is not a measurement.

**Primary tool: `ta_bench_direct`** — the only thing that times `libta-lib.a`,
the artifact a real consumer links:
```bash
# Isolated — ground truth, no icache noise
cd bin && ./ta_bench_direct --function=NAME --iters=500 --points=100000

# Full suite — overview, verify outliers in isolation
cd bin && ./ta_bench_direct --iters=200 --points=100000
```
Its ratio is `ta_bench_cg` (single TU, `-flto`) over `libta-lib.a` (separate
TUs, no LTO) — a build-configuration difference, not an algorithm one. See
"The same source, six binaries" in the root CLAUDE.md before quoting it.

**Secondary tool: `ta_bench`** — cross-language, and the only way to reach the
frozen pre-cutover reference (`cref`). Its `timing_ns` is measured *inside* the
server around the call, so JSON-RPC transport is NOT in the timed region:
```bash
cd bin && ./ta_bench --language=cref,c --function=NAME --points=100000 --iters=500
```
Note it times `ta_codegen_serve_c`, a different binary from `ta_bench_direct`'s
C column, so the two tools' absolute ns are not comparable with each other.

Parse direct bench output:
```python
import re
text = re.sub(r'\033\[[0-9;]*m', '', raw_output)
for line in text.split('\n'):
    m = re.match(r'(\S+)\s+(\d+)\s+(\d+)\s+(\S+)x', line.strip())
    if m:
        name, ref, cg, ratio = m.group(1), int(m.group(2)), int(m.group(3)), float(m.group(4))
```

### ANALYZE

Categorize results:
- **Broken** (>2.0x slower): Something fundamentally wrong
- **Slow** (1.10x-2.0x): Investigate — dispatch a subagent
- **Parity** (0.90x-1.10x): Acceptable
- **Faster** (<0.90x): Verify correctness — could indicate skipped work

For each slow indicator, **dispatch a subagent** for deep analysis:
1. Compile both assemblies: `cc -O3 -DNDEBUG -Wno-everything -S` codegen and reference
2. Extract function bodies, count basic blocks, inner loops, fdiv instructions
3. Trace the hot loop critical path — cycle-count per iteration
4. Check for speculative computation (both sides of `&&` computed before short-circuit)
5. Check for binary layout effects (identical assembly but different timing)

### PLAN
Pick the **single highest-impact** fix. Priority:
1. Broken indicators (>2.0x) first
2. Groups sharing a root cause (e.g., all CDL patterns)
3. Individual slow indicators

Root cause categories:
- **Speculative computation**: compiler computing both sides of `&&` before short-circuit. Fix: split into nested `if`s (only when both sides contain `TA_CANDLEAVERAGE`).
- **Candle macros**: `TA_CANDLERANGE`/`TA_CANDLEAVERAGE` macros with static globals enable constant propagation. This is a NET WIN (53 CDL faster, 3 slower). Don't fight it.
- **Circular buffer**: modulo `%` vs conditional reset `if(idx>=max) idx=0`
- **Validation**: missing NULL checks or param range checks that change compiler register allocation
- **Binary layout / icache**: identical assembly but different timing in full-run. NOT fixable in source. Verify by testing in isolation.

### FIX
Make ONE change. Fix locations in priority order:
1. `ta_codegen/input/<name>/<name>.c` — indicator source (plain C)
2. `ta_codegen/generator/src/backends/c.rs` — C backend rendering
3. `ta_codegen/generator/src/backends/builtins.rs` + `ta_codegen/generator/templates/` — shared macros, types, globals
4. `ta_codegen/generator/src/parser/` — parser changes

After fixing, go back to GENERATE and repeat the full loop.

### COMMIT or REVERT
- Tests pass AND target indicator improved → commit with descriptive message
- Tests fail OR indicator worse → `git checkout` changed files, try different approach
- 5 consecutive cycles with no improvement → stop and report

## Quality Gates

| Gate | Criterion | How to Check |
|------|-----------|-------------|
| Correctness | every function passes | `ta_regtest --codegen --language=c` |
| Core parity | RSI, SMA, EMA, MACD, STOCH within 1.05x | `ta_bench_direct --function=RSI,SMA,EMA,MACD,STOCH --iters=500` |
| CDL performance | 53+ CDL patterns faster, <=3 slower | `ta_bench_direct --function=CDL --iters=300` |
| No regressions | No indicator >1.15x in isolation | Compare against saved baseline |

## Cron Support

Use `/loop` to run the perf iteration autonomously:
```
/loop 15m /codegen-perf-iteration
```
This runs the full loop every 15 minutes. Each iteration:
1. Regenerates, builds, tests
2. Benchmarks the previously-slow indicators
3. If regressions detected, investigates and fixes
4. Logs results to `.plans/perf-iteration-log.md`

For one-off runs: just invoke `/codegen-perf-iteration` directly.

## Autonomy Rules

1. **Never wait for human input.** Log questions to `.plans/perf-iteration-questions.md`, pick faster-to-test approach, keep going.
2. **One change per cycle.** Don't fix three things at once.
3. **Use subagents for analysis.** Dispatch one subagent per slow indicator — they read assembly, count cycles, find root causes.
4. **Trust isolation over full-run.** A full-corpus run has ~10-20% noise from icache. Isolated `ta_bench_direct` runs are the best available signal — but check the `+-` column, they are not automatically ground truth.
5. **Revert failures quickly.** Don't spend 3 cycles saving a bad idea.
6. **Consult external AI when stuck.** 2+ failed cycles on the same indicator → get a second opinion.
7. **Log everything.** Each iteration → `.plans/perf-iteration-log.md`: what changed, why, before/after, outcome.

## Rebuilding ta_ref_serve

`scripts/regtest.py` rebuilds `ta_ref_serve` automatically in its cmake step, so the
normal pipeline handles this. The manual fallback (when cmake rebuilds `libta-lib.a`
and you need the reference server refreshed by hand) reads from
`ta_codegen/output/c/tools/ta_codegen_serve.c`:
```bash
sed '/#include "ta_[A-Z].*\.c"/d' ta_codegen/output/c/tools/ta_codegen_serve.c > /tmp/ta_ref_serve.c
sed -i '' '/#include "ta_lib_globals.c"/a\
extern int TA_Initialize(void);\
extern int TA_RestoreCandleDefaultSettings(int settingType);
' /tmp/ta_ref_serve.c
sed -i '' 's|int main(void) {|int main(void) { TA_Initialize(); TA_RestoreCandleDefaultSettings(11);|' /tmp/ta_ref_serve.c
cc -O3 -DNDEBUG -Wno-everything -I ta_codegen/output/c -o bin/ta_ref_serve /tmp/ta_ref_serve.c cmake-build/libta-lib.a -lm
```

## Current State (historical snapshot, 2026-03-21)

### Resolved
- Candle macros (`TA_CANDLERANGE`/`TA_CANDLEAVERAGE`) match reference pattern
- Short-circuit `&&` split for CDL patterns with dual `TA_CANDLEAVERAGE` (CDLHARAMI 1.39x → 0.82x)
- MINMAX was never slow (0.68x) — server overhead inflated it to 1.25x
- `ta_bench_direct` times libta-lib.a in-process; report its spread with its ratio

### Remaining (icache/layout, not code quality)
- CDL3BLACKCROWS: 1.16x isolated — compiler short-circuits correctly, minor fdiv interleaving diff
- CDLBREAKAWAY: 1.24x isolated — only 1 fdiv, codegen produces fewer instructions, layout effect
- SAR: 1.13x isolated — assembly identical to reference, binary layout from single-TU

### Scorecard (isolated, 500 iters, 100k points)
- 72 faster (<0.90x)
- 83 parity (0.90-1.10x)
- 3 slower (1.10-1.25x) — all layout effects, not code quality

## Key Files

| File | Role |
|------|------|
| `ta_codegen/generator/src/backends/c.rs` | C code generation — validation, candle macros, `&&` split |
| `ta_codegen/generator/src/bench_gen.rs` | Generates ta_bench_cg direct-call benchmark binary |
| `ta_codegen/generator/src/server_gen.rs` | Server generation — dispatch, load_data, timing |
| `ta_codegen/generator/src/main.rs` | Build step, generate-bench command |
| `ta_codegen/input/<name>/<name>.c` | Indicator source — the logic itself (plain C) |
| `ta_codegen/generator/src/backends/builtins.rs`, `ta_codegen/generator/templates/` | Shared macros, static globals, candle helpers |
| `src/tools/ta_bench/ta_bench_direct.c` | Direct-call benchmark orchestrator (cmake) |
| `src/tools/ta_bench/ta_bench.c` | Server-based benchmark with thermal canary |
| `scripts/regtest.py` | Full pipeline: generate + build + test + bench + direct-bench |
