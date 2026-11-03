// Bounds-checked typed arrays for the TypeScript server, the analogue of the
// JVM's, the CLR's and safe Rust's index checks:
//
//   NODE_OPTIONS=--import=<abs path to this file> ./ta_regtest --codegen --language=c,ts
//
// A typed array read past its end yields undefined and a write is dropped, so an
// off-by-one in a ring buffer or an output store changes values only when the
// harness data happens to route the NaN into a compared output. Here every
// Float64Array, Float32Array and Int32Array constructed after this module loads
// (the library's scratch and rings, and every array the server hands it) throws
// a RangeError on any out-of-range canonical numeric index, read or write.
//
// TA_TS_CHECKED_LOG=<file> appends one line per process that loaded this
// module, so a CI step can prove the servers really ran checked.
//
// `node ts_checked_arrays.mjs` runs the self-test.

import { appendFileSync } from 'node:fs';
import { pathToFileURL } from 'node:url';

const TARGET = new WeakMap();   // proxy -> backing array
const PROXY = new WeakMap();    // backing array -> proxy

const raw = (x) => (x !== null && typeof x === 'object' && TARGET.get(x)) || x;

// The key of a typed array's integer-indexed exotic behaviour: a string whose
// ToString(ToNumber) round-trips, or "-0". Non-integers, negatives, NaN and
// Infinity are such keys too, and are as silent as an index past the end.
function numericKey(key) {
  const c = key.charCodeAt(0);
  if (!((c >= 48 && c <= 57) || c === 45 || c === 73 || c === 78)) return undefined;
  if (key === '-0') return -0;
  const n = +key;
  return String(n) === key ? n : undefined;
}

function outOfRange(t, key, op) {
  return new RangeError(
    `checked ${t.constructor.name}: ${op} at index ${key} of length ${t.length}`);
}

const METHOD = new WeakMap();   // built-in method -> wrapper

// The built-ins clamp a range past either end and count a negative index from
// the end, so C's memcpy written as subarray/fill/copyWithin would stay silent.
const PROTO = Object.getPrototypeOf(Int32Array.prototype);
const isIndex = (x, len) => Number.isInteger(x) && x >= 0 && x <= len && !Object.is(x, -0);
function bounds(t, name, start, end) {
  const len = t.length;
  const s = start === undefined ? 0 : start;
  const e = end === undefined ? len : end;
  if (!isIndex(s, len) || !isIndex(e, len) || s > e) {
    throw new RangeError(`checked ${t.constructor.name}: ${name}(${start}, ${end}) of length ${len}`);
  }
  return e - s;
}
const RANGED = new Map([
  [PROTO.subarray, (t, a) => bounds(t, 'subarray', a[0], a[1])],
  [PROTO.slice, (t, a) => bounds(t, 'slice', a[0], a[1])],
  [PROTO.fill, (t, a) => bounds(t, 'fill', a[1], a[2])],
  [PROTO.copyWithin, (t, a) => {
    const n = bounds(t, 'copyWithin', a[1], a[2]);
    if (!isIndex(a[0], t.length) || a[0] + n > t.length) {
      throw new RangeError(`checked ${t.constructor.name}: copyWithin(${a[0]}, ...) of ${n} past length ${t.length}`);
    }
  }],
]);

// Built-ins need the real array as `this` (a Proxy has no typed-array slots).
// A typed array they return is wrapped, and `this` comes back as the proxy.
function method(fn) {
  let w = METHOD.get(fn);
  if (w) return w;
  w = function (...args) {
    const t = raw(this);
    RANGED.get(fn)?.(t, args);
    const r = fn.apply(t, args.map(raw));
    if (r === t) return this;
    return isUnchecked(r) ? check(r) : r;
  };
  METHOD.set(fn, w);
  return w;
}

const HANDLER = {
  get(t, key) {
    if (typeof key === 'string') {
      const i = numericKey(key);
      if (i !== undefined) {
        if (Number.isInteger(i) && i >= 0 && i < t.length && !Object.is(i, -0)) return t[i];
        throw outOfRange(t, key, 'read');
      }
    }
    const v = Reflect.get(t, key, t);
    return typeof v === 'function' && key !== 'constructor' ? method(v) : v;
  },
  set(t, key, value) {
    if (typeof key === 'string') {
      const i = numericKey(key);
      if (i !== undefined) {
        if (Number.isInteger(i) && i >= 0 && i < t.length && !Object.is(i, -0)) {
          t[i] = value;
          return true;
        }
        throw outOfRange(t, key, 'write');
      }
    }
    return Reflect.set(t, key, value, t);
  },
};

