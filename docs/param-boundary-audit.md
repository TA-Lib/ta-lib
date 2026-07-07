# Parameter-boundary & lookback-arithmetic audit (issue #94)

This is the committed record for GitHub issue **#94** — *"Audit parameter-boundary
OOB cases; extend ta_regtest to sweep min/default/default±1/max for every parameter."*

The period=1 investigation (#48, #59) surfaced memory-safety defects in the
**released** library caused by an unchecked `*_Lookback` result poisoning buffer
arithmetic (`memcpy` offsets, buffer sizes). Those were found by hand, not by the
test suite. This audit (a) enumerates every place a `*_Lookback`-derived value
feeds a buffer copy/size and records its safety basis, and (b) documents the two
automated backstops that now make the whole class fail loudly.

## Automated backstops

1. **Parameter-boundary sweep** — `src/tools/ta_regtest/ta_test_func/test_period_boundary.c`
   (`testMinBoundarySweep`, group `PERIOD1/BOUNDARY`). Driven by `TA_ForEachFunc`,
   it sweeps **every optional parameter of every function**:
   - integer ranges: `min`, `min+1`, `default-1`, `default`, `default+1`, and a
     large "past the data" period, plus the out-of-range `min-1` and (bounded)
     `max+1`;
   - integer/real lists (e.g. `MAType`): every enumerated value;
   - real ranges: `min`, `default`, `max` (realistic magnitudes).

   Contract (issue #94): the default must compute coherently; every other in-range
   value must **either** succeed with a coherent result (`outBegIdx == TA_GetLookback`,
   coverage to the last bar, **or** an empty result when the period consumes the
   range) **or** return a clean `TA_BAD_PARAM` — never a crash, an out-of-bounds
   access, or a non-finite / subnormal output. All successful outputs are scanned
   with `isfinite` + a subnormal check. Set `PB_SWEEP_LIST_ALL=1` to enumerate
   every failing case in one run instead of aborting on the first.

2. **ASan/UBSan nightly** — the `sanitizers` job in `.github/workflows/dev-nightly-tests.yml`
   builds the library + `ta_regtest` with `-fsanitize=address,undefined
   -fno-sanitize-recover=all` (`cmake -DENABLE_SANITIZERS=ON`, or
   `scripts/build.py ta_regtest --sanitize`) and runs the full C reference suite —
   including the boundary sweep. An out-of-bounds or uninitialized read that
   previously returned plausible garbage now aborts the job. This is the only
   check that catches a `fastEMABuffer[-1]`-class under-read; the value/alignment
   tests provably cannot.

## Lookback → buffer-arithmetic sites

`*_Lookback` results (and lookback-derived offsets) feed a buffer copy or size at
the sites below. `outNBElement`, all `lookback*` offsets, and `endIdx-startIdx+1`
are `>= 0` under the current (period≥1) lookback contract, so each source/length
is in range; the remaining hazard is **source/destination overlap**, addressed by
using `memmove` wherever the caller's buffer may be reused as scratch.

| Function | Site | Basis / status |
|----------|------|----------------|
| **MACD / MACDFIX** | `signalPeriod=1` temp-buffer offset (`&fastEMABuffer[-1]`) | **Fixed** by the period=1 lockstep rewrite (temp buffers removed; lookback contract floors the offset at 0). |
| **TRIX** | `TRIX(1)` chained `-2` lookback shifting the internal EMA passes | **Fixed** by the period=1 contract (no negative lookback). |
| **ULTOSC** | `ULTOSC(1,1,1)` reading `inClose[-1]` | **Fixed** — lookback ≥ 1. |
| **STOCH** | `stoch.c` `memmove(outSlowK, &tempBuffer[lookbackDSlow], …)` | **Fixed (#94)** — was `memcpy`; `tempBuffer` aliases `outSlowK` when the caller buffer is scratch, so the ranges overlap. Now `memmove`. |
| **STOCHF** | `stochf.c` `memmove(outFastK, &tempBuffer[lookbackFastD], …)` | **Fixed (#94)** — same pattern; `memcpy` → `memmove`. |
| **MACDEXT** | `macdext.c` `memmove(outMACD, &fastMABuffer[lookbackSignal], …)` | **Fixed (#94)** — same pattern; `memcpy` → `memmove`. |
| **RSI** | `rsi.c` `period==1` in-place copy `memmove(&outReal[0], &inReal[startIdx], …)` | **Fixed (#94)** — was `memcpy`; overlaps for an in-place caller (`outReal == inReal`) with `startIdx > 0`. Now `memmove`, matching WMA. |
| **CMO** | `cmo.c` `period==1` in-place copy | **Fixed (#94)** — same as RSI; `memcpy` → `memmove`. |
| **MAVP** | inverted window `optInMinPeriod > optInMaxPeriod` | **Fixed (#94)** — the per-bar clamp pushed the period above `optInMaxPeriod`, exceeding the lookback and reading uninitialized scratch. Now returns `TA_BAD_PARAM`. |
| **BBANDS** | `bbands.c` `memcpy(outRealMiddleBand, tempBuffer1, …)` | **Safe** — guarded by `if( tempBuffer1 != outRealMiddleBand )`; provably non-overlapping (ASan-clean). |
| **BBANDS** | `bbands.c` `memmove(tempBuffer1, &tempBuffer1[shiftIdx], …)` | **Safe** — same-buffer realign (#99), already `memmove`. |
| **WMA** | `wma.c` `memmove(outReal, &inReal[startIdx], …)` | **Safe** — already `memmove` (the reference pattern the RSI/CMO fixes now match). |

All `memcpy` → `memmove` changes are byte-identical in output (same bytes copied,
overlap handled correctly), so they are numerically invisible: the bit-exact
`--fuzz-064` oracle and the value-comparing range tests are unaffected. In the
Rust backend the copy targets distinct `&mut`/`&` slices (aliasing is impossible),
so the change is comment-only there.

## Residual scope (deliberately not covered)

- **True-max / integer-overflow inputs** (e.g. a period of `INT_MAX` overflowing a
  lookback formula) are left to the ASan/UBSan job rather than forced by the
  boundary sweep: they are values no caller passes and are unsafe to provoke in a
  plain build. The sweep uses an overflow-safe "large" period near the data length.
- **Real-parameter range rejection**: the library does not range-check real
  optional parameters, so the sweep exercises real endpoints for the finite-output
  scan only, not for `TA_BAD_PARAM` behavior.
