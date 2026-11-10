# RFC: TA-Lib Arrow API

Status: draft, for discussion.

Last Update: 2026-11-09

## Summary

Two layers:

- **Arrow API**: a Python facade that takes and returns Arrow arrays, and a C
  helper that exports results as Arrow arrays.
- **Segmented batch**: one new entry per function, in C and Rust, that reads
  Arrow arrays in place, in one chunk or several. C reads the Arrow C Data
  Interface struct; Rust reads a safe view of the same layout. A caller without
  Arrow (NumPy, plain C) wraps its buffer in one.

TA-Lib depends on no Arrow library.

## Motivation

**Missing values inside a series.** Today a NaN or infinity in the input is
unsupported and not detected. Wrappers pass a missing value as NaN, and
functions react differently: some propagate NaN, some recover, and some (RSI
among them) return wrong finite values for every later bar. We want simple
deterministic behavior to eliminate unexpected NaN.

**Many short series per call.** A common table workload is a panel: one
column with many symbols, computed per symbol (`.over("symbol")` in Polars,
`groupby(...).transform(...)` in pandas). Today each symbol is a separate call,
and the call overhead far exceeds the calculation: in a Polars plugin computing
ATR on 20-row groups, a call costs about 32,700 CPU instructions, of which the
ATR calculation is about 600. Only fewer calls remove that overhead.

A wrapper could solve both itself. Solving them in the TA-Lib C and Rust Core
gives every wrapper the same result.

## Terms

- **Partition.** A range of consecutive rows computed independently, such as
  one symbol in a panel.
- **Column.** One input, as one or more Arrow arrays (chunks).
- **Validity.** Arrow's null bitmap: one bit per row, set when the row is
  valid.
- **Segment.** A run of valid, finite rows in a partition, between nulls or the
  partition's ends. It may span chunks. TA-Lib Core finds segments and
  processes every one of their rows.

![Partitions, Arrow chunks, validity bitmaps and segments for a two-input function, and its output under keep-null and fill-previous](RFC-Arrow-API-terms.svg)

## Semantics

- **Skip on null.** A null input row is skipped, and the state carries across
  it.
- **Output control.** On a null input row, `keep-null` (the default) makes the
  output null. Alternatively, the `fill-previous` option copies the previous
  row's output and validity; a pattern-recognition output gets a valid 0
  instead. Before the partition's first output, the row is null. The lookback
  (warm-up), counted in non-null rows, always outputs null.
- **Partitions.** Nothing carries from one partition to the next.
- **Non-finite is null.** A NaN or infinite input makes its row null, so no
  calculation uses it. A non-finite result in any output makes its row null in
  every output, under either output option. Arrow treats NaN as a value, not a
  null; TA-Lib does not, because a non-finite price is never usable.
- **Alignment.** Output row `i` belongs to input row `i`. An index output
  (MAXINDEX) counts positions from its partition's first row, which is 0; nulls
  count.
- **Validity on multi-input.** A row is null when any input is null there, such
  as a missing low in a price bar.

### Chaining

An output's validity serves as the next call's input validity, so calls chain
directly. Whatever the output option, the result is deterministic: a NaN no
longer causes undefined behavior.

## Segmented batch

The caller owns these structs. They only point at its data; Core allocates
nothing for them and keeps nothing after the call.

```c
typedef struct TA_Column {
   const struct ArrowSchema       *schema;   /* NULL means float64 */
   const struct ArrowArray *const *chunks;   /* borrowed, never released */
   int64_t                         nChunks;
} TA_Column;

typedef struct TA_Partitions {
   unsigned int    flags;         /* TA_PARTITIONS_FILL_PREVIOUS */
   int64_t         len;           /* rows in every input and output */
   const int64_t  *offsets;       /* nPartitions+1 entries, or NULL */
   int64_t         nPartitions;   /* ignored when offsets is NULL */
} TA_Partitions;

TA_RetCode TA_SMA_Segmented(const TA_Partitions *partitions,
                            const TA_Column *inReal,
                            int optInTimePeriod,
                            double outReal[],
                            uint8_t outValidity[]);
```

- **Chunks** share their column's type: float64, or a list or large list of
  float64. Core reads them in place. An input's chunks add up to `len` rows, and
  inputs may be chunked differently.
- **Offsets** split flat inputs into partitions, from 0 to `len`; NULL means one
  partition. A list input has one partition per list; the rows of a null list
  are null.
- **Validity** follows Arrow, per chunk: row `r` is bit `offset + r`,
  least-significant bit first. A NULL buffer, or a `null_count` of 0, means
  every row is valid. For a multi-input function (high, low, close), Core ANDs
  the inputs' bitmaps. Inputs chunked alike may share one bitmap.
- **Outputs** are buffers of `len` rows that the caller allocates, plus one
  validity bitmap for the call, row `r` at bit `r`. A null row holds NaN, or 0
  in an integer output. The bitmap is optional, but without it a null row and a
  candlestick with no pattern both read 0.