function check(t) {
  let p = PROXY.get(t);
  if (p) return p;
  p = new Proxy(t, HANDLER);
  TARGET.set(p, t);
  PROXY.set(t, p);
  return p;
}

const KINDS = ['Float64Array', 'Float32Array', 'Int32Array'];
const BASE = Object.fromEntries(KINDS.map((k) => [k, globalThis[k]]));
const isUnchecked = (x) =>
  x !== null && typeof x === 'object' && !TARGET.has(x) && KINDS.some((k) => x instanceof BASE[k]);

function checkedKind(Base) {
  const Checked = class extends Base {
    constructor(...args) {
      super(...args.map(raw));
      return check(this);
    }
    // A built-in validates what a species constructor returns, and a Proxy fails
    // that; so built-ins make plain arrays and `method` wraps them.
    static get [Symbol.species]() { return Base; }
    static [Symbol.hasInstance](x) { return x instanceof Base; }
    static from(...args) { return check(Base.from(...args.map(raw))); }
    static of(...args) { return check(Base.of(...args)); }
  };
  Object.defineProperty(Checked, 'name', { value: Base.name });
  return Checked;
}

for (const k of KINDS) globalThis[k] = checkedKind(BASE[k]);

if (process.env.TA_TS_CHECKED_LOG)
  appendFileSync(process.env.TA_TS_CHECKED_LOG, `${process.pid} ${process.argv[1] ?? ''}\n`);

function selfTest() {
  const expectThrow = (what, f) => {
    try { f(); } catch (e) { if (e instanceof RangeError) return; throw e; }
    throw new Error(`self-test: ${what} did not throw`);
  };
  const expect = (what, ok) => { if (!ok) throw new Error(`self-test: ${what}`); };
  let n = 0;
  for (const k of KINDS) {
    const C = globalThis[k];
    const a = new C(4);
    expectThrow(`${k} read [4]`, () => a[4]);
    expectThrow(`${k} write [4]`, () => { a[4] = 1; });
    expectThrow(`${k} read [-1]`, () => a[-1]);
    expectThrow(`${k} write [1.5]`, () => { a[1.5] = 1; });
    expectThrow(`${k} read ["-0"]`, () => a['-0']);
    a[3] = 7;
    expect(`${k} in-range write`, a[3] === 7);
    expect(`${k} fill returns the checked array`, a.fill(2) === a && a[0] === 2);
    a.set([1, 2], 2);
    a.copyWithin(0, 2);
    expect(`${k} set/copyWithin`, a[0] === 1 && a[1] === 2);
    const views = [a.subarray(1, 3), a.slice(0, 2), a.subarray(0, 2).map((x) => x), C.from([1, 2]),
      C.of(1, 2), new C(a.subarray(2)), new C(a.buffer, C.BYTES_PER_ELEMENT, 2)];
    for (const v of views) {
      expect(`${k} derived view is ${k}`, v instanceof C && v.length === 2);
      expectThrow(`${k} derived view read [2]`, () => v[2]);
      n++;
    }
    expectThrow(`${k} subarray past the end`, () => a.subarray(2, 9));
    expectThrow(`${k} subarray from a negative begin`, () => a.subarray(-1, 4));
    expectThrow(`${k} subarray with begin > end`, () => a.subarray(3, 2));
    expectThrow(`${k} slice past the end`, () => a.slice(0, 5));
    expectThrow(`${k} fill past the end`, () => a.fill(9, 2, 9));
    expectThrow(`${k} copyWithin past the end`, () => a.copyWithin(3, 0, 4));
    expectThrow(`${k} set past the end`, () => a.set([1, 2, 3], 2));
    expect(`${k} an empty range at the end`, a.subarray(4).length === 0 && a.subarray(4, 4).length === 0);
    const b = new C(4);
    b.set(a);
    expect(`${k} set from a checked source`, b[0] === a[0]);
    expect(`${k} iteration`, [...a].length === 4 && Array.from(a.subarray(0, 2)).length === 2);
  }
  console.log(`ts_checked_arrays self-test: ok (${n} derived views checked)`);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) selfTest();
