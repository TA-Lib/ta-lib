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
 *  100626 MF,CC  First Version (issue #506).
 */

/* Hand-written test; ta_codegen never opens this file. */

using System;
using TALib;

namespace TALib.Test;

/// <summary>
/// <c>SqrtRun</c> stores the bits of the element loop it replaces, wherever the
/// destination sits against the source: apart, on it, behind it or ahead of it.
/// </summary>
public static class SqrtRunTest
{
    private static int _failures;
    private static int _checks;

    private static readonly double[] Values =
    {
        2.0, 0.0, -0.0, 9.0, -1.0, double.PositiveInfinity, double.NegativeInfinity, double.NaN,
        BitConverter.Int64BitsToDouble(0x7FF0_0000_0000_0001), double.Epsilon, 1e-310, 3e37, 0.1, 123.45, 1e-300, 7.0,
    };

    private static void Compare(string what, double[] got, double[] want, double scale)
    {
        for (int i = 0; i < want.Length; i++)
        {
            _checks++;
            // Which NaN a product of two NaNs keeps follows the operand order the
            // JIT picked, in the element loop as much as here.
            bool same = double.IsNaN(scale) && double.IsNaN(want[i])
                ? double.IsNaN(got[i])
                : BitConverter.DoubleToInt64Bits(got[i]) == BitConverter.DoubleToInt64Bits(want[i]);
            if (!same)
            {
                _failures++;
                Console.WriteLine($"FAIL {what}: [{i}] got {got[i]:R}, want {want[i]:R}");
            }
        }
    }

    // One array holds both runs, so `from` against `at` places the destination
    // behind, on or ahead of the source.
    private static void OneArray(int from, int at, int n, double scale)
    {
        double[] want = new double[Values.Length];
        double[] got = new double[Values.Length];
        Values.CopyTo(want, 0);
        Values.CopyTo(got, 0);
        for (int i = 0; i < n; i++)
        {
            double x = want[from + i];
            want[at + i] = double.IsNegativeInfinity(scale) ? Math.Sqrt(x) : Math.Sqrt(x) * scale;
        }
        int stored = double.IsNegativeInfinity(scale)
            ? Core.SqrtRun(got, from, from + n, got, at)
            : Core.SqrtRun(got, from, from + n, got, at, scale);
        string what = $"one array from={from} at={at} n={n} scale={scale:R}";
        Compare(what, got, want, scale);
        _checks++;
        if (stored != Math.Max(n, 0))
        {
            _failures++;
            Console.WriteLine($"FAIL {what}: stored {stored}");
        }
    }

    private static void TwoArrays(int from, int at, int n, double scale)
    {
        double[] want = new double[Values.Length];
        double[] got = new double[Values.Length];
        for (int i = 0; i < n; i++)
        {
            double x = Values[from + i];
            want[at + i] = double.IsNegativeInfinity(scale) ? Math.Sqrt(x) : Math.Sqrt(x) * scale;
        }
        _ = double.IsNegativeInfinity(scale)
            ? Core.SqrtRun(Values, from, from + n, got, at)
            : Core.SqrtRun(Values, from, from + n, got, at, scale);
        Compare($"two arrays from={from} at={at} n={n} scale={scale:R}", got, want, scale);
    }

    /// <summary>Runs every case; returns 0 on success, 1 on any failure.</summary>
    public static int Run()
    {
        // Negative infinity stands for the overload without a scale.
        double[] scales =
        {
            double.NegativeInfinity, 1.5, 0.0, double.PositiveInfinity, double.NaN,
        };
        foreach (double scale in scales)
        {
            for (int n = -1; n <= 9; n++)
            {
                for (int from = 0; from <= 4; from++)
                {
                    for (int at = 0; at <= 4; at++)
                    {
                        OneArray(from, at, n, scale);
                        TwoArrays(from, at, n, scale);
                    }
                }
            }
        }

        if (_failures == 0)
        {
            Console.WriteLine($"SqrtRunTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"SqrtRunTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
