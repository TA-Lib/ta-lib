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

namespace TALib;

/// <summary>Status code returned by the internal indicator cores.</summary>
/// <remarks>The public API surfaces these as exceptions (the guarded wrappers
/// throw on anything but <see cref="Success"/>); the values match the C
/// library's <c>TA_RetCode</c> so cross-language tooling can compare them
/// directly.</remarks>
public enum RetCode
{
    /// <summary>The call succeeded (<c>TA_SUCCESS</c> = 0). A valid range
    /// shorter than the lookback is a success with zero output values, not a
    /// failure.</summary>
    Success = 0,
    /// <summary>A parameter is out of its documented range, or two output
    /// arrays alias each other (<c>TA_BAD_PARAM</c> = 2).</summary>
    BadParam = 2,
    /// <summary>An internal allocation failed (<c>TA_ALLOC_ERR</c> = 3). C parity only, never
    /// returned here: an allocation failure terminates the process (#178).</summary>
    AllocErr = 3,
    /// <summary>A <see cref="TALib.Metadata.FunctionCall"/> was invoked with an
    /// input left unbound (<c>TA_INPUT_NOT_ALL_INITIALIZE</c> = 10). Reachable only
    /// from the dynamic binder — the typed API takes its inputs as
    /// arguments.</summary>
    InputNotAllInitialize = 10,
    /// <summary>A <see cref="TALib.Metadata.FunctionCall"/> was invoked with an
    /// output left unbound (<c>TA_OUTPUT_NOT_ALL_INITIALIZE</c> = 11). Reachable
    /// only from the dynamic binder.</summary>
    OutputNotAllInitialize = 11,
    /// <summary><c>startIdx</c> is negative (<c>TA_OUT_OF_RANGE_START_INDEX</c>
    /// = 12).</summary>
    OutOfRangeStartIndex = 12,
    /// <summary><c>endIdx</c> is negative or precedes <c>startIdx</c>
    /// (<c>TA_OUT_OF_RANGE_END_INDEX</c> = 13).</summary>
    OutOfRangeEndIndex = 13,
    /// <summary>A stream opener was given fewer than <c>lookback + 1</c> bars
    /// (<c>TA_INSUFFICIENT_HISTORY</c> = 17) — the library's one recoverable
    /// condition. Streaming only: a batch range shorter than the lookback is
    /// <see cref="Success"/> with a zero count.</summary>
    InsufficientHistory = 17,
    /// <summary>An unexpected internal error (<c>TA_INTERNAL_ERROR</c> =
    /// 5000).</summary>
    InternalError = 5000,
}
