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
 *  080726 MF,CC  First Version — the package page for the published javadoc.
 */

/**
 * TA-Lib's technical analysis indicators: moving averages, oscillators,
 * volatility and volume studies, and the candlestick patterns.
 *
 * <p>Every indicator is a method on {@link io.github.talib.Core}. The batch
 * form computes a range in one call and reports what it wrote as an
 * {@link io.github.talib.OutRange}:
 *
 * <pre>{@code
 * double[] out = new double[close.length];
 * OutRange r = Core.DEFAULT.SMA(0, close.length - 1, close, 30, out);
 * // out[0 .. r.count() - 1] are the values; r.begIdx() is where they start
 * }</pre>
 *
 * <p>Output is written from index {@code 0}, not from {@code begIdx}: an
 * indicator needs {@code <NAME>_Lookback(params)} bars before it can produce
 * anything, so {@code begIdx} says where in the <i>input</i> the first value
 * belongs. A range shorter than the lookback is a success with a count of
 * zero, never an exception.
 *
 * <p>The same indicators are also available as streams — {@code <name>Open}
 * returns a handle whose {@code update} carries one new bar for a fraction of
 * the cost of recomputing, and is bit-identical to the batch call at that bar.
 *
 * <h2>Configuration</h2>
 *
 * {@link io.github.talib.Core#DEFAULT} is the all-defaults instance and is the
 * right choice unless unstable periods or candle settings need tuning. A
 * {@code Core} is deeply immutable and safe to share across threads with no
 * synchronization; derive a configured one through
 * {@link io.github.talib.CoreBuilder}:
 *
 * <pre>{@code
 * Core core = Core.builder()
 *     .unstablePeriod(FuncUnstId.EMA, 10)
 *     .build();
 * }</pre>
 *
 * <h2>Errors</h2>
 *
 * Parameters outside their documented range throw
 * {@link java.lang.IllegalArgumentException}; indices outside the input throw
 * {@link java.lang.IndexOutOfBoundsException}; a stream opened on less history
 * than its lookback throws {@link io.github.talib.InsufficientHistoryException},
 * which extends {@code IllegalArgumentException} so it can be caught to
 * accumulate more bars and retry.
 *
 * @see io.github.talib.metadata
 * @see <a href="https://ta-lib.org">ta-lib.org</a>
 */
package io.github.talib;
