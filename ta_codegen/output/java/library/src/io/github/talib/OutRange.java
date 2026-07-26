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
 *  072626 MF,CC  First Version — replaces the MInteger out-param pair.
 */

package io.github.talib;

/**
 * The range of an indicator's output: where the first computed value lands in
 * the caller's input coordinates, and how many values were written.
 *
 * <p>Every batch method returns one of these instead of filling a pair of
 * mutable out-params. The output array holds the values compactly — element
 * {@code i} of the output corresponds to input bar {@code begIdx + i}, for
 * {@code i} in {@code [0, count)}. Nothing outside that range is written, and
 * the library never pads with NaN.
 *
 * <p><b>{@code count == 0} is a success, not a failure.</b> A valid range
 * shorter than the indicator's lookback simply produces no values — exactly
 * matching the C library's {@code TA_SUCCESS} with {@code outNBElement == 0}.
 * It is never signalled by an exception.
 *
 * <p>The component names are deliberately {@code begIdx}/{@code count} rather
 * than something more Java-ish: they match the C and Rust surfaces and the
 * cross-language verification harness, so the same concept greps the same way
 * in every backend.
 *
 * @param begIdx index, in the input array's coordinates, of the first computed
 *               value; {@code 0} when {@code count} is {@code 0}
 * @param count  number of values written to the output array(s)
 */
public record OutRange(int begIdx, int count) {

    /** An empty result — a successful call that produced no values. */
    public static final OutRange EMPTY = new OutRange(0, 0);

    /** Whether this range holds no values (a successful but empty result). */
    public boolean isEmpty() {
        return count == 0;
    }

    /**
     * Index, in the input array's coordinates, one past the last computed value.
     * {@code begIdx} when the range is empty.
     */
    public int endIdx() {
        return begIdx + count;
    }
}
