// Bit-exact port of src/tools/ta_regtest/fuzz_data.h. Every value equals the C
// generator's bit for bit: JS never contracts a*b+c, and each C statement keeps
// its own rounding here. Unsigned 64-bit C arithmetic is BigInt reduced with
// BigInt.asUintN(64, ...) after every wrapping + and *.

export const FUZZ_RANDWALK = 0;
export const FUZZ_MONO_UP = 1;
export const FUZZ_MONO_DOWN = 2;
export const FUZZ_CONSTANT = 3;
export const FUZZ_TIE_HEAVY = 4;
export const FUZZ_EXTREME = 5;
export const FUZZ_WITH_ZEROS = 6;
export const FUZZ_CANDLE = 7;
export const FUZZ_ZEROSUM = 8;
export const FUZZ_DOMAIN_MAX = 9;
export const FUZZ_NSHAPES = 10;

const FUZZ_DOMAIN_BOUND = 3e37;

class SplitMix64 {
  s: bigint;

  constructor(s: bigint) {
    this.s = s;
  }

  next(): bigint {
    this.s = BigInt.asUintN(64, this.s + 0x9E3779B97F4A7C15n);
    let z = this.s;
    z = BigInt.asUintN(64, (z ^ (z >> 30n)) * 0xBF58476D1CE4E5B9n);
    z = BigInt.asUintN(64, (z ^ (z >> 27n)) * 0x94D049BB133111EBn);
    return z ^ (z >> 31n);
  }

  unit(): number {
    return Number(this.next() >> 11n) * (1.0 / 9007199254740992.0);
  }

  bit(): number {
    return Number(this.next() & 1n);
  }
}

function seedState(seed: number, salt: bigint): SplitMix64 {
  return new SplitMix64(
    0x243F6A8885A308D3n ^ BigInt.asUintN(64, BigInt(seed >>> 0) * 0xD1B54A32D192ED03n) ^ salt,
  );
}

type Bars = [Float64Array, Float64Array, Float64Array, Float64Array, Float64Array, Float64Array];

function cdlBar(b: Bars, p: number, n: number, O: number, H: number, L: number, Cl: number): number {
  let hi = H;
  let lo = L;
  if (hi < O) hi = O;
  if (hi < Cl) hi = Cl;
  if (lo > O) lo = O;
  if (lo > Cl) lo = Cl;
  if (p < n) {
    b[0][p] = O; b[1][p] = hi; b[2][p] = lo; b[3][p] = Cl; b[4][p] = 1000.0; b[5][p] = 100.0;
    p++;
  }
  return p;
}

function cdlFlat(b: Bars, p: number, n: number, k: number, base: number): number {
  while (k-- > 0) p = cdlBar(b, p, n, base, base + 1.0, base - 1.0, base);
  return p;
}

function cdlHikkake(b: Bars, p: number, n: number, base: number, w: number, dir: number, brk: number, conf: number): number {
  const H1 = base + w, L1 = base - w;
  const H2 = brk === 1 ? base + w + 0.5 : base + 0.6 * w;
  const L2 = brk === 2 ? base - w - 0.5 : base - 0.6 * w;
  let H3: number, L3: number;
  if (dir > 0) { H3 = brk === 3 ? H2 + 0.4 * w : H2 - 0.4 * w; L3 = L2 - 0.4 * w; }
  else { H3 = brk === 4 ? H2 - 0.4 * w : H2 + 0.4 * w; L3 = L2 + 0.4 * w; }
  p = cdlBar(b, p, n, base, H1, L1, base);
  p = cdlBar(b, p, n, base, H2, L2, base);
  p = cdlBar(b, p, n, base, H3, L3, base);
  if (conf === 2) p = cdlFlat(b, p, n, 3, base);
  if (conf !== 0) { const cc = dir > 0 ? H2 + w : L2 - w; p = cdlBar(b, p, n, cc, cc + 0.5, cc - 0.5, cc); }
  return p;
}

