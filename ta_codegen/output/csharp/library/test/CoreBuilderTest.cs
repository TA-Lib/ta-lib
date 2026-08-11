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
 */

using System;

namespace TALib.Test;

/// <summary>The C# configuration surface: setting an unstable period, and the
/// domain that setting is held to.</summary>
/// <remarks>
/// <para>Ported case-for-case from the Java <c>CoreApiTest</c> so the two
/// managed bindings are held to the same contract, which is in turn the C
/// library's: a period outside <c>0..=MAX_INDEX</c> is refused, and a refused
/// call writes nothing.</para>
/// <para>Framework-free, like the other suites here — discovered by name and
/// run through <c>public static int Run()</c>.</para>
/// </remarks>
public static class CoreBuilderTest
{
    private static int _failures;
    private static int _checks;

    private static void Check(bool condition, string what)
    {
        _checks++;
        if (!condition)
        {
            _failures++;
            Console.WriteLine("  FAIL: " + what);
        }
    }

    private static void CheckThrows<TException>(Action body, string what)
        where TException : Exception
    {
        _checks++;
        try
        {
            body();
            _failures++;
            Console.WriteLine("  FAIL: " + what + " (no exception thrown)");
        }
        catch (Exception e)
        {
            if (e is not TException)
            {
                _failures++;
                Console.WriteLine("  FAIL: " + what + " (threw " + e.GetType().FullName + ")");
            }
        }
    }

    private static void DefaultsAreZero()
    {
        Core core = Core.Builder().Build();
        foreach (FuncUnstId id in Enum.GetValues<FuncUnstId>())
        {
            if (id != FuncUnstId.ALL)
            {
                Check(core.UnstablePeriod(id) == 0, $"{id} defaults to 0");
            }
        }
    }

    private static void SetsOneAndOnlyOne()
    {
        Core core = Core.Builder().UnstablePeriod(FuncUnstId.EMA, 10).Build();
        Check(core.UnstablePeriod(FuncUnstId.EMA) == 10, "EMA was set to 10");
        Check(core.UnstablePeriod(FuncUnstId.RSI) == 0, "RSI was left alone");
    }

    private static void WildcardSetsEveryFunction()
    {
        Core core = Core.Builder().UnstablePeriod(FuncUnstId.ALL, 7).Build();
        Check(core.UnstablePeriod(FuncUnstId.EMA) == 7, "the wildcard reached EMA");
        Check(core.UnstablePeriod(FuncUnstId.T3) == 7, "the wildcard reached T3");
    }

    private static void ReachesTheIndicator()
    {
        // A behavioural witness rather than a field read: the period must move the
        // lookback, or the setter could be storing into something nothing reads.
        Core plain = new Core();
        Core tuned = Core.Builder().UnstablePeriod(FuncUnstId.EMA, 5).Build();
        Check(tuned.EMA_Lookback(10) == plain.EMA_Lookback(10) + 5,
            "the unstable period is added to EMA's lookback");
    }

