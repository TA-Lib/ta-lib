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

namespace TALib;

public sealed partial class Core
{
    /// <summary><c>while( j &lt; e ) { a[j] = a[j + 1]; j++; }</c> as one copy.</summary>
    internal static void ShiftDown(Span<double> a, int j, int e) => a.Slice(j + 1, e - j).CopyTo(a.Slice(j));

    /// <inheritdoc cref="ShiftDown(Span{double}, int, int)"/>
    internal static void ShiftDown(Span<int> a, int j, int e) => a.Slice(j + 1, e - j).CopyTo(a.Slice(j));

    /// <summary><c>while( j &gt; e ) { a[j] = a[j - 1]; j--; }</c> as one copy.</summary>
    internal static void ShiftUp(Span<double> a, int j, int e) => a.Slice(e, j - e).CopyTo(a.Slice(e + 1));

    /// <inheritdoc cref="ShiftUp(Span{double}, int, int)"/>
    internal static void ShiftUp(Span<int> a, int j, int e) => a.Slice(e, j - e).CopyTo(a.Slice(e + 1));
}
