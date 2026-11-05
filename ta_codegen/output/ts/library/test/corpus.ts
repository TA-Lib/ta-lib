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

// The corpus as the tests see it: every function module's signature, read from
// the generated source, and seeded OHLCV series. A Spec's `batch`, `lookback`,
// `shift`, `open` and `fill` take C's argument order (inputs, parameters,
// outputs, each in its registry order) and make the public call, so a sweep
// can vary one argument by its registry position; `api` holds the public
// functions themselves.

import assert from 'node:assert/strict';
import { readdirSync, readFileSync } from 'node:fs';

import * as lib from '../src/index.ts';
import { MAType, OutRange } from '../src/index.ts';

export type Fn = (...args: unknown[]) => unknown;
export type Arr = Float64Array | Int32Array;
export type Range = { begIdx: number; count: number };

export type Param = {
  /** C's name, as the registry spells it. */
  name: string;
  kind: 'input' | 'int' | 'real' | 'enum' | 'output';
  int32: boolean;
  /** The value the body substitutes for the sentinel; null for an input or output. */
  dflt: number | null;
  /**
   * The public spelling: an option or buffer key, or for an input the argument
   * that carries it (`real`, `prices.high`), as every message names it.
   */
  key: string;
  /** For an output, its key in the result (`values` for a single output); otherwise null. */
  result: string | null;
};

/** What a public batch or openAndFill returned, outputs in registry order. */
export type Full = { range: OutRange; outs: Arr[]; stream?: unknown };

