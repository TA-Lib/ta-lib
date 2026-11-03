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

// The MAVP stream against the batch, bit for bit, for every MAType. Every slot
// of the bank reads its price history from one tape the bank owns, so the legs
// look for a wrong lag in one slot, a tape one slot too short, a peek that sees
// a bar the commit would not, a fork sharing the tape, and the tape drifting
// after an advance:
// - bands whose deepest lag is exactly a power of two ([2,2], [2,32], [2,33],
//   [2,65]), the only place an undersized tape shows;
// - a ramp that selects every slot every nBank + 2 bars, out-of-band periods
//   included, and for WMA and HMA a staircase that holds each period for
//   8p + 1 bars so a re-anchor bar is selected in every slot;
// - a fork driven to the end BEFORE its original, so a shared tape shows;
// - two handles fed alike, one of them advanced once.

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { Core, FuncUnstId, MAType, type MavpStream, mavp, mavpLookback, mavpOpen, mavpOpenAndFill } from '../src/index.ts';

const BANDS: [number, number][] = [[1, 1], [2, 2], [1, 13], [2, 32], [2, 33], [2, 65]];
const RAMP_BARS = 600;

/** Two-decimal prices, so a wrong lag cannot cancel out; flat runs and rare large prints reach KAMA's and WMA's edge paths. */
function prices(n: number): Float64Array {
  let seed = 445;
  let x = 100;
  const out = new Float64Array(n);
  for (let i = 0; i < n; i++) {
    seed = (Math.imul(seed, 1103515245) + 12345) >>> 0;
    if (i % 1500 < 1460) {
      if (i % 997 === 0) x *= Math.floor(i / 997) % 2 === 1 ? 3.7 : 0.29;
      else x += (((seed >>> 8) % 201) - 100) / 100;
    }
    if (x < 1) x = 1 + ((seed >>> 8) % 97) / 100;
    x = Math.trunc(x * 100 + 0.5) / 100;
    out[i] = x;
  }
  return out;
}

function ramp(n: number, [min, max]: [number, number]): Float64Array {
  const cycle = max - min + 3;
  return Float64Array.from({ length: n }, (_, i) => min - 1 + (i % cycle) + (i % 3 === 1 ? 0.37 : 0));
}

/** Period p held for 8p + 1 bars from the first output on, min up to max. */
function staircase(lb: number, [min, max]: [number, number]): Float64Array {
  let len = 0;
  for (let p = min; p <= max; p++) len += 8 * p + 1;
  const out = new Float64Array(lb + 16 + len);
  let p = min;
  let held = 0;
  for (let i = 0; i < out.length; i++) {
    out[i] = p;
    if (i > lb) {
      held++;
      if (held === 8 * p + 1 && p < max) {
        p++;
        held = 0;
      }
    }
  }
  return out;
}

type Case = {
  core: Core;
  ty: MAType;
  band: [number, number];
  shape: string;
  price: Float64Array;
  per: Float64Array;
  batch: Float64Array;
  lb: number;
};

const at = (c: Case, what: string, bar: number): string => `MAVP stream ${what}: MAType ${c.ty} band [${c.band}] ${c.shape} bar ${bar}`;

/** Open at `prefix`, then peek and update every remaining bar; a clone taken at `forkAt` runs to the end first; with `twin`, a second handle takes the same bars and one advance at the midpoint. */
function drive(c: Case, prefix: number, forkAt: number | null, twin: boolean): number {
  const n = c.per.length;
  const [min, max] = c.band;
  const open = (): MavpStream => mavpOpen(c.price.subarray(0, prefix), c.per.subarray(0, prefix), min, max, c.ty, c.core);
  const h = open();
  assert.ok(Object.is(h.value(), c.batch[prefix - 1 - c.lb]), at(c, 'open', prefix - 1));
  const h2 = twin ? open() : null;
  let fork: number[] = [];
  let compared = 0;
  for (let t = prefix; t < n; t++) {
    if (t === forkAt) {
      const f = h.clone();
      fork = [];
      for (let u = t; u < n; u++) fork.push(f.update(c.price[u], c.per[u]));
    }
    if (t === (prefix + n) >> 1) h2?.advance();
    const pk = h.peek(c.price[t], c.per[t]);
    const v = h.update(c.price[t], c.per[t]);
    const want = c.batch[t - c.lb];
    if (!Object.is(v, want)) assert.fail(`${at(c, 'update', t)}: ${v} vs batch ${want}`);
    if (!Object.is(pk, v)) assert.fail(`${at(c, 'peek', t)}: peek ${pk} vs update ${v}`);
    if (forkAt !== null && t >= forkAt && !Object.is(fork[t - forkAt], v)) assert.fail(`${at(c, 'clone', t)}: fork ${fork[t - forkAt]} vs original ${v}`);
    if (h2 !== null) {
      const v2 = h2.update(c.price[t], c.per[t]);
      if (!Object.is(v2, v)) assert.fail(`${at(c, 'advance twin', t)}: twin ${v2} vs ${v}`);
    }
    compared++;
  }
  if (h2 !== null) {
    const a = h.outRange();
    const b = h2.outRange();
    assert.ok(b.begIdx === a.begIdx && b.count === a.count + 1, `advance twin range ${JSON.stringify(b)} vs ${JSON.stringify(a)}`);
  }
  return compared;
}

function streamBank(core: Core): number {
  const types = Object.values(MAType);
  assert.ok(types.length >= 14, `only ${types.length} MATypes`);
  let compared = 0;
  for (const ty of types) {
    for (const band of BANDS) {
      const lb = mavpLookback(band[0], band[1], ty, core);
      const shapes: [string, Float64Array][] = [['ramp', ramp(lb + RAMP_BARS, band)]];
      if (ty === MAType.WMA || ty === MAType.HMA) shapes.push(['staircase', staircase(lb, band)]);
      for (const [shape, per] of shapes) {
        const n = per.length;
        const price = prices(n);
        const batch = new Float64Array(n);
        const r = mavp(0, n - 1, price, per, band[0], band[1], ty, batch, core);
        assert.deepEqual([r.begIdx, r.count], [lb, n - lb]);
        const c: Case = { core, ty, band, shape, price, per, batch, lb };

        const out = new Float64Array(n);
        const f = mavpOpenAndFill(price, per, band[0], band[1], ty, out, core);
        const fr = f.outRange();
        assert.deepEqual([fr.begIdx, fr.count], [lb, n - lb]);
        for (let i = 0; i < fr.count; i++) if (!Object.is(out[i], batch[i])) assert.fail(at(c, 'openAndFill', lb + i));
        compared += fr.count;

        compared += drive(c, lb + 1, null, false);
        compared += drive(c, lb + 2, null, true);
        compared += drive(c, lb + Math.trunc((n - lb) / 3), lb + Math.trunc((n - lb) / 2), false);
      }
    }
  }
  return compared;
}

test('the MAVP bank streams the batch', () => {
  const compared = streamBank(Core.DEFAULT);
  assert.ok(compared > 200_000, `compared only ${compared} values`);
});

test('the MAVP bank streams the batch with an unstable period', () => {
  const compared = streamBank(Core.builder().unstablePeriod(FuncUnstId.ALL, 7).build());
  assert.ok(compared > 200_000, `compared only ${compared} values`);
});
