// Reads golden.c's records on stdin, checks the library's fma against each,
// and prints one JSON summary line. Run by ts_fma_check.py.
import { fma } from '../../ta_codegen/output/ts/library/src/runtime/fma.ts';

const RECORD = 40;
const KEEP = Number(process.argv[2] ?? 10);
const checked = [];
const naiveDiffers = [];
const mismatched = [];
const examples = [];
const got = new DataView(new ArrayBuffer(8));

function hex64(view, at) {
  return view.getUint32(at + 4, true).toString(16).padStart(8, '0') +
    view.getUint32(at, true).toString(16).padStart(8, '0');
}

function check(view, end) {
  for (let at = 0; at < end; at += RECORD) {
    const family = view.getUint32(at + 32, true);
    if (checked[family] === undefined) {
      checked[family] = 0;
      naiveDiffers[family] = 0;
      mismatched[family] = 0;
    }
    checked[family]++;
    naiveDiffers[family] += view.getUint32(at + 36, true) & 1;
    const r = fma(view.getFloat64(at, true), view.getFloat64(at + 8, true), view.getFloat64(at + 16, true));
    const want = view.getFloat64(at + 24, true);
    let ok;
    if (want !== want) {
      ok = r !== r;
    } else {
      got.setFloat64(0, r, true);
      ok = got.getUint32(0, true) === view.getUint32(at + 24, true) &&
        got.getUint32(4, true) === view.getUint32(at + 28, true);
    }
    if (!ok) {
      mismatched[family]++;
      if (examples.length < KEEP) {
        got.setFloat64(0, r, true);
        examples.push([family, hex64(view, at), hex64(view, at + 8), hex64(view, at + 16),
          hex64(view, at + 24), hex64(got, 0)]);
      }
    }
  }
}

let carry = Buffer.alloc(0);
for await (const chunk of process.stdin) {
  const buf = carry.length ? Buffer.concat([carry, chunk]) : chunk;
  const whole = buf.length - (buf.length % RECORD);
  check(new DataView(buf.buffer, buf.byteOffset, whole), whole);
  carry = Buffer.from(buf.subarray(whole));
}
if (carry.length !== 0) {
  process.stderr.write(`check.mjs: ${carry.length} trailing bytes\n`);
  process.exit(2);
}

const families = {};
checked.forEach((n, f) => {
  if (n !== undefined) families[f] = [n, naiveDiffers[f], mismatched[f]];
});
process.stdout.write(JSON.stringify({ families, examples }) + '\n');