export type Spec = {
  name: string;
  batch: Fn;
  /** `batch` returning the outputs too. */
  batchFull: (...args: unknown[]) => Full;
  lookback: Fn;
  /** Display shift: the parameters, then the output index. */
  shift: Fn;
  open: Fn;
  /** Returns the stream; the result beside it is checked against it. */
  fill: Fn;
  fillFull: (...args: unknown[]) => Full;
  /** The public arguments before the options object, from inputs in registry order. */
  args: (ins: readonly unknown[]) => unknown[];
  /** The options object for parameters and buffers in registry order: one key per entry given, an `undefined` one included. */
  options: (opts: readonly unknown[], outs?: readonly unknown[]) => Record<string, unknown>;
  api: { batch: Fn; lookback: Fn; shift: Fn; open: Fn; fill: Fn };
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

function bodyOf(src: string, head: RegExp, what: string): RegExpExecArray {
  const m = head.exec(src);
  assert.ok(m !== null, what);
  return m;
}

function isView(a: unknown): a is ArrayBufferView {
  return ArrayBuffer.isView(a);
}

export function specs(): Spec[] {
  const out: Spec[] = [];
  for (const file of readdirSync(FUNC_DIR).sort()) {
    const src = readFileSync(new URL(file, FUNC_DIR), 'utf8');
    const cls = /^export class (\w+)Stream \{/m.exec(src);
    if (cls === null) continue;
    const stem = file.replace(/\.ts$/, '');
    const [, pub, list, body] = bodyOf(
      src,
      /^export function (\w+)Positional\(\n((?: {2}.*\n)+)\): OutRange \{\n((?:.*\n)*?)\}/m,
      `${file}: no positional batch function`,
    );
    // A public name that is a reserved word takes a trailing underscore (`var_`).
    const [, apiName, sigArgs, apiBody] = bodyOf(
      src,
      new RegExp(`^export function (${pub}_?)\\((.*), options\\?: \\w+Options\\): \\w+Result \\{\\n((?:.*\\n)*?)\\}`, 'm'),
      `${file}: no public batch function ${pub}`,
    );
    const call = bodyOf(apiBody, new RegExp(`^ {2}const range = ${pub}Positional\\(startIdx, endIdx, (.*)\\);$`, 'm'), `${file}: no positional call`)[1].split(', ');
    assert.equal(call.pop(), 'core', `${file}: core is not the last positional argument`);
    const results = new Map([...bodyOf(apiBody, /^ {2}return \{ begIdx: range\.begIdx, (.*) \};$/m, `${file}: no return`)[1].matchAll(/(\w+): (\w+)\.subarray\(0, range\.count\)/g)].map((m) => [m[2], m[1]]));
    const params: Param[] = [];
    for (const line of list.split('\n')) {
      const m = /^ {2}(\w+): (.+?)(?: = .*)?,$/.exec(line);
      if (m === null) continue;
      const [, name, type] = m;
      if (name === 'startIdx' || name === 'endIdx' || name === 'core') continue;
      const key = call[params.length];
      const base = { name, int32: false, dflt: null, key, result: null };
      if (type === 'ArrayLike<number>') params.push({ ...base, kind: 'input' });
      else if (type === 'Float64Array' || type === 'Int32Array') {
        assert.equal(key, name, `${file}: output ${name} passed as ${key}`);
        const buf = bodyOf(apiBody, new RegExp(`^ {2}const ${name} = o\\.(\\w+) === undefined \\? new ${type}\\(capacity\\) : o\\.\\1;$`, 'm'), `${file}: no buffer key for ${name}`)[1];
        const result = results.get(name);
        assert.ok(result !== undefined, `${file}: ${name} is not returned`);
        params.push({ ...base, kind: 'output', int32: type === 'Int32Array', key: buf, result });
      } else {
        assert.equal(key, name, `${file}: parameter ${name} passed as ${key}`);
        const opt = bodyOf(apiBody, new RegExp(`^ {2}const ${name} = orDefault\\(o\\.(\\w+), Core\\.(?:INTEGER|REAL)_DEFAULT\\);$`, 'm'), `${file}: no option key for ${name}`)[1];
        if (type.includes('MAType')) params.push({ ...base, kind: 'enum', int32: true, dflt: defaultOf(src, name), key: opt });
        else {
          const kind = body.includes(`${name} = requireInt32(`) ? 'int' : 'real';
          params.push({ ...base, kind, dflt: defaultOf(src, name), key: opt });
        }
      }
    }
    assert.equal(params.length, call.length, `${file}: ${params.length} parameters, ${call.length} positional arguments`);
    const get = (n: string): Fn => {
      const f = L[n];
      assert.equal(typeof f, 'function', `${n} is not exported`);
      return f as Fn;
    };
    const inputs = params.filter((p) => p.kind === 'input');
    const opts = params.filter((p) => p.kind !== 'input' && p.kind !== 'output');
    const outputs = params.filter((p) => p.kind === 'output');
    const argList = [...sigArgs.matchAll(/(\w+): (\{[^}]*\}|ArrayLike<number>)/g)].map((m) => ({ name: m[1], price: m[2].startsWith('{') }));
    assert.deepEqual(
      inputs.map((p) => p.key.split('.')[0]).filter((a, i, all) => all.indexOf(a) === i),
      argList.map((a) => a.name),
      `${file}: inputs vs the public arguments`,
    );
    const args = (ins: readonly unknown[]): unknown[] =>
      argList.map((a) =>
        a.price
          ? Object.fromEntries(inputs.flatMap((p, i) => (p.key.startsWith(`${a.name}.`) ? [[p.key.slice(a.name.length + 1), ins[i]]] : [])))
          : ins[inputs.findIndex((p) => p.key === a.name)],
      );
    const options = (o: readonly unknown[], b: readonly unknown[] = []): Record<string, unknown> => {
      const r: Record<string, unknown> = {};
      opts.forEach((p, k) => {
        if (k < o.length) r[p.key] = o[k];
      });
      outputs.forEach((p, k) => {
        if (k < b.length) r[p.key] = b[k];
      });
      return r;
    };
    const api = {
      batch: get(apiName),
      lookback: get(`${stem}Lookback`),
      shift: get(`${stem}DisplayShift`),
      open: get(`${stem}Open`),
      fill: get(`${stem}OpenAndFill`),
    };
    const name = stem;
    // A result's outputs: each exactly `count` long and of its own type, a given
    // buffer returned as a view of its own memory from its first element, an
    // allocated one sharing memory with no given buffer and no input.
    const outsOf = (what: string, r: Record<string, unknown>, ins: readonly unknown[], given: readonly unknown[], extra: string[]): Full => {
      assert.deepEqual(Object.keys(r).sort(), ['begIdx', ...extra, ...outputs.map((p) => p.result as string)].sort(), `${name} ${what}: result keys`);
      const got = outputs.map((p) => r[p.result as string] as Arr);
      const count = got[0].length;
      got.forEach((a, j) => {
        const p = outputs[j];
        assert.ok(a instanceof (p.int32 ? Int32Array : Float64Array), `${name} ${what}: ${p.result} is a ${String(a?.constructor?.name)}`);
        assert.equal(a.length, count, `${name} ${what}: ${p.result} has length ${a.length}, ${outputs[0].result} ${count}`);
        const g = given[j];
        if (isView(g)) {
          assert.ok(a.buffer === g.buffer && a.byteOffset === g.byteOffset, `${name} ${what}: ${p.result} is not a view of ${p.key} from its first element`);
        } else {
          for (const o of [...given, ...ins]) {
            if (isView(o)) assert.notEqual(a.buffer, o.buffer, `${name} ${what}: an allocated ${p.result} shares memory with an argument`);
          }
        }
      });
      const begIdx = r.begIdx as number;
      if (count === 0) assert.equal(begIdx, 0, `${name} ${what}: an empty result's begIdx`);
      return { range: new OutRange(begIdx, count), outs: got };
    };
    const split = (a: readonly unknown[], first: number): [unknown[], unknown[], unknown[]] => {
      assert.equal(a.length, first + inputs.length + opts.length + outputs.length, `${name}: adapter called with ${a.length} arguments`);
      const ins = a.slice(first, first + inputs.length);
      const o = a.slice(first + inputs.length, first + inputs.length + opts.length);
      return [ins, o, a.slice(first + inputs.length + opts.length)];
    };
    const batchFull = (...a: unknown[]): Full => {
      const [ins, o, b] = split(a, 2);
      const r = api.batch(...args(ins), { startIdx: a[0], endIdx: a[1], ...options(o, b) }) as Record<string, unknown>;
      return outsOf('batch', r, ins, b, []);
    };
    const fillFull = (...a: unknown[]): Full => {
      const [ins, o, b] = split(a, 0);
      const r = api.fill(...args(ins), options(o, b)) as Record<string, unknown>;
      const full = outsOf('openAndFill', r, ins, b, ['stream']);
      const s = (r.stream as { outRange(): OutRange }).outRange();
      assert.deepEqual(full.range, new OutRange(s.begIdx, s.count), `${name} openAndFill: the result's range vs the stream's`);
      return { ...full, stream: r.stream };
    };
    out.push({
      name,
      batch: (...a) => batchFull(...a).range,
      batchFull,
      lookback: (...o) => api.lookback(options(o)),
      shift: (...a) => api.shift(a[opts.length], options(a.slice(0, opts.length))),
      open: (...a) => {
        assert.equal(a.length, inputs.length + opts.length, `${name} open: adapter called with ${a.length} arguments`);
        return api.open(...args(a.slice(0, inputs.length)), options(a.slice(inputs.length)));
      },
      fill: (...a) => fillFull(...a).stream,
      fillFull,
      args,
      options,
      api,
      outClass: outputs.length > 1 ? `${cls[1]}Out` : null,
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
