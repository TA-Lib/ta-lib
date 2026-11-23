/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  111226 MF,CC  First Version
 */

// The default condition's entry, as a browser bundle gets it: no bytes until
// ready() imports them. This file never imports the Node entry, which would
// hand the shared engine its bytes; the type import below is erased.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import type { EngineStatus } from 'ta-lib';
import * as js from 'ta-lib/js';
import { defaultAbstract, defaultEntry, hooks } from './engineHooks.ts';

const lib = (await import(defaultEntry)) as typeof import('ta-lib');
const { getRuntimeInfo } = (await import(defaultAbstract)) as typeof import('ta-lib/abstract');
const { WASM_BASE64 } = (await import(new URL('../runtime/wasmBytes.js', import.meta.resolve('ta-lib')).href)) as {
  WASM_BASE64: string;
};
const close = Float64Array.from({ length: 200 }, (_, i) => Math.round((50 + 5 * Math.sin(i / 6) + i / 30) * 100) / 100);

interface Api {
  Module: new (bytes: Uint8Array) => object;
  Instance: new (module: object, imports: object) => { readonly exports: Record<string, unknown> };
  CompileError: new (message: string) => Error;
  compile: (bytes: Uint8Array) => Promise<unknown>;
  validate: (bytes: Uint8Array) => boolean;
}
const wasm = (globalThis as unknown as { WebAssembly: Api }).WebAssembly;
const real = { Module: wasm.Module, Instance: wasm.Instance, compile: wasm.compile };
const restoreWasm = (): void => void Object.assign(wasm, real);
const unbase64 = (text: string): Uint8Array => Uint8Array.from(atob(text), (c) => c.charCodeAt(0));
const isPreflight = (bytes: Uint8Array): boolean => Buffer.from(bytes).equals(Buffer.from(unbase64(hooks.PREFLIGHT)));
const preflightRuns = { compiles: 0, engineCompiles: 0 };

/** A `WebAssembly.Module` that counts compiles and, when asked, refuses the preflight module as a runtime without relaxed SIMD does. */
function moduleStub(refusePreflight: boolean): Api['Module'] {
  return class extends real.Module {
    constructor(bytes: Uint8Array) {
      if (isPreflight(bytes)) {
        preflightRuns.compiles++;
        if (refusePreflight) {
          throw new wasm.CompileError('WebAssembly.Module(): relaxed SIMD is not supported');
        }
      } else {
        preflightRuns.engineCompiles++;
      }
      super(bytes);
    }
  };
}

/** A `WebAssembly.Instance` whose memory-less modules answer the plain, unfused `a * b + c`. */
class Unfused extends real.Instance {
  constructor(module: object, imports: object) {
    super(module, imports);
    if (!('memory' in this.exports)) {
      const plain = (a: number, b: number, c: number): number => a * b + c;
      return { exports: Object.fromEntries(Object.keys(this.exports).map((name) => [name, plain])) };
    }
  }
}

/** A `WebAssembly.Instance` that refuses the memory-less preflight module as a runtime that forbids instantiation does. */
class Uninstantiable extends real.Instance {
  constructor(module: object, imports: object) {
    super(module, imports);
    if (!('memory' in this.exports)) {
      throw new RangeError('WebAssembly.Instance(): instantiation is not allowed');
    }
  }
}

const FAILING: readonly (readonly ['compile' | 'instantiate' | 'fma', Partial<Api>])[] = [
  ['compile', { Module: moduleStub(true) }],
  ['instantiate', { Module: moduleStub(false), Instance: Uninstantiable }],
  ['fma', { Module: moduleStub(false), Instance: Unfused }],
];

/** `wasm.engine` and `wasm.reason`, which must agree with the engine's own status. */
function info(): [number, number] {
  return [getRuntimeInfo('wasm.engine'), getRuntimeInfo('wasm.reason')];
}

function bits(a: Float64Array): string {
  return Buffer.from(a.buffer, a.byteOffset, a.byteLength).toString('hex');
}

/** One force-mode call through the default entry: its values against ta-lib/js, and the fall-throughs it made. */
function forced(): number {
  hooks.setEngineMode('force');
  const before = hooks.fallThroughCount();
  try {
    const got = lib.ema(close, { timePeriod: 12 });
    const want = js.ema(close, { timePeriod: 12 });
    assert.equal(got.begIdx, want.begIdx);
    assert.equal(bits(got.values), bits(want.values));
  } finally {
    hooks.setEngineMode('table');
  }
  return hooks.fallThroughCount() - before;
}

test('without its bytes the default entry answers from ta-lib/js and says why', () => {
  assert.deepEqual(info(), [0, 1]);
  assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'not-loaded' });
  assert.equal(forced(), 1);
  assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'not-loaded' });
  assert.deepEqual(info(), [0, 1]);
  hooks.setEngineMode('strict');
  try {
    assert.throws(() => lib.ema(close), hooks.StrictFallThrough);
    assert.throws(() => lib.ema(close, { timePeriod: 0 }), { name: 'TALibError', code: 'TA_BAD_PARAM' });
  } finally {
    hooks.setEngineMode('table');
  }
});

