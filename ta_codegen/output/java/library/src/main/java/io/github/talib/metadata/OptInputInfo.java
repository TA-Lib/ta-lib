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

/* GENERATED FILE — do not edit. Produced by ta_codegen
 * (generator/src/backends/java_metadata.rs) from ta_codegen/input/.
 * MF,CC
 */

package io.github.talib.metadata;

/**
 * One optional parameter of an indicator, with its domain.
 *
 * <p>Which fields carry meaning depends on {@link #type()}:
 * {@link OptInputType#REAL_RANGE} uses {@code min}/{@code max}/{@code precision}
 * and the {@code suggested*} triple; {@link OptInputType#INTEGER_RANGE} uses
 * {@code intMin}/{@code intMax} and {@code intSuggested*};
 * {@link OptInputType#INTEGER_LIST} uses {@link #valueList()}.
 *
 * @param type         the parameter's domain
 * @param paramName    the parameter's name, e.g. {@code optInTimePeriod}
 * @param flags        display hints (see {@link OptInputFlags})
 * @param displayName  a human-readable label, e.g. {@code Time Period}
 * @param hint         per-parameter help text ({@code ""} when none)
 * @param defaultValue the value {@code Integer.MIN_VALUE} (or the real-default
 *                     sentinel) selects
 */
public record OptInputInfo(
       OptInputType type,
       String paramName,
       int flags,
       String displayName,
       String hint,
       double defaultValue,
       double min,
       double max,
       int precision,
       double suggestedStart,
       double suggestedEnd,
       double suggestedIncrement,
       int intMin,
       int intMax,
       int intSuggestedStart,
       int intSuggestedEnd,
       int intSuggestedIncrement,
       String valueList) {
}
