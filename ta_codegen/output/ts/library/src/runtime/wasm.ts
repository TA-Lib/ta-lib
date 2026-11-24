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


import { float64, overlaps } from './checks.ts';
import { Core } from './core.ts';

/** @internal */
export interface WasmModule {
  readonly wasmModule?: never;
}

/** @internal */
export interface WasmMemory {
  readonly buffer: ArrayBuffer;
}

/** @internal */
export interface WasmInstance {
  readonly exports: { readonly [name: string]: unknown };
}

declare const WebAssembly: {
  readonly Module: new (bytes: Uint8Array) => WasmModule;
  readonly Instance: new (module: WasmModule, imports: object) => WasmInstance;
  readonly Memory: abstract new (...args: never[]) => WasmMemory;
  readonly RuntimeError: abstract new (...args: never[]) => Error;
  compile(bytes: Uint8Array): Promise<WasmModule>;
  instantiate(module: WasmModule, imports: object): Promise<WasmInstance>;
};
declare function atob(data: string): string;

/** @internal */
export type WasmBatch = (...args: number[]) => number;

/** @internal The most bytes of inputs and outputs a call may place in linear memory. */
export const BUDGET = 1 << 28;
const HIGH_WATER = 1 << 29;
/** @internal Calls that threw inside the module before the engine is given up. */
export const MAX_TRAPS = 3;
const DECLINED = -2147483648;
const OUT_OF_FRAME = -2147483647;
/** Read from an array so that no compiler sees the probe's operands as constants. */
const PROBE = [1 + 2 ** -30, -1];
const FUSED = 2 ** -29 + 2 ** -60;
/** @internal A module whose one export is lane 0 of `f64x2.relaxed_madd` over its three splatted arguments. */
export const PREFLIGHT = 'AGFzbQEAAAABCAFgA3x8fAF8AwIBAAcFAQFwAAAKFgEUACAA/RQgAf0UIAL9FP2HAv0hAAs=';

/**
 * Why the WebAssembly engine is not loaded:
 * - `'not-loaded'`: the engine has not been loaded yet. Under the `node`, `bun`
 *   and `deno` export conditions it loads on the first call that needs it;
 *   under any other, only when {@link ready} is called.
 * - `'unavailable'`: the engine's bytes could not be loaded, or this entry has
 *   no engine (`ta-lib/js/abstract`, and the entries a `workerd`, `edge-light`
 *   or `react-native` build resolves to).
 * - `'compile'`: the runtime cannot compile the engine, as one without
 *   relaxed SIMD or under a Content-Security-Policy that forbids WebAssembly does.
 * - `'instantiate'`: the engine, or its preflight module, compiled but could not be
 *   instantiated.
 * - `'fma'`: this runtime does not fuse the multiply-add the engine relies on, so
 *   its values would differ from the TypeScript engine's.
 * - `'trap'`: the engine failed during calls too many times and was given up.
 *
 * A minor release may add a value: handle one you do not know.
 */
export type EngineReason = 'not-loaded' | 'unavailable' | 'compile' | 'instantiate' | 'fma' | 'trap';

/**
 * Whether the WebAssembly engine is loaded; what {@link ready} resolves to. A
 * loaded engine runs the batch calls the routing table sends it, by function
 * and the number of bars a call reads; every other call runs in TypeScript.
 */
export interface EngineStatus {
  /**
   * `'wasm'` when the WebAssembly engine is loaded, `'js'` when every call runs
   * in TypeScript. Both give the same values. A minor release may add a value.
   */
  readonly engine: 'wasm' | 'js';
  /** Why `engine` is `'js'`; `null` when it is `'wasm'`. */
  readonly reason: EngineReason | null;
}

let source: string | null = null;
let compiled: WasmModule | null = null;
let live: WasmEngine | null = null;
let failure: EngineReason | null = null;
let traps = 0;
let loading: Promise<EngineStatus | null> | null = null;
let generation = 0;
let fallThroughs = 0;
let lastReason = '';
let lastThrown: unknown = undefined;
let highWater = HIGH_WATER;
let budget = BUDGET;
let retired = 0;
let verdict: 'compile' | 'instantiate' | 'fma' | null | undefined;

/** @internal */
export type BytesImport = () => Promise<{ readonly WASM_BASE64: string }>;

