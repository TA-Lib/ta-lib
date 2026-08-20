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

/// <summary>Implemented by every exception this library raises, so the
/// condition that was reported can be recovered as the <see cref="TALib.RetCode"/>
/// C would have returned for the same call.</summary>
/// <remarks>
/// <para>The exception <i>types</i> stay the ones the API documents — a rejected
/// index is still an <see cref="ArgumentOutOfRangeException"/>, a bad parameter
/// still an <see cref="ArgumentException"/> — because that is what a caller
/// catches. What the types cannot carry is <i>which</i> condition: one
/// <see cref="InvalidOperationException"/> serves both an allocation failure and
/// an internal error. This interface is what makes them separable again, without
/// narrowing the catch types.</para>
/// <para>The mapping is <b>total</b> over the batch and streaming tiers — every
/// failure a call to an indicator raises implements it, including the length
/// checks C cannot make (they report the catch-all, the code C uses for an
/// argument it can detect) — and <b>lossless</b>: distinct codes never share one
/// thrown representation.</para>
/// <para>Outside it, deliberately: <see cref="CoreBuilder"/> and the
/// <c>TALib.Metadata</c> binder still raise plain .NET types. Neither is an
/// indicator call, so neither has a <see cref="TALib.RetCode"/> to carry.</para>
/// </remarks>
public interface ITaLibFailure
{
    /// <summary>The condition reported, as the code C would have returned.</summary>
    RetCode RetCode { get; }
}

/// <summary>An argument was rejected: an optional parameter outside its
/// documented range, two outputs sharing one buffer, or a span too short for the
/// values the call would read or write.</summary>
public class TaLibArgumentException : ArgumentException, ITaLibFailure
{
    private readonly RetCode _retCode;

    internal TaLibArgumentException(string message, RetCode retCode)
        : base(message)
    {
        _retCode = retCode;
    }

    internal TaLibArgumentException(string message, string? paramName, RetCode retCode)
        : base(message, paramName)
    {
        _retCode = retCode;
    }

    internal TaLibArgumentException(string message, Exception? innerException, RetCode retCode)
        : base(message, innerException)
    {
        _retCode = retCode;
    }

    /// <inheritdoc/>
    public RetCode RetCode => _retCode;
}

/// <summary><c>startIdx</c> or <c>endIdx</c> is outside
/// <c>[0, Core.MAX_INDEX]</c>, or <c>endIdx</c> precedes <c>startIdx</c>.</summary>
/// <remarks><see cref="ITaLibFailure.RetCode"/> distinguishes the two, which the
/// exception's <c>ParamName</c> can only hint at.</remarks>
public class TaLibArgumentOutOfRangeException : ArgumentOutOfRangeException, ITaLibFailure
{
    private readonly RetCode _retCode;

    internal TaLibArgumentOutOfRangeException(string paramName, string message, RetCode retCode)
        : base(paramName, message)
    {
        _retCode = retCode;
    }

    /// <inheritdoc/>
    public RetCode RetCode => _retCode;
}

/// <summary>The library failed for a reason that is not the caller's argument:
/// an allocation, or an invariant it owns.</summary>
/// <remarks>Neither is expected in normal use — an allocation failure aborts the
/// process long before it reaches here.</remarks>
public class TaLibInvalidOperationException : InvalidOperationException, ITaLibFailure
{
    private readonly RetCode _retCode;

    internal TaLibInvalidOperationException(string message, RetCode retCode)
        : base(message)
    {
        _retCode = retCode;
    }

    /// <inheritdoc/>
    public RetCode RetCode => _retCode;
}