function cdlHikkakeMod(b: Bars, p: number, n: number, base: number, w: number, dir: number, brk: number, conf: number): number {
  const H1 = base + w, L1 = base - w;
  const H2 = brk === 1 ? base + w + 0.5 : base + 0.7 * w;
  const L2 = brk === 1 ? base - w - 0.5 : base - 0.7 * w;
  const H3 = brk === 2 ? H2 + 0.3 * w : base + 0.45 * w;
  const L3 = brk === 2 ? L2 - 0.3 * w : base - 0.45 * w;
  let H4: number, L4: number;
  if (dir > 0) { H4 = brk === 3 ? H3 + 0.3 * w : H3 - 0.3 * w; L4 = L3 - 0.3 * w; }
  else { H4 = brk === 3 ? H3 - 0.3 * w : H3 + 0.3 * w; L4 = L3 + 0.3 * w; }
  const C2 = brk === 4 ? base : (dir > 0 ? L2 : H2);
  p = cdlBar(b, p, n, base, H1, L1, base);
  p = cdlBar(b, p, n, base, H2, L2, C2);
  p = cdlBar(b, p, n, base, H3, L3, base);
  p = cdlBar(b, p, n, base, H4, L4, base);
  if (conf !== 0) { const cc = dir > 0 ? H3 + w : L3 - w; p = cdlBar(b, p, n, cc, cc + 0.5, cc - 0.5, cc); }
  return p;
}

function cdlPrimer(b: Bars, p: number, n: number, k: number, base: number, bd: number, hr: number): number {
  for (let i = 0; i < k; i++) {
    const O = (i & 1) !== 0 ? base : base + bd;
    const C = (i & 1) !== 0 ? base + bd : base;
    p = cdlBar(b, p, n, O, base + bd + hr, base - hr, C);
  }
  return p;
}

// `fuzz_cdl_<name>`: a primer, then one row per bar of open, high, low, close as offsets from base.
type CdlWindow = { readonly name: string; readonly hr: number; readonly bars: readonly (readonly [number, number, number, number])[] };

const CDL_CATALOG: readonly CdlWindow[] = [
  { name: '2crows', hr: 1.0, bars: [[0, 12.5, -0.5, 12.0], [18.0, 18.5, 13.5, 14.0], [16.0, 16.5, 5.5, 6.0]] },
  { name: '3blackcrows', hr: 1.0, bars: [[0, 11, -1, 10], [8, 8, 0, 0], [5, 5, -5, -5], [2, 2, -8, -8]] },
  { name: '3whitesoldiers', hr: 1.0, bars: [[0, 10.2, -0.2, 10.0], [5.0, 15.2, 4.8, 15.0], [10.0, 20.2, 9.8, 20.0]] },
  { name: '3starsinsouth', hr: 1.0, bars: [[8.0, 8.0, -12.0, 0], [6.0, 6.0, -4.0, 2.0], [1.0, 1.0, 0, 0]] },
  { name: '3linestrike', hr: 1.0, bars: [[0, 5.0, -1.0, 4.0], [2.0, 7.0, 1.0, 6.0], [4.0, 9.0, 3.0, 8.0], [10.0, 11.0, -3.0, -2.0]] },
  { name: 'concealbabyswall', hr: 1.0, bars: [[0, 0, -3.0, -3.0], [-4.0, -4.0, -7.0, -7.0], [-9.0, -6.0, -12.0, -11.0], [-5.0, -4.0, -16.0, -15.0]] },
  { name: 'mathold', hr: 1.0, bars: [[0, 21.0, -1.0, 20.0], [31.0, 32.0, 29.0, 30.0], [16.0, 17.0, 14.0, 15.0], [14.0, 15.0, 12.0, 13.0], [35.0, 41.0, 34.0, 40.0]] },
  { name: 'risefall3methods', hr: 1.0, bars: [[0, 12.5, -0.5, 12.0], [10.0, 10.5, 8.5, 9.0], [8.0, 8.5, 6.5, 7.0], [6.0, 6.5, 4.5, 5.0], [6.0, 20.5, 5.5, 20.0]] },
  { name: 'advanceblock', hr: 1.0, bars: [[0, 6.0, 0, 6.0], [5.0, 7.0, 5.0, 7.0], [7.0, 8.0, 7.0, 8.0]] },
  { name: 'inneck', hr: 6.0, bars: [[10.0, 12.0, -1.0, 0], [-5.0, 1.0, -6.0, 0.35]] },
  { name: 'unique3river', hr: 1.0, bars: [[10.0, 11.0, -12.0, -10.0], [5.0, 6.0, -15.0, -5.0], [-4.0, -2.0, -5.0, -3.0]] },
  { name: 'kicking', hr: 1.0, bars: [[20, 20, 0, 0], [30, 50, 30, 50]] },
  { name: 'kickingbylength', hr: 1.0, bars: [[40, 40, 20, 20], [45, 70, 45, 70]] },
  { name: 'darkcloudcover', hr: 1.0, bars: [[0, 21, -1, 20], [30, 31, 4, 5]] },
  { name: 'piercing', hr: 1.0, bars: [[20, 21, -1, 0], [-10, 16, -11, 15]] },
  { name: 'thrusting', hr: 1.0, bars: [[10, 11, -1, 0], [-10, 4, -12, 2.5]] },
  { name: 'homingpigeon', hr: 1.0, bars: [[20, 21, -1, 0], [7, 8, 4, 5]] },
  { name: '3inside', hr: 1.0, bars: [[0, 12.5, -0.5, 12], [6, 7.5, 5.5, 7], [8, 8.5, -2.5, -2]] },
  { name: 'identical3crows', hr: 1.0, bars: [[0, 0, -3, -3], [-3, -3, -6, -6], [-6, -6, -9, -9]] },
  { name: 'stalledpattern', hr: 1.0, bars: [[0, 12, 0, 12], [8, 25, 8, 25], [25, 26, 25, 26]] },
  { name: 'upsidegap2crows', hr: 1.0, bars: [[0, 10.5, -0.5, 10], [16, 16.5, 14.5, 15], [18, 18.5, 11.5, 12]] },
  { name: 'breakaway', hr: 1.0, bars: [[20, 21, 7, 8], [4, 5, -1, 0], [-2, 0, -5, -4], [-7, -6, -11, -9], [5, 7, 4, 6]] },
  { name: 'ladderbottom', hr: 1.0, bars: [[20, 21, 14, 15], [15, 16, 9, 10], [10, 11, 4, 5], [5, 12, -2, 0], [6, 15, 6, 15]] },
  { name: 'xsidegap3methods', hr: 1.0, bars: [[0, 6, -1, 5], [8, 14, 7, 13], [10, 11, 1, 2]] },
];

