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
using System.Runtime.InteropServices;
using System.Runtime.Intrinsics;

namespace TALib;

public sealed partial class Core
{
    /// <summary>
    /// <c>for( ; from &lt; end; from++, at++ ) dst[at] = Math.Sqrt(src[from]);</c>
    /// two elements at a time, returning how many it stored.
    /// </summary>
    /// <remarks>
    /// A pair is read whole before it is stored, which only matches the loop
    /// when no store lands on an element still to be read: a destination that
    /// starts inside the source, past its first element, takes the loop.
    /// </remarks>
    [MethodImpl(MethodImplOptions.AggressiveOptimization)]
    internal static int SqrtRun(ReadOnlySpan<double> src, int from, int end, Span<double> dst, int at)
    {
        int n = end - from;
        if( n <= 0 ) return 0;
        ReadOnlySpan<double> s = src.Slice(from, n);
        Span<double> d = dst.Slice(at, n);
        int k = 0;
        if( !StoresAhead(s, d) )
        {
            for( ; k <= n - Vector128<double>.Count; k += Vector128<double>.Count )
                Vector128.Sqrt(Vector128.Create(s.Slice(k))).CopyTo(d.Slice(k));
        }
        for( ; k < n; k++ )
        {
            double x = s[k];
            d[k] = Math.Sqrt(x);
        }
        return n;
    }

    /// <summary>As the unscaled form, storing <c>Math.Sqrt(src[from]) * scale</c>.</summary>
    /// <inheritdoc cref="SqrtRun(ReadOnlySpan{double}, int, int, Span{double}, int)" path="/remarks"/>
    [MethodImpl(MethodImplOptions.AggressiveOptimization)]
    internal static int SqrtRun(ReadOnlySpan<double> src, int from, int end, Span<double> dst, int at, double scale)
    {
        int n = end - from;
        if( n <= 0 ) return 0;
        ReadOnlySpan<double> s = src.Slice(from, n);
        Span<double> d = dst.Slice(at, n);
        int k = 0;
        if( !StoresAhead(s, d) )
        {
            for( ; k <= n - Vector128<double>.Count; k += Vector128<double>.Count )
                (Vector128.Sqrt(Vector128.Create(s.Slice(k))) * scale).CopyTo(d.Slice(k));
        }
        for( ; k < n; k++ )
        {
            double x = s[k];
            d[k] = Math.Sqrt(x) * scale;
        }
        return n;
    }

    private static bool StoresAhead(ReadOnlySpan<double> s, Span<double> d)
    {
        nint lead = Unsafe.ByteOffset(ref MemoryMarshal.GetReference(s), ref MemoryMarshal.GetReference(d));
        return lead > 0 && lead < (nint)s.Length * sizeof(double);
    }
}
