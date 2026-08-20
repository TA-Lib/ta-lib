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

/// <summary>A stream could not be opened because the history given is shorter
/// than the indicator needs to produce its first value.</summary>
/// <remarks>
/// <para>An indicator consumes a number of leading bars — its <em>lookback</em>
/// — before it can produce anything, so opening a stream needs at least
/// <c>lookback + 1</c> bars. This is the one routine, data-dependent way an
/// open fails, which is why it has its own type: it is catchable separately
/// from the programming errors (a parameter out of range, two outputs sharing
/// one array) that arrive as a plain <see cref="ArgumentException"/>. It stays an
/// <see cref="ArgumentException"/> by inheritance, so an existing <c>catch</c>
/// keeps working, and it reports <see cref="RetCode.InsufficientHistory"/>
/// through <see cref="ITaLibFailure.RetCode"/>.</para>
/// <para>It is knowable in advance: query the matching <c>*_Lookback</c> method
/// and feed one more bar than it returns.</para>
/// <para>Deliberately not serializable. Stream handles are not serializable
/// either — the sanctioned checkpoint story is to retain the history and
/// re-open, which is bit-identical by contract — and adding the legacy
/// <c>SerializationInfo</c> constructor here would be an obsolete API
/// (SYSLIB0051) that this project builds as an error.</para>
/// </remarks>
public sealed class InsufficientHistoryException : TaLibArgumentException
{
    /// <summary>Create the exception with a message.</summary>
    /// <param name="message">What was too short, carrying the
    /// <c>"&lt;NAME&gt; open: "</c> prefix the other language bindings use.</param>
    public InsufficientHistoryException(string message)
        : base(message, RetCode.InsufficientHistory)
    {
    }

    /// <summary>Create the exception with a message and the offending parameter
    /// name.</summary>
    /// <param name="message">What was too short.</param>
    /// <param name="paramName">The parameter that was too short.</param>
    public InsufficientHistoryException(string message, string? paramName)
        : base(message, paramName, RetCode.InsufficientHistory)
    {
    }

    /// <summary>Create the exception with a message and an underlying
    /// cause.</summary>
    /// <param name="message">What was too short.</param>
    /// <param name="innerException">The underlying cause.</param>
    public InsufficientHistoryException(string message, Exception? innerException)
        : base(message, innerException, RetCode.InsufficientHistory)
    {
    }
}
