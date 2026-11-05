// Usage: node require.cjs <package specifier> [<subpath specifier> ...]
// require() of an ES module throws ERR_REQUIRE_ASYNC_MODULE when its graph has a top-level await.
const specs = process.argv.slice(2);
const modules = {};
for (const spec of specs) {
  modules[spec] = require(spec);
}
const { sma } = modules[specs[0]];
const close = Float64Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
const { begIdx, values } = sma(close, { timePeriod: 3 });
const names = Object.fromEntries(specs.map((s) => [s, Object.keys(modules[s]).sort()]));
console.log(JSON.stringify({ exports: names, sma: [begIdx, values.length, ...values] }));
