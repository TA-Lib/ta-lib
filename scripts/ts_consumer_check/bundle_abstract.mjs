// Bundled by ts_package_check.py with esbuild for Node, then run. Exits 1 unless
// a call through ta-lib/abstract, with no ready(), runs in WASM: a bundler that
// drops the abstract entry's hand-over of the bytes leaves it in TypeScript, silently.
import { Functions } from 'ta-lib/abstract';
import * as hooks from './node_modules/ta-lib/dist/runtime/wasm.js';

const close = Float64Array.from({ length: 500 }, (_, i) => Math.round((100 + 10 * Math.sin(i / 7)) * 100) / 100);
const call = Functions.byName('EMA').newCall().setInput(0, close).setOptInput(0, 10).setOutput(0, new Float64Array(close.length));
hooks.setEngineMode('force');
hooks.resetFallThroughs();
const entered = hooks.moduleCalls();
call.call();
const ran = hooks.moduleCalls() - entered;
const status = hooks.engineStatus();
const fell = hooks.fallThroughCount();
if (status.engine !== 'wasm' || ran !== 1 || fell !== 0) {
  console.error(`engine ${JSON.stringify(status)}, the module counted ${ran} batch call(s), ${fell} fall-through(s) (${hooks.lastFallThrough()})`);
  process.exit(1);
}
console.log('bundle_abstract.mjs: bundled for Node, a ta-lib/abstract call with no ready() ran in WASM');
