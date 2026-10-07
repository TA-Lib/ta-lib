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
 *  100626 MF,CC  First Version (issue #506).
 */

/* Hand-written test; ta_codegen never opens this file. */

using System;
using System.Buffers;
using System.Collections.Generic;
using TALib;

namespace TALib.Test;

/// <summary>
/// A batch body that rents its scratch from the shared pool returns the same
/// bits whatever the pool's arrays held before: nothing reads a rented element
/// it did not write first.
/// </summary>
/// <remarks>
/// Nothing else sees a stale read: every other gate runs with a pool whose
/// arrays hold the previous call's finite values.
/// </remarks>
public static class PooledScratchTest
{
    private const int N = 300;
    private static int _failures;
    private static int _checks;

    private static readonly double[] High = new double[N];
    private static readonly double[] Low = new double[N];
    private static readonly double[] Close = new double[N];
    private static readonly double[] Volume = new double[N];
    private static readonly double[] Periods = new double[N];

    private delegate OutRange Call(Core core, int start, int end, double[][] o);

    // Fills what the next rents on this thread will receive, in every bucket
    // these bodies can ask for.
    private static void Poison(double value, int intValue)
    {
        var doubles = new List<double[]>();
        var ints = new List<int[]>();
        for (int size = 16; size <= 1024; size *= 2)
        {
            for (int k = 0; k < 8; k++)
            {
                doubles.Add(ArrayPool<double>.Shared.Rent(size));
                ints.Add(ArrayPool<int>.Shared.Rent(size));
            }
        }
        foreach (double[] a in doubles)
        {
            Array.Fill(a, value);
            ArrayPool<double>.Shared.Return(a);
        }
        foreach (int[] a in ints)
        {
            Array.Fill(a, intValue);
            ArrayPool<int>.Shared.Return(a);
        }
    }

    private static string Run(Core core, Call call, int start, int end)
    {
        var o = new double[3][];
        for (int i = 0; i < o.Length; i++)
        {
            o[i] = new double[N];
            Array.Fill(o[i], -7.0);
        }
        OutRange r = call(core, start, end, o);
        long hash = 17;
        foreach (double[] a in o)
        {
            foreach (double d in a)
            {
                hash = unchecked(hash * 31 + BitConverter.DoubleToInt64Bits(d));
            }
        }
        return $"{r.BegIdx},{r.Count}:{hash}";
    }

    /// <summary>Runs every case; returns 0 on success, 1 on any failure.</summary>
    public static int Run()
    {
        var rng = new Random(12345);
        double px = 100;
        for (int i = 0; i < N; i++)
        {
            px = Math.Max(5, px + Math.Round(rng.NextDouble() * 4 - 2, 2));
            High[i] = px + Math.Round(rng.NextDouble() * 2, 2);
            Low[i] = px - Math.Round(rng.NextDouble() * 2, 2);
            Close[i] = Math.Round(Low[i] + (High[i] - Low[i]) * rng.NextDouble(), 2);
            Volume[i] = rng.Next(100, 100000);
            Periods[i] = rng.Next(2, 30);
        }

        var cases = new (string Name, Call Call)[]
        {
            ("ADXR", (k, s, e, o) => k.Adxr(s, e, High, Low, Close, 14, o[0])),
            ("APO", (k, s, e, o) => k.Apo(s, e, Close, 12, 26, MAType.EMA, o[0])),
            ("BBANDS", (k, s, e, o) => k.Bbands(s, e, Close, 9, 2.0, 1.5, MAType.EMA, o[0], o[1], o[2])),
            ("BBW", (k, s, e, o) => k.Bbw(s, e, Close, 9, 2.0, 1.5, MAType.WMA, o[0])),
            ("CRSI", (k, s, e, o) => k.Crsi(s, e, Close, 3, 2, 30, o[0])),
            ("KC", (k, s, e, o) => k.Kc(s, e, High, Low, Close, 20, 10, 2.0, o[0], o[1], o[2])),
            ("KSTEXT", (k, s, e, o) => k.Kstext(s, e, Close, 10, 15, 20, 30, 10, 10, 10, 15, 9, MAType.EMA, MAType.SMA, o[0], o[1])),
            ("MACDEXT", (k, s, e, o) => k.Macdext(s, e, Close, 12, MAType.EMA, 26, MAType.SMA, 9, MAType.EMA, o[0], o[1], o[2])),
            ("MAVP", (k, s, e, o) => k.Mavp(s, e, Close, Periods, 2, 30, MAType.SMA, o[0])),
            ("PERCENTB", (k, s, e, o) => k.Percentb(s, e, Close, 9, 2.0, 1.5, MAType.EMA, o[0])),
            ("PPO", (k, s, e, o) => k.Ppo(s, e, Close, 12, 26, MAType.EMA, o[0])),
            ("PVO", (k, s, e, o) => k.Pvo(s, e, Volume, 12, 26, MAType.EMA, o[0])),
            ("RVIR", (k, s, e, o) => k.Rvir(s, e, High, Low, 10, 14, o[0])),
            ("STOCH", (k, s, e, o) => k.Stoch(s, e, High, Low, Close, 5, 3, MAType.EMA, 3, MAType.SMA, o[0], o[1])),
            ("STOCHF", (k, s, e, o) => k.Stochf(s, e, High, Low, Close, 5, 3, MAType.EMA, o[0], o[1])),
            ("STOCHRSI", (k, s, e, o) => k.Stochrsi(s, e, Close, 14, 5, 3, MAType.EMA, o[0], o[1])),
        };

        // The poison has to arrive, or every comparison below proves nothing.
        Poison(double.NaN, int.MinValue / 2);
        double[] probe = ArrayPool<double>.Shared.Rent(N);
        _checks++;
        if (!double.IsNaN(probe[0]) || !double.IsNaN(probe[^1]))
        {
            _failures++;
            Console.WriteLine("FAIL the pool did not hand back a poisoned array");
        }
        ArrayPool<double>.Shared.Return(probe);

        var core = new Core();
        foreach ((string name, Call call) in cases)
        {
            foreach ((int start, int end) in new[] { (0, N - 1), (120, N - 1) })
            {
                Poison(0.0, 0);
                string clean = Run(core, call, start, end);
                _checks++;
                if (clean.StartsWith("0,0:", StringComparison.Ordinal))
                {
                    _failures++;
                    Console.WriteLine($"FAIL {name} [{start},{end}] produced nothing");
                }
                foreach ((double fill, int intFill) in new[] { (double.NaN, int.MinValue / 2), (1e300, 1 << 28), (-3.25, -1) })
                {
                    Poison(fill, intFill);
                    string got = Run(core, call, start, end);
                    _checks++;
                    if (got != clean)
                    {
                        _failures++;
                        Console.WriteLine($"FAIL {name} [{start},{end}] pool filled with {fill:R}: {got}, want {clean}");
                    }
                }
            }
        }

        if (_failures == 0)
        {
            Console.WriteLine($"PooledScratchTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"PooledScratchTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
