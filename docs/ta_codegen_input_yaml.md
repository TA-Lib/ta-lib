---
title: "ta_codegen Input: Metadata (<name>.yaml) Reference"
---

# ta_codegen Input: Metadata (`<name>.yaml`) Reference

The `ta_codegen/input/` directory is the **Interface Definition Language (IDL)** for
TA-Lib functions and the single source of truth for every generated backend. Each
function lives in `ta_codegen/input/<name>/` as a small set of sibling files, each with
its own reference:

| File | Holds | Reference |
|------|-------|-----------|
| `<name>.yaml` | Metadata — **data only, no logic** | **this document** |
| `<name>.c` | The algorithm, in cross-language C | [ta_codegen_input_code.md](ta_codegen_input_code.md) |
| `<name>.md` | Function documentation (prose) | [ta_codegen_input_doc.md](ta_codegen_input_doc.md) |

The `ta_codegen` tool reads these definitions and generates output for all target
languages (C, Rust, Java, .NET). This document specifies the **`<name>.yaml` metadata
schema**.

## Directory Structure

```
ta_codegen/input/
├── enums.yaml              # Shared enum type definitions
├── sma/
│   ├── sma.yaml            # Function metadata
│   └── sma.c               # Function logic (cross-language C)
├── rsi/
│   ├── rsi.yaml
│   └── rsi.c
└── ...
```

## YAML Schema

