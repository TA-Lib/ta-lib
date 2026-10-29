# RFC: Arrow API

Status: draft, for discussion.

## Summary

Two layers:

- **Arrow API**, the interop edge: C helpers that read and build Arrow C Data
  Interface structs without copying, and a Python facade that takes and returns
  Arrow arrays. Arrow structures cross here and nowhere else.
- **Segmented batch**, underneath: one new entry per function, in C and Rust,
  that takes plain buffers in Arrow's memory layout. A caller without Arrow
  (NumPy, plain C) uses it directly.

TA-Lib takes no dependency on any Arrow library.

## Motivation

**Missing values inside a series.** Non-finite input values are unsupported by
contract and not detected. Bindings pass a missing value to the kernels as NaN,
and the result is not uniform: some functions propagate NaN, some recover, and
several (RSI among them) return finite wrong values for every later bar. Every
table-level layer must therefore guard, each writes its own guard, and no two
agree on what follows a gap.

**Many short series per call.** The most common table workload is a panel: one
column holding many symbols, computed per symbol (`.over("symbol")` in Polars,
`groupby(...).transform(...)` in pandas). Each symbol is a separate call today,
and the call boundary costs far more than the kernel. Measured under callgrind
on a Polars plugin (ATR, 20-row groups), a call retires about 32,700
instructions, of which the kernel and its entry are about 600. Only fewer calls
can reach the rest.

A binding can solve both on its own, by walking its partitions and nulls and
calling the batch kernel on each stretch. Solving them in core gives every
binding one definition of the result, under the same gates, instead of one
implementation per binding.

## Terms

- **Partition.** A contiguous range of rows the caller declares independent,
  such as one symbol in a panel. Given as offsets, like an Arrow list array's.
- **Validity.** A bitmap with one bit per row, set meaning valid, in Arrow's
  convention, so an Arrow caller passes its null bitmap unchanged.
- **Segment.** A maximal stretch of valid, finite rows inside one partition.
  Computed by core, never passed in. It is the unit the kernel restarts on.

## Semantics

- **Restart.** The batch kernel runs on each segment separately, from a fresh
  state. Nothing carries across a partition boundary or a null row.
- **Non-finite is null, by default.** A NaN or infinite value in a valid input
  row makes that row null for the call, so the kernel never sees one. A
  non-finite result makes that output row null. A flag turns both off for a
  caller that guarantees finite data. This departs from Arrow, where NaN is a
  value and not a null, because a non-finite price is never a value a function
  can use.
- **Output alignment.** Every output has one row per input row; output row `i`
  corresponds to input row `i`. Rows outside the partitions are not written.
- **Output nulls.** A row is null in every output if its input row is null, or
  if it falls in its segment's first `lookback` rows; a segment no longer than
  `lookback` is null throughout. A non-finite result nulls only its own output.
  A null row holds NaN in a real output and 0 in an integer output; since 0 is
  also a valid integer value (a candlestick reporting no pattern), only the
  output validity tells them apart.
- **One validity per output.** Each output has its own bitmap, as each Arrow
  array does.

For a window function of length N, a null row nulls that row and the next N-1
rows of its partition: exactly the rows whose window contains it, which matches
pandas and Polars rolling windows. A recursive function warms up again after the
gap.

### Chaining

An output's validity is the next call's input validity, with the same
partitions. Nulls only accumulate along a chain: a call keeps its input's nulls
and adds warm-up rows at the start of each segment, which shortens the segment
without splitting it, while a non-finite result splits it. So a chain needs no
step between calls, and RSI over EMA equals batch RSI over batch EMA, segment by
segment, bit for bit.

## Segmented batch

The inputs' layout is described by a caller-owned descriptor that holds
references only; core allocates nothing for it and keeps nothing after the call
returns.

```c
typedef struct TA_Segments {
   size_t          size;          /* sizeof(TA_Segments) */
   unsigned int    flags;         /* TA_SEGMENTS_KEEP_NONFINITE */
   int64_t         len;           /* rows in every input and output buffer */
   const int64_t  *offsets;       /* nPartitions+1 entries, or NULL */
   int64_t         nPartitions;
   const uint8_t  *validity;      /* or NULL */
   int64_t         validityBit;
} TA_Segments;

TA_RetCode TA_SMA_Segmented(const TA_Segments *segments,
                            const double inReal[],
                            int optInTimePeriod,
                            double outReal[],
                            uint8_t *outRealValidity,
                            int64_t outValidityBit);
```

- **Offsets** are non-decreasing and within `len`; they need not start at 0,
  because a sliced Arrow list array's do not, and the rows they cover are
  `offsets[0]` up to `offsets[nPartitions]`. An empty partition is allowed.
  NULL means one partition of `len` rows. They are 64-bit, as in Arrow's large
  list; a 32-bit list caller widens one value per partition. A caller whose
  partitions are not contiguous reorders the rows first.
