# TypeScript API design

**Status:** the public API of the `ta-lib` npm package, unreleased. Values and validation are the C library's; the call shape is TypeScript's. Rows marked Ruled are the owner's (2026-11-04); Adopted rows stand unless he reopens them. Where TypeScript departs from a rule, the [spec](../website/src/spec/README.md#typescript) states it as a TypeScript Caveat (`rJ`); section 9 records what would avoid each.

## 1. Decisions

| # | Decision | Status |
|---|---|---|
| A1 | No npm publish until this design is built | Ruled (owner, 2026-11-04) |
| A2 | One call form: the input arrays, then one optional object, and no second form | Ruled (owner, 2026-11-04) |
| A3 | The object holds the parameters, `startIdx`, `endIdx` and `core`, all optional | Ruled (owner, 2026-11-04) |
| A4 | Multi-output names keep `out`, in camelCase: `outMacd`, `outMacdSignal`, `outMacdHist` | Ruled (owner, 2026-11-04) |
| A5 | A single output is returned as `values` | Ruled (owner, 2026-11-04) |
| A6 | An omitted range is the whole input; `endIdx` is inclusive | Adopted |
| A7 | Caller buffers go in the same object as `out*` keys; the result returns views of them | Adopted |
| A8 | Each output buffer is optional on its own; a missing one is allocated | Adopted |
| A9 | An omitted or `undefined` parameter takes its default, as a sentinel does | Adopted |
| A10 | A rejected lookback is -1 and a rejected display shift `INT_MIN`, each a named constant | Adopted |
| A11 | Lookback, display shift, open and openAndFill take the same object | Adopted |
| A12 | Functions on price series take one `{ open, high, low, close, volume, openInterest }` object | Adopted |
| A13 | Output names drop C's type word: `outRealUpperBand` is `outUpperBand` | Adopted |
| A14 | Multi-output streams return their outputs; the sink is optional | Adopted |
| A15 | Errors are exceptions; `RetCode` is exported | Adopted |
| A16 | An empty history is `TA_INSUFFICIENT_HISTORY` in all five backends | Adopted |
| A17 | No C leftovers in the public types; function names keep the shared rule | Adopted |

## 2. Batch call

```ts
sma(closes)                                              // every default, whole series
sma(closes, { timePeriod: 20 })
sma(closes, { timePeriod: 20, startIdx: 100, endIdx: 199, core })
bbands(closes, { timePeriod: 20, nbDevUp: 2.5, maType: MAType.EMA })
atr({ high, low, close }, { timePeriod: 14 })
cdldoji({ open, high, low, close })
beta(seriesA, seriesB, { timePeriod: 5 })                // plain series: positional, Inputs order
mavp(closes, periods, { minPeriod: 2, maxPeriod: 30 })
```

