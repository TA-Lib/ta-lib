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

// The metadata tier swept over the whole registry: each function's holder
// against its typed entry points, and each row against itself.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { type Arr, N, series } from './corpus.ts';
import { bytes, type Named, namedSpecs } from './contract.ts';
import { Core, MAType } from '../src/index.ts';
import { type FuncInfo, FunctionDescription, Functions, InputType, OptInputType, OutputType, type ParamHolder } from '../src/metadata/index.ts';

const ALL = Functions.all();
const SPEC = new Map(namedSpecs().map((s) => [s.fn, s]));
const data = series(51);

function spec(f: FuncInfo): Named {
  const s = SPEC.get(f.name);
  assert.ok(s !== undefined, `${f.name}: no typed module`);
  return s;
}

function newOuts(f: FuncInfo): Arr[] {
  return f.outputs.map((o) => (o.type === OutputType.REAL ? new Float64Array(N).fill(Number.NaN) : new Int32Array(N).fill(-7)));
}

/** Every input and output bound; optional parameters left unset. */
function bind(f: FuncInfo, outs: Arr[]): ParamHolder {
  const h = f.newCall();
  f.inputs.forEach((inp, i) => {
    if (inp.type === InputType.PRICE) {
      h.setPriceInput(i, { open: data.inOpen, high: data.inHigh, low: data.inLow, close: data.inClose, volume: data.inVolume, openInterest: data.inVolume });
    } else {
      const series = data[inp.paramName];
      assert.ok(series !== undefined, `${f.name}: no series for ${inp.paramName}`);
      h.setInput(i, series);
    }
  });
  outs.forEach((o, k) => h.setOutput(k, o));
  return h;
}

function typed(s: Named, opts: number[], outs: Arr[]): { begIdx: number; count: number } {
  const ins = s.params.filter((p) => p.kind === 'input').map((p) => data[p.name]);
  return s.batch(0, N - 1, ...ins, ...opts, ...outs) as { begIdx: number; count: number };
}

const sentinels = (f: FuncInfo): number[] =>
  f.optInputs.map((o) => (o.type === OptInputType.REAL_RANGE || o.type === OptInputType.REAL_LIST ? Core.REAL_DEFAULT : Core.INTEGER_DEFAULT));

test('call-by-name matches the typed API bit for bit on every function, with every parameter left unset', () => {
  let compared = 0;
  let nonEmpty = 0;
  for (const f of ALL) {
    const a = newOuts(f);
    const b = newOuts(f);
    const ra = bind(f, a).call(0, N - 1);
    const rb = typed(spec(f), sentinels(f), b);
    assert.deepEqual({ begIdx: ra.begIdx, count: ra.count }, { begIdx: rb.begIdx, count: rb.count }, `${f.name}: range`);
    a.forEach((x, k) => assert.deepEqual(bytes(x, ra.count), bytes(b[k], ra.count), `${f.name}: output ${k}`));
    compared++;
    if (ra.count > 0) nonEmpty++;
  }
  assert.equal(compared, ALL.length);
  assert.ok(nonEmpty >= compared - 5, `only ${nonEmpty} of ${compared} comparisons produced values`);
});

test('a choice-list parameter set to its sentinel, left unset, or set to its declared default runs the same call', () => {
  let covered = 0;
  for (const f of ALL) {
    f.optInputs.forEach((o, p) => {
      if (o.type !== OptInputType.INTEGER_LIST) return;
      const what = `${f.name}.${o.paramName}`;
      const [oU, oS, oE] = [newOuts(f), newOuts(f), newOuts(f)];
      const unset = bind(f, oU);
      const sent = bind(f, oS).setOptInput(p, Core.INTEGER_DEFAULT);
      const expl = bind(f, oE).setOptInput(p, o.defaultValue);
      const [rU, rS, rE] = [unset.call(0, N - 1), sent.call(0, N - 1), expl.call(0, N - 1)];
      assert.deepEqual(rS, rE, `${what}: sentinel range`);
      assert.deepEqual(rU, rE, `${what}: unset range`);
      assert.equal(sent.lookback(), expl.lookback(), `${what}: sentinel lookback`);
      assert.equal(unset.lookback(), expl.lookback(), `${what}: unset lookback`);
      oE.forEach((x, k) => {
        assert.deepEqual(bytes(oS[k], rE.count), bytes(x, rE.count), `${what}: sentinel output ${k}`);
        assert.deepEqual(bytes(oU[k], rE.count), bytes(x, rE.count), `${what}: unset output ${k}`);
      });
      covered++;
    });
  }
  assert.ok(covered >= 13, `covered ${covered} choice-list parameters`);
});

