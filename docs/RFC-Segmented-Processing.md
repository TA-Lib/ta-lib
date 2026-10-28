# RFC: Segmented Processing

Status: draft, for discussion.

## Motivation

Two problems share one solution.

**Many short series per call.** The most common table workload is a panel: one
column holding many symbols, computed per symbol (`.over("symbol")` in Polars,
`groupby(...).transform(...)` in pandas). Each group is a separate call today,
and the call boundary costs far more than the kernel. Measured under callgrind
on a Polars plugin (ATR, 20-bar groups), a call retires about 32,700
instructions, of which the kernel and its entry are about 600. The rest is the
binding's per-call work: decoding parameters, importing and exporting the
series. No kernel optimisation can reach it; only fewer calls can.

**Missing values inside a series.** Non-finite input values are unsupported by
contract and not detected. Bindings pass a missing value to the kernels as NaN,
and the result is not uniform: some functions propagate NaN, some recover, and
several (RSI among them) return finite wrong values for every later bar. Every
table-level layer must therefore guard, and each one writes its own guard.

## Proposed solution

A second batch entry per function that takes a validity bitmap for the input
and writes a validity bitmap for the output. Core treats each run of valid rows
as an independent series.

### Semantics

- **Run.** A maximal sequence of consecutive rows whose input bit is set.
- **Restart.** The existing batch kernel runs on each run separately, from a
  fresh state. Nothing carries across a gap.
- **Output alignment.** Output row `i` corresponds to input row `i`; the output
  is the full input length, not packed from 0 as in the batch tier.
- **Output nulls.** A row is null if its input bit is clear, or if it falls in
  its run's warm-up (the first `lookback` rows of the run). A null row's value
  is NaN for real outputs and 0 for integer outputs. At the output, null and NaN
  mean the same thing.
- **No input detection.** Core does not inspect values. A NaN in a row marked
  valid is garbage in, garbage out, as today. Detecting it is the caller's job.

For a window function of length N, a single invalid row nulls that row and the
next N-1: exactly the rows whose window contains it, which matches pandas and
Polars rolling windows. A recursive function warms up again after the gap.

### Bitmap convention

Arrow's, so Arrow-backed callers pass their buffers unchanged:

- bit set means valid; bits are least-significant first within each byte;
- every bitmap is passed as a pointer plus a bit offset, because a sliced
  array's bitmap starts mid-byte;
- one input bitmap per call: the caller ANDs the bitmaps of a multi-input
  function (high, low, close) before calling;
- a null input bitmap means every row is valid, which reproduces today's batch
  result in output-aligned form;
- the output bitmap is optional; when absent, only the NaN (or 0) values mark
  null rows.

All outputs of a function share one output bitmap.

### Shape

C, with the name to be settled against the naming spec:

```c
TA_RetCode TA_<N>_Segmented(int len,
                            const double in<X>[], ...,
                            const unsigned char *validIn, size_t validInBit,
                            <params>,
                            double out<Y>[], ...,
                            unsigned char *validOut, size_t validOutBit);
```

Rust, Java and C# carry the same contract in their own idiom (a byte or `u64`
slice plus a bit offset; `long[]` in Java and C#).

The entry is generated per function and contains no numerics of its own: it
walks the bitmap a 64-bit word at a time and calls the existing batch kernel on
each run. The ABI change is additive.

### Callers

- **Arrow-backed (Polars, pandas with Arrow dtypes, DuckDB).** Pass the AND of
  the inputs' validity bitmaps; attach the output bitmap to the result array.
  Group boundaries in a panel become cleared bits or separate runs, so a whole
  panel is one call.
- **NumPy.** Pass no bitmap for today's behaviour. To treat NaN as missing,
  build the bitmap with one scan; Core offers that scan as a shared helper so
  each binding does not write its own.
- **A Float64 column holding NaN as a value** (possible in Polars) is not
  covered by its null bitmap. The layer either documents NaN as the user's
  responsibility or adds the helper's scan.

## Cost

- Clean data with no bitmap: the batch kernel plus one pass to find that there
  is a single run.
- Per run: one batch entry, a few hundred instructions.
- Per gap: `lookback` rows of output lost to warm-up. Frequent gaps with a long
  lookback leave little output; carrying state across gaps (skip semantics) is
  out of scope here and can be added later as a mode without changing the
  bitmap contract.

## Gates

- **Equivalence, every function:** the segmented output equals plain batch
  calls run by run, bit for bit, with NaN (or 0) and a cleared bit in every gap
  and warm-up row.
- **Clean data:** with no bitmap, the output equals today's batch output
  shifted to input alignment.
- **Bit offsets:** input and output bitmaps at offsets not a multiple of 8, and
  lengths not a multiple of 64. Getting these wrong produces wrong nulls with no
  error.
- **Helper agreement:** NaN-marked input through the scan helper equals the
  same input with an explicit bitmap.

## Open questions

- Entry name, and whether the scan helper is a separate function or a mode of
  the entry.
- Whether a separate `lookback`-per-run report is needed, or the output bitmap
  is enough.
- Confirm that no function needs a different validity per output
  (`TA_<N>_DisplayShift` is the place to check).
- Whether Java and C# need this entry at all, or only C and Rust.
