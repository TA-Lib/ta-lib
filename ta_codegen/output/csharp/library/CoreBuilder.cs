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
 *  081026 MF,CC  First Version -- the unstable-period builder (#186).
 *
 * ta_codegen never opens this file: it is hand-written scaffolding living one
 * level above library/src/, which is the generated tree.
 */

using System;

namespace TALib;

/// <summary>Builds a <see cref="Core"/> with non-default settings.</summary>
/// <remarks>
/// <para>Obtain one from <see cref="Core.Builder"/> (TA-Lib defaults) or
/// <see cref="Core.ToBuilder"/> (seeded from an existing <c>Core</c>), configure
/// it, then call <see cref="Build"/>:</para>
/// <code>
/// Core core = Core.Builder()
///     .UnstablePeriod(FuncUnstId.EMA, 10)
///     .Build();
/// </code>
/// <para>Setters throw rather than return a code, which is the house contract
/// here — no public C# path surfaces a <see cref="RetCode"/>. Rust's peer of
/// this type reports the same rejection as <c>RetCode::BadParam</c> from its
/// <c>build()</c>, because a Rust setter cannot throw; the domain being enforced
/// is identical in both.</para>
/// <para>A builder is <b>not</b> thread-safe (the standard builder contract —
/// confine it to one thread); the <c>Core</c> it produces is.</para>
/// </remarks>
public sealed class CoreBuilder
{
    /* Sized by the id count, so the ALL wildcard gets no slot (#144). */
    private readonly int[] unstablePeriod;

    /* In CandleSettingType order; the AllCandleSettings wildcard gets no slot. */
    private readonly CandleSetting[] candleSettings;

    /// <summary>A builder seeded with TA-Lib's defaults.</summary>
    public CoreBuilder()
    {
        unstablePeriod = new int[FuncUnstIds.Count];
        // CandleSetting is immutable, so the default instances are shared, not copied.
        candleSettings = (CandleSetting[])Core.DefaultCandleSettings.Clone();
    }

    /* Seeded from an existing Core's settings (see Core.ToBuilder). */
    internal CoreBuilder(int[] seedPeriods, CandleSetting[] seedCandles)
    {
        unstablePeriod = (int[])seedPeriods.Clone();
        candleSettings = (CandleSetting[])seedCandles.Clone();
    }

    /// <summary>Sets the unstable period for one function, or for every
    /// function at once when given <see cref="FuncUnstId.ALL"/>.</summary>
    /// <param name="id">The function to configure, or <see cref="FuncUnstId.ALL"/>
    /// as the set-all wildcard, mirroring C's <c>TA_SetUnstablePeriod</c>.</param>
    /// <param name="period">Extra warm-up bars, in <c>0</c>..<see cref="Core.MAX_INDEX"/>.</param>
    /// <returns>This builder, for chaining.</returns>
    /// <exception cref="ArgumentOutOfRangeException"><paramref name="period"/> is
    /// negative or above <see cref="Core.MAX_INDEX"/>, or <paramref name="id"/>
    /// is neither a function id nor the wildcard. A rejected call writes
    /// nothing.</exception>
    public CoreBuilder UnstablePeriod(FuncUnstId id, int period)
    {
        /* The period is added to a lookback which is then used as an index, so an
         * unbounded one overflows that lookback negative and the function indexes
         * far past the end of its input. MAX_INDEX is the ceiling the index space
         * already enforces on startIdx/endIdx; a warm-up longer than the largest
         * addressable series could never produce output, so nothing legitimate is
         * refused. C applies the same bound in TA_SetUnstablePeriod.
         */
        if (period < 0 || period > Core.MAX_INDEX)
        {
            throw new ArgumentOutOfRangeException(nameof(period), period,
                "unstable period must be in 0.." + Core.MAX_INDEX);
        }

        /* A C# enum is NOT a closed domain -- (FuncUnstId)(-1) and (FuncUnstId)9999
         * are representable values a caller can pass, and both index off the end of
         * the table. Java can lean on its enum type here; C# cannot, so the id is
         * range-checked numerically exactly as C does with its unsigned compare.
         */
        if (id == FuncUnstId.ALL)
        {
            for (int i = 0; i < unstablePeriod.Length; i++)
            {
                unstablePeriod[i] = period;
            }
            return this;
        }

        int slot = (int)id;
        if (slot < 0 || slot >= FuncUnstIds.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(id), id,
                "not a function id, and not the ALL wildcard");
        }
        unstablePeriod[slot] = period;
        return this;
    }

    /// <summary>Overrides one candlestick threshold, mirroring C's
    /// <c>TA_SetCandleSettings</c>.</summary>
    /// <param name="settingType">Which threshold to override.</param>
    /// <param name="rangeType">What the candle dimension is measured against.</param>
    /// <param name="avgPeriod">How many prior bars to average, in
    /// <c>0</c>..<see cref="Core.MAX_INDEX"/>. <c>0</c> means no averaging.</param>
    /// <param name="factor">The multiplier applied to that average. Any value
    /// except NaN, negatives included.</param>
    /// <returns>This builder, for chaining.</returns>
    /// <exception cref="ArgumentOutOfRangeException"><paramref name="settingType"/>
    /// does not name a single setting (<see cref="CandleSettingType.AllCandleSettings"/>
    /// is a wildcard, meaningful only for <see cref="RestoreCandleDefault"/>),
    /// <paramref name="rangeType"/> is not a <see cref="TALib.RangeType"/> member,
    /// <paramref name="avgPeriod"/> is outside its range, or
    /// <paramref name="factor"/> is NaN. A rejected call writes nothing.</exception>
    public CoreBuilder CandleSetting(CandleSettingType settingType, RangeType rangeType,
                                     int avgPeriod, double factor)
    {
        int slot = (int)settingType;
        if (slot < 0 || slot >= candleSettings.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(settingType), settingType,
                "not a single candlestick setting");
        }
        /* A choice list's domain is its member list. Anything else falls through
         * every arm of the range computation to 0.0 -- every range zero, every
         * threshold zero, and a silently meaningless answer. */
        if (rangeType != TALib.RangeType.RealBody
            && rangeType != TALib.RangeType.HighLow
            && rangeType != TALib.RangeType.Shadows)
        {
            throw new ArgumentOutOfRangeException(nameof(rangeType), rangeType,
                "not a range type");
        }
        /* avgPeriod is the lookback of every CDL* function that reads the
         * setting, so it is bounded like one: a negative starts the main loop
         * that many bars late while outBegIdx still reports startIdx, shifting
         * every value underneath a correct-looking index. */
        if (avgPeriod < 0 || avgPeriod > Core.MAX_INDEX)
        {
            throw new ArgumentOutOfRangeException(nameof(avgPeriod), avgPeriod,
                "avgPeriod must be in 0.." + Core.MAX_INDEX);
        }
        /* Only NaN is refused -- a negative factor is an unusual but legal
         * threshold scale. NaN makes every comparison it feeds false, so the
         * patterns simply stop matching, indistinguishable from "this shape
         * never occurs". */
        if (double.IsNaN(factor))
        {
            throw new ArgumentOutOfRangeException(nameof(factor), factor,
                "factor must not be NaN");
        }
        candleSettings[slot] = new CandleSetting(rangeType, avgPeriod, factor);
        return this;
    }

    /// <summary>Restores one candlestick threshold to its TA-Lib default, or
    /// every one when given <see cref="CandleSettingType.AllCandleSettings"/>.</summary>
    /// <param name="settingType">The setting to restore, or the wildcard.</param>
    /// <returns>This builder, for chaining.</returns>
    /// <exception cref="ArgumentOutOfRangeException"><paramref name="settingType"/>
    /// is neither a setting nor the wildcard.</exception>
    public CoreBuilder RestoreCandleDefault(CandleSettingType settingType)
    {
        if (settingType == CandleSettingType.AllCandleSettings)
        {
            Array.Copy(Core.DefaultCandleSettings, candleSettings, candleSettings.Length);
            return this;
        }
        int slot = (int)settingType;
        if (slot < 0 || slot >= candleSettings.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(settingType), settingType,
                "not a candlestick setting, and not the wildcard");
        }
        candleSettings[slot] = Core.DefaultCandleSettings[slot];
        return this;
    }

    /// <summary>Produces the immutable <see cref="Core"/>. The builder stays
    /// usable afterwards, and the <c>Core</c> does not alias it.</summary>
    /// <returns>A configured <see cref="Core"/>.</returns>
    public Core Build()
    {
        return new Core(this);
    }

    /* Defensive copies handed to Core's constructor -- the builder keeps its own
     * arrays, so later builder calls cannot reach into a built Core. */
    internal int[] SnapshotUnstablePeriod()
    {
        return (int[])unstablePeriod.Clone();
    }

    internal CandleSetting[] SnapshotCandleSettings()
    {
        return (CandleSetting[])candleSettings.Clone();
    }
}
