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


import { float64, isOptionsObject } from './checks.ts';
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
const NO_OPTIONS = Object.freeze({});
/** A typed array's own element count, which an overridden `length` property cannot change. */
const typedLength = Object.getOwnPropertyDescriptor(Object.getPrototypeOf(Uint8Array.prototype), 'length')!.get as (
  this: unknown,
) => number;

/**
 * Why the default entry runs its batch calls in TypeScript:
 * - `'not-loaded'`: the engine has not been loaded yet. In a browser it loads
 *   only when {@link ready} is called; elsewhere, on the first call that needs it.
 * - `'unavailable'`: the module holding the engine's bytes could not be imported.
 * - `'compile'`: the runtime refused to compile the engine, as one without
 *   relaxed SIMD or under a Content-Security-Policy that forbids WebAssembly does.
 * - `'instantiate'`: the engine compiled but could not be instantiated.
 * - `'fma'`: this runtime does not fuse the multiply-add the engine relies on, so
 *   its values would differ from the TypeScript engine's.
 * - `'trap'`: the engine failed during calls too many times and was given up.
 */
export type EngineReason = 'not-loaded' | 'unavailable' | 'compile' | 'instantiate' | 'fma' | 'trap';

/** Which engine the default entry's batch calls run in; what {@link ready} resolves to. */
export interface EngineStatus {
  /** `'wasm'` for the WebAssembly engine, `'js'` for TypeScript. Both give the same values. */
  readonly engine: 'wasm' | 'js';
  /** Why `engine` is `'js'`; `null` when it is `'wasm'`. */
  readonly reason: EngineReason | null;
}

let source: string | null = null;
let compiled: WasmModule | null = null;
let live: WasmEngine | null = null;
let failure: EngineReason | null = null;
let traps = 0;
let loading: Promise<EngineStatus> | null = null;
let generation = 0;
let fallThroughs = 0;
let lastReason = '';
let lastThrown: unknown = undefined;
let highWater = HIGH_WATER;
let budget = BUDGET;
let retired = 0;

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

/** @internal Forgets the bytes, the module, the instance and every failure. */
export function resetEngine(): void {
  source = null;
  compiled = null;
  live = null;
  failure = null;
  traps = 0;
  loading = null;
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
 * Loads the WebAssembly engine, and resolves to the engine the batch calls of
 * this entry run in from then on. No call needs it: in Node, Bun and Deno the
 * engine loads on the first call that uses it, and in a browser every call
 * runs in TypeScript until `ready()` has loaded it. Never rejects, and the
 * values are the same either way. `ta-lib/js` has no engine and no `ready`.
 */
export function ready(): Promise<EngineStatus> {
  if (loading === null) {
    loading = load().then(engineStatus, engineStatus);
  }
  return loading;
}

async function load(): Promise<void> {
  if (live !== null || compiled !== null || failure !== null) {
    return;
  }
  const epoch = generation;
  let base64 = source;
  if (base64 === null) {
    try {
      base64 = (await import('./wasmBytes.js')).WASM_BASE64;
    } catch {
      if (epoch === generation) {
        failure ??= 'unavailable';
      }
      return;
    }
    if (epoch !== generation) {
      return;
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
    return;
  }
  try {
    instance = await WebAssembly.instantiate(module, {});
  } catch {
    if (epoch === generation) {
      failure ??= 'instantiate';
    }
    return;
  }
  if (epoch === generation && live === null && compiled === null && failure === null) {
    compiled = module;
    adopt(instance);
  }
}

function decode(base64: string): Uint8Array {
  const text = atob(base64);
  const bytes = new Uint8Array(text.length);
  for (let i = 0; i < text.length; i++) {
    bytes[i] = text.charCodeAt(i);
  }
  return bytes;
}

function current(): WasmEngine | null {
  if (live !== null) {
    return live;
  }
  if (failure !== null) {
    return null;
  }
  if (compiled === null) {
    if (source === null) {
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

/** @internal An omitted options object, or one `ta-lib/js` accepts; `null` for anything else. */
export function routedOptions<T extends object>(options: T | undefined): T | null {
  if (options === undefined) {
    return NO_OPTIONS as T;
  }
  return isOptionsObject(options) ? options : null;
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

/** @internal The length of an object whose `length` is an integer in 0..INDEX_MAX + 1, else -1. */
export function seriesLength(x: unknown): number {
  if (typeof x !== 'object' || x === null) {
    return -1;
  }
  const n: unknown = (x as { length?: unknown }).length;
  if (typeof n !== 'number' || (n | 0) !== n || n < 0 || n > Core.INDEX_MAX + 1) {
    return -1;
  }
  return ArrayBuffer.isView(x) && ownLength(x) !== n ? -1 : n;
}

function ownLength(x: object): number {
  try {
    return typedLength.call(x);
  } catch {
    return -1;
  }
}

/**
 * @internal `x` converted as the `ta-lib/js` tier converts it, when that gives
 * a real `Float64Array` of `n` elements; `null` otherwise. A `Proxy` passes
 * `instanceof Float64Array` and is passed through unconverted, so it is refused
 * here: copying it into linear memory would run its traps after the views
 * were taken.
 */
export function routedSeries(x: unknown, n: number): Float64Array | null {
  if (seriesLength(x) !== n) {
    return null;
  }
  let a: Float64Array;
  try {
    a = float64(x as ArrayLike<number>);
  } catch {
    return null;
  }
  return ArrayBuffer.isView(a) && ownLength(a) === n ? a : null;
}

/**
 * @internal Scratch for one call's `bytes`, with the views derived after it,
 * since taking it may grow memory. `null` once the call fell through.
 */
export function wasmFrame(bytes: number): WasmEngine | null {
  if (bytes > budget) {
    return fellThrough('budget');
  }
  const engine = current();
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
