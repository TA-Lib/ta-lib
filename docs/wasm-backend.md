# WASM engine for the npm package

**Status:** planned, nothing built. The `ta-lib` npm package gains a WebAssembly engine behind the API of [typescript-api-design.md](typescript-api-design.md); the TypeScript library of [typescript-backend.md](typescript-backend.md) stays, as the `ta-lib/js` entry and as the default entry's fallback. Rows marked Ruled are the owner's; Adopted rows stand unless he reopens them; an Open row names the measurement that settles it.

## 1. Scope

- **What ships.** One package, two entries. `ta-lib` runs each call in WASM when the runtime passes the load checks (W4) and in the TypeScript body otherwise. `ta-lib/js` is the TypeScript library alone and never loads WASM.
- **Same values.** The two engines are bit-identical on every call, so which one answers changes speed only. That is what lets the default entry choose by itself.
- **What does not ship:** a WASM-only entry, an API that must be awaited before its first call, one WASM module per function.

## 2. Decisions

| # | Decision | Choice | Why | Status |
|---|---|---|---|---|
| W1 | Engine | A WASM engine generated beside the TypeScript library, in the same npm package | Software `fma` is TypeScript's dominant cost ([D9](typescript-backend.md)); a hardware fused multiply-add in WASM removes it | Ruled (owner, 2026-11-11) |
| W2 | Entries | `"."`: a generated layer per function that calls WASM when it is loaded and the `./js` function otherwise. `"./js"`: the TypeScript library as it is, with no routing and no WASM glue. Both export the same names and share `Core`, `TALibError`, the enums and the result types | Whoever must not load WASM (a strict Content-Security-Policy, a size budget) imports `ta-lib/js` and gets today's code. Everyone else gets the fast path with no code change | Ruled (owner, 2026-11-11) |
| W3 | FMA | `f64x2.relaxed_madd` (relaxed SIMD), used only where W4 proves it fused | Fused, it gives C's bits. Relaxed SIMD lets an engine compute it unfused, so it is trusted only after the check | Ruled (owner, 2026-11-11): hardware FMA comes first |
| W4 | Load checks | The module compiles, and one multiply-add whose fused and unfused results differ returns the fused one. A failure of either leaves the TypeScript path in place and throws nothing. `getRuntimeInfo` ([D18](typescript-backend.md)) reports the engine in use and, for TypeScript, which check failed | Compilation fails where relaxed SIMD is missing and where a Content-Security-Policy forbids WASM; there is no way to ask without trying. A missing `.wasm` asset or a non-FMA CPU would otherwise be a silent slow path | Adopted |
| W5 | API | Unchanged and synchronous: no top-level await ([D3](typescript-backend.md)), no call that must be awaited first | The fallback is what makes this possible in a browser, which compiles a module of this size only asynchronously | Adopted |
| W6 | Settings | `Core` stays the JavaScript object of [D12](typescript-backend.md) and the only owner of unstable periods and candle settings. The engine keeps none: a call passes what it reads | One source of truth whichever engine answers, and for any number of WASM modules | Adopted |
| W7 | Loading | Node, Deno and Bun compile at import, synchronously. A browser loads the engine only when the caller awaits `ready()`; before that, and without it, every call runs TypeScript. `ready()` takes engine-loading arguments only (where the binary is); settings stay on `Core` (W6) | A server runtime needs no network and no await. In a browser an explicit call fetches the binary only for a caller who asked, and a page whose Content-Security-Policy forbids WASM logs no violation unless it called | Ruled (owner, 2026-11-11) |
| W8 | Source | The module is the generated Rust crate built for `wasm32-unknown-unknown`, behind generated `extern "C"` exports | The crate builds for that target as it is, with its own allocator, and a bad index traps where C would corrupt linear memory. The generated C built for wasm32 is no faster (level in batch, slower in `update` on most functions measured) and needs a hand-written runtime | Adopted |
| W9 | Modules | One, holding every function's batch and streaming tiers | A function taking an MA type at run time carries every moving average, so a split would repeat that code in each part | Adopted |
| W10 | Transcendental calls | A call that can reach a transcendental function ([D11](typescript-backend.md)'s predicate: the function carries `uses_transcendental`, or an MA-type parameter selects MAMA or ALMA) runs in TypeScript. The module imports nothing | A libm inside the module differs from the JS engine's `Math.*` in the last bit, silently, and a recursive filter amplifies it. Importing `Math.*` restores the bits but gains nothing in batch | Adopted |
| W11 | Stream class | One `<N>Stream` per indicator in the default entry, holding a WASM handle or the `./js` stream, each method choosing by that | A call site sees one class whichever engine answers. Timed against a class per engine and a closure, no shape costs more than a few ns per bar | Adopted |

## 3. Open

| # | Question | Settled by |
|---|---|---|
| W-O2 | A second binary with no relaxed SIMD and a software `fma`, for a runtime that fails W4 only on the fused check. Its `fma` must be `runtime/fma.ts`'s algorithm: a libm `fma` compiled to WASM is slower than TypeScript on every multiply-add chain | Its timings against TypeScript on a chain function and on one with no fused site, and the binary's size |
| W-O4 | Releasing a WASM stream. Its state is linear memory, which never shrinks, and [D15](typescript-backend.md) has no `close()`. A `FinalizationRegistry` runs only after a major collection, which small wrapper objects rarely cause, so it bounds nothing by itself. Choices: an explicit `close()` with the registry behind it; streams kept in TypeScript; or the first with a TypeScript stream once WASM memory passes a budget | Owner |
| W-O6 | Where the argument checks run, so a call is checked once whichever engine answers and a rejection carries the `./js` message | The implementation plan |
| W-O7 | Gates: every gate of the TypeScript backend run over both engines, and engine against engine bitwise | The implementation plan |
