# RFC: Arrow API

Status: draft, for discussion.

## Summary

Two layers:

- **Arrow API**, the interop edge: a C export helper and a Python facade that
  takes and returns Arrow arrays.
- **Segmented batch**, underneath: one new entry per function, in C and Rust,
  that reads Arrow arrays in place, in one chunk or several: the C Data
  Interface struct in C, a safe view of the same layout in Rust. A caller
  without Arrow (NumPy, plain C) wraps its buffer in one.

TA-Lib takes no dependency on any Arrow library.

## Motivation

**Missing values inside a series.** Non-finite input values are unsupported by
contract and not detected. Wrappers pass a missing value to the kernels as NaN,
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

A wrapper can solve both on its own, by walking its partitions and nulls
itself. Solving them in core gives every wrapper one definition of the result,
under the same gates, instead of one implementation per wrapper.

## Terms

- **Partition.** A contiguous range of rows the caller declares independent,
  such as one symbol in a panel. Given as offsets, like an Arrow list array's.
- **Column.** One input: one or more chunks, each an Arrow array, as an Arrow
  chunked array holds them.
- **Validity.** A bitmap with one bit per row, set meaning valid, in Arrow's
  convention, so an Arrow caller passes its null bitmap unchanged.
- **Segment.** A run of valid, finite rows in a partition, bounded by nulls or
  the partition's ends. It may span chunks. Computed by core, never passed in.

## Semantics

- **Input control.** `skip-on-null`, the default, skips a null row and carries
  the state across it. `reset-on-null` restarts the calculation after it.
- **Output control.** `fill-previous`, the default, fills a null input row with
  the previous row's output; a pattern-recognition output fills zero in place of
  the previous value. `keep-null` makes the row null. Rows before a
  calculation's first finite output are null under both.
- **Partitions.** Nothing carries across a partition boundary.
- **Non-finite is null.** A NaN or infinite value in a valid input row makes
  that row null for the call, so no calculation uses it. A non-finite result
  makes that row null. This departs from Arrow, where NaN is a value and not a
  null, because a non-finite price is never a value a function can use.
- **Output alignment.** Every output has one row per input row; output row `i`
  corresponds to input row `i`.
- **One validity per call.** A row is a null row for the call when any input is
  null there, such as a missing low in a price bar. Every output is null on the
  same rows, so all outputs share one bitmap. A null output row holds NaN in a
  real output and 0 in an integer output; since 0 is also a valid integer value
  (a candlestick reporting no pattern), only the validity tells them apart.

Under `reset-on-null`, a window function of length N warms up again over the N-1
rows after a null: the rows whose window contains it, as in pandas and Polars
rolling windows.

### Chaining

An output's validity is the next call's input validity, with the same
partitions and input control, so a chain needs no step between calls. A filled
row is valid, so the next call counts it as a bar. With `keep-null` on every
call but the last and finite intermediate results, RSI over EMA equals batch
RSI over batch EMA, as the equivalence gate compares: segment by segment under
`reset-on-null`, over each partition's valid rows under `skip-on-null`.

## Segmented batch

The partitions and the input columns are described by caller-owned descriptors
that hold references only; core allocates nothing for them and keeps nothing
after the call returns.

```c
typedef struct TA_Column {
   const struct ArrowSchema       *schema;   /* NULL means float64 */
   const struct ArrowArray *const *chunks;   /* borrowed, never released */
   int64_t                         nChunks;
} TA_Column;

typedef struct TA_Partitions {
   unsigned int    flags;         /* TA_PARTITIONS_RESET_ON_NULL, _KEEP_NULL */
   int64_t         len;           /* rows in every input and output */
   const int64_t  *offsets;       /* nPartitions+1 entries, or NULL */
   int64_t         nPartitions;
} TA_Partitions;

TA_RetCode TA_SMA_Segmented(const TA_Partitions *partitions,
                            const TA_Column *inReal,
                            int optInTimePeriod,
                            double outReal[],
                            uint8_t outValidity[]);
```

