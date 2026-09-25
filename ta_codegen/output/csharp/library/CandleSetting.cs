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

using System.Globalization;

namespace TALib;

/// <summary>One candlestick threshold: how a candle dimension is measured
/// (<see cref="RangeType"/>), over how many bars it is averaged, and the
/// factor a comparison is scaled by.</summary>
public sealed class CandleSetting
{
    internal readonly RangeType _rangeType;
    internal readonly int _avgPeriod;
    internal readonly double _factor;

    /* Read accessors, matching Java's. The fields stay internal because the
     * generated indicator bodies read them directly; these are what a caller
     * outside the assembly sees. */

    /// <summary>What the candle dimension is measured against.</summary>
    public RangeType RangeType => _rangeType;

    /// <summary>How many prior bars are averaged (<c>0</c> means no averaging —
    /// the current candle only).</summary>
    public int AvgPeriod => _avgPeriod;

    /// <summary>The multiplier applied to that average to form the
    /// threshold.</summary>
    public double Factor => _factor;

    /// <summary>For example
    /// <c>CandleSetting { RangeType = HighLow, AvgPeriod = 10, Factor = 0.1 }</c>,
    /// the same in every culture.</summary>
    /// <returns>The three components.</returns>
    public override string ToString() =>
        string.Create(CultureInfo.InvariantCulture,
            $"CandleSetting {{ RangeType = {_rangeType}, AvgPeriod = {_avgPeriod}, Factor = {_factor} }}");

    internal CandleSetting(RangeType rangeType, int avgPeriod, double factor)
    {
        _rangeType = rangeType;
        _avgPeriod = avgPeriod;
        _factor = factor;
    }
}
