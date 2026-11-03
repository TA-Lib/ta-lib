# TA-Lib for TypeScript and JavaScript

[TA-Lib](https://ta-lib.org), the widely used technical-analysis library, as pure TypeScript
compiled to standard ES modules: 200+ indicators covering moving averages, momentum
oscillators (RSI, MACD, Stochastic), volatility (Bollinger Bands, ATR), volume, Hilbert
Transform cycle analysis, statistics, price transforms and candlestick patterns.

Every function is generated from the same canonical definitions as the C library and is
bit-identical to the C reference implementation over the same inputs, except where a
calculation calls a transcendental math function such as `exp` or `log`: the JavaScript
engine's `Math` functions can round their result differently from the C library in the last
bit, which is beyond TA-Lib's control. No WASM, no native addon, no dependencies.

## Install

```
npm install ta-lib
```

ESM only, with no top-level await, so `require('ta-lib')` also works on every supported Node
line. Node 22.12 or later; any runtime that loads standard ES modules.

## Quick start

```ts
import { sma } from 'ta-lib';

const close = new Float64Array([/* ...your closing prices... */]);
const out = new Float64Array(close.length);

const { begIdx, count } = sma(0, close.length - 1, close, 30, out);

// out[0 .. count - 1] holds the SMA; out[k] is input bar begIdx + k.
for (let k = 0; k < count; k++) console.log(`bar ${begIdx + k} = ${out[k]}`);
```

Every indicator is a function with the same calling pattern: a `startIdx`/`endIdx` range,
input arrays, parameters, caller-provided output arrays, and an `OutRange` saying where the
values start and how many there are. A range that ends inside the lookback is a success with
no values, not an error.

Inputs take any `ArrayLike<number>`: a `Float64Array` is read in place, and any other form,
`number[]` included, is copied into one first. Outputs are `Float64Array`, or `Int32Array`
for integer outputs.

Invalid arguments throw a `TALibError`; match on its `code`:

```ts
import { sma, TALibError } from 'ta-lib';

try {
  sma(0, close.length - 1, close, 0, out);
} catch (e) {
  if (e instanceof TALibError && e.code === 'TA_BAD_PARAM') { /* ... */ }
}
```

## Configuration

`Core` is immutable. The value-affecting settings, the unstable period and the candlestick
thresholds, are chosen up front and passed as the last argument:

```ts
import { CoreBuilder, ema, FuncUnstId } from 'ta-lib';

const core = new CoreBuilder().unstablePeriod(FuncUnstId.EMA, 10).build();
ema(0, close.length - 1, close, 30, out, core);
```

## Live data

For a feed that arrives one bar at a time, each indicator also has a streaming form: open it
on the history you already have, then each closed bar in gives that bar's value out,
bit-identical to the batch call.

```ts
import { smaOpen } from 'ta-lib';

const s = smaOpen(history, 30);

const v = s.update(newClose);            // a closed bar: commits it
const provisional = s.peek(formingClose); // the forming bar: commits nothing
```

## Documentation

- TypeScript guide: <https://ta-lib.org/api/typescript/>, and the streaming tier: <https://ta-lib.org/api/typescript/stream/>
- Per-function reference (formulas, notes, sources): <https://ta-lib.org/functions/>
- Source: <https://github.com/TA-Lib/ta-lib>

## License

BSD-3-Clause, see [LICENSE](https://github.com/TA-Lib/ta-lib/blob/main/LICENSE).
