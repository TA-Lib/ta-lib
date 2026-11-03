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

// The corpus as the tests see it: every function module's public batch
// signature, read from the generated source, and seeded OHLCV series.

import assert from 'node:assert/strict';
import { readdirSync, readFileSync } from 'node:fs';

import * as lib from '../src/index.ts';
import { MAType } from '../src/index.ts';

export type Fn = (...args: unknown[]) => unknown;
export type Arr = Float64Array | Int32Array;
export type Range = { begIdx: number; count: number };

export type Param = {
  name: string;
  kind: 'input' | 'int' | 'real' | 'enum' | 'output';
  int32: boolean;
  /** The value the body substitutes for the sentinel; null for an input or output. */
  dflt: number | null;
};
export type Spec = {
  name: string;
  batch: Fn;
  lookback: Fn;
  open: Fn;
  fill: Fn;
  outClass: string | null;
  params: Param[];
};

export const L = lib as unknown as Record<string, unknown>;
export const N = 256;
const FUNC_DIR = new URL('../src/func/', import.meta.url);

function defaultOf(src: string, name: string): number {
  const m = new RegExp(
    `if \\(${name} === Core\\.(?:INTEGER|REAL)_DEFAULT(?: \\|\\| ${name} === MAType\\.DEFAULT)?\\) \\{\\n\\s+${name} = ([^;]+);`,
  ).exec(src);
  assert.ok(m !== null, `no default substitution for ${name}`);
  const v = m[1].startsWith('MAType.') ? MAType[m[1].slice(7) as keyof typeof MAType] : Number(m[1]);
  assert.ok(Number.isFinite(v), `${name}: unreadable default ${m[1]}`);
  return v;
}

export function specs(): Spec[] {
  const out: Spec[] = [];
  for (const file of readdirSync(FUNC_DIR).sort()) {
    const src = readFileSync(new URL(file, FUNC_DIR), 'utf8');
    const cls = /^export class (\w+)Stream \{/m.exec(src);
    if (cls === null) continue;
    const stem = file.replace(/\.ts$/, '');
    const head = /^export function (\w+)\(\n((?: {2}.*\n)+)\): OutRange \{\n((?:.*\n)*?)\}/m.exec(src);
    assert.ok(head !== null, `${file}: no public batch function`);
    const [, pub, list, body] = head;
    const params: Param[] = [];
    for (const line of list.split('\n')) {
      const m = /^ {2}(\w+): (.+?)(?: = .*)?,$/.exec(line);
      if (m === null) continue;
      const [, name, type] = m;
      if (name === 'startIdx' || name === 'endIdx' || name === 'core') continue;
      if (type === 'ArrayLike<number>') params.push({ name, kind: 'input', int32: false, dflt: null });
      else if (type === 'Float64Array' || type === 'Int32Array') {
        params.push({ name, kind: 'output', int32: type === 'Int32Array', dflt: null });
      } else if (type.includes('MAType')) params.push({ name, kind: 'enum', int32: true, dflt: defaultOf(src, name) });
      else {
        const kind = body.includes(`${name} = requireInt32(`) ? 'int' : 'real';
        params.push({ name, kind, int32: false, dflt: defaultOf(src, name) });
      }
    }
    const get = (n: string): Fn => {
      const f = L[n];
      assert.equal(typeof f, 'function', `${n} is not exported`);
      return f as Fn;
    };
    const outs = params.filter((p) => p.kind === 'output').length;
    out.push({
      name: stem,
      batch: get(pub),
      lookback: get(`${stem}Lookback`),
      open: get(`${stem}Open`),
      fill: get(`${stem}OpenAndFill`),
      outClass: outs > 1 ? `${cls[1]}Out` : null,
      params,
    });
  }
  return out;
}

export function rng(seed: number): () => number {
  let s = seed >>> 0;
  return () => {
    s = (s + 0x6d2b79f5) >>> 0;
    let t = s;
    t = Math.imul(t ^ (t >>> 15), t | 1);
    t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

export const r2 = (x: number): number => Math.round(x * 100) / 100;

/** One seeded OHLCV series with two-decimal prices. */
export function series(seed: number): Record<string, Float64Array> {
  const r = rng(seed);
  const o = new Float64Array(N);
  const h = new Float64Array(N);
  const l = new Float64Array(N);
  const c = new Float64Array(N);
  const v = new Float64Array(N);
  const alt = new Float64Array(N);
  const per = new Float64Array(N);
  let last = 50 + r() * 50;
  for (let i = 0; i < N; i++) {
    const open = r2(last * (1 + (r() - 0.5) * 0.02));
    const close = r2(open * (1 + (r() - 0.5) * 0.04));
    o[i] = open;
    c[i] = close;
    h[i] = r2(Math.max(open, close) * (1 + r() * 0.01));
    l[i] = r2(Math.min(open, close) * (1 - r() * 0.01));
    v[i] = r2(1000 + r() * 9000);
    alt[i] = r2(30 + r() * 40);
    per[i] = 2 + Math.floor(r() * 29);
    last = close;
  }
  return { inOpen: o, inHigh: h, inLow: l, inClose: c, inVolume: v, inReal: c, inReal0: c, inReal1: alt, inPeriods: per };
}

export function isBadParam(e: unknown): boolean {
  return e instanceof lib.TALibError && e.code === 'TA_BAD_PARAM';
}
