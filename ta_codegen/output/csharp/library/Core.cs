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

/* Hand-written library scaffolding; ta_codegen never opens this file. The
 * generated per-indicator methods live in src/Core_<NAME>.cs — each a
 * `partial class Core` piece of this type. */

using System;

namespace TALib;

/// <summary>All TA-Lib indicators, as instance methods on this class.</summary>
/// <remarks>
/// Every indicator follows the same pattern: inputs are <c>double[]</c> (or
/// <c>float[]</c> overloads), computed over the bar range
/// <c>startIdx..endIdx</c> inclusive; outputs are written into caller-provided
/// arrays; the returned <see cref="OutRange"/> reports the input index of the
/// first output value and how many were written. An indicator consumes a
/// number of leading bars (its <em>lookback</em>) before producing output —
/// query it with the matching <c>*Lookback</c> method. Integer parameters
/// accept <c>int.MinValue</c>, and real parameters <c>-4e37</c>, to select
/// their documented default.
/// <para>Per-instance settings — unstable periods and candlestick thresholds —
/// take their documented defaults unless chosen up front with
/// <see cref="Builder"/>. A <c>Core</c> whose settings are never mutated is safe
/// to share read-only across threads.</para>
/// </remarks>
public partial class Core
{
    /// <summary>The catalogue of every indicator, for choosing one at run
    /// time.</summary>
    /// <remarks>A discovery hook, so a caller does not have to know the
    /// <c>TALib.Metadata</c> namespace exists. See
    /// <see cref="TALib.Metadata.FunctionCatalog"/>.</remarks>
    public static TALib.Metadata.FunctionCatalog Functions => TALib.Metadata.FunctionCatalog.Default;

    /* The parameter sentinels the generated validation names. Values match
     * the C library's ta_defs.h. */
    internal const double TA_REAL_DEFAULT = -4e37;
    internal const double TA_REAL_MIN = -3e37;
    internal const double TA_REAL_MAX = 3e37;
    internal const int TA_INTEGER_DEFAULT = int.MinValue;
    internal const int TA_INTEGER_MIN = int.MinValue + 1;
    internal const int TA_INTEGER_MAX = int.MaxValue;

    /// <summary>Largest value <c>startIdx</c> or <c>endIdx</c> may take. Above
    /// it a call returns <see cref="RetCode.OutOfRangeStartIndex"/> or
    /// <see cref="RetCode.OutOfRangeEndIndex"/> rather than computing.</summary>
    /// <remarks><para>This bounds the <i>API domain</i> and nothing else — in
    /// particular it is not an accuracy guarantee. A handful of functions
    /// accumulate rounding error that grows with the series length and are
    /// already imprecise well below this cap.</para>
    /// <para>Identical in C, Rust and Java, so the same call is accepted or
    /// rejected the same way in all four.</para></remarks>
    public const int MAX_INDEX = 100000000;

    /* Sized by the id count, so the ALL wildcard gets no slot (#144). */
    internal readonly int[] unstablePeriod = new int[FuncUnstIds.Count];

    /* The 11 defaults, in CandleSettingType order, from
     * TA_RestoreCandleDefaultSettings in ta_global.c. ONE source of truth: both a
     * fresh Core and CoreBuilder.RestoreCandleDefault read this array, so the
     * literals cannot drift between the two the way two copies would. Safe to
     * share rather than clone because CandleSetting is immutable. */
    internal static readonly CandleSetting[] DefaultCandleSettings =
    {
        new CandleSetting(RangeType.RealBody, 10, 1.0),   // BodyLong
        new CandleSetting(RangeType.RealBody, 10, 3.0),   // BodyVeryLong
        new CandleSetting(RangeType.RealBody, 10, 1.0),   // BodyShort
        new CandleSetting(RangeType.HighLow,  10, 0.1),   // BodyDoji
        new CandleSetting(RangeType.RealBody, 0,  1.0),   // ShadowLong
        new CandleSetting(RangeType.RealBody, 0,  2.0),   // ShadowVeryLong
        new CandleSetting(RangeType.Shadows,  10, 1.0),   // ShadowShort
        new CandleSetting(RangeType.HighLow,  10, 0.1),   // ShadowVeryShort
        new CandleSetting(RangeType.HighLow,  5,  0.2),   // Near
        new CandleSetting(RangeType.HighLow,  5,  0.6),   // Far
        new CandleSetting(RangeType.HighLow,  5,  0.05),  // Equal
    };

    /* candleSettings[] in CandleSettingType order. */
    internal readonly CandleSetting[] candleSettings = (CandleSetting[])DefaultCandleSettings.Clone();

    /// <summary>Create a Core with every setting at its documented
    /// default.</summary>
    public Core()
    {
    }

    /* Built through CoreBuilder.Build(). Takes a snapshot rather than the
     * builder's own array, so later builder calls cannot reach in here. */
    internal Core(CoreBuilder builder)
    {
        unstablePeriod = builder.SnapshotUnstablePeriod();
        candleSettings = builder.SnapshotCandleSettings();
    }

