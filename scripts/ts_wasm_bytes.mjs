// Usage: node scripts/ts_wasm_bytes.mjs <library dir>
//
// Writes <library>/dist/runtime/wasmBytes.js, the module holding bin/ta_lib.wasm
// as base64 that the default entry imports, and its declaration beside it. Run
// after tsc has emitted dist/. Refuses a bin/ta_lib.wasm that is missing or older
// than any source of the crate it is built from: `scripts/build.py wasm` rebuilds it.
import { copyFileSync, readFileSync, readdirSync, statSync, writeFileSync } from 'node:fs';
import { dirname, join, relative, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const library = resolve(process.argv[2] ?? '');
const wasm = join(root, 'bin', 'ta_lib.wasm');
const sources = [
  'rust-toolchain.toml',
  'ta_codegen/output/wasm/Cargo.toml',
  'ta_codegen/output/wasm/src',
  'ta_codegen/output/rust/library/Cargo.toml',
  'ta_codegen/output/rust/library/src',
];

function fail(message) {
  console.error(`ts_wasm_bytes: ${message}`);
  process.exit(1);
}

function files(path) {
  return statSync(path).isDirectory() ? readdirSync(path).flatMap((name) => files(join(path, name))) : [path];
}

if (process.argv.length !== 3) fail('usage: node scripts/ts_wasm_bytes.mjs <library dir>');
let built;
try {
  built = statSync(wasm).mtimeMs;
} catch {
  fail(`${relative(root, wasm)} is missing; build it with scripts/build.py wasm`);
}
for (const file of sources.flatMap((s) => files(join(root, s)))) {
  if (statSync(file).mtimeMs > built) {
    fail(`${relative(root, wasm)} is older than ${relative(root, file)}; rebuild it with scripts/build.py wasm`);
  }
}

const index = readFileSync(join(library, 'dist', 'index.js'), 'utf8');
const end = index.indexOf('*/');
if (!index.startsWith('/*! TA-LIB Copyright') || end < 0) fail('dist/index.js does not open with the licence; run tsc first');
const licence = index.slice(0, end + 2);
const base64 = readFileSync(wasm).toString('base64');
writeFileSync(join(library, 'dist', 'runtime', 'wasmBytes.js'), `${licence}\nexport const WASM_BASE64 = "${base64}";\n`);
copyFileSync(join(library, 'src', 'runtime', 'wasmBytes.d.ts'), join(library, 'dist', 'runtime', 'wasmBytes.d.ts'));
console.log(`ts_wasm_bytes: dist/runtime/wasmBytes.js holds ${relative(root, wasm)} (${statSync(wasm).size} bytes)`);