### Function-level fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | yes | The function's sole identity (e.g., `SMA`, `RSI`), spelled verbatim in every backend (see [Naming](#naming)) |
| `group` | string | yes | Functional category (see [Groups](#groups)) |
| `hint` | string | no | Short description |
| `description` | string | no | Longer description (used if `hint` is absent) |
| `flags` | list | no | Function-level flags (see [Function Flags](#function-flags)) |
| `inputs` | list | yes | Required input parameters |
| `optional_inputs` | list | no | Optional parameters with defaults |
| `outputs` | list | yes | Output parameters |

### Naming

`name` is the only spelling there is. Rust, Java and C# use it verbatim; C alone
prefixes `TA_`. Generated variants append an underscore-separated suffix that
mirrors C minus that prefix:

| C | Rust / Java | C# |
|---|---|---|
| `TA_SMA`, `TA_SMA_Lookback`, `TA_SMA_Open`, `TA_SMA_Stream` | `SMA`, `SMA_Lookback`, `SMA_Open`, `SMA_Stream` | `SMA`, `SMA_Lookback` |

One spelling means nothing can drift out of sync, so pick `name` carefully: it is
the public API in four languages at once. Rust file and module names stay
lower-case (`sma.rs`, `mod sma`) — only public identifiers are verbatim.

### Input parameters

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | yes | Parameter name (e.g., `inReal`, `inReal0`) |
| `type` | string | yes | Data type (see [Input Types](#input-types)) |
| `price_components` | list | for `price` | OHLCV components for a `price`-type input (e.g., `[high, low, close]`) |

### Optional input parameters

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | yes | Parameter name (e.g., `optInTimePeriod`) |
| `type` | string | yes | Data type: `integer`, `real`, or `enum:EnumName` |
| `display_name` | string | no | UI-friendly label (e.g., `"Time Period"`) |
| `hint` | string | no | Help text |
| `range` | [min, max] | no | Valid value range |
| `default` | number | no | Default value when not specified |
| `flags` | list | no | Parameter flags (see [Optional Input Flags](#optional-input-flags)) |
| `suggested` | [start, end, step] | no | Optimization search space hints |

### Output parameters

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | yes | Parameter name (e.g., `outReal`) |
| `type` | string | yes | Data type: `real` or `integer` |
| `flags` | list | no | Display/value hints (see [Output Flags](#output-flags)) |

## Types

### Input Types

| YAML type | C type | Rust type | Description |
|-----------|--------|-----------|-------------|
| `real` | `const double[]` | `&[f64]` | Floating-point array |
| `integer` | `const int[]` | `&[i32]` | Integer array |
| `price` | multiple arrays | multiple slices | Candlestick / OHLCV data |

A `price` input names its required OHLCV components via `price_components`; the
generator expands it into one array parameter per component:

```yaml
inputs:
  - name: inPriceHLC
    type: price
    price_components: [high, low, close]
```

Available components: `open`, `high`, `low`, `close`, `volume`, `open_interest`

### Enum Types

Optional inputs can reference enum types defined in `enums.yaml`:

```yaml
optional_inputs:
  - name: optInMAType
    type: enum:MAType
    default: 0
```

Each backend renders enums appropriately:
- **C**: `TA_MAType` (typedef'd enum)
- **Java**: `MAType` (Java enum)
- **Rust**: `i32` (the raw variant value)
- **.NET**: `MAType` (C# enum)

## Flags

### Function Flags

| Flag | Description | C equivalent |
|------|-------------|--------------|
| `overlap` | Output overlaps the price chart (same scale) | `TA_FUNC_FLG_OVERLAP` |
| `unstable_period` | Has an initial unstable calculation period | `TA_FUNC_FLG_UNST_PER` |
| `volume` | Output is based on volume data | `TA_FUNC_FLG_VOLUME` |
| `candlestick` | Output is a candlestick pattern signal | `TA_FUNC_FLG_CANDLESTICK` |
| `stream` | Generate the streaming API (Open/Update/Peek/…) | `TA_FUNC_FLG_STREAM` |
| `path_dependent` | Absolute output depends on `startIdx` and never converges across ranges (a running accumulation seeded at the first bar, or a path-dependent state machine); the same bar computed from a different `startIdx` can differ | `TA_FUNC_FLG_PATH_DEP` |
| `nan_inf_output` | Some inputs of ordinary magnitude have no finite result, so a successful call can write NaN or ±Inf | `TA_FUNC_FLG_NAN_INF_OUT` |
| `period1_identity` | A period of 1 performs no smoothing: the lookback is 0 and every output value is a bit-exact copy of its input value | `TA_FUNC_FLG_PERIOD1_IDENTITY` |

```yaml
flags: [overlap, unstable_period]
```

`path_dependent` is public `ta_abstract` metadata (issue #127): a wrapper reads
it from `TA_FuncInfo.flags`, and the `ta_regtest` range sweep reads the same bit
to decide it cannot cross-compare the function's values across ranges. Dropping
it is fail-safe — the sweep then value-compares the function and fails loudly if
it is genuinely start-dependent (issue #98).

`nan_inf_output` (issue #191) marks the seven functions with a hole in their own
domain — `ACOS`/`ASIN` outside [-1,1], `LN`/`LOG10`/`SQRT` on a negative value
(and `LN`/`LOG10` on zero, which is -Inf), `DIV` on 0/0 or x/0, `VWMA` on a
window with no volume at all. Each one's `<name>.md` says when, in a `## Notes`
bullet, and the website renders the flag as a `Can Output NaN or ±Inf` display
flag. It is not set for a non-finite value that only appears once the
intermediate arithmetic overflows on the *input* magnitudes themselves (around
1e160 and up), which is a property of `double`, not of the indicator.

`NVI` and `PVI` were the eighth and ninth candidates and are deliberately not
flagged: they are running *products* with no upper bound, so sustained gains on
their qualifying bars used to compound past the range of a double (reachable in
~460 bars with prices held inside [1, 100], and hit for real by the 2000-bar
random dataset). Rather than declare that, both now carry the last representable
value forward instead of writing Inf — the `IS_FINITE(...)` guard in
`input/nvi/nvi.c` and `input/pvi/pvi.c`. No other library guards this (Tulip,
ta4j and bukosabino/ta all compound unguarded; pandas-ta side-steps it by
summing returns instead of compounding them), so the choice was ours to make.

The flag is a **contract, not an annotation**: `test_abstract.c` holds every
function *without* it to finite output across all five of its datasets
(negative, zero, two epsilon sets, random), so adding a function that emits NaN
or Inf on ordinary input fails the suite until the flag — and the `## Notes`
sentence explaining when — are written.

`period1_identity` (issue #184) is the same shape of contract, for the promise
that a period of 1 returns the input untouched. It has to be *declared* because
the two ways of honouring it are indistinguishable in the source: `SMA`'s window
math is already exact at a period of 1, while `EMA`'s recurrence reduces to
`(x - prev) + prev` and needs an explicit arm — so a generator cannot tell "needs
no arm" from "forgot one". What the declaration buys is that the promise is
checked: `test_period_boundary.c` sweeps every flagged function at a period of 1
and requires lookback 0 plus a bit-exact copy, on the reference series and on two
series built to break the naive forms, through the batch, `TA_S_`, streaming and
cross-language surfaces.

Membership is gated from both ends in `ta_codegen/generator/tests/period1_suite.rs`:

- Every `MAType` member that resolves to a function with a period must carry it.
  A moving average that does not copy its input at a period of 1 is not a moving
  average. (`MAMA` is exempt — its parameters are the real fast/slow limits, so it
  has no period to set to 1.)
- Every function whose `.c` carries a recognisable identity arm must carry it,
  which is the half that catches the real omissions: `VWMA` hand-copied the arm
  and stayed outside the sweep for a release, because it is not a `MAType` and
  nothing tied the two together.

The reverse implication does not hold and is not asserted — a flagged function
need not have an arm (`SMA`, `TRIMA`), and `MACD`/`MACDFIX` have an arm shape
that is deliberately *not* this flag: only their signal stage degenerates at
`signalPeriod == 1`, the MACD line is still computed, so their output is not a
copy of anything.

The flag states what the **public domain** offers, not what the body contains, and
the two can differ: `RSI` and `CMO` carry the arm but declare `range: [2, …]`, so a
period of 1 is rejected before any of it runs. They are therefore unflagged and
outside the sweep, and the arm gate pins that (`UNREACHABLE_ARM`) rather than
leaving it to be rediscovered — flagging them would have the metadata promise a
call that returns `TA_BAD_PARAM`.

### Optional Input Flags

| Flag | Description | C equivalent |
|------|-------------|--------------|
| `is_percent` | Value is a percentage | `TA_OPTIN_IS_PERCENT` |
| `is_degree` | Value is 0-360 degrees | `TA_OPTIN_IS_DEGREE` |
| `is_currency` | Value is a currency amount | `TA_OPTIN_IS_CURRENCY` |
| `advanced` | Rarely-changed parameter (hide in simple UIs) | `TA_OPTIN_ADVANCED` |

### Output Flags

**Display hints** (how to render):

| Flag | Description | C equivalent |
|------|-------------|--------------|
| `line` | Connected line (default) | `TA_OUT_LINE` |
| `dot_line` | Dotted line | `TA_OUT_DOT_LINE` |
| `dash_line` | Dashed line | `TA_OUT_DASH_LINE` |
| `dot` | Individual dots | `TA_OUT_DOT` |
| `histogram` | Bar chart / histogram | `TA_OUT_HISTO` |

**Pattern hints** (for candlestick pattern outputs):

| Flag | Description | C equivalent |
|------|-------------|--------------|
| `pattern_bool` | 0 = no pattern, != 0 = pattern exists | `TA_OUT_PATTERN_BOOL` |
| `pattern_bull_bear` | >0 = bullish, <0 = bearish, 0 = none | `TA_OUT_PATTERN_BULL_BEAR` |
| `pattern_strength` | 0..100 = bullish, -100..0 = bearish | `TA_OUT_PATTERN_STRENGTH` |

**Value range hints**:

| Flag | Description | C equivalent |
|------|-------------|--------------|
| `positive` | Output can be positive | `TA_OUT_POSITIVE` |
| `negative` | Output can be negative | `TA_OUT_NEGATIVE` |
| `zero` | Output can be zero | `TA_OUT_ZERO` |
| `upper_limit` | Values are upper bounds (e.g., upper Bollinger Band) | `TA_OUT_UPPER_LIMIT` |
| `lower_limit` | Values are lower bounds (e.g., lower Bollinger Band) | `TA_OUT_LOWER_LIMIT` |

Multiple flags combine in a list:

```yaml
outputs:
  - name: outMACDHist
    type: real
    flags: [histogram]
  - name: outRealUpperBand
    type: real
    flags: [line, upper_limit]
```

## Groups

Functions are organized into these categories:

| Group | Examples |
|-------|----------|
| Math Operators | MULT, ADD, SUB, DIV |
| Math Transform | SQRT, LN, LOG10, CEIL |
| Overlap Studies | SMA, EMA, BBANDS, MA |
| Momentum Indicators | RSI, MACD, STOCH, ADX |
| Volume Indicators | OBV, AD, ADOSC |
| Volatility Indicators | ATR, NATR, TRANGE |
| Cycle Indicators | HT_DCPERIOD, HT_TRENDMODE |
| Pattern Recognition | CDL* (candlestick patterns) |
| Statistic | STDDEV, VAR, CORREL |
| Price Transform | AVGPRICE, MEDPRICE, TYPPRICE |

## enums.yaml

Shared enum definitions used by `enum:` type references:

```yaml
MAType:
  c_prefix: TA_MAType_
  variants:
    - { name: SMA,   value: 0 }
    - { name: EMA,   value: 1 }
    - { name: WMA,   value: 2 }
    - { name: DEMA,  value: 3 }
    - { name: TEMA,  value: 4 }
    - { name: TRIMA, value: 5 }
    - { name: KAMA,  value: 6 }
    - { name: MAMA,  value: 7 }
    - { name: T3,    value: 8 }
    - { name: HMA,   value: 9 }
    - { name: DISABLED, value: 10 }
```

Each enum declares a `c_prefix` — the C constant prefix — plus its `variants`.
A variant has:

- `name` — the variant identity. Used verbatim wherever a backend renders the
  enum as a type (`MAType.SMA` in Java and C#, `FuncUnstId::HT_DCPERIOD` in
  Rust), and appended to `c_prefix` for C (`TA_MAType_SMA`).
- `value` — the pinned integer. Part of the ABI: identical in every backend, and
  append-only. Retire a variant by reserving its slot (`UNUSED_<n>`), never by
  deleting or renumbering it.

## C Logic Files

The `.c` file in each function directory contains the actual computation logic, written in cross-language C using macros from `ta_defs.h`. See [ta_codegen_input_code.md](ta_codegen_input_code.md) for the macro reference.

## Complete Example

```yaml
# ta_codegen/input/sma/sma.yaml
name: SMA
group: Overlap Studies
hint: Simple Moving Average
flags: [overlap]
inputs:
  - name: inReal
    type: real
optional_inputs:
  - name: optInTimePeriod
    type: integer
    display_name: Time Period
    hint: Time period
    range: [2, 100000]
    default: 30
    suggested: [4, 200, 1]
outputs:
  - name: outReal
    type: real
    flags: [line]
```

## Migration from C Tables

This YAML IDL replaces the C-based abstract tables in `src/ta_abstract/tables/`. The mapping:

| C source | YAML equivalent |
|----------|----------------|
| `table_*.c` TA_FuncDef entries | `<func>/<func>.yaml` |
| `ta_group_idx.c` group list | `group:` field in each YAML |
| `TA_FuncInfo.flags` | `flags:` list |
| `TA_InputParameterInfo` | `inputs:` list |
| `TA_OptInputParameterInfo` | `optional_inputs:` list |
| `TA_OutputParameterInfo` | `outputs:` list |
| `TA_IntegerRange` / `TA_RealRange` | `range:` + `suggested:` |
| `TA_MA_TypeList` / `TA_IntegerList` | `type: enum:MAType` in `enums.yaml` |
