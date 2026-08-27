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
 *  080226 MF,CC  First Version — the OutRange batch contract, ported from the
 *                Java BatchApiTest to the C# surface.
 *  082526 MF,CC  Declinable outputs and distinct empty ones (#262).
 */

/* Hand-written test; ta_codegen never opens this file. */

using System;
using System.Collections.Generic;
using TALib;

namespace TALib.Test;

/// <summary>
/// The batch API's contract: what <see cref="OutRange"/> means, and which
/// misuses throw what.
/// </summary>
/// <remarks>
/// <para>Framework-free by design, matching the Java suites: the shipped
/// library takes no test dependency, and `dotnet build` + a plain `Main` is the
/// whole harness. Numerical correctness is NOT this file's job —
/// <c>ta_regtest --codegen</c> and <c>--xlang-hash</c> prove that bit-for-bit
/// against the C reference for all 168 functions. What is tested here is the
/// surface a C# caller touches, which no cross-language harness ever sees:
/// the OutRange value type, the exception mapping, the unguarded tier, and the
/// float overloads.</para>
/// <para>Ported case-for-case from the Java BatchApiTest so the two managed
/// bindings are held to the same contract. Where C# genuinely differs the test
/// says so rather than silently skipping.</para>
/// </remarks>
public static class BatchApiTest
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

    /// <summary>
    /// Same, plus the message has to name what a caller needs to fix it. The
    /// length rejections all share one exception type, so the message is the only
    /// thing that says WHICH span and by how much.
    /// </summary>
    private static void CheckThrows<TException>(Action body, string what, params string[] needles)
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
                return;
            }
            string msg = e.Message ?? "";
            foreach (string needle in needles)
            {
                if (!msg.Contains(needle, StringComparison.Ordinal))
                {
                    _failures++;
                    Console.WriteLine("  FAIL: " + what + " (message \"" + msg
                        + "\" omits \"" + needle + "\")");
                    return;
                }
            }
        }
    }

    private static double[] Closes(int n)
    {
        var outv = new double[n];
        for (int i = 0; i < n; i++)
        {
            outv[i] = 100.0 + 10.0 * Math.Sin(i / 7.0) + 3.0 * Math.Cos(i / 3.0);
        }
        return outv;
    }

    /* ---------------------------------------------------- ported from CoreTest */

    /// <summary>MAX over three bars with a period of 2 — known begIdx, count and values.</summary>
    private static void MaxWithKnownOutputs()
    {
        var core = new Core();
        double[] input = { 2.0, 1.2, 1.5 };
        var output = new double[3];

        OutRange r = core.MAX(0, 2, input, 2, output);

        Check(r.BegIdx == 1, $"MAX BegIdx == 1 (got {r.BegIdx})");
        Check(r.Count == 2, $"MAX Count == 2 (got {r.Count})");
        Check(output[0] == 2.0, "MAX[0] == 2.0");
        Check(output[1] == 1.5, "MAX[1] == 1.5");
        Check(!r.IsEmpty, "MAX range is not empty");
    }

    /// <summary>The first output lands exactly at the lookback.</summary>
    private static void BegIdxEqualsLookback()
    {
        var core = new Core();
        double[] input = Closes(200);
        var output = new double[input.Length];

        OutRange r = core.MA(0, input.Length - 1, input, 10, MAType.SMA, output);
        Check(r.BegIdx == core.MA_Lookback(10, MAType.SMA), "SMA BegIdx == lookback");
        Check(r.Count == input.Length - r.BegIdx, "SMA Count fills to the end");
    }

    /// <summary>
    /// CMO over ±FLT_EPSILON data: the output starts at the lookback and nothing
    /// past <c>Count</c> is written. The untouched tail is the real assertion —
    /// it catches a writer running past the range it reported.
    /// </summary>
    private static void CmoLeavesTheTailUntouched()
    {
        const double fltEpsilon = 1.192092896e-07;
        const double sentinel = -3e37;

        var core = new Core();
        var input = new double[100];
        for (int i = 0; i < input.Length; i++)
        {
            input[i] = (i % 2 == 0 ? 1.0 : -1.0) * fltEpsilon;
        }
        var output = new double[100];
        Array.Fill(output, sentinel);

        int lookback = core.CMO_Lookback(int.MinValue);
        OutRange r = core.CMO(0, input.Length - 1, input, int.MinValue, output);

        Check(r.BegIdx == lookback, "CMO BegIdx == lookback");
        Check(r.Count > 0, "CMO produced values (so the tail check is not vacuous)");
        bool tailUntouched = true;
        for (int i = r.Count; i < output.Length; i++)
        {
            if (output[i] != sentinel)
            {
                tailUntouched = false;
            }
        }
        Check(tailUntouched, "CMO wrote nothing past the range it reported");
    }

    /* -------------------------------------------------- the OutRange contract */

    /// <summary>
    /// The invariant this whole error model hangs on: a valid range shorter than
    /// the lookback is a SUCCESS with no values, never an exception. Matches C's
    /// <c>TA_SUCCESS</c> + <c>outNBElement == 0</c>.
    /// </summary>
    private static void ShortRangeIsAnEmptySuccessNotAnException()
    {
        var core = new Core();
        double[] input = Closes(10);
        var output = new double[10];

        Check(core.SMA_Lookback(30) > 9,
              "the 30-period lookback really does exceed this 10-bar range");

        OutRange r = core.SMA(0, input.Length - 1, input, 30, output);
        Check(r.Count == 0, "too-short range yields Count == 0");
        Check(r.IsEmpty, "too-short range IsEmpty");
        Check(r.BegIdx == 0, "empty range reports BegIdx 0");
    }

    /// <summary>Misuse mapping. Note what is absent: no public path returns a RetCode.</summary>
    private static void MisuseThrowsTheDocumentedException()
    {
        var core = new Core();
        double[] input = Closes(100);
        var output = new double[100];

        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SMA(-1, 50, input, 10, output), "negative startIdx -> ArgumentOutOfRange");
        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SMA(0, -1, input, 10, output), "negative endIdx -> ArgumentOutOfRange");
        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SMA(50, 10, input, 10, output), "endIdx < startIdx -> ArgumentOutOfRange");
        CheckThrows<ArgumentException>(
            () => core.SMA(0, 50, input, 0, output), "period below range -> ArgumentException");

        // The cast is required, not incidental: `null` alone is ambiguous between
        // the double[] and float[] overloads. Real callers pass a typed array.
        // A span is never null: a null array converts to a span of length 0, and
        // so does an empty array, so for this library the two ARE the same state.
        // Both are caught by the length bound, not by a separate emptiness check
        // — any valid range needs endIdx >= 0 and therefore at least one element,
        // so an empty input can never satisfy it.
        CheckThrows<ArgumentException>(
            () => core.SMA(0, 50, (double[])null!, 10, output),
            "null input -> ArgumentException");
        CheckThrows<ArgumentException>(
            () => core.SMA(0, 50, Array.Empty<double>(), 10, output),
            "empty input -> ArgumentException");

        // It names the parameter, which is the whole point of checking rather
        // than letting the body fault on its first index.
        _checks++;
        try
        {
            core.SMA(0, 50, (double[])null!, 10, output);
            _failures++;
            Console.WriteLine("  FAIL: expected an empty-input rejection");
        }
        catch (ArgumentException e)
        {
            if (e.ParamName != "inReal")
            {
                _failures++;
                Console.WriteLine($"  FAIL: ParamName was \"{e.ParamName}\", expected \"inReal\"");
            }
        }

        // An empty output is legitimate when the requested range is shorter than
        // the lookback and the call writes nothing, so the output bound is
        // switched off on exactly that branch. (Outputs ARE capacity-checked
        // otherwise — see TheLengthBoundFromBothSides below.)
        _checks++;
        {
            OutRange r = core.SMA(0, 5, input, 30, Array.Empty<double>());
            if (!r.IsEmpty)
            {
                _failures++;
                Console.WriteLine("  FAIL: a range shorter than the lookback must write nothing");
            }
        }

        // Two outputs sharing one array has no correct answer (issue #108).
        var shared = new double[100];
        var third = new double[100];
        CheckThrows<ArgumentException>(
            () => core.BBANDS(0, 50, input, 20, 2.0, 2.0, MAType.SMA, shared, shared, third),
            "aliased output arrays -> ArgumentException");
    }

    /* ------------------------------------------ span-length checks (#172 C2) */

    /*
     * The same four misuses the Java suite covers, on C#'s span surface. Before
     * these checks each one was "IndexOutOfRangeException: Index was outside the
     * bounds of the array" — raised from inside the algorithm, after the output
     * was partly written, naming neither the buffer nor either size.
     *
     * The bound is the one the Rust backend asserts and Java's wrappers check:
     * an input must reach endIdx, an output must hold the values actually
     * PRODUCED, endIdx - max(startIdx, lookback) + 1. The exact boundary is
     * pinned from both sides below; without that pair every assertion here would
     * pass against a bound that was merely "some number".
     */

    /// <summary>An undersized output names the span and both sizes.</summary>
    private static void UndersizedOutputIsRejected()
    {
        var core = new Core();
        double[] input = Closes(200);

        CheckThrows<ArgumentException>(
            () => core.SMA(0, 199, input, 10, new double[3]),
            "undersized output -> ArgumentException",
            "SMA", "outReal", "3", "191");
    }

    /// <summary>Requesting more than the input holds names the input.</summary>
    private static void UndersizedInputIsRejected()
    {
        var core = new Core();
        double[] input = Closes(200);

        CheckThrows<ArgumentException>(
            () => core.SMA(0, 500, input, 10, new double[501]),
            "endIdx past the input end -> ArgumentException",
            "SMA", "inReal", "200", "501");
    }

    /// <summary>Two inputs of different lengths: the short one is named.</summary>
    private static void MismatchedInputLengthsAreRejected()
    {
        var core = new Core();
        double[] longer = Closes(200);
        double[] shorter = Closes(50);
        var output = new double[200];

        CheckThrows<ArgumentException>(
            () => core.ADD(0, 199, longer, shorter, output),
            "mismatched input lengths -> ArgumentException",
            "ADD", "inReal1", "50", "200");
        // Control: over the range both legs cover, the same call succeeds.
        Check(core.ADD(0, 49, longer, shorter, output).Count == 50,
              "ADD over the range both legs cover succeeds");
    }

    /// <summary>
    /// The exact boundary, from both sides. This is what makes every other length
    /// assertion here mean something: a bound of endIdx - startIdx + 1 instead of
    /// the produced count would still reject all the calls above, and would fail
    /// on the accepted side of this one.
    /// </summary>
    private static void TheLengthBoundFromBothSides()
    {
        var core = new Core();
        double[] input = Closes(200);
        int produced = 199 - core.SMA_Lookback(10) + 1;

        Check(produced == 191, "the produced count really is 191 (got " + produced + ")");
        Check(produced < 200, "the produced count is shorter than the requested range");
        Check(core.SMA(0, 199, input, 10, new double[produced]).Count == produced,
              "an exactly-sized output is accepted and filled");
        CheckThrows<ArgumentException>(
            () => core.SMA(0, 199, input, 10, new double[produced - 1]),
            "one element short of the produced count -> ArgumentException",
            "outReal", (produced - 1).ToString(), produced.ToString());
    }

    /// <summary>
    /// The complaint this fixes: not that the call fails, but that it used to fail
    /// halfway through, having already written to the caller's buffer.
    /// </summary>
    private static void ARejectedCallWritesNothing()
    {
        const double sentinel = -3e37;
        var core = new Core();
        double[] input = Closes(200);
        var output = new double[3];
        Array.Fill(output, sentinel);

        CheckThrows<ArgumentException>(
            () => core.SMA(0, 199, input, 10, output), "undersized output throws");

        bool untouched = true;
        foreach (double v in output)
        {
            if (v != sentinel)
            {
                untouched = false;
            }
        }
        Check(untouched, "a rejected call left the output buffer untouched");
        // Non-vacuity: the same buffer IS writable by a call that is accepted.
        core.SMA(0, 2, input, 1, output);
        Check(output[0] != sentinel, "the sentinel is overwritten by a call that runs");
    }

    /// <summary>
    /// The checks do not pre-empt the core's own RetCode mapping. Every case is
    /// BOTH a bad argument and an unusable buffer; the core owns the diagnosis.
    /// </summary>
    private static void TheCoreStillOwnsItsOwnDiagnoses()
    {
        var core = new Core();
        double[] input = Closes(200);
        var tiny = new double[3];

        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SMA(50, 10, input, 10, tiny),
            "endIdx < startIdx still -> ArgumentOutOfRange", "endIdx");
        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SMA(-1, 199, input, 10, tiny),
            "negative startIdx still -> ArgumentOutOfRange", "startIdx");
        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SMA(0, Core.MAX_INDEX + 1, input, 10, tiny),
            "endIdx above MAX_INDEX still -> ArgumentOutOfRange", "endIdx");
        CheckThrows<ArgumentException>(
            () => core.SMA(0, 199, input, 0, tiny),
            "out-of-range period still -> the parameter message", "bad parameter");

        // And an EMPTY input does not change any of those answers. It used to:
        // a separate emptiness check ran first and reported "inReal is empty"
        // when the caller's actual mistake was the index or the parameter.
        //
        // One case per condition under which ClampedStart returns -1, which is
        // the complete set of arguments whose diagnosis belongs to the core.
        // Fewer than all four would leave a branch where the old behaviour could
        // come back unnoticed.
        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SMA(-1, 199, ReadOnlySpan<double>.Empty, 10, tiny),
            "empty input does not mask a bad startIdx", "startIdx");
        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SMA(50, 10, ReadOnlySpan<double>.Empty, 10, tiny),
            "empty input does not mask endIdx < startIdx", "endIdx");
        CheckThrows<ArgumentOutOfRangeException>(
            () => core.SMA(0, Core.MAX_INDEX + 1, ReadOnlySpan<double>.Empty, 10, tiny),
            "empty input does not mask endIdx above MAX_INDEX", "endIdx");
        CheckThrows<ArgumentException>(
            () => core.SMA(0, 199, ReadOnlySpan<double>.Empty, 0, tiny),
            "empty input does not mask a bad parameter", "bad parameter");
    }

    /// <summary>Each output is checked on its own, and named on its own.</summary>
    private static void EachOutputIsCheckedSeparately()
    {
        var core = new Core();
        double[] input = Closes(200);
        int produced = 199 - core.MACD_Lookback(12, 26, 9) + 1;
        Check(produced > 0, "MACD produces values over this range");

        var big1 = new double[200];
        var big2 = new double[200];
        var small = new double[produced - 1];

        CheckThrows<ArgumentException>(
            () => core.MACD(0, 199, input, 12, 26, 9, small, big1, big2),
            "short first output is named", "outMACD", produced.ToString());
        CheckThrows<ArgumentException>(
            () => core.MACD(0, 199, input, 12, 26, 9, big1, small, big2),
            "short second output is named", "outMACDSignal", produced.ToString());
        CheckThrows<ArgumentException>(
            () => core.MACD(0, 199, input, 12, 26, 9, big1, big2, small),
            "short third output is named", "outMACDHist", produced.ToString());
    }

    /// <summary>The int outputs the candlestick patterns write are checked too.</summary>
    private static void IntegerOutputsAreChecked()
    {
        var core = new Core();
        double[] o = Closes(200);
        double[] h = Closes(200);
        double[] l = Closes(200);
        double[] c = Closes(200);
        int produced = 199 - core.CDLDOJI_Lookback() + 1;

        CheckThrows<ArgumentException>(
            () => core.CDLDOJI(0, 199, o, h, l, c, new int[3]),
            "short int output -> ArgumentException",
            "CDLDOJI", "outInteger", "3", produced.ToString());
    }

    /// <summary>The float overload carries the identical checks.</summary>
    private static void FloatOverloadIsCheckedToo()
    {
        var core = new Core();
        double[] input = Closes(200);
        var inF = new float[200];
        for (int i = 0; i < input.Length; i++)
        {
            inF[i] = (float)input[i];
        }

        CheckThrows<ArgumentException>(
            () => core.SMA(0, 199, inF, 10, new double[3]),
            "float overload: undersized output", "SMA", "outReal", "3", "191");
        CheckThrows<ArgumentException>(
            () => core.SMA(0, 500, inF, 10, new double[501]),
            "float overload: endIdx past the input end", "SMA", "inReal", "200", "501");
        Check(core.SMA(0, 199, inF, 10, new double[191]).Count == 191,
              "float overload accepts an exactly-sized output");
    }

    /// <summary>
    /// A leg the algorithm never indexes is checked like any other (#260). Four
    /// candlestick patterns declare an OHLC input they never read; Rust, Java and
    /// C# used to exempt exactly those while C's NULL checks covered them, so the
    /// identical call was <c>TA_BAD_PARAM</c> in C and a success here. A declared
    /// input must be supplied; that rule now needs no exception list.
    ///
    /// <para>A span is never null, so both spellings of "not supplied" — an empty
    /// span and <c>(double[])null</c>, which converts to one — arrive at the same
    /// length rejection. The control is the leg beside it, which IS read and was
    /// never exempt.</para>
    /// </summary>
    private static void AnUnreadLegIsCheckedLikeAnyOther()
    {
        var core = new Core();
        double[] real = Closes(200);
        var outv = new int[200];

        CheckThrows<ArgumentException>(
            () => core.CDL3OUTSIDE(0, 199, real, ReadOnlySpan<double>.Empty,
                                   ReadOnlySpan<double>.Empty, real, outv),
            "an empty high leg the body never reads", "inHigh", "0", "200");
        double[] nullLeg = null!;
        CheckThrows<ArgumentException>(
            () => core.CDL3OUTSIDE(0, 199, real, real, nullLeg, real, outv),
            "a null low leg the body never reads", "inLow", "0", "200");
        CheckThrows<ArgumentException>(
            () => core.CDLHIKKAKE(0, 199, ReadOnlySpan<double>.Empty, real, real, real, outv),
            "CDLHIKKAKE's open leg, the other shape of the same exemption",
            "inOpen", "0", "200");

        CheckThrows<ArgumentException>(
            () => core.CDL3OUTSIDE(0, 199, ReadOnlySpan<double>.Empty, real, real, real, outv),
            "the open leg, which IS read, is still checked", "inOpen", "0", "200");
        // Non-vacuity: every leg supplied and sized is the success these reject.
        Check(core.CDL3OUTSIDE(0, 199, real, real, real, real, outv).Count > 0,
              "CDL3OUTSIDE runs when every declared leg is supplied");
    }

    /// <summary>
    /// The input bound survives the short-range escape that switches the output
    /// bound off: an endIdx past the end of the supplied series is a caller bug in
    /// any range, including one that produces no values.
    /// </summary>
    private static void AnEndIdxPastTheInputIsRejectedEvenProducingNothing()
    {
        var core = new Core();
        double[] input = Closes(24);
        double[] wide = Closes(25);

        Check(core.APO_Lookback(12, 26, MAType.EMA) > 24,
              "APO's lookback exceeds this range, so nothing is produced");
        CheckThrows<ArgumentException>(
            () => core.APO(0, 24, input, 12, 26, MAType.EMA, Array.Empty<double>()),
            "endIdx past the input, producing nothing", "APO", "inReal", "24", "25");
        Check(core.APO(0, 24, wide, 12, 26, MAType.EMA, Array.Empty<double>()).Count == 0,
              "an input reaching endIdx is an empty success, zero-length output");
    }

    /// <summary>
    /// The public surface exposes no unguarded tier. Pinned by reflection over the
    /// shipped Core rather than by a string assertion on generator output.
    /// </summary>
    private static void NoUnguardedTierOnThePublicSurface()
    {
        int leaked = 0;
        int sma = 0;
        foreach (var m in typeof(Core).GetMethods())
        {
            if (m.Name.EndsWith("Unguarded", System.StringComparison.Ordinal))
            {
                leaked++;
            }
            if (m.Name == "SMA")
            {
                sma++;
            }
        }
        Check(leaked == 0, "no Unguarded method survives on the public Core surface");
        // Non-vacuity: the reflection actually sees the surface it is asserting over.
        Check(sma >= 2, "reflection sees the SMA overloads it is filtering over");
    }

    /// <summary>The float overload adopts the identical shape (C's TA_S_* parity).</summary>
    private static void FloatOverloadHasTheSameShape()
    {
        var core = new Core();
        double[] input = Closes(100);
        var inputF = new float[100];
        for (int i = 0; i < input.Length; i++)
        {
            inputF[i] = (float)input[i];
        }
        var outputD = new double[100];
        var outputF = new double[100];

        OutRange rd = core.SMA(0, input.Length - 1, input, 10, outputD);
        OutRange rf = core.SMA(0, inputF.Length - 1, inputF, 10, outputF);

        Check(rd.BegIdx == rf.BegIdx && rd.Count == rf.Count,
              "float overload reports the same OutRange");
        Check(rf.Count > 0, "float overload produced values");
    }

    /// <summary>
    /// OutRange is a value type: equality by components. C# gives a readonly
    /// struct value equality for free, so unlike Java's record there is no
    /// hand-written equals to get wrong — but the semantics still have to hold,
    /// because callers do compare ranges.
    /// </summary>
    private static void OutRangeValueSemantics()
    {
        Check(new OutRange(3, 7).Equals(new OutRange(3, 7)), "OutRange equals by value");
        Check(!new OutRange(3, 7).Equals(new OutRange(3, 8)), "OutRange distinguishes Count");
        Check(!new OutRange(3, 7).Equals(new OutRange(4, 7)), "OutRange distinguishes BegIdx");
        Check(new OutRange(3, 7).GetHashCode() == new OutRange(3, 7).GetHashCode(),
              "OutRange GetHashCode agrees with Equals");
        Check(new OutRange(0, 0).IsEmpty && new OutRange(0, 0).Count == 0, "default OutRange is empty");
        Check(default(OutRange).IsEmpty, "default(OutRange) is empty");
    }

    /// <summary>
    /// <c>int.MinValue</c> selects the documented default for an integer
    /// parameter — the sentinel contract Core's class doc promises. Pinned
    /// against the explicit period so a change to either side shows up.
    /// </summary>
    private static void IntegerSentinelSelectsTheDocumentedDefault()
    {
        var core = new Core();
        double[] input = Closes(200);
        var outputDefault = new double[200];
        var outputExplicit = new double[200];

        Check(core.SMA_Lookback(int.MinValue) == core.SMA_Lookback(30),
              "SMA lookback: int.MinValue == the documented default of 30");

        OutRange rDefault = core.SMA(0, input.Length - 1, input, int.MinValue, outputDefault);
        OutRange rExplicit = core.SMA(0, input.Length - 1, input, 30, outputExplicit);

        Check(rDefault.BegIdx == rExplicit.BegIdx && rDefault.Count == rExplicit.Count,
              "SMA: int.MinValue reports the same range as period 30");
        bool same = rDefault.Count > 0;
        for (int i = 0; i < rDefault.Count; i++)
        {
            if (BitConverter.DoubleToInt64Bits(outputDefault[i])
                != BitConverter.DoubleToInt64Bits(outputExplicit[i]))
            {
                same = false;
            }
        }
        Check(same, "SMA: int.MinValue is bit-identical to period 30");
    }

    /// <summary>Runs every case; returns 0 on success, 1 on any failure.</summary>
    /// <summary>Overlapping buffers are rejected; whole-buffer in-place is not.</summary>
    /// <remarks>
    /// <para>Spans made partial overlap expressible — <c>double[]</c> could not, because two
    /// arrays are identical or disjoint with nothing in between. Equality is therefore no
    /// longer a complete distinctness test, and several transcribed bodies branch on series
    /// identity as ALGORITHM (BBANDS elects its scratch that way), so a partial overlap makes
    /// them write through their own input and return Success with wrong values.</para>
    /// <para>No cross-language gate can see this: every aliasing probe in the tree passes
    /// whole, identical buffers, where equality and overlap behave alike. This suite is the
    /// only place it is checked.</para>
    /// </remarks>
    private static void OverlappingBuffersAreRejected()
    {
        var core = new Core();
        const int n = 64;
        double[] src = Closes(n);
        var big = new double[n + 40];

        // Two outputs offset within one buffer.
        CheckThrows<ArgumentException>(
            () => core.BBANDS(0, n - 1, src, 5, 2.0, 2.0, MAType.SMA,
                              big.AsSpan(0, n), big.AsSpan(10, n), big.AsSpan(n + 12, 4)),
            "outputs overlapping at an offset are rejected");

        // Same start, DIFFERENT length: the same memory, which an equality test
        // reads as not-equal. This is the case that motivated the fix.
        CheckThrows<ArgumentException>(
            () => core.BBANDS(0, n - 1, src, 5, 2.0, 2.0, MAType.SMA,
                              big.AsSpan(0, n), big.AsSpan(0, n + 1), big.AsSpan(n + 12, 4)),
            "outputs sharing a start but differing in length are rejected");

        // An output partially overlapping an INPUT.
        var buf = new double[n + 40];
        Array.Copy(src, buf, n);
        CheckThrows<ArgumentException>(
            () => core.BBANDS(0, n - 1, buf.AsSpan(0, n), 5, 2.0, 2.0, MAType.SMA,
                              buf.AsSpan(20, n), new double[n], new double[n]),
            "an output partially overlapping an input is rejected");

        // ...and the same for a single-output function, through MAVP's in-place defence.
        var per = new double[n];
        for (int i = 0; i < n; i++)
        {
            per[i] = 5 + (i % 10);
        }
        var mbuf = new double[n + 40];
        Array.Copy(src, mbuf, n);
        CheckThrows<ArgumentException>(
            () => core.MAVP(0, n - 1, mbuf.AsSpan(0, n), per, 2, 20, MAType.SMA, mbuf.AsSpan(15, n)),
            "MAVP rejects a partially overlapping output");

        // THE OTHER HALF: whole-buffer in-place must still work, and still be
        // correct. Rejecting it would break callers the bodies were written for.
        var inplace = (double[])src.Clone();
        var reference = new double[n];
        core.SMA(0, n - 1, src, 5, reference);
        OutRange r = core.SMA(0, n - 1, inplace, 5, inplace);
        Check(r.Count > 0, "whole-buffer in-place is still accepted");

        bool same = true;
        for (int i = 0; i < r.Count; i++)
        {
            if (BitConverter.DoubleToInt64Bits(inplace[i]) != BitConverter.DoubleToInt64Bits(reference[i]))
            {
                same = false;
            }
        }
        Check(same, "in-place SMA is bit-identical to the disjoint call");

        // And a disjoint multi-output call is untouched by the guard.
        var o1 = new double[n];
        var o2 = new double[n];
        var o3 = new double[n];
        OutRange rb = core.BBANDS(0, n - 1, src, 5, 2.0, 2.0, MAType.SMA, o1, o2, o3);
        Check(rb.Count > 0, "disjoint outputs still produce values");
    }

    /// <summary>Every failure carries the code C would have returned, and the
    /// mapping back is TOTAL and LOSSLESS.</summary>
    /// <remarks>
    /// <para>Total: every exception the public API raises implements
    /// <see cref="ITaLibFailure"/>, including the condition C cannot detect (a
    /// span too short). Anything not covered leaves a caller with a thrown
    /// object it cannot classify, which is the state this replaced.</para>
    /// <para>Lossless: no two codes share one thrown representation. That is the
    /// half the exception TYPES cannot carry — one
    /// <see cref="InvalidOperationException"/> serves both library-side codes,
    /// and the two index codes are told apart only by a ParamName string — so a
    /// check on the type alone would pass with the arms of <c>Failure()</c>
    /// swapped.</para>
    /// </remarks>
    private static void EveryFailureCarriesItsCode()
    {
        var core = new Core();
        double[] input = Closes(200);
        double[] output = new double[200];

        // Lossless, the pair the type cannot separate.
        CheckCode(RetCode.OutOfRangeStartIndex,
            () => core.SMA(-1, 50, input, 10, output), "negative startIdx carries OutOfRangeStartIndex");
        CheckCode(RetCode.OutOfRangeEndIndex,
            () => core.SMA(50, 10, input, 10, output), "endIdx < startIdx carries OutOfRangeEndIndex");

        // The rest of the batch tier's vocabulary.
        CheckCode(RetCode.BadParam,
            () => core.SMA(0, 50, input, 0, output), "an out-of-range period carries BadParam");
        CheckCode(RetCode.BadParam,
            () => core.MACD(0, 199, input, 12, 26, 9, output, output, new double[200]),
            "two outputs sharing one buffer carries BadParam");

        // The condition C has no code for. It reports the code C answers for an
        // absent argument it CAN detect, so the mapping stays total.
        CheckCode(RetCode.BadParam,
            () => core.SMA(0, 199, input, 10, new double[3]), "a short output carries BadParam");

        // Streaming's one recoverable condition, which is why it has a code.
        int lookback = core.SMA_Lookback(30);
        CheckCode(RetCode.InsufficientHistory,
            () => core.SMA_Open(new double[lookback], 30), "a short history carries InsufficientHistory");
        CheckThrows<InsufficientHistoryException>(
            () => core.SMA_Open(new double[lookback], 30), "a short history is still typed");

        // ...and the REST of the streaming tier, which is a separate reject
        // ladder from the batch one. Totality is a property of every failure the
        // library raises, not of the tier someone happened to convert first.
        CheckCode(RetCode.OutOfRangeStartIndex,
            () => core.SMA_Open(ReadOnlySpan<double>.Empty, 30),
            "an empty history carries OutOfRangeStartIndex");
        CheckCode(RetCode.BadParam,
            () => core.SMA_Open(input, 0),
            "an out-of-range period on a stream open carries BadParam");

        // The numbers the cross-language harness compares. Hardcoded, because
        // asking the enum for its own value would prove nothing.
        Check((int)RetCode.Success == 0, "Success is 0");
        Check((int)RetCode.BadParam == 2, "BadParam is 2");
        Check((int)RetCode.AllocErr == 3, "AllocErr is 3");
        Check((int)RetCode.OutOfRangeStartIndex == 12, "OutOfRangeStartIndex is 12");
        Check((int)RetCode.OutOfRangeEndIndex == 13, "OutOfRangeEndIndex is 13");
        Check((int)RetCode.InsufficientHistory == 17, "InsufficientHistory is 17");
        Check((int)RetCode.InternalError == 5000, "InternalError is 5000");

        // Non-vacuity: the cases above have to REACH every code the batch and
        // streaming tiers can produce, or a member could stop being emitted
        // anywhere and nothing here would move. AllocErr and InternalError are
        // the two exceptions — one is unreachable here (#178) and the other
        // needs a corrupted CIRCBUF size — so they are named rather than
        // silently excluded.
        var expected = new HashSet<RetCode>
        {
            RetCode.OutOfRangeStartIndex, RetCode.OutOfRangeEndIndex,
            RetCode.BadParam, RetCode.InsufficientHistory,
        };
        Check(_seenCodes.SetEquals(expected),
            "the probes reached exactly " + string.Join(",", expected)
                + " (got " + string.Join(",", _seenCodes) + ")");
    }

    private static readonly HashSet<RetCode> _seenCodes = new();

    /// <summary>The call must throw, the throw must carry a code, and it must be
    /// this one.</summary>
    private static void CheckCode(RetCode expected, Action body, string what)
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
            if (e is not ITaLibFailure f)
            {
                _failures++;
                Console.WriteLine("  FAIL: " + what + " (" + e.GetType().FullName
                    + " carries no RetCode)");
                return;
            }
            if (f.RetCode != expected)
            {
                _failures++;
                Console.WriteLine("  FAIL: " + what + " (carried " + f.RetCode + ")");
                return;
            }
            _seenCodes.Add(f.RetCode);
        }
    }

    /// <summary>
    /// Rule B6a: an output the .yaml marks <c>nullable</c> may be declined, and
    /// declining it changes nothing about the output that was asked for. MAMA's
    /// <c>outFAMA</c> is the only one in the corpus.
    /// </summary>
    /// <remarks>C# cannot spell "absent" apart from "empty" — a <c>Span&lt;T&gt;</c>
    /// is a ref struct and a null array converts to an empty span — so an empty
    /// span IS the declination (Appendix F of the error-handling spec).
    /// <para>Acceptance alone would not test this: a body that stopped computing
    /// FAMA, or took a different path without it, would be accepted here just the
    /// same. So the declining call has to reproduce the supplied one bit for bit
    /// and leave everything above its own count untouched (rule N2). No
    /// cross-language gate can see any of it — the JSON-RPC servers bind every
    /// declared output.</para></remarks>
    private static void ANullableOutputMayBeDeclined()
    {
        var core = new Core();
        double[] input = Closes(252);
        var mamaRef = new double[252];
        var famaRef = new double[252];
        OutRange reference = core.MAMA(0, 251, input, 0.5, 0.05, mamaRef, famaRef);
        Check(reference.Count > 0, "the reference call produces values");

        const double canary = -1.2345678901234e300;
        var mama = new double[252];
        Array.Fill(mama, canary);
        OutRange r = core.MAMA(0, 251, input, 0.5, 0.05, mama, default);

        Check(r.BegIdx == reference.BegIdx && r.Count == reference.Count,
            "declining outFAMA leaves the reported range alone");
        bool same = true;
        for (int i = 0; i < reference.Count; i++)
        {
            same &= BitConverter.DoubleToInt64Bits(mama[i])
                 == BitConverter.DoubleToInt64Bits(mamaRef[i]);
        }
        Check(same, "declining outFAMA leaves outMAMA bit-identical");
        bool untouched = true;
        for (int i = r.Count; i < mama.Length; i++)
        {
            untouched &= BitConverter.DoubleToInt64Bits(mama[i])
                      == BitConverter.DoubleToInt64Bits(canary);
        }
        Check(untouched, "the declining call writes nothing past its own count");

        // A null array converts to an empty span, so this is the same call —
        // which is exactly why C# cannot tell "declined" from "empty".
        double[]? absent = null;
        core.MAMA(0, 251, input, 0.5, 0.05, new double[252], absent);

        // Controls, so the acceptance above is about the FLAG and not about MAMA
        // having stopped checking its outputs.
        CheckThrows<ArgumentException>(
            () => core.MAMA(0, 251, input, 0.5, 0.05, default, famaRef),
            "the non-nullable output is still required", "MAMA", "outMAMA");
        CheckThrows<ArgumentException>(
            () => core.MAMA(0, 251, input, 0.5, 0.05, mamaRef, new double[1]),
            "a SUPPLIED nullable output is still length-checked", "MAMA", "outFAMA");
    }

    /// <summary>
    /// Appendix D item 11: distinct zero-length outputs are not aliases. A range
    /// shorter than the lookback produces nothing and needs no output space
    /// (rule N1), so the call is a success with an empty range — which is what C
    /// and Java always answered.
    /// </summary>
    /// <remarks>C# used to reject it outright: the pair guard carried an explicit
    /// <c>a.IsEmpty &amp;&amp; b.IsEmpty</c> arm, because a span carries no
    /// identity with no elements. That arm also made "declined" unspellable,
    /// which is why it went with #262 rather than being narrowed.</remarks>
    private static void DistinctEmptyOutputsAreNotAliases()
    {
        var core = new Core();
        double[] input = Closes(252);
        const int period = 253;
        Check(core.ACCBANDS_Lookback(period) > 251,
            "the probe needs a lookback past the range, or it proves nothing");

        OutRange r = core.ACCBANDS(0, 251, input, input, input, period,
            default, default, default);
        Check(r.Count == 0, "a sub-lookback range needs no output space");

        // Control: the same three empty spans on a range that DOES produce values
        // are still rejected, so this is about the count and not about the bound
        // having gone away.
        CheckThrows<ArgumentException>(
            () => core.ACCBANDS(0, 251, input, input, input, 20, default, default, default),
            "an output that has to hold values is still bounded", "ACCBANDS");
        // And a REAL alias of two outputs is still rejected.
        var shared = new double[252];
        CheckThrows<ArgumentException>(
            () => core.ACCBANDS(0, 251, input, input, input, 20,
                shared, shared, new double[252]),
            "two outputs that are one span are still rejected", "ACCBANDS");
    }

    public static int Run()
    {
        MaxWithKnownOutputs();
        BegIdxEqualsLookback();
        CmoLeavesTheTailUntouched();
        ShortRangeIsAnEmptySuccessNotAnException();
        MisuseThrowsTheDocumentedException();
        UndersizedOutputIsRejected();
        UndersizedInputIsRejected();
        MismatchedInputLengthsAreRejected();
        TheLengthBoundFromBothSides();
        ARejectedCallWritesNothing();
        TheCoreStillOwnsItsOwnDiagnoses();
        EachOutputIsCheckedSeparately();
        IntegerOutputsAreChecked();
        FloatOverloadIsCheckedToo();
        AnUnreadLegIsCheckedLikeAnyOther();
        AnEndIdxPastTheInputIsRejectedEvenProducingNothing();
        OverlappingBuffersAreRejected();
        NoUnguardedTierOnThePublicSurface();
        FloatOverloadHasTheSameShape();
        OutRangeValueSemantics();
        IntegerSentinelSelectsTheDocumentedDefault();
        EveryFailureCarriesItsCode();
        ANullableOutputMayBeDeclined();
        DistinctEmptyOutputsAreNotAliases();

        if (_failures == 0)
        {
            Console.WriteLine($"BatchApiTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"BatchApiTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
