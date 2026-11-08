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
 *  110126 MF,CC  First Version
 */

import { Core } from './core.ts';
import { type FailureRetCode, RetCode, TALibError } from './errors.ts';

/*
 * The argument contract shared by every public entry point; `funcName` is the
 * registry spelling (`SMA`, `HT_TRENDLINE`). Each check fixes the ORDER of the
 * diagnosis, so a caller is told about the first broken rule, not a later one.
 */

/** @internal */
export function failure(funcName: string, retCode: FailureRetCode): TALibError {
  const where = funcName + ': ';
  switch (retCode) {
    case RetCode.OUT_OF_RANGE_START_INDEX:
      return new TALibError(retCode, where + 'startIdx out of range');
    case RetCode.OUT_OF_RANGE_END_INDEX:
      return new TALibError(retCode, where + 'endIdx out of range');
    case RetCode.BAD_PARAM:
      return new TALibError(retCode, where + 'bad parameter');
    case RetCode.ALLOC_ERR:
      return new TALibError(retCode, where + 'allocation failed');
    case RetCode.INTERNAL_ERROR:
      return new TALibError(retCode, where + 'internal error');
    case RetCode.INSUFFICIENT_HISTORY:
      return new TALibError(retCode, where + 'history shorter than lookback + 1');
    default:
      return new TALibError(retCode, where + 'TA_RetCode ' + retCode);
  }
}

/** @internal The opener's tail; `funcName` carries the verb (`SMA open`). */
export function streamFailure(funcName: string, retCode: FailureRetCode): TALibError {
  const where = funcName + ': ';
  switch (retCode) {
    case RetCode.BAD_PARAM:
      return new TALibError(retCode, where + 'bad parameter');
    case RetCode.INSUFFICIENT_HISTORY:
      return new TALibError(retCode, where + 'history shorter than lookback + 1');
    case RetCode.OUT_OF_RANGE_END_INDEX:
      return new TALibError(retCode, where + 'outRange() has reached Core.INDEX_MAX');
    case RetCode.INTERNAL_ERROR:
      return new TALibError(retCode, where + 'internal error');
    default:
      return new TALibError(retCode, where + 'TA_RetCode ' + retCode);
  }
}

/**
 * @internal An integer argument must be an int32 number. `(v | 0) !== v`
 * rejects NaN and fractions and keeps the `INTEGER_DEFAULT` sentinel; the
 * `typeof` test ahead of it rejects strings and BigInts, which `|` would coerce
 * or throw on. Run it before every range check, and use the value it returns:
 * a -0 passes the test and must not reach a body, where it would carry its
 * sign into real arithmetic.
 */
export function requireInt32(funcName: string, argName: string, v: number): number {
  if (typeof v !== 'number' || (v | 0) !== v) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is not an int32, got ${show(v)}`);
  }
  return v | 0;
}

/** @internal A rejected value as a message shows it: a string quoted and a BigInt with its `n`, so neither reads as the number it is not. */
export function show(v: unknown): string {
  return typeof v === 'string' ? JSON.stringify(v) : typeof v === 'bigint' ? `${v}n` : String(v);
}

/**
 * @internal A real argument must be a number. Without this a string passes the
 * range checks by coercion and then concatenates in the body.
 */
export function requireReal(funcName: string, argName: string, v: number): number {
  if (typeof v !== 'number') {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is not a number, got ${show(v)}`);
  }
  return v;
}

/**
 * @internal A batch input as the body reads it. A `Float64Array` is read in
 * place, so an input that is also an output stays one; any other form is
 * copied once, which applies ToNumber to every element.
 */
export function float64(array: ArrayLike<number>): Float64Array {
  return array instanceof Float64Array ? array : Float64Array.from(array);
}

/** @internal */
export function requireIndexRange(funcName: string, startIdx: number, endIdx: number): void {
  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {
    throw new TALibError(
      RetCode.OUT_OF_RANGE_START_INDEX,
      `${funcName}: startIdx must be in 0..${Core.INDEX_MAX}, got ${startIdx}`,
    );
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    throw new TALibError(
      RetCode.OUT_OF_RANGE_END_INDEX,
      `${funcName}: endIdx must be in ${startIdx}..${Core.INDEX_MAX}, got ${endIdx}`,
    );
  }
}

/**
 * @internal `max(startIdx, lookback)`. A negative lookback is the parameter
 * rejection `<n>Lookback` signals. A result above `endIdx` is not an error: the
 * call produces nothing, which switches off the output bound but not the input
 * bound.
 */
export function clampedStart(funcName: string, startIdx: number, lookback: number): number {
  if (lookback < 0) {
    throw failure(funcName, RetCode.BAD_PARAM);
  }
  return startIdx > lookback ? startIdx : lookback;
}