test('a corrupted byte string is reported as a failed compile, never thrown', async () => {
  for (const corrupt of ['AAAA', 'not base64 at all!']) {
    hooks.resetEngine();
    hooks.provideWasmBytes(corrupt);
    assert.deepEqual(info(), [0, 1], `${corrupt}: asking compiled the bytes`);
    assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'not-loaded' }, corrupt);
    assert.equal(forced(), 1, corrupt);
    assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'compile' }, corrupt);
    assert.deepEqual(info(), [0, 3], corrupt);
    const status: EngineStatus = await lib.ready();
    assert.deepEqual(status, { engine: 'js', reason: 'compile' }, corrupt);
  }
});

test('a module without the exports the engine needs is reported as a failed instantiate', async () => {
  hooks.resetEngine();
  hooks.provideWasmBytes('AGFzbQEAAAA=');
  try {
    assert.equal(forced(), 1);
    assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'instantiate' });
    assert.deepEqual(info(), [0, 4]);
    assert.deepEqual(await lib.ready(), { engine: 'js', reason: 'instantiate' });
  } finally {
    hooks.resetEngine();
  }
});

test('a failed import of the bytes stays unavailable until the next ready(), which imports them again', async () => {
  const unavailable = { engine: 'js', reason: 'unavailable' };
  let imports = 0;
  hooks.resetEngine();
  const real = hooks.setBytesImport(() => {
    imports++;
    return imports === 1 ? Promise.reject(new TypeError('Failed to fetch dynamically imported module')) : real();
  });
  try {
    const failed = [lib.ready(), lib.ready()];
    assert.deepEqual(await Promise.all(failed), [unavailable, unavailable]);
    assert.equal(imports, 1, 'two concurrent ready() calls did not share one import');
    assert.deepEqual(hooks.engineStatus(), unavailable);
    assert.deepEqual(info(), [0, 2]);
    assert.equal(forced(), 1);
    assert.equal(hooks.lastFallThrough(), 'unavailable');
    assert.deepEqual(hooks.engineStatus(), unavailable);
    const retried = [lib.ready(), lib.ready()];
    assert.deepEqual(await failed[0], unavailable, 'the failed ready() changed its answer');
    assert.deepEqual(await Promise.all(retried), [{ engine: 'wasm', reason: null }, { engine: 'wasm', reason: null }]);
    assert.equal(imports, 2, 'the retry did not import the bytes exactly once');
  } finally {
    hooks.setBytesImport(real);
  }
  assert.deepEqual(info(), [1, 0]);
  assert.equal(forced(), 0);
});

test('a failed compile is not retried', async () => {
  let imports = 0;
  hooks.resetEngine();
  const real = hooks.setBytesImport(async () => {
    imports++;
    return { WASM_BASE64: 'AAAA' };
  });
  try {
    assert.deepEqual(await lib.ready(), { engine: 'js', reason: 'compile' });
    assert.deepEqual(await lib.ready(), { engine: 'js', reason: 'compile' });
    assert.equal(imports, 1);
  } finally {
    hooks.setBytesImport(real);
    hooks.resetEngine();
  }
});

test('ready() imports the bytes and resolves to the WASM engine', async () => {
  hooks.resetEngine();
  const status: EngineStatus = await lib.ready();
  assert.deepEqual(status, { engine: 'wasm', reason: null });
  assert.deepEqual(await lib.ready(), status);
  assert.deepEqual(info(), [1, 0]);
  hooks.setEngineMode('strict');
  try {
    const entered = hooks.moduleCalls();
    const got = lib.ema(close, { timePeriod: 12 });
    assert.equal(hooks.moduleCalls() - entered, 1, 'the module did not count one batch call');
    assert.equal(bits(got.values), bits(js.ema(close, { timePeriod: 12 }).values));
  } finally {
    hooks.setEngineMode('table');
  }
});

