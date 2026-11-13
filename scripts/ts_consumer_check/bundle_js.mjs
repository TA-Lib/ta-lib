// Bundled by ts_package_check.py with esbuild for Node; its metafile must name
// no module of the default entry's engine.
import { ema } from 'ta-lib/js';

console.log(ema(Float64Array.from({ length: 20 }, (_, i) => i + 1), { timePeriod: 5 }).values.length);