function cdlCatalog(b: Bars, p: number, n: number): number {
  const base = 100.0;
  for (const w of CDL_CATALOG) {
    p = cdlPrimer(b, p, n, 6, base, 2.0, w.hr);
    for (const [o, h, l, c] of w.bars) {
      p = cdlBar(b, p, n, base + o, base + h, base + l, base + c);
    }
  }
  return p;
}

function candleGen(seed: number, n: number, b: Bars): void {
  const sm = seedState(seed, 0n);
  let p = cdlFlat(b, 0, n, 6, 100.0);
  p = cdlCatalog(b, p, n);
  while (p < n - 16) {
    const base = 100.0 + (sm.unit() - 0.5) * 40.0;
    const w = 8.0 + sm.unit() * 20.0;
    const variant = Math.trunc(sm.unit() * 10.0);
    const dir = sm.bit() !== 0 ? 1 : -1;
    switch (variant) {
      case 0: p = cdlHikkake(b, p, n, base, w, dir, 0, 1); break;
      case 1: p = cdlHikkake(b, p, n, base, w, dir, 0, 2); break;
      case 2: p = cdlHikkake(b, p, n, base, w, dir, 1, 0); break;
      case 3: p = cdlHikkake(b, p, n, base, w, dir, 2, 0); break;
      case 4: p = cdlHikkake(b, p, n, base, w, dir, dir > 0 ? 3 : 4, 0); break;
      case 5: p = cdlHikkakeMod(b, p, n, base, w, dir, 0, 1); break;
      case 6: p = cdlHikkakeMod(b, p, n, base, w, dir, 0, 0); break;
      case 7: p = cdlHikkakeMod(b, p, n, base, w, dir, 1, 0); break;
      case 8: p = cdlHikkakeMod(b, p, n, base, w, dir, 3, 0); break;
      default: p = cdlHikkakeMod(b, p, n, base, w, dir, 4, 1); break;
    }
    p = cdlFlat(b, p, n, 6, base);
  }
  while (p < n) p = cdlFlat(b, p, n, 1, 100.0);
}

