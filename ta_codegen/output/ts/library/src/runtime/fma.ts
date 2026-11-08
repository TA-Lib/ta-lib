/*! TA-LIB Copyright (c) 1999-2026, Mario Fortier
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

import { float64Bits, float64FromBits } from './bits.ts';

// fmaSlow's window. Each bound keeps one step of fma's arithmetic exact: the
// Veltkamp split cannot overflow, the product's error term is a normal number,
// and TwoSum cannot round to infinity. A NaN or an infinity fails a comparison,
// so a window test is the only guard either path needs.
const SPLIT_MAX = 2 ** 995;
const PRODUCT_MIN = 2 ** -968;
const ADDEND_MAX = 2 ** 1021;

const scratch = new DataView(new ArrayBuffer(8));

// V8 inlines calls into one optimized function only up to about 920 bytecode
// bytes in all, three calls of this body, so the generator copies the body
// verbatim where a function has more: keep it to the common case. Its window
// w (|a|, |b|, |c| < 2^500, |p| > 2^-500) lies inside fmaSlow's. In w, an
// exact product (zero pl) or an exact a*b + c (zero v) leaves th the answer,
// and exact operands make both common. Test a zero operand, not a zero p: an
// underflowed product is not exact.
/** IEEE binary64 fused multiply-add, correctly rounded to nearest even: C's `fma`. */
export function fma(a: number, b: number, c: number): number {
  const p = a * b;
  let t = 134217729 * a;
  const ah = t - (t - a), al = a - ah;
  t = 134217729 * b;
  const bh = t - (t - b), bl = b - bh;
  const pl = ((ah * bh - p) + ah * bl + al * bh) + al * bl;
  const th = c + p, z = th - c, tl = (c - (th - z)) + (p - z);
  const v = tl + pl, v2 = v + v;
  const w = a * a + b * b + c * c < 2 ** 1000 && p * p > 2 ** -1000;
  return w && (pl === 0 || v === 0) ? th
    : w && v2 !== (th + v2) - th ? th + v
    : a === 0 || b === 0 ? p + c : fmaSlow(a, b, c, th, tl, pl);
}

// th + RN(tl + pl) is already correctly rounded unless RN(tl + pl) landed
// exactly on a midpoint of th's grid, where fma's 2v is th's neighbour gap;
// th + RO(tl + pl) is correctly rounded everywhere in the window
// (Boldo-Melquiond).
/** `fma` past its fast path, given that path's th, tl and pl. */
export function fmaSlow(a: number, b: number, c: number, th: number, tl: number, pl: number): number {
  const ap = Math.abs(a * b);
  if (ap > PRODUCT_MIN && ap + Math.abs(c) < ADDEND_MAX && Math.abs(a) + Math.abs(b) < SPLIT_MAX) {
    return th + sumRoundedToOdd(tl, pl, tl + pl);
  }
  return fmaOutsideWindow(a, b, c);
}

// x + y rounded to odd, given s = RN(x + y).
function sumRoundedToOdd(x: number, y: number, s: number): number {
  const z = s - x;
  const err = (x - (s - z)) + (y - z);
  if (err === 0) return s;
  scratch.setFloat64(0, s, true);
  const lo = scratch.getUint32(0, true);
  if ((lo & 1) !== 0) return s;
  // s is nonzero, so the borrow cannot pass zero, and an even low word cannot carry.
  if ((s > 0) === (err > 0)) {
    scratch.setUint32(0, lo + 1, true);
  } else if (lo !== 0) {
    scratch.setUint32(0, lo - 1, true);
  } else {
    scratch.setUint32(0, 0xffffffff, true);
    scratch.setUint32(4, scratch.getUint32(4, true) - 1, true);
  }
  return scratch.getFloat64(0, true);
}

function fmaOutsideWindow(a: number, b: number, c: number): number {
  if (!Number.isFinite(a) || !Number.isFinite(b)) return a * b + c;
  // An infinite c wins even when a*b overflows in double.
  if (!Number.isFinite(c)) return c;
  // The product is an exact signed zero, so one IEEE add gets the sign right.
  if (a === 0 || b === 0) return a * b + c;

  const ea = exponentOf(a);
  const eb = exponentOf(b);
  const mp = significandOf(a) * significandOf(b);
  const ep = ea + eb;
  let n: bigint;
  let e: number;
  if (c === 0) {
    n = mp;
    e = ep;
  } else {
    const mc = significandOf(c);
    const ec = exponentOf(c);
    if (ep <= ec) {
      n = mp + (mc << BigInt(ec - ep));
      e = ep;
    } else {
      n = (mp << BigInt(ep - ec)) + mc;
      e = ec;
    }
  }
  if (n === 0n) return 0;
  return n < 0n ? -roundToDouble(-n, e) : roundToDouble(n, e);
}

// value = significandOf(x) * 2^exponentOf(x), exactly, for finite x.
function significandOf(x: number): bigint {
  const bits = float64Bits(x);
  let m = bits & 0xfffffffffffffn;
  if ((bits & 0x7ff0000000000000n) !== 0n) m |= 0x10000000000000n;
  return bits >> 63n !== 0n ? -m : m;
}

function exponentOf(x: number): number {
  const field = Number((float64Bits(x) >> 52n) & 0x7ffn);
  return field === 0 ? -1074 : field - 1075;
}

// n * 2^e rounded once to nearest even, for n > 0; subnormal and overflowing
// results included.
function roundToDouble(n: bigint, e: number): number {
  let shift = bitLength(n) - 53;
  if (shift < -1074 - e) shift = -1074 - e;
  let q: bigint;
  if (shift <= 0) {
    q = n << BigInt(-shift);
  } else {
    const s = BigInt(shift);
    q = n >> s;
    const rem = n - (q << s);
    const half = 1n << (s - 1n);
    if (rem > half || (rem === half && (q & 1n) === 1n)) q += 1n;
  }
  let r = e + shift;
  if (q === 0x20000000000000n) {
    q = 0x10000000000000n;
    r += 1;
  }
  if (q < 0x10000000000000n) return float64FromBits(q);
  const biased = r + 1075;
  if (biased >= 2047) return Infinity;
  return float64FromBits((BigInt(biased) << 52n) | (q - 0x10000000000000n));
}

function bitLength(n: bigint): number {
  const hex = n.toString(16);
  return (hex.length - 1) * 4 + parseInt(hex[0], 16).toString(2).length;
}
