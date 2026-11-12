import { Core, CoreBuilder, FuncUnstId, MAType, TALibError, atr, ema, macd, macdOpen, MacdOut, ready, sma, smaLookback } from 'ta-lib';
import type { EngineStatus, FailureRetCode, MacdResult, RetCodeName, SmaOptions, SmaResult } from 'ta-lib';
import * as js from 'ta-lib/js';
import { Functions } from 'ta-lib/metadata';

const close = new Float64Array(10);
const out = new Float64Array(close.length);
const r: SmaResult = sma(close, { timePeriod: 3, outReal: out });
const values: Float64Array = r.values;
const core: Core = new CoreBuilder().unstablePeriod(FuncUnstId.EMA, 3).build();
const e: Float64Array = ema([1, 2, 3], { timePeriod: 3, outReal: out, core }).values;
const opts: SmaOptions = { timePeriod: undefined, startIdx: 0, endIdx: close.length - 1 };
const lb: number = smaLookback(opts);
const m: MacdResult = macd(close, { fastPeriod: 12, outMacd: out });
// @ts-expect-error the C-shaped signature is not public
sma(0, close.length - 1, close, 3, out);
const sink: MacdOut = macdOpen(close).update(1, new MacdOut());
const a: Float64Array = atr({ high: close, low: close, close }, { timePeriod: 14 }).values;
const t: MAType = MAType.EMA;
const codeOf = (err: unknown): RetCodeName | undefined => (err instanceof TALibError ? err.code : undefined);
const retCodeOf = (err: unknown): FailureRetCode | undefined => (err instanceof TALibError ? err.retCode : undefined);
const status: Promise<EngineStatus> = ready();
const sameCore: Core = js.Core.DEFAULT;
const jsValues: Float64Array = js.sma(close, { timePeriod: 3 }).values;
// @ts-expect-error ta-lib/js has no engine to wait for
js.ready();
const names: string[] = Functions.all().map((f) => f.name);
export { r, values, e, lb, m, sink, a, t, codeOf, retCodeOf, status, sameCore, jsValues, names };
