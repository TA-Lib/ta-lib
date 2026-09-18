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
 *  080226 MF,CC  First Version — the float-overload widening contract,
 *                ported from the Java SMathOverflowTest.
 */

/* Hand-written test; ta_codegen never opens this file. */

using System;
using TALib;

namespace TALib.Test;

/// <summary>
/// The <c>TA_S_*</c> float-input contract (PR #33): a float overload must do its
/// arithmetic in double, not in float.
/// </summary>
/// <remarks>
/// <para>Every float variant takes <c>float[]</c> inputs but writes a
/// <c>double[]</c> output. C#, like Java, evaluates <c>float * float</c> in
/// float, so without a widening cast the result overflows to infinity BEFORE
/// reaching the double output (<c>3e38f * 10f</c> -> inf). The generated C#
/// casts the first operand — <c>(double)inReal[i]</c> — which performs the
/// arithmetic in double, so a result beyond <c>float.MaxValue</c> (~3.4e38)
/// survives.</para>
/// <para>This is the one part of the C# surface no cross-language gate covers:
/// <c>--codegen</c> and <c>--xlang-hash</c> drive the double entry points only,
/// so the float overloads were compiled but numerically unexercised. Each case
/// feeds float operands whose exact result overflows float range and asserts the
/// output is finite and equal to the same operation done in double.</para>
/// </remarks>
public static class SMathOverflowTest
{
    private static int _failures;
    private static int _checks;

    private static void CheckFiniteAndEqual(string op, OutRange r, double value, double expected)
    {
        _checks++;
        string? bad = null;
        if (r.Count != 1)
        {
            bad = $"expected one value, got {r.Count}";
        }
        else if (double.IsInfinity(value) || double.IsNaN(value))
        {
            bad = $"overflowed to non-finite {value} (float arithmetic before widening)";
        }
        else if (Math.Abs(value - expected) > Math.Abs(expected) * 1e-9)
        {
            bad = $"value {value} != expected {expected}";
        }

        if (bad != null)
        {
            _failures++;
            Console.WriteLine($"  FAIL: {op} {bad}");
        }
    }

    private static void AddFloatOverflow()
    {
        var core = new Core();
        float[] a = { 3.0e38f }, b = { 3.0e38f };   // 6e38 > float.MaxValue
        double[] o = { -1.0 };
        OutRange r = core.Add(0, 0, a, b, o);
        CheckFiniteAndEqual("ADD", r, o[0], (double)a[0] + (double)b[0]);
    }

    private static void SubFloatOverflow()
    {
        var core = new Core();
        float[] a = { 3.0e38f }, b = { -3.0e38f };  // 6e38 > float.MaxValue
        double[] o = { -1.0 };
        OutRange r = core.Sub(0, 0, a, b, o);
        CheckFiniteAndEqual("SUB", r, o[0], (double)a[0] - (double)b[0]);
    }

    private static void MultFloatOverflow()
    {
        var core = new Core();
        float[] a = { 3.0e38f }, b = { 10.0f };     // 3e39 > float.MaxValue (PR #33)
        double[] o = { -1.0 };
        OutRange r = core.Mult(0, 0, a, b, o);
        CheckFiniteAndEqual("MULT", r, o[0], (double)a[0] * (double)b[0]);
    }

    private static void DivFloatOverflow()
    {
        var core = new Core();
        float[] a = { 3.0e38f }, b = { 1.0e-3f };   // 3e41 > float.MaxValue
        double[] o = { -1.0 };
        OutRange r = core.Div(0, 0, a, b, o);
        CheckFiniteAndEqual("DIV", r, o[0], (double)a[0] / (double)b[0]);
    }

    /// <summary>
    /// A windowed indicator, not just the two-operand math ops: SMA's float
    /// overload accumulates a running total, so a float accumulator would
    /// overflow across the window even though no single element does.
    /// </summary>
    private static void SmaFloatAccumulatorStaysInDouble()
    {
        var core = new Core();
        var input = new float[10];
        for (int i = 0; i < input.Length; i++)
        {
            input[i] = 3.0e38f;                     // sum over 10 bars = 3e39 > float.MaxValue
        }
        var output = new double[10];

        OutRange r = core.Sma(0, input.Length - 1, input, 10, output);

        _checks++;
        if (r.Count != 1)
        {
            _failures++;
            Console.WriteLine($"  FAIL: SMA(float) expected one value, got {r.Count}");
        }
        else if (double.IsInfinity(output[0]) || double.IsNaN(output[0]))
        {
            _failures++;
            Console.WriteLine($"  FAIL: SMA(float) overflowed to {output[0]} "
                              + "(float accumulator across the window)");
        }
        else if (Math.Abs(output[0] - 3.0e38) > 3.0e38 * 1e-6)
        {
            _failures++;
            Console.WriteLine($"  FAIL: SMA(float) value {output[0]} != expected ~3e38");
        }
    }

    /// <summary>Runs every case; returns 0 on success, 1 on any failure.</summary>
    public static int Run()
    {
        AddFloatOverflow();
        SubFloatOverflow();
        MultFloatOverflow();
        DivFloatOverflow();
        SmaFloatAccumulatorStaysInDouble();

        if (_failures == 0)
        {
            Console.WriteLine($"SMathOverflowTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"SMathOverflowTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
