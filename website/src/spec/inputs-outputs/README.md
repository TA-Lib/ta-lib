---
title: Inputs and Outputs
description: "What a TA-Lib call accepts and writes: index range, input lengths, optional parameters, value domain, non-finite values, output range and size, argument order, integer and declinable outputs, and aliasing."
---

*Part of TA-Lib's exhaustive [specifications](/spec/), intended for precise AI-agent-driven integration with TA-Lib, to minimize errors.*

The contract a correct call meets, and what a successful call writes. Rules use C's spelling; [the hub](/spec/#names) maps it onto Rust, Java and C#. Each rule links or states what happens when a call breaks it; the codes are on the [errors page](/spec/errors/).

## Index range

<a id="i1"></a>**I1** `startIdx` and `endIdx` are zero-based, inclusive indices into the input series passed. They select the bars to produce output for; bars before `startIdx` are read as history when the lookback needs them. In C# they index the span passed, not the array behind it: a slice holds no bars before its first element, so its lookback comes from inside the slice. Bounds and codes: [B1](/spec/errors/#b1), [B2](/spec/errors/#b2).

## Input series

<a id="i2"></a>**I2** Every input series a function declares holds at least `endIdx + 1` elements, including one it never reads ([CDLENGULFING](/functions/cdlengulfing) declares `inHigh` and `inLow` and reads neither) and including a range that produces no output. A stream opener's inputs all have the history's length (in C, each holds `historyLen` elements). Checked under [B5](/spec/errors/#b5) and [S5](/spec/streaming/#s5); C cannot check.

## Optional parameters

<a id="i3"></a>**I3** Each optional parameter's default and accepted values are per function: the Parameters table of its [function page](/functions/), and the same data through the abstraction layer (in C, `TA_GetOptInputParameterInfo`). A NaN or infinite value is outside every real parameter's accepted values, and in Java so is a null `MAType`. A function may also reject a single value inside the listed range, which its page's Notes state: current behaviour, [FRAMA](/functions/frama) rejects an odd `optInTimePeriod`. And a function may reject a combination of values that are each accepted: current behaviour, [MAVP](/functions/mavp) alone does, rejecting `optInMinPeriod > optInMaxPeriod`. Rejections: [B3](/spec/errors/#b3), [S3](/spec/streaming/#s3), [L1](/spec/lookback/#l1).

An MA-type parameter accepts every `MAType` member, and a release may add members ([V3](/spec/versions/#v3)). To check a raw integer, use the enum your code was built with, never a fixed list: in C the range `TA_MATYPE_MIN` to `TA_MATYPE_MAX`, in Rust `MAType::try_from`, in Java `MAType.values()`, in C# `Enum.IsDefined`.

<a id="n3"></a>**N3** A default sentinel selects the function's documented default: `TA_INTEGER_DEFAULT` for an integer parameter, `TA_REAL_DEFAULT` for a real one, the `DEFAULT` member for an MA type (in C, `TA_INTEGER_DEFAULT` works there too). The call is then bit-identical to one passing the default explicitly. A default MA type is not always SMA: APO's is EMA. Spellings per language: [the hub](/spec/#names).

## Values

<a id="i4"></a>**I4** Every real input value is expected within `TA_REAL_MIN` to `TA_REAL_MAX` (±3e37). Nothing checks the bound: a finite value outside it is accepted everywhere, by `Update` and `Peek` too, and outside it nothing is defined. Non-finite values: [I5](/spec/inputs-outputs/#i5). The bound limits the domain only; it is not an accuracy promise.

<a id="i5"></a>**I5** A non-finite value (NaN, ±Inf) is handled by where it arrives:

| Arrives as | Result |
|---|---|
| an element of an input series, or of an opener's history | Undefined. Not detected; nothing is promised about the output, or about a handle opened from it. |
| a bar passed to Update or Peek | Rejected: [U3](/spec/streaming/#u3), [H4](/spec/streaming/#h4) |
| a real optional parameter | Rejected ([I3](/spec/inputs-outputs/#i3)): [B3](/spec/errors/#b3), [S3](/spec/streaming/#s3), [L1](/spec/lookback/#l1) |
| a candle setting's factor | The setter's rule: [G6](/spec/settings-threads/#g6) |

<a id="i6"></a>**I6** Float inputs: C's `TA_S_<N>` functions, and the `float[]` (Java) and `ReadOnlySpan<float>` (C#) overloads, widen each element to double when they read it and compute in double. Their outputs, still `double` or `int`, are bit-identical to the double call on the same values widened. Rust has no float form, and streams take double only. Storing a series as float rounds it, so a float call can differ from a double call on the original values.

## Output range

<a id="o1"></a>**O1** A successful call reports `begIdx` and `count` (C's `*outBegIdx` and `*outNBElement`). When `count` is above 0, `begIdx` is `max(startIdx, lookback)` ([L5](/spec/lookback/#l5)) and the range ends at `endIdx`: `count = endIdx - begIdx + 1`. Each output is written from index 0, not aligned to the input: `out[i]` is the value for input bar `begIdx + i`, every `i` from 0 to `count - 1` is written, and `out[count - 1]` is the value for `endIdx`.

<a id="n1"></a>**N1** A valid range that ends before the lookback (`endIdx < lookback`) succeeds with `count` 0: no error, no values. Test `count`; `begIdx` then carries nothing (current behaviour: 0).

<a id="n2"></a>**N2** Nothing in an output past `count` is written, except where that output is also an input ([N4](/spec/inputs-outputs/#n4)). No native API pads the warm-up with NaN or any other fill value.

<a id="o2"></a>**O2** Each output needs `count` elements: `endIdx - max(startIdx, lookback) + 1` when that is positive, none otherwise; `endIdx - startIdx + 1` always suffices. `lookback` is the function's lookback for the same parameters and settings ([L5](/spec/lookback/#l5)). Sizing methods: [Output Size and Lookback](/api/#output_size). A shorter output: [B5](/spec/errors/#b5).

## Argument order and several outputs

<a id="o3"></a>**O3** A batch call takes its arguments in the same order in every language: `startIdx`, `endIdx`, the inputs in the order of the Inputs list on its [function page](/functions/), the optional parameters in the order of its Parameters table, in C `outBegIdx` and `outNBElement`, then the outputs in the order of its Outputs list. Each output is a buffer the caller owns. A function with several outputs reports one range, shared by all of them. How a stream hands back one bar's outputs in each language: [the hub](/spec/#names).

## Integer outputs

<a id="o4"></a>**O4** What an integer output holds:

- **Candlestick patterns** (`CDL*`): 0 means no pattern on that bar. The sign is the pattern's direction (+ bullish, - bearish) or, for some, only the candle's color (+ white, - black); a pattern with neither, such as [CDLDOJI](/functions/cdldoji), reports +100. Current behaviour: every value is 0, ±80, ±100 or ±200. Which values a pattern emits, and what each means, is in the Output Values table on its function page.
- **Index outputs** ([MININDEX](/functions/minindex), [MAXINDEX](/functions/maxindex), [MINMAXINDEX](/functions/minmaxindex)): the position of a bar in the input passed (in C#, the span), not relative to `startIdx` or `begIdx`. Which of several tied bars it names is unspecified. In a stream: [H9](/spec/streaming/#h9).
- **Other integer outputs** (for example [HT_TRENDMODE](/functions/ht_trendmode)): as their function page says.

## Declinable outputs

<a id="o5"></a>**O5** An output whose metadata flags include `TA_OUT_NULLABLE` (Rust `OutputFlags::NULLABLE`, Java `OutputFlags.NULLABLE`, C# `OutputFlags.Nullable`) may be declined. Current behaviour: MAMA's `outFAMA` is the only one; read the flag rather than rely on that. Decline it with `NULL` in C, `None` in Rust (the parameter is an `Option`), `null` in Java, an empty span such as `default` in C#. It is still computed: every other output is bit-identical to the same call with it supplied, and a stream opened that way still reports its value. Any other output cannot be declined; what passing it null or empty does: [B6a](/spec/errors/#b6a), [S6a](/spec/streaming/#s6a), [U6a](/spec/streaming/#u6a).

## Non-finite outputs

<a id="o6"></a>**O6** A function whose flags include `TA_FUNC_FLG_NAN_INF_OUT` (Rust `FuncFlags::NAN_INF_OUTPUT`, Java `FuncFlags.NAN_INF_OUTPUT`, C# `FuncFlags.NanInfOutput`; "Can Output NaN or ±Inf" on its function page) can write NaN or ±Inf in a successful call on ordinary finite input. The Notes on its function page say when. Current behaviour: the test suite holds every function without the flag to finite output on its datasets.

<a id="o7"></a>**O7** Intermediate overflow: a running sum, a smoothed value or a ratio against a nearly flat window can leave the range of a double on bars that are each finite. Past that point nothing is defined: not the output, not the return code, not a stream handle's state. Treat such a handle as spent and open a new one. No flag marks this.

## Aliasing

<a id="n4"></a>**N4** In the batch tier, an output may be the very buffer of an input of the same element type: whole buffer, the same start, and in C# the same span. The outputs are bit-identical to a call with separate buffers, and afterwards the buffer holds the output in `[0, count)`. Past `count` it is not promised to keep the input: current behaviour, [STOCH](/functions/stoch), [STOCHF](/functions/stochf) and [KDJ](/functions/kdj) leave intermediate values there when their first output is in place. Two outputs on one buffer: [B6](/spec/errors/#b6). `OpenAndFill` takes no output on an input or on another output: the identical buffer is rejected ([S6](/spec/streaming/#s6)), and a partial overlap there is [N8](/spec/inputs-outputs/#n8).

<a id="n8"></a>**N8** Any other shared memory is unspecified: buffers that partially overlap (the same memory from a different start, or in C# a different length), or an output laid over an input of another element type, such as a `TA_S_` call's double output over its float input. Detection stops at identity ([B6](/spec/errors/#b6)); in C such a call can return `TA_SUCCESS` with wrong values. Current behaviour, not to be relied on: C# rejects as `TA_BAD_PARAM` any overlap between two outputs, any overlap between an output and an input other than N4's case, and any overlap across element types.

| | C | Rust | Java | C# |
|---|---|---|---|---|
| "The same buffer" means | the same pointer | not expressible in safe code | the same array | equal spans: same start and length; a null array is no buffer |
| Output on an input (N4) | allowed | not expressible | allowed | allowed |
| N8's cases expressible | yes | no, in safe code | no | yes |
| N8's cases detected | no | n/a | n/a | yes (current behaviour) |