/**
 * @internal A series must be array-like: an object with a numeric `length`.
 * Only a JavaScript caller can pass anything else, and a number, a string or a
 * plain object would otherwise be read as an empty or all-NaN series and the
 * call succeed.
 */
export function requireArrayLike(funcName: string, argName: string, array: unknown): asserts array is ArrayLike<number> {
  if (array == null) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is absent`);
  }
  if (typeof array !== 'object' || !isLength((array as ArrayLike<number>).length)) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is not an array`);
  }
}

/** @internal An array's length: a non-negative integer. NaN or a fraction would pass every later bound. */
function isLength(v: unknown): v is number {
  return typeof v === 'number' && Number.isInteger(v) && v >= 0;
}

/**
 * @internal An absent, non-array, empty or too-short series. `required` is 0 on
 * a range that produces nothing, which leaves only the other tests.
 */
export function requireLength(
  funcName: string,
  argName: string,
  array: ArrayLike<number> | null | undefined,
  required: number,
): void {
  requireArrayLike(funcName, argName, array);
  const actual = array.length;
  if (actual < required) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} has length ${actual}, needs ${required}`);
  }
  if (actual === 0) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is empty`);
  }
}

/**
 * @internal An output of another element type would store every value rounded,
 * truncated or wrapped, with no error.
 */
export function requireOutput(
  funcName: string,
  argName: string,
  array: Float64Array | Int32Array | null | undefined,
  required: number,
  type: typeof Float64Array | typeof Int32Array,
): void {
  requireLength(funcName, argName, array, required);
  if (!(array instanceof type)) {
    const article = type === Int32Array ? 'an' : 'a';
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is not ${article} ${type.name}`);
  }
}

/**
 * @internal An absent or primitive argument, checked before anything reads a
 * property of it: a series or a price object.
 */
export function requireArgument(funcName: string, argName: string, array: unknown): void {
  if (array == null) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is absent`);
  }
  if (typeof array !== 'object') {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is a ${typeof array}, not an array or object`);
  }
}

/** @internal A batch input: absent, not an array or empty is `TA_BAD_PARAM` (rB4). */
export function requireSeries(funcName: string, argName: string, array: ArrayLike<number> | null | undefined): void {
  requireArrayLike(funcName, argName, array);
  if (array.length === 0) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is empty`);
  }
}

/**
 * @internal An opener's history holds at most `INDEX_MAX + 1` bars (rS2). An
 * empty one passes: it is a short history (rS7), which the core answers after
 * every argument check.
 */
export function requireHistory(funcName: string, argName: string, historyLen: number): void {
  if (!isLength(historyLen)) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is not an array`);
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    throw new TALibError(
      RetCode.OUT_OF_RANGE_END_INDEX,
      `${funcName}: history has length ${historyLen}, at most ${Core.INDEX_MAX + 1}`,
    );
  }
}

/**
 * @internal Two typed arrays share memory: the same buffer and intersecting
 * byte ranges, whatever their element types. `===` misses two views of one
 * buffer.
 */
export function overlaps(a: ArrayBufferView, b: ArrayBufferView): boolean {
  return (
    a.buffer === b.buffer && a.byteOffset < b.byteOffset + b.byteLength && b.byteOffset < a.byteOffset + a.byteLength
  );
}

/**
 * @internal An output over an input other than the input itself: the same
 * buffer, start and length is computing in place, which every body supports.
 */
export function overlapsPartly(out: Float64Array, input: Float64Array): boolean {
  return overlaps(out, input) && (out.byteOffset !== input.byteOffset || out.length !== input.length);
}

/** @internal Two arguments the call refused to let share memory. */
export function sharedMemory(funcName: string, a: string, b: string): TALibError {
  return new TALibError(RetCode.BAD_PARAM, `${funcName}: ${a} and ${b} share memory`);
}

/**
 * @internal One parameter as its range check sees it: the options key, the
 * default sentinel, the inclusive bounds and the accepted values as a message
 * states them, empty for a parameter no range check covers.
 */
export type ParamRange = readonly [key: string, sentinel: number, min: number, max: number, accepted: string];

/**
 * @internal The rejection of parameters a lookback answered -1 for. `ranges`
 * must list them in the order the checks run, which every range check precedes:
 * the first value outside its range is then the reason, and with none outside
 * only the combination can be, so no range is claimed.
 */
export function rejectedParams(funcName: string, ranges: readonly ParamRange[], ...values: number[]): TALibError {
  for (let k = 0; k < ranges.length; k++) {
    const [key, sentinel, min, max, accepted] = ranges[k];
    const v = values[k];
    if (accepted !== '' && v !== sentinel && !(v >= min && v <= max)) {
      return new TALibError(RetCode.BAD_PARAM, `${funcName}: ${key} must be ${accepted}, got ${show(v)}`);
    }
  }
  const keys = ranges.map((r) => r[0]);
  const verdict = keys.length === 1 ? 'is in range but rejected' : 'are each in range but rejected';
  return new TALibError(RetCode.BAD_PARAM, `${funcName}: ${keys.join(', ')} ${verdict}; see the function's documentation`);
}

