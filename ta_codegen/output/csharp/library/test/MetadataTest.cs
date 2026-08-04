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
 *  080326 MF,CC  First Version.
 */

/* Hand-written test; ta_codegen never opens this file. */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using TALib;
using TALib.Metadata;

namespace TALib.Test;

/// <summary>
/// The contract of the shipped metadata catalogue, and of calling a function
/// chosen at run time.
/// </summary>
/// <remarks>
/// <para>What this file does NOT do is re-assert what a cross-language gate
/// already proves. <c>ta_regtest --codegen --language=csharp</c> compares every
/// row of this catalogue against the C library's <c>ta_abstract</c>, and drives
/// <see cref="FunctionCall"/> for all 168 functions with the output values
/// compared to C. So the numbers and the dispatch are covered from outside;
/// what is covered here is the surface a C# caller touches and no server ever
/// sees, plus the two shapes an external oracle structurally cannot check:
/// an <em>extra</em> flag bit, and the catalogue's own invariants.</para>
/// <para>Deliberately not modelled on the Java <c>MetadataTest</c>'s counting:
/// most of its 896 checks are two loops asserting strings are non-empty, and
/// its flag assertions are presence tests <c>(flags &amp; C) != 0</c> that pass
/// on any overlapping bit. Every flag assertion here is an exact comparison
/// against a generated constant instead.</para>
/// </remarks>
public static class MetadataTest
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
            Console.WriteLine($"  FAIL: {what} (nothing was thrown)");
        }
        catch (Exception e) when (e is TException)
        {
            // expected
        }
        catch (Exception e)
        {
            _failures++;
            Console.WriteLine($"  FAIL: {what} (threw {e.GetType().Name})");
        }
    }

    /* ---------------------------------------------------------------- data */

    private const int N = 160;

    private static double[] Series(double baseline, double amplitude, double phase)
    {
        var o = new double[N];
        for (int i = 0; i < N; i++)
        {
            o[i] = baseline + (amplitude * Math.Sin((i / 9.0) + phase)) + (0.4 * Math.Cos((i / 3.0) + phase));
        }
        return o;
    }

    private static readonly double[] Close = Series(100.0, 8.0, 0.0);
    private static readonly double[] Open = Series(100.0, 8.0, 0.35);
    private static readonly double[] High = MakeHigh();
    private static readonly double[] Low = MakeLow();
    private static readonly double[] Volume = Series(1.0e6, 2.0e5, 1.1);
    private static readonly double[] OpenInterest = Series(5.0e5, 1.0e5, 0.7);

    private static double[] MakeHigh()
    {
        var o = new double[N];
        for (int i = 0; i < N; i++)
        {
            o[i] = Math.Max(Close[i], Open[i]) + 1.0 + Math.Abs(Math.Sin(i * 1.7));
        }
        return o;
    }

    private static double[] MakeLow()
    {
        var o = new double[N];
        for (int i = 0; i < N; i++)
        {
            o[i] = Math.Min(Close[i], Open[i]) - 1.0 - Math.Abs(Math.Cos(i * 1.3));
        }
        return o;
    }

    private static double[] ComponentSeries(PriceComponents c) => c switch
    {
        PriceComponents.Open => Open,
        PriceComponents.High => High,
        PriceComponents.Low => Low,
        PriceComponents.Close => Close,
        PriceComponents.Volume => Volume,
        _ => OpenInterest,
    };

    /* ------------------------------------------------------------ catalogue */

    private static void CatalogueIsComplete()
    {
        FunctionCatalog c = FunctionCatalog.Default;

        // Exact, not a threshold: `>= 160` against 168 would let eight
        // functions disappear without a word.
        Check(c.Count == CatalogFacts.FunctionCount,
            $"catalogue holds exactly {CatalogFacts.FunctionCount} functions (got {c.Count})");
        Check(ReferenceEquals(c, Core.Functions), "Core.Functions is the same catalogue");

        var names = new HashSet<string>(StringComparer.Ordinal);
        for (int i = 0; i < c.Count; i++)
        {
            FunctionInfo f = c[i];
            Check(names.Add(f.Name), $"{f.Name}: appears once");
            Check(ReferenceEquals(c[f.Name], f), $"{f.Name}: name lookup round-trips");
            Check(f.Outputs.Length > 0, $"{f.Name}: has at least one output");
            Check(f.Hint.Length > 0, $"{f.Name}: carries a hint");
            Check(f.MethodName.Length > 0, $"{f.Name}: names its Core method");
        }

        Check(string.CompareOrdinal(c[0].Name, c[c.Count - 1].Name) < 0, "catalogue is name-ordered");
        CheckThrows<KeyNotFoundException>(() => _ = c["NOSUCHFUNCTION"], "unknown name throws");
        Check(!c.TryGet("NOSUCHFUNCTION", out _), "TryGet reports an unknown name");
        Check(c.TryGet("SMA", out FunctionInfo? sma) && sma.Name == "SMA", "TryGet finds a known name");

        // Case sensitivity is a contract, not an accident: C's TA_GetFuncHandle
        // is a strcmp, and the catalogue pins StringComparer.Ordinal to match.
        Check(!c.TryGet("sma", out _), "lookup is case-sensitive, matching C");

        int inGroups = FunctionCatalog.Groups.Sum(g => c.InGroup(g).Count());
        Check(inGroups == c.Count, $"the groups partition the catalogue ({inGroups} vs {c.Count})");
        foreach (FunctionGroup g in FunctionCatalog.Groups)
        {
            Check(c.InGroup(g).Any(), $"group {g} is non-empty");
            Check(g.ToDisplayName().Length > 0, $"group {g} has a display name");
        }
    }

    /* ----------------------------------------------------- domain invariants */

    private static void DomainsAreCoherent()
    {
        int ranges = 0;
        int lists = 0;
        foreach (FunctionInfo f in FunctionCatalog.Default)
        {
            foreach (OptInputInfo o in f.OptInputs)
            {
                string where = $"{f.Name}.{o.ParamName}";
                Check(o.ParamName.Length > 0 && o.DisplayName.Length > 0, $"{where}: named");
                switch (o.Domain)
                {
                    case OptInputDomain.IntegerRange r:
                        ranges++;
                        Check(r.Min <= r.Default && r.Default <= r.Max,
                            $"{where}: default {r.Default} is inside [{r.Min}, {r.Max}]");
                        CheckSweep(where, r.Sweep().Select(v => (double)v), r.Min, r.Max, r.Default);
                        CheckSweepReachesTheEnd(where, r.Sweep().Select(v => (double)v),
                            r.SuggestedStart, r.SuggestedEnd, r.SuggestedIncrement);
                        break;
                    case OptInputDomain.RealRange r:
                        ranges++;
                        Check(r.Min <= r.Default && r.Default <= r.Max,
                            $"{where}: default {r.Default} is inside [{r.Min}, {r.Max}]");
                        Check(r.Precision >= 0, $"{where}: precision is not negative");
                        CheckSweep(where, r.Sweep(), r.Min, r.Max, r.Default);
                        CheckSweepReachesTheEnd(where, r.Sweep(),
                            r.SuggestedStart, r.SuggestedEnd, r.SuggestedIncrement);
                        break;
                    case OptInputDomain.IntegerList l:
                        lists++;
                        Check(l.Values.Length > 0, $"{where}: the choice list is populated");
                        Check(l.Values.Any(v => v.Value == l.Default),
                            $"{where}: the default {l.Default} is one of the choices");
                        Check(l.ToValueListString().StartsWith("0=", StringComparison.Ordinal),
                            $"{where}: renders C's value list");
                        break;
                    default:
                        Check(false, $"{where}: unexpected domain {o.Domain.GetType().Name}");
                        break;
                }

                Check(Math.Abs(o.DefaultValue - o.Domain.DefaultValue) < 1e-12,
                    $"{where}: DefaultValue forwards to the domain");
            }
        }

        // Non-vacuity: the loops above must actually have run.
        Check(ranges > 100, $"swept {ranges} range parameters");
        Check(lists > 10, $"checked {lists} choice-list parameters");
    }

    /// <summary>A sweep must terminate, stay inside the domain, and never be empty.</summary>
    private static void CheckSweep(string where, IEnumerable<double> sweep, double min, double max, double def)
    {
        // Take() bounds a non-terminating sweep so a bug here fails the test
        // instead of hanging the suite.
        double[] values = sweep.Take(100_001).ToArray();
        Check(values.Length > 0 && values.Length <= 100_000, $"{where}: sweep is non-empty and terminates");
        Check(values.All(v => v >= min && v <= max), $"{where}: every swept value is inside the domain");
        if (values.Length == 1)
        {
            Check(Math.Abs(values[0] - def) < 1e-12,
                $"{where}: a degenerate sweep yields exactly the default");
        }
    }

    /// <summary>A sweep must actually reach the endpoint it advertises.</summary>
    /// <remarks>Asserting only "non-empty, in range, terminates" let SAREXT's
    /// <c>optInOffsetOnReverse</c> (0.01..0.15 step 0.01) silently stop at 0.14,
    /// because <c>0.01 + 14 * 0.01</c> is one ULP above 0.15.</remarks>
    private static void CheckSweepReachesTheEnd(string where, IEnumerable<double> sweep,
                                                double start, double end, double inc)
    {
        if (inc <= 0)
        {
            return;   // degenerate: yields the default, covered above
        }

        double[] values = sweep.Take(100_001).ToArray();
        double last = values[^1];
        Check(last > end - inc,
            $"{where}: sweep ends at {last}, within one step of the advertised end {end}");
        Check(Math.Abs(values[0] - start) < 1e-12, $"{where}: sweep starts at {start}");
    }

    /* -------------------------------------------------------- the flag census */

    private static void FlagsAreExact()
    {
        uint func = 0, price = 0, opt = 0, output = 0;
        foreach (FunctionInfo f in FunctionCatalog.Default)
        {
            func |= (uint)f.Flags;
            foreach (InputInfo i in f.Inputs)
            {
                price |= (uint)i.Components;
            }
            foreach (OptInputInfo o in f.OptInputs)
            {
                opt |= (uint)o.Flags;
            }
            foreach (OutputInfo o in f.Outputs)
            {
                output |= (uint)o.Flags;
            }
        }

        // EXACT, not a presence test. `(flags & X) != 0` structurally cannot
        // see an EXTRA bit: an emitter that OR'd a stray flag into every output
        // would pass a presence test and fails this one.
        Check(func == CatalogFacts.AllFunctionFlags,
            $"function flags OR to 0x{CatalogFacts.AllFunctionFlags:X8} (got 0x{func:X8})");
        Check(price == CatalogFacts.AllPriceComponents,
            $"price components OR to 0x{CatalogFacts.AllPriceComponents:X8} (got 0x{price:X8})");
        Check(opt == CatalogFacts.AllOptInputFlags,
            $"optional-parameter flags OR to 0x{CatalogFacts.AllOptInputFlags:X8} (got 0x{opt:X8})");
        Check(output == CatalogFacts.AllOutputFlags,
            $"output flags OR to 0x{CatalogFacts.AllOutputFlags:X8} (got 0x{output:X8})");

        // The dead set is pinned, so the day a definition starts using one of
        // these the change is announced rather than absorbed.
        Check(CatalogFacts.DeadFlags.Length == CatalogFacts.DeadFlagCount,
            $"{CatalogFacts.DeadFlagCount} flag members are set by no function");
        Check(CatalogFacts.DeadFlags.Contains("FunctionFlags.VolumeUsed"),
            "VolumeUsed is still set by no function (AD and OBV consume volume without the bit)");

        // And a spot check that the census is not vacuously zero.
        Check(FunctionCatalog.Default["CDLDOJI"].Flags.HasFlag(FunctionFlags.Candlestick), "CDLDOJI is a candlestick");
        Check(!FunctionCatalog.Default["SMA"].Flags.HasFlag(FunctionFlags.Candlestick), "SMA is not");
    }

    /* ------------------------------------------- unstable period, structurally */

    private static void UnstableIdAgreesWithTheFlag()
    {
        int withId = 0;
        foreach (FunctionInfo f in FunctionCatalog.Default)
        {
            bool flagged = f.Flags.HasFlag(FunctionFlags.UnstablePeriod);
            Check(flagged == (f.UnstableId is not null),
                $"{f.Name}: the UnstablePeriod flag and UnstableId agree (flag={flagged}, id={f.UnstableId})");
            if (f.UnstableId is not null)
            {
                withId++;
            }
        }

        // The two sides come from different places — the YAML `flags:` list and
        // an enums.yaml name match — so the assertion above is not a tautology.
        Check(withId == 20, $"exactly 20 functions carry an unstable period (got {withId})");
        Check(FunctionCatalog.Default["RSI"].UnstableId == FuncUnstId.Rsi, "RSI maps to its own id");
    }

    /* ------------------------------------------------- price bundles are ONE input */

    private static void PriceBundlesAreOneInput()
    {
        FunctionInfo adx = FunctionCatalog.Default["ADX"];
        Check(adx.Inputs.Length == 1, $"ADX declares one input, not three (got {adx.Inputs.Length})");
        InputInfo price = adx.Inputs[0];
        Check(price.Kind == InputKind.Price, "ADX's input is a price bundle");
        Check(price.ParamName == "inPriceHLC", $"ADX's bundle is named inPriceHLC (got {price.ParamName})");
        Check(price.Requires(PriceComponents.High | PriceComponents.Low | PriceComponents.Close),
            "ADX consumes high, low and close");
        Check(!price.Requires(PriceComponents.Volume), "ADX does not consume volume");
        Check(price.SignatureOrder.Length == 3, "the typed method takes three arrays");

        // SignatureOrder must be the order Core.Adx actually declares them in.
        Check(price.SignatureOrder.SequenceEqual(
                new[] { PriceComponents.High, PriceComponents.Low, PriceComponents.Close }),
            "ADX's signature order is high, low, close");

        FunctionInfo sma = FunctionCatalog.Default["SMA"];
        Check(sma.Inputs[0].Kind == InputKind.Real, "SMA takes a plain real series");
        Check(sma.Inputs[0].Components == PriceComponents.None, "a real input carries no components");
        Check(sma.Inputs[0].SignatureOrder.IsEmpty, "a real input has an empty signature order");
    }

    /* ------------------------------------------------------------ binder misuse */

    private static void BinderRejectsMisuse()
    {
        FunctionInfo sma = FunctionCatalog.Default["SMA"];
        FunctionInfo stoch = FunctionCatalog.Default["STOCH"];
        FunctionInfo doji = FunctionCatalog.Default["CDLDOJI"];
        var buf = new double[N];

        CheckThrows<ArgumentOutOfRangeException>(() => sma.CreateCall().SetInput(5, Close), "input index out of range");
        CheckThrows<ArgumentOutOfRangeException>(() => sma.CreateCall().SetOption(9, 30), "parameter index out of range");
        CheckThrows<ArgumentOutOfRangeException>(() => sma.CreateCall().SetOutput(9, buf), "output index out of range");
        CheckThrows<ArgumentException>(() => sma.CreateCall().SetOption(0, 1.5), "a real value on an integer parameter");
        // The mirror of the line above, and the one that was missing: SMA's
        // optInTimePeriod is an IntegerRange, so a MAType must not bind to it.
        // Delegating this overload to SetOption(int, int) accepted it and bound
        // a period of 1.
        CheckThrows<ArgumentException>(
            () => sma.CreateCall().SetOption(0, MAType.Ema), "a MAType on a non-choice-list parameter");
        FunctionInfo ma = FunctionCatalog.Default["MA"];
        CheckThrows<ArgumentException>(
            () => ma.CreateCall().SetOption(1, 1.5), "a real value on a choice-list parameter");
        // ...and the choice-list parameter still accepts what it should.
        Check(ma.CreateCall().SetOption(1, MAType.Ema) is not null, "a MAType binds to a choice list");
        CheckThrows<ArgumentException>(() => stoch.CreateCall().SetInput(0, Close), "a real series on a price input");
        CheckThrows<ArgumentException>(
            () => sma.CreateCall().SetPriceInput(0, PriceComponents.High, High), "a price component on a real input");
        CheckThrows<ArgumentException>(
            () => stoch.CreateCall().SetPriceInput(0, PriceComponents.Volume, Volume),
            "a component the function does not consume");
        CheckThrows<ArgumentException>(() => doji.CreateCall().SetOutput(0, buf), "a real buffer on an integer output");
        CheckThrows<ArgumentException>(
            () => sma.CreateCall().SetOutput(0, buf).Invoke(0, N - 1), "an unbound input");
        CheckThrows<ArgumentException>(
            () => sma.CreateCall().SetInput(0, Close).Invoke(0, N - 1), "an unbound output");
        CheckThrows<ArgumentException>(
            () => stoch.CreateCall().SetPriceInput(0, high: High, low: Low).Invoke(0, N - 1),
            "a price bundle missing a required component");

        // A row belonging to another function must not bind here.
        CheckThrows<ArgumentException>(
            () => sma.CreateCall().SetParam(stoch.OptInputs[0], 5), "a parameter row from another function");
    }

    /* --------------------------------------- the two call paths must agree, bitwise */

    /// <summary>
    /// Every function driven both through <see cref="FunctionCall"/> and through
    /// its typed method, with the outputs compared bit for bit.
    /// </summary>
    /// <remarks>The typed arm is reached reflectively and reads nothing from the
    /// catalogue except the argument shapes, so a transposed input, a wrong slot
    /// index or a wrong parameter order in a generated thunk cannot produce
    /// identical doubles by accident. (The library itself uses no reflection —
    /// this is test scaffolding, and the shipped assembly is built with
    /// <c>IsAotCompatible</c>.)</remarks>
    private static void BothCallPathsAgree()
    {
        int compared = 0;
        int withValues = 0;
        int named = 0;

        foreach (FunctionInfo f in FunctionCatalog.Default)
        {
            int nout = f.Outputs.Length;
            var realA = new double[nout][];
            var intA = new int[nout][];
            var realB = new double[nout][];
            var intB = new int[nout][];

            // Alternate the binding STYLE across the catalogue. The per-component
            // and named-argument SetPriceInput overloads are separate code paths
            // that must agree; driving only one left the other with no
            // success-path coverage at all, so a swapped component inside it
            // produced Success with silently wrong output and every suite stayed
            // green. Parity of the index is deterministic, so a failure names a
            // fixed function.
            bool useNamedArgs = named % 2 == 0;
            FunctionCall call = f.CreateCall();
            for (int i = 0; i < f.Inputs.Length; i++)
            {
                InputInfo info = f.Inputs[i];
                if (info.Kind == InputKind.Price)
                {
                    if (useNamedArgs)
                    {
                        call.SetPriceInput(
                            i,
                            open: info.Requires(PriceComponents.Open) ? Open : null,
                            high: info.Requires(PriceComponents.High) ? High : null,
                            low: info.Requires(PriceComponents.Low) ? Low : null,
                            close: info.Requires(PriceComponents.Close) ? Close : null,
                            volume: info.Requires(PriceComponents.Volume) ? Volume : null,
                            openInterest: info.Requires(PriceComponents.OpenInterest) ? OpenInterest : null);
                    }
                    else
                    {
                        foreach (PriceComponents comp in info.SignatureOrder)
                        {
                            call.SetPriceInput(i, comp, ComponentSeries(comp));
                        }
                    }

                    named++;
                }
                else if (info.Kind == InputKind.Real)
                {
                    call.SetInput(i, i == 0 ? Close : High);
                }
                else
                {
                    var ints = new int[N];
                    for (int k = 0; k < N; k++)
                    {
                        ints[k] = k;
                    }
                    call.SetInput(i, ints);
                }
            }

            // Every optional parameter bound EXPLICITLY through its typed
            // overload, so those overloads are on this bitwise comparison too
            // rather than only the unbound-default path.
            for (int i = 0; i < f.OptInputs.Length; i++)
            {
                switch (f.OptInputs[i].Domain)
                {
                    case OptInputDomain.IntegerRange r:
                        call.SetOption(i, r.Default);
                        break;
                    case OptInputDomain.IntegerList l:
                        call.SetOption(i, (MAType)(int)l.Default);
                        break;
                    default:
                        call.SetOption(i, f.OptInputs[i].DefaultValue);
                        break;
                }
            }

            for (int k = 0; k < nout; k++)
            {
                if (f.Outputs[k].Kind == OutputKind.Real)
                {
                    realA[k] = new double[N];
                    call.SetOutput(k, realA[k]);
                }
                else
                {
                    intA[k] = new int[N];
                    call.SetOutput(k, intA[k]);
                }
            }

            RetCode rcA = call.TryInvoke(0, N - 1, out OutRange a);
            OutRange? b = TypedCall(f, realB, intB);
            if (b is null)
            {
                _failures++;
                _checks++;
                Console.WriteLine($"  FAIL: {f.Name}: no typed overload matched the row's shapes");
                continue;
            }

            compared++;
            Check(rcA == RetCode.Success, $"{f.Name}: the bound call succeeded ({rcA})");
            Check(a.Equals(b.Value), $"{f.Name}: both paths report the same range ({a.BegIdx}/{a.Count} vs {b.Value.BegIdx}/{b.Value.Count})");
            if (a.Count > 0)
            {
                withValues++;
            }

            for (int k = 0; k < nout; k++)
            {
                bool same = true;
                for (int i = 0; i < a.Count; i++)
                {
                    same &= f.Outputs[k].Kind == OutputKind.Real
                        ? BitConverter.DoubleToInt64Bits(realA[k][i]) == BitConverter.DoubleToInt64Bits(realB[k][i])
                        : intA[k][i] == intB[k][i];
                }

                Check(same, $"{f.Name} output {k} ({f.Outputs[k].ParamName}): bit-identical on both paths");
            }
        }

        Check(compared == CatalogFacts.FunctionCount, $"compared every function ({compared})");
        Check(named >= 2, $"both price-binding overloads were exercised ({named} price inputs bound)");
        Check(withValues >= compared - 5,
            $"almost every comparison produced values ({withValues}/{compared}) — an all-empty run compares nothing");
    }

    /// <summary>Invokes the typed public wrapper, as an independent second path.</summary>
    private static OutRange? TypedCall(FunctionInfo f, double[][] realOut, int[][] intOut)
    {
        var args = new List<object> { 0, N - 1 };
        var types = new List<Type> { typeof(int), typeof(int) };

        for (int i = 0; i < f.Inputs.Length; i++)
        {
            InputInfo info = f.Inputs[i];
            if (info.Kind == InputKind.Price)
            {
                foreach (PriceComponents comp in info.SignatureOrder)
                {
                    args.Add(ComponentSeries(comp));
                    types.Add(typeof(double[]));
                }
            }
            else if (info.Kind == InputKind.Real)
            {
                args.Add(i == 0 ? Close : High);
                types.Add(typeof(double[]));
            }
            else
            {
                var ints = new int[N];
                for (int k = 0; k < N; k++)
                {
                    ints[k] = k;
                }
                args.Add(ints);
                types.Add(typeof(int[]));
            }
        }

        foreach (OptInputInfo o in f.OptInputs)
        {
            switch (o.Domain)
            {
                case OptInputDomain.RealRange or OptInputDomain.RealList:
                    args.Add(-4e37);            // the real "use the default" sentinel
                    types.Add(typeof(double));
                    break;
                case OptInputDomain.IntegerRange:
                    args.Add(int.MinValue);     // the integer sentinel
                    types.Add(typeof(int));
                    break;
                default:
                    // A choice list takes the sentinel too, since issue #162.
                    // The bound path this is compared against passes the value
                    // explicitly, so this leg is what asserts the typed API maps
                    // (MAType)int.MinValue back to the declared default — which
                    // is not the same value for every function (APO/PPO/PVO
                    // default to EMA, the rest to SMA).
                    args.Add((MAType)int.MinValue);
                    types.Add(typeof(MAType));
                    break;
            }
        }

        for (int k = 0; k < f.Outputs.Length; k++)
        {
            if (f.Outputs[k].Kind == OutputKind.Real)
            {
                realOut[k] = new double[N];
                args.Add(realOut[k]);
                types.Add(typeof(double[]));
            }
            else
            {
                intOut[k] = new int[N];
                args.Add(intOut[k]);
                types.Add(typeof(int[]));
            }
        }

        MethodInfo? m = typeof(Core).GetMethod(f.MethodName, BindingFlags.Public | BindingFlags.Instance,
                                               binder: null, types.ToArray(), modifiers: null);
        return m is null ? null : (OutRange)m.Invoke(new Core(), args.ToArray())!;
    }

    /* ------------------------------------ the binder's defaults ARE the sentinels */

    /// <summary>
    /// An optional parameter left unbound must behave exactly as the documented
    /// sentinel does.
    /// </summary>
    /// <remarks>Every domain is covered, choice lists included: since issue #162
    /// the binder fills an unbound one with the integer sentinel like any other
    /// integer domain, and <c>(MAType)int.MinValue</c> is mapped back to the
    /// declared default by the typed API itself. So this asserts the binder's
    /// contract — unbound means the documented default — over the substitution
    /// rather than around it. The sentinel's own cross-language contract stays
    /// with <c>test_codegen.c</c>'s sweep.</remarks>
    private static void UnboundParametersTakeTheDocumentedDefault()
    {
        int covered = 0;
        foreach (FunctionInfo f in FunctionCatalog.Default)
        {
            if (f.OptInputs.Length == 0 || f.Inputs.Any(i => i.Kind == InputKind.Integer))
            {
                continue;
            }

            var unbound = new double[f.Outputs.Length][];
            var unboundInt = new int[f.Outputs.Length][];
            var explicitly = new double[f.Outputs.Length][];
            var explicitlyInt = new int[f.Outputs.Length][];

            FunctionCall a = Bind(f, unbound, unboundInt);
            FunctionCall b = Bind(f, explicitly, explicitlyInt);
            for (int i = 0; i < f.OptInputs.Length; i++)
            {
                OptInputInfo o = f.OptInputs[i];
                switch (o.Domain)
                {
                    case OptInputDomain.IntegerRange r:
                        b.SetOption(i, r.Default);
                        break;
                    case OptInputDomain.IntegerList l:
                        b.SetOption(i, (int)l.Default);
                        break;
                    default:
                        b.SetOption(i, o.DefaultValue);
                        break;
                }
            }

            RetCode rcA = a.TryInvoke(0, N - 1, out OutRange ra);
            RetCode rcB = b.TryInvoke(0, N - 1, out OutRange rb);
            covered++;

            Check(rcA == rcB && ra.Equals(rb),
                $"{f.Name}: an unbound parameter matches its explicit default ({rcA}/{ra.BegIdx},{ra.Count} vs {rcB}/{rb.BegIdx},{rb.Count})");
            Check(a.Lookback() == b.Lookback(), $"{f.Name}: and the two agree on the lookback");

            for (int k = 0; k < f.Outputs.Length; k++)
            {
                bool same = true;
                for (int i = 0; i < ra.Count; i++)
                {
                    same &= f.Outputs[k].Kind == OutputKind.Real
                        ? BitConverter.DoubleToInt64Bits(unbound[k][i]) == BitConverter.DoubleToInt64Bits(explicitly[k][i])
                        : unboundInt[k][i] == explicitlyInt[k][i];
                }

                Check(same, $"{f.Name} output {k}: unbound and explicit-default values are bit-identical");
            }
        }

        Check(covered > 60, $"covered {covered} functions with optional parameters");
    }

    private static FunctionCall Bind(FunctionInfo f, double[][] realOut, int[][] intOut)
    {
        FunctionCall call = f.CreateCall();
        for (int i = 0; i < f.Inputs.Length; i++)
        {
            InputInfo info = f.Inputs[i];
            if (info.Kind == InputKind.Price)
            {
                foreach (PriceComponents comp in info.SignatureOrder)
                {
                    call.SetPriceInput(i, comp, ComponentSeries(comp));
                }
            }
            else
            {
                call.SetInput(i, i == 0 ? Close : High);
            }
        }

        for (int k = 0; k < f.Outputs.Length; k++)
        {
            if (f.Outputs[k].Kind == OutputKind.Real)
            {
                realOut[k] = new double[N];
                call.SetOutput(k, realOut[k]);
            }
            else
            {
                intOut[k] = new int[N];
                call.SetOutput(k, intOut[k]);
            }
        }

        return call;
    }

    /* -------------------------------------------------- the immutability guard */

    /// <summary>
    /// No metadata type may have a public constructor.
    /// </summary>
    /// <remarks>This is the single guard between the design and a latent
    /// <c>NullReferenceException</c>: <c>default(ImmutableArray&lt;T&gt;)</c>
    /// throws on <c>.Length</c> and on <c>foreach</c>, so the day one of these
    /// records gains a public constructor an uninitialised collection field
    /// becomes reachable from an object that looks valid.</remarks>
    private static void MetadataTypesCannotBeConstructedOutside()
    {
        Type[] closed =
        [
            typeof(FunctionInfo), typeof(InputInfo), typeof(OptInputInfo), typeof(OutputInfo),
            typeof(NamedValue), typeof(NamedRealValue),
            typeof(OptInputDomain.RealRange), typeof(OptInputDomain.IntegerRange),
            typeof(OptInputDomain.IntegerList), typeof(OptInputDomain.RealList),
        ];

        foreach (Type t in closed)
        {
            ConstructorInfo[] ctors = t.GetConstructors(BindingFlags.Public | BindingFlags.Instance);
            Check(ctors.Length == 0, $"{t.Name} has no public constructor (found {ctors.Length})");
        }

        // And every collection the catalogue hands out is actually initialised.
        foreach (FunctionInfo f in FunctionCatalog.Default)
        {
            Check(!f.Inputs.IsDefault && !f.OptInputs.IsDefault && !f.Outputs.IsDefault,
                $"{f.Name}: no default(ImmutableArray) reached a row");
        }
    }

    public static int Run()
    {
        CatalogueIsComplete();
        DomainsAreCoherent();
        FlagsAreExact();
        UnstableIdAgreesWithTheFlag();
        PriceBundlesAreOneInput();
        BinderRejectsMisuse();
        BothCallPathsAgree();
        UnboundParametersTakeTheDocumentedDefault();
        MetadataTypesCannotBeConstructedOutside();

        if (_failures == 0)
        {
            Console.WriteLine($"MetadataTest: ALL PASS ({_checks} checks)");
            return 0;
        }

        Console.WriteLine($"MetadataTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
