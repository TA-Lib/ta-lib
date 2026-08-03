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
 *  080226 MF,CC  First Version.
 */

/* Hand-written test runner; ta_codegen never opens this file. */

using System;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;

namespace TALib.Test;

/// <summary>
/// Runs every C# suite in one process and aggregates the exit codes.
/// </summary>
/// <remarks>
/// <para>One entry point rather than one Main per suite: a single assembly can
/// only have one, and running each separately would pay the startup cost N
/// times. Prints the running framework, so if the library ever multi-targets
/// again the log says which TFM each pass actually ran on rather than leaving
/// it to be inferred.</para>
/// <para>Suites are DISCOVERED, not listed. The generator's Java runner carries
/// the reason in a comment: a hardcoded roster is how the retired Java
/// <c>AllTests</c> went vacuous — it named one class, a later change deleted
/// that class, and the runner kept passing. Here any
/// <c>TALib.Test.*Test</c> class is picked up automatically, and one without a
/// public static <c>int Run()</c> is a hard error rather than a silent
/// skip.</para>
/// </remarks>
public static class AllTests
{
    public static int Main(string[] args)
    {
        Console.WriteLine($"TALib C# tests on {RuntimeInformation.FrameworkDescription}");

        Type[] suites = typeof(AllTests).Assembly.GetTypes()
            .Where(t => t.IsClass && t.Namespace == "TALib.Test" && t.Name.EndsWith("Test", StringComparison.Ordinal))
            .OrderBy(t => t.Name, StringComparer.Ordinal)
            .ToArray();

        if (suites.Length == 0)
        {
            Console.WriteLine("FAIL: no test suites discovered (expected TALib.Test.*Test classes)");
            return 1;
        }

        int failed = 0;
        foreach (Type suite in suites)
        {
            MethodInfo? run = suite.GetMethod("Run", BindingFlags.Public | BindingFlags.Static,
                                              binder: null, types: Type.EmptyTypes, modifiers: null);
            if (run == null || run.ReturnType != typeof(int))
            {
                Console.WriteLine($"FAIL: {suite.Name} has no public static int Run()");
                failed++;
                continue;
            }
            failed += (int)run.Invoke(null, null)!;
        }

        if (failed != 0)
        {
            Console.WriteLine($"{failed} suite(s) FAILED");
            return 1;
        }
        Console.WriteLine($"All {suites.Length} C# suite(s) passed.");
        return 0;
    }
}