- **Validity** is a pointer plus a bit offset: row `r` is bit `validityBit + r`,
  least-significant first within each byte, as an Arrow array's `offset` places
  it. One bitmap covers every input: the caller ANDs those of a multi-input
  function (high, low, close). NULL means every row is valid.
- **Output validity** is per call and per output, so it is an argument, not part
  of the descriptor. Each pointer may be NULL, and then only the NaN (or 0)
  values mark that output's null rows.
- **`size`** lets a later field (a skip mode) stay ABI-compatible.
- With neither offsets nor validity, and the flag set, the result is today's
  batch result in input alignment.

Rust takes the same descriptor with slices, through a facade on `Core` rather
than one more method per function beside batch:

| | C | Rust |
|---|---|---|
| batch | `TA_SMA` | `core.sma(...)` |
| segmented batch | `TA_SMA_Segmented` | `core.segmented().sma(...)` |

Rust takes no Arrow type: the crate forbids unsafe code, which reading an
`ArrowArray` needs, and Polars uses its own Arrow crate rather than arrow-rs, so
slices are what every Rust caller can pass without a copy.

### Implementation

- **Scan ahead.** An iterator finds the next segment from the offsets and the
  validity, 64 rows a word, with a trailing-zero count on each word that holds
  a null. With non-finite checking on, it ANDs each word with the inputs'
  finiteness on the fly, so no scratch bitmap is allocated.
- **Kernels unchanged.** Each segment goes to the existing numerics tier, with
  the parameters validated once per call, so bit-identity with batch holds by
  construction. A segment no longer than `lookback` skips the kernel.
- **Output validity** is the effective input validity with each segment's first
  `lookback` bits cleared, written a word at a time from the segment bounds;
  the kernel never touches it. Non-finite results then clear their own bits.

Later optimisations fit behind the same signature: scratch reused across
segments by functions that compose others, and several partitions computed in
lockstep, one per SIMD lane.

## Arrow API

- **C import.** `TA_ArrowImport` reads an `ArrowArray` and its `ArrowSchema`, a
  float64 column or a list or large list of float64, and fills a
  `TA_Segments` and a values pointer without copying. Any other type answers
  `TA_BAD_PARAM`.
- **C export.** `TA_ArrowExport` wraps an output buffer and its validity in an
  `ArrowArray` without copying. The caller owns those buffers, so it supplies
  the release callback.
- **No dependency.** The two struct definitions are copied in under the
  `ARROW_C_DATA_INTERFACE` guard, as the Arrow specification intends.
- **Python.** `talib.arrow.SMA(x)` takes any object exposing
  `__arrow_c_array__` (Polars, pyarrow, DuckDB) and returns an Arrow array. A
  NumPy array goes to the same entry with no validity, and its NaN rows come
  back null by default. The facade belongs to ta-lib-python.

Both helpers are written once, not per function.

## Scope

C and Rust. Java and C# get the entry when a user asks for it: their users run
bar by bar, which streaming serves, and adding it later is not a breaking
change.

## Cost

- Flag set, no validity: one kernel entry per partition, which a per-partition
  loop in a binding would pay, without the binding's per-call boundary.
- Non-finite checking: about one vectorised compare per row per input, and per
  row per output.
- Validity: one scan a word at a time, in which a fully valid word costs one
  compare.
- Per segment: one kernel entry, a few hundred instructions.
- Per null: `lookback` rows of output lost to warm-up. Frequent nulls with a
  long lookback leave little output; carrying state across nulls (skip
  semantics) is out of scope here and can be added later as a flag.

## Gates

- **Equivalence, every function:** the output equals plain batch calls run
  segment by segment, bit for bit, with NaN (or 0) and a cleared bit in every
  null and warm-up row.
- **No validity, flag set:** the output equals today's batch output in input
  alignment.
- **Partitions:** adjacent partitions with no null between them, an empty
  partition, a partition no longer than `lookback`, a null on a partition's
  first and last row, and offsets not starting at 0.
- **Bit offsets:** input and output bitmaps at offsets not a multiple of 8, and
  lengths not a multiple of 64. Getting these wrong produces wrong nulls with no
  error.
- **Non-finite:** NaN and infinite inputs become nulls by default and reach the
  kernel with the flag set; a non-finite result (`LN` of a negative) nulls only
  its own output.
- **Chaining:** two chained calls equal batch over batch on each segment.
- **Malformed offsets:** decreasing, or beyond `len`, answer `TA_BAD_PARAM`.
- **Arrow round trip:** import and export of a float64 column, a list and a
  large list, sliced and not, keep the caller's buffers (no copy) and reproduce
  the input; any other type answers `TA_BAD_PARAM`.
- **Rust equals C**, through ta_regtest.

## Open questions

- One validity per input, ANDed on the fly by the iterator, instead of one the
  caller ANDs: it saves an Arrow caller a buffer and a pass.
- The ASF trademark policy, before a released function or package name contains
  "Arrow".
- The null value of an integer output: 0, or a sentinel.
