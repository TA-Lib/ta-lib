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
 *  FI       Fernando J. Iglesias García (github @iglesias)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  070726 FI,CC  First version. Regression guard for the single-precision
 *                (float[] input) vector-arithmetic overflow reported in
 *                PR #33.
 */

package com.tictactec.ta.lib.test;

import junit.framework.TestCase;

import com.tictactec.ta.lib.Core;
import com.tictactec.ta.lib.MInteger;
import com.tictactec.ta.lib.RetCode;

/* The single-precision (float[] input) overloads of add/sub/mult/div write a
 * double[] output. Java evaluates float*float in float, so before PR #33 the
 * result overflowed to Float/Double.POSITIVE_INFINITY *before* being widened
 * to the double output (e.g. 3e38f * 10f -> inf). Casting the first operand to
 * double first performs the arithmetic in double, so a result beyond FLT_MAX
 * (~3.4e38) is representable in the double output.
 *
 * Each case feeds float operands whose exact-precision result overflows float
 * range and asserts the output is finite and bit-equal to the same operation
 * done in double. */
public class TestSMathOverflow extends TestCase {

    private final Core core = new Core();

    private void assertFiniteAndEqual(String op, RetCode rc, double out, double expected) {
        assertEquals(op + " retCode", RetCode.Success, rc);
        assertFalse(op + " overflowed to non-finite value " + out
                        + " (float arithmetic before widening)",
                    Double.isInfinite(out) || Double.isNaN(out));
        assertEquals(op + " value", expected, out, Math.abs(expected) * 1e-9);
    }

    public void testAddFloatOverflow() {
        float[] a = { 3.0e38f }, b = { 3.0e38f };   // 6e38 > FLT_MAX
        double[] o = { -1.0 };
        RetCode rc = core.add(0, 0, a, b, new MInteger(), new MInteger(), o);
        assertFiniteAndEqual("ADD", rc, o[0], (double) a[0] + (double) b[0]);
    }

    public void testSubFloatOverflow() {
        float[] a = { 3.0e38f }, b = { -3.0e38f };  // 6e38 > FLT_MAX
        double[] o = { -1.0 };
        RetCode rc = core.sub(0, 0, a, b, new MInteger(), new MInteger(), o);
        assertFiniteAndEqual("SUB", rc, o[0], (double) a[0] - (double) b[0]);
    }

    public void testMultFloatOverflow() {
        float[] a = { 3.0e38f }, b = { 10.0f };     // 3e39 > FLT_MAX (PR #33)
        double[] o = { -1.0 };
        RetCode rc = core.mult(0, 0, a, b, new MInteger(), new MInteger(), o);
        assertFiniteAndEqual("MULT", rc, o[0], (double) a[0] * (double) b[0]);
    }

    public void testDivFloatOverflow() {
        float[] a = { 3.0e38f }, b = { 1.0e-3f };   // 3e41 > FLT_MAX
        double[] o = { -1.0 };
        RetCode rc = core.div(0, 0, a, b, new MInteger(), new MInteger(), o);
        assertFiniteAndEqual("DIV", rc, o[0], (double) a[0] / (double) b[0]);
    }
}