let importBytes: BytesImport = () => import('./wasmBytes.js');

/** @internal The live instance: its exports, and views over its memory that a call re-derives before use. */
export class WasmEngine {
  readonly x: { readonly [name: string]: WasmBatch };
  f64: Float64Array;
  base = 0;
  private u32: Uint32Array;
  private buffer: ArrayBuffer;
  private readonly memory: WasmMemory;
  private readonly rangeAt: number;

  constructor(x: { readonly [name: string]: WasmBatch }, memory: WasmMemory, rangeAddress: number) {
    this.x = x;
    this.memory = memory;
    this.rangeAt = rangeAddress / 4;
    this.buffer = memory.buffer;
    this.f64 = new Float64Array(this.buffer);
    this.u32 = new Uint32Array(this.buffer);
  }

  refresh(): void {
    const buffer = this.memory.buffer;
    if (buffer !== this.buffer) {
      this.buffer = buffer;
      this.f64 = new Float64Array(buffer);
      this.u32 = new Uint32Array(buffer);
    }
  }

  begIdx(): number {
    this.refresh();
    return this.u32[this.rangeAt];
  }

  reals(address: number, count: number): Float64Array<ArrayBuffer> {
    const out = new Float64Array(count === 0 ? 1 : count);
    if (count === 0) {
      return out.subarray(0, 0);
    }
    out.set(new Float64Array(this.memory.buffer, address, count));
    return out;
  }

  ints(address: number, count: number): Int32Array<ArrayBuffer> {
    const out = new Int32Array(count === 0 ? 1 : count);
    if (count === 0) {
      return out.subarray(0, 0);
    }
    out.set(new Int32Array(this.memory.buffer, address, count));
    return out;
  }

  /** `count` values copied out of linear memory into `into`, which must hold them. */
  realsInto(address: number, count: number, into: Float64Array): void {
    if (count > 0) {
      into.set(new Float64Array(this.memory.buffer, address, count));
    }
  }

  intsInto(address: number, count: number, into: Int32Array): void {
    if (count > 0) {
      into.set(new Int32Array(this.memory.buffer, address, count));
    }
  }

  /** Adds `by` to `count` int32s in linear memory: bar positions the module counted from bar `by`. */
  shiftInts(address: number, count: number, by: number): void {
    if (by !== 0 && count > 0) {
      const a = new Int32Array(this.memory.buffer, address, count);
      for (let i = 0; i < count; i++) {
        a[i] += by;
      }
    }
  }

  memoryBytes(): number {
    return this.memory.buffer.byteLength;
  }

  /** Linear memory never shrinks: an instance past the high-water mark is dropped once its results are copied out. */
  done(): void {
    if (live === this && this.memory.buffer.byteLength > highWater) {
      retire(this);
    }
  }
}

function retire(engine: WasmEngine): void {
  try {
    retired += engine.x.batch_calls() >>> 0;
  } catch {
    // A half-dead instance may not answer; its count is then lost.
  }
  live = null;
}

/**
 * @internal 0 routes by the routing table; 1 routes every call and falls
 * through as the table mode does; 2 routes every call and throws where a call
 * `ta-lib/js` accepts would fall through.
 */
export const engineRoute: { mode: 0 | 1 | 2 } = { mode: 0 };

/** @internal */
export function setEngineMode(mode: 'table' | 'force' | 'strict'): void {
  engineRoute.mode = mode === 'strict' ? 2 : mode === 'force' ? 1 : 0;
}

/** @internal */
export function fallThroughCount(): number {
  return fallThroughs;
}

/** @internal */
export function resetFallThroughs(): void {
  fallThroughs = 0;
  lastReason = '';
  lastThrown = undefined;
}

/**
 * @internal The batch calls the module counted, over every instance dropped
 * after its call and the live one: a call ran in WASM only if this moved by one.
 */
export function moduleCalls(): number {
  return live === null ? retired : retired + (live.x.batch_calls() >>> 0);
}

/** @internal The reason the latest fall-through gave; `''` when none since the last reset. */
export function lastFallThrough(): string {
  return lastReason;
}

/** @internal What the latest call that threw inside the module threw. */
export function lastTrapError(): unknown {
  return lastThrown;
}

/** @internal */
export function provideWasmBytes(base64: string): void {
  source = base64;
}

