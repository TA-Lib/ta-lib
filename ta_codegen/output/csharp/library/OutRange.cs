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

using System.Globalization;

namespace TALib;

/// <summary>The output range an indicator call wrote: where the values start
/// and how many there are.</summary>
/// <remarks>An indicator consumes a number of leading bars (its lookback)
/// before producing output, so the first value lands at input index
/// <see cref="BegIdx"/> and output index 0. Nothing outside the range is
/// touched — the library never pads with NaN. <see cref="Count"/> of 0 is a
/// legitimate result (a valid range that ends before the lookback), not an
/// error.</remarks>
/// <param name="BegIdx">Input index of the first output value.</param>
/// <param name="Count">Number of values written, starting at output index
/// 0.</param>
public readonly record struct OutRange(int BegIdx, int Count)
{
    /// <summary>True when no values were written.</summary>
    public bool IsEmpty => Count == 0;

    /// <summary>The range that wrote nothing.</summary>
    /// <remarks>A valid range that ends before the indicator's lookback
    /// produces no values, and that is this. Equal to <c>default(OutRange)</c> — named so
    /// that reads say what they mean. A live stream never reports it: a
    /// successful open has already produced at least one value.</remarks>
    public static OutRange Empty => default;

    /// <summary>For example <c>OutRange { BegIdx = 3, Count = 7 }</c>, the same
    /// in every culture.</summary>
    /// <returns>The two components.</returns>
    public override string ToString() =>
        string.Create(CultureInfo.InvariantCulture, $"OutRange {{ BegIdx = {BegIdx}, Count = {Count} }}");
}
