// Bundled by ts_package_check.py with esbuild for Node, then run. Exits 1 unless
// a routed call, with no ready(), runs in WASM: a bundler that drops the Node
// entry's hand-over of the bytes leaves every call in TypeScript, silently.
import { ema } from 'ta-lib';
import * as hooks from './node_modules/ta-lib/dist/runtime/wasm.js';

const close = Float64Array.from({ length: 500 }, (_, i) => Math.round((100 + 10 * Math.sin(i / 7)) * 100) / 100);
hooks.setEngineMode('force');
const entered = hooks.moduleCalls();
ema(close, { timePeriod: 10 });
const ran = hooks.moduleCalls() - entered;
const status = hooks.engineStatus();
if (status.engine !== 'wasm' || ran !== 1) {
  console.error(`engine ${JSON.stringify(status)}, the module counted ${ran} batch call(s)`);
  process.exit(1);
}
console.log('bundle_node.mjs: bundled for Node, a routed call with no ready() ran in WASM');