test('the holder call, lookback and display shift take each parameter from its own slot, on every function', () => {
  const MA3 = [MAType.SMA, MAType.EMA, MAType.WMA];
  let compared = 0;
  let distinct = 0;
  for (const f of ALL) {
    const s = spec(f);
    const viaHolder = newOuts(f);
    const viaTyped = newOuts(f);
    const h = bind(f, viaHolder);
    const opts = sentinels(f);
    let any = false;
    f.optInputs.forEach((o, p) => {
      if (o.type === OptInputType.INTEGER_RANGE) {
        opts[p] = Math.min(o.min + 2 + p, o.max);
        any = true;
      } else if (o.type === OptInputType.INTEGER_LIST) {
        opts[p] = MA3[p % 3];
      } else return;
      h.setOptInput(p, opts[p]);
    });
    if (any) distinct++;
    const lb = h.lookback();
    assert.ok(lb >= 0 && lb < N, `${f.name}: holder lookback ${lb}`);
    assert.equal(lb, s.lookback(...opts), `${f.name}: holder lookback vs the typed lookback`);
    const r = h.call(0, N - 1);
    assert.equal(r.begIdx, lb, `${f.name}: holder lookback vs outBegIdx`);
    assert.deepEqual({ ...typed(s, opts, viaTyped) }, { ...r }, `${f.name}: holder range vs the typed range`);
    viaHolder.forEach((x, k) => assert.deepEqual(bytes(x, r.count), bytes(viaTyped[k], r.count), `${f.name}: output ${k}`));
    for (let k = 0; k <= f.outputs.length; k++) assert.equal(h.displayShift(k), s.shift(...opts, k), `${f.name}: display shift ${k}`);
    assert.equal(h.displayShift(f.outputs.length), -2147483648, `${f.name}: an index past the outputs`);
    compared++;
  }
  assert.equal(compared, ALL.length);
  assert.ok(distinct >= 65, `only ${distinct} functions driven with distinct periods`);
});

test('byName folds ASCII case over the whole registry, onto a name stored in upper case', () => {
  const alternating = (n: string): string => [...n].map((c, i) => (i % 2 === 0 ? c.toUpperCase() : c.toLowerCase())).join('');
  let found = 0;
  for (const f of ALL) {
    assert.equal(Functions.byName(f.name.toLowerCase()), f, `${f.name}: lower case`);
    assert.equal(Functions.byName(alternating(f.name)), f, `${f.name}: mixed case`);
    assert.match(f.name, /^[A-Z0-9_]+$/, `${f.name}: stored in its canonical upper case`);
    found++;
  }
  assert.equal(found, ALL.length);
  assert.ok(ALL.some((f) => /[0-9]/.test(f.name)) && ALL.some((f) => f.name.includes('_')));
});

test('the XML description describes every function', () => {
  const xml = FunctionDescription.xml();
  assert.ok(xml.startsWith('<?xml') && xml.trimEnd().endsWith('</FinancialFunctions>'));
  const missing = ALL.filter((f) => !xml.includes(`<Abbreviation>${f.name}</Abbreviation>`)).map((f) => f.name);
  assert.deepEqual(missing, []);
  assert.equal(xml.split('<Abbreviation>').length - 1, ALL.length);
});

test('every optional parameter domain is coherent: named, its default inside it', () => {
  let ranges = 0;
  let lists = 0;
  for (const f of ALL) {
    for (const o of f.optInputs) {
      const where = `${f.name}.${o.paramName}`;
      assert.ok(o.paramName.length > 0 && o.displayName.length > 0, `${where}: named`);
      if (o.type === OptInputType.INTEGER_RANGE || o.type === OptInputType.REAL_RANGE) {
        assert.ok(o.min <= o.defaultValue && o.defaultValue <= o.max, `${where}: default ${o.defaultValue} inside [${o.min}, ${o.max}]`);
        if (o.type === OptInputType.INTEGER_RANGE) {
          assert.ok([o.min, o.max, o.defaultValue].every(Number.isInteger), `${where}: integer bounds`);
        } else {
          assert.ok(Number.isInteger(o.precision) && o.precision >= 0, `${where}: precision ${o.precision}`);
        }
        ranges++;
      } else {
        assert.ok(o.values.length > 0, `${where}: the choice list is populated`);
        assert.ok(o.values.some((v) => v.value === o.defaultValue), `${where}: the default ${o.defaultValue} is one of the choices`);
        assert.equal(new Set(o.values.map((v) => v.value)).size, o.values.length, `${where}: distinct choices`);
        lists++;
      }
    }
  }
  assert.ok(ranges > 100, `swept ${ranges} range parameters`);
  assert.ok(lists > 10, `checked ${lists} choice-list parameters`);
});