/** @internal Replaces how `ready()` imports the bytes module; returns the previous importer. */
export function setBytesImport(next: BytesImport): BytesImport {
  const previous = importBytes;
  importBytes = next;
  return previous;
}

/** @internal Forgets the bytes, the module, the instance and every failure. */
export function resetEngine(): void {
  source = null;
  compiled = null;
  live = null;
  failure = null;
  traps = 0;
  loading = null;
  verdict = undefined;
  generation++;
}

/** @internal Sets the linear-memory size past which an instance is dropped after its call; returns the previous one. */
export function setHighWater(bytes: number): number {
  const previous = highWater;
  highWater = bytes;
  return previous;
}

/** @internal Sets the most bytes a call may place in linear memory; returns the previous limit. */
export function setBudget(bytes: number): number {
  const previous = budget;
  budget = bytes;
  return previous;
}

/** @internal */
export function wasmMemoryBytes(): number {
  return live === null ? 0 : live.memoryBytes();
}

/** @internal */
export function engineStatus(): EngineStatus {
  if (failure !== null) {
    return { engine: 'js', reason: failure };
  }
  if (live !== null || compiled !== null) {
    return { engine: 'wasm', reason: null };
  }
  return { engine: 'js', reason: 'not-loaded' };
}

/**
 * Loads the WebAssembly engine and resolves to its status at that time. No
 * call needs it: under the `node`, `bun` and `deno` export conditions the
 * engine loads on the first call that uses it, and awaiting `ready()` first
 * moves that one-time compile out of that call; under any other condition, a
 * browser bundle included, every call runs in TypeScript until `ready()` has
 * loaded it. Never rejects, and the values are the same either way. After it
 * resolves to `'unavailable'` on an entry that has an engine, a later `ready()`
 * tries again. `ta-lib/js` and `ta-lib/js/abstract` have no engine and no
 * `ready`.
 */
export function ready(): Promise<EngineStatus> {
  if (failure === 'unavailable') {
    failure = null;
    loading = null;
  }
  loading ??= load();
  return loading.then((settled) => settled ?? engineStatus(), engineStatus);
}

async function load(): Promise<EngineStatus | null> {
  if (live !== null || compiled !== null || failure !== null) {
    return null;
  }
  const blocked = preflight();
  if (blocked !== null) {
    failure = blocked;
    return null;
  }
  const epoch = generation;
  let base64 = source;
  if (base64 === null) {
    try {
      base64 = (await importBytes()).WASM_BASE64;
    } catch {
      if (epoch !== generation || failure !== null) {
        return null;
      }
      failure = 'unavailable';
      // Settled here: the next ready() clears this failure, possibly before
      // the callbacks of the calls that shared this load have run.
      return engineStatus();
    }
    if (epoch !== generation) {
      return null;
    }
  }
  let module: WasmModule;
  let instance: WasmInstance;
  try {
    module = await WebAssembly.compile(decode(base64));
  } catch {
    if (epoch === generation) {
      failure ??= 'compile';
    }
    return null;
  }
  try {
    instance = await WebAssembly.instantiate(module, {});
  } catch {
    if (epoch === generation) {
      failure ??= 'instantiate';
    }
    return null;
  }
  if (epoch === generation && live === null && compiled === null && failure === null) {
    compiled = module;
    adopt(instance);
  }
  return null;
}

/**
 * Why the engine cannot run here, judged once on {@link PREFLIGHT}, so that a
 * runtime that would refuse the engine imports and compiles none of its bytes.
 */
function preflight(): 'compile' | 'instantiate' | 'fma' | null {
  if (verdict === undefined) {
    let module: WasmModule;
    try {
      module = new WebAssembly.Module(decode(PREFLIGHT));
    } catch {
      verdict = 'compile';
      return verdict;
    }
    try {
      const madd = new WebAssembly.Instance(module, {}).exports.p as WasmBatch;
      verdict = madd(PROBE[0], PROBE[0], PROBE[1]) === FUSED ? null : 'fma';
    } catch {
      verdict = 'instantiate';
    }
  }
  return verdict;
}

function decode(base64: string): Uint8Array {
  const text = atob(base64);
  const bytes = new Uint8Array(text.length);
  for (let i = 0; i < text.length; i++) {
    bytes[i] = text.charCodeAt(i);
  }
  return bytes;
}

