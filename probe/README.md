# Issue #548: block scratch for composed bodies (parked 2026-11-13)

**Status.** Parked by the owner on 2026-11-13. The optimization has a limited scope (nine functions fault, on glibc mostly) and a high development cost, and other work has priority.

- The branch is a prototype on dev `dc4de9381b`. It must not be merged as is.
- C backend only. Rust, Java, C# and TypeScript are untouched.
- Constraints already ruled in the issue: bit-identical to today's batch body; portable; no threading change, mutex or memory pool; all backends move together.
- Gates run on it: `build.py` (0 warnings), `build.py test`, `build.py ref --versions=0_8_1`, the C server build, `regen-check`, generator `cargo test` and clippy, corpus harness `check` (243 ok, also under ASan+UBSan and at non-default periods), old/new A/B on four hosts.
- Gates never run: `ta_regtest --codegen` and stream verification, `synth_gate`, the autotools build, `scripts/sync.py`, the other backends' suites, adversarial review.

Hosts: Xeon (Gold 6248, gcc 14.2, glibc 2.40, Ubuntu 24.10), intel-1 (gcc 14.2), amd-1 (gcc 13.3, WSL2), macos-1 (M2, Apple clang 17). Every timing below is at default parameters.

## What is on this branch

Commits on dev: `5fff721d9b` (kernel from each step), `1381838a06` (restart sweep), `f08372600b` (composed kernels, `BlockBatch`), `a6294b55ca` (MA block entry, batch re-call), `82f1a8c91d` (threshold dispatch), `9b26809129` (64-bar sub-blocks, ICHIMOKU off the list).

| Piece | Path | Purpose |
|---|---|---|
| Kernel emitter | `ta_codegen/generator/src/backends/c_stream.rs` | `TA_<N>_KernImpl` derived from each stream step (243 of 244; not MAVP), private `TA_<N>_UpdateBlock`; composed kernels over `TA_KERN_BLOCK`-bar stack pads; MA block entry over the Dispatch plan; batch re-call of restart-exact subs; private `TA_<N>_BlockBatch` |
| Dispatch | `ta_codegen/generator/src/backends/c.rs` | public double `TA_<N>` hands off to `TA_<N>_BlockBatch` when the range reaches the threshold and no buffers overlap |
| Private header | `ta_codegen/generator/src/server_gen.rs` (renders `src/ta_func/ta_func_stream_private.h`) | `TA_KERN_BLOCK` (64), `TA_SpansOverlap` |
| Harness emitter | `ta_codegen/generator/src/kern_harness.rs` | writes the corpus harness C only when `TA_CODEGEN_KERN_HARNESS=<path>` is set |
| Per-function switch | `c_block_batch_min_bars: 20000` in `ta_codegen/input/{sqzmom,gator,kc,kstext,macdext,stochrsi}/*.yaml` | C only; not an attribute, because attributes are public API in every language |
| Generated C | `src/ta_func/*.c` | regenerated |
| `probe/kern/` | driver for the generated harness | modes `check` (kernel and `BlockBatch` bits against batch), `time`, `restart` (bitwise sub-range sweep, walk and spike shapes), `spin`; env `KERN_ONLY`, `KERN_PERIOD`, `KERN_WARM_MS`, `KERN_FAULTS`; `build.sh`, then `run.sh <cpu> <max reps>` for a corpus pass, `run9.sh` for one process per function and length |
| `probe/ab2/` | old/new library A/B | ns per call, faults, bit hashes, aliased calls (output offset is `run.sh` arg 6; the alias check only bites with the output past the look-back, e.g. 5000) |
| `probe/pybench/` | timing through ta-lib-python | written, never run: see the last section |
| `probe/blockpad/`, `probe/stepblk/` | first hand-written SQZMOM probes | variants `ref` (shipped), `five` (hand copy, control), `blk` (hand-carried block loops), `one` (single allocation), `strm` (public stream `Update`), `iblk`/`iblkv`/`hblk` (step loops), `kblk` (kernel from step); `run_time.sh <cpu> [maxreps=6] [variants]`, `run_parts.sh <cpu> 6`, `run_kern.sh <cpu> 6`; env `PROBE_ONLY`, `PROBE_TOL`, `PROBE_WARM_MS`, `PROBE_ROTATE`, `PROBE_REVERT`; `probe_mutant` (`PROBE_RESEED`: restarts rolling sums at each block edge) must fail `check` |
| `probe/abbench/` | A/B used for the single allocation | `run.sh <cpu> <cap> "<functions>" "<lengths>"`; CRSI needs more than 100 bars, so the short length is 250 |
| `probe/one-alloc/one-alloc.patch` | single-allocation generator pass, against dev `dc4de9381b` | coalesces a composed body's same-type scratch mallocs into one (batch `TA_`, `TA_S_`, composed Open) |