- **Flags.** `TA_PARTITIONS_FILL_PREVIOUS` selects `fill-previous`.
- With no offsets, no validity, one flat chunk per input and finite inputs, the
  result matches today's batch result in input alignment within
  [rH1](https://ta-lib.org/spec/streaming/#rh1)'s exceptions, with a non-finite
  result null.

Rust takes the same partitions, and per chunk a safe view: the values slice,
the validity bitmap with its bit offset, and a list's offsets and validity. It
is a facade on the Rust `Core` struct, not a second method per function:

| | C | Rust |
|---|---|---|
| batch | `TA_SMA` | `core.sma(...)` |
| segmented batch | `TA_SMA_Segmented` | `core.segmented().sma(...)` |

Rust takes no Arrow type: the crate forbids unsafe code, which reading an
`ArrowArray` needs, and Polars uses its own Arrow crate, not arrow-rs. Native
arrow-rs and polars-arrow arrays expose these slices safely, so Rust callers
fill the view without copying.

### Chunks

A pyarrow `ChunkedArray`, a pandas column in Arrow mode, or a Parquet file with
several row groups is one column split across buffers. Concatenating would copy
it, and restarting at each chunk would change values at boundaries the storage
chose. So the calculation runs on across chunk boundaries:

- The result equals the same input in one chunk, within
  [rH1](https://ta-lib.org/spec/streaming/#rh1)'s exceptions.
- Partition offsets count rows of the whole column, so a partition may span
  chunks. A chunked list column goes in one call; Core reads each chunk's lists
  in place, rebased to rows of the column.

## Arrow API

Python, sketched for ta-lib-python:

```python
import polars as pl
import talib.arrow as ta

bars = pl.read_parquet("bars.parquet")          # symbol, high, low, close

# One series: columns go in and come back without a copy, chunked or not.
ibm = bars.filter(pl.col("symbol") == "IBM")
atr = ta.ATR(ibm["high"], ibm["low"], ibm["close"], timeperiod=14)

# A panel: one list per symbol, one partition each.
closes = bars.group_by("symbol", maintain_order=True).agg("close")["close"]
sma = ta.SMA(closes, timeperiod=20)

# The output option.
sma = ta.SMA(closes, timeperiod=20, fill_previous=True)
```

C, for a float64 column that arrived in two chunks:

```c
const struct ArrowArray *chunks[] = { &close0, &close1 };
TA_Column     inReal = { NULL, chunks, 2 };     /* NULL schema: float64 */
TA_Partitions parts  = { 0, len, NULL, 0 };    /* one partition, keep-null */

double  *out   = malloc(len * sizeof *out);
uint8_t *valid = malloc((len + 7) / 8);
TA_RetCode rc = TA_SMA_Segmented(&parts, &inReal, 20, out, valid);

struct ArrowArray result;                     /* back to Arrow, no copy */
rc = TA_ArrowExport(&result, out, valid, len, free_buffers);
```

- **Chunked columns** arrive as an `ArrowArrayStream`. The caller walks it and
  keeps each array until the call returns.
- **C export.** `TA_ArrowExport` wraps an output and the call's validity in an
  `ArrowArray` without copying; all outputs of a call share the validity
  buffer. The caller owns the buffers and supplies the release callback, in C:
  a Python callable can be freed before the last consumer lets go of the array.
- **No dependency.** The two struct definitions are copied in under the
  `ARROW_C_DATA_INTERFACE` guard, as the Arrow specification intends.
- **Python.** The facade takes any object with `__arrow_c_array__` or
  `__arrow_c_stream__` (pyarrow, Polars, a pandas 3.1 Series). For a pandas 2.2
  Series it calls `pyarrow.array(series)`, which returns the series' own chunks.
  A NumPy array has no validity, and its NaN rows count as null. The facade
  belongs to ta-lib-python.
- **pandas without a copy.** On both pandas routes, Core reads each chunk at
  the series' own address, and the output reaches the returned Series,
  Arrow-backed or `float64`, at the address Core wrote it. Measured with pandas
  2.2.3 and 3.1.1 on pyarrow 26.0.1.

## Scope

Only C and Rust are planned. Native support in Java, C# and TypeScript may
follow if demand arises.

## Gates

Every gate compares within [rH1](https://ta-lib.org/spec/streaming/#rh1)'s
exceptions, as streaming's gates do.

- **Equivalence, every function and output:** each output equals one plain
  batch call over each partition's non-null rows packed together, with each
  result put back on its row and an index output converted from a position
  among the packed rows to that row's position in the partition. Null rows
  follow the output control, a non-finite result in any output nulls its row
  in every output under either option, and a null output holds NaN (or 0) with
  a cleared bit.
- **Output control:** `fill-previous`, `keep-null` and a pattern's zero, on null
  rows before and after the first output, a run of several nulls, a partition
  whose first `lookback + 1` non-null rows have nulls between them; each with
  and without an output bitmap.
- **Partitions,** under each output option: adjacent partitions with no null
  between them, an empty partition, a partition with at most `lookback` non-null
  rows, a null on a partition's first and last row, and a null list.
- **Bit offsets:** input chunks, flat and list children, at offsets not a
  multiple of 8, and lengths not a multiple of 64. A mistake here gives wrong
  nulls with no error.
- **Nulls and non-finite:** a null in one input only; NaN, with its validity bit
  set or cleared, and infinite inputs become nulls; a non-finite result (`LN` of
  a negative) nulls its row.
- **Malformed input** (bad offsets, chunks not adding up to `len`, an
  unsupported type) answers `TA_BAD_PARAM`.
- **Chunked equals unchunked,** every function: a boundary inside the warm-up,
  on a null and right after one, inside a partition and on a partition
  boundary; inputs chunked at different rows, or with different validities, bit
  offsets and chunk boundaries in one call.
- **Arrow round trip:** a float64 column, a list and a large list, sliced or
  not, chunked or not, read in place and exported back unchanged.
- **pandas round trip** (ta-lib-python): pandas 2.2 and 3.x Series in Arrow
  mode, sliced and multi-chunk, reach Core and come back without a copy.
- **Rust equals C**, through ta_regtest.
