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

Two C functions, named after the directory (`<name>`) — plus, where a function
needs one, an alternate implementation (see `PRAGMA TA_ALT` below):

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

## Rescan loops: write the window start INLINE

A body that rebuilds a window sum -- the periodic re-anchor `TA_VAR`,
`TA_CORREL`, `TA_BETA`, the `TA_LINEARREG` family, `TA_WMA` and `TA_HMA` all
carry -- must name the window start **in the loop init**, not through a local:

```c
/* YES -- recognised as a rescan window */
for( j = today - lookbackTotal; j <= today; j++ )
   periodTotal += inReal[j];

/* NO -- reads well, and is not recognised */
windowStart = today - lookbackTotal;
for( j = windowStart; j <= today; j++ )
   periodTotal += inReal[j];
```

`streaming.rs`'s `match_cursor_anchored_loop` matches `for( v = <cursor> - E;
v <= <cursor>; v++ )` and nothing else, and that match is what puts the rescan on
the stream classifier's primary path. The named form is not recognised as a
window at all: its index bookkeeping reads a non-index symbol, the classifier
retries in absolute-index (extrema) mode, and the loop is lowered through a
synthesised mask ring instead.

**Both spellings work for most functions, which is exactly the problem.** The
extrema fallback is refused when the body also owns a CIRCBUF, a counter, or
another window -- so a body that streams today stops streaming the moment one is
added, and the refusal names the ring rather than the spelling that caused it.
`TA_HMA` is the live instance: it keeps a CIRCBUF for its de-lagged series, so
its three fused WMA stages could only be given a re-anchor once their rescans
were written inline.

The refusal message now says this, and names the cursor for you. If you see
`extrema automaton mixed with other buffer forms`, check the loop init first.

One case cannot be written inline, and is the reason this is a convention rather
than a check: `TA_VAR` reads `inReal[windowStart]` later in the same loop, so the
local is load-bearing there and the function keeps the extrema path deliberately.

## `ta_defs.h` vocabulary

Beyond plain C, the input files use a small set of TA-Lib macros/functions that the
generator recognizes and maps per language:

| Construct | Meaning |
|---|---|
| `TA_IS_ZERO(x)` / `TA_IS_ZERO_OR_NEG(x)` | epsilon comparison against zero |
| `TA_GetUnstablePeriod(TA_FUNC_UNST_<NAME>)` | this function's configured unstable period |
| `TA_COMPATIBILITY_DEFAULT` / `TA_COMPATIBILITY_METASTOCK` | compatibility-mode constants — **not for new functions**: preserved for those that already honour one, and unreachable from the Rust, Java and C# APIs, which expose no compatibility setting |
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

maps to `TA_SMA(...)` in C and the public `SMA(...)` in Rust, Java and C# — the
callee's public entry point in every backend, which is what C has always done
(#236 step 3, #267). Its rejection surfaces as a throw in Java and C# and as an
`Err(RetCode)` in Rust.
`sma_lookback(...)` similarly maps to `TA_SMA_Lookback(...)` in C and
`SMA_Lookback(...)` (`self.SMA_Lookback(...)` in Rust) elsewhere.

## What the generator adds (do NOT write these)

- Parameter validation (NULL checks, range checks, `INTEGER_DEFAULT` substitution)
- Single-precision (`TA_S_*`) variants — generated automatically with `(double)` casts
  on inputs
- Per-language function signatures/naming, doc comments, file headers, imports

## Alternate implementations — `PRAGMA TA_ALT`

Alongside `<name>`, a file may declare `<name>_ALT1`, `<name>_ALT2`, … — whole
alternative bodies for the same function, each under a **single-line** decoration:

```c
/* PRAGMA TA_ALT={<api>,<lang>} free text after the brace is ignored */
TA_RetCode <name>_ALT<n>( /* the same parameters as <name> */ )

<api>  ::= BATCH | STREAM | ALL_API
<lang> ::= C | RUST | JAVA | CSHARP | ALL_LANGUAGES
```

**Later declarations override earlier ones for every cell they claim**, with the
base as the implicit first entry claiming `{ALL_API,ALL_LANGUAGES}` — so "everyone
but C" is `ALL_LANGUAGES` followed by `C`, and there is no specificity table to
learn. An alternate that ends up winning no cell is a hard error, as is a claim
with no `=`, an unrecognized directive name, a `PRAGMA TA_ALT` on the base, an
`_ALT<n>` with no decoration, numbering that is not ascending and contiguous from
1 in file order, and a signature that differs from the base's.

An alternate must be **strictly functionally identical** to the base (some may
differ within `TA_STABLE_EPSILON`; none may compute something else), and the
accepted reason to carry one is a significant performance gain — a second copy of
an algorithm is a maintenance cost. There is no alternate lookback: one
`<name>_lookback` serves every cell.

Nothing ships a language-scoped claim today. Note before writing the first one
that `--xlang-hash` holds every language server bit-identical to the in-process C
library with no tolerance, and C is the golden — so a `{...,C}` alternate that is
not bit-identical to the others moves the reference and fails all three remaining
languages at once. That is a loud failure, not a silent one, but it is the gate
such a change has to answer for.

`_ALT<n>` is **generator input only and never becomes a symbol**. There is still
one `TA_MIN`, one `TA_MIN_Open`; the claim decides only which body the generator
transcribes into them. Calling `min(...)` from another indicator resolves exactly
as it always did, and naming `min_ALT1` in a call is an error. Where an alternate
wins, the generated file says so: `/* Using min_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */`.

`PRAGMA` is the general mechanism, not a `TA_ALT` mechanism: any comment whose
first word is `PRAGMA` addresses the generator and is stripped from every backend's
output, several may precede one signature, and an unrecognized name **without** an
`=` is free-form prose the generator ignores.

## What the logic file is NOT

- Not a macro template — no `#include`, no preprocessor `#if defined(_RUST)`, no
  `GENCODE` markers
- Not language-specific in the small — the same `.c` file produces all four target
  languages. A *whole* alternative algorithm may be selected per language (see
  `PRAGMA TA_ALT` above): self-contained, co-located, skippable in one read.
  Scattering per-language conditionals through a single body is still forbidden.
