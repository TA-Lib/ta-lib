// Usage: node import.mjs <package specifier> [<subpath specifier> ...]
const specs = process.argv.slice(2);
const modules = {};
for (const spec of specs) {
  modules[spec] = await import(spec);
}
const { sma } = modules[specs[0]];
const close = Float64Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
const out = new Float64Array(close.length);
const r = sma(0, close.length - 1, close, 3, out);
const names = Object.fromEntries(specs.map((s) => [s, Object.keys(modules[s]).sort()]));
console.log(JSON.stringify({ exports: names, sma: [r.begIdx, r.count, ...out.subarray(0, r.count)] }));
