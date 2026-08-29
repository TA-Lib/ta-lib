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
 *  082326 MF,CC  First Version — DIV's documented zero-divisor result
 *                (issue #249).
 */

/* Hand-written test; ta_codegen never opens this file. */

using System;
using TALib;

namespace TALib.Test;

/// <summary>
/// DIV's documented zero-divisor result (issue #249).
/// </summary>
/// <remarks>
/// <para>DIV's published Notes say: "Zero divided by zero gives NaN; anything
/// else divided by zero gives positive or negative infinity. Neither is
/// reported as an error." That was true by construction in every backend and
/// asserted nowhere — no test in any language had ever handed DIV a zero
/// divisor. In C#'s case the batch tier throws on a rejected call, so "not an
/// error" is a statement about control flow as much as about the value.</para>
/// <para>One table covers every sign combination IEEE-754 division
/// distinguishes, plus two controls a wrongly-placed guard would break: a zero
/// NUMERATOR over a non-zero divisor stays a signed zero, and ordinary
/// quotients are untouched.</para>
/// <para>The streaming tier takes the same table twice, because it emits two
/// loops. <c>DivOpenImpl</c> carries its own transcription of the batch body
/// (the warm-up fill) and <c>DivStepImpl</c> carries the per-bar one, so a
/// guard added to one is invisible to the other — measured on the C side: an
/// <c>_OpenImpl</c>-only guard on a zero divisor survived every other assertion
/// in the group. Neither entry point may reject the bar: both guard their
/// INPUTS with <c>double.IsFinite</c>, and a zero divisor is finite.</para>
/// <para>The C group <c>test_div_zero.c</c> makes the same assertions against
/// the shipped C library and re-issues each batch call to every language
/// server, comparing each server's output to C's bit for bit.</para>
/// </remarks>
public static class DivZeroTest
{
    private static int _failures;
    private static int _checks;

    private static readonly double[] Num = { 0.0, 0.0, -0.0, -0.0, 1.5, -1.5, 1.5, -1.5, 0.0, -0.0, 6.0, -6.0 };
    private static readonly double[] Den = { 0.0, -0.0, 0.0, -0.0, 0.0, 0.0, -0.0, -0.0, 4.0, 4.0, 3.0, 3.0 };

    /// <summary><c>null</c> means "the only defined answer is NaN".</summary>
    private static readonly double?[] Expected =
    {
        null, null, null, null,
        double.PositiveInfinity, double.NegativeInfinity,
        double.NegativeInfinity, double.PositiveInfinity,
        0.0, -0.0,          // controls: the sign of a zero numerator survives
        2.0, -2.0           // controls: an ordinary quotient is untouched
    };

    private static string Label(int i) => $"DIV({Num[i]}, {Den[i]})";

    /* Six rows need more than ==: the four NaN rows, which are not comparable
       at all, and the two signed-zero rows, where +0.0 == -0.0. (== separates
       the two infinities on its own; the sign test is there for the zeros.)
       Comparing raw bits instead would over-assert: the NaN payload is the
       host's, not the language's. */
    private static void Check(string tier, int i, double got)
    {
        _checks++;
        double? want = Expected[i];
        bool ok = want is null
            ? double.IsNaN(got)
            : got == want.Value && double.IsNegative(got) == double.IsNegative(want.Value);
        if (!ok)
        {
            _failures++;
            Console.WriteLine($"  FAIL: {tier} {Label(i)} = {got}, expected "
                              + (want is null ? "NaN" : want.Value.ToString()));
        }
    }

    private static void Fail(string what)
    {
        _checks++;
        _failures++;
        Console.WriteLine($"  FAIL: {what}");
    }

    private static void BatchFullRange()
    {
        var core = new Core();
        double[] outReal = new double[Num.Length];
        OutRange r;
        try
        {
            r = core.DIV(0, Num.Length - 1, Num, Den, outReal);
        }
        catch (Exception e)
        {
            Fail($"DIV threw on a zero divisor: {e.GetType().Name}: {e.Message}");
            return;
        }

        _checks++;
        if (r.BegIdx != 0 || r.Count != Num.Length)
        {
            _failures++;
            Console.WriteLine($"  FAIL: DIV range {r.BegIdx}/{r.Count}, expected 0/{Num.Length}");
            return;
        }
        for (int i = 0; i < Num.Length; i++)
        {
            Check("batch", i, outReal[i]);
        }
    }

    private static void BatchSubRange()
    {
        // 4..7 is the +/-Inf block, so a range that silently restarted at 0
        // would land on the NaN rows and fail rather than looking right.
        var core = new Core();
        double[] outReal = new double[Num.Length];
        OutRange r = core.DIV(4, 7, Num, Den, outReal);
        _checks++;
        if (r.BegIdx != 4 || r.Count != 4)
        {
            _failures++;
            Console.WriteLine($"  FAIL: DIV sub-range {r.BegIdx}/{r.Count}, expected 4/4");
            return;
        }
        for (int i = 0; i < r.Count; i++)
        {
            Check("sub-range", 4 + i, outReal[i]);
        }
    }

    private static void FloatOverload()
    {
        // The float overload widens each element to double before dividing, so
        // it divides the same two numbers -- every literal above is exactly
        // representable as a float.
        var core = new Core();
        float[] n = new float[Num.Length], d = new float[Den.Length];
        for (int i = 0; i < Num.Length; i++) { n[i] = (float)Num[i]; d[i] = (float)Den[i]; }
        double[] outReal = new double[Num.Length];
        core.DIV(0, Num.Length - 1, n, d, outReal);
        for (int i = 0; i < Num.Length; i++)
        {
            Check("float", i, outReal[i]);
        }
    }

    private static void FillLoop()
    {
        // DivOpenImpl, not DivStepImpl: a separate transcription of the batch
        // body, and the only assertion that reaches it.
        var core = new Core();
        double[] outReal = new double[Num.Length];
        Core.DivStream s;
        OutRange r;
        try
        {
            s = core.DivOpenAndFill(Num, Den, outReal);
            r = s.OutRange;
        }
        catch (Exception e)
        {
            Fail($"DivOpenAndFill threw on a zero divisor: {e.GetType().Name}: {e.Message}");
            return;
        }

        _checks++;
        if (r.BegIdx != 0 || r.Count != Num.Length)
        {
            _failures++;
            Console.WriteLine($"  FAIL: DivOpenAndFill range {r.BegIdx}/{r.Count}, expected 0/{Num.Length}");
            return;
        }
        for (int i = 0; i < Num.Length; i++)
        {
            Check("fill", i, outReal[i]);
        }
        // The same loop with no output array: only the last row survives.
        Check("open-last", Num.Length - 1, core.DivOpen(Num, Den).Value);
    }

    private static void StreamingTier()
    {
        var core = new Core();
        Core.DivStream s;
        try
        {
            s = core.DivOpen(new[] { Num[0] }, new[] { Den[0] });
        }
        catch (Exception e)
        {
            Fail($"DivOpen threw on a zero divisor: {e.GetType().Name}: {e.Message}");
            return;
        }
        Check("open", 0, s.Value);

        for (int i = 1; i < Num.Length; i++)
        {
            // A zero divisor is a FINITE input. The tier rejects non-finite
            // INPUTS; a guard widened to the OUTPUT would surface here and
            // nowhere else.
            double peeked, updated;
            try
            {
                peeked = s.Peek(Num[i], Den[i]);
                updated = s.Update(Num[i], Den[i]);
            }
            catch (Exception e)
            {
                Fail($"stream rejected {Label(i)}: {e.GetType().Name}: {e.Message}");
                return;
            }
            Check("peek", i, peeked);
            Check("update", i, updated);
            _checks++;
            if (BitConverter.DoubleToInt64Bits(peeked) != BitConverter.DoubleToInt64Bits(updated))
            {
                _failures++;
                Console.WriteLine($"  FAIL: Peek != Update at {Label(i)}");
            }
        }

        _checks++;
        if (s.OutRange.Count != Num.Length)
        {
            _failures++;
            Console.WriteLine($"  FAIL: stream committed {s.OutRange.Count} bars, expected {Num.Length}");
        }
    }

    private static void NonFiniteBarStillRejected()
    {
        // The control for the case above: the tier's own contract is unchanged,
        // so "does not reject a zero divisor" cannot be read as "rejects
        // nothing". The MESSAGE is checked, not just the type -- see
        // StreamApiTest.ThrowsBadParam for why.
        var core = new Core();
        Core.DivStream s = core.DivOpen(new[] { 1.0 }, new[] { 2.0 });
        foreach (double bad in new[] { double.NaN, double.PositiveInfinity, double.NegativeInfinity })
        {
            for (int slot = 0; slot < 2; slot++)
            {
                double a0 = slot == 0 ? bad : 1.0, a1 = slot == 0 ? 1.0 : bad;
                // Peek as well as Update: they carry the guard separately, so a
                // control on one of them leaves the other's removable.
                MustReject("Peek", bad, () => s.Peek(a0, a1));
                MustReject("Update", bad, () => s.Update(a0, a1));
            }
        }
    }

    private static void MustReject(string what, double bad, Action body)
    {
        _checks++;
        try
        {
            body();
            _failures++;
            Console.WriteLine($"  FAIL: {what} accepted a non-finite bar ({bad})");
        }
        catch (ArgumentException e) when (e.Message.EndsWith(": BadParam", StringComparison.Ordinal))
        {
            /* expected */
        }
        catch (Exception e)
        {
            _failures++;
            Console.WriteLine($"  FAIL: {what} on {bad} threw {e.GetType().FullName}: {e.Message}");
        }
    }

    /// <summary>Runs every case; returns 0 on success, 1 on any failure.</summary>
    public static int Run()
    {
        BatchFullRange();
        BatchSubRange();
        FloatOverload();
        FillLoop();
        StreamingTier();
        NonFiniteBarStillRejected();

        // Green must also be non-vacuous: a leg that stopped running would
        // otherwise leave "ALL PASS" printing a smaller number nobody reads.
        int n = Num.Length;
        int expected = (1 + n)                  // batch, full range
                     + (1 + 4)                  // batch, sub-range
                     + n                        // the float overload
                     + (1 + n + 1)              // fill: range, rows, open's last
                     + (1 + 3 * (n - 1) + 1)    // open, peek/update/bits, OutRange
                     + 12;                      // Peek+Update reject 3 x 2 slots
        if (_failures == 0 && _checks != expected)
        {
            Console.WriteLine($"DivZeroTest: took {_checks} checks, expected {expected}");
            return 1;
        }

        if (_failures == 0)
        {
            Console.WriteLine($"DivZeroTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"DivZeroTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
