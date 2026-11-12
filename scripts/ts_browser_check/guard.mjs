// Imported before ta-lib, so every compile the package makes passes through
// these counters. A synchronous compile of the whole module blocks the main
// thread, which a browser allows only for small modules.
const wasm = globalThis.WebAssembly;
export const compiles = { sync: 0, async: 0 };
wasm.Module = new Proxy(wasm.Module, {
  construct(target, args, newTarget) {
    compiles.sync++;
    return Reflect.construct(target, args, newTarget);
  },
});
for (const name of ['compile', 'compileStreaming', 'instantiate', 'instantiateStreaming']) {
  const original = wasm[name];
  wasm[name] = function (...args) {
    if (name !== 'instantiate' || !(args[0] instanceof wasm.Module)) {
      compiles.async++;
    }
    return original.apply(this, args);
  };
}

export const thrown = [];
addEventListener('error', (e) => thrown.push(`error: ${e.message}`));
addEventListener('unhandledrejection', (e) => thrown.push(`unhandled rejection: ${e.reason}`));