## Findings

### Cause

glibc raises its dynamic mmap threshold to the size of the last freed mmapped chunk and the trim threshold to twice that. A body that frees five chunks of size S leaves 5S free at the heap top, above 2S: trimmed, faulted in again on the next call. One chunk of 5S leaves 5S, under 10S: kept. So two or more series-length scratch arrays freed per call fault every call; one array never does. The threshold caps at 32 MB: a larger chunk is mmapped every call whatever its count (SQZMOM's scratch is 40 B/bar, so above about 838,000 bars).

Census, 244 functions at default parameters, a fresh process each; the other 235 read 0.00:

| function | arrays | faults per call, 20,000 bars | 100,000 bars |
|---|---|---|---|
| SQZMOM | 5 | 163 | 945 |
| CRSI | 3 | 85 | 554 |
| GATOR | 3 | 85 | 554 |
| ICHIMOKU | 2 | 46 | 359 |
| KC | 2 | 46 | 358 |
| KSTEXT | 2 | 46 | 358 |
| MACDEXT | 2 (default SMA path) | 46 | 358 |
| STOCHRSI | 1 own + 1 inside STOCHF | 46 | 358 |
| MAVP | mixed int/double | 0 | 358 |

At 1,000,000 bars: SQZMOM 9,734, CRSI and GATOR 5,827, the two-array functions 3,874. This corrects the issue's "eighteen functions" to nine. One array never faults (STOCH, STOCHF, ADXR, RVIR, ALLIGATOR, KDJ). BBANDS, PERCENTB, BBW, APO, PPO, PVO and MA take a fused or default path with no malloc at defaults. Faults start well below 20,000 bars: SQZMOM 7 per call at 4,000 bars, 46 at 8,000, 124 at 16,000; KC none at 8,000, 31 at 16,000. On amd-1 (WSL2) a fault costs more: shipped SQZMOM climbs from 12.0 ns/bar at 2,000 bars to 31.3 at 20,000 and 47.0 at 1,000,000.

macOS: no faults on any path. Its cost is the opposite: one allocation summing several arrays loses where the sum first crosses about 32 KB while each array alone stays under (believed to be the allocator's small/medium size-class boundary; not measured).

### Approaches measured

**(a) Public stream `Update` as the carry.** Bit-identical, zero faults, too slow. SQZMOM on intel-1: 16,640 -> 47,712 ns at 2,000 bars (186.7% slower); 1,490,428 -> 2,710,552 ns at 100,000 (81.9% slower). `Update` also validates and rejects non-finite input per bar, which batch does not.

**(b) Step loops (each callee's in-file `_StepImpl` called per bar in a block loop).** Compiler-fragile. Apple clang does not inline `TA_VAR_StepImpl` (a call per bar). SQZMOM at 2,000 bars: `hblk` 3.3% slower on intel-1 (16,675 -> 17,233 ns), 52.9% slower on macos-1 (25,055 -> 38,301 ns). MIDPRICE's step is 5.3 to 5.9 ns/bar dearer than its batch on intel-1: stateless and exact-scan callees must be re-called in batch form, not stepped.

**(c) One allocation per call.** No new transcription. intel-1, clean pass, dev -> one allocation:

| function | 250 | 2,000 | 100,000 | 1,000,000 |
|---|---|---|---|---|
| SQZMOM | 3.2% faster | 0.2% faster | 1,495,080 -> 870,425, 41.8% faster | 18,620,078 -> 18,597,568, 0.1% faster |
| CRSI | 0.7% faster | 0.2% slower | 1,985,913 -> 1,618,778, 18.5% faster | 22,457,377 -> 16,799,113, 25.2% faster |
| GATOR | 2.9% faster | 0.2% faster | 796,039 -> 450,459, 43.4% faster | 10,317,276 -> 5,498,276, 46.7% faster |
| ICHIMOKU | 1.7% faster | 0.1% slower | 744,140 -> 487,488, 34.5% faster | 9,361,706 -> 6,115,754, 34.7% faster |
| KC | 1.9% faster | 0.2% faster | 480,719 -> 249,906, 48.0% faster | 7,038,858 -> 4,101,598, 41.7% faster |
| KSTEXT | 1.1% faster | 0.5% slower | 986,615 -> 744,014, 24.6% faster | 11,754,391 -> 8,315,595, 29.3% faster |
| MACDEXT | 1.5% faster | 0.1% slower | 527,737 -> 292,485, 44.6% faster | 7,493,903 -> 4,077,203, 45.6% faster |

Xeon and amd-1, noisy passes, percent faster at 100,000 / 1,000,000 bars:

| function | Xeon | amd-1 |
|---|---|---|
| SQZMOM | 35.1 / 2.3% slower (five passes, floors 39.53 -> 40.43 ms) | 67.3 / 9.4 (noise) |
| CRSI | 14.5 / 20.5 | 38.0 / 37.4 |
| GATOR | 45.0 / 45.5 | 73.4 / 66.8 |
| ICHIMOKU | 31.9 / 37.8 | 63.5 / 62.1 |
| KC | 42.9 / 42.7 | 75.3 / 71.4 |
| KSTEXT | 18.4 / 19.1 | 43.6 / 45.2 |
| MACDEXT | 34.4 / 39.3 | 68.5 / 63.6 |

- Above 32 MB of scratch there is no gain (SQZMOM at 1,000,000 bars: 9,766 faults, parity on intel-1, macos-1, amd-1; on the Xeon slower in all five passes, inside the SMA control's -6.6% .. +5.5% swing).
- glibc gain starts early: SQZMOM 15.8-16.2% faster at 4,000 bars, 30.5-30.9% at 8,000; GATOR 27.3-28.9% at 8,000 (intel-1). 500-2,000 bars within -0.4% .. +2.3%.
- macOS, three passes; level within -0.8% .. +1.4% at 100,000 and 1,000,000 bars:

| function | slower | faster |
|---|---|---|
| GATOR (3) | 2,000: 7.6-8.8% (9,884 -> 10,749 ns); 4,000: 1.8-4.6% | 8,000: 2.6-3.3%; 16,000: 1.1-1.7% |
| SQZMOM (5) | 1,000: 4.1-5.2% (12,273 -> 12,772 ns); 2,000: 1.6-2.4%; 4,000: 0.8-1.0% | 8,000: 2.2%; 16,000: 1.0-1.2% |
| KC (2) | 4,000: 3.6-5.1% (12,216 -> 12,662 ns) | 8,000-16,000: 1.2-2.1% |
| CRSI (3) | 2,000: 1.6-7.6%; 4,000: 1.0-1.2% | elsewhere within 1.3% |

- No reliable gate by allocator: `__GLIBC__` names the headers C compiled against, not the allocator that runs (a preloaded or replaced malloc); Rust's `target_env = "gnu"` is compile-time and an application can install its own global allocator; Java, C# and TypeScript have no such notion.

**(d) Block kernels.** A block loop that restarts each sub-call at every block edge changes the last bits (the `PROBE_RESEED` mutant is caught). Each stateful sub-call must continue across the edge, in one of three ways:

1. a kernel derived from the stream step: state hoisted into locals around a loop over the block;
2. the public batch re-called on the sub-range, for restart-exact functions (a sub-range call equals the slice of a full call bitwise). Needs the look-back bars contiguous before the block: true on caller arrays, false on an intermediate pad without a tail;
3. neither: a kernel keeping array look-backs instead of the step's ring (VAR, WMA, ROC, MOM class). Untested.

From-step kernel against batch over 221 functions (intel-1, block 512): median 6.0% slower at 2,000 bars, 6.2% at 100,000; within 10% for about 110; 25% or more slower for about 40% of them (vectorizable maps and lag differences, e.g. MOM 0.14 -> 0.73 ns/bar; exact window scans MAX, MIN, MIDPRICE, MIDPOINT, WILLR; candlestick scans; PERCENTRANK 7.5 -> 94.6 ns/bar). macos-1 has the same shape. One kernel from the step does not replace batch everywhere.

Classification, intel-1 kernel/batch at block 512 (worse of 2,000 and 100,000 bars) plus the bitwise restart sweep over 244 functions (117 restart-exact on both shapes, 127 not):

| level threshold | (1) kernel level | (2) batch re-call only | (3) neither |
|---|---|---|---|
| 3% | 94 | 92 | 35 |
| 5% | 106 (26 also restart-exact) | 89 (57 candlesticks + 32 others) | 26 |
| 10% | 118 | 84 | 19 |

At 5%, (3) is CDLSHORTLINE, CDLLONGLINE, CDLCLOSINGMARUBOZU (exact on the walk, not on spikes), PSO, VORTEX, HMA, WMA, ZIGZAG, FISHER, ER, DPO, LINEARREG_SLOPE, KAMA, HT_TRENDLINE, VAR, SMI, RVI, ULTOSC, DEMA, MASSI, CMF, OBV, CMOU, CORREL, SORTINO, HT_TRENDMODE. Class (3) is compiler-specific: VAR, KAMA, DEMA are level and WMA is faster on M2. The generator's `RESTART_EXACT` table holds those 117 names; a function with a bar-index output (MAXINDEX, MININDEX, MINMAXINDEX) is excluded from re-call by rule.

Kernel rules that turned out load-bearing:
- Hoisted locals keep the field's name: fma typing is by name, so another spelling silently moves fusion sites. Combine statements keep the step text for the same reason.
- A kernel from an fma step needs its callers' `TA_FMA_MULTIVERSION`; the generator must know the step uses fma. Multiversioned kernels become local IFUNCs (58 functions); `UpdateBlock` is hidden, absent from `nm -D`.
- Candle-setting globals are read once per block.
- Bind each bar input once per bar: re-reading `in[i]` after ring stores that may alias it is wrong.
- Load every field, not only those read: a field written only on the reseed path is otherwise stored back uninitialised.
- Reserved loop-counter name (VAR has a field `i`); decline on any name collision.
- A bare `return` becomes `continue` only outside a step loop; values that live only in Open (EMA's seed and coefficients) are read from the handle, never recomputed.
- A step that calls a public `Update` (STDDEV) cannot be derived: sub-kernel, then combine map.
- A stateless function needs no kernel: its batch on the sub-range is the block form.
- `BlockBatch` opens on the full arrays at the caller's `startIdx`. A rebased window broke APO, PPO, PVO (EMA arms seed from `startIdx`) and MAXINDEX, MININDEX, MINMAXINDEX, ZIGZAG (index outputs count from bar 0).
- `restrict` changes nothing.

Sub-block size, ns per call at 100,000 bars, dev -> sub-block 64 / 128 / 256 / 512 (bits identical across arms):

| function | intel-1 | macos-1 |
|---|---|---|
| SQZMOM | 1,471,248 -> 909,036 / 901,239 / 894,391 / 890,233 | 1,285,500 -> 1,162,800 / 1,222,700 / 1,251,150 / 1,271,450 |
| GATOR | 788,486 -> 424,810 / 487,667 / 523,227 / 539,477 | 528,300 -> 470,200 / 567,950 / 615,100 / 634,500 |
| KC | 492,217 -> 198,053 / 203,766 / 244,999 / 251,967 | 292,650 -> 222,250 / 266,150 / 290,650 / 295,700 |
| ICHIMOKU | 722,462 -> 563,440 / 504,001 / 466,471 / 458,237 | 551,000 -> 794,150 / 668,550 / 601,300 / 677,050 |
| KSTEXT | 983,260 -> 704,670 / 716,320 / 706,018 / 710,144 | 1,173,800 -> 1,072,750 / 1,188,600 / 1,174,900 / 1,177,600 |
| MACDEXT | 531,214 -> 259,289 / 259,534 / 267,302 / 267,668 | 574,400 -> 524,600 / 579,350 / 572,400 / 579,000 |
| STOCHRSI | 1,043,009 -> 929,156 / 921,085 / 910,718 / 909,451 | 1,100,450 -> 1,042,400 / 1,112,950 / 1,122,200 / 1,135,950 |

The best size is per function: functions of batch re-calls (ICHIMOKU) want large blocks, since each re-call recomputes its look-back; carried kernels with short look-backs (GATOR, KC) want small ones. 64 makes six of seven faster than dev on M2 and costs intel-1 nothing. In the hand SQZMOM probe, 1,024 gained nothing on intel-1 (flat from 128 to 1,024) and was the worst length on M2 (monotonic, 32 the best measured).

Threshold dispatch: before it, the blocked batch at 2,000 bars was up to 29.3% slower (GATOR, Xeon) and mixed at 8,000; from 20,000 bars it was faster on the three Linux hosts for every function but CRSI. So the public double `TA_<N>` takes `BlockBatch` only when the range is at least `c_block_batch_min_bars` (20,000 for all six) and `TA_SpansOverlap` finds no input/output or output/output overlap; otherwise today's body runs unchanged.

### Result of the prototype (`9b26809129`)

Percent faster, dev -> new, at 20,000 / 100,000 / 1,000,000 bars, fresh process per row:

| function | Xeon (noisy) | intel-1 (clean) | macos-1 | amd-1 |
|---|---|---|---|---|
| SQZMOM | 30.6 / 34.6 / 54.7 | 35.8 / 38.3 / 50.4 | 4.8 / 3.9 / 10.7 | 60.4 / 65.5 / 72.9 |
| GATOR | 30.8 / 41.3 / 55.1 | 37.1 / 46.6 / 57.5 | 11.1 / 11.0 / 14.8 | 68.8 / 71.9 / 78.8 |
| KC | 52.7 / 59.1 / 70.8 | 50.2 / 59.2 / 69.9 | 25.7 / 24.2 / 28.9 | 73.5 / 75.2 / 84.1 |
| KSTEXT | 20.4 / 31.6 / 34.2 | 17.7 / 28.5 / 38.1 | 8.2 / 8.3 / 8.6 | 42.4 / 51.9 / 58.5 |
| MACDEXT | 27.1 / 36.2 / 46.1 | 37.1 / 51.3 / 64.2 | 3.6 / 9.6 / 11.2 | 59.5 / 67.8 / 79.1 |
| STOCHRSI | 14.9% slower / 24.9 / 24.6 | 2.6 / 11.1 / 17.6 | 8.1 / 5.2 / 5.3 | 55.8 / 34.6 / 46.2 |

ns per call at 100,000 bars: SQZMOM 1,475,409 -> 911,049 and KC 481,549 -> 196,460 (intel-1); SQZMOM 3,280,112 -> 2,146,780 (Xeon); SQZMOM 3,797,509 -> 1,309,680 (amd-1). The sub-block table's 64 column has the rest (a separate run).

- Bits identical to dev on all four hosts, aliased calls included (every alias row, at output offsets 0 and 5000).
- Zero faults on the new path at every length.
- Below 20,000 bars nothing changes: same code path; intel-1 rows within 0.6%, macos-1 within 1.2%.
- The one slower cell, STOCHRSI at 20,000 on the Xeon, sits inside that pass's SMA control swing (-11.7% .. +9.1%); the same row was 10.1% faster at sub-block 128 and is 2.6% faster on clean intel-1. Its margin at exactly 20,000 is thin: the threshold is per-function data and can be raised.

### Benchmark method the numbers depend on

- **One process per function when faults matter.** glibc's thresholds are per process: a function run after another has raised them shows no faults (KC batch read 256,400 ns in a shared process against 480,719 in a fresh one).
- **Adaptive repetitions.** One warm-up rep, then reps until the two lowest agree within 1%, capped (6 on Xeon, intel-1 and macos-1; 10 to 15 on amd-1). Rows that hit the cap are flagged and are not evidence for small effects.
- **Warm-up by time**, not by count (300 ms in process; on intel-1 a 2 s spin first; 25 ms proved too short).
- **Rotate series at short lengths** (100 series at 2,000 bars): a replayed series lets the branch predictor learn data-dependent steps.
- **Healthy data past 100,000 bars.** The probe's multiplicative walk with cents rounding froze at 0.50: 97.5% of bars flat from 100,000 to 500,000, 100% beyond, which makes window scans rescan every bar. Use the mean-reverting walk (`PROBE_REVERT=0.0005`: close stays 52..193, 0% flat to 1,000,000). The A/B harnesses use an additive walk clamped at 20, not affected.
- **Pinning and noise.** intel-1: pin cpu 2, 4 or 6, never 8 or 10 (two clock modes 13% apart); about one process in seven runs 2-6% slow for its whole life, so a claim finer than about 6% needs the minimum over 3-4 processes. macos-1: no affinity, QoS user-initiated; speed level varies per process, so ratios survive but absolute ns need two processes. amd-1: cpu 6, Windows background load makes it noisy. Xeon: shared machine, cpu 12, THP `madvise`, noisy. Every A/B carries an SMA control row.

## Not done and known defects

- **ICHIMOKU** is off the list. On M2 it is slower than dev at every sub-block (best 256: 550,850 -> 601,300 ns, 9.1% slower at 100,000 bars); on intel-1 it wants 512. Its three MIDPRICE re-calls per sub-block each recompute a 51-bar look-back. Fix: run re-called stages once over the whole range (they read caller inputs and write caller outputs; there is no reason to chop them).
- **CRSI** stays on today's body: its PERCENTRANK sub is pad-fed, so it runs as a step kernel (20 -> 200 ns/bar on intel-1). Needs a pad with a tail so batch can be re-called, or a faster step.
- **MAVP** has no block entry (the PeriodBank shape) and mixed int/double scratch.
- **Short calls.** `BlockBatch` opens heap-allocated handles: at 250 bars it is slower on intel-1 (e.g. SMA 14%, SQZMOM 22%, GATOR 34%) and 4% to 49% slower on macos-1. Hidden by the threshold; state on the stack would remove it.
- **Perf left on the table.** Combine loops render the step's branches where batch renders branch-free selects (SQZMOM's squeeze ladder); VAR-class rings cost about 0.22 ns/bar on gcc/x86 (STDDEV 11% slower blocked); functions with hand-fused batch arms (BBANDS, BBW, PERCENTB, APO, PPO, PVO) should keep batch.
- **Review items:**
  - the `BlockBatch` allocation-failure path returns `TA_ALLOC_ERR` without zeroing `outBegIdx`/`outNBElement` (the old body zeroes them);
  - `<stdint.h>` and plain `static inline` are new to library sources (C11 is set by CMake; autotools unchecked);
  - block entries skip `Update`'s per-bar non-finite check on intermediates; composed `UpdateBlock` requires outputs not overlapping inputs;
  - an existing generator test (the managed stream suite's delay-line case) was adjusted to skip the kernel body; the single-allocation patch changed three tests in `streaming_dispatch_suite.rs`;
  - the restart-exact table is pasted measured data, verified at default parameters and a few `KERN_PERIOD` values only;
  - field types are parsed back from rendered struct text;
  - a `*out` read inside an `a*b+c` could change fusion silently (no step does this today);
  - the `UpdateBlock` fallback inside `BlockBatch` is unreachable; a very large period makes the open's head as long as the period (cost, not correctness);
  - a row was added to `docs/ta_codegen_input_yaml.md`; several FuncDef literals gained a field.
- **macOS last digit.** The nine stateless transcendental maps differ from batch in the last digit when run through a kernel (batch uses the vector math path there, #521). The design answer is above: stateless functions get no kernel.
- **Unverified side finding.** MIDPRICE's stream output may differ from batch on signed zeros (-0.0 against +0.0); SQZMOM hides it.

## Scope, re-evaluated

A corpus-wide block kernel is not justified by this issue. 235 of 244 functions do not fault, so it buys them no batch speed. The rest is a long tail: functions without a level continuation, non-default parameters, and four more backends.

Owner direction of 2026-11-13 for any restart:
1. A per-function table holding the length threshold and whether the technique is on.
2. The technique written in the indicator's input code (`ta_codegen/input/`), not as new generator machinery.
3. Only the functions that matter in comparative benchmarks. Of the nine, MACDEXT and STOCHRSI are the first candidates.

Two source-level forms, and their limits:
- **Block loop in source.** It can only re-call a sub-function on a sub-range. That is bit-exact for window functions (SMA, MAX, MIN), not for recursive ones (EMA, RSI, ATR). It fits MACDEXT at SMA types only, not STOCHRSI (RSI is recursive). Precedent: the SMA arms of `bbw.c` and `percentb.c` already fuse callee loops over a 256-bar tile with a stack pad.
- **One allocation above a length threshold.** Fits any sub-call. The threshold avoids the macOS loss at 1,000-4,000 bars; the gain is glibc only and none above 32 MB of scratch.
- STOCHRSI's second array is allocated inside STOCHF (`ta_codegen/input/stochf/stochf.c`), so either form needs a change there.
- `PRAGMA TA_ALT={BATCH,...}` is the existing way for an input file to carry a batch-only body.

## To decide by measurement before resuming

1. **Through ta-lib-python.** The wrapper allocates its output arrays on every call and numpy's own large frees move the same glibc thresholds, so the gain measured from C may shrink or vanish. `probe/pybench` compares three library builds (dev `dc4de9381b`; single allocation, which is dev plus `probe/one-alloc/one-alloc.patch`; blocked `9b26809129`) on wrapper `bf29beb338`, per function in a fresh process at 10,000 / 100,000 / 1,000,000 bars and as a 16-function suite in one process. Not run: the wrapper extension was not built on the timing hosts (check RUNPATH against RPATH with `readelf -d`, since `setup.py` sets `runtime_library_dirs`). If the gain does not survive there, stop.
2. **Rust.** Same system allocator on Linux-gnu, so the same faults are expected (SQZMOM's Rust body has five `vec![0.0; n]`); unmeasured. Java, C# and TypeScript are not expected to have this fault.
3. **musl, Windows, and a host process that replaces malloc.** Unmeasured; musl and Windows are expected to fault regardless of array count.
4. **Non-default parameters.** Both the timing and the restart-exact table; every number here is at defaults.
5. **BBANDS with a non-default MA type.** The census ran at defaults, where BBANDS takes a path with no malloc; whether another MA type faults is unknown.
6. **MACDEXT: single allocation above a threshold against source-level re-call blocks**, on glibc and macOS at the threshold length. This picks the form for the first candidate.
7. **Distro coverage.** Only the Xeon's distro was recorded (Ubuntu 24.10, glibc 2.40); the census and every Linux number assume that glibc's behaviour.
