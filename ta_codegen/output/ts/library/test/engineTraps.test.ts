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

// The engine's failure paths, over modules built here byte by byte: each has
// the exports the loader requires, and `sma_batch` traps, is missing or is
// never reached.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { sma } from 'ta-lib';
import * as js from 'ta-lib/js';
import { hooks } from './engineHooks.ts';

const close = Float64Array.from({ length: 50 }, (_, i) => Math.round((20 + 3 * Math.sin(i / 4)) * 100) / 100);

function leb(n: number): number[] {
  const out: number[] = [];
  for (;;) {
    const b = n & 0x7f;
    n >>>= 7;
    if (n === 0) {
      out.push(b);
      return out;
    }
    out.push(b | 0x80);
  }
}

const section = (id: number, body: number[]): number[] => [id, ...leb(body.length), ...body];
const vec = (items: number[][]): number[] => [...leb(items.length), ...items.flat()];
const name = (s: string): number[] => [...leb(s.length), ...Buffer.from(s, 'latin1')];
const body = (code: number[]): number[] => [...leb(code.length + 2), 0, ...code, 0x0b];

/** `fma_probe` answers the fused product when `fused`, else `a * b + c` rounded twice. */
function engineBytes(fused: boolean, withBatch = true): string {
  const fusedValue = [...new Uint8Array(new Float64Array([2 ** -29 + 2 ** -60]).buffer)];
  const probe = fused ? [0x44, ...fusedValue] : [0x20, 0, 0x20, 1, 0xa2, 0x20, 2, 0xa0];
  const types = vec([
    [0x60, 1, 0x7f, 1, 0x7f],
    [0x60, 0, 1, 0x7f],
    [0x60, 3, 0x7c, 0x7c, 0x7c, 1, 0x7c],
    [0x60, 0, 1, 0x7f],
  ]);
  const exports = vec([
    [...name('memory'), 2, 0],
    [...name('scratch'), 0, 0],
    [...name('out_range'), 0, 1],
    [...name('batch_calls'), 0, 1],
    [...name('fma_probe'), 0, 2],
    ...(withBatch ? [[...name('sma_batch'), 0, 3]] : []),
  ]);
  const code = vec([body([0x41, ...leb(1024)]), body([0x41, 0]), body(probe), body([0x00])]);
  const module = [
    ...[0x00, 0x61, 0x73, 0x6d, 1, 0, 0, 0],
    ...section(1, types),
    ...section(3, vec([[0], [1], [2], [3]])),
    ...section(5, vec([[0, 2]])),
    ...section(7, exports),
    ...section(10, code),
  ];
  return Buffer.from(module).toString('base64');
}

type Ctor = new (...args: unknown[]) => object;
const wasm = (globalThis as unknown as { WebAssembly: { Instance: Ctor; RuntimeError: new () => Error } }).WebAssembly;

function bits(a: Float64Array): string {
  return Buffer.from(a.buffer, a.byteOffset, a.byteLength).toString('hex');
}

function answersAsJs(where: string): void {
  const got = sma(close, { timePeriod: 5 });
  const want = js.sma(close, { timePeriod: 5 });
  assert.equal(got.begIdx, want.begIdx, where);
  assert.equal(bits(got.values), bits(want.values), where);
}

function trappedInWasm(where: string): void {
  assert.equal(hooks.lastFallThrough(), 'trap', where);
  assert.ok(hooks.lastTrapError() instanceof wasm.RuntimeError, `${where}: the call threw ${String(hooks.lastTrapError())}, not a WebAssembly trap`);
}

test('a trap is answered from ta-lib/js and the engine instantiated again, until the limit gives it up', () => {
  const Instance = wasm.Instance;
  let instances = 0;
  wasm.Instance = new Proxy(Instance, {
    construct(target, args, newTarget) {
      instances++;
      return Reflect.construct(target, args, newTarget) as object;
    },
  });
  hooks.resetEngine();
  hooks.provideWasmBytes(engineBytes(true));
  hooks.resetFallThroughs();
  hooks.setEngineMode('force');
  try {
    answersAsJs('first trap');
    trappedInWasm('first trap');
    assert.equal(instances, 1);
    assert.deepEqual(hooks.engineStatus(), { engine: 'wasm', reason: null });
    let calls = 1;
    while (hooks.engineStatus().engine === 'wasm' && calls < hooks.MAX_TRAPS) {
      answersAsJs(`trap ${++calls}`);
      trappedInWasm(`trap ${calls}`);
      assert.equal(instances, calls, 'a call after a trap did not instantiate the engine again');
    }
    assert.equal(calls, hooks.MAX_TRAPS);
    assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'trap' });
    answersAsJs('given up');
    assert.equal(instances, calls);
    assert.equal(hooks.fallThroughCount(), calls + 1);
  } finally {
    hooks.setEngineMode('table');
    wasm.Instance = Instance;
    hooks.resetEngine();
  }
});

test('a call that throws in the glue, not in the module, is not reported as a trap', () => {
  hooks.resetEngine();
  hooks.provideWasmBytes(engineBytes(true, false));
  hooks.resetFallThroughs();
  hooks.setEngineMode('force');
  try {
    answersAsJs('missing export');
    assert.equal(hooks.lastFallThrough(), 'threw');
    assert.ok(hooks.lastTrapError() instanceof TypeError, `the call threw ${String(hooks.lastTrapError())}`);
    assert.equal(hooks.fallThroughCount(), 1);
  } finally {
    hooks.setEngineMode('table');
    hooks.resetEngine();
  }
});

test('a module whose fma_probe does not fuse is refused, and calls answer from ta-lib/js', () => {
  hooks.resetEngine();
  hooks.provideWasmBytes(engineBytes(false));
  hooks.resetFallThroughs();
  hooks.setEngineMode('force');
  try {
    answersAsJs('unfused probe');
    assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'fma' });
    answersAsJs('after the refusal');
    assert.equal(hooks.fallThroughCount(), 2);
  } finally {
    hooks.setEngineMode('table');
    hooks.resetEngine();
  }
});
