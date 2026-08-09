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

public enum FuncUnstId {
	
	  /* 000 */  ADX(0),
	  /* 001 */  UNUSED_1(1),
	  /* 002 */  ATR(2),
	  /* 003 */  CMO(3),
	  /* 004 */  DX(4),
	  /* 005 */  EMA(5),
	  /* 006 */  HT_DCPERIOD(6),
	  /* 007 */  HT_DCPHASE(7),
	  /* 008 */  HT_PHASOR(8),
	  /* 009 */  HT_SINE(9),
	  /* 010 */  HT_TRENDLINE(10),
	  /* 011 */  HT_TRENDMODE(11),
	  /* 012 */  UNUSED_12(12),
	  /* 013 */  KAMA(13),
	  /* 014 */  MAMA(14),
	  /* 015 */  UNUSED_15(15),
	  /* 016 */  MINUS_DI(16),
	  /* 017 */  MINUS_DM(17),
	  /* 018 */  NATR(18),
	  /* 019 */  PLUS_DI(19),
	  /* 020 */  PLUS_DM(20),
	  /* 021 */  RSI(21),
	  /* 022 */  UNUSED_22(22),
	  /* 023 */  T3(23),

	  /** Wildcard: sets the unstable period for every function at once.
	   *  Pinned, so adding an indicator can never move it. */
	             ALL(65535);

	/** Number of function ids — the size of the unstable-period table.
	 *  Not an id, and not {@link #ALL}. Mirrors C's TA_FUNC_UNST_COUNT. */
	public static final int COUNT = 24;

	private final int value;

	FuncUnstId(int value) { this.value = value; }

	/** The C {@code TA_FuncUnstId} value for this id. */
	public int value() { return value; }
};
