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
 *
 * List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  081626 MF,CC  First Version — the streaming API's C# contract.
 */

/* Hand-written test; ta_codegen never opens this file. */

using System;
using System.Linq;
using System.Reflection;
using TALib;
using TALib.Metadata;

namespace TALib.Test;

/// <summary>
/// The streaming API's contract: what a C# caller may rely on from a stream
/// handle, and which misuses throw what.
/// </summary>
/// <remarks>
/// <para>Numerical correctness is NOT this file's job. <c>ta_regtest --codegen</c>
/// proves every streamed bar bit-identical to the batch tier for all catalogued
/// streaming functions, in-process, against the same inputs — a far stronger
/// check than anything expressible here. What this file pins is the surface a
/// C# caller touches and the promises the XML docs make, neither of which any
/// cross-language harness ever sees.</para>
/// <para>Framework-free, like the other suites: a plain <c>Run()</c>, no test
/// dependency in the shipped library.</para>
/// <para><b>Every count comes from <see cref="FunctionCatalog.Default"/>, never
/// from a literal.</b> The synthetic-function gate injects extra functions into
/// the generator's input tree and regenerates, so a hardcoded corpus size here
/// would break the build before any gate leg ran — which is exactly how the
/// Java suite's pinned count once behaved.</para>
/// </remarks>
public static class StreamApiTest
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

    /* A gently trending, non-monotonic series: enough shape that a windowed
     * indicator produces distinct values, so the "did anything actually move"
     * assertions below are not satisfied by a constant. */
    private static double[] Closes(int n)
    {
        var v = new double[n];
        for (int i = 0; i < n; i++)
        {
            v[i] = 100.0 + (i * 0.37) + (8.0 * Math.Sin(i * 0.21)) + (2.0 * Math.Sin(i * 1.7));
        }
        return v;
    }

    private static long Bits(double d) => BitConverter.DoubleToInt64Bits(d);

    /* ------------------------------------------------------------------ */

    /// <summary>A stream tracks the batch call bar for bar.</summary>
    private static void StreamMatchesBatch()
    {
        var core = new Core();
        double[] closes = Closes(300);
        const int period = 14;

        int lookback = core.SMA_Lookback(period);
        var history = closes[..(lookback + 1)];
        Core.SMA_Stream s = core.SMA_Open(history, period);

        // Open seeds the value at the last history bar.
        var batch0 = new double[closes.Length];
        OutRange r0 = core.SMA(0, lookback, closes, period, batch0);
        Check(Bits(s.Value) == Bits(batch0[r0.Count - 1]), "open seeds Value from the last history bar");

        int moved = 0;
        for (int t = lookback + 1; t < closes.Length; t++)
        {
            double streamed = s.Update(closes[t]);

            var batch = new double[closes.Length];
            OutRange r = core.SMA(0, t, closes, period, batch);
            double expected = batch[r.Count - 1];

            if (Bits(streamed) != Bits(expected))
            {
                Check(false, $"Update at bar {t} is bit-identical to batch(0,{t})");
                return;
            }
            if (t > lookback + 1 && Bits(streamed) != Bits(s.Value))
            {
                Check(false, $"Value equals the last Update at bar {t}");
                return;
            }
            moved++;
        }
        Check(moved > 0, "the Update sweep ran at least one bar");
        Check(true, $"Update tracks batch bit-for-bit over {moved} bars");
    }

    /// <summary>Peek evaluates without committing.</summary>
    private static void PeekDoesNotCommit()
    {
        var core = new Core();
        double[] closes = Closes(120);
        const int period = 10;
        int lookback = core.SMA_Lookback(period);

        Core.SMA_Stream s = core.SMA_Open(closes[..(lookback + 1)], period);
        double before = s.Value;

        double peeked = s.Peek(closes[lookback + 1]);
        Check(Bits(s.Value) == Bits(before), "Peek leaves Value untouched");

        double committed = s.Update(closes[lookback + 1]);
        Check(Bits(peeked) == Bits(committed), "Peek returns exactly what the next Update returns");
        Check(Bits(s.Value) != Bits(before), "Update does move Value (the Peek check above is not vacuous)");

        // Repeated peeks are idempotent, which is what makes the shared
        // per-thread scratch safe to reuse.
        double p1 = s.Peek(closes[lookback + 2]);
        double p2 = s.Peek(closes[lookback + 2]);
        Check(Bits(p1) == Bits(p2), "Peek is idempotent");
    }

    /// <summary>Clone forks: both halves evolve independently.</summary>
    private static void CloneIsAnIndependentDeepCopy()
    {
        var core = new Core();
        double[] closes = Closes(160);
        const int period = 12;
        int lookback = core.SMA_Lookback(period);

        Core.SMA_Stream a = core.SMA_Open(closes[..(lookback + 1)], period);
        for (int t = lookback + 1; t < 60; t++)
        {
            a.Update(closes[t]);
        }

        Core.SMA_Stream b = a.Clone();
        Check(Bits(a.Value) == Bits(b.Value), "a Clone starts at the same value");

        // Drive them on DIFFERENT bars. A shallow copy of the ring would let
        // one leak into the other.
        int diverged = 0;
        for (int t = 60; t < closes.Length; t++)
        {
            double ua = a.Update(closes[t]);
            double ub = b.Update(closes[t] + 25.0);
            if (Bits(ua) != Bits(ub))
            {
                diverged++;
            }
        }
        Check(diverged > 0, "the two halves genuinely diverged (the fork test is not vacuous)");

        // And the original still matches a stream that never forked.
        Core.SMA_Stream fresh = core.SMA_Open(closes[..(lookback + 1)], period);
        for (int t = lookback + 1; t < closes.Length; t++)
        {
            fresh.Update(closes[t]);
        }
        Check(Bits(a.Value) == Bits(fresh.Value), "forking did not disturb the original");
    }

    /// <summary>OpenAndFill writes the whole history and reports its range.</summary>
    private static void OpenAndFillMatchesBatch()
    {
        var core = new Core();
        double[] closes = Closes(200);
        const int period = 20;

        var batch = new double[closes.Length];
        OutRange br = core.SMA(0, closes.Length - 1, closes, period, batch);

        var filled = new double[closes.Length];
        Core.SMA_Stream s = core.SMA_OpenAndFill(closes, period, filled);

        Check(s.FillRange.BegIdx == br.BegIdx && s.FillRange.Count == br.Count,
              "FillRange equals the batch OutRange");
        Check(!s.FillRange.IsEmpty, "a filled range is not empty");

        bool same = true;
        for (int i = 0; i < br.Count; i++)
        {
            if (Bits(filled[i]) != Bits(batch[i]))
            {
                same = false;
            }
        }
        Check(same, "the filled array is bit-identical to batch(0, n-1)");

        // A plain open fills nothing, and says so.
        Core.SMA_Stream plain = core.SMA_Open(closes, period);
        Check(plain.FillRange.IsEmpty, "a plain Open reports an empty FillRange");
        Check(Bits(plain.Value) == Bits(s.Value), "both openers agree on the current value");
    }

    /// <summary>The documented rejections, with the documented types.</summary>
    private static void MisuseThrowsTheDocumentedException()
    {
        var core = new Core();
        double[] closes = Closes(100);
        const int period = 30;
        int lookback = core.SMA_Lookback(period);

        // Exactly `lookback` bars is one short: no output is defined.
        CheckThrows<InsufficientHistoryException>(
            () => core.SMA_Open(closes[..lookback], period),
            "history of exactly `lookback` bars throws InsufficientHistoryException");

        // ...and one more bar is enough.
        _checks++;
        try
        {
            core.SMA_Open(closes[..(lookback + 1)], period);
        }
        catch (Exception e)
        {
            _failures++;
            Console.WriteLine("  FAIL: lookback + 1 bars must open (threw " + e.GetType().Name + ")");
        }

        // The typed exception is catchable as ArgumentException too, which is
        // what lets a caller treat every open rejection uniformly if it wants.
        CheckThrows<ArgumentException>(
            () => core.SMA_Open(closes[..lookback], period),
            "InsufficientHistoryException is an ArgumentException");

        // A parameter outside its documented range is a plain ArgumentException.
        CheckThrows<ArgumentException>(
            () => core.SMA_Open(closes, 0),
            "an out-of-range period throws ArgumentException");

        // The message carries the stable "<NAME> open: " prefix.
        _checks++;
        try
        {
            core.SMA_Open(closes[..lookback], period);
            _failures++;
            Console.WriteLine("  FAIL: expected a short-history rejection");
        }
        catch (InsufficientHistoryException e)
        {
            if (!e.Message.StartsWith("SMA open:", StringComparison.Ordinal))
            {
                _failures++;
                Console.WriteLine("  FAIL: reject message lacks the \"SMA open:\" prefix: " + e.Message);
            }
        }
    }

    /// <summary>OpenAndFill refuses to write through an array it also reads.</summary>
    private static void OpenAndFillRejectsAliasing()
    {
        var core = new Core();
        double[] closes = Closes(120);

        CheckThrows<ArgumentException>(
            () => core.SMA_OpenAndFill(closes, 14, closes),
            "output aliasing the input is rejected");

        // A distinct array of the same length is fine — so the rejection above
        // is about identity, not about size.
        _checks++;
        try
        {
            core.SMA_OpenAndFill(closes, 14, new double[closes.Length]);
        }
        catch (Exception e)
        {
            _failures++;
            Console.WriteLine("  FAIL: a distinct output array must be accepted (threw " + e.GetType().Name + ")");
        }
    }

    /// <summary>Empty spans — which is what a null array becomes — are named.</summary>
    /// <remarks>The public openers are the only place this is checked — the
    /// composition seam and the internal cores are reached only with arrays the
    /// generator created, so a check there would be dead weight.</remarks>
    private static void NullArgumentsAreNamed()
    {
        var core = new Core();
        double[] closes = Closes(120);
        var outReal = new double[closes.Length];

        CheckThrows<ArgumentException>(
            () => core.SMA_Open(null!, 14),
            "SMA_Open(null) throws ArgumentException");
        CheckThrows<ArgumentException>(
            () => core.SMA_Open(Array.Empty<double>(), 14),
            "SMA_Open(empty) throws ArgumentException");
        CheckThrows<ArgumentException>(
            () => core.SMA_OpenAndFill(null!, 14, outReal),
            "SMA_OpenAndFill with a null input throws ArgumentException");

        // It names the offending parameter. Without the check the empty input
        // and the empty output would compare as overlapping and be rejected as
        // aliasing instead, which names the wrong problem.
        _checks++;
        try
        {
            core.SMA_OpenAndFill(null!, 14, null!);
            _failures++;
            Console.WriteLine("  FAIL: expected an empty-input rejection");
        }
        catch (ArgumentException e)
        {
            if (e.ParamName != "inReal")
            {
                _failures++;
                Console.WriteLine($"  FAIL: reported \"{e.ParamName}\", expected \"inReal\"");
            }
        }
    }

    /// <summary>int.MinValue selects the documented default, as in batch.</summary>
    private static void IntegerSentinelSelectsTheDocumentedDefault()
    {
        var core = new Core();
        double[] closes = Closes(200);

        Core.SMA_Stream sentinel = core.SMA_Open(closes, int.MinValue);
        Core.SMA_Stream explicitly = core.SMA_Open(closes, 30);   // SMA's documented default
        Check(Bits(sentinel.Value) == Bits(explicitly.Value),
              "SMA_Open(int.MinValue) equals SMA_Open(30) bitwise");
    }

    /// <summary>A handle answers to the Core it was opened on.</summary>
    /// <remarks>Uses a candlestick threshold rather than an unstable period:
    /// an unstable period only moves where output STARTS (the recursion is
    /// seeded at bar 0 either way), so it would make the divergence assertion
    /// below vacuous.</remarks>
    private static void SettingsAreCapturedFromTheOpeningCore()
    {
        double[] closes = Closes(200);
        var open = new double[closes.Length];
        var high = new double[closes.Length];
        var low = new double[closes.Length];
        for (int i = 0; i < closes.Length; i++)
        {
            open[i] = closes[i] - 0.4;
            high[i] = Math.Max(open[i], closes[i]) + 1.1;
            low[i] = Math.Min(open[i], closes[i]) - 1.1;
        }

        var plain = new Core();
        // A body-doji threshold this wide makes almost every bar a doji; the
        // default makes almost none. The two cores must disagree.
        var tuned = Core.Builder()
            .CandleSetting(CandleSettingType.BodyDoji, RangeType.HighLow, 10, 5.0)
            .Build();

        Core.CDLDOJI_Stream a = plain.CDLDOJI_Open(open, high, low, closes);
        Core.CDLDOJI_Stream b = tuned.CDLDOJI_Open(open, high, low, closes);

        int differing = 0;
        for (int t = 100; t < closes.Length; t++)
        {
            int ua = a.Update(open[t], high[t], low[t], closes[t]);
            int ub = b.Update(open[t], high[t], low[t], closes[t]);
            if (ua != ub)
            {
                differing++;
            }
        }
        Check(differing > 0, "a tuned candle threshold actually changes the streamed value");

        // ...and each still tracks ITS OWN core's batch call.
        var batch = new int[closes.Length];
        OutRange r = tuned.CDLDOJI(0, closes.Length - 1, open, high, low, closes, batch);
        Check(b.Value == batch[r.Count - 1],
              "a handle tracks the batch call on the Core it was opened from");

        var batchPlain = new int[closes.Length];
        OutRange rp = plain.CDLDOJI(0, closes.Length - 1, open, high, low, closes, batchPlain);
        Check(a.Value == batchPlain[rp.Count - 1],
              "the default-settings handle tracks the default-settings batch call");
    }

    /// <summary>Update allocates nothing.</summary>
    private static void UpdateDoesNotAllocate()
    {
        var core = new Core();
        double[] closes = Closes(4000);
        Core.SMA_Stream s = core.SMA_Open(closes[..40], 30);

        // Warm up: first-call JIT and any lazy init must not be attributed.
        double sink = 0;
        for (int t = 40; t < 400; t++)
        {
            sink += s.Update(closes[t]);
        }

        long before = GC.GetAllocatedBytesForCurrentThread();
        for (int t = 400; t < closes.Length; t++)
        {
            sink += s.Update(closes[t]);
        }
        long after = GC.GetAllocatedBytesForCurrentThread();

        // Consume the sink so the calls cannot be elided; without this the
        // measurement could read zero for a step that does allocate.
        Check(!double.IsNaN(sink), "the Update sweep produced a value (the sink is consumed)");
        Check(after == before, $"Update allocates nothing ({after - before} bytes over {closes.Length - 400} bars)");
    }

    /// <summary>Multi-output handles return a value type, not a fresh object.</summary>
    private static void MultiOutputValueIsAStruct()
    {
        var core = new Core();
        double[] closes = Closes(200);

        Core.BBANDS_Stream s = core.BBANDS_Open(closes, 20, 2.0, 2.0, MAType.SMA);
        Core.BBANDS_Value v = s.Value;

        Check(typeof(Core.BBANDS_Value).IsValueType, "BBANDS_Value is a value type");
        Check(v.RealUpperBand >= v.RealMiddleBand && v.RealMiddleBand >= v.RealLowerBand,
              "the bands are ordered upper >= middle >= lower");

        // Deconstruction is part of the surface a record struct promises.
        (double up, double mid, double low) = s.Value;
        Check(Bits(up) == Bits(v.RealUpperBand)
              && Bits(mid) == Bits(v.RealMiddleBand)
              && Bits(low) == Bits(v.RealLowerBand),
              "BBANDS_Value deconstructs into its components");

        // Value is a read, not a recompute: it must not move without an Update.
        Core.BBANDS_Value again = s.Value;
        Check(Bits(again.RealUpperBand) == Bits(v.RealUpperBand), "Value is stable between Updates");
    }

    /// <summary>
    /// Every function the catalogue advertises as streaming really has the
    /// surface, and nothing else does.
    /// </summary>
    /// <remarks>The count is derived, never written down — see the class
    /// remarks.</remarks>
    private static void CatalogueAgreesWithTheEmittedSurface()
    {
        var advertised = FunctionCatalog.Default
            .Where(f => (f.Flags & FunctionFlags.Stream) != 0)
            .Select(f => f.Name)
            .OrderBy(n => n, StringComparer.Ordinal)
            .ToArray();

        var emitted = typeof(Core).GetNestedTypes(BindingFlags.Public)
            .Where(t => t.Name.EndsWith("_Stream", StringComparison.Ordinal))
            .Select(t => t.Name[..^"_Stream".Length])
            .OrderBy(n => n, StringComparer.Ordinal)
            .ToArray();

        Check(advertised.Length > 0, "the catalogue advertises at least one streaming function");
        Check(advertised.SequenceEqual(emitted),
              $"every advertised streaming function has a handle type "
              + $"(advertised {advertised.Length}, emitted {emitted.Length}; "
              + $"only-advertised: [{string.Join(",", advertised.Except(emitted))}], "
              + $"only-emitted: [{string.Join(",", emitted.Except(advertised))}])");

        // Each handle type must be sealed, expose no public constructor, and
        // carry the four members the docs promise.
        foreach (Type t in typeof(Core).GetNestedTypes(BindingFlags.Public)
                     .Where(t => t.Name.EndsWith("_Stream", StringComparison.Ordinal)))
        {
            if (!t.IsSealed || t.GetConstructors(BindingFlags.Public | BindingFlags.Instance).Length != 0)
            {
                Check(false, $"{t.Name} is sealed with no public constructor");
                return;
            }
            foreach (string member in new[] { "Update", "Peek", "Clone" })
            {
                if (t.GetMethod(member, BindingFlags.Public | BindingFlags.Instance) == null)
                {
                    Check(false, $"{t.Name} has a public {member}");
                    return;
                }
            }
            foreach (string prop in new[] { "Value", "FillRange" })
            {
                if (t.GetProperty(prop, BindingFlags.Public | BindingFlags.Instance) == null)
                {
                    Check(false, $"{t.Name} has a public {prop} property");
                    return;
                }
            }
            if (typeof(IDisposable).IsAssignableFrom(t))
            {
                Check(false, $"{t.Name} does not implement IDisposable (handles own only managed state)");
                return;
            }
        }
        Check(true, $"all {emitted.Length} handle types are sealed, constructor-free and complete");
    }

    public static int Run()
    {
        StreamMatchesBatch();
        PeekDoesNotCommit();
        CloneIsAnIndependentDeepCopy();
        OpenAndFillMatchesBatch();
        MisuseThrowsTheDocumentedException();
        OpenAndFillRejectsAliasing();
        NullArgumentsAreNamed();
        IntegerSentinelSelectsTheDocumentedDefault();
        SettingsAreCapturedFromTheOpeningCore();
        UpdateDoesNotAllocate();
        MultiOutputValueIsAStruct();
        CatalogueAgreesWithTheEmittedSurface();

        if (_failures == 0)
        {
            Console.WriteLine($"StreamApiTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"StreamApiTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
