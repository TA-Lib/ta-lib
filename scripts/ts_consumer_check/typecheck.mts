import { Core, CoreBuilder, FuncUnstId, OutRange, TALibError, ema, sma } from 'ta-lib';
import type { FailureRetCode, RetCodeName } from 'ta-lib';

const close = new Float64Array(10);
const out = new Float64Array(close.length);
const r: OutRange = sma(0, close.length - 1, close, 3, out);
const core: Core = new CoreBuilder().unstablePeriod(FuncUnstId.EMA, 3).build();
const e: OutRange = ema(0, close.length - 1, [1, 2, 3], 3, out, core);
const codeOf = (err: unknown): RetCodeName | undefined => (err instanceof TALibError ? err.code : undefined);
const retCodeOf = (err: unknown): FailureRetCode | undefined => (err instanceof TALibError ? err.retCode : undefined);
export { r, e, codeOf, retCodeOf };