function current(compile: boolean): WasmEngine | null {
  if (live !== null) {
    return live;
  }
  if (failure !== null) {
    return null;
  }
  if (compiled === null) {
    if (source === null || !compile) {
      return null;
    }
    const blocked = preflight();
    if (blocked !== null) {
      failure = blocked;
      return null;
    }
    try {
      compiled = new WebAssembly.Module(decode(source));
    } catch {
      failure = 'compile';
      return null;
    }
  }
  let instance: WasmInstance;
  try {
    instance = new WebAssembly.Instance(compiled, {});
  } catch {
    failure = 'instantiate';
    return null;
  }
  return adopt(instance);
}

function adopt(instance: WasmInstance): WasmEngine | null {
  const x = instance.exports;
  const memory = x.memory;
  if (
    !(memory instanceof WebAssembly.Memory) ||
    typeof x.scratch !== 'function' ||
    typeof x.out_range !== 'function' ||
    typeof x.batch_calls !== 'function' ||
    typeof x.fma_probe !== 'function'
  ) {
    failure = 'instantiate';
    return null;
  }
  const exports = x as { readonly [name: string]: WasmBatch };
  if (exports.fma_probe(PROBE[0], PROBE[0], PROBE[1]) !== FUSED) {
    failure = 'fma';
    return null;
  }
  live = new WasmEngine(exports, memory, exports.out_range() >>> 0);
  return live;
}

/** @internal */
export function fellThrough(reason: string): null {
  fallThroughs++;
  lastReason = reason;
  return null;
}

/** @internal */
export class StrictFallThrough extends Error {
  readonly reason: string;

  constructor(funcName: string, reason: string) {
    super(`${funcName}: a call ta-lib/js accepts did not run in WASM (${reason})`);
    this.name = 'StrictFallThrough';
    this.reason = reason;
  }
}

/**
 * @internal The caller evaluates `accepted`, the `ta-lib/js` answer, before
 * this runs, so a call `ta-lib/js` rejects still throws its own error.
 */
export function strictFallThrough(funcName: string, _accepted: unknown): never {
  throw new StrictFallThrough(funcName, lastReason);
}

/** @internal */
export function usesDefaultCore(core: Core | undefined): boolean {
  return core === undefined || core === Core.DEFAULT;
}

/** @internal */
export function optionalInt32(v: unknown): v is number | undefined {
  return v === undefined || (typeof v === 'number' && (v | 0) === v);
}

/** @internal */
export function optionalReal(v: unknown): v is number | undefined {
  return v === undefined || typeof v === 'number';
}

/** @internal */
export function isInt32(v: unknown): v is number {
  return typeof v === 'number' && (v | 0) === v;
}

/** @internal */
export function isReal(v: unknown): v is number {
  return typeof v === 'number';
}

/**
 * @internal What a call with caller output buffers may write: the count
 * `ta-lib/js` requires each buffer to hold, given the lookback it computes for
 * the call's parameters; -1 when that lookback rejects them, -2 when a buffer
 * might not be accepted, or written, by `ta-lib/js` exactly as here. A buffer
 * must be an instance of its `types` class, non-empty and at least that long,
 * and share no byte with another buffer or a typed-array input, not even
 * exactly, which `ta-lib/js` computes in place with bodies that may use that
 * output as scratch. An `undefined` buffer is one the call allocates.
 */
export function outputRoom(
  first: number,
  last: number,
  lookback: number,
  inputs: readonly unknown[],
  outputs: readonly (Float64Array | Int32Array | undefined)[],
  types: readonly (typeof Float64Array | typeof Int32Array)[],
): number {
  if (lookback < 0) {
    return -1;
  }
  const start = first > lookback ? first : lookback;
  const want = start > last ? 0 : last - start + 1;
  for (let k = 0; k < outputs.length; k++) {
    const out = outputs[k];
    if (out === undefined) {
      continue;
    }
    if (!(out instanceof types[k]) || out.length === 0 || out.length < want) {
      return -2;
    }
    for (const input of inputs) {
      if (ArrayBuffer.isView(input) && overlaps(out, input)) {
        return -2;
      }
    }
    for (let j = 0; j < k; j++) {
      const other = outputs[j];
      if (other !== undefined && overlaps(out, other)) {
        return -2;
      }
    }
  }
  return want;
}