- **Chunks** share their column's schema: float64, or a list or large list of
  float64; any other type answers `TA_BAD_PARAM`. They are read in place: a flat
  chunk's values and validity at its `offset`; a list chunk's offsets and list
  validity at its own `offset`, its values and their validity at its child's. A
  list chunk's rows are the values its lists cover. A null list that covers rows
  answers `TA_BAD_PARAM`. A plain C caller wraps its buffer in an `ArrowArray`
  whose release does nothing, with a NULL schema. The chunks of an input add up
  to `len` rows; otherwise `TA_BAD_PARAM`. Each input may be chunked
  differently.
- **Offsets** give the partitions of flat inputs: non-decreasing, from 0 to
  `len`. An empty partition is allowed. A caller whose partitions are not
  contiguous reorders the rows first. NULL means one partition of `len` rows.
  List inputs take one partition per list instead: offsets must be NULL, every
  input must be a list, and their lists must have the same lengths; otherwise
  `TA_BAD_PARAM`.
- **Validity** is Arrow's, per chunk: row `r` is bit `offset + r`,
  least-significant first within each byte. A NULL validity buffer, or a
  `null_count` of 0, means every row is valid. Core ANDs the inputs' validities
  of a multi-input function (high, low, close).
- **Outputs** are one buffer each of `len` rows, with one validity for the call,
  row `r` at bit `r`; the caller allocates them. A NULL validity leaves only the
  NaN (or 0) values to mark null rows.
- **Flags.** `TA_PARTITIONS_RESET_ON_NULL` selects `reset-on-null`, and
  `TA_PARTITIONS_KEEP_NULL` selects `keep-null`.
