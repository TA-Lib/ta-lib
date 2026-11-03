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
      return new TALibError(
        retCode,
        where + 'bad parameter (out-of-range optional parameter, or overlapping arrays)',
      );
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
    case RetCode.INTERNAL_ERROR:
      return new TALibError(retCode, where + 'internal error');
    default:
      return new TALibError(retCode, where + 'TA_RetCode ' + retCode);
  }
}

/**
 * @internal An integer argument must be an int32 number. `(v | 0) !== v` also
 * rejects NaN, fractions and numeric strings, and keeps the `INTEGER_DEFAULT`
 * sentinel. Run it before every range check, and use the value it returns: a
 * -0 passes the test and must not reach a body, where it would carry its sign
 * into real arithmetic.
 */
export function requireInt32(funcName: string, argName: string, v: number): number {
  if ((v | 0) !== v) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is not an int32, got ${String(v)}`);
  }
  return v | 0;
}

/**
 * @internal A real argument must be a number. Without this a string passes the
 * range checks by coercion and then concatenates in the body.
 */
export function requireReal(funcName: string, argName: string, v: number): number {
  if (typeof v !== 'number') {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is not a number, got ${String(v)}`);
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
    throw failure(funcName, RetCode.OUT_OF_RANGE_START_INDEX);
  }
  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {
    throw failure(funcName, RetCode.OUT_OF_RANGE_END_INDEX);
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
 * @internal An absent, empty or too-short array. `required` is 0 on a range
 * that produces nothing, which leaves only the absent and empty tests.
 */
export function requireLength(
  funcName: string,
  argName: string,
  array: ArrayLike<number> | null | undefined,
  required: number,
): void {
  if (array == null) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is absent`);
  }
  const actual = array.length;
  if (actual < required) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} has length ${actual}, needs ${required}`);
  }
  if (actual === 0) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is empty`);
  }
}

/** @internal An absent argument, checked before anything reads its length. */
export function requireArgument(funcName: string, argName: string, array: unknown): void {
  if (array == null) {
    throw new TALibError(RetCode.BAD_PARAM, `${funcName}: ${argName} is absent`);
  }
}

/**
 * @internal An opener's history must hold 1 to `INDEX_MAX + 1` bars. Not
 * {@link requireIndexRange} over `[0, historyLen - 1]`, which reports an empty
 * history as the end index rather than the start.
 */
export function requireHistory(funcName: string, historyLen: number): void {
  if (historyLen < 1) {
    throw failure(funcName, RetCode.OUT_OF_RANGE_START_INDEX);
  }
  if (historyLen > Core.INDEX_MAX + 1) {
    throw failure(funcName, RetCode.OUT_OF_RANGE_END_INDEX);
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

/** @internal Every input of an opener is exactly the history's length. */
export function requireHistoryLength(funcName: string, argName: string, actual: number, historyLen: number): void {
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

/** @internal A multi-output sink that is absent or not the function's own `<N>Out`. */
export function badSink(funcName: string, className: string): TALibError {
  return new TALibError(RetCode.BAD_PARAM, `${funcName}: out must be a ${className}`);
}
