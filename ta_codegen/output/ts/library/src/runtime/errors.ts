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

/**
 * The condition a call reported, numbered as C's `TA_RetCode`. The public API
 * throws rather than returning one: a caught {@link TALibError} carries it.
 */
export const RetCode = Object.freeze({
  SUCCESS: 0,
  BAD_PARAM: 2,
  /** C parity only: never thrown here. */
  ALLOC_ERR: 3,
  /** Metadata API only: the function handle is not a registry row. */
  INVALID_HANDLE: 6,
  /** Metadata API only: a setter was given a value of another kind than its slot. */
  INVALID_PARAM_HOLDER_TYPE: 8,
  /** Metadata API only: an input is not bound. */
  INPUT_NOT_ALL_INITIALIZE: 10,
  /** Metadata API only: an output is not bound. */
  OUTPUT_NOT_ALL_INITIALIZE: 11,
  OUT_OF_RANGE_START_INDEX: 12,
  OUT_OF_RANGE_END_INDEX: 13,
  /**
   * A stream opener was given `lookback` bars or fewer: the one recoverable
   * condition. A batch range that ends before the lookback is not an error but
   * an empty result.
   */
  INSUFFICIENT_HISTORY: 17,
  INTERNAL_ERROR: 5000,
} as const);
export type RetCode = (typeof RetCode)[keyof typeof RetCode];

/** Every code a {@link TALibError} can carry. */
export type FailureRetCode = Exclude<RetCode, typeof RetCode.SUCCESS>;

/** The stable name of a failure, C's enumerator spelling. */
export type RetCodeName = `TA_${Exclude<keyof typeof RetCode, 'SUCCESS'>}`;

const CODE_NAME = Object.fromEntries(
  Object.entries(RetCode).map(([name, value]) => [value, `TA_${name}`]),
) as { readonly [retCode: number]: RetCodeName };

/**
 * The one error class the library throws. Match on {@link TALibError.code}:
 * it survives a second copy of the package and a second realm, where
 * `instanceof` does not.
 */
export class TALibError extends Error {
  override readonly name = 'TALibError';
  readonly code: RetCodeName;
  /** The number C, Java and C# report for the same call. */
  readonly retCode: FailureRetCode;

  constructor(retCode: FailureRetCode, message: string) {
    super(message);
    this.retCode = retCode;
    this.code = CODE_NAME[retCode];
  }
}
