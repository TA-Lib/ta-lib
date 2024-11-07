/* TA-LIB Copyright (c) 1999-2007, Mario Fortier
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
 *  RG       Richard Gomes
 *
 * Change history:
 *
 *  YYYYMMDD BY     Description
 *  -------------------------------------------------------------------
 *  20070311 RG     First Version
 */

package com.tictactec.ta.lib.meta.annotation;


public final class OutputFlags {
    static public final int TA_OUT_LINE              = 0x00000001; /* Suggest to display as a connected line. */
    static public final int TA_OUT_DOT_LINE          = 0x00000002; /* Suggest to display as a 'dotted' line. */
    static public final int TA_OUT_DASH_LINE         = 0x00000004; /* Suggest to display as a 'dashed' line. */
    static public final int TA_OUT_DOT               = 0x00000008; /* Suggest to display with dots only. */
    static public final int TA_OUT_HISTO             = 0x00000010; /* Suggest to display as an histogram. */
    static public final int TA_OUT_PATTERN_BOOL      = 0x00000020; /* Indicates if pattern exists (!=0) or not (0) */
    static public final int TA_OUT_PATTERN_BULL_BEAR = 0x00000040; /* =0 no pattern, > 0 bullish, < 0 bearish */
    static public final int TA_OUT_PATTERN_STRENGTH  = 0x00000080; /* =0 neutral, ]0..100] getting bullish, ]100..200] bullish, [-100..0[ getting bearish, [-200..100[ bearish */
    static public final int TA_OUT_POSITIVE          = 0x00000100; /* Output can be positive */
    static public final int TA_OUT_NEGATIVE          = 0x00000200; /* Output can be negative */
    static public final int TA_OUT_ZERO              = 0x00000400; /* Output can be zero */
}
