// A page script that imports only ta-lib/js: its bundle must not carry the engine's bytes.
import { sma } from 'ta-lib/js';

document.body.textContent = String(sma([1, 2, 3, 4, 5], { timePeriod: 3 }).values[0]);
