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
 */

using System;

namespace TALib.Test;

/// <summary>The C# configuration surface: setting an unstable period, and the
/// domain that setting is held to.</summary>
/// <remarks>
/// <para>Ported case-for-case from the Java <c>CoreApiTest</c> so the two
/// managed bindings are held to the same contract, which is in turn the C
/// library's: a period outside <c>0..=MAX_INDEX</c> is refused, and a refused
/// call writes nothing.</para>
/// <para>Framework-free, like the other suites here — discovered by name and
/// run through <c>public static int Run()</c>.</para>
/// </remarks>
public static class CoreBuilderTest
{
    private static int _failures;
    private static int _checks;

    private static void Check(bool condition, string what)
    {
        _checks++;
        if (!condition)
        {
            _failures++;
            Console.WriteLine("  FAIL: " + what);
        }
    }

    private static void CheckThrows<TException>(Action body, string what)
        where TException : Exception
    {
        _checks++;
        try
        {
            body();
            _failures++;
            Console.WriteLine("  FAIL: " + what + " (no exception thrown)");
        }
        catch (Exception e)
        {
            if (e is not TException)
            {
                _failures++;
                Console.WriteLine("  FAIL: " + what + " (threw " + e.GetType().FullName + ")");
            }
        }
    }

    private static void DefaultsAreZero()
    {
        Core core = Core.Builder().Build();
        foreach (FuncUnstId id in Enum.GetValues<FuncUnstId>())
        {
            if (id != FuncUnstId.ALL)
            {
                Check(core.UnstablePeriod(id) == 0, $"{id} defaults to 0");
            }
        }
    }

    private static void SetsOneAndOnlyOne()
    {
        Core core = Core.Builder().UnstablePeriod(FuncUnstId.EMA, 10).Build();
        Check(core.UnstablePeriod(FuncUnstId.EMA) == 10, "EMA was set to 10");
        Check(core.UnstablePeriod(FuncUnstId.RSI) == 0, "RSI was left alone");
    }

    private static void WildcardSetsEveryFunction()
    {
        Core core = Core.Builder().UnstablePeriod(FuncUnstId.ALL, 7).Build();
        Check(core.UnstablePeriod(FuncUnstId.EMA) == 7, "the wildcard reached EMA");
        Check(core.UnstablePeriod(FuncUnstId.T3) == 7, "the wildcard reached T3");
    }

    private static void ReachesTheIndicator()
    {
        // A behavioural witness rather than a field read: the period must move the
        // lookback, or the setter could be storing into something nothing reads.
        Core plain = new Core();
        Core tuned = Core.Builder().UnstablePeriod(FuncUnstId.EMA, 5).Build();
        Check(tuned.EMA_Lookback(10) == plain.EMA_Lookback(10) + 5,
            "the unstable period is added to EMA's lookback");
    }