/** @internal Every input of an opener is exactly the history's length. */
export function requireHistoryLength(funcName: string, argName: string, actual: number, historyLen: number): void {
  if (!isLength(actual)) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is not an array`);
  }
  if (actual !== historyLen) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} has length ${actual}, needs ${historyLen}`);
  }
}

/**
 * @internal How many values an `OpenAndFill` writes. A short history floors to
 * 0 because its rejection is the later history check's to make; a negative
 * lookback is the parameter rejection and must not be floored.
 */
export function openFillCount(funcName: string, historyLen: number, lookback: number): number {
  if (lookback < 0) {
    throw failure(funcName, RetCode.BAD_PARAM);
  }
  return historyLen <= lookback ? 0 : historyLen - lookback;
}

/** @internal */
export function insufficientHistory(
  funcName: string,
  historyLen: number,
  startIdx: number,
  lookback: number,
): TALibError {
  return new TALibError(
    RetCode.INSUFFICIENT_HISTORY,
    `${funcName}: history has length ${historyLen}, needs ${(startIdx > lookback ? startIdx : lookback) + 1}`,
  );
}

/** @internal */
export function nonFiniteBar(funcName: string, argName: string): TALibError {
  return new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is not finite`);
}

/** @internal A multi-output sink given but not an object. */
export function badSink(funcName: string, className: string): TALibError {
  return new TALibError(RetCode.BAD_PARAM, `${funcName}: out must be a ${className}`);
}

const NO_OPTIONS = Object.freeze({});

/**
 * @internal The options object of a public call: an omitted one has every
 * default. `null`, a primitive or an array is a caller error, not a request for
 * defaults: a second series passed where the options go would otherwise run
 * the call with every default and no error.
 */
export function optionsOf<T extends object>(funcName: string, options: T | undefined): T {
  if (options === undefined) {
    return NO_OPTIONS as T;
  }
  if (typeof options !== 'object' || options === null || Array.isArray(options) || ArrayBuffer.isView(options)) {
    const got = Array.isArray(options) ? 'an array' : ArrayBuffer.isView(options) ? 'a typed array or DataView' : show(options);
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: options must be a plain object, got ${got}`);
  }
  return options;
}

/**
 * @internal `undefined` only selects `fallback`: a `null` reaches the
 * positional tier's own check and is rejected there.
 */
export function orDefault<T>(value: T | undefined, fallback: T): T {
  return value === undefined ? fallback : value;
}

/** @internal An omitted `core` is the default; anything but a `Core` is `TA_BAD_PARAM`. */
export function coreOf(funcName: string, core: Core | undefined): Core {
  if (core === undefined) {
    return Core.DEFAULT;
  }
  if (!(core instanceof Core)) {
    const hint = typeof core === 'object' && core !== null ? '; a Core built by another copy of ta-lib does not count' : '';
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: core is not a Core${hint}`);
  }
  return core;
}

function isIndex(v: number): boolean {
  return (v | 0) === v && v >= 0 && v <= Core.INDEX_MAX;
}

/**
 * @internal Elements to allocate for a batch output the caller did not give:
 * the count the call writes, or 1 for a call whose range or parameters the
 * positional tier will reject, so no allocation exceeds the inputs' length.
 * Never 0: an empty output is itself a rejection.
 */
export function batchCapacity(
  startIdx: number,
  endIdx: number,
  lookback: number,
  ...inputs: readonly (ArrayLike<number> | null | undefined)[]
): number {
  if (!isIndex(startIdx) || !isIndex(endIdx) || endIdx < startIdx || !isIndex(lookback)) {
    return 1;
  }
  for (const input of inputs) {
    if (input == null || !(input.length > endIdx)) {
      return 1;
    }
  }
  const first = startIdx > lookback ? startIdx : lookback;
  return first > endIdx ? 1 : endIdx - first + 1;
}

/**
 * @internal {@link batchCapacity} for an `OpenAndFill`: the history less the
 * lookback when every input is a history of one valid length, else 1.
 */
export function fillCapacity(lookback: number, ...inputs: readonly (ArrayLike<number> | null | undefined)[]): number {
  const first = inputs[0];
  if (first == null || !isIndex(lookback)) {
    return 1;
  }
  const historyLen = first.length;
  if (!((historyLen | 0) === historyLen && historyLen > lookback && historyLen <= Core.INDEX_MAX + 1)) {
    return 1;
  }
  for (const input of inputs) {
    if (input == null || input.length !== historyLen) {
      return 1;
    }
  }
  return historyLen - lookback;
}