function zerosumGen(seed: number, n: number, b: Bars): void {
  const sm = seedState(seed, BigInt(FUZZ_ZEROSUM) << 32n);
  for (let i = 0; i < n; i++) {
    const r = sm.unit();
    let open: number, hi: number, lo: number, close: number, t: number;
    if (r < 0.34) {
      const a = 1.0 + sm.unit() * 50.0;
      hi = a; lo = -a; open = 0.0; close = 0.0;
    } else if (r < 0.50) {
      hi = 0.0; lo = 0.0; open = 0.0; close = 0.0;
    } else {
      const base = 90.0 + sm.unit() * 20.0;
      const w = 0.5 + sm.unit() * 5.0;
      close = base;
      t = sm.unit() - 0.5;
      open = base + t;
      hi = open > close ? open : close;
      hi = hi + w;
      lo = open < close ? open : close;
      lo = lo - w;
    }
    b[0][i] = open; b[1][i] = hi; b[2][i] = lo; b[3][i] = close;
    t = sm.unit() * 1000.0;
    b[4][i] = 1000.0 + t;
    t = sm.unit() * 100.0;
    b[5][i] = 100.0 + t;
  }
}

function domainMaxGen(seed: number, n: number, b: Bars): void {
  const sm = seedState(seed, BigInt(FUZZ_DOMAIN_MAX) << 32n);
  for (let i = 0; i < n; i++) {
    const r = sm.unit();
    const scale = r < 0.90 ? FUZZ_DOMAIN_BOUND : 100.0;
    let t = sm.unit() * 0.07;
    let a = 0.93 + t;
    let hi = a * scale;
    t = sm.unit() * 0.02;
    const w = hi * t;
    let lo = hi - w;
    const span = hi - lo;
    t = sm.unit() * span;
    let close = lo + t;
    t = sm.unit() * span;
    let open = lo + t;
    if (r >= 0.45 && r < 0.90) {
      t = hi; hi = -lo; lo = -t;
      open = -open; close = -close;
    }
    b[0][i] = open; b[1][i] = hi; b[2][i] = lo; b[3][i] = close;
    t = sm.unit() * 0.99;
    a = 0.01 + t;
    b[4][i] = a * FUZZ_DOMAIN_BOUND;
    t = sm.unit() * 0.99;
    a = 0.01 + t;
    b[5][i] = a * FUZZ_DOMAIN_BOUND;
  }
}

/** `fuzz_gen`: fills the first `n` bars of each array from `(shape, seed)`. */
export function fuzzGen(
  shape: number, seed: number, n: number,
  o: Float64Array, h: Float64Array, l: Float64Array, c: Float64Array, v: Float64Array, oi: Float64Array,
): void {
  const b: Bars = [o, h, l, c, v, oi];
  if (shape === FUZZ_CANDLE) { candleGen(seed, n, b); return; }
  if (shape === FUZZ_ZEROSUM) { zerosumGen(seed, n, b); return; }
  if (shape === FUZZ_DOMAIN_MAX) { domainMaxGen(seed, n, b); return; }
  const sm = seedState(seed, BigInt(shape >>> 0) << 32n);
  let walk = 100.0;
  for (let i = 0; i < n; i++) {
    let close: number, t: number;
    if (shape === FUZZ_CONSTANT) {
      o[i] = h[i] = l[i] = c[i] = 42.0;
      v[i] = 1000000.0; oi[i] = 10000.0;
      continue;
    }
    if (shape === FUZZ_TIE_HEAVY) {
      close = 3 + Math.trunc(sm.unit() * 5.0);
      o[i] = close;
      c[i] = close;
      h[i] = close + sm.bit();
      l[i] = close - sm.bit();
      v[i] = (1 + Math.trunc(sm.unit() * 4.0)) * 1000.0;
      oi[i] = 1000.0;
      continue;
    }
    switch (shape) {
      case FUZZ_MONO_UP:
        t = i * 0.5; close = 10.0 + t; break;
      case FUZZ_MONO_DOWN:
        t = i * 0.25; close = 500.0 - t; break;
      case FUZZ_EXTREME:
        t = sm.unit();
        close = sm.bit() !== 0 ? (1.0 + t) * 1.0e9 : (1.0 + t) * 1.0e-7;
        break;
      case FUZZ_WITH_ZEROS: {
        const r = sm.unit();
        if (r < 0.15) close = 0.0;
        else if (r < 0.30) close = -0.0;
        else { t = r - 0.5; close = t * 8.0; }
        break;
      }
      default:
        t = sm.unit() - 0.5;
        t = t * 0.04;
        walk = walk * (1.0 + t);
        close = walk;
        break;
    }
    const mag = Math.abs(close) * 0.01 + 0.001;
    t = sm.unit() - 0.5;
    t = t * mag;
    const open = close + t;
    let hi = open > close ? open : close;
    t = sm.unit() * mag;
    hi = hi + t;
    let lo = open < close ? open : close;
    t = sm.unit() * mag;
    lo = lo - t;
    o[i] = open; h[i] = hi; l[i] = lo; c[i] = close;
    t = sm.unit() * 1.0e6;
    v[i] = 1000.0 + t;
    t = sm.unit() * 1.0e4;
    oi[i] = 100.0 + t;
  }
}

