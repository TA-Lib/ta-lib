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
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  121005 MF     First Version
 */

package io.github.talib;

/**
 * The condition a call reported, mirroring C's {@code TA_RetCode}.
 *
 * <p>The public API raises rather than returning one, so this is reached
 * through a caught failure: every exception the library throws implements
 * {@link TaLibFailure}, whose {@link TaLibFailure#retCode()} answers the member
 * below. The numbers are C's, and are what the cross-language harness compares —
 * Java's ordinals are not, which is why {@link #asCInt()} exists.
 */
public enum RetCode
{
    /** The call completed; the reported range says how much was written. */
    Success(0),
    /** The catch-all rejection ({@code TA_BAD_PARAM}). */
    BadParam(2),
    /** C parity only, never returned here: an allocation failure terminates the process (#178). */
    AllocErr(3),
    /** {@code startIdx} outside the addressable index domain. */
    OutOfRangeStartIndex(12),
    /** {@code endIdx} outside the domain, or below {@code startIdx}. */
    OutOfRangeEndIndex(13),
    /**
     * A stream opener was given fewer than {@code lookback + 1} bars — the
     * library's one recoverable condition. Streaming only: a batch range
     * shorter than the lookback is {@link #Success} with a zero count.
     */
    InsufficientHistory(17),
    /** An invariant the library owns was violated. */
    InternalError(5000);

    private final int cValue;

    RetCode(int cValue) {
        this.cValue = cValue;
    }

    /**
     * This code's {@code TA_RetCode} integer — the value C returns for the same
     * condition ({@code include/ta_defs.h}).
     *
     * <p>Carried on the member rather than derived by a {@code switch}, so a new
     * member cannot reach the wire as whatever a {@code default:} arm said.
     */
    public int asCInt() {
        return cValue;
    }
};
