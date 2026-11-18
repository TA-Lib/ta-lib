// Bundled by ts_package_check.py with esbuild for Node; its metafile must name
// no module of the engine, not runtime/wasm.js and not the bytes.
import { Functions, getRuntimeInfo } from 'ta-lib/js/abstract';

const close = Float64Array.from({ length: 20 }, (_, i) => i + 1);
const range = Functions.byName('EMA').newCall().setInput(0, close).setOptInput(0, 5).setOutput(0, new Float64Array(close.length)).call();
console.log(range.count, getRuntimeInfo('wasm.engine'));
