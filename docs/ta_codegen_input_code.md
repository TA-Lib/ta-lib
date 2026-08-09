# ta_codegen Input: Code (`<name>.c`) Reference

Each indicator's algorithm lives in `ta_codegen/input/<name>/<name>.c`, written as
**plain, standard C** using the types and macros from `ta_defs.h` — essentially the
same code you would find in `src/ta_func`. The generator parses this C
(parser: `ta_codegen/generator/src/parser/c_source.rs`) and re-emits it for every
backend: C, Rust, Java, .NET.

There is **no bespoke DSL** — write idiomatic C. Metadata (inputs, optional params,
outputs, flags) lives in the companion `<name>.yaml`; see
[ta_codegen_input_yaml.md](ta_codegen_input_yaml.md). The generator adds parameter
validation, single-precision variants, and per-language
naming — so the `.c` file contains only the algorithm.

## File contents

Two C functions, named after the directory (`<name>`):

```c
int <name>_lookback( /* optional params */ )
{
    return /* first valid output index */;
}

TA_RetCode <name>( int startIdx, int endIdx,
                   const double inReal[], /* optional params */,
                   int *outBegIdx, int *outNBElement, double outReal[] )
{
    /* ... algorithm ... */
    *outBegIdx    = startIdx;
    *outNBElement = outIdx;
    return TA_SUCCESS;
}
```

### Complete example — `ta_codegen/input/sma/sma.c`

```c
int sma_lookback(int optInTimePeriod)
{
    return optInTimePeriod - 1;
}

TA_RetCode sma(int startIdx, int endIdx, const double *inReal, int optInTimePeriod,
               int *outBegIdx, int *outNBElement, double *outReal)
{
    double periodTotal, tempReal;
    size_t i, outIdx, trailingIdx, lookbackTotal;

    lookbackTotal = (size_t)(optInTimePeriod - 1);
    if( startIdx < lookbackTotal ) {
        startIdx = lookbackTotal;
    }
    if( startIdx > endIdx ) {
        *outBegIdx = 0;
        *outNBElement = 0;
        return TA_SUCCESS;
    }

    periodTotal = 0.0;
    trailingIdx = startIdx - lookbackTotal;
    i = trailingIdx;
    if( optInTimePeriod > 1 ) {
        while( i < startIdx ) {
            periodTotal += (double)(inReal[i]);
            i = i + 1;
        }
    }

    outIdx = 0;
    while( i <= endIdx ) {
        periodTotal += (double)(inReal[i]);
        i = i + 1;
        tempReal = periodTotal;
        periodTotal -= (double)(inReal[trailingIdx]);
        trailingIdx = trailingIdx + 1;
        outReal[outIdx] = tempReal / (double)optInTimePeriod;
        outIdx = outIdx + 1;
    }

    *outNBElement = outIdx;
    *outBegIdx    = startIdx;
    return TA_SUCCESS;
}
```

Array parameters may be written either `const double inReal[]` (the common style) or
`const double *inReal` — both parse identically.

## Types

| Type | Use |
|------|-----|
| `TA_RetCode` | main function return type |
| `double`, `const double inReal[]`, `double outReal[]` | price inputs and outputs |
| `int` | optional params, counters, `int *outBegIdx` / `int *outNBElement` |
| `size_t` | array indices and counts |

Outputs are written through their pointer/array parameters: `*outBegIdx = ...`,
`*outNBElement = ...`, `outReal[outIdx] = ...`.

Return values are the real `ta_defs.h` codes: `TA_SUCCESS`, `TA_BAD_PARAM`,
`TA_ALLOC_ERR`. The generator maps these to each language's enum
(`RetCode::Success` in Rust, `RetCode.Success` in Java, etc.).

## Control flow & expressions

Standard C: `if` / `else if` / `else`, `while`, `for`, `switch` / `case`; the
arithmetic (`+ - * /`), comparison (`< <= > >= == !=`), and boolean (`&& || !`)
operators; C-style casts (`(double)x`, `(size_t)(n - 1)`); and array indexing. Follow
the bracing/spacing style of the existing input files.

## `ta_defs.h` vocabulary

Beyond plain C, the input files use a small set of TA-Lib macros/functions that the
generator recognizes and maps per language:

| Construct | Meaning |
|---|---|
| `TA_IS_ZERO(x)` / `TA_IS_ZERO_OR_NEG(x)` | epsilon comparison against zero |
| `TA_GetUnstablePeriod(TA_FUNC_UNST_<NAME>)` | this function's configured unstable period |
| `TA_COMPATIBILITY_DEFAULT` / `TA_COMPATIBILITY_METASTOCK` | compatibility-mode constants |
| candle-settings access (CDL* patterns) | resolved via the generated candle helpers |
| `CIRCBUF_PROLOG_CLASS` / `CIRCBUF_INIT_CLASS` / `CIRCBUF_NEXT` / `CIRCBUF_DESTROY` | circular scratch buffer over a local `typedef struct` element type (`src/ta_common/ta_memory.h`); see `cmf.c` or `ultosc.c` for usage |

Standard math functions (`sqrt`, `floor`, `ceil`, `fabs`, `sin`, `cos`, `atan`,
`atan2`, `log`, `exp`, `pow`, `fmod`, …) are mapped to each language's math library.

## Cross-indicator calls

Call another indicator by its **bare lowercase name** (matching its directory) — the
generator resolves it to the correct symbol per language. From
`ta_codegen/input/ma/ma.c`:

```c
retCode = sma( startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal );
```

maps to `TA_SMA(...)` in C, `self.SMA(...)` in Rust, `SMA_Internal(...)` in Java and
`SMA(...)` in C#. Cross-indicator calls target the **guarded** entry point.
`sma_lookback(...)` similarly maps to `TA_SMA_Lookback(...)` in C and
`SMA_Lookback(...)` (`self.SMA_Lookback(...)` in Rust) elsewhere.

## What the generator adds (do NOT write these)

- Parameter validation (NULL checks, range checks, `INTEGER_DEFAULT` substitution)
- Single-precision (`TA_S_*`) variants — generated automatically with `(double)` casts
  on inputs
- Per-language function signatures/naming, doc comments, file headers, imports

## What the logic file is NOT

- Not a macro template — no `#include`, no preprocessor `#if defined(_RUST)`, no
  `GENCODE` markers
- Not language-specific — the same `.c` file produces all four target languages
