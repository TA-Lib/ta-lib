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
 * How an output is meant to be drawn, whether it may be omitted, and what its values can be. Values match C's {@code TA_OUT_*}. A pattern output writes 0 or a sign times a level: the sign flags give its signs, the level flags its levels (100 always).
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

   /** 0 is no pattern, 100 a pattern; no other value. */
   public static final int PATTERN_BOOL = 0x00000020;

   /** The sign is a call: positive bullish, negative bearish. */
   public static final int PATTERN_BULL_BEAR = 0x00000040;

   /** Adds level 200: this bar confirms the output's most recent earlier pattern. */
   public static final int PATTERN_CONFIRM = 0x00000080;

   /** Positive values occur. */
   public static final int POSITIVE = 0x00000100;

   /** Negative values occur. */
   public static final int NEGATIVE = 0x00000200;

   /** Zero occurs; on a pattern output, no pattern on this bar. An output setting any of the three sign flags declares all its signs; one setting none declares nothing. */
   public static final int ZERO = 0x00000400;

   /** An upper band/limit line. */
   public static final int UPPER_LIMIT = 0x00000800;

   /** A lower band/limit line. */
   public static final int LOWER_LIMIT = 0x00001000;

   /** The typed call lets the caller decline it. A {@code ParamHolder} still needs it bound. */
   public static final int NULLABLE = 0x00002000;

   /** A chart draws it ahead of or behind the bar that computed it, by the bars {@code ParamHolder.displayShift} reports. The values are never shifted. */
   public static final int DISPLAY_SHIFT = 0x00004000;

   /** Adds level 80: a weaker form of the pattern, on the same bar. */
   public static final int PATTERN_WEAK = 0x00008000;

}