    private static void MisuseThrows()
    {
        // ArgumentOutOfRangeException, not merely its ArgumentException base:
        // CheckThrows accepts subclasses, so asking for the base would pass
        // against any argument exception and leave the documented type untested.
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().UnstablePeriod(FuncUnstId.RSI, -1),
            "negative period -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().UnstablePeriod(FuncUnstId.RSI, Core.MAX_INDEX + 1),
            "period above MAX_INDEX -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().UnstablePeriod(FuncUnstId.RSI, int.MaxValue),
            "int.MaxValue period -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().UnstablePeriod(FuncUnstId.ALL, Core.MAX_INDEX + 1),
            "wildcard period above MAX_INDEX -> ArgumentOutOfRangeException");
        // Unlike Java, a C# enum is not a closed domain -- (FuncUnstId)(-1) and
        // (FuncUnstId)9999 are representable values a caller can pass, and both
        // index off the end of a 24-slot array. C guards this with an unsigned
        // compare for exactly that reason (#144).
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().UnstablePeriod((FuncUnstId)(-1), 1),
            "a negative id cast into the enum -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().UnstablePeriod((FuncUnstId)9999, 1),
            "an out-of-range id cast into the enum -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => new Core().UnstablePeriod(FuncUnstId.ALL),
            "the wildcard has no single value to read -> ArgumentOutOfRangeException");
    }

    private static void BoundIsABoundNotAnOffByOne()
    {
        // MAX_INDEX itself is legal: C accepts it and rejects MAX_INDEX + 1, so a
        // guard tightened by one would be caught here rather than shipping.
        Core core = Core.Builder().UnstablePeriod(FuncUnstId.EMA, Core.MAX_INDEX).Build();
        Check(core.UnstablePeriod(FuncUnstId.EMA) == Core.MAX_INDEX,
            "the MAX_INDEX ceiling is accepted, not rejected");
    }

    private static void ARejectedCallWritesNothing()
    {
        // The half of the contract an "it throws" assertion cannot see.
        CoreBuilder b = Core.Builder().UnstablePeriod(FuncUnstId.EMA, 7);
        CheckThrows<ArgumentOutOfRangeException>(
            () => b.UnstablePeriod(FuncUnstId.EMA, Core.MAX_INDEX + 1),
            "the rejected overwrite still throws");
        Check(b.Build().UnstablePeriod(FuncUnstId.EMA) == 7,
            "a rejected period leaves the previous value in place");

        // The wildcard path writes 24 slots, so a rejection there must not have
        // filled any of them before noticing.
        CoreBuilder w = Core.Builder().UnstablePeriod(FuncUnstId.ALL, 3);
        CheckThrows<ArgumentOutOfRangeException>(
            () => w.UnstablePeriod(FuncUnstId.ALL, int.MaxValue),
            "the rejected wildcard still throws");
        Core after = w.Build();
        bool intact = true;
        foreach (FuncUnstId id in Enum.GetValues<FuncUnstId>())
        {
            if (id != FuncUnstId.ALL && after.UnstablePeriod(id) != 3)
            {
                intact = false;
            }
        }
        Check(intact, "a rejected wildcard leaves all 24 slots at their previous value");
    }

    private static void BuiltCoreIsIsolatedFromTheBuilder()
    {
        CoreBuilder b = Core.Builder().UnstablePeriod(FuncUnstId.EMA, 4);
        Core core = b.Build();
        b.UnstablePeriod(FuncUnstId.EMA, 9);
        Check(core.UnstablePeriod(FuncUnstId.EMA) == 4,
            "a built Core does not alias the builder's array");
    }

    private static void ToBuilderRoundTrips()
    {
        Core original = Core.Builder()
            .UnstablePeriod(FuncUnstId.RSI, 5)
            .CandleSetting(CandleSettingType.BodyLong, RangeType.Shadows, 20, 1.5)
            .Build();
        Core derived = original.ToBuilder().UnstablePeriod(FuncUnstId.EMA, 9).Build();
        Check(original.UnstablePeriod(FuncUnstId.EMA) == 0, "the original is untouched");
        Check(derived.UnstablePeriod(FuncUnstId.RSI) == 5, "the derived Core inherits RSI");
        Check(derived.UnstablePeriod(FuncUnstId.EMA) == 9, "the derived Core gains EMA");
        // Candle settings survive the round trip too -- this is what catches
        // ToBuilder dropping a field, which a periods-only check cannot see.
        CandleSetting carried = derived.CandleSettings(CandleSettingType.BodyLong);
        Check(carried.AvgPeriod == 20 && carried.Factor == 1.5
              && carried.RangeType == RangeType.Shadows,
            "the derived Core inherits the candle settings");
    }

    private static void CandleDefaultsAreTheDocumentedOnes()
    {
        Core core = Core.Builder().Build();
        // A representative default (BodyDoji: HighLow range, 10 bars, 0.1).
        CandleSetting doji = core.CandleSettings(CandleSettingType.BodyDoji);
        Check(doji.RangeType == RangeType.HighLow, "BodyDoji defaults to the HighLow range");
        Check(doji.AvgPeriod == 10, "BodyDoji defaults to a 10-bar average");
        Check(doji.Factor == 0.1, "BodyDoji defaults to a 0.1 factor");
    }

    private static void CandleSettingOverridesOneLeavesTheRest()
    {
        Core core = Core.Builder()
            .CandleSetting(CandleSettingType.BodyLong, RangeType.Shadows, 20, 1.5)
            .Build();
        CandleSetting bodyLong = core.CandleSettings(CandleSettingType.BodyLong);
        Check(bodyLong.RangeType == RangeType.Shadows, "BodyLong took the new range type");
        Check(bodyLong.AvgPeriod == 20, "BodyLong took the new avgPeriod");
        Check(bodyLong.Factor == 1.5, "BodyLong took the new factor");
        Check(core.CandleSettings(CandleSettingType.BodyDoji).AvgPeriod == 10,
            "a different setting keeps its default");
    }

    private static void CandleSettingReachesTheIndicator()
    {
        // A behavioural witness, not a getter echo: identical clear candles --
        // real body 4, high-low range 6 -- are never dojis at the default
        // BodyDoji threshold (0.1), but a huge factor makes the threshold
        // enormous so every candle qualifies.
        const int n = 20;
        double[] open = new double[n], high = new double[n], low = new double[n], close = new double[n];
        for (int i = 0; i < n; i++)
        {
            open[i] = 100.0;
            close[i] = 104.0;
            high[i] = 105.0;
            low[i] = 99.0;
        }

        int[] outDefault = new int[n], outTuned = new int[n];
        OutRange rd = new Core().CDLDOJI(0, n - 1, open, high, low, close, outDefault);
        Core tuned = Core.Builder()
            .CandleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 10, 1.0e9)
            .Build();
        OutRange rt = tuned.CDLDOJI(0, n - 1, open, high, low, close, outTuned);

        bool noneByDefault = true, allWhenTuned = true;
        for (int i = 0; i < rd.Count; i++)
        {
            if (outDefault[i] != 0) { noneByDefault = false; }
        }
        for (int i = 0; i < rt.Count; i++)
        {
            if (outTuned[i] != 100) { allWhenTuned = false; }
        }
        Check(rd.Count > 0 && rt.Count > 0, "both CDLDOJI calls produced values");
        Check(noneByDefault, "clear candles are not dojis at the default threshold");
        Check(allWhenTuned, "a huge BodyDoji factor marks every candle a doji");
    }

    private static void CandleMisuseThrows()
    {
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().CandleSetting(
                CandleSettingType.AllCandleSettings, RangeType.HighLow, 10, 1.0),
            "AllCandleSettings as a single-setting target -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().CandleSetting(
                CandleSettingType.BodyDoji, (RangeType)3, 10, 1.0),
            "a range type outside the enum -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().CandleSetting(
                CandleSettingType.BodyDoji, RangeType.HighLow, -1, 1.0),
            "negative avgPeriod -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().CandleSetting(
                CandleSettingType.BodyDoji, RangeType.HighLow, Core.MAX_INDEX + 1, 1.0),
            "avgPeriod above MAX_INDEX -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().CandleSetting(
                CandleSettingType.BodyDoji, RangeType.HighLow, 10, double.NaN),
            "NaN factor -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => new Core().CandleSettings(CandleSettingType.AllCandleSettings),
            "AllCandleSettings has no single value to read -> ArgumentOutOfRangeException");
    }

    private static void CandleBoundsAreBoundsNotOffByOnes()
    {
        // Every accepted boundary on the legal side, so a guard tightened by one
        // is caught here rather than shipping.
        Core zero = Core.Builder()
            .CandleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 0, 0.1).Build();
        Check(zero.CandleSettings(CandleSettingType.BodyDoji).AvgPeriod == 0,
            "a zero avgPeriod means no averaging and is legal");

        Core ceiling = Core.Builder()
            .CandleSetting(CandleSettingType.BodyDoji, RangeType.Shadows, Core.MAX_INDEX, 0.1).Build();
        Check(ceiling.CandleSettings(CandleSettingType.BodyDoji).AvgPeriod == Core.MAX_INDEX,
            "the MAX_INDEX ceiling is accepted, not rejected");

        // Only NaN is refused; a negative factor is unusual but legal, and C
        // accepts it too.
        Core negative = Core.Builder()
            .CandleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 10, -1.5).Build();
        Check(negative.CandleSettings(CandleSettingType.BodyDoji).Factor == -1.5,
            "a negative factor is legal");
    }

    private static void ARejectedCandleSettingWritesNothing()
    {
        CoreBuilder b = Core.Builder()
            .CandleSetting(CandleSettingType.BodyDoji, RangeType.Shadows, 7, 2.5);
        CheckThrows<ArgumentOutOfRangeException>(
            () => b.CandleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, -1, 1.0),
            "the rejected overwrite still throws");
        CandleSetting kept = b.Build().CandleSettings(CandleSettingType.BodyDoji);
        Check(kept.AvgPeriod == 7 && kept.Factor == 2.5 && kept.RangeType == RangeType.Shadows,
            "a rejected candle setting leaves the previous one in place");
    }

    private static void RestoreCandleDefaultUndoesAnOverride()
    {
        CoreBuilder b = Core.Builder()
            .CandleSetting(CandleSettingType.BodyDoji, RangeType.Shadows, 33, 9.0)
            .CandleSetting(CandleSettingType.Equal, RangeType.RealBody, 44, 8.0);

        Core one = b.RestoreCandleDefault(CandleSettingType.BodyDoji).Build();
        Check(one.CandleSettings(CandleSettingType.BodyDoji).AvgPeriod == 10,
            "restoring one setting returns it to its default");
        Check(one.CandleSettings(CandleSettingType.Equal).AvgPeriod == 44,
            "restoring one setting leaves the others overridden");

        // Both witnesses read Equal, the ONLY setting still overridden at this
        // point -- reading BodyDoji here would prove nothing, because the line
        // above restored it individually.
        Core all = b.RestoreCandleDefault(CandleSettingType.AllCandleSettings).Build();
        Check(all.CandleSettings(CandleSettingType.Equal).AvgPeriod == 5,
            "the wildcard restores every setting");
        Check(all.CandleSettings(CandleSettingType.Equal).Factor == 0.05,
            "the wildcard restores every setting's factor too");
        Check(all.CandleSettings(CandleSettingType.Equal).RangeType == RangeType.HighLow,
            "the wildcard restores every setting's range type too");
    }

    private static void BuiltCoreDoesNotAliasTheBuildersCandles()
    {
        CoreBuilder b = Core.Builder()
            .CandleSetting(CandleSettingType.Near, RangeType.HighLow, 12, 0.3);
        Core core = b.Build();
        b.CandleSetting(CandleSettingType.Near, RangeType.RealBody, 99, 7.0);
        Check(core.CandleSettings(CandleSettingType.Near).AvgPeriod == 12,
            "a built Core does not alias the builder's candle array");

        // ...and the shared defaults array is never written through.
        Check(Core.DefaultCandleSettings[(int)CandleSettingType.Near].AvgPeriod == 5,
            "overriding a setting never mutates the shared defaults");
    }

    /// <summary>Runs every check in this suite.</summary>
    /// <returns>0 when they all pass, 1 otherwise.</returns>
    public static int Run()
    {
        DefaultsAreZero();
        SetsOneAndOnlyOne();
        WildcardSetsEveryFunction();
        ReachesTheIndicator();
        MisuseThrows();
        BoundIsABoundNotAnOffByOne();
        ARejectedCallWritesNothing();
        BuiltCoreIsIsolatedFromTheBuilder();
        ToBuilderRoundTrips();
        CandleDefaultsAreTheDocumentedOnes();
        CandleSettingOverridesOneLeavesTheRest();
        CandleSettingReachesTheIndicator();
        CandleMisuseThrows();
        CandleBoundsAreBoundsNotOffByOnes();
        ARejectedCandleSettingWritesNothing();
        RestoreCandleDefaultUndoesAnOverride();
        BuiltCoreDoesNotAliasTheBuildersCandles();

        if (_failures == 0)
        {
            Console.WriteLine($"CoreBuilderTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"CoreBuilderTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
