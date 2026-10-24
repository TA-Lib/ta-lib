---
url: 'https://ta-lib.org/spec/inputs-outputs/index.md'
description: >-
  One batch calling pattern for every function in C, Rust, Java and C#: the same
  argument order, a sentinel that selects any parameter's default, outputs
  holding only defined values from index 0 and how to size them, pattern
  outputs, computing in place, and what the caller must pass.
---
# Batch Inputs and Outputs

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

The rules for what a batch call takes and what a successful call writes, in C's spelling ([names](/spec/#names)). What a rejected call reports is on the [errors page](/spec/errors/).

A batch call runs tight loops over the series: on modern AMD, Intel and Apple processors, most functions take under 10 nanoseconds per bar at their default parameters, in each of the four languages, and the slowest, such as HT_SINE, a few hundred.

## The caller must {#caller}

* <a id="input-length"></a>**Pass full-length inputs.** Every input series a function declares holds at least `endIdx + 1` elements ([rB5](/spec/errors/#rb5)), even when the range produces no output ([rW2](/spec/inputs-outputs/#rw2)); a stream opener's inputs all have the history's length ([rS5](/spec/streaming/#rs5)).
* <a id="output-size"></a>**Size every output.** Each output holds at least `max(1, count)` elements ([rB4](/spec/errors/#rb4), [rB5](/spec/errors/#rb5)), where `count` is the one the call produces ([rW1](/spec/inputs-outputs/#rw1)). `endIdx - startIdx + 1` always suffices. Sizing methods: [Output Size and Lookback](/api/#output_size).
* <a id="finite-inputs"></a>**Pass only finite values in a series.** A NaN or infinity in a bar a call reads ([rL1](/spec/lookback/#rl1)), or inside the history a stream opens on, is not detected, and nothing is promised about the output or about a handle opened from it. Clean or split the series first. A single bar passed to `Update` or `Peek` is checked ([rU3](/spec/streaming/#ru3)).
* <a id="input-domain"></a>**Expect no promise beyond `TA_REAL_MIN` to `TA_REAL_MAX`** (±3e37). This is the tested domain, not a limit: nothing checks it.
* <a id="overflow"></a>**Expect no overflow detection.** A running sum, a smoothed value or a ratio against a nearly flat window can leave the range of a double on bars that are each finite. Past that point nothing is defined: not the output, not the return code, not a stream handle's state. Open a new handle rather than keep one that went through it.
* <a id="no-overlap"></a>**Do not overlap buffers**, except an output placed whole on an input ([rW6](/spec/inputs-outputs/#rw6)). C detects neither partial overlap, such as the same memory from a different start, nor an output over an input of another element type: the call can return `TA_SUCCESS` with wrong values.

## What a call takes

<a id="rp1"></a>**rP1** `startIdx` and `endIdx` are zero-based, inclusive indices into the input series passed. They select the bars to produce output for; bars before `startIdx` are read as history when the lookback needs them. In C# they index the span passed, not the array behind it: a slice holds no bars before its first element. Bounds and codes: [rB1](/spec/errors/#rb1), [rB2](/spec/errors/#rb2).

<a id="rp2"></a>**rP2** Each optional parameter's default and accepted values are per function: the Parameters table of its [function page](/functions/), and the same data through the [Abstract API](/spec/abstract/#provides). A value outside them is rejected, and so is a NaN or infinite value for a real parameter and, in Java, a null `MAType`. A function may also reject a single value inside the listed range, or a combination of values that are each accepted; its page's Notes state it ([FRAMA](/functions/frama) rejects an odd `optInTimePeriod`, [MAVP](/functions/mavp) rejects `optInMinPeriod > optInMaxPeriod`). Rejections: [rB3](/spec/errors/#rb3), [rS3](/spec/streaming/#rs3), [rL2](/spec/lookback/#rl2).

An MA-type parameter accepts every `MAType` member, and a release may add members ([rV3](/spec/versions/#rv3)). To check a raw integer, use the enum your code was built with: in C the range `TA_MATYPE_MIN` to `TA_MATYPE_MAX`, in Rust `MAType::try_from`, in Java `MAType.values()`, in C# `Enum.IsDefined`.

<a id="rp3"></a>**rP3** A default sentinel selects the function's documented default: `TA_INTEGER_DEFAULT` for an integer parameter, `TA_REAL_DEFAULT` for a real one, the `DEFAULT` member for an MA type (in C, `TA_INTEGER_DEFAULT` works there too). The call is then bit-identical to one passing the default explicitly. A default MA type is not always SMA.

<a id="rp4"></a>**rP4** Float inputs: C's `TA_S_<N>` functions, and the `float[]` (Java) and `ReadOnlySpan<float>` (C#) overloads, widen each element to double when they read it. Their outputs are still `double` or `int`, bit-identical to the double call on the same values widened. Rust has no float form, and streams take double only. Storing a series as float rounds it, so a float call can differ from a double call on the original values.

<a id="rp5"></a>**rP5** A batch call takes its arguments in the same order in every language: `startIdx`, `endIdx`, the inputs in the order of the Inputs list on its [function page](/functions/), the optional parameters in the order of its Parameters table, in C `outBegIdx` and `outNBElement`, then the outputs in the order of its Outputs list. Each output is a buffer the caller owns. A function with several outputs reports one range, shared by all of them.

## What a successful call writes

<a id="rw1"></a>**rW1** A successful call reports `begIdx` and `count` (C's `*outBegIdx` and `*outNBElement`). When `count` is above 0, `begIdx` is `max(startIdx, lookback)`, where `lookback` is the function's [lookback](/spec/lookback/#rl1) for the same parameters and settings, and the range ends at `endIdx`: `count = endIdx - begIdx + 1`. Each output is written from index 0, not aligned to the input: `out[i]` is the value for input bar `begIdx + i`, for every `i` from 0 to `count - 1`.

<a id="rw2"></a>**rW2** A valid range that ends before the lookback (`endIdx < lookback`) succeeds with `count` 0. Test `count`, not `begIdx`.

<a id="rw3"></a>**rW3** Nothing in an output past `count` is written, except where that output is also an input ([rW6](/spec/inputs-outputs/#rw6)). No native API pads the warm-up with NaN or any other fill value.

<a id="rw4"></a>**rW4** What an integer output holds:

* **Pattern outputs**: [rW7](/spec/inputs-outputs/#rw7).
* **Index outputs** ([MININDEX](/functions/minindex), [MAXINDEX](/functions/maxindex), [MINMAXINDEX](/functions/minmaxindex)): the position of a bar in the input passed, not relative to `startIdx` or `begIdx`. Of several tied bars it names the most recent.
* **Other integer outputs** (for example [HT_TRENDMODE](/functions/ht_trendmode)): as their function page says.

<a id="rw5"></a>**rW5** A function whose [metadata flags](/spec/abstract/#flags) include `TA_FUNC_FLG_NAN_INF_OUT` ("Can Output NaN or ±Inf" on its function page) can write NaN or ±Inf in a successful call on ordinary finite input. The Notes on its function page say when.

## Computing in place {#aliasing}

<a id="rw6"></a>**rW6** In a batch call, an output may be the same buffer as an input of the same element type: in C the same pointer, in Java the same array, in C# a span of the same start and length. Rust's safe code cannot express it. The outputs are bit-identical to a call with separate buffers. Afterwards, in that buffer, the elements at `[0, count)` hold the output, the elements at `[count, endIdx]` may hold any value, because the call may use them as scratch space for intermediate results, and the elements after `endIdx` keep their input values.

## Pattern outputs {#patterns}

<a id="rw7"></a>**rW7** A pattern output is an output carrying `TA_OUT_PATTERN_BOOL` or `TA_OUT_PATTERN_BULL_BEAR`, or any integer output of a `TA_FUNC_FLG_CANDLESTICK` function. It writes 0 (no pattern) or a sign times a level. Its [metadata flags](/spec/abstract/#flags) say which values it writes and what they mean, and the Output Values table on its function page says what each value means for that pattern.

* **The values.** The flags list every value the output can write, and it writes no other:

  * Its levels are 100 always, 80 too if it carries `TA_OUT_PATTERN_WEAK`, and 200 too if it carries `TA_OUT_PATTERN_CONFIRM`.
  * `TA_OUT_POSITIVE` means it can write each level as a positive value, `TA_OUT_NEGATIVE` each level as a negative value, and `TA_OUT_ZERO` 0.

  | Sign flags | Level flag | Values | For example |
  |---|---|---|---|
  | `TA_OUT_POSITIVE`, `TA_OUT_ZERO` | none | 0, 100 | [CDLHAMMER](/functions/cdlhammer) |
  | `TA_OUT_NEGATIVE`, `TA_OUT_ZERO` | none | -100, 0 | [CDLHANGINGMAN](/functions/cdlhangingman) |
  | `TA_OUT_POSITIVE`, `TA_OUT_NEGATIVE`, `TA_OUT_ZERO` | none | -100, 0, 100 | [CDLMARUBOZU](/functions/cdlmarubozu) |
  | `TA_OUT_POSITIVE`, `TA_OUT_NEGATIVE`, `TA_OUT_ZERO` | `TA_OUT_PATTERN_WEAK` | -100, -80, 0, 80, 100 | [CDLENGULFING](/functions/cdlengulfing) |
  | `TA_OUT_POSITIVE`, `TA_OUT_NEGATIVE`, `TA_OUT_ZERO` | `TA_OUT_PATTERN_CONFIRM` | -200, -100, 0, 100, 200 | [CDLHIKKAKE](/functions/cdlhikkake) |

* **The sign.**
  * With `TA_OUT_PATTERN_BOOL` the output has two states, 0 for no pattern and non-zero for the pattern.
  * With `TA_OUT_PATTERN_BULL_BEAR` it is the pattern's call: + bullish, - bearish.
  * With neither, it is the color of the bar the value is for: + when close >= open, - otherwise.

* **The level.** 100 is the pattern, found on this bar, and 80 a weaker form of it. 200 is not a new pattern: it confirms the output's live pattern, which has the same sign. The live pattern is the output's most recent value of level 100 or 80, provided no 200 came after it: a pattern is confirmed at most once. The live pattern can lie before the first value the caller received: before `begIdx`, or in the history a stream opened on.