- With no offsets, no validity, one flat chunk per input and finite inputs, the
  result is today's batch result in input alignment within
  [rH1](https://ta-lib.org/spec/streaming/#rh1)'s exceptions, a non-finite
  result null.

Rust takes the same partitions and, per chunk, a safe view of the same layout:
the values slice, the validity bitmap and its bit offset, and a list's offsets
and list validity. It is reached through a facade on `Core` rather than one
more method per function beside batch:

| | C | Rust |
|---|---|---|
| batch | `TA_SMA` | `core.sma(...)` |
| segmented batch | `TA_SMA_Segmented` | `core.segmented().sma(...)` |

Rust takes no Arrow type: the crate forbids unsafe code, which reading an
`ArrowArray` needs, and Polars uses its own Arrow crate rather than arrow-rs.
Native arrow-rs and polars-arrow arrays expose these slices without unsafe
code, so every Rust caller fills the view without a copy.

### Chunks

A pyarrow `ChunkedArray`, a pandas column in Arrow mode, or a Parquet file read
with several row groups is one column in several buffers. Concatenating copies
it, and restarting at each chunk changes values at boundaries the storage
chose. So the calculation continues across every input's chunk boundaries,
under either input control:

- The result equals the same inputs in one chunk, within
  [rH1](https://ta-lib.org/spec/streaming/#rh1)'s exceptions.
- Partitions are rows of the whole column, so a partition may span chunks. A
  chunked list column goes in one call: core reads each chunk's lists in place,
  rebased to rows of the column.

## Arrow API

- **Chunked columns** arrive as an `ArrowArrayStream`, which the caller walks,
  keeping each array until the call returns, or until the last export that keeps
  it is released.
- **C export.** `TA_ArrowExport` wraps an output buffer and the call's validity
  in an `ArrowArray` without copying; every output of a call shares that
  validity buffer. For a list input, it can instead make one list array per
  input chunk over the matching slice of the output, with that chunk's `offset`,
  list validity and offsets. The offsets are reused, with the child starting the
  chunk's first offset before the slice, when that offset is no larger than the
  slice's first output row; otherwise they are rebased from that index. The
  caller moves each such chunk into the export: the exports of a call share it,
  and it is released on the last release, as the shared validity is. The caller
  owns the output buffers, so it supplies the release callback, as C code that
  outlives them: a Python callable can be freed before the last consumer
  releases the array.
- **No dependency.** The two struct definitions are copied in under the
  `ARROW_C_DATA_INTERFACE` guard, as the Arrow specification intends.
- **Python.** `talib.arrow.SMA(x)` takes any object exposing `__arrow_c_array__`
  (a pyarrow `Array`) or `__arrow_c_stream__` (a Polars Series, a pandas 3.1
  Series, a pyarrow `ChunkedArray`) and returns an Arrow array. A pandas 2.2
  Series has no `__arrow_c_stream__`; the facade takes `pyarrow.array(series)`,
  which returns the series' own chunks. A NumPy array goes to the same entry
  with no validity, and its NaN rows are null rows by default. The facade
  belongs to ta-lib-python.
- **pandas without a copy.** Both pandas routes hand core each chunk at
  the series' own address, sliced columns and columns with nulls included, and
  hand back the output at the address core wrote it to, as an Arrow-backed or
  a `float64` Series. Measured with pandas 2.2.3 and 3.1.1 on pyarrow 26.0.1.

The export helper is written once, not per function.

## Scope

C and Rust. Java and C# get the entry when a user asks for it: their users run
bar by bar, which streaming serves, and adding it later is not a breaking
change.

## Cost

- No validity, finite inputs, one flat chunk per input: what a per-partition
  loop of batch calls in a wrapper costs, without the wrapper's per-call
  boundary.
- Non-finite checking: about one vectorised compare per row per input, and per
  row per output.
- Validity: one scan a word at a time, in which a fully valid word costs one
  compare.
- Per null, under `skip-on-null`: no output lost.
- Per null, under `reset-on-null`: `lookback` rows of output lost to warm-up.
  Frequent nulls with a long lookback leave little output.

## Gates

- **Equivalence, every function:** under `reset-on-null`, the output equals
  plain batch calls run segment by segment; under `skip-on-null`, a plain batch
  call over each partition's valid rows, compacted, with each result put back on
  its row. Null rows filled as the output control says, and NaN (or 0) and a
  cleared bit in every null output row. Equal within
  [rH1](https://ta-lib.org/spec/streaming/#rh1)'s exceptions, compared as
  streaming's gates compare.
- **Output control:** `fill-previous`, `keep-null` and a pattern's zero, on null
  rows before and after the first output, under each input control, and a
  partition whose first `lookback + 1` valid rows have nulls between them.
- **No validity, one flat chunk per input, finite inputs:** the output equals
  today's batch output in input alignment within rH1's exceptions, a non-finite
  result null.
- **Partitions:** adjacent partitions with no null between them, an empty
  partition, a partition no longer than `lookback`, and a null on a partition's
  first and last row.
- **Bit offsets:** input chunks, flat and list children, at offsets not a
  multiple of 8, and lengths not a multiple of 64. Getting these wrong produces
  wrong nulls with no error.
- **Non-finite:** NaN and infinite inputs become nulls; a
  non-finite result (`LN` of a negative) nulls its row.
- **Chaining:** two chained calls, with `keep-null` on the first, equal batch
  over batch under each input control, compared as the equivalence gate
  compares.
- **Malformed input:** offsets decreasing, not starting at 0 or not ending at
  `len` answer `TA_BAD_PARAM`; so do chunks not adding up to `len`, an
  unsupported schema, offsets with list inputs, list and flat inputs mixed, and
  list inputs whose lists differ in length.
- **Chunked equals unchunked,** every function, both input controls, compared as
  streaming's gates compare: a boundary inside the warm-up, on a null and right
  after one, inside a partition, and inputs chunked at different rows; inputs of
  one call with different validities, bit offsets and chunk boundaries.
- **Arrow round trip:** a call and export over a float64 column, a list and a
  large list, sliced and not, chunked and not, read the caller's buffers in
  place and reproduce the input; any other type answers `TA_BAD_PARAM`, and so
  does a null list covering rows.
- **pandas round trip** (ta-lib-python): pandas 2.2 and 3.x Series in Arrow
  mode, sliced and multi-chunk, reach core and come back without a copy.
- **Rust equals C**, through ta_regtest.

## Open questions

- The ASF trademark policy, before a released function or package name contains
  "Arrow".
- The null value of an integer output: 0, or a sentinel.
