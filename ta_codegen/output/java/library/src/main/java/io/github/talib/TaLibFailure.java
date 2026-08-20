/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * This file is part of the TA-LIB project.
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
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF,CC    Mario Fortier, Claude (Anthropic AI)
 */
package io.github.talib;

/**
 * Implemented by every exception this library raises, so the condition that was
 * reported can be recovered as the {@link RetCode} C would have returned for
 * the same call.
 *
 * <p>The exception <i>types</i> are the ones the API documents — a rejected
 * index is still an {@link IndexOutOfBoundsException}, a bad parameter still an
 * {@link IllegalArgumentException} — because that is what a caller catches.
 * What the types cannot carry is <i>which</i> condition: one
 * {@code IndexOutOfBoundsException} serves both {@link RetCode#OutOfRangeStartIndex}
 * and {@link RetCode#OutOfRangeEndIndex}, and one {@link IllegalStateException}
 * serves both {@link RetCode#AllocErr} and {@link RetCode#InternalError}. This
 * interface is what makes the two separable again, without narrowing the catch
 * types.
 *
 * <p>The mapping is <b>total</b> over the batch and streaming tiers — every
 * failure a call to an indicator raises implements it, including the length and
 * presence checks C cannot make (they report {@link RetCode#BadParam}, the code
 * C uses for an argument it can detect) — and <b>lossless</b>: distinct codes
 * never share one thrown representation.
 *
 * <p>Outside it, deliberately: {@link CoreBuilder} and the
 * {@code io.github.talib.metadata} binder still raise plain JDK types. Neither
 * is an indicator call, so neither has a {@link RetCode} to carry.
 */
public interface TaLibFailure {

   /** The condition reported, as the code C would have returned. */
   RetCode retCode();
}
