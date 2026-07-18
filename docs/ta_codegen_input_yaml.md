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
| `<name>.md` | Function documentation (prose) — _proposed_ | [ta_codegen_input_doc.md](ta_codegen_input_doc.md) |

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
| `name` | string | yes | Uppercase function name (e.g., `SMA`, `RSI`) |
| `camel_case` | string | no | PascalCase name for Java/.NET (e.g., `Sma`) |
| `group` | string | yes | Functional category (see [Groups](#groups)) |
| `hint` | string | no | Short description |
| `description` | string | no | Longer description (used if `hint` is absent) |
| `flags` | list | no | Function-level flags (see [Function Flags](#function-flags)) |
| `inputs` | list | yes | Required input parameters |
| `optional_inputs` | list | no | Optional parameters with defaults |
| `outputs` | list | yes | Output parameters |

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
- **Rust**: `i32` (integer constants)
- **.NET**: `int`

## Flags

### Function Flags

| Flag | Description | C equivalent |
|------|-------------|--------------|
| `overlap` | Output overlaps the price chart (same scale) | `TA_FUNC_FLG_OVERLAP` |
| `unstable_period` | Has an initial unstable calculation period | `TA_FUNC_FLG_UNST_PER` |
| `volume` | Output is based on volume data | `TA_FUNC_FLG_VOLUME` |
| `candlestick` | Output is a candlestick pattern signal | `TA_FUNC_FLG_CANDLESTICK` |
| `stream` | Generate the streaming API (Open/Update/Peek/…) | `TA_FUNC_FLG_STREAM` |
| `start_dependent` | Absolute output depends on `startIdx` and never converges across ranges (a running accumulation seeded at the first bar, or a path-dependent state machine); the same bar computed from a different `startIdx` can differ | `TA_FUNC_FLG_START_DEP` |

```yaml
flags: [overlap, unstable_period]
```

`start_dependent` is public `ta_abstract` metadata (issue #127): a wrapper reads
it from `TA_FuncInfo.flags`, and the `ta_regtest` range sweep reads the same bit
to decide it cannot cross-compare the function's values across ranges. Dropping
it is fail-safe — the sweep then value-compares the function and fails loudly if
it is genuinely start-dependent (issue #98).

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
  - { c_name: TA_MAType_SMA,   pascal_name: Sma,   value: 0 }
  - { c_name: TA_MAType_EMA,   pascal_name: Ema,   value: 1 }
  - { c_name: TA_MAType_WMA,   pascal_name: Wma,   value: 2 }
  - { c_name: TA_MAType_DEMA,  pascal_name: Dema,  value: 3 }
  - { c_name: TA_MAType_TEMA,  pascal_name: Tema,  value: 4 }
  - { c_name: TA_MAType_TRIMA, pascal_name: Trima, value: 5 }
  - { c_name: TA_MAType_KAMA,  pascal_name: Kama,  value: 6 }
  - { c_name: TA_MAType_MAMA,  pascal_name: Mama,  value: 7 }
  - { c_name: TA_MAType_T3,    pascal_name: T3,    value: 8 }
```

Each variant has:
- `c_name` — C constant (e.g., `TA_MAType_SMA`)
- `pascal_name` — PascalCase for Java/.NET (e.g., `Sma`)
- `value` — Integer value

The `short_name` (uppercase, e.g., `SMA`) is derived automatically from the `c_name` by stripping the `{EnumName}_` prefix.

## C Logic Files

The `.c` file in each function directory contains the actual computation logic, written in cross-language C using macros from `ta_defs.h`. See [ta_codegen_input_code.md](ta_codegen_input_code.md) for the macro reference.

## Complete Example

```yaml
# ta_codegen/input/sma/sma.yaml
name: SMA
camel_case: Sma
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
