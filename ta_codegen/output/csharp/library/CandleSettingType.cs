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

/* Hand-written library scaffolding; ta_codegen never opens this file. */

namespace TALib;

/// <summary>Identifies one configurable candlestick threshold. The values
/// index <c>Core</c>'s candle-settings table, matching the C library's
/// <c>TA_CandleSettingType</c>.</summary>
public enum CandleSettingType
{
    /// <summary>A long candle body.</summary>
    BodyLong = 0,
    /// <summary>A very long candle body.</summary>
    BodyVeryLong = 1,
    /// <summary>A short candle body.</summary>
    BodyShort = 2,
    /// <summary>A doji body (near-zero).</summary>
    BodyDoji = 3,
    /// <summary>A long shadow.</summary>
    ShadowLong = 4,
    /// <summary>A very long shadow.</summary>
    ShadowVeryLong = 5,
    /// <summary>A short shadow.</summary>
    ShadowShort = 6,
    /// <summary>A very short shadow (near-zero).</summary>
    ShadowVeryShort = 7,
    /// <summary>Two prices are "near" each other.</summary>
    Near = 8,
    /// <summary>Two prices are "far" from each other.</summary>
    Far = 9,
    /// <summary>Two prices are "equal" (within tolerance).</summary>
    Equal = 10,
    /// <summary>Wildcard: all settings at once (also the table size).</summary>
    AllCandleSettings = 11,
}