test('a call made while ready() compiles falls through without compiling, and ready() still ends in WASM', async () => {
  const { compile } = real;
  let syncCompiles = 0;
  let release!: () => void;
  const released = new Promise<void>((resolve) => {
    release = resolve;
  });
  let compiling!: () => void;
  const started = new Promise<void>((resolve) => {
    compiling = resolve;
  });
  hooks.resetEngine();
  wasm.Module = class extends real.Module {
    constructor(bytes: Uint8Array) {
      if (bytes.byteLength > 4096) {
        syncCompiles++;
        throw new RangeError('WebAssembly.Module is disallowed on the main thread for a module this size');
      }
      super(bytes);
    }
  };
  wasm.compile = async (bytes: Uint8Array): Promise<unknown> => {
    compiling();
    await released;
    return compile.call(wasm, bytes);
  };
  let status: EngineStatus;
  try {
    const pending = lib.ready();
    const first = await Promise.race([started.then(() => 'compile'), pending.then(() => 'settled')]);
    assert.equal(first, 'compile', 'ready() settled without reaching WebAssembly.compile');
    assert.equal(forced(), 1);
    assert.equal(syncCompiles, 0, 'a routed call compiled the module while ready() was compiling it');
    assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'not-loaded' });
    release();
    status = await pending;
  } finally {
    restoreWasm();
  }
  assert.deepEqual(status, { engine: 'wasm', reason: null });
  hooks.setEngineMode('strict');
  try {
    const entered = hooks.moduleCalls();
    const got = lib.ema(close, { timePeriod: 12 });
    assert.equal(hooks.moduleCalls() - entered, 1, 'the module did not count one batch call');
    assert.equal(bits(got.values), bits(js.ema(close, { timePeriod: 12 }).values));
  } finally {
    hooks.setEngineMode('table');
  }
});

test('the preflight module is one relaxed multiply-add and answers as the engine\'s own probe', () => {
  const tiny = unbase64(hooks.PREFLIGHT);
  assert.ok(wasm.validate(tiny));
  const exported = Object.values(new real.Instance(new real.Module(tiny), {}).exports);
  assert.equal(exported.length, 1);
  const madd = exported[0] as (a: number, b: number, c: number) => number;
  const probe = new real.Instance(new real.Module(unbase64(WASM_BASE64)), {}).exports.fma_probe as typeof madd;
  const a = 1 + 2 ** -30;
  for (const [x, y, z] of [[a, a, -1], [0.1, 0.1, -0.01], [Math.PI, Math.E, -8.539734222673566]]) {
    assert.ok(Object.is(madd(x, y, z), probe(x, y, z)), `${x} * ${y} + ${z}: ${madd(x, y, z)} against ${probe(x, y, z)}`);
  }
});

test('a failing preflight settles ready() without importing the bytes', async () => {
  for (const [reason, stub] of FAILING) {
    let imports = 0;
    hooks.resetEngine();
    const realImport = hooks.setBytesImport(() => {
      imports++;
      return realImport();
    });
    Object.assign(wasm, stub);
    try {
      assert.deepEqual(await lib.ready(), { engine: 'js', reason }, reason);
      assert.deepEqual(await lib.ready(), { engine: 'js', reason }, reason);
      assert.equal(imports, 0, `${reason}: ready() imported the bytes`);
      assert.equal(forced(), 1, reason);
    } finally {
      restoreWasm();
      hooks.setBytesImport(realImport);
      hooks.resetEngine();
    }
  }
});

test('with the bytes handed over, a failing preflight sends routed calls to ta-lib/js without compiling the engine', () => {
  for (const [reason, stub] of FAILING) {
    hooks.resetEngine();
    hooks.provideWasmBytes(WASM_BASE64);
    preflightRuns.compiles = 0;
    preflightRuns.engineCompiles = 0;
    Object.assign(wasm, stub);
    try {
      assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'not-loaded' }, reason);
      assert.equal(forced(), 1, reason);
      assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason }, reason);
      assert.equal(hooks.lastFallThrough(), reason);
      assert.equal(forced(), 1, reason);
      assert.deepEqual(preflightRuns, { compiles: 1, engineCompiles: 0 }, reason);
    } finally {
      restoreWasm();
      hooks.resetEngine();
    }
  }
});

test('the preflight runs once, and resetEngine() forgets its verdict', async () => {
  hooks.resetEngine();
  preflightRuns.compiles = 0;
  let imports = 0;
  const realImport = hooks.setBytesImport(() => {
    imports++;
    return imports === 1 ? Promise.reject(new TypeError('Failed to fetch dynamically imported module')) : realImport();
  });
  wasm.Module = moduleStub(false);
  try {
    assert.deepEqual(await lib.ready(), { engine: 'js', reason: 'unavailable' });
    assert.deepEqual(await lib.ready(), { engine: 'wasm', reason: null });
    assert.equal(imports, 2);
    assert.equal(preflightRuns.compiles, 1, 'the retry after unavailable ran the preflight again');
    hooks.resetEngine();
    wasm.Module = moduleStub(true);
    assert.deepEqual(await lib.ready(), { engine: 'js', reason: 'compile' });
    assert.equal(preflightRuns.compiles, 2, 'resetEngine() kept the passing verdict');
    restoreWasm();
    hooks.resetEngine();
    assert.deepEqual(await lib.ready(), { engine: 'wasm', reason: null }, 'resetEngine() kept the failing verdict');
  } finally {
    restoreWasm();
    hooks.setBytesImport(realImport);
  }
  assert.equal(forced(), 0);
});