/**
 * @internal The `length` of an object when it is an integer in
 * 0..INDEX_MAX + 1, else -1. It may be a getter: this can run caller code.
 */
export function seriesLength(x: unknown): number {
  if (typeof x !== 'object' || x === null) {
    return -1;
  }
  const n: unknown = (x as { length?: unknown }).length;
  if (typeof n !== 'number' || (n | 0) !== n || n < 0 || n > Core.INDEX_MAX + 1) {
    return -1;
  }
  return n;
}

/** @internal `x` as `ta-lib/js` converts it; `null` when that conversion throws. */
export function routedSeries(x: unknown): Float64Array | null {
  try {
    return float64(x as ArrayLike<number>);
  } catch {
    return null;
  }
}

const TYPED_ARRAY = Object.getPrototypeOf(Float64Array.prototype) as object;
function slotOf<T>(key: 'buffer' | 'byteOffset' | 'length'): (this: unknown) => T {
  return Object.getOwnPropertyDescriptor(TYPED_ARRAY, key)?.get as (this: unknown) => T;
}
const bufferOf = slotOf<ArrayBufferLike>('buffer');
const byteOffsetOf = slotOf<number>('byteOffset');
const lengthOf = slotOf<number>('length');

/**
 * @internal Bars `lo` to `lo + len - 1` of `x`, without a copy, located through
 * the built-in accessors: `x.subarray`, `x.buffer`, `x.byteOffset` and
 * `x.length` may run caller code, or name other bars than the ones `ta-lib/js`
 * indexes. Throws, so the call falls through, when `x` holds fewer bars.
 */
export function seriesBars(x: Float64Array, lo: number, len: number): Float64Array {
  const length = lengthOf.call(x);
  if (lo + len > length) {
    throw new RangeError('the series holds fewer bars than its length says');
  }
  return lo === 0 && len === length ? x : new Float64Array(bufferOf.call(x), byteOffsetOf.call(x) + lo * 8, len);
}

/**
 * @internal True, counted as a fall-through, when no engine can run the call:
 * a front asks before it converts or allocates anything. Compiles nothing, so
 * a call a later check sends to `ta-lib/js` leaves the engine unloaded.
 */
export function engineDown(): boolean {
  if (failure === null && (live !== null || compiled !== null || source !== null)) {
    return false;
  }
  fellThrough(failure ?? 'not-loaded');
  return true;
}

/**
 * @internal False, counted as a fall-through, when a call of `bytes` is over
 * the budget or the engine cannot load, which may compile it now. A front
 * asks last before its first conversion, so a call the engine cannot run
 * runs no conversion twice.
 */
export function engineFor(bytes: number): boolean {
  if (bytes > budget) {
    fellThrough('budget');
    return false;
  }
  if (current(true) !== null) {
    return true;
  }
  fellThrough(failure ?? 'not-loaded');
  return false;
}

/**
 * @internal Scratch for one call's `bytes`, with the views derived after it,
 * since taking it may grow memory. `null` once the call fell through. Asked
 * after {@link engineFor}; a call made by a conversion in between may have
 * dropped the instance, so this may instantiate one, but never compiles.
 */
export function wasmFrame(bytes: number): WasmEngine | null {
  const engine = current(false);
  if (engine === null) {
    return fellThrough(failure ?? 'not-loaded');
  }
  try {
    engine.base = engine.x.scratch(bytes) >>> 0;
  } catch (e) {
    return trapped(engine, e);
  }
  engine.refresh();
  return engine;
}

/**
 * @internal A call into the module that threw: the instance is dropped, and
 * after MAX_TRAPS the engine. The reason is `'trap'` only for a WebAssembly
 * trap; anything else thrown there, such as the `TypeError` of a missing
 * export, is `'threw'`.
 */
export function trapped(engine: WasmEngine, error: unknown): null {
  if (live === engine) {
    retire(engine);
  }
  lastThrown = error;
  traps++;
  if (traps >= MAX_TRAPS) {
    failure = 'trap';
  }
  return fellThrough(error instanceof WebAssembly.RuntimeError ? 'trap' : 'threw');
}

/** @internal `'frame'` is a defect in the glue, never a call to answer from TypeScript by design. */
export function refused(code: number): null {
  return fellThrough(code === DECLINED ? 'declined' : code === OUT_OF_FRAME ? 'frame' : 'rejected');
}