    /// <summary>Start building a <c>Core</c> with non-default settings.</summary>
    /// <returns>A builder seeded with TA-Lib's defaults.</returns>
    public static CoreBuilder Builder()
    {
        return new CoreBuilder();
    }

    /// <summary>Seed a builder from this <c>Core</c>'s settings, for
    /// clone-and-modify.</summary>
    /// <returns>A builder carrying this instance's current settings.</returns>
    public CoreBuilder ToBuilder()
    {
        return new CoreBuilder(unstablePeriod, candleSettings);
    }

    /// <summary>Reads one candlestick threshold.</summary>
    /// <param name="settingType">The setting to query.</param>
    /// <returns>Its range type, averaging period and factor.</returns>
    /// <exception cref="ArgumentOutOfRangeException"><paramref name="settingType"/>
    /// is <see cref="CandleSettingType.AllCandleSettings"/>, which is a wildcard
    /// naming no single setting, or is not a setting at all.</exception>
    public CandleSetting CandleSettings(CandleSettingType settingType)
    {
        int slot = (int)settingType;
        if (slot < 0 || slot >= DefaultCandleSettings.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(settingType), settingType,
                "not a single candlestick setting");
        }
        return candleSettings[slot];
    }

    /// <summary>Reads the unstable period configured for one function.</summary>
    /// <param name="id">The function to query.</param>
    /// <returns>Its extra warm-up bars.</returns>
    /// <exception cref="ArgumentOutOfRangeException"><paramref name="id"/> is
    /// <see cref="FuncUnstId.ALL"/>, which is the set-all wildcard and names no
    /// single function, or is not a function id at all.</exception>
    /// <remarks>C's <c>TA_GetUnstablePeriod</c> answers <c>0</c> for the same
    /// input and cannot report an error; <c>0</c> is itself a legal period, so
    /// that answer is indistinguishable from a genuine reading. Java throws here
    /// too.</remarks>
    public int UnstablePeriod(FuncUnstId id)
    {
        int slot = (int)id;
        if (slot < 0 || slot >= FuncUnstIds.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(id), id,
                "not a function with a single unstable period");
        }
        return unstablePeriod[slot];
    }

    /* The requested start after the lookback clamp -- max(startIdx, lookback) --
     * or -1 when the core will reject the call before it reaches the algorithm.
     *
     * -1 means "check no length at all": a negative or out-of-range index,
     * endIdx < startIdx, or the -1 a lookback returns for an out-of-range
     * optional parameter. The core owns that diagnosis and Failure() translates
     * it, so pre-empting it would replace a documented exception with a length
     * complaint.
     *
     * A result ABOVE endIdx is not an error: the range is shorter than the
     * lookback, so the call produces no values. That switches the OUTPUT bound
     * off -- any length will do, including none -- but not the input bound. An
     * endIdx past the end of the series the caller supplied is a caller bug in
     * any range; C answers it with TA_SUCCESS only because it has no size to
     * check against.
     *
     * Same shape and same bound as Java's Core.clampedStart, and as Rust's public
     * wrapper: rust_lang.rs's gen_argument_checks takes the
     * `_assertStart > endIdx ||` escape on the output bound and not on the input
     * one, exactly as here, so SMA(0, 5, &[], 30, &mut []) is Err(BadParam) there
     * and a throw here -- one verdict, spelled two ways. What does take the escape
     * on BOTH bounds is Rust's <N>_Impl assertion preamble, which is pub(crate)
     * and off the public path since #267; that tier is what the JSON-RPC servers
     * exercise, and it is the reason this paragraph used to read "NOT the same as
     * Rust". So C is the one backend that checks less, having no size to check
     * against. It is a diagnostic, not a safety net, since NoPhantomIoTest pins
     * that no core reads on such a range. The Rust crate's tests/empty_range.rs
     * pins the rows named here. */
    internal static int ClampedStart(int startIdx, int endIdx, int lookback)
    {
        if (lookback < 0 || startIdx < 0 || endIdx < startIdx || endIdx > MAX_INDEX)
        {
            return -1;
        }
        return startIdx > lookback ? startIdx : lookback;
    }

    /* Reject a span too short for the values this call would read or write.
     *
     * C cannot make this check -- it is handed bare pointers and has no sizes --
     * and without it an undersized span surfaces as
     * "IndexOutOfRangeException: Index was outside the bounds of the array",
     * from inside the algorithm, with the output already partly written, naming
     * neither the buffer nor either size.
     *
     * Takes the length rather than the span: a Span<T> is a ref struct that can
     * never be null, so there is nothing to check but the count, and one method
     * then serves every element type and both directions. */
    internal static void RequireLength(string funcName, string argName, int actual, int required)
    {
        if (actual < required)
        {
            throw new TaLibArgumentException(
                "TA_" + funcName + ": " + argName + " has length " + actual
                    + ", needs " + required,
                argName, RetCode.BadParam);
        }
    }

    /* Rule S5's bound: how many values an OpenAndFill writes.
     *
     * An opener is a batch call over [0, historyLen - 1], so ClampedStart's
     * produced count -- endIdx - max(startIdx, lookback) + 1 -- collapses to
     * historyLen - lookback. Unlike the batch tier's it has no legitimate zero
     * case: rule S7 refuses a history shorter than lookback + 1, so a fill that
     * runs writes at least one value.
     *
     * A short history is still floored to 0 rather than answered here: S7 has not
     * run yet, and that rejection is S7's to make. A lookback of -1 is NOT
     * floored -- it is the parameter contract's rejection signal, and swallowing
     * it reported an absent output (S4) for a call whose fault was its parameter
     * (S3). Raising it here is what puts S3 ahead of the buffer rules, exactly as
     * ClampedStart does one tier over. */
    internal static int OpenFillCount(string funcName, string verb, int historyLen, int lookback)
    {
        if (lookback < 0)
        {
            throw StreamFailure(funcName, verb, RetCode.BadParam);
        }
        return historyLen <= lookback ? 0 : historyLen - lookback;
    }

    /* Rule S5's input half: every declared input carries the history, so every
     * one of them is the history's length.
     *
     * B5 states the two halves as one rule, inputs first, and this is B5 over
     * [0, historyLen - 1]. The difference from B5's wording is that there is no
     * separate endIdx to reach here: the history's own length IS the range, so a
     * longer series is a disagreement rather than a tail to ignore -- which is
     * what the generated docs have always promised. */
    internal static void RequireHistoryLength(string funcName, string verb, string argName,
                                              int actual, int historyLen)
    {
        if (actual != historyLen)
        {
            throw new TaLibArgumentException(
                funcName + " " + verb + ": " + argName + " has length " + actual
                    + ", needs " + historyLen,
                argName, RetCode.BadParam);
        }
    }

    /* RequireLength for the STREAMING tier, which spells the prefix
     * "<NAME> <verb>: " where the batch tier spells it "TA_<NAME>: ". Same
     * reason StreamFailure is not a reuse of Failure(): the prefix is a
     * cross-language contract the stream gate greps for. */
    internal static void RequireFillLength(string funcName, string verb, string argName,
                                           int actual, int required)
    {
        if (actual < required)
        {
            throw new TaLibArgumentException(
                funcName + " " + verb + ": " + argName + " has length " + actual
                    + ", needs " + required,
                argName, RetCode.BadParam);
        }
    }

    /* The RetCode -> exception mapping for the STREAMING tier. Deliberately not
     * a reuse of Failure(): the two tiers spell the same code differently. A
     * stream CAN still report OutOfRangeEndIndex (a history longer than
     * MAX_INDEX + 1), and Failure() would render that as
     * ArgumentOutOfRangeException("endIdx") — meaningless to a caller whose
     * method has no endIdx parameter.
     *
     * The "<NAME> open: " prefix is a cross-language contract (see
     * docs/streaming-api-design.md) and deliberately differs from Failure()'s
     * "TA_<NAME>: ". Do not unify them. Centralising the mapping here also
     * means the ~520 generated reject sites are one line each instead of four,
     * and the message prefix the stream gate greps has a single source. */
    internal static Exception StreamFailure(string funcName, string what, RetCode retCode)
    {
        string where = funcName + " " + what + ": ";
        return retCode switch
        {
            RetCode.InsufficientHistory => new InsufficientHistoryException(
                where + "history shorter than lookback + 1"),
            RetCode.InternalError => new TaLibInvalidOperationException(where + "internal error", retCode),
            RetCode.AllocErr => new TaLibInvalidOperationException(where + "allocation failed", retCode),
            _ => new TaLibArgumentException(where + retCode, retCode),
        };
    }

    /* The RetCode -> exception mapping the generated guarded wrappers throw
     * through. Returns (rather than throws) so the call sites read
     * `throw Failure(...)` and the compiler knows the path ends.
     *
     * Every type returned here implements ITaLibFailure, so the code is
     * recoverable from the thrown object. The exception types are coarser than
     * the codes -- one InvalidOperationException serves both AllocErr and
     * InternalError -- and a caller that cannot tell them apart cannot respond
     * to either. */
    internal static Exception Failure(string funcName, RetCode retCode)
    {
        string where = "TA_" + funcName + ": ";
        switch (retCode)
        {
            case RetCode.OutOfRangeStartIndex:
                return new TaLibArgumentOutOfRangeException("startIdx", where + "startIdx out of range", retCode);
            case RetCode.OutOfRangeEndIndex:
                return new TaLibArgumentOutOfRangeException("endIdx", where + "endIdx out of range", retCode);
            case RetCode.BadParam:
                return new TaLibArgumentException(where + "bad parameter", retCode);
            case RetCode.AllocErr:
                return new TaLibInvalidOperationException(where + "allocation failed", retCode);
            case RetCode.InternalError:
                return new TaLibInvalidOperationException(where + "internal error", retCode);
            case RetCode.InsufficientHistory:
                /* Streaming-only in practice: a batch range shorter than the
                 * lookback is Success with a zero count, never this. Mapped
                 * anyway so the code -> exception function stays total. */
                return new InsufficientHistoryException(where + "history shorter than the lookback");
            default:
                return new TaLibInvalidOperationException(where + retCode, retCode);
        }
    }
}
