---
name: new-ta-func
description: Use when adding a new TA-Lib indicator to ta_codegen, modifying an existing indicator's logic or metadata, or extending the generator (parser/IR/backends) for a new C construct. Triggers on "new ta func", "add indicator", "modify indicator", "next function", or any TA-Lib function name (SMA, RSI, EMA, MA, BBANDS, etc).
---

# Add / Modify a TA Function via ta_codegen

`ta_codegen` (Rust, in `ta_codegen/generator/`) is the single code generator. Each
function is defined by three files in `ta_codegen/input/<name>/`:

- `<name>.yaml` — metadata (inputs, optional params, outputs, group, flags)
- `<name>.c` — the algorithm, written as plain C (see `docs/ta_codegen_input_code.md`)
- `<name>.md` — documentation prose (see `docs/ta_codegen_input_doc.md`)

From these it generates all **four** backends: **C** (in place under `src/ta_func` /
`src/ta_abstract`), **Rust**, **Java**, **C#** (under `ta_codegen/output/`) — plus the
website function page and the Rust rustdoc from `<name>.md`.

> Use this skill to **add a brand-new** function, **modify** an existing one, or
> **extend the generator** to support a new C construct. The correctness baseline is
> the frozen pre-cutover reference (the `reference-pre-cutover` tag, served as
> `ta_ref_serve`) plus ta_regtest's hardcoded expected values. The contributor-facing
> workflow (spec approval, golden values, PR) is `website/src/contribute/README.md`.

## Usage

- `/new-ta-func BBANDS` — work on a specific function
- `/new-ta-func` — resume in-progress work

## Workflow

```dot
digraph new_ta_func {
    "Pick / scope indicator" -> "Write name.yaml + name.c + name.md";
    "Write name.yaml + name.c + name.md" -> "cargo run -- generate";
    "cargo run -- generate" -> "Parse error?";
    "Parse error?" -> "Extend parser (c_source.rs)" [label="yes"];
    "Extend parser (c_source.rs)" -> "cargo run -- generate";
    "Parse error?" -> "Review generated output" [label="no"];
    "Review generated output" -> "New IR node needed?";
    "New IR node needed?" -> "Extend IR + all 4 backends" [label="yes"];
    "Extend IR + all 4 backends" -> "cargo run -- generate";
    "New IR node needed?" -> "Output correct?" [label="no"];
    "Output correct?" -> "ta_regtest --codegen + commit" [label="yes"];
    "Output correct?" -> "Fix backend rendering" [label="no"];
    "Fix backend rendering" -> "cargo run -- generate";
}
```

## Step-by-step

### 1. Find / scope the indicator

```bash
ls ta_codegen/input/                 # existing definitions
ls ta_codegen/input/<name>/          # the target's .yaml + .c (if it already exists)
```

### 2. Write / adjust the metadata — `ta_codegen/input/<name>/<name>.yaml`

Full schema in `docs/ta_codegen_input_yaml.md`. Example:

```yaml
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
    range: [2, 100000]
    default: 30
    suggested: [4, 200, 1]
outputs:
  - name: outReal
    type: real
    flags: [line]
```

There is **no `lookback:` field** — lookback is a C function in the `.c` file (below).
Use `hint:` for the short description (not `description:`).

### 3. Write / adjust the logic — `ta_codegen/input/<name>/<name>.c`

Plain C, exactly as it would appear in `src/ta_func`: two functions,
`int <name>_lookback(...)` and
`TA_RetCode <name>(int startIdx, int endIdx, const double inReal[], ..., int *outBegIdx, int *outNBElement, double outReal[])`.
Full syntax and the `ta_defs.h` vocabulary (`TA_IS_ZERO`,
`TA_GetUnstablePeriod(TA_FUNC_UNST_X)`, `TA_COMPATIBILITY_*`, …) are in
`docs/ta_codegen_input_code.md`.

**Rules:**

- A complete C function: full signature, `TA_RetCode` return, pointer/array outputs
  (`*outBegIdx = ...`, `outReal[outIdx] = ...`), `return TA_SUCCESS;`
- Do **not** write parameter validation — the generator adds it
- Cross-indicator calls use the **bare lowercase name** (`sma(...)`,
  `ema_lookback(...)`); the generator resolves them per language

### 4. Write / adjust the documentation — `ta_codegen/input/<name>/<name>.md`