// The hash state is two uint32 halves while bytes stream in, BigInt only for the
// finalizer: FNV's prime is 2^40 + 0x1B3, so h * prime mod 2^64 is exact in
// doubles (every partial product stays below 2^42). Bytes go in little-endian,
// as C hashes them on every host it ships on and as the Java and C# servers do.

const LITTLE_ENDIAN = new Uint8Array(new Uint32Array([1]).buffer)[0] === 1;
const LO = LITTLE_ENDIAN ? 0 : 1;
const HI = LITTLE_ENDIAN ? 1 : 0;

/** `fuzz_hash_init()`; never retype it: it is not the standard FNV basis. */
export function fuzzHashInit(): bigint {
  return 1469598103934665603n;
}

function hashWord(h: Uint32Array, word: number, nbytes: number): void {
  let lo = h[0];
  let hi = h[1];
  for (let k = 0; k < nbytes; k++) {
    lo = (lo ^ ((word >>> (8 * k)) & 0xff)) >>> 0;
    const t = lo * 0x1B3;
    const tLo = t >>> 0;
    hi = (hi * 0x1B3 + (t - tLo) / 4294967296 + ((lo << 8) >>> 0)) >>> 0;
    lo = tLo;
  }
  h[0] = lo;
  h[1] = hi;
}

function hashSplit(h: bigint): Uint32Array {
  return new Uint32Array([Number(h & 0xFFFFFFFFn), Number(h >> 32n)]);
}

function hashJoin(h: Uint32Array): bigint {
  return (BigInt(h[1]) << 32n) | BigInt(h[0]);
}

/** `fuzz_hash_bytes` over the raw bits of `a[0..n)`, read through an integer view so a NaN payload survives. */
export function fuzzHashF64(hsh: bigint, a: Float64Array, n: number): bigint {
  const words = new Uint32Array(a.buffer, a.byteOffset, 2 * n);
  const h = hashSplit(hsh);
  for (let i = 0; i < n; i++) {
    hashWord(h, words[2 * i + LO], 4);
    hashWord(h, words[2 * i + HI], 4);
  }
  return hashJoin(h);
}

/** `fuzz_hash_bytes` over `a[0..n)` as C `int`s. */
export function fuzzHashI32(hsh: bigint, a: Int32Array, n: number): bigint {
  const h = hashSplit(hsh);
  for (let i = 0; i < n; i++) hashWord(h, a[i] >>> 0, 4);
  return hashJoin(h);
}

/** `fuzz_hash_fin`: the murmur fmix64 finalizer. */
export function fuzzHashFin(hsh: bigint): bigint {
  hsh ^= hsh >> 33n;
  hsh = BigInt.asUintN(64, hsh * 0xFF51AFD7ED558CCDn);
  hsh ^= hsh >> 33n;
  hsh = BigInt.asUintN(64, hsh * 0xC4CEB9FE1A85EC53n);
  hsh ^= hsh >> 33n;
  return hsh;
}
