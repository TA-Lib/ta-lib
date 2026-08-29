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
        Core.SmaStream s = core.SmaOpen(history, period);

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

        Core.SmaStream s = core.SmaOpen(closes[..(lookback + 1)], period);
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

        Core.SmaStream a = core.SmaOpen(closes[..(lookback + 1)], period);
        for (int t = lookback + 1; t < 60; t++)
        {
            a.Update(closes[t]);
        }

        Core.SmaStream b = a.Clone();
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
        Core.SmaStream fresh = core.SmaOpen(closes[..(lookback + 1)], period);
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
        Core.SmaStream s = core.SmaOpenAndFill(closes, period, filled);

        Check(s.OutRange.BegIdx == br.BegIdx && s.OutRange.Count == br.Count,
              "OutRange equals the batch OutRange");
        Check(!s.OutRange.IsEmpty, "a filled range is not empty");

        bool same = true;
        for (int i = 0; i < br.Count; i++)
        {
            if (Bits(filled[i]) != Bits(batch[i]))
            {
                same = false;
            }
        }
        Check(same, "the filled array is bit-identical to batch(0, n-1)");

        // A plain open fills nothing, but it warmed over the same bars — so it
        // reports the same range (issue #241). Nothing a stream can produce is
        // empty any more; a successful open writes at least one value.
        Core.SmaStream plain = core.SmaOpen(closes, period);
        Check(plain.OutRange.BegIdx == br.BegIdx && plain.OutRange.Count == br.Count,
              "a plain Open reports the range it warmed over");
        Check(Bits(plain.Value) == Bits(s.Value), "both openers agree on the current value");
    }

    /// <summary>#241: feed a stream N bars by any mixture of opener and updates
    /// and its <c>OutRange</c> is the batch range over those same N bars.</summary>
    private static void OutRangeTracksTheBatchRange()
    {
        var core = new Core();
        double[] closes = Closes(200);
        const int period = 20;
        int lb = core.SMA_Lookback(period);

        // Every warm-up length from the shortest legal one up, each brought to
        // the full series by Update: the range must not depend on where the
        // opener stopped and the updates took over.
        foreach (int warm in new[] { lb + 1, lb + 7, closes.Length / 2, closes.Length })
        {
            var batch = new double[closes.Length];
            OutRange br = core.SMA(0, closes.Length - 1, closes, period, batch);
            Core.SmaStream s = core.SmaOpen(closes[..warm], period);
            Check(s.OutRange.BegIdx == lb && s.OutRange.Count == warm - lb,
                  $"Open({warm}) reports (lookback, {warm} - lookback)");
            for (int t = warm; t < closes.Length; t++)
            {
                OutRange before = s.OutRange;
                s.Peek(closes[t]);
                Check(s.OutRange.Count == before.Count, "Peek does not commit a bar");
                s.Update(closes[t]);
                Check(s.OutRange.BegIdx == before.BegIdx && s.OutRange.Count == before.Count + 1,
                      "Update adds exactly one to Count and leaves BegIdx alone");
            }
            Check(s.OutRange.BegIdx == br.BegIdx && s.OutRange.Count == br.Count,
                  $"Open({warm}) + updates == the batch range");
        }

        // A history shorter than the bank's shared anchor is InsufficientHistory,
        // not a handle carrying a nonsense range (#241). Honest about scope:
        // MAVP rejects at its own-lookback precheck, which predates #241, so
        // this passes with or without the post-clamp re-check 96d1052f8 added —
        // measured. Outside Rust that re-check is unreachable from the public
        // API, and the anchored _OpenInternal seam cannot be driven out of its
        // startIdx <= endIdx contract safely. It is gated in the generator, by
        // identity_anchor_clamps_before_it_rechecks_in_every_backend. What this
        // asserts is the public contract around it.
        try
        {
            core.MavpOpen(closes[..10], closes[..10], 1, 30, MAType.SMA);
            Check(false, "MavpOpen on a history shorter than the bank's anchor must throw");
        }
        catch (InsufficientHistoryException)
        {
            Check(true, "MavpOpen rejects an anchor past the history");
        }
        // The positive half, so this is not a rejection sweep.
        {
            int mavpLb = core.MAVP_Lookback(1, 30, MAType.SMA);
            var px = closes[..(mavpLb + 3)];
            Core.MavpStream mv = core.MavpOpen(px, px, 1, 30, MAType.SMA);
            Check(mv.OutRange.BegIdx == mavpLb && mv.OutRange.Count == 3,
                  "MavpOpen just past its anchor reports (lookback, 3)");
        }

        // A clone forks: its own updates extend only itself.
        Core.SmaStream a = core.SmaOpen(closes, period);
        Core.SmaStream b = a.Clone();
        Check(b.OutRange.BegIdx == a.OutRange.BegIdx && b.OutRange.Count == a.OutRange.Count,
              "Clone carries the range verbatim");
        b.Update(closes[^1]);
        Check(b.OutRange.Count == a.OutRange.Count + 1, "the clone's update extends only the clone");
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
            () => core.SmaOpen(closes[..lookback], period),
            "history of exactly `lookback` bars throws InsufficientHistoryException");

        // ...and one more bar is enough.
        _checks++;
        try
        {
            core.SmaOpen(closes[..(lookback + 1)], period);
        }
        catch (Exception e)
        {
            _failures++;
            Console.WriteLine("  FAIL: lookback + 1 bars must open (threw " + e.GetType().Name + ")");
        }

        // The typed exception is catchable as ArgumentException too, which is
        // what lets a caller treat every open rejection uniformly if it wants.
        CheckThrows<ArgumentException>(
            () => core.SmaOpen(closes[..lookback], period),
            "InsufficientHistoryException is an ArgumentException");

        // A parameter outside its documented range is a plain ArgumentException.
        CheckThrows<ArgumentException>(
            () => core.SmaOpen(closes, 0),
            "an out-of-range period throws ArgumentException");

        // The message carries the stable "<NAME> open: " prefix.
        _checks++;
        try
        {
            core.SmaOpen(closes[..lookback], period);
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
            () => core.SmaOpenAndFill(closes, 14, closes),
            "output aliasing the input is rejected");

        // A distinct array of the same length is fine — so the rejection above
        // is about identity, not about size.
        _checks++;
        try
        {
            core.SmaOpenAndFill(closes, 14, new double[closes.Length]);
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

        // A span cannot be null, so a null array arrives as an empty one and
        // there is one condition here, not two: the history is empty, which is
        // rule S1 -- the implied startIdx of 0 names no bar. It answers B1's
        // code, because an opener is a batch call over [0, historyLen - 1].
        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SmaOpen(null!, 14),
            "SmaOpen(null) throws ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SmaOpen(Array.Empty<double>(), 14),
            "SmaOpen(empty) throws ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SmaOpenAndFill(null!, 14, outReal),
            "SmaOpenAndFill with a null input throws ArgumentOutOfRangeException");

        // The type is coarser than the code -- it serves both index rules -- so
        // the code is what pins WHICH one fired.
        CheckRetCode(() => core.SmaOpen(Array.Empty<double>(), 14),
                     RetCode.OutOfRangeStartIndex,
                     "SmaOpen(empty) carries OutOfRangeStartIndex");

        // It names the offending parameter — the core's own rejection travels
        // through a shared ladder that has no argument to name — and the index
        // pair is evaluated first: this call is also an absent output, and the
        // empty history is still what it reports.
        _checks++;
        try
        {
            core.SmaOpenAndFill(null!, 14, null!);
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
            if ((e as ITaLibFailure)?.RetCode != RetCode.OutOfRangeStartIndex)
            {
                _failures++;
                Console.WriteLine("  FAIL: a null output pre-empted the empty history");
            }
        }
    }

    /// <summary>The thrown object carries the code; the type alone cannot say
    /// which of two index rules fired.</summary>
    private static void CheckRetCode(Action body, RetCode expected, string what)
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
            RetCode? got = (e as ITaLibFailure)?.RetCode;
            if (got != expected)
            {
                _failures++;
                Console.WriteLine("  FAIL: " + what + " (carried " + got + ")");
            }
        }
    }


    /// <summary>Rule B6a at the opener: an empty span declines
    /// <c>outFAMA</c>, and declining changes nothing but the write.</summary>
    /// <remarks>Non-vacuous in three directions. The supplied run is the oracle,
    /// so a fill that stopped computing FAMA when it is declined — the easy way
    /// to "support" this — fails on <c>Value</c>, which the handle caches from
    /// the same expression the guarded store writes. The declined run must still
    /// reject an undersized <c>outMAMA</c>, so the conditional bound cannot have
    /// been dropped wholesale. And a SUPPLIED but undersized <c>outFAMA</c> is
    /// still rejected, so "declinable" did not become "unchecked".</remarks>
    private static void ADeclinedFillOutputIsStillComputed()
    {
        var core = new Core();
        double[] closes = Closes(252);
        int produced = closes.Length - core.MAMA_Lookback(0.5, 0.05);

        var refMama = new double[produced];
        var refFama = new double[produced];
        Core.MamaStream both = core.MamaOpenAndFill(closes, 0.5, 0.05, refMama, refFama);

        var soloMama = new double[produced];
        Core.MamaStream declined = core.MamaOpenAndFill(closes, 0.5, 0.05, soloMama, default);

        _b6a++;
        Check(refMama.AsSpan().SequenceEqual(soloMama),
            "declining outFAMA leaves outMAMA bit-identical");
        Check(both.OutRange.BegIdx == declined.OutRange.BegIdx
              && both.OutRange.Count == declined.OutRange.Count,
            "declining outFAMA leaves the reported range unchanged");
        _b6a++;
        Check(BitConverter.DoubleToInt64Bits(both.Value.FAMA)
              == BitConverter.DoubleToInt64Bits(declined.Value.FAMA),
            "a declined outFAMA is still computed: the handle reports it");
        _b6a++;
        Check(BitConverter.DoubleToInt64Bits(refFama[produced - 1])
              == BitConverter.DoubleToInt64Bits(declined.Value.FAMA),
            "and it is the value the supplied run wrote last");
        _b6a++;

        CheckThrows<ArgumentException>(
            () => core.MamaOpenAndFill(closes, 0.5, 0.05, new double[produced - 1], default),
            "an undersized outMAMA is still rejected when outFAMA is declined");
        _b6a++;
        CheckThrows<ArgumentException>(
            () => core.MamaOpenAndFill(closes, 0.5, 0.05, new double[produced],
                      new double[produced - 1]),
            "a supplied outFAMA is still bounded");
        _b6a++;
    }

    /// <summary>Rule U6a: <c>UpdateAndFill</c> declines a nullable output exactly
    /// as the opener does, and the choice is the CALL's — the four open/fill
    /// combinations are all accepted and all compute the same numbers.</summary>
    /// <remarks>Non-vacuous in the same directions as the opener's probe, plus the
    /// one this rule adds. The supplied run is the oracle, and the comparison a
    /// backend that stopped computing FAMA cannot satisfy is the handle's own
    /// <c>Value</c> after the fill, not the arrays. The mixed combinations are the
    /// point of the rule: declining at <c>OpenAndFill</c> and supplying here — and
    /// the reverse — must be as ordinary as either matching pair. A declining call
    /// must still bound <c>outMAMA</c>, and still bound <c>outFAMA</c> where it IS
    /// supplied.</remarks>
    private const double U6aCanary = -1.2345678901234e300;

    private static double[] CanaryFilled(int n)
    {
        var a = new double[n];
        Array.Fill(a, U6aCanary);
        return a;
    }

    private static bool WasWritten(double[] a)
    {
        foreach (double v in a) if (v == U6aCanary) return false;
        return true;
    }

    private static void ADeclinedOutputAtUpdateAndFillIsAPropertyOfTheCall()
    {
        var core = new Core();
        double[] closes = Closes(252);
        int produced = closes.Length - core.MAMA_Lookback(0.5, 0.05);
        var bars = new double[8];
        for (int i = 0; i < bars.Length; i++) bars[i] = closes[closes.Length - 1] + 1.0 + i * 0.25;

        Core.MamaStream Opened(bool declinedAtOpen) =>
            declinedAtOpen
                ? core.MamaOpenAndFill(closes, 0.5, 0.05, new double[produced], default)
                : core.MamaOpenAndFill(closes, 0.5, 0.05, new double[produced], new double[produced]);

        // The oracle: supplied at open, supplied here.
        Core.MamaStream oracle = Opened(false);
        // Canary-filled, not zero-filled: comparing two arrays the fill never
        // wrote would otherwise pass on their shared initial value, which is
        // exactly the break the supplied/supplied leg below is meant to catch.
        double[] refMama = CanaryFilled(bars.Length);
        double[] refFama = CanaryFilled(bars.Length);
        oracle.UpdateAndFill(bars, refMama, refFama);
        _u6a++;
        Check(WasWritten(refMama) && WasWritten(refFama), "the oracle fill wrote both outputs");
        double oracleFama = oracle.Value.FAMA;
        double oracleMama = oracle.Value.MAMA;

        foreach (bool declinedAtOpen in new[] { false, true })
        {
            string what = declinedAtOpen ? "declined at open" : "supplied at open";

            Core.MamaStream h = Opened(declinedAtOpen);
            double[] mama = CanaryFilled(bars.Length);
            h.UpdateAndFill(bars, mama, default);
            _u6a++;
            Check(WasWritten(mama) && refMama.AsSpan().SequenceEqual(mama),
                what + ", declined here: outMAMA");
            _u6a++;
            Check(h.OutRange.BegIdx == oracle.OutRange.BegIdx
                  && h.OutRange.Count == oracle.OutRange.Count,
                what + ", declined here: the range");
            // The state, not the write: FAMA feeds the next bar.
            _u6a++;
            Check(BitConverter.DoubleToInt64Bits(h.Value.FAMA)
                  == BitConverter.DoubleToInt64Bits(oracleFama),
                what + ", declined here: a declined outFAMA is still computed");
            _u6a++;
            Check(BitConverter.DoubleToInt64Bits(h.Value.MAMA)
                  == BitConverter.DoubleToInt64Bits(oracleMama),
                what + ", declined here: the handle's outMAMA");

            Core.MamaStream h2 = Opened(declinedAtOpen);
            double[] mama2 = CanaryFilled(bars.Length);
            double[] fama2 = CanaryFilled(bars.Length);
            h2.UpdateAndFill(bars, mama2, fama2);
            _u6a++;
            Check(WasWritten(mama2) && WasWritten(fama2)
                  && refMama.AsSpan().SequenceEqual(mama2) && refFama.AsSpan().SequenceEqual(fama2),
                what + ", supplied here: both outputs");
        }

        // "May differ again on the NEXT call" — the sentence the whole rule rests
        // on. One handle, three fills, alternating; each has to agree with an
        // oracle driven the same way with everything supplied.
        Core.MamaStream alt = Opened(false);
        Core.MamaStream altRef = Opened(false);
        bool[] plan = { true, false, true };
        for (int k = 0; k < plan.Length; k++)
        {
            var leg = new double[bars.Length];
            for (int i = 0; i < bars.Length; i++) leg[i] = bars[i] + k;
            double[] wantM = CanaryFilled(leg.Length);
            double[] wantF = CanaryFilled(leg.Length);
            altRef.UpdateAndFill(leg, wantM, wantF);
            double[] gotM = CanaryFilled(leg.Length);
            double[] gotF = CanaryFilled(leg.Length);
            if (plan[k])
            {
                alt.UpdateAndFill(leg, gotM, default);
            }
            else
            {
                alt.UpdateAndFill(leg, gotM, gotF);
                _u6a++;
                Check(WasWritten(gotF) && wantF.AsSpan().SequenceEqual(gotF),
                    $"alternating leg {k}: outFAMA");
            }
            _u6a++;
            Check(WasWritten(gotM) && wantM.AsSpan().SequenceEqual(gotM)
                  && alt.OutRange.Count == altRef.OutRange.Count,
                $"alternating leg {k}: outMAMA and the range");
        }
        _u6a++;
        Check(BitConverter.DoubleToInt64Bits(alt.Value.FAMA)
              == BitConverter.DoubleToInt64Bits(altRef.Value.FAMA),
            "alternating the declined set left the handle's FAMA identical");

        // Declining one output disarms neither the other's bound nor its own
        // where it IS supplied, and a rejected fill commits nothing.
        Core.MamaStream guarded = Opened(false);
        int before = guarded.OutRange.Count;
        CheckThrows<ArgumentException>(
            () => guarded.UpdateAndFill(bars, new double[bars.Length - 1], default),
            "an undersized outMAMA is still rejected when outFAMA is declined");
        _u6a++;
        CheckThrows<ArgumentException>(
            () => guarded.UpdateAndFill(bars, new double[bars.Length], new double[bars.Length - 1]),
            "a supplied outFAMA is still bounded");
        _u6a++;
        Check(guarded.OutRange.Count == before, "a rejected fill commits nothing");
        _u6a++;
    }

    /// <summary>Rule S5, from both sides. The bound is
    /// <c>historyLen - lookback</c> — the count the fill actually writes, not
    /// the width of the history — so an exactly-sized output has to be ACCEPTED
    /// and one element shorter REJECTED. Only the pair pins the arithmetic: a
    /// bound of <c>historyLen</c> would reject the first, and no bound at all
    /// would accept the second.</summary>
    /// <remarks>An undersized output used to fault inside the fill with an
    /// <c>IndexOutOfRangeException</c>, the buffer already partly written.</remarks>
    private static void TheFillOutputBoundFromBothSides()
    {
        var core = new Core();
        double[] closes = Closes(252);
        int lookback = core.SMA_Lookback(30);
        int produced = closes.Length - lookback;

        Check(lookback == 29, "the probe needs a lookback it can be one short of");
        Check(produced < closes.Length, "the produced count is shorter than the history");

        var exact = new double[produced];
        Core.SmaStream h = core.SmaOpenAndFill(closes, 30, exact);
        Check(h.OutRange.BegIdx == lookback, "the fill starts at the lookback");
        Check(h.OutRange.Count == produced, "the fill wrote exactly the bound");

        CheckThrows<ArgumentException>(
            () => core.SmaOpenAndFill(closes, 30, new double[produced - 1]),
            "one element short of the produced count throws ArgumentException");
        CheckRetCode(() => core.SmaOpenAndFill(closes, 30, new double[produced - 1]),
                     RetCode.BadParam,
                     "an undersized fill output carries BadParam");
        _s5++;

        // A rejected fill writes nothing: the check is ahead of the numerics,
        // not a report from inside them.
        var shortOut = new double[produced - 1];
        Array.Fill(shortOut, -3e37);
        try
        {
            core.SmaOpenAndFill(closes, 30, shortOut);
            Check(false, "expected the undersized fill to be rejected");
        }
        catch (ArgumentException)
        {
            bool untouched = true;
            foreach (double v in shortOut)
            {
                if (v != -3e37) { untouched = false; }
            }
            Check(untouched, "a rejected fill leaves the output as it found it");
        }

        // An oversized output is legal and bit-identical: the bound is a
        // minimum, which is what says the exact case above was not luck.
        var roomy = new double[closes.Length];
        core.SmaOpenAndFill(closes, 30, roomy);
        bool same = true;
        for (int i = 0; i < produced; i++)
        {
            if (BitConverter.DoubleToInt64Bits(exact[i]) != BitConverter.DoubleToInt64Bits(roomy[i]))
            {
                same = false;
            }
        }
        Check(same, "the exactly-sized fill is bit-identical to the roomy one");
    }

    /// <summary>The same bound on the tiers that hand-roll their own fill: the
    /// dispatch tier (including its identity arm, whose lookback is 0), the
    /// period bank, and a composed multi-output whose output spans double as its
    /// sub-calls' scratch — each output bounded separately.</summary>
    private static void TheFillOutputBoundHoldsOnEveryTier()
    {
        var core = new Core();
        double[] closes = Closes(252);
        var periods = new double[252];
        Array.Fill(periods, 5.0);

        foreach (int period in new[] { 30, 1 })
        {
            int lb = core.MA_Lookback(period, MAType.EMA);
            int produced = closes.Length - lb;
            core.MaOpenAndFill(closes, period, MAType.EMA, new double[produced]);
            CheckThrows<ArgumentException>(
                () => core.MaOpenAndFill(closes, period, MAType.EMA, new double[produced - 1]),
                "MA one short of the bound");
            _s5++;
        }

        int mavpLb = core.MAVP_Lookback(2, 30, MAType.SMA);
        int mavpProduced = closes.Length - mavpLb;
        core.MavpOpenAndFill(closes, periods, 2, 30, MAType.SMA, new double[mavpProduced]);
        CheckThrows<ArgumentException>(
            () => core.MavpOpenAndFill(closes, periods, 2, 30, MAType.SMA,
                new double[mavpProduced - 1]),
            "MAVP one short of the bound");
        _s5++;

        int bbLb = core.BBANDS_Lookback(20, 2.0, 2.0, MAType.SMA);
        int bbProduced = closes.Length - bbLb;
        core.BbandsOpenAndFill(closes, 20, 2.0, 2.0, MAType.SMA,
            new double[bbProduced], new double[bbProduced], new double[bbProduced]);
        CheckThrows<ArgumentException>(
            () => core.BbandsOpenAndFill(closes, 20, 2.0, 2.0, MAType.SMA,
                new double[bbProduced], new double[bbProduced], new double[bbProduced - 1]),
            "each output is bounded separately");
        _s5++;

        // A history too short to produce anything is still S7, whatever the
        // output holds: the bound floors at zero rather than going negative.
        CheckThrows<InsufficientHistoryException>(
            () => core.SmaOpenAndFill(closes.AsSpan(0, 29), 30, Span<double>.Empty),
            "a short history reaches the warm-up check, not the capacity one");

        // The floors for this method and the declined-output one live in Run(),
        // where a deleted CALL cannot take its own floor with it.
    }

    /// <summary>int.MinValue selects the documented default, as in batch.</summary>
    private static void IntegerSentinelSelectsTheDocumentedDefault()
    {
        var core = new Core();
        double[] closes = Closes(200);

        Core.SmaStream sentinel = core.SmaOpen(closes, int.MinValue);
        Core.SmaStream explicitly = core.SmaOpen(closes, 30);   // SMA's documented default
        Check(Bits(sentinel.Value) == Bits(explicitly.Value),
              "SmaOpen(int.MinValue) equals SmaOpen(30) bitwise");
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

        Core.CdldojiStream a = plain.CdldojiOpen(open, high, low, closes);
        Core.CdldojiStream b = tuned.CdldojiOpen(open, high, low, closes);

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
        Core.SmaStream s = core.SmaOpen(closes[..40], 30);

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

        Core.BbandsStream s = core.BbandsOpen(closes, 20, 2.0, 2.0, MAType.SMA);
        Core.BbandsValue v = s.Value;

        Check(typeof(Core.BbandsValue).IsValueType, "BbandsValue is a value type");
        Check(v.RealUpperBand >= v.RealMiddleBand && v.RealMiddleBand >= v.RealLowerBand,
              "the bands are ordered upper >= middle >= lower");

        // Deconstruction is part of the surface a record struct promises.
        (double up, double mid, double low) = s.Value;
        Check(Bits(up) == Bits(v.RealUpperBand)
              && Bits(mid) == Bits(v.RealMiddleBand)
              && Bits(low) == Bits(v.RealLowerBand),
              "BbandsValue deconstructs into its components");

        // Value is a read, not a recompute: it must not move without an Update.
        Core.BbandsValue again = s.Value;
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

        // The catalogue keeps the canonical (batch) name, e.g. "HT_TRENDLINE";
        // the handle type is PascalCase with the acronym single-capitalized and
        // no underscore, e.g. "HtTrendlineStream" (issue #278). Re-derive that
        // spelling from the canonical name rather than hard-coding it, so this
        // stays a real cross-check rather than a second copy of the mapping.
        static string ToPascal(string screaming) =>
            string.Concat(screaming.Split('_')
                .Select(w => char.ToUpperInvariant(w[0]) + w[1..].ToLowerInvariant()));

        var advertisedAsHandleNames = advertised
            .Select(ToPascal)
            .OrderBy(n => n, StringComparer.Ordinal)
            .ToArray();

        var emitted = typeof(Core).GetNestedTypes(BindingFlags.Public)
            .Where(t => t.Name.EndsWith("Stream", StringComparison.Ordinal))
            .Select(t => t.Name[..^"Stream".Length])
            .OrderBy(n => n, StringComparer.Ordinal)
            .ToArray();

        Check(advertised.Length > 0, "the catalogue advertises at least one streaming function");
        Check(advertisedAsHandleNames.SequenceEqual(emitted),
              $"every advertised streaming function has a handle type "
              + $"(advertised {advertised.Length}, emitted {emitted.Length}; "
              + $"only-advertised: [{string.Join(",", advertisedAsHandleNames.Except(emitted))}], "
              + $"only-emitted: [{string.Join(",", emitted.Except(advertisedAsHandleNames))}])");

        // Each handle type must be sealed, expose no public constructor, and
        // carry the four members the docs promise.
        foreach (Type t in typeof(Core).GetNestedTypes(BindingFlags.Public)
                     .Where(t => t.Name.EndsWith("Stream", StringComparison.Ordinal)))
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
            foreach (string prop in new[] { "Value", "OutRange" })
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

    /* Non-finite counters, incremented AT the assertion rather than derived
       from the loop, so deleting the assertions inside shows up here. */
    /* Rule S5's own counter and floor: sharing the non-finite ones would let a
       deleted capacity case hide behind a bar-rejection one. */
    private static int _s5;
    /* Rule B6a at the opener — a declined output, counted apart from S5's. */
    private static int _b6a;
    /* Rule U6a at UpdateAndFill — counted apart from the opener's. */
    private static int _u6a;

    private static int _nfOpen;
    private static int _nfBar;
    private static int _nfState;

    /* The MESSAGE is checked, not just the type. InsufficientHistoryException
       derives from ArgumentException, so asserting the base type alone would let
       "rejected because the history was too short" pass as "rejected the
       non-finite value" -- and every probe here deliberately supplies enough
       history, so the confusion would surface only the day a lookback grew. */
    private static void ThrowsBadParam(string what, Action body)
    {
        _checks++;
        try
        {
            body();
            _failures++;
            Console.WriteLine("  FAIL: " + what + " (no exception thrown)");
        }
        catch (ArgumentException e) when (e.Message.EndsWith(": BadParam", StringComparison.Ordinal))
        {
            /* expected */
        }
        catch (Exception e)
        {
            _failures++;
            Console.WriteLine("  FAIL: " + what + " (threw " + e.GetType().FullName + ": " + e.Message + ")");
        }
    }

    private static void OpenMustReject(string what, Action body)
    {
        ThrowsBadParam(what + ": open must reject a non-finite parameter", body);
        _nfOpen++;
    }

    private static void BarMustReject(string what, Action body)
    {
        ThrowsBadParam(what + ": Update/Peek must reject a non-finite bar", body);
        _nfBar++;
    }

    private static void StateMustHold(string what, double a, double b)
    {
        Check(Bits(a) == Bits(b), what + ": a rejected bar must not move the handle");
        _nfState++;
    }

    /// <summary>Non-finite rejection is a property of SINGLE VALUES, never of
    /// arrays.</summary>
    /// <remarks>
    /// <para>A bar handed to <c>Update</c>/<c>Peek</c> is checked, and so is a real
    /// optional parameter: both are one comparison, and a handle RETAINS state, so
    /// a single NaN bar would poison every later value it produces long after the
    /// feed recovers. Rejecting it and leaving the handle untouched is strictly
    /// more useful.</para>
    /// <para>The warm-up history handed to <c>Open</c>/<c>OpenAndFill</c> is
    /// deliberately NOT checked: it is an input array, and the library does not
    /// scan input arrays. Passing a non-finite one is undefined behaviour — see
    /// <c>docs/error-handling-spec.md</c>, "Non-finite input".</para>
    /// <para>Coverage is by stream TIER, not by function count. The check is emitted
    /// from one place, but into the entry points of five different tiers: SMA is
    /// the loop tier, MINUS_DI dual-mode, MA the dispatch tier (including its
    /// identity arm, which never reaches a sub-stream at all), MAVP the
    /// period-bank tier, BBANDS and STOCH composed. CDLDOJI adds an integer output
    /// over four price inputs.</para>
    /// <para>The handle-unchanged property is verified against a control stream, not
    /// by inspection: a rejection that half-advanced the state would pass every
    /// "it threw" assertion and fail only here.</para>
    /// </remarks>
    private static void NonFiniteInputsAreRejected()
    {
        var core = new Core();
        const int warm = 60;
        double[] closes = Closes(warm + 2);
        double[] highs = new double[warm + 2];
        double[] lows = new double[warm + 2];
        double[] opens = new double[warm + 2];
        double[] periods = new double[warm + 2];
        for (int i = 0; i < closes.Length; i++)
        {
            highs[i] = closes[i] + 1.5;
            lows[i] = closes[i] - 1.5;
            opens[i] = closes[i] - 0.4;
            periods[i] = 5.0 + (i % 11);
        }

        double[] bad = { double.NaN, double.PositiveInfinity, double.NegativeInfinity };

        foreach (double v in bad)
        {
            /* --- Update / Peek, and the handle-unchanged property. --------- */
            /* Clean warm-up series: nothing here poisons an input ARRAY, which
               the library does not scan. Only the per-bar values
               below carry a non-finite value. */
            var c = closes[..warm].ToArray();
            var h = highs[..warm].ToArray();
            var l = lows[..warm].ToArray();
            var o = opens[..warm].ToArray();
            var p = periods[..warm].ToArray();

            var sa = core.SmaOpen(c, 14);
            var sb = core.SmaOpen(c, 14);
            BarMustReject("SMA.Update", () => sa.Update(v));
            BarMustReject("SMA.Peek", () => sa.Peek(v));
            StateMustHold("SMA", sa.Update(closes[warm]), sb.Update(closes[warm]));

            var da = core.MinusDiOpen(h, l, c, 14);
            var db = core.MinusDiOpen(h, l, c, 14);
            BarMustReject("MINUS_DI.Update(high)", () => da.Update(v, lows[warm], closes[warm]));
            BarMustReject("MINUS_DI.Update(low)", () => da.Update(highs[warm], v, closes[warm]));
            BarMustReject("MINUS_DI.Update(close)", () => da.Update(highs[warm], lows[warm], v));
            BarMustReject("MINUS_DI.Peek", () => da.Peek(v, lows[warm], closes[warm]));
            StateMustHold("MINUS_DI",
                da.Update(highs[warm], lows[warm], closes[warm]),
                db.Update(highs[warm], lows[warm], closes[warm]));

            var ma = core.MaOpen(c, 14, MAType.EMA);
            var mb = core.MaOpen(c, 14, MAType.EMA);
            BarMustReject("MA.Update", () => ma.Update(v));
            BarMustReject("MA.Peek", () => ma.Peek(v));
            StateMustHold("MA", ma.Update(closes[warm]), mb.Update(closes[warm]));

            /* Period 1 is the dispatch identity arm: it copies the bar to the
               output and never reaches a sub-stream, so a check delegated to the
               sub would miss it. */
            var mi = core.MaOpen(c, 1, MAType.SMA);
            BarMustReject("MA(identity).Update", () => mi.Update(v));
            BarMustReject("MA(identity).Peek", () => mi.Peek(v));

            var va = core.MavpOpen(c, p, 2, 30, MAType.SMA);
            var vb = core.MavpOpen(c, p, 2, 30, MAType.SMA);
            BarMustReject("MAVP.Update(real)", () => va.Update(v, p[0]));
            BarMustReject("MAVP.Update(period)", () => va.Update(closes[warm], v));
            BarMustReject("MAVP.Peek(period)", () => va.Peek(closes[warm], v));
            StateMustHold("MAVP", va.Update(closes[warm], p[0]), vb.Update(closes[warm], p[0]));

            var ba = core.BbandsOpen(c, 20, 2.0, 2.0, MAType.SMA);
            var bb = core.BbandsOpen(c, 20, 2.0, 2.0, MAType.SMA);
            BarMustReject("BBANDS.Update", () => ba.Update(v));
            BarMustReject("BBANDS.Peek", () => ba.Peek(v));
            var bav = ba.Update(closes[warm]);
            var bbv = bb.Update(closes[warm]);
            StateMustHold("BBANDS.upper", bav.RealUpperBand, bbv.RealUpperBand);
            StateMustHold("BBANDS.lower", bav.RealLowerBand, bbv.RealLowerBand);

            var ka = core.StochOpen(h, l, c, 5, 3, MAType.SMA, 3, MAType.SMA);
            var kb = core.StochOpen(h, l, c, 5, 3, MAType.SMA, 3, MAType.SMA);
            BarMustReject("STOCH.Update", () => ka.Update(v, lows[warm], closes[warm]));
            BarMustReject("STOCH.Peek", () => ka.Peek(highs[warm], v, closes[warm]));
            var kav = ka.Update(highs[warm], lows[warm], closes[warm]);
            var kbv = kb.Update(highs[warm], lows[warm], closes[warm]);
            StateMustHold("STOCH.slowK", kav.SlowK, kbv.SlowK);
            StateMustHold("STOCH.slowD", kav.SlowD, kbv.SlowD);

            var ja = core.CdldojiOpen(o, h, l, c);
            var jb = core.CdldojiOpen(o, h, l, c);
            BarMustReject("CDLDOJI.Update(open)",
                () => ja.Update(v, highs[warm], lows[warm], closes[warm]));
            BarMustReject("CDLDOJI.Peek(close)",
                () => ja.Peek(opens[warm], highs[warm], lows[warm], v));
            Check(ja.Update(opens[warm], highs[warm], lows[warm], closes[warm])
                    == jb.Update(opens[warm], highs[warm], lows[warm], closes[warm]),
                  "CDLDOJI: a rejected bar must not move the handle");
            _nfState++;
        }

        /* A NaN real PARAMETER. Not redundant with the range check: `x < min` and
           `x > max` are both false for NaN, so a plain range test admits it —
           which is why the streaming tier spells the same two comparisons
           inverted. An infinity is already outside every declared bound. */
        double[] cp = closes[..warm].ToArray();
        OpenMustReject("BBANDS(nbDevUp=NaN)",
            () => core.BbandsOpen(cp, 20, double.NaN, 2.0, MAType.SMA));
        OpenMustReject("BBANDS(nbDevDn=NaN)",
            () => core.BbandsOpen(cp, 20, 2.0, double.NaN, MAType.SMA));

        /* Non-vacuity. Literal floors: one derived from the loop above moves with
           it and would let the assertions inside be deleted. */
        Check(_nfOpen >= 2 && _nfBar >= 57 && _nfState >= 27,
              $"the non-finite gate ran fewer checks than it was written with "
              + $"({_nfOpen}/{_nfBar}/{_nfState})");
    }

    private static int _ufCommits;
    private static int _ufValues;
    private static int _ufSlots;

    private const int UfN = 6;
    private const int UfBad = 3;
    private const double UfCanary = -1.2345678901234e300;
    private const int UfCanaryI = -987654321;

    private static void UfRangeEq(string what, OutRange a, OutRange b)
    {
        Check(a.BegIdx == b.BegIdx && a.Count == b.Count,
              $"{what}: UpdateAndFill committed ({a.BegIdx},{a.Count}), "
              + $"{UfBad} Updates committed ({b.BegIdx},{b.Count})");
        _ufCommits++;
    }

    private static void UfValueEq(string what, double a, double b)
    {
        Check(Bits(a) == Bits(b), $"{what}: UpdateAndFill wrote {a} where Update returned {b}");
        _ufValues++;
    }

    private static void UfUntouched(string what, double x)
    {
        Check(Bits(x) == Bits(UfCanary), $"{what}: UpdateAndFill wrote past the bar it rejected");
        _ufSlots++;
    }

    /// <summary><c>UpdateAndFill</c> commits the bars before the one it rejects.</summary>
    /// <remarks>
    /// <para><c>UpdateAndFill</c> is <c>n</c> back-to-back <see cref="object"/>
    /// <c>Update</c> calls and nothing else, so a non-finite bar <c>k</c> throws
    /// exactly as <c>Update</c> would — and the bars before it stay committed with
    /// their values written. That is the one place in the API where a call fails
    /// AND leaves output behind, so what it leaves is pinned against a CONTROL
    /// handle driven over the same first <c>k</c> bars one at a time: same
    /// <c>OutRange</c>, same values, same answer on the next good bar, and nothing
    /// written at or above <c>k</c>. A whole-array pre-scan would satisfy "it
    /// throws" and fail every one of those.</para>
    /// <para>Coverage is by the emitter each tier's <c>UpdateAndFill</c> comes
    /// from: SMA stands for the whole step-loop family, BBANDS adds three outputs,
    /// MA both dispatch arms, MAVP the period bank and CDLDOJI an integer output
    /// over four inputs.</para>
    /// </remarks>
    private static void UpdateAndFillCommitsThePrefix()
    {
        var core = new Core();
        const int warm = 60;
        double[] closes = Closes(warm + UfN + 1);
        double[] highs = new double[closes.Length];
        double[] lows = new double[closes.Length];
        double[] opens = new double[closes.Length];
        double[] periods = new double[closes.Length];
        for (int i = 0; i < closes.Length; i++)
        {
            highs[i] = closes[i] + 1.5;
            lows[i] = closes[i] - 1.5;
            opens[i] = closes[i] - 0.4;
            periods[i] = 5.0 + (i % 11);
        }
        var c = closes[..warm].ToArray();
        var h = highs[..warm].ToArray();
        var l = lows[..warm].ToArray();
        var o = opens[..warm].ToArray();
        var pp = periods[..warm].ToArray();

        double[] bad = { double.NaN, double.PositiveInfinity, double.NegativeInfinity };

        foreach (double v in bad)
        {
            double[] bars = new double[UfN];
            double[] goodBars = new double[UfN];
            for (int i = 0; i < UfN; i++)
            {
                bars[i] = closes[warm + i];
                goodBars[i] = closes[warm + i];
            }
            bars[UfBad] = v;

            /* --- the shared step loop --------------------------------------- */
            var sa = core.SmaOpen(c, 14);
            var sb = core.SmaOpen(c, 14);
            double[] want = new double[UfBad];
            for (int i = 0; i < UfBad; i++)
            {
                want[i] = sb.Update(bars[i]);
            }
            double[] outp = new double[UfN];
            Array.Fill(outp, UfCanary);
            BarMustReject("SMA.UpdateAndFill", () => sa.UpdateAndFill(bars, outp));
            UfRangeEq("SMA", sa.OutRange, sb.OutRange);
            for (int i = 0; i < UfBad; i++)
            {
                UfValueEq("SMA", outp[i], want[i]);
            }
            for (int i = UfBad; i < UfN; i++)
            {
                UfUntouched("SMA", outp[i]);
            }
            StateMustHold("SMA(UpdateAndFill)",
                sa.Update(closes[warm + UfN]), sb.Update(closes[warm + UfN]));

            /* --- composed, three outputs ------------------------------------ */
            var ba = core.BbandsOpen(c, 20, 2.0, 2.0, MAType.SMA);
            var bb = core.BbandsOpen(c, 20, 2.0, 2.0, MAType.SMA);
            var wantB = new (double U, double M, double L)[UfBad];
            for (int i = 0; i < UfBad; i++)
            {
                var w = bb.Update(bars[i]);
                wantB[i] = (w.RealUpperBand, w.RealMiddleBand, w.RealLowerBand);
            }
            double[] bu = new double[UfN];
            double[] bm = new double[UfN];
            double[] bl = new double[UfN];
            Array.Fill(bu, UfCanary);
            Array.Fill(bm, UfCanary);
            Array.Fill(bl, UfCanary);
            BarMustReject("BBANDS.UpdateAndFill", () => ba.UpdateAndFill(bars, bu, bm, bl));
            UfRangeEq("BBANDS", ba.OutRange, bb.OutRange);
            for (int i = 0; i < UfBad; i++)
            {
                UfValueEq("BBANDS.upper", bu[i], wantB[i].U);
                UfValueEq("BBANDS.middle", bm[i], wantB[i].M);
                UfValueEq("BBANDS.lower", bl[i], wantB[i].L);
            }
            for (int i = UfBad; i < UfN; i++)
            {
                UfUntouched("BBANDS.upper", bu[i]);
                UfUntouched("BBANDS.middle", bm[i]);
                UfUntouched("BBANDS.lower", bl[i]);
            }
            /* Value is built fresh from the handle's fields here (a record
               struct, no cache), so it names the last committed bar for free —
               asserted anyway, because Java's does need a refresh and the two
               surfaces are meant to agree. */
            Check(Bits(ba.Value.RealUpperBand) == Bits(wantB[UfBad - 1].U),
                  "BBANDS: Value must name the last committed bar after a partial fill");
            _ufValues++;

            /* --- dispatch, both arms (period 1 is the identity loop) --------- */
            foreach (int period in new[] { 1, 14 })
            {
                var ma = core.MaOpen(c, period, MAType.SMA);
                var mb = core.MaOpen(c, period, MAType.SMA);
                double[] wantM = new double[UfBad];
                for (int i = 0; i < UfBad; i++)
                {
                    wantM[i] = mb.Update(bars[i]);
                }
                double[] mo = new double[UfN];
                Array.Fill(mo, UfCanary);
                BarMustReject($"MA({period}).UpdateAndFill", () => ma.UpdateAndFill(bars, mo));
                UfRangeEq($"MA({period})", ma.OutRange, mb.OutRange);
                for (int i = 0; i < UfBad; i++)
                {
                    UfValueEq("MA", mo[i], wantM[i]);
                }
                for (int i = UfBad; i < UfN; i++)
                {
                    UfUntouched("MA", mo[i]);
                }
            }

            /* --- period bank: poison the PERIOD series, the input that reaches
               an (int) cast ------------------------------------------------- */
            double[] pers = new double[UfN];
            for (int i = 0; i < UfN; i++)
            {
                pers[i] = 2.0 + (i % 8);
            }
            pers[UfBad] = v;
            var va = core.MavpOpen(c, pp, 2, 30, MAType.SMA);
            var vb = core.MavpOpen(c, pp, 2, 30, MAType.SMA);
            double[] wantV = new double[UfBad];
            for (int i = 0; i < UfBad; i++)
            {
                wantV[i] = vb.Update(goodBars[i], pers[i]);
            }
            double[] vo = new double[UfN];
            Array.Fill(vo, UfCanary);
            BarMustReject("MAVP.UpdateAndFill", () => va.UpdateAndFill(goodBars, pers, vo));
            UfRangeEq("MAVP", va.OutRange, vb.OutRange);
            for (int i = 0; i < UfBad; i++)
            {
                UfValueEq("MAVP", vo[i], wantV[i]);
            }
            for (int i = UfBad; i < UfN; i++)
            {
                UfUntouched("MAVP", vo[i]);
            }

            /* --- integer output, four inputs; poison the LOW ----------------- */
            double[] os = new double[UfN];
            double[] hs = new double[UfN];
            double[] ls = new double[UfN];
            for (int i = 0; i < UfN; i++)
            {
                os[i] = opens[warm + i];
                hs[i] = highs[warm + i];
                ls[i] = lows[warm + i];
            }
            ls[UfBad] = v;
            var ja = core.CdldojiOpen(o, h, l, c);
            var jb = core.CdldojiOpen(o, h, l, c);
            int[] wantJ = new int[UfBad];
            for (int i = 0; i < UfBad; i++)
            {
                wantJ[i] = jb.Update(os[i], hs[i], ls[i], goodBars[i]);
            }
            int[] jo = new int[UfN];
            Array.Fill(jo, UfCanaryI);
            BarMustReject("CDLDOJI.UpdateAndFill",
                () => ja.UpdateAndFill(os, hs, ls, goodBars, jo));
            UfRangeEq("CDLDOJI", ja.OutRange, jb.OutRange);
            for (int i = 0; i < UfBad; i++)
            {
                Check(jo[i] == wantJ[i],
                      $"CDLDOJI: UpdateAndFill wrote {jo[i]} where Update returned {wantJ[i]}");
                _ufValues++;
            }
            for (int i = UfBad; i < UfN; i++)
            {
                Check(jo[i] == UfCanaryI, "CDLDOJI: UpdateAndFill wrote past the rejected bar");
                _ufSlots++;
            }
        }

        /* The rejections spans make visible: a short output, and an output that
           OVERLAPS an input — `Span.Overlaps` sees the partially-shifted case
           Java's reference equality cannot. Plus the zero-bar call, a success
           that changes nothing. */
        var s2 = core.SmaOpen(c, 14);
        OutRange before = s2.OutRange;
        double[] tail = new double[UfN];
        for (int i = 0; i < UfN; i++)
        {
            tail[i] = closes[warm + i];
        }
        double[] o2 = new double[UfN];
        Array.Fill(o2, UfCanary);
        s2.UpdateAndFill(ReadOnlySpan<double>.Empty, o2);
        Check(before.BegIdx == s2.OutRange.BegIdx && before.Count == s2.OutRange.Count,
              "a zero-bar UpdateAndFill must not move the handle");
        _ufCommits++;
        UfUntouched("SMA(zero bars)", o2[0]);
        BarMustReject("SMA.UpdateAndFill(short output)",
            () => s2.UpdateAndFill(tail, new double[UfN - 1]));
        /* Overlap, NOT a length mistake: both spans are exactly barCount long
           and sit one element apart in the same array, so the only condition
           that can reject is `Span.Overlaps` — the partially-shifted case
           Java's reference equality cannot see. */
        double[] wide = new double[UfN + 1];
        for (int i = 0; i < UfN + 1; i++)
        {
            wide[i] = closes[warm + i];
        }
        BarMustReject("SMA.UpdateAndFill(output overlaps input)",
            () => s2.UpdateAndFill(wide.AsSpan(0, UfN), wide.AsSpan(1, UfN)));
        Check(before.BegIdx == s2.OutRange.BegIdx && before.Count == s2.OutRange.Count,
              "a rejected UpdateAndFill must not move the handle");
        _ufCommits++;
        /* Control: the same call, correctly sized and disjoint, succeeds and
           advances by exactly the bars it was handed. */
        s2.UpdateAndFill(tail, o2);
        Check(s2.OutRange.Count == before.Count + UfN,
              "UpdateAndFill must advance by every bar it commits");
        _ufCommits++;

        /* Non-vacuity. Literal floors, every counter incremented at its
           assertion. */
        Check(_ufCommits >= 21 && _ufValues >= 75 && _ufSlots >= 73,
              $"the UpdateAndFill gate ran fewer checks than it was written with "
              + $"({_ufCommits}/{_ufValues}/{_ufSlots})");
    }

    public static int Run()
    {
        StreamMatchesBatch();
        PeekDoesNotCommit();
        CloneIsAnIndependentDeepCopy();
        OpenAndFillMatchesBatch();
        OutRangeTracksTheBatchRange();
        MisuseThrowsTheDocumentedException();
        OpenAndFillRejectsAliasing();
        NullArgumentsAreNamed();
        TheFillOutputBoundFromBothSides();
        ADeclinedFillOutputIsStillComputed();
        ADeclinedOutputAtUpdateAndFillIsAPropertyOfTheCall();
        TheFillOutputBoundHoldsOnEveryTier();
        // Literal floors, HERE and not inside the methods above: a floor that
        // rides its own method is deleted along with the call it guards.
        Check(_s5 >= 5, $"the fill-capacity gate ran fewer checks than it was written with ({_s5})");
        Check(_b6a >= 6, $"the declined-output gate ran fewer checks than it was written with ({_b6a})");
        Check(_u6a >= 19, $"the declined-at-updateAndFill gate ran fewer checks than it was written with ({_u6a})");
        IntegerSentinelSelectsTheDocumentedDefault();
        SettingsAreCapturedFromTheOpeningCore();
        UpdateDoesNotAllocate();
        MultiOutputValueIsAStruct();
        CatalogueAgreesWithTheEmittedSurface();
        NonFiniteInputsAreRejected();
        UpdateAndFillCommitsThePrefix();

        if (_failures == 0)
        {
            Console.WriteLine($"StreamApiTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"StreamApiTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
