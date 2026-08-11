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

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  081026 MF,CC  First Version -- the unstable-period builder (#186).
 *
 * ta_codegen never opens this file: it is hand-written scaffolding living one
 * level above library/src/, which is the generated tree.
 */

using System;

namespace TALib;

/// <summary>Builds a <see cref="Core"/> with non-default settings.</summary>
/// <remarks>
/// <para>Obtain one from <see cref="Core.Builder"/> (TA-Lib defaults) or
/// <see cref="Core.ToBuilder"/> (seeded from an existing <c>Core</c>), configure
/// it, then call <see cref="Build"/>:</para>
/// <code>
/// Core core = Core.Builder()
///     .UnstablePeriod(FuncUnstId.EMA, 10)
///     .Build();
/// </code>
/// <para>Setters throw rather than return a code, which is the house contract
/// here — no public C# path surfaces a <see cref="RetCode"/>. Rust's peer of
/// this type reports the same rejection as <c>RetCode::BadParam</c> from its
/// <c>build()</c>, because a Rust setter cannot throw; the domain being enforced
/// is identical in both.</para>
/// <para>A builder is <b>not</b> thread-safe (the standard builder contract —
/// confine it to one thread); the <c>Core</c> it produces is.</para>
/// <para>Candlestick thresholds are not configurable from C# yet — this builder
/// covers the unstable period only.</para>
/// </remarks>
public sealed class CoreBuilder
{
    /* Sized by the id count, so the ALL wildcard gets no slot (#144). */
    private readonly int[] unstablePeriod;

    /// <summary>A builder seeded with TA-Lib's defaults.</summary>
    public CoreBuilder()
    {
        unstablePeriod = new int[FuncUnstIds.Count];
    }

    /* Seeded from an existing Core's settings (see Core.ToBuilder). */
    internal CoreBuilder(int[] seed)
    {
        unstablePeriod = (int[])seed.Clone();
    }

    /// <summary>Sets the unstable period for one function, or for every
    /// function at once when given <see cref="FuncUnstId.ALL"/>.</summary>
    /// <param name="id">The function to configure, or <see cref="FuncUnstId.ALL"/>
    /// as the set-all wildcard, mirroring C's <c>TA_SetUnstablePeriod</c>.</param>
    /// <param name="period">Extra warm-up bars, in <c>0</c>..<see cref="Core.MAX_INDEX"/>.</param>
    /// <returns>This builder, for chaining.</returns>
    /// <exception cref="ArgumentOutOfRangeException"><paramref name="period"/> is
    /// negative or above <see cref="Core.MAX_INDEX"/>, or <paramref name="id"/>
    /// is neither a function id nor the wildcard. A rejected call writes
    /// nothing.</exception>
    public CoreBuilder UnstablePeriod(FuncUnstId id, int period)
    {
        /* The period is added to a lookback which is then used as an index, so an
         * unbounded one overflows that lookback negative and the function indexes
         * far past the end of its input. MAX_INDEX is the ceiling the index space
         * already enforces on startIdx/endIdx; a warm-up longer than the largest
         * addressable series could never produce output, so nothing legitimate is
         * refused. C applies the same bound in TA_SetUnstablePeriod.
         */
        if (period < 0 || period > Core.MAX_INDEX)
        {
            throw new ArgumentOutOfRangeException(nameof(period), period,
                "unstable period must be in 0.." + Core.MAX_INDEX);
        }

        /* A C# enum is NOT a closed domain -- (FuncUnstId)(-1) and (FuncUnstId)9999
         * are representable values a caller can pass, and both index off the end of
         * the table. Java can lean on its enum type here; C# cannot, so the id is
         * range-checked numerically exactly as C does with its unsigned compare.
         */
        if (id == FuncUnstId.ALL)
        {
            for (int i = 0; i < unstablePeriod.Length; i++)
            {
                unstablePeriod[i] = period;
            }
            return this;
        }

        int slot = (int)id;
        if (slot < 0 || slot >= FuncUnstIds.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(id), id,
                "not a function id, and not the ALL wildcard");
        }
        unstablePeriod[slot] = period;
        return this;
    }

    /// <summary>Produces the immutable <see cref="Core"/>. The builder stays
    /// usable afterwards, and the <c>Core</c> does not alias it.</summary>
    /// <returns>A configured <see cref="Core"/>.</returns>
    public Core Build()
    {
        return new Core(this);
    }

    /* Defensive copy handed to Core's constructor -- the builder keeps its own
     * array, so later builder calls cannot reach into a built Core. */
    internal int[] SnapshotUnstablePeriod()
    {
        return (int[])unstablePeriod.Clone();
    }
}