    private static void MisuseThrows()
    {
        // ArgumentOutOfRangeException, not merely its ArgumentException base:
        // CheckThrows accepts subclasses, so asking for the base would pass
        // against any argument exception and leave the documented type untested.
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().UnstablePeriod(FuncUnstId.RSI, -1),
            "negative period -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().UnstablePeriod(FuncUnstId.RSI, Core.MAX_INDEX + 1),
            "period above MAX_INDEX -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().UnstablePeriod(FuncUnstId.RSI, int.MaxValue),
            "int.MaxValue period -> ArgumentOutOfRangeException");
        CheckThrows<ArgumentOutOfRangeException>(
            () => Core.Builder().UnstablePeriod(FuncUnstId.ALL, Core.MAX_INDEX + 1),
            "wildcard period above MAX_INDEX -> ArgumentOutOfRangeException");
        // Unlike Java, a C# enum is not a closed domain -- (FuncUnstId)(-1) and
        // (FuncUnstId)9999 are representable values a caller can pass, and both
        // index off the end of a 24-slot array. C guards this with an unsigned
        // compare for exactly that reason (#144).
        CheckThrows<ArgumentException>(
            () => Core.Builder().UnstablePeriod((FuncUnstId)(-1), 1),
            "a negative id cast into the enum -> ArgumentException");
        CheckThrows<ArgumentException>(
            () => Core.Builder().UnstablePeriod((FuncUnstId)9999, 1),
            "an out-of-range id cast into the enum -> ArgumentException");
        CheckThrows<ArgumentException>(
            () => new Core().UnstablePeriod(FuncUnstId.ALL),
            "the wildcard has no single value to read -> ArgumentException");
    }

    private static void BoundIsABoundNotAnOffByOne()
    {
        // MAX_INDEX itself is legal: C accepts it and rejects MAX_INDEX + 1, so a
        // guard tightened by one would be caught here rather than shipping.
        Core core = Core.Builder().UnstablePeriod(FuncUnstId.EMA, Core.MAX_INDEX).Build();
        Check(core.UnstablePeriod(FuncUnstId.EMA) == Core.MAX_INDEX,
            "the MAX_INDEX ceiling is accepted, not rejected");
    }

    private static void ARejectedCallWritesNothing()
    {
        // The half of the contract an "it throws" assertion cannot see.
        CoreBuilder b = Core.Builder().UnstablePeriod(FuncUnstId.EMA, 7);
        CheckThrows<ArgumentException>(
            () => b.UnstablePeriod(FuncUnstId.EMA, Core.MAX_INDEX + 1),
            "the rejected overwrite still throws");
        Check(b.Build().UnstablePeriod(FuncUnstId.EMA) == 7,
            "a rejected period leaves the previous value in place");

        // The wildcard path writes 24 slots, so a rejection there must not have
        // filled any of them before noticing.
        CoreBuilder w = Core.Builder().UnstablePeriod(FuncUnstId.ALL, 3);
        CheckThrows<ArgumentException>(
            () => w.UnstablePeriod(FuncUnstId.ALL, int.MaxValue),
            "the rejected wildcard still throws");
        Core after = w.Build();
        bool intact = true;
        foreach (FuncUnstId id in Enum.GetValues<FuncUnstId>())
        {
            if (id != FuncUnstId.ALL && after.UnstablePeriod(id) != 3)
            {
                intact = false;
            }
        }
        Check(intact, "a rejected wildcard leaves all 24 slots at their previous value");
    }

    private static void BuiltCoreIsIsolatedFromTheBuilder()
    {
        CoreBuilder b = Core.Builder().UnstablePeriod(FuncUnstId.EMA, 4);
        Core core = b.Build();
        b.UnstablePeriod(FuncUnstId.EMA, 9);
        Check(core.UnstablePeriod(FuncUnstId.EMA) == 4,
            "a built Core does not alias the builder's array");
    }

    private static void ToBuilderRoundTrips()
    {
        Core original = Core.Builder().UnstablePeriod(FuncUnstId.RSI, 5).Build();
        Core derived = original.ToBuilder().UnstablePeriod(FuncUnstId.EMA, 9).Build();
        Check(original.UnstablePeriod(FuncUnstId.EMA) == 0, "the original is untouched");
        Check(derived.UnstablePeriod(FuncUnstId.RSI) == 5, "the derived Core inherits RSI");
        Check(derived.UnstablePeriod(FuncUnstId.EMA) == 9, "the derived Core gains EMA");
    }

    /// <summary>Runs every check in this suite.</summary>
    /// <returns>0 when they all pass, 1 otherwise.</returns>
    public static int Run()
    {
        DefaultsAreZero();
        SetsOneAndOnlyOne();
        WildcardSetsEveryFunction();
        ReachesTheIndicator();
        MisuseThrows();
        BoundIsABoundNotAnOffByOne();
        ARejectedCallWritesNothing();
        BuiltCoreIsIsolatedFromTheBuilder();
        ToBuilderRoundTrips();

        if (_failures == 0)
        {
            Console.WriteLine($"CoreBuilderTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"CoreBuilderTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
