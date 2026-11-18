// A page script that imports only ta-lib/js/abstract: its bundle must not carry the engine's bytes.
import { Functions } from 'ta-lib/js/abstract';

const close = Float64Array.from({ length: 20 }, (_, i) => i + 1);
const range = Functions.byName('EMA').newCall().setInput(0, close).setOptInput(0, 5).setOutput(0, new Float64Array(close.length)).call();
document.body.textContent = String(range.count);
