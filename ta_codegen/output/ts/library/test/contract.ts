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
 *  110226 MF,CC  First Version
 */

// The corpus joined to the registry, for the contract suites: each function's
// registry name is the prefix of every message it throws.

import assert from 'node:assert/strict';

import { type Arr, type Param, type Spec, specs } from './corpus.ts';
import { Core, TALibError } from '../src/index.ts';
import { type FuncInfo, Functions } from '../src/metadata/index.ts';

export type Named = Spec & { fn: string; info: FuncInfo };

export function namedSpecs(): Named[] {
  const byKey = new Map(Functions.all().map((f) => [f.name.replace(/_/g, '').toLowerCase(), f]));
  return specs().map((s) => {
    const info = byKey.get(s.name.toLowerCase());
    assert.ok(info !== undefined, `${s.name}: no registry row`);
    assert.deepEqual(optsOf(s).map((p) => p.name), info.optInputs.map((o) => o.paramName), `${s.name}: parameter order`);
    assert.deepEqual(outputsOf(s).map((p) => p.name), info.outputs.map((o) => o.paramName), `${s.name}: output order`);
    return { ...s, fn: info.name, info };
  });
}

export const inputsOf = (s: Spec): Param[] => s.params.filter((p) => p.kind === 'input');
export const outputsOf = (s: Spec): Param[] => s.params.filter((p) => p.kind === 'output');
export const optsOf = (s: Spec): Param[] => s.params.filter((p) => p.kind !== 'input' && p.kind !== 'output');
export const sentinels = (s: Spec): number[] =>
  optsOf(s).map((p) => (p.kind === 'real' ? Core.REAL_DEFAULT : Core.INTEGER_DEFAULT));

export const REAL_FILL = -3e37;
export const INT_FILL = -7;

/** An output no call can produce, so an untouched one is told from a written one. */
export function filled(p: Param, n: number): Arr {
  return p.int32 ? new Int32Array(n).fill(INT_FILL) : new Float64Array(n).fill(REAL_FILL);
}

export function bytes(a: Arr, n = a.length): Uint8Array {
  return new Uint8Array(a.buffer, a.byteOffset, n * a.BYTES_PER_ELEMENT);
}

export function untouched(a: Arr): boolean {
  const fill = a instanceof Int32Array ? INT_FILL : REAL_FILL;
  return a.every((x) => x === fill);
}

/** The TALibError `fn` throws; anything else, or no throw, fails. */
export function caught(fn: () => unknown, what: string): TALibError {
  try {
    fn();
  } catch (e) {
    if (e instanceof TALibError) return e;
    assert.fail(`${what}: threw ${String(e)}, not a TALibError`);
  }
  assert.fail(`${what}: accepted`);
}

/** The call throws `code`; with `message`, exactly that message, or one it starts with when it ends in '...'. */
export function rejects(fn: () => unknown, code: string, what: string, message?: string): TALibError {
  const e = caught(fn, what);
  assert.equal(e.code, code, `${what}: ${e.message}`);
  if (message !== undefined) {
    if (message.endsWith('...')) assert.ok(e.message.startsWith(message.slice(0, -3)), `${what}: "${e.message}" vs "${message}"`);
    else assert.equal(e.message, message, what);
  }
  return e;
}

/**
 * `e` names parameter `k` of `s` by its options key, the value `v` it was given
 * and its accepted values: the registry's bounds, read back as numbers, or the
 * enum it must be.
 */
export function assertNamesRange(s: Named, k: number, entry: string, v: number, e: TALibError, what: string): void {
  const p = optsOf(s)[k];
  const head = `${entry}: ${p.key} must be `;
  const tail = `, got ${String(v)}`;
  assert.ok(e.message.startsWith(head) && e.message.endsWith(tail), `${what}: "${e.message}" does not name ${p.key} and ${String(v)}`);
  const accepted = e.message.slice(head.length, -tail.length);
  if (p.kind === 'enum') {
    assert.equal(accepted, 'a MAType', what);
    return;
  }
  const o = s.info.optInputs[k] as { min: number; max: number };
  const m = /^in (\S+)\.\.(\S+)$/.exec(accepted);
  assert.ok(m !== null, `${what}: "${accepted}" is not a range`);
  assert.deepEqual([Number(m[1]), Number(m[2])], [o.min, o.max], `${what}: the bounds of "${accepted}"`);
  if (p.kind === 'int') assert.equal(accepted, `in ${o.min}..${o.max}`, what);
}

/** An in-int32 value outside the parameter's domain, so the int32 test passes and the range test fails. */
export function outOfDomain(s: Named, k: number): number {
  const p = optsOf(s)[k];
  if (p.kind === 'real') return Number.NaN;
  if (p.kind === 'enum') return 99;
  const o = s.info.optInputs[k] as { min: number; max: number };
  return o.min - 1 > Core.INTEGER_MIN ? o.min - 1 : o.max + 1;
}
