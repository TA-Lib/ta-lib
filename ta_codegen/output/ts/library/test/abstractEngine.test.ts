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
 *  111726 MF,CC  First Version
 */

// ta-lib/abstract against ta-lib/js/abstract over every function, as installed
// from the tarball, in force mode: each call runs in WASM exactly when the
// function has an engine export, or falls through for a reason this file
// allows for the shape it sent.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import * as routedAbstract from 'ta-lib/abstract';
import * as jsAbstract from 'ta-lib/js/abstract';
import { TALibError } from 'ta-lib/js';
import { hooks } from './engineHooks.ts';

type Arr = Float64Array | Int32Array;
type Lib = typeof jsAbstract;
type Info = jsAbstract.FuncInfo;

const N = 400;
const PAD = 3;

/** Every input series as a view at a byte offset past 0 of one shared buffer. */
function series(): Record<string, Float64Array> {
  const keys = ['open', 'high', 'low', 'close', 'volume', 'periods'];
  const memory = new Float64Array(keys.length * (N + PAD) + PAD);
  const out: Record<string, Float64Array> = {};
  keys.forEach((key, k) => {
    out[key] = memory.subarray(PAD + k * (N + PAD), PAD + k * (N + PAD) + N);
  });
  let s = 20261117;
  const rnd = (): number => {
    s = (Math.imul(s, 1664525) + 1013904223) >>> 0;
    return s / 4294967296;
  };
  const cents = (x: number): number => Math.round(x * 100) / 100;
  let c = 100;
  for (let i = 0; i < N; i++) {
    const open = cents(c + rnd() - 0.5);
    c = cents(Math.max(5, c + (rnd() - 0.5) * 3));
    out.open[i] = open;
    out.close[i] = c;
    out.high[i] = cents(Math.max(open, c) + rnd() * 2);
    out.low[i] = cents(Math.min(open, c) - rnd() * 2);
    out.volume[i] = cents(1000 + rnd() * 9000);
    out.periods[i] = 2 + Math.floor(rnd() * 29);
  }
  return out;
}

const DATA = series();

function seriesFor(paramName: string): Float64Array {
  if (paramName.includes('Periods')) return DATA.periods;
  return paramName.endsWith('1') ? DATA.open : DATA.close;
}

/**
 * One call shape: the range, and each output's length given the count the
 * range needs. Every output is a view past byte 0 of one buffer the outputs
 * share at disjoint offsets, filled with a value no call computes.
 */
type Shape = { label: string; start: number; end: number; length: (need: number) => number; allowed: readonly string[] };

const SHAPES: readonly Shape[] = [
  { label: 'outputs of exactly the needed length', start: 0, end: N - 1, length: (need) => Math.max(need, 1), allowed: [] },
  { label: 'startIdx 37, endIdx N - 11', start: 37, end: N - 11, length: (need) => Math.max(need, 1), allowed: [] },
  { label: 'outputs one element short', start: 0, end: N - 1, length: (need) => need - 1, allowed: ['buffer'] },
];

function outputs(f: Info, length: number): { memory: ArrayBuffer; outs: Arr[] } {
  const stride = Math.max(length, 0) + PAD;
  const memory = new ArrayBuffer(8 * (PAD + f.outputs.length * stride));
  new Uint8Array(memory).fill(0xa5);
  const outs = f.outputs.map((o, k) => {
    const at = 8 * (PAD + k * stride);
    return o.type === jsAbstract.OutputType.REAL ? new Float64Array(memory, at, length) : new Int32Array(memory, at, length);
  });
  return { memory, outs };
}

function call(lib: Lib, f: Info, shape: Shape, outs: Arr[]): { range?: { begIdx: number; count: number }; error?: unknown } {
  const info = lib.Functions.byName(f.name)!;
  const h = info.newCall();
  info.inputs.forEach((input, i) => {
    if (input.type === lib.InputType.PRICE) {
      h.setPriceInput(i, { open: DATA.open, high: DATA.high, low: DATA.low, close: DATA.close, volume: DATA.volume, openInterest: DATA.volume });
    } else {
      h.setInput(i, seriesFor(input.paramName));
    }
  });
  outs.forEach((out, k) => h.setOutput(k, out));
  try {
    const r = h.call(shape.start, shape.end);
    return { range: { begIdx: r.begIdx, count: r.count } };
  } catch (error) {
    return { error };
  }
}

