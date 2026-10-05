---
url: 'https://ta-lib.org/spec/inputs-outputs/index.md'
description: >-
  What a TA-Lib call takes and what a successful call writes: what the caller
  must pass, index range, optional parameters, output range and size, argument
  order, integer, declinable and pattern outputs, and computing in place.
---
# Batch Inputs and Outputs

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

What a batch call takes, and what a successful call writes. Rules use C's spelling; [the hub](/spec/#names) maps it onto Rust, Java and C#. What a rejected call reports is on the [errors page](/spec/errors/).

## The caller must {#caller}

Nothing here is checked unless the item says so. A call that breaks an unchecked item can succeed with wrong values.

* <a id="input-length"></a>**Pass full-length inputs.** Every input series a function declares holds at least `endIdx + 1` elements. That includes an input the function never reads ([CDLENGULFING](/functions/cdlengulfing) declares `inHigh` and `inLow` and reads neither), and a range that produces no output. A stream opener's inputs all have the history's length (in C, each holds `historyLen` elements). Rust, Java and C# reject a wrong length ([rB5](/spec/errors/#rb5), [rS5](/spec/streaming/#rs5)); C is handed bare pointers and reads past the end.
* <a id="output-size"></a>**Size every output.** Each output holds at least the `count` the call produces ([rW1](/spec/inputs-outputs/#rw1)), `endIdx - max(startIdx, lookback) + 1` when that is positive, and never less than one element: an empty output is an absent one ([rB4](/spec/errors/#rb4)). `endIdx - startIdx + 1` always suffices. `lookback` is the function's [lookback](/spec/lookback/#rl1) for the same parameters and settings. Sizing methods: [Output Size and Lookback](/api/#output_size). Rust, Java and C# reject a shorter output ([rB5](/spec/errors/#rb5)); C writes past the end.
* <a id="finite-inputs"></a>**Pass only finite values in a series.** A NaN or infinity in a bar a call reads ([rL1](/spec/lookback/#rl1)), or inside the history a stream opens on, is not detected, and nothing is promised about the output or about a handle opened from it. Clean or split the series first. A single bar passed to `Update` or `Peek` is checked ([rU3](/spec/streaming/#ru3)).
* <a id="input-domain"></a>**Keep real inputs within `TA_REAL_MIN` to `TA_REAL_MAX`** (±3e37). Nothing checks the bound, and outside it nothing is defined. The bound limits the domain only; it is not an accuracy promise.
* <a id="overflow"></a>**Expect no overflow detection.** A running sum, a smoothed value or a ratio against a nearly flat window can leave the range of a double on bars that are each finite. Past that point nothing is defined: not the output, not the return code, not a stream handle's state. Open a new handle rather than keep one that went through it.
* <a id="no-overlap"></a>**Do not overlap buffers**, except an output placed whole on an input ([rW7](/spec/inputs-outputs/#rw7)). Buffers that partially overlap (the same memory from a different start, or in a C# batch call a different length), or an output laid over an input of another element type, are not detected in C, where such a call can return `TA_SUCCESS` with wrong values.

## What a call takes

<a id="rp1"></a>**rP1** `startIdx` and `endIdx` are zero-based, inclusive indices into the input series passed. They select the bars to produce output for; bars before `startIdx` are read as history when the lookback needs them. In C# they index the span passed, not the array behind it: a slice holds no bars before its first element. Bounds and codes: [rB1](/spec/errors/#rb1), [rB2](/spec/errors/#rb2).

<a id="rp2"></a>**rP2** Each optional parameter's default and accepted values are per function: the Parameters table of its [function page](/functions/), and the same data through the [Abstract API](/spec/abstract/#provides). A value outside them is rejected, and so is a NaN or infinite value for a real parameter and, in Java, a null `MAType`. A function may also reject a single value inside the listed range, or a combination of values that are each accepted; its page's Notes state it ([FRAMA](/functions/frama) rejects an odd `optInTimePeriod`, [MAVP](/functions/mavp) rejects `optInMinPeriod > optInMaxPeriod`). Rejections: [rB3](/spec/errors/#rb3), [rS3](/spec/streaming/#rs3), [rL2](/spec/lookback/#rl2).

An MA-type parameter accepts every `MAType` member, and a release may add members ([rV3](/spec/versions/#rv3)). To check a raw integer, use the enum your code was built with, never a fixed list: in C the range `TA_MATYPE_MIN` to `TA_MATYPE_MAX`, in Rust `MAType::try_from`, in Java `MAType.values()`, in C# `Enum.IsDefined`.

<a id="rp3"></a>**rP3** A default sentinel selects the function's documented default: `TA_INTEGER_DEFAULT` for an integer parameter, `TA_REAL_DEFAULT` for a real one, the `DEFAULT` member for an MA type (in C, `TA_INTEGER_DEFAULT` works there too). The call is then bit-identical to one passing the default explicitly. A default MA type is not always SMA: read it from the function's page. Spellings per language: [the hub](/spec/#names).

<a id="rp4"></a>**rP4** Float inputs: C's `TA_S_<N>` functions, and the `float[]` (Java) and `ReadOnlySpan<float>` (C#) overloads, widen each element to double when they read it and compute in double. Their outputs are still `double` or `int`, bit-identical to the double call on the same values widened. Rust has no float form, and streams take double only. Storing a series as float rounds it, so a float call can differ from a double call on the original values.

<a id="rp5"></a>**rP5** A batch call takes its arguments in the same order in every language: `startIdx`, `endIdx`, the inputs in the order of the Inputs list on its [function page](/functions/), the optional parameters in the order of its Parameters table, in C `outBegIdx` and `outNBElement`, then the outputs in the order of its Outputs list. Each output is a buffer the caller owns. A function with several outputs reports one range, shared by all of them. How a stream hands back one bar's outputs in each language: [the hub](/spec/#names).

## What a successful call writes

<a id="rw1"></a>**rW1** A successful call reports `begIdx` and `count` (C's `*outBegIdx` and `*outNBElement`). When `count` is above 0, `begIdx` is `max(startIdx, lookback)` and the range ends at `endIdx`: `count = endIdx - begIdx + 1`. Each output is written from index 0, not aligned to the input: `out[i]` is the value for input bar `begIdx + i`, every `i` from 0 to `count - 1` is written, and `out[count - 1]` is the value for `endIdx`.

<a id="rw2"></a>**rW2** A valid range that ends before the lookback (`endIdx < lookback`) succeeds with `count` 0: no error, no values. Test `count`, not `begIdx`.

<a id="rw3"></a>**rW3** Nothing in an output past `count` is written, except where that output is also an input ([rW7](/spec/inputs-outputs/#rw7)). No native API pads the warm-up with NaN or any other fill value.

<a id="rw4"></a>**rW4** What an integer output holds:

* **Pattern outputs** (every `CDL*` output, among others): [rW8](/spec/inputs-outputs/#rw8).
* **Index outputs** ([MININDEX](/functions/minindex), [MAXINDEX](/functions/maxindex), [MINMAXINDEX](/functions/minmaxindex)): the position of a bar in the input passed, not relative to `startIdx` or `begIdx`. Which of several tied bars it names is unspecified.
* **Other integer outputs** (for example [HT_TRENDMODE](/functions/ht_trendmode)): as their function page says.

<a id="rw5"></a>**rW5** An output whose [metadata flags](/spec/abstract/#flags) include `TA_OUT_NULLABLE` (Rust `OutputFlags::NULLABLE`, Java `OutputFlags.NULLABLE`, C# `OutputFlags.Nullable`) may be declined, for example MAMA's `outFAMA`; read the flag rather than a list of names. Decline it with `NULL` in C, `None` in Rust (the parameter is an `Option`), `null` in Java, an empty span such as `default` in C#. It is still computed: every other output is bit-identical to the same call with it supplied, and a stream opened that way still reports its value. No other output can be declined: [rB7](/spec/errors/#rb7), [rS7](/spec/streaming/#rs7), [rU5](/spec/streaming/#ru5).

<a id="rw6"></a>**rW6** A function whose [metadata flags](/spec/abstract/#flags) include `TA_FUNC_FLG_NAN_INF_OUT` (Rust `FuncFlags::NAN_INF_OUTPUT`, Java `FuncFlags.NAN_INF_OUTPUT`, C# `FuncFlags.NanInfOutput`; "Can Output NaN or ±Inf" on its function page) can write NaN or ±Inf in a successful call on ordinary finite input. The Notes on its function page say when.

## Computing in place {#aliasing}

<a id="rw7"></a>**rW7** In a batch call, an output may be the very buffer of an input of the same element type: whole buffer, the same start, and in C# the same span. The outputs are bit-identical to a call with separate buffers. Afterwards, in that buffer, the elements at `[0, count)` hold the output, the elements at `[count, endIdx]` may hold any value, because the call may use them as scratch space for intermediate results, and the elements after `endIdx` keep their input values. Two outputs must be different buffers ([rB6](/spec/errors/#rb6)), and `OpenAndFill` takes no output on an input or on another output ([rS6](/spec/streaming/#rs6)).

| | C | Rust | Java | C# |
|---|---|---|---|---|
| "The same buffer" means | the same pointer | not expressible in safe code | the same array | equal spans: same start and length; at `OpenAndFill`, the same start; a null array is no buffer |
| Output on an input | allowed | not expressible | allowed | allowed |

## Pattern outputs {#patterns}

<a id="rw8"></a>**rW8** A pattern output writes 0 or a sign times a level. Its [metadata flags](/spec/abstract/#flags) say which values it writes and what they mean, and the Output Values table on its function page says what each value means for that pattern.

* **The values.** `TA_OUT_POSITIVE`, `TA_OUT_NEGATIVE` and `TA_OUT_ZERO` are the signs that occur: an output setting any of them sets every sign it writes, and one setting none declares nothing. The levels are 100, plus 80 with `TA_OUT_PATTERN_WEAK` and 200 with `TA_OUT_PATTERN_CONFIRM`. The output writes every declared sign times every level, 0 when `TA_OUT_ZERO` is set, and nothing else. 0 means no pattern on that bar.
* **The sign.**
  * With `TA_OUT_PATTERN_BOOL` it means nothing, and the values are 0 and 100.
  * With `TA_OUT_PATTERN_BULL_BEAR` it is the pattern's call: + bullish, - bearish. No function checks the trend a pattern's definition presumes.
  * With neither, on an integer output of a `TA_FUNC_FLG_CANDLESTICK` function that sets all three sign flags, it is the color of the input bar at the value's own index: + when close >= open, - otherwise. A display shift does not move that bar.
  * Any other output is not a pattern output.
* **The level.** 100 is the pattern, found on this bar, and 80 a weaker form of it. 200 confirms the output's live pattern: its most recent earlier value of level 100 or 80, which has the same sign. A value of level 100 or 80 ends any earlier live pattern, and a 200 ends the one it confirms. The confirmed value can lie before the first value the caller received: before `begIdx`, or in the history a stream opened on. A peek value is never one.
* **Counting.** The patterns found are the values of level 100 or 80; the confirmations are those of level 200.
* **A flag a program does not know.** A later release may add flags. On a pattern output carrying one, the values listed above may not be all, and the sign keeps its meaning.
