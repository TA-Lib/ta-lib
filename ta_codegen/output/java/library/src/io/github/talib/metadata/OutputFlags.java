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
 * How an output is meant to be drawn, and whether it may be omitted. Values match C's {@code TA_OUT_*}. The old hand-written island stopped at {@code ZERO} and left consumers hardcoding the rest.
 */
public final class OutputFlags {

   private OutputFlags() { }

   /** Draw as a continuous line. */
   public static final int LINE = 0x00000001;

   /** Draw as a dotted line. */
   public static final int DOT_LINE = 0x00000002;

   /** Draw as a dashed line. */
   public static final int DASH_LINE = 0x00000004;

   /** Draw as unconnected dots. */
   public static final int DOT = 0x00000008;

   /** Draw as a histogram. */
   public static final int HISTOGRAM = 0x00000010;

   /** 0 = no pattern, 100 = pattern. */
   public static final int PATTERN_BOOL = 0x00000020;

   /** -100 = bearish, 0 = none, 100 = bullish. */
   public static final int PATTERN_BULL_BEAR = 0x00000040;

   /** -200..-100 = bearish, 100..200 = bullish. */
   public static final int PATTERN_STRENGTH = 0x00000080;

   /** Always &gt;= 0. */
   public static final int POSITIVE = 0x00000100;

   /** Always &lt;= 0. */
   public static final int NEGATIVE = 0x00000200;

   /** Zero is a meaningful reference level. */
   public static final int ZERO = 0x00000400;

   /** An upper band/limit line. */
   public static final int UPPER_LIMIT = 0x00000800;

   /** A lower band/limit line. */
   public static final int LOWER_LIMIT = 0x00001000;

   /** Discardable: C accepts NULL for it. Java still requires an array. */
   public static final int NULLABLE = 0x00002000;

}
