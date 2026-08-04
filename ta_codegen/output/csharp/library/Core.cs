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

/* Hand-written library scaffolding; ta_codegen never opens this file. The
 * generated per-indicator methods live in src/Core_<NAME>.cs — each a
 * `partial class Core` piece of this type. */

using System;

namespace TALib;

/// <summary>All TA-Lib indicators, as instance methods on this class.</summary>
/// <remarks>
/// Every indicator follows the same pattern: inputs are <c>double[]</c> (or
/// <c>float[]</c> overloads), computed over the bar range
/// <c>startIdx..endIdx</c> inclusive; outputs are written into caller-provided
/// arrays; the returned <see cref="OutRange"/> reports the input index of the
/// first output value and how many were written. An indicator consumes a
/// number of leading bars (its <em>lookback</em>) before producing output —
/// query it with the matching <c>*Lookback</c> method. Integer parameters
/// accept <c>int.MinValue</c>, and real parameters <c>-4e37</c>, to select
/// their documented default.
/// <para>Per-instance settings (unstable periods, candlestick thresholds)
/// currently take their documented defaults; a configuration builder arrives
/// with a later milestone. A <c>Core</c> whose settings are never mutated is
/// safe to share read-only across threads.</para>
/// </remarks>
public partial class Core
{
    /// <summary>The catalogue of every indicator, for choosing one at run
    /// time.</summary>
    /// <remarks>A discovery hook, so a caller does not have to know the
    /// <c>TALib.Metadata</c> namespace exists. See
    /// <see cref="TALib.Metadata.FunctionCatalog"/>.</remarks>
    public static TALib.Metadata.FunctionCatalog Functions => TALib.Metadata.FunctionCatalog.Default;

    /* The parameter sentinels the generated validation names. Values match
     * the C library's ta_defs.h. */
    internal const double TA_REAL_DEFAULT = -4e37;
    internal const double TA_REAL_MIN = -3e37;
    internal const double TA_REAL_MAX = 3e37;
    internal const int TA_INTEGER_DEFAULT = int.MinValue;
    internal const int TA_INTEGER_MIN = int.MinValue + 1;
    internal const int TA_INTEGER_MAX = int.MaxValue;

    /* Sized by the id count, so the All wildcard gets no slot (#144). */
    internal readonly int[] unstablePeriod = new int[FuncUnstIds.Count];

    /* candleSettings[] in CandleSettingType order. Defaults from
     * TA_RestoreCandleDefaultSettings in ta_global.c. */
    internal readonly CandleSetting[] candleSettings =
    {
        new CandleSetting(RangeType.RealBody, 10, 1.0),   // BodyLong
        new CandleSetting(RangeType.RealBody, 10, 3.0),   // BodyVeryLong
        new CandleSetting(RangeType.RealBody, 10, 1.0),   // BodyShort
        new CandleSetting(RangeType.HighLow,  10, 0.1),   // BodyDoji
        new CandleSetting(RangeType.RealBody, 0,  1.0),   // ShadowLong
        new CandleSetting(RangeType.RealBody, 0,  2.0),   // ShadowVeryLong
        new CandleSetting(RangeType.Shadows,  10, 1.0),   // ShadowShort
        new CandleSetting(RangeType.HighLow,  10, 0.1),   // ShadowVeryShort
        new CandleSetting(RangeType.HighLow,  5,  0.2),   // Near
        new CandleSetting(RangeType.HighLow,  5,  0.6),   // Far
        new CandleSetting(RangeType.HighLow,  5,  0.05),  // Equal
    };

    /// <summary>Create a Core with every setting at its documented
    /// default.</summary>
    public Core()
    {
    }

    /* The RetCode -> exception mapping the generated guarded wrappers throw
     * through. Returns (rather than throws) so the call sites read
     * `throw Failure(...)` and the compiler knows the path ends. */
    internal static Exception Failure(string funcName, RetCode retCode)
    {
        string where = "TA_" + funcName + ": ";
        switch (retCode)
        {
            case RetCode.OutOfRangeStartIndex:
                return new ArgumentOutOfRangeException("startIdx", where + "startIdx out of range");
            case RetCode.OutOfRangeEndIndex:
                return new ArgumentOutOfRangeException("endIdx", where + "endIdx out of range");
            case RetCode.BadParam:
                return new ArgumentException(where + "bad parameter");
            case RetCode.AllocErr:
                return new InvalidOperationException(where + "allocation failed");
            case RetCode.InternalError:
                return new InvalidOperationException(where + "internal error");
            default:
                return new InvalidOperationException(where + retCode);
        }
    }
}
