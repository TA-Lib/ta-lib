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
 */

/* Hand-written test; ta_codegen never opens this file. */

using System;
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
        Check(r.EndIdx == 3, "MAX EndIdx == BegIdx + Count");
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
        CheckThrows<NullReferenceException>(
            () => core.SMA(0, 50, (double[])null!, 10, output),
            "null input -> NullReferenceException");

        // Two outputs sharing one array has no correct answer (issue #108).
        var shared = new double[100];
        var third = new double[100];
        CheckThrows<ArgumentException>(
            () => core.BBANDS(0, 50, input, 20, 2.0, 2.0, MAType.SMA, shared, shared, third),
            "aliased output arrays -> ArgumentException");
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
    public static int Run()
    {
        MaxWithKnownOutputs();
        BegIdxEqualsLookback();
        CmoLeavesTheTailUntouched();
        ShortRangeIsAnEmptySuccessNotAnException();
        MisuseThrowsTheDocumentedException();
        NoUnguardedTierOnThePublicSurface();
        FloatOverloadHasTheSameShape();
        OutRangeValueSemantics();
        IntegerSentinelSelectsTheDocumentedDefault();

        if (_failures == 0)
        {
            Console.WriteLine($"BatchApiTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"BatchApiTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