The canonical prose source: summary, the formula in its **original algebraic form**
(never implementation artifacts — no zero-guards, epsilons or `period == 1` cases),
inputs/outputs, references. Rendered into the website function page and the Rust
rustdoc. Schema and gated sections in `docs/ta_codegen_input_doc.md`.

### 5. Generate and iterate

```bash
cd ta_codegen/generator
cargo run --release -- generate --func=<NAME>
cargo test
```

If the parser panics or output is wrong, extend:

| Missing | Where |
|---|---|
| New statement type | `ir.rs` + `parser/c_source.rs` + all 4 backends |
| New expression type | `ir.rs` + `parser/c_source.rs` + all 4 backends |
| New builtin / macro | `backends/builtins.rs` + each backend's render |
| New type keyword | `parser/c_source.rs` + backends |
| New variable mapping | `Expr::Var` match in each backend's `render_expr()` |

**When extending the IR you MUST update ALL 4 backends** (C, Rust, Java, C#) or Rust
exhaustiveness errors will point you to each.

### 6. Verify across languages and commit

```bash
scripts/build.py servers
cd bin && ./ta_regtest --codegen --function=<NAME>     # all langs vs the C reference
```

`--codegen` needs `bin/ta_ref_serve`; on a fresh clone build it via `scripts/regtest.py`.
A brand-new function is *skipped* by the generic sweep (absent from the frozen
reference) — it is verified by a hand-written golden-value test instead
(`src/tools/ta_regtest/ta_test_func/test_composite.c` pattern), whose calls the
harness also checks bitwise against every language server.

`git diff` the other backends' generated output to confirm an unrelated language
didn't change. SMA/MULT have byte-identical reference comparisons — if those break,
the change is wrong.

## Key files

| File | Purpose |
|------|---------|
| `ta_codegen/input/<name>/<name>.yaml` | Metadata: inputs, outputs, params, flags |
| `ta_codegen/input/<name>/<name>.c` | Algorithm (plain C) |
| `ta_codegen/input/<name>/<name>.md` | Documentation (canonical prose source) |
| `ta_codegen/generator/src/ir.rs` | IR types (FuncDef, Statement, Expr, ParamType) |
| `ta_codegen/generator/src/parser/c_source.rs` | C-source → IR parser |
| `ta_codegen/generator/src/parser/yaml.rs` | YAML metadata parser |
| `ta_codegen/generator/src/backends/*.rs` | Backends (c, rust_lang, java, csharp) |
| `ta_codegen/generator/src/server_gen.rs` | JSON-RPC server generation |
| `ta_codegen/generator/tests/validate.sh` | Dev validation harness |
| `docs/ta_codegen_input_yaml.md` | YAML schema reference |
| `docs/ta_codegen_input_code.md` | `.c` logic / `ta_defs.h` macro reference |
| `docs/ta_codegen_input_doc.md` | `.md` documentation reference |

## Backend rendering

Each backend has the same structure:

- **`render_statement()`** — Statement variants (VarDecl, Assign, While, If, Switch, …)
- **`render_expr()`** — Expr variants (Var, Literal, BinOp, Cast, FuncCall, …)
- builtins + cross-indicator dispatch (`backends/builtins.rs` + per-backend resolution)

**Cross-call dispatch per language** (a bare `sma(...)` in the `.c` file):

| Call in `<name>.c` | C | Rust | Java | C# |
|---|---|---|---|---|
| `sma(...)` | `TA_SMA(...)` | `self.SMA(...)` | `SMA_Internal(...)` | `SMA(...)` |
| `sma_lookback(...)` | `TA_SMA_Lookback(...)` | `self.SMA_Lookback(...)` | `SMA_Lookback(...)` | `SMA_Lookback(...)` |

(C also emits single-precision `TA_S_*` variants automatically; there is no Rust `_s`
variant — Rust is concrete `f64`.)

## Complexity tiers (reference order of increasing difficulty)

| Tier | Example | Features |
|---|---|---|
| Simple loop | MULT | while, assign, array access |
| Accumulator | SMA | if/else, return, cast, running sum |
| Stateful | RSI | `TA_GetUnstablePeriod`, `TA_IS_ZERO`, for-loop, complex lookback |
| Recursive | EMA | k factor, `TA_COMPATIBILITY_*` compat, operator precedence |
| Dispatcher | MA | switch/case, cross-call dispatch, `TA_BAD_PARAM`/`TA_SUCCESS` |
| Multi-output | BBANDS | multiple output arrays |