- **Inputs.** The positional arguments are the function's inputs as the Abstract API lists them: a price input is one object holding the components it reads, keyed `open`, `high`, `low`, `close`, `volume`, `openInterest` (the metadata API's `PriceInput` shape), and any other input is an `ArrayLike<number>`. So `obv(closes, { volume })`, and swapped price arrays cannot compile. Positional names drop C's `in` (`real`, `real0`, `periods`); a price input is `prices`.
- **Parameter keys.** C's name without `optIn`, in camelCase by the word split of [rJ1](../website/src/spec/README.md#rj1): `timePeriod`, `nbDevUp`, `maType`, `fastDMaType`.
- **Range.** `startIdx` defaults to 0 and `endIdx` to the last bar of the first input. `endIdx` is inclusive, as in every TA-Lib API; the TSDoc says so because `slice` is not. Bars before `startIdx` are still read as warm-up, which slicing the input would lose.
- **`core`** defaults to `Core.DEFAULT`.
- **First fault.** What only TypeScript has comes first: that a price object exists and that `options` is an object; then, for a batch call with `endIdx` omitted, each series present and non-empty, because the default reads their lengths; then `core`. Every backend's order follows: the range first, and in an opener the history's upper bound (rS2) ahead of an absent leg, and an empty history after every argument check. [rE1](../website/src/spec/errors/README.md#re1) leaves the order of several faults open.
- **`core`** that is not a `Core` is `TA_BAD_PARAM` at every entry point; lookback and display shift throw for it and for options that are not an object, and answer a rejected parameter with their invalid constant.
- **Function names** keep the rule every backend shares (`cdlengulfing`, `stochrsi`, `var_`).
- **Speed.** The timed benchmark request calls the body, so the options object is on no measured path. A caller who passes every buffer allocates no output (A7).

## 3. Results

```ts
const { begIdx, values } = sma(closes, { timePeriod: 20 });             // Float64Array
const { begIdx, outMacd, outMacdSignal, outMacdHist } = macd(closes);
const { begIdx, values } = cdldoji({ open, high, low, close });         // Int32Array
```

- `values[i]` (or `outX[i]`) is the value for input bar `begIdx + i`; every output is exactly as long as the count, so there is no `count` field.
- An empty result is a success: `begIdx` 0, empty arrays.
- **Output names.** C's output name, camelCase with an acronym as one word (`outMACDSignal` is `outMacdSignal`), C's type word dropped (`outRealUpperBand` is `outUpperBand`, `outRealShort` is `outShort`). The `out` prefix keeps an output from ever meeting `begIdx` or a parameter key.
- The warm-up is not padded; an aligned form is in section 10.

## 4. Caller buffers

```ts
const buf = new Float64Array(closes.length);
const r = sma(closes, { timePeriod: 20, outReal: buf });   // r.values: a view of buf, no copy
macd(closes, { outMacd: a, outMacdSignal: b, outMacdHist: c });   // no output allocated
macd(closes, { outMacd: a });                               // the other two are allocated
```

- A buffer key is the output's result name; a single output's key is `outReal` or `outInteger`.
- Each returned output is `buf.subarray(0, count)`: the same memory, trimmed to the values written. The buffer holds the values from index 0; what lies past them is as rW3 and rW6 say.
- Each buffer is optional on its own. A given buffer that is too short, has the wrong element type, or shares memory with another output is `TA_BAD_PARAM`; an output may be its input (rW6), but not overlap it partly.

## 5. Defaults, sentinels, constants

```ts
sma(closes)                                         // timePeriod 30
sma(closes, { timePeriod: undefined })              // the same
sma(closes, { timePeriod: Core.INTEGER_DEFAULT })   // the same, as in C, Java, C#
```

- `undefined` maps to the sentinel, which the range check takes as the default. Keys are declared `timePeriod?: number | undefined` for `exactOptionalPropertyTypes`.
- Lookback answers `Core.LOOKBACK_INVALID` (-1) and display shift `Core.DISPLAY_SHIFT_INVALID` (`INT_MIN`) for rejected parameters: the values of C, Java and C#, named.

## 6. Lookback, display shift, openers

```ts
smaLookback({ timePeriod: 20 })                                  // 19
macdDisplayShift(1, { fastPeriod: 12 })                          // output index, then the object
const s = smaOpen(history, { timePeriod: 20, core });
const a = atrOpen({ high, low, close }, { timePeriod: 14 });
const { stream, begIdx, values } = smaOpenAndFill(history, { timePeriod: 20 });
```

- Each takes the batch call's options type and ignores the keys it has no use for, so one object serves every call.
- openAndFill returns the stream with the batch result over the history. Its buffers follow section 4, except that an output shares no memory with an input, and each holds `historyLen - lookback` values.

## 7. Streams

```ts
const m = macdOpen(history);
const { outMacd, outMacdSignal } = m.update(101.2);   // a new object
m.update(101.5, sink);                                 // fills and returns sink: no allocation
```

- `update`, `peek` and `value` of a multi-output stream take an optional sink and return it, or a new object. Sink fields carry the result names (`outMacd`); a fresh sink starts at `NaN`.
- `peek` into the sink `update` filled overwrites it.
- A single-output stream returns a `number`. `update` and `peek` take one number per input component, as in every backend.

## 8. Errors and C leftovers

- A rejected call throws `TALibError`; `code` is C's name as a string and `retCode` C's number, compared against the exported `RetCode`.
- **Empty history** (A16): every backend's opener answers an empty history with `TA_INSUFFICIENT_HISTORY` ([rS7](../website/src/spec/streaming/README.md#rs7)), after every argument check, like any too-short history. This is the spec's rule, not a TypeScript Caveat.
- **Messages** name the parameter, its value and its range (`SMA: timePeriod must be in 1..100000, got 0`); overlap gets its own (`MACD: outMacd and outMacdSignal share memory`). Messages are not API.
- **No C leftovers:** no `FuncUnstId.UNUSED_*`; no `ALL` or `ALL_CANDLE_SETTINGS` where a parameter only throws on them; hover text in plain terms ("Exponential moving average", not "The `TA_MAType_EMA` moving average").

## 9. Spec caveats

The spec's [TypeScript Caveats](../website/src/spec/README.md#typescript) state each departure; each amended rule links its caveat. What would avoid each:

| Caveat | To avoid it |
|---|---|
| rJ1 names | Keys spelled exactly as C (`optInTimePeriod`, `outMACDSignal`, `outRealUpperBand`) |
| rJ2 call shape | none: this is A2 |
| rJ3 absent output allocated | none: this is A8 |
| rJ4 lookback takes the options object | none idiomatic |
| rJ5 display shift takes the index first | `(options, index)` with `options` required |

## 10. Later, without breaking anyone

| Item | Note |
|---|---|
| Input-aligned result (`align: 'input'`, NaN warm-up) | An exception to rW3 |
| A stream opened before warm-up (`rsiStream(14)`, `undefined` until warm) | An exception to rS7 |
| A shared stream interface type | For `Map<string, Stream>` and generic drivers |
| Metadata: a `FunctionName` union for `byName`, `setOptInput` by name, boolean flag getters, `PriceInput` keys `\| undefined` | |
| Metadata opens a stream by name | An exception to rH9 |
| `Core.create({ unstablePeriods, candleSettings })` beside the builder | |
| Display shift by output name | |
| `TALibError` fields `funcName`, `argName`; `ErrorOptions` with `cause` | |
| Output type check that survives a second realm (the check is `instanceof`) | |
| TSDoc: `{@link}` instead of site URLs, ranges of real parameters | |
| `export { var_ as var }` for namespace imports | |
| Stream internals non-enumerable | `console.log(stream)` dumps the ring |
| An async-iterable adapter for streams | |
| A `ta-lib/math` subpath for the math transforms (`max`, `sum`, `sqrt`) | Auto-import noise from the root |

