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
 *  072626 MF,CC  Ported off junit/MInteger to the OutRange API.
 */

package io.github.talib.test;

import io.github.talib.Core;
import io.github.talib.OutRange;

/**
 * The {@code TA_S_*} float-input contract (PR #33): a float overload must do its
 * arithmetic in double, not in float.
 *
 * <p>Every float variant takes {@code float[]} inputs but writes a
 * {@code double[]} output. Java evaluates {@code float * float} in float, so
 * before PR #33 the result overflowed to infinity <i>before</i> being widened to
 * the double output (e.g. {@code 3e38f * 10f -> inf}). Casting the first operand
 * to double first performs the arithmetic in double, so a result beyond
 * {@code FLT_MAX} (~3.4e38) survives into the double output.
 *
 * <p>Each case feeds float operands whose exact result overflows float range and
 * asserts the output is finite and equal to the same operation done in double.
 */
public class SMathOverflowTest {

    private static int failures = 0;
    private static int checks = 0;

    private static void checkFiniteAndEqual(String op, OutRange r, double out, double expected) {
        checks++;
        String bad = null;
        if (r.count() != 1) {
            bad = "expected one value, got " + r.count();
        } else if (Double.isInfinite(out) || Double.isNaN(out)) {
            bad = "overflowed to non-finite " + out + " (float arithmetic before widening)";
        } else if (Math.abs(out - expected) > Math.abs(expected) * 1e-9) {
            bad = "value " + out + " != expected " + expected;
        }
        if (bad != null) {
            failures++;
            System.out.println("  FAIL: " + op + " " + bad);
        }
    }

    static void addFloatOverflow() {
        float[] a = { 3.0e38f }, b = { 3.0e38f };   // 6e38 > FLT_MAX
        double[] o = { -1.0 };
        OutRange r = Core.DEFAULT.ADD(0, 0, a, b, o);
        checkFiniteAndEqual("ADD", r, o[0], (double) a[0] + (double) b[0]);
    }

    static void subFloatOverflow() {
        float[] a = { 3.0e38f }, b = { -3.0e38f };  // 6e38 > FLT_MAX
        double[] o = { -1.0 };
        OutRange r = Core.DEFAULT.SUB(0, 0, a, b, o);
        checkFiniteAndEqual("SUB", r, o[0], (double) a[0] - (double) b[0]);
    }

    static void multFloatOverflow() {
        float[] a = { 3.0e38f }, b = { 10.0f };     // 3e39 > FLT_MAX (PR #33)
        double[] o = { -1.0 };
        OutRange r = Core.DEFAULT.MULT(0, 0, a, b, o);
        checkFiniteAndEqual("MULT", r, o[0], (double) a[0] * (double) b[0]);
    }

    static void divFloatOverflow() {
        float[] a = { 3.0e38f }, b = { 1.0e-3f };   // 3e41 > FLT_MAX
        double[] o = { -1.0 };
        OutRange r = Core.DEFAULT.DIV(0, 0, a, b, o);
        checkFiniteAndEqual("DIV", r, o[0], (double) a[0] / (double) b[0]);
    }

    public static void main(String[] args) {
        addFloatOverflow();
        subFloatOverflow();
        multFloatOverflow();
        divFloatOverflow();

        if (failures == 0) {
            System.out.println("SMathOverflowTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("SMathOverflowTest: " + failures + " of " + checks + " checks FAILED");
            System.exit(1);
        }
    }
}
