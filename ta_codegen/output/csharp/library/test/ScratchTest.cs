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
using TALib;

namespace TALib.Test;

/// <summary>
/// <c>ReturnScratch</c> gives a rented buffer back once: it clears the caller's
/// reference, and a second return of it throws instead of reaching the pool.
/// </summary>
public static class ScratchTest
{
    private static int _failures;
    private static int _checks;

    private static void Check(string what, bool ok)
    {
        _checks++;
        if (!ok)
        {
            _failures++;
            Console.WriteLine($"FAIL {what}");
        }
    }

    /// <summary>Runs every case; returns 0 on success, 1 on any failure.</summary>
    public static int Run()
    {
        foreach (int length in new[] { 0, 1, 100, 200_000 })
        {
            double[]? rented = ArrayPool<double>.Shared.Rent(length);
            Check($"rent {length} is long enough", rented.Length >= length);
            Core.ReturnScratch(ref rented);
            Check($"return {length} clears the reference", rented is null);
            bool threw = false;
            try
            {
                Core.ReturnScratch(ref rented);
            }
            catch (ArgumentNullException)
            {
                threw = true;
            }
            Check($"second return {length} throws", threw);
        }

        int[]? ints = ArrayPool<int>.Shared.Rent(64);
        Core.ReturnScratch(ref ints);
        Check("int buffer cleared", ints is null);

        if (_failures == 0)
        {
            Console.WriteLine($"ScratchTest: ALL PASS ({_checks} checks)");
            return 0;
        }
        Console.WriteLine($"ScratchTest: {_failures} of {_checks} checks FAILED");
        return 1;
    }
}
