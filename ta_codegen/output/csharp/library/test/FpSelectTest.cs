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
 *  092326 MF,CC  First Version (issue #436).
 */

/* Hand-written test; ta_codegen never opens this file. */

using System;
using TALib;

namespace TALib.Test;

/// <summary>
/// Every double helper in <c>FpSelect.cs</c> returns the bits of the C select it replaces,
/// over NaN, signed zero, infinities and subnormals as well as ordinary values.
/// </summary>
/// <remarks>The reference for each helper is the plain C# ternary, which has
/// the C semantics.</remarks>
public static class FpSelectTest
{
    private static int _failures;
    private static int _checks;

    private static readonly double[] Specials =
    {
        0.0, -0.0, 1.0, -1.0, 0.5, double.PositiveInfinity, double.NegativeInfinity,
        BitConverter.Int64BitsToDouble(0x7FF8_0000_0000_0000),
        BitConverter.Int64BitsToDouble(unchecked((long)0xFFF8_0000_0000_0000)),
        BitConverter.Int64BitsToDouble(0x7FF8_0000_0000_1234),
        BitConverter.Int64BitsToDouble(0x7FF0_0000_0000_0001),
        BitConverter.Int64BitsToDouble(unchecked((long)0xFFF0_0000_0000_0001)),
        double.Epsilon, -double.Epsilon,
        BitConverter.Int64BitsToDouble(0x000F_FFFF_FFFF_FFFF), 2.2250738585072014E-308,
        double.MaxValue, -double.MaxValue,
    };

    private static void Check(string name, double a, double b, double x, double got, double want)
    {
        _checks++;
        if (BitConverter.DoubleToInt64Bits(got) != BitConverter.DoubleToInt64Bits(want))
        {
            if (_failures++ < 10)
            {
                Console.WriteLine($"  FAIL: {name}({a:R}, {b:R}, {x:R}) = {got:R}, C gives {want:R}");
            }
        }
    }

    private static void CheckAll(double a, double b, double x)
    {
        Check("MaxGt", a, b, x, Core.MaxGt(a, b), a > b ? a : b);
        Check("MinLt", a, b, x, Core.MinLt(a, b), a < b ? a : b);
        Check("KeepIfGt", a, b, x, Core.KeepIfGt(a, b, x), a > b ? x : 0.0);
        Check("KeepIfLt", a, b, x, Core.KeepIfLt(a, b, x), a < b ? x : 0.0);
        Check("ZeroIfGt", a, b, x, Core.ZeroIfGt(a, b, x), a > b ? 0.0 : x);
        Check("ZeroIfLt", a, b, x, Core.ZeroIfLt(a, b, x), a < b ? 0.0 : x);
    }

    // The C rule written out, so the no-intrinsics path (where the helper IS the
    // ternary) is still checked against C rather than against itself.
    private static void PinnedToC()
    {
        double nan = BitConverter.Int64BitsToDouble(0x7FF8_0000_0000_1234);
        Check("MaxGt", 0.0, -0.0, 0, Core.MaxGt(0.0, -0.0), -0.0);
        Check("MaxGt", -0.0, 0.0, 0, Core.MaxGt(-0.0, 0.0), 0.0);
        Check("MaxGt", 1.0, nan, 0, Core.MaxGt(1.0, nan), nan);
        Check("MaxGt", nan, 1.0, 0, Core.MaxGt(nan, 1.0), 1.0);
        Check("MinLt", 0.0, -0.0, 0, Core.MinLt(0.0, -0.0), -0.0);
        Check("MinLt", -0.0, 0.0, 0, Core.MinLt(-0.0, 0.0), 0.0);
        Check("MinLt", 1.0, nan, 0, Core.MinLt(1.0, nan), nan);
        Check("MinLt", nan, 1.0, 0, Core.MinLt(nan, 1.0), 1.0);
        Check("KeepIfGt", nan, 0.0, 5.0, Core.KeepIfGt(nan, 0.0, 5.0), 0.0);
        Check("KeepIfLt", -0.0, 0.0, 5.0, Core.KeepIfLt(-0.0, 0.0, 5.0), 0.0);
        Check("ZeroIfGt", nan, 0.0, 5.0, Core.ZeroIfGt(nan, 0.0, 5.0), 5.0);
        Check("ZeroIfLt", -0.0, 0.0, 5.0, Core.ZeroIfLt(-0.0, 0.0, 5.0), 5.0);
    }

    /// <summary>Runs every case; returns 0 on success, 1 on any failure.</summary>
    public static int Run()
    {
        PinnedToC();
        foreach (double a in Specials)
        {
            foreach (double b in Specials)
            {
                foreach (double x in Specials)
                {
                    CheckAll(a, b, x);
                }
            }
        }

        // Arbitrary bit patterns, each paired with itself and with its negation
        // so ties and signed-zero-like pairs are not left to chance.
        ulong s = 0x9E37_79B9_7F4A_7C15;
        for (int i = 0; i < 200_000; i++)
        {
            s = s * 6364136223846793005UL + 1442695040888963407UL;
            double a = BitConverter.Int64BitsToDouble((long)s);
            s = s * 6364136223846793005UL + 1442695040888963407UL;
            double b = BitConverter.Int64BitsToDouble((long)s);
            CheckAll(a, b, a);
            CheckAll(a, a, b);
            CheckAll(a, -a, b);
        }

        if (_failures == 0)
        {
            Console.WriteLine($"FpSelectTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"FpSelectTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