function engineExports(): Set<string> {
  const w = hooks.engineFor(8) ? hooks.wasmFrame(8) : null;
  assert.ok(w !== null, `no engine: ${JSON.stringify(hooks.engineStatus())}`);
  return new Set(Object.keys(w.x).filter((k) => k.endsWith('_batch')).map((k) => k.slice(0, -'_batch'.length).toUpperCase()));
}

test('ta-lib/abstract, installed, answers every function as ta-lib/js/abstract does, in WASM exactly where the function has an engine export', () => {
  assert.notEqual(routedAbstract.Functions, jsAbstract.Functions);
  assert.ok(DATA.close.byteOffset > 0);
  const exported = engineExports();
  const all = jsAbstract.Functions.all();
  const ranEvery = new Map<string, number>();
  let rejected = 0;
  let elsewhere = 0;
  let started = 0;
  hooks.resetFallThroughs();
  for (const f of all) {
    const engine = exported.has(f.name);
    const lookback = jsAbstract.Functions.byName(f.name)!.newCall().lookback();
    for (const shape of SHAPES) {
      const first = Math.max(shape.start, lookback);
      const need = first > shape.end ? 0 : shape.end - first + 1;
      const length = shape.length(need);
      if (length < 0) continue;
      const where = `${f.name}, ${shape.label}`;
      const mine = outputs(f, length);
      const theirs = outputs(f, length);
      assert.ok(mine.outs.every((o) => o.byteOffset > 0), where);
      hooks.setEngineMode('force');
      const fell = hooks.fallThroughCount();
      const entered = hooks.moduleCalls();
      let got: ReturnType<typeof call>;
      try {
        got = call(routedAbstract, f, shape, mine.outs);
      } finally {
        hooks.setEngineMode('table');
      }
      const fellThrough = hooks.fallThroughCount() - fell;
      const ran = hooks.moduleCalls() - entered;
      const want = call(jsAbstract, f, shape, theirs.outs);
      assert.ok(Buffer.from(mine.memory).equals(Buffer.from(theirs.memory)), `${where}: the output bytes differ`);
      if (want.error !== undefined) {
        assert.ok(want.error instanceof TALibError, `${where}: ta-lib/js/abstract threw ${String(want.error)}`);
        assert.ok(got.error instanceof TALibError, `${where}: ta-lib/abstract gave ${String(got.error ?? JSON.stringify(got.range))}`);
        assert.equal(got.error.code, want.error.code, where);
        assert.equal(got.error.message, want.error.message, where);
        assert.ok(shape.allowed.length > 0, `${where}: ta-lib/js/abstract rejected a shape it should accept: ${want.error.message}`);
        assert.equal(ran, 0, `${where}: a rejected call entered the module`);
        assert.equal(fellThrough, engine ? 1 : 0, where);
        if (engine) {
          assert.ok(shape.allowed.includes(hooks.lastFallThrough()), `${where}: fell through as '${hooks.lastFallThrough()}', not ${shape.allowed.join(' or ')}`);
          rejected++;
        }
        continue;
      }
      assert.equal(got.error, undefined, `${where}: ${String(got.error)}`);
      assert.deepEqual(got.range, want.range, where);
      assert.equal(fellThrough, 0, `${where}: fell through as '${hooks.lastFallThrough()}'`);
      assert.equal(ran, engine ? 1 : 0, `${where}: the module counted ${ran} batch calls`);
      if (engine) ranEvery.set(f.name, (ranEvery.get(f.name) ?? 0) + 1);
      else elsewhere++;
      if (engine && shape.start > 0 && got.range!.count > 0) started++;
    }
  }
  const accepted = SHAPES.filter((s) => s.allowed.length === 0).length;
  const missed = [...exported].filter((name) => ranEvery.get(name) !== accepted);
  assert.deepEqual(missed, [], 'functions with an engine export that did not run in WASM in every accepted shape');
  assert.equal(ranEvery.size, exported.size);
  assert.ok(exported.size < all.length && elsewhere > 0, 'no function without an engine export was called');
  assert.ok(rejected >= exported.size, `${rejected} rejected calls fell through as allowed`);
  assert.ok(started > 0, 'no call with startIdx past 0 produced values');
});
