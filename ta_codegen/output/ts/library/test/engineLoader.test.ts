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
import { defaultEntry, hooks } from './engineHooks.ts';

const lib = (await import(defaultEntry)) as typeof import('ta-lib');
const close = Float64Array.from({ length: 200 }, (_, i) => Math.round((50 + 5 * Math.sin(i / 6) + i / 30) * 100) / 100);

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
  assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'not-loaded' });
  assert.equal(forced(), 1);
  assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'not-loaded' });
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
    assert.equal(forced(), 1, corrupt);
    assert.deepEqual(hooks.engineStatus(), { engine: 'js', reason: 'compile' }, corrupt);
    const status: EngineStatus = await lib.ready();
    assert.deepEqual(status, { engine: 'js', reason: 'compile' }, corrupt);
  }
});

test('ready() imports the bytes and resolves to the WASM engine', async () => {
  hooks.resetEngine();
  const status: EngineStatus = await lib.ready();
  assert.deepEqual(status, { engine: 'wasm', reason: null });
  assert.equal(await lib.ready(), status);
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
  type Api = { Module: unknown; compile: (bytes: Uint8Array) => Promise<unknown> };
  const wasm = (globalThis as unknown as { WebAssembly: Api }).WebAssembly;
  const { Module, compile } = wasm;
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
  wasm.Module = function refused(): never {
    syncCompiles++;
    throw new RangeError('WebAssembly.Module is disallowed on the main thread for a module this size');
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
    wasm.Module = Module;
    wasm.compile = compile;
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
