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
 *
 * List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  081826 MF,CC  First Version — the C# half of the no-phantom-I/O probe (#235).
 */

/* Hand-written test; ta_codegen never opens this file. */

using System;
using System.Collections.Generic;
using System.Linq;
using TALib;
using TALib.Metadata;

namespace TALib.Test;

/// <summary>
/// Negative-space coverage: buffer sizes chosen so that any access the contract
/// forbids is an out-of-range throw rather than a comment.
/// </summary>
/// <remarks>
/// <para>The C# half of the Java <c>NoPhantomIoTest</c>, and the same two
/// load-bearing sweeps. The value gates (<c>ta_regtest --codegen</c>,
/// <c>--xlang-hash</c>, <c>--fuzz-064</c>) can only see work that reaches an
/// output; work a function does and then discards leaves no trace in any output,
/// so nothing there can see it. Every language server allocates its buffers at a
/// fixed <c>MAX_ARRAY_SIZE</c>, which is why no cross-language harness can host
/// this check either — the buffers are never tight enough for a phantom access
/// to fall off the end of one.</para>
///
/// <para>Two sweeps:</para>
/// <list type="number">
/// <item><description><b>Sub-lookback</b> — a range strictly shorter than the
/// lookback, with <b>zero-length</b> buffers everywhere. That call is a
/// documented success with no values and must touch nothing; with no elements,
/// every index is out of range.</description></item>
/// <item><description><b>Exact extent</b> — a range that <i>does</i> produce
/// values, with each input sized to exactly <c>endIdx + 1</c> and each output to
/// exactly the count the call reported. A read past <c>endIdx</c> or a write past
/// the reported count is then out of range. This is what reaches the 30 functions
/// whose lookback is 0, for which no sub-lookback range exists.</description></item>
/// </list>
///
/// <para>Why C# is worth running as well as Java: the two are independent
/// transcriptions of the same <c>ta_codegen/input/</c> C. A shared-source bug
/// (the reason the Java probe exists — it caught APO, PPO, PVO and BBANDS
/// computing a sub-indicator over the whole range before discovering their own
/// range was too short) shows up in both, but an emitter bug that changes what
/// one backend touches without changing what it produces shows up only here.</para>
///
/// <para>Driven off <see cref="FunctionCatalog"/> rather than reflection: it
/// enumerates all 174 functions with their input kinds, price components,
/// parameter domains and output kinds, and <see cref="FunctionCall"/>'s thunks
/// reach the <c>RetCode</c> tier directly — the typed <see cref="Core"/> wrapper
/// rejects a too-short input span, empty included, before the body could touch it,
/// which is a different (and separately tested) property. It does not reject the
/// OHLC legs a few candlestick patterns declare but never index.</para>
/// </remarks>
public static class NoPhantomIoTest
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

    private static void Violation(string what)
    {
        _checks++;
        _failures++;
        Console.WriteLine("  FAIL: " + what);
    }

    /// <summary>Whether <paramref name="e"/> is an out-of-range buffer access.</summary>
    /// <remarks>A span reports an out-of-range index as
    /// <see cref="IndexOutOfRangeException"/> and an out-of-range slice as
    /// <see cref="ArgumentOutOfRangeException"/>; both mean the body reached
    /// outside a buffer. Nothing else is folded in — the <c>RetCode</c> tier
    /// reports bad arguments as codes, so any other exception is itself a
    /// finding.</remarks>
    private static bool IsOutOfRange(Exception e) =>
        e is IndexOutOfRangeException or ArgumentOutOfRangeException;

    /* ---------------------------------------------------------------- fixtures */

    /// <summary>Bar <paramref name="i"/> of one leg of a deterministic series.</summary>
    /// <remarks>Ordered so the OHLC relations hold, because a pattern that reads a
    /// bar it should not is the thing being caught, not a pattern that takes a
    /// different branch on nonsense input.</remarks>
    private static double Bar(string leg, int i)
    {
        double bse = 100.0 + (10.0 * Math.Sin(i / 7.0)) + (3.0 * Math.Cos(i / 3.0));
        return leg switch
        {
            "open" => bse - 0.5,
            "high" => bse + 2.0,
            "low" => bse - 2.0,
            "close" => bse + 0.5,
            "volume" => 1000.0 + i,
            // MAVP's per-bar period. It is the one leg whose value sets how far
            // back the function reads, so the fixture has to reach the deepest
            // legal bar or the exact-extent sweep leaves slack below the window
            // and asserts nothing for it: every third bar asks for more than any
            // documented maximum, which mavp clamps to optInMaxPeriod -- the bar
            // its own lookback is computed from. The rest stay short so the
            // period buckets are not all one value.
            "inPeriods" => (i % 3) == 0 ? 1.0e5 : 5.0 + (i % 7),
            _ => bse,
        };
    }

    private static double[] Series(string leg, int len)
    {
        var s = new double[len];
        for (int i = 0; i < len; i++)
        {
            s[i] = Bar(leg, i);
        }

        return s;
    }

    /// <summary>Binds every input and output of <paramref name="f"/> at the given sizes.</summary>
    private static void Bind(FunctionInfo f, FunctionCall call, int inLen, int outLen)
    {
        for (int slot = 0; slot < f.Inputs.Length; slot++)
        {
            InputInfo input = f.Inputs[slot];
            switch (input.Kind)
            {
                case InputKind.Real:
                    call.SetInput(slot, Series(input.ParamName, inLen));
                    break;
                case InputKind.Integer:
                    call.SetInput(slot, new int[inLen]);
                    break;
                default:
                    // Only the components the function declares: SetPriceInput
                    // accepts an unconsumed one, but binding it would say this
                    // sweep covers a leg it does not.
                    foreach ((PriceComponents bit, string leg) in PriceLegs)
                    {
                        if ((input.Components & bit) != 0)
                        {
                            call.SetPriceInput(slot, bit, Series(leg, inLen));
                        }
                    }

                    break;
            }
        }

        for (int i = 0; i < f.Outputs.Length; i++)
        {
            if (f.Outputs[i].Kind == OutputKind.Real)
            {
                call.SetOutput(i, new double[outLen]);
            }
            else
            {
                call.SetOutput(i, new int[outLen]);
            }
        }
    }

    /// <summary>A value no indicator produces, so a slot still holding it was not written.</summary>
    private const double RealSentinel = -3.0e37;

    private const int IntSentinel = unchecked((int)0xDEADBEEF);

    /// <summary>
    /// Binds inputs of zero length and outputs of exactly one sentinel-filled element.
    /// </summary>
    /// <remarks>Not zero-length outputs, which would be the sharper probe and which
    /// Java uses: C#'s multi-output guard reads
    /// <c>a.Overlaps(b) || (a.IsEmpty &amp;&amp; b.IsEmpty)</c>, and that second clause
    /// rejects two empty output spans outright — a span carries no identity when it
    /// has no elements, so the guard cannot tell "two distinct empty buffers" from
    /// "the same buffer twice" and refuses. So all 14 multi-output functions answer
    /// <c>BadParam</c> before their body runs, and no zero-length probe of them is
    /// possible at all. (Java's guard is reference equality on the arrays, so two
    /// distinct empty arrays pass there: this is a real difference in what the two
    /// bindings accept, not a quirk of the probe.)
    /// <para>One sentinel-filled element keeps both directions: a write at index 0
    /// destroys the sentinel, and a write anywhere above it is out of range — so the
    /// pair covers exactly what a zero-length buffer would, at the cost of needing
    /// two things checked instead of one.</para></remarks>
    private static (double[][] Real, int[][] Int) BindQuiet(FunctionInfo f, FunctionCall call)
    {
        for (int slot = 0; slot < f.Inputs.Length; slot++)
        {
            InputInfo input = f.Inputs[slot];
            switch (input.Kind)
            {
                case InputKind.Real:
                    call.SetInput(slot, Array.Empty<double>());
                    break;
                case InputKind.Integer:
                    call.SetInput(slot, Array.Empty<int>());
                    break;
                default:
                    foreach ((PriceComponents bit, string _) in PriceLegs)
                    {
                        if ((input.Components & bit) != 0)
                        {
                            call.SetPriceInput(slot, bit, Array.Empty<double>());
                        }
                    }

                    break;
            }
        }

        var reals = new List<double[]>();
        var ints = new List<int[]>();
        for (int i = 0; i < f.Outputs.Length; i++)
        {
            if (f.Outputs[i].Kind == OutputKind.Real)
            {
                double[] b = [RealSentinel];
                reals.Add(b);
                call.SetOutput(i, b);
            }
            else
            {
                int[] b = [IntSentinel];
                ints.Add(b);
                call.SetOutput(i, b);
            }
        }

        return (reals.ToArray(), ints.ToArray());
    }

    private static bool AnySentinelGone((double[][] Real, int[][] Int) buffers) =>
        buffers.Real.Any(b => b[0] != RealSentinel) || buffers.Int.Any(b => b[0] != IntSentinel);

    private static readonly (PriceComponents Bit, string Leg)[] PriceLegs =
    [
        (PriceComponents.Open, "open"),
        (PriceComponents.High, "high"),
        (PriceComponents.Low, "low"),
        (PriceComponents.Close, "close"),
        (PriceComponents.Volume, "volume"),
        (PriceComponents.OpenInterest, "openinterest"),
    ];

    /* -------------------------------------------------- parameter vectors */

    private sealed record Vector(string Label, Action<FunctionCall> Bind, int Lookback);

    /// <summary>The parameter vectors to probe <paramref name="f"/> at.</summary>
    /// <remarks><para>The all-defaults vector alone probes a function whose I/O
    /// extent depends on a parameter at exactly one point, and for every function
    /// taking a moving-average type that one point is <c>SMA</c> — which is how a
    /// live BBANDS phantom read sat under the Java probe until the enum was swept.
    /// Added to it: every parameter at its documented minimum (where the period-1
    /// identity fast paths live), and each choice-list value in turn
    /// (<c>DISABLED</c> included, whose lookback of 0 is the tightest sizing there
    /// is).</para>
    /// <para>Validity is not judged here: a vector is kept only if
    /// <see cref="FunctionCall.Lookback"/> accepts it, which it signals by
    /// returning <c>-1</c>. An impossible combination drops out by the library's
    /// own rule rather than by a list kept in step by hand.</para></remarks>
    private static List<Vector> Vectors(FunctionInfo f, Core core)
    {
        var candidates = new List<(string Label, Action<FunctionCall> Bind)>
        {
            ("defaults", _ => { }),      // an unbound parameter IS its default
        };

        if (f.OptInputs.Length > 0)
        {
            candidates.Add(("minimums", c =>
            {
                for (int i = 0; i < f.OptInputs.Length; i++)
                {
                    switch (f.OptInputs[i].Domain)
                    {
                        case OptInputDomain.IntegerRange r:
                            c.SetOption(i, r.Min);
                            break;
                        case OptInputDomain.RealRange r:
                            c.SetOption(i, r.Min);
                            break;
                        default:
                            break;   // a choice list has no "minimum"; swept below
                    }
                }
            }));
        }

        // Each choice-list value on its own, and again with every integer parameter
        // raised. Raised, because for a composed function the two axes interact:
        // BBANDS bails early only when the moving average's lookback is BELOW the
        // deviation's, which at MAType.MAMA (a constant lookback of 32) needs a
        // period above 33 -- a bar the default of 20 never reaches. The choice
        // value alone pins the DISABLED half of that guard and leaves the MAMA
        // half unpinned.
        var raise = new List<(int Slot, int Value)>();
        for (int i = 0; i < f.OptInputs.Length; i++)
        {
            if (f.OptInputs[i].Domain is OptInputDomain.IntegerRange r
                && Math.Min(2 * r.Default, r.Max) > r.Default)
            {
                raise.Add((i, Math.Min(2 * r.Default, r.Max)));
            }
        }

        for (int i = 0; i < f.OptInputs.Length; i++)
        {
            if (f.OptInputs[i] is not { Domain: OptInputDomain.IntegerList list } p)
            {
                continue;
            }

            foreach (NamedValue v in list.Values)
            {
                int slot = i;
                int value = (int)v.Value;
                candidates.Add(($"{p.ParamName}={v.Name}", c => c.SetOption(slot, value)));
                if (raise.Count > 0)
                {
                    List<(int Slot, int Value)> raised = raise;
                    candidates.Add(($"{p.ParamName}={v.Name}, periods doubled", c =>
                    {
                        foreach ((int s2, int v2) in raised)
                        {
                            c.SetOption(s2, v2);
                        }

                        c.SetOption(slot, value);
                    }));
                }
            }
        }

        var kept = new List<Vector>();
        foreach ((string label, Action<FunctionCall> bind) in candidates)
        {
            FunctionCall probe = f.CreateCall(core);
            bind(probe);
            int lookback = probe.Lookback();
            if (lookback >= 0)
            {
                kept.Add(new Vector(label, bind, lookback));
            }
        }

        return kept;
    }

    /// <summary>The one function whose sub-lookback probe is out of reach, and
    /// why it is one.</summary>
    /// <remarks>
    /// <para>This sweep works by handing a function ZERO-LENGTH buffers and
    /// reading what happens: silence means no I/O, a fault means the detector is
    /// live. Since #236 step 3 the transcribed body calls its callee's PUBLIC
    /// overload, and the callee's input bound (rule B-5a) requires
    /// <c>endIdx + 1</c> elements — deliberately without the sub-lookback escape
    /// the OUTPUT bound takes. A function that forwards on a range shorter than
    /// its own lookback therefore answers <c>BadParam</c> before touching a
    /// buffer, and the probe cannot tell "read nothing" from "never ran".</para>
    /// <para>Nothing about the PUBLIC API moved: reached through the caller's own
    /// wrapper the callee's check is provably redundant, same <c>endIdx</c>, same
    /// buffer.</para>
    /// <para><b>The fix is an early return, and every other function that needed
    /// one has it.</b> <c>apo</c>, <c>bbands</c>, <c>ppo</c>, <c>pvo</c> and now
    /// <c>stddev</c> return <c>0,0</c> before forwarding when the range is shorter
    /// than their lookback; the rest never forwarded on such a range. <c>ma</c> is
    /// the holdout because it is a DISPATCH function: the generator admits only
    /// decls, comments, the identity path, one switch and a final return at the
    /// top level of a dispatch body — the shape the stream planner is built on —
    /// so a guard there is a generator change, not an edit to <c>ma.c</c>. The
    /// other way out is #236 deciding the input bound does not keep its stricter
    /// reading.</para>
    /// <para>An explicit list, not a symptom test: a function that starts
    /// answering <c>BadParam</c> here for any other reason is still a hard
    /// failure. The size is asserted, so the debt can be paid down but not quietly
    /// grown.</para>
    /// </remarks>
    private static readonly HashSet<string> CrossCallGuarded = new()
    {
        "MA",
    };

    /* -------------------------------------------------- sweep 1: sub-lookback */

    private static void SubLookbackSweep(Core core, IReadOnlyList<FunctionInfo> catalog)
    {
        int probed = 0;
        int noSubLookbackRange = 0;
        int violations = 0;
        var live = new List<string>();
        var withheld = new List<string>();

        foreach (FunctionInfo f in catalog)
        {
            List<Vector> vectors = Vectors(f, core);
            Check(vectors.Count > 0, $"{f.Name} has at least one parameter vector its own lookback accepts");
            if (CrossCallGuarded.Contains(f.Name))
            {
                // Only the zero-length I/O probe below is out of reach; the
                // vector check above still applies and still runs.
                withheld.Add(f.Name);
                continue;
            }
            if (vectors.Count == 0)
            {
                continue;
            }

            // The per-function control arm. One bar longer than the quiet range is
            // a call that produces exactly one value, so it MUST index a buffer --
            // and with zero-length buffers that is a throw. A function that stops
            // throwing here has stopped computing, and its silence in the sweep
            // below would otherwise read as compliance. This is also what covers
            // the functions whose lookback is 0: they have no quiet range, but they
            // do have this one.
            Vector d = vectors[0];
            FunctionCall control = f.CreateCall(core);
            d.Bind(control);
            (double[][], int[][]) controlBuffers = BindQuiet(f, control);
            try
            {
                RetCode rc = control.TryInvoke(0, d.Lookback, out OutRange _);
                if (AnySentinelGone(controlBuffers))
                {
                    live.Add(f.Name);
                }
                else if (rc == RetCode.BadParam)
                {
                    // A rejection counts as liveness, and only here. The vector is
                    // this function's own defaults on a range that produces, so its
                    // parameters are valid; the thunk binds NAME_Impl, which has no
                    // length check. So a BadParam can only be a CALLEE's public-tier
                    // length guard, converted by TryInvoke -- which means this
                    // function reached a callee, which is equally a proof that it
                    // still computes. It is the only proof available for a composed
                    // function since #236 step 3 routed cross-calls through the
                    // public callee. Java asserts the same thing on the exception
                    // type, which it still has because its cores throw.
                    live.Add(f.Name);
                }
                else
                {
                    Violation($"{f.Name} at endIdx == lookback ({d.Lookback}) returned {rc} without "
                        + "touching a buffer; a call that produces a value must read an input or "
                        + "write an output, so this sweep could not detect I/O for it");
                    violations++;
                }
            }
            catch (Exception e) when (IsOutOfRange(e))
            {
                live.Add(f.Name);
            }
            catch (Exception e)
            {
                Violation($"{f.Name} at endIdx == lookback threw {e.GetType().Name}: {e.Message}");
                violations++;
            }

            foreach (Vector v in vectors)
            {
                // A lookback of 0 means every bar produces a value, so no range is
                // short enough to expect no I/O from. Counted, not silently dropped;
                // the exact-extent sweep is what reaches these.
                if (v.Lookback < 1)
                {
                    if (v.Label == "defaults")
                    {
                        noSubLookbackRange++;
                    }

                    continue;
                }

                if (v.Label == "defaults")
                {
                    probed++;
                }

                FunctionCall call = f.CreateCall(core);
                v.Bind(call);
                (double[][], int[][]) buffers = BindQuiet(f, call);
                string where = $"{f.Name}[{v.Label}] (lookback {v.Lookback}, endIdx {v.Lookback - 1})";
                try
                {
                    RetCode rc = call.TryInvoke(0, v.Lookback - 1, out OutRange range);
                    if (rc != RetCode.Success)
                    {
                        Violation($"{where} returned {rc}, expected Success");
                        violations++;
                    }
                    else if (range.Count != 0)
                    {
                        Violation($"{where} reported {range.Count} values on a sub-lookback range");
                        violations++;
                    }
                    else if (AnySentinelGone(buffers))
                    {
                        Violation($"{where} wrote an output value on a sub-lookback range");
                        violations++;
                    }
                }
                catch (Exception e)
                {
                    Violation($"{where} touched a buffer: {e.GetType().Name}: {e.Message}");
                    violations++;
                }
            }
        }

        Check(probed + noSubLookbackRange + withheld.Count == catalog.Count,
              $"sub-lookback: every function is probed, counted or withheld ({probed} + "
              + $"{noSubLookbackRange} + {withheld.Count} vs {catalog.Count})");
        Check(live.Count + withheld.Count == catalog.Count,
              $"sub-lookback: the detector is proved live for every function that is not "
              + $"withheld ({live.Count} + {withheld.Count} of {catalog.Count}; not proved "
              + $"{string.Join(", ", catalog.Select(f => f.Name).Except(live).Except(withheld))})");
        // The debt cannot grow silently: the list is what it is, and a function
        // that leaves it has to leave this number too.
        Check(withheld.Count == CrossCallGuarded.Count,
              $"sub-lookback: every withheld function is one of the {CrossCallGuarded.Count} named "
              + $"in CrossCallGuarded (got {withheld.Count}: {string.Join(", ", withheld)})");
        Console.WriteLine($"  sub-lookback: {probed} functions probed, {violations} violation(s), "
            + $"{noSubLookbackRange} skipped (lookback 0, no sub-lookback range exists); "
            + $"{live.Count} detector control(s) fired; {withheld.Count} WITHHELD, out of this "
            + $"sweep's reach since #236 step 3 -> {string.Join(", ", withheld)}");
    }

    /* ------------------------------------------------- sweep 2: exact extent */

    /// <summary>The ranges the exact-extent sweep uses, as offsets from the lookback.</summary>
    /// <remarks><c>(0, 0)</c> is the tightest output there is — one value, so an
    /// output buffer of length 1, which catches a writer that stores two and
    /// reports one. Starting at <c>startIdx == lookback</c> is what makes the input
    /// bound <b>two-sided</b>: the legal read window is then exactly
    /// <c>[0..endIdx]</c>, so a read below it is a negative index and a read above
    /// it is past the end. <c>(3, 7)</c> gives that up in exchange for the one path
    /// the others never take — <c>startIdx &gt; lookback</c>, so the body's clamp
    /// to the lookback does not fire.</remarks>
    private static readonly (int Lo, int Hi)[] Ranges = [(0, 0), (0, 4), (3, 7)];

    private static void ExactExtentSweep(Core core, IReadOnlyList<FunctionInfo> catalog)
    {
        const int Pad = 16;
        int probes = 0;
        int violations = 0;
        var reached = new List<string>();

        foreach (FunctionInfo f in catalog)
        {
            bool reachedAtDefaults = false;
            foreach (Vector v in Vectors(f, core))
            {
                foreach ((int lo, int hi) in Ranges)
                {
                    int startIdx = v.Lookback + lo;
                    int endIdx = v.Lookback + hi;

                    // Pass 1: padded, to learn the count. A throw here is not an
                    // over-read of one element -- it is a read far outside the
                    // range, and worth its own message.
                    FunctionCall loose = f.CreateCall(core);
                    v.Bind(loose);
                    Bind(f, loose, endIdx + 1 + Pad, endIdx - startIdx + 1 + Pad);
                    RetCode rc;
                    OutRange range;
                    try
                    {
                        rc = loose.TryInvoke(startIdx, endIdx, out range);
                    }
                    catch (Exception e)
                    {
                        Violation($"{f.Name}[{v.Label}] [{startIdx}..{endIdx}] threw on PADDED "
                            + $"buffers ({Pad} spare elements): {e.GetType().Name}: {e.Message}");
                        violations++;
                        continue;
                    }

                    if (rc != RetCode.Success)
                    {
                        // An out-of-range parameter combination the lookback let
                        // through. Not this sweep's business; it simply is not a
                        // call, so there is nothing to hold to a bound.
                        continue;
                    }

                    // Sizing pass 2 to the reported count alone would be fail-open:
                    // a body that writes N+1 values AND reports N+1 satisfies it,
                    // while a caller who allocated by the published formula
                    // overflows. So hold the report to the formula first, and only
                    // then use it as the bound.
                    if (range.BegIdx != startIdx || range.Count != endIdx - startIdx + 1)
                    {
                        Violation($"{f.Name}[{v.Label}] [{startIdx}..{endIdx}] reported begIdx "
                            + $"{range.BegIdx} and count {range.Count}, not the documented "
                            + $"{startIdx} and {endIdx - startIdx + 1}");
                        violations++;
                        continue;
                    }

                    // Pass 2: exactly what the contract allows the call to touch.
                    FunctionCall tight = f.CreateCall(core);
                    v.Bind(tight);
                    Bind(f, tight, endIdx + 1, range.Count);
                    probes++;
                    reachedAtDefaults |= v.Label == "defaults";
                    try
                    {
                        tight.TryInvoke(startIdx, endIdx, out OutRange _);
                    }
                    catch (Exception e)
                    {
                        Violation($"{f.Name}[{v.Label}] [{startIdx}..{endIdx}] with inputs of "
                            + $"{endIdx + 1} and outputs of {range.Count} (the count it reported) "
                            + $"touched an element outside them: {e.GetType().Name}: {e.Message}");
                        violations++;
                    }
                }
            }

            if (reachedAtDefaults)
            {
                reached.Add(f.Name);
            }
        }

        // Fail-closed coverage. A floor ("at least 140 probed") passes in exactly
        // the case that matters -- add ten functions, silently lose ten from the
        // catalogue, and the floor is still met. Every function must instead be
        // reached at its own defaults vector, on a range built from its own
        // lookback, so one that stops being reachable is red rather than quiet.
        Check(reached.Count == catalog.Count,
              $"exact extent: every function is reached at its defaults ({reached.Count} of "
              + $"{catalog.Count}; missing {string.Join(", ", catalog.Select(f => f.Name).Except(reached))})");
        Console.WriteLine($"  exact extent: {probes} probes over {catalog.Count} functions, "
            + $"{violations} violation(s)");
    }

    /* -------------------------------------------------- proving the detectors */

    /// <summary>Each sweep's detector, proved before the sweep is trusted.</summary>
    /// <remarks>Control arms: calls that <b>must</b> throw. If the bounds check the
    /// whole file rests on ever stopped firing — an exception caught too broadly, a
    /// buffer quietly resized, an argument bound wrong — the sweeps would all go
    /// green, and only these would go red.</remarks>
    private static void TheProbesCanFail(Core core)
    {
        FunctionInfo sma = FunctionCatalog.Default["SMA"];
        int lookback = sma.CreateCall(core).SetOption(0, 30).Lookback();

        // 1. sub-lookback: the quiet case is silent, one bar longer is not.
        FunctionCall quiet = sma.CreateCall(core);
        quiet.SetOption(0, 30);
        (double[][], int[][]) quietBuffers = BindQuiet(sma, quiet);
        Check(quiet.TryInvoke(0, lookback - 1, out OutRange empty) == RetCode.Success
                  && empty.Count == 0 && !AnySentinelGone(quietBuffers),
              "a sub-lookback range with zero-length inputs is a silent success");

        FunctionCall oneMore = sma.CreateCall(core);
        oneMore.SetOption(0, 30);
        BindQuiet(sma, oneMore);
        Check(ThrowsOutOfRange(() => oneMore.TryInvoke(0, lookback, out OutRange _)),
              "one bar longer DOES read the input, so sweep 1 can detect a phantom read");

        // The other half of sweep 1's detector: an output written where none should
        // be. Sized correctly for the range but called one bar short of the lookback
        // -- so a body that stored a value anyway would leave no sentinel behind.
        FunctionCall writeProbe = sma.CreateCall(core);
        writeProbe.SetOption(0, 30);
        var canary = new double[1];
        canary[0] = RealSentinel;
        writeProbe.SetInput(0, Series("inReal", lookback));
        writeProbe.SetOutput(0, canary);
        writeProbe.TryInvoke(0, lookback - 1, out OutRange _);
        Check(canary[0] == RealSentinel,
              "the sentinel survives a call that writes nothing, so it can report one that does");
        FunctionCall writeProbe2 = sma.CreateCall(core);
        writeProbe2.SetOption(0, 30);
        writeProbe2.SetInput(0, Series("inReal", lookback + 1));
        writeProbe2.SetOutput(0, canary);
        writeProbe2.TryInvoke(0, lookback, out OutRange _);
        Check(canary[0] != RealSentinel,
              "a call that DOES write destroys the sentinel, so sweep 1 can detect a phantom write");

        // 2. exact extent: the exact sizes pass, one element short in either
        //    direction throws. Proves BOTH bounds, not just the output one.
        int endIdx = lookback + 4;
        int count = endIdx - lookback + 1;

        FunctionCall exact = sma.CreateCall(core);
        exact.SetOption(0, 30);
        Bind(sma, exact, endIdx + 1, count);
        Check(exact.TryInvoke(0, endIdx, out OutRange full) == RetCode.Success && full.Count == count,
              "exactly-sized input and output are enough for SMA");

        FunctionCall shortOut = sma.CreateCall(core);
        shortOut.SetOption(0, 30);
        Bind(sma, shortOut, endIdx + 1, count - 1);
        Check(ThrowsOutOfRange(() => shortOut.TryInvoke(0, endIdx, out OutRange _)),
              "an output one short of the count throws, so sweep 2 sees over-writes");

        FunctionCall shortIn = sma.CreateCall(core);
        shortIn.SetOption(0, 30);
        Bind(sma, shortIn, endIdx, count);
        Check(ThrowsOutOfRange(() => shortIn.TryInvoke(0, endIdx, out OutRange _)),
              "an input one short of endIdx+1 throws, so sweep 2 sees over-reads");
    }

    private static bool ThrowsOutOfRange(Action body)
    {
        try
        {
            body();
            return false;
        }
        catch (Exception e)
        {
            return IsOutOfRange(e);
        }
    }

    public static int Run()
    {
        var core = new Core();
        IReadOnlyList<FunctionInfo> catalog = FunctionCatalog.Default;

        // Non-vacuity, tied to a GENERATED source of truth rather than a literal.
        // A floor fails open in exactly the case that matters, so the sweeps assert
        // against the catalogue itself, which ta_codegen writes from the same
        // input/ directory the bodies come from.
        Check(catalog.Count > 0, "the catalogue enumerates the functions the sweeps walk");

        TheProbesCanFail(core);
        SubLookbackSweep(core, catalog);
        ExactExtentSweep(core, catalog);

        // Once more against a non-zero unstable period, mirroring the pass in
        // NoPhantomIoTest.java. A dozen cores carry an explicit
        //     /* Skip the unstable period */ i = unstablePeriod[...]; while (i-- > 0)
        // warm-up loop that reads the input on every trip -- and at the default of
        // 0, NINE of those loops (RSI, CMO, ATR, NATR, ADX, MINUS_DM, PLUS_DM, T3,
        // KAMA) run zero times, so in C# their reads had never executed under a
        // bounds check at all. Only DX, MINUS_DI and PLUS_DI are live at the
        // default, because their form is `i = unstable + 1; while (i-- != 0)`.
        //
        // The setting also raises the published lookback of every function that
        // carries it, and of every composite that calls one, which is exactly the
        // lookback-versus-what-the-body-reads coupling every bug found here so far
        // has lived in.
        //
        // Batch sweeps only: the control and streaming shapes do not move with it.
        Core unstable = Core.Builder().UnstablePeriod(FuncUnstId.ALL, 3).Build();
        int moved = 0;
        foreach (FunctionInfo f in catalog)
        {
            List<Vector> baseline = Vectors(f, core);
            List<Vector> shifted = Vectors(f, unstable);
            if (baseline.Count > 0 && shifted.Count > 0
                && baseline[0].Lookback != shifted[0].Lookback)
            {
                moved++;
            }
        }
        // Tied to what the setting actually moves, not to a literal: if no lookback
        // changed, this pass is a duplicate of the one above and the reader should
        // be told rather than left to assume it added coverage.
        Check(moved > 0,
            $"a non-zero unstable period moves at least one lookback ({moved} of {catalog.Count})");
        Console.WriteLine($"  unstable period 3: {moved} of {catalog.Count} lookbacks move");
        SubLookbackSweep(unstable, catalog);
        ExactExtentSweep(unstable, catalog);

        if (_failures == 0)
        {
            Console.WriteLine($"NoPhantomIoTest: ALL PASS ({_checks} checks)");
            return 0;
        }

        Console.WriteLine($"NoPhantomIoTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
