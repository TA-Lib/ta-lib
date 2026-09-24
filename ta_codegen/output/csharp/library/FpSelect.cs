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

/* Hand-written library scaffolding; ta_codegen never opens this file. */

using System;
using System.Runtime.CompilerServices;
using System.Runtime.Intrinsics;
using System.Runtime.Intrinsics.Arm;
using System.Runtime.Intrinsics.X86;

namespace TALib;

// Each helper returns the bits of the C select it stands for, NaN and signed
// zero included: MAXSD/MINSD answer the second operand on a tie or a NaN, as
// `a > b ? a : b` does, and so does a bit select on FCMGT's mask. Math.Max and
// Math.Min over doubles do not, so they are no substitute. Every helper needs
// its own FpSelectTest row: no other gate feeds a select a NaN or a signed-zero
// tie.
public sealed partial class Core
{
    /// <summary><c>a &gt; b ? a : b</c>, the C <c>max</c> macro.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static double MaxGt(double a, double b)
    {
        if (Sse2.IsSupported)
        {
            return Sse2.MaxScalar(Lo128(a), Lo128(b)).ToScalar();
        }
        if (AdvSimd.Arm64.IsSupported)
        {
            return AdvSimd.BitwiseSelect(AdvSimd.Arm64.CompareGreaterThanScalar(Lo64(a), Lo64(b)), Lo64(a), Lo64(b)).ToScalar();
        }
        return a > b ? a : b;
    }

    /// <summary><c>a &lt; b ? a : b</c>, the C <c>min</c> macro.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static double MinLt(double a, double b)
    {
        if (Sse2.IsSupported)
        {
            return Sse2.MinScalar(Lo128(a), Lo128(b)).ToScalar();
        }
        if (AdvSimd.Arm64.IsSupported)
        {
            return AdvSimd.BitwiseSelect(AdvSimd.Arm64.CompareLessThanScalar(Lo64(a), Lo64(b)), Lo64(a), Lo64(b)).ToScalar();
        }
        return a < b ? a : b;
    }

    // Over integers every tie is the same bits, and Math.Max/Math.Min are what
    // the JIT's range analysis understands: a ternary here costs lookbacks
    // their bounds-check elimination downstream.

    /// <summary><c>a &gt; b ? a : b</c> over integers.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static int MaxGt(int a, int b) => Math.Max(a, b);

    /// <summary><c>a &lt; b ? a : b</c> over integers.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static int MinLt(int a, int b) => Math.Min(a, b);

    /// <summary><c>a &gt; b ? a : b</c> over integers.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static long MaxGt(long a, long b) => Math.Max(a, b);

    /// <summary><c>a &lt; b ? a : b</c> over integers.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static long MinLt(long a, long b) => Math.Min(a, b);

    // SSE2 has no greater-than compare: `a > b` is spelled `b < a`, which spares
    // the JIT's emulation a register merge on hardware without AVX.

    /// <summary><c>a &gt; b ? x : 0.0</c>.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static double KeepIfGt(double a, double b, double x)
    {
        if (Sse2.IsSupported)
        {
            return Sse2.And(Sse2.CompareScalarLessThan(Lo128(b), Lo128(a)), Lo128(x)).ToScalar();
        }
        if (AdvSimd.Arm64.IsSupported)
        {
            return AdvSimd.And(AdvSimd.Arm64.CompareGreaterThanScalar(Lo64(a), Lo64(b)), Lo64(x)).ToScalar();
        }
        return a > b ? x : 0.0;
    }

    /// <summary><c>a &lt; b ? x : 0.0</c>.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static double KeepIfLt(double a, double b, double x)
    {
        if (Sse2.IsSupported)
        {
            return Sse2.And(Sse2.CompareScalarLessThan(Lo128(a), Lo128(b)), Lo128(x)).ToScalar();
        }
        if (AdvSimd.Arm64.IsSupported)
        {
            return AdvSimd.And(AdvSimd.Arm64.CompareLessThanScalar(Lo64(a), Lo64(b)), Lo64(x)).ToScalar();
        }
        return a < b ? x : 0.0;
    }

    /// <summary><c>a &gt; b ? 0.0 : x</c>.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static double ZeroIfGt(double a, double b, double x)
    {
        if (Sse2.IsSupported)
        {
            return Sse2.AndNot(Sse2.CompareScalarLessThan(Lo128(b), Lo128(a)), Lo128(x)).ToScalar();
        }
        if (AdvSimd.Arm64.IsSupported)
        {
            return AdvSimd.BitwiseClear(Lo64(x), AdvSimd.Arm64.CompareGreaterThanScalar(Lo64(a), Lo64(b))).ToScalar();
        }
        return a > b ? 0.0 : x;
    }

    /// <summary><c>a &lt; b ? 0.0 : x</c>.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    internal static double ZeroIfLt(double a, double b, double x)
    {
        if (Sse2.IsSupported)
        {
            return Sse2.AndNot(Sse2.CompareScalarLessThan(Lo128(a), Lo128(b)), Lo128(x)).ToScalar();
        }
        if (AdvSimd.Arm64.IsSupported)
        {
            return AdvSimd.BitwiseClear(Lo64(x), AdvSimd.Arm64.CompareLessThanScalar(Lo64(a), Lo64(b))).ToScalar();
        }
        return a < b ? 0.0 : x;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static Vector128<double> Lo128(double x) => Vector128.CreateScalarUnsafe(x);

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static Vector64<double> Lo64(double x) => Vector64.CreateScalarUnsafe(x);
}
