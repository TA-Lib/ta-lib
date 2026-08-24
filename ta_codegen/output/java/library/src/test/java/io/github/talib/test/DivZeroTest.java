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
 *  082326 MF,CC  First version. DIV's documented zero-divisor result
 *                (issue #249).
 */

package io.github.talib.test;

import io.github.talib.Core;
import io.github.talib.OutRange;
import io.github.talib.RetCode;
import io.github.talib.TaLibArgumentException;

/**
 * DIV's documented zero-divisor result (issue #249).
 *
 * <p>DIV's published Notes say: "Zero divided by zero gives NaN; anything else
 * divided by zero gives positive or negative infinity. Neither is reported as
 * an error." That was true by construction in every backend and asserted
 * nowhere — no test in any language had ever handed DIV a zero divisor. In
 * Java's case the batch tier throws on a rejected call, so "not an error" is a
 * statement about control flow as much as about the value.
 *
 * <p>One table covers every sign combination IEEE-754 division distinguishes,
 * plus two controls a wrongly-placed guard would break: a zero NUMERATOR over a
 * non-zero divisor stays a signed zero, and ordinary quotients are untouched.
 *
 * <p>The streaming tier takes the same table twice, because it emits two loops.
 * {@code DIV_OpenImpl} carries its own transcription of the batch body (the
 * warm-up fill) and {@code DIV_StepImpl} carries the per-bar one, so a guard
 * added to one is invisible to the other — measured on the C side: an
 * {@code _OpenImpl}-only guard on a zero divisor survived every other assertion
 * in the group. Neither entry point may reject the bar: both guard their INPUTS
 * with {@code Double.isFinite}, and a zero divisor is finite.
 *
 * <p>The C group {@code test_div_zero.c} makes the same assertions against the
 * shipped C library and re-issues each batch call to every language server —
 * including this one, which embeds its own generated copy of Core — and
 * compares each server's output to C's bit for bit.
 */
public class DivZeroTest {

    private static int failures = 0;
    private static int checks = 0;

    private static final double[] NUM = {  0.0,  0.0, -0.0, -0.0,  1.5, -1.5,  1.5, -1.5,  0.0, -0.0,  6.0, -6.0 };
    private static final double[] DEN = {  0.0, -0.0,  0.0, -0.0,  0.0,  0.0, -0.0, -0.0,  4.0,  4.0,  3.0,  3.0 };
    /** {@code null} means "the only defined answer is NaN". */
    private static final Double[] EXPECTED = {
        null, null, null, null,
        Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY,
        Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY,
        0.0, -0.0,          // controls: the sign of a zero numerator survives
        2.0, -2.0           // controls: an ordinary quotient is untouched
    };

    private static String label(int i) {
        return "DIV(" + NUM[i] + ", " + DEN[i] + ")";
    }

    /**
     * Six rows need more than {@code ==}: the four NaN rows, which are not
     * comparable at all, and the two signed-zero rows, where
     * {@code +0.0 == -0.0}. ({@code ==} separates the two infinities on its
     * own; the sign test is there for the zeros.) Comparing raw bits instead
     * would over-assert: the NaN payload is the host's, not the language's.
     */
    private static void check(String tier, int i, double got) {
        checks++;
        Double want = EXPECTED[i];
        boolean ok = (want == null)
                ? Double.isNaN(got)
                : got == want && (Math.copySign(1.0, got) == Math.copySign(1.0, want));
        if (!ok) {
            failures++;
            System.out.println("  FAIL: " + tier + " " + label(i) + " = " + got
                               + ", expected " + (want == null ? "NaN" : want));
        }
    }

    private static void fail(String what) {
        checks++;
        failures++;
        System.out.println("  FAIL: " + what);
    }

    static void batchFullRange() {
        double[] out = new double[NUM.length];
        OutRange r;
        try {
            r = Core.DEFAULT.DIV(0, NUM.length - 1, NUM, DEN, out);
        } catch (RuntimeException e) {
            fail("DIV threw on a zero divisor: " + e);
            return;
        }
        checks++;
        if (r.begIdx() != 0 || r.count() != NUM.length) {
            failures++;
            System.out.println("  FAIL: DIV range " + r.begIdx() + "/" + r.count()
                               + ", expected 0/" + NUM.length);
            return;
        }
        for (int i = 0; i < NUM.length; i++) {
            check("batch", i, out[i]);
        }
    }

    static void batchSubRange() {
        // 4..7 is the +/-Inf block, so a range that silently restarted at 0
        // would land on the NaN rows and fail rather than looking right.
        double[] out = new double[NUM.length];
        OutRange r = Core.DEFAULT.DIV(4, 7, NUM, DEN, out);
        checks++;
        if (r.begIdx() != 4 || r.count() != 4) {
            failures++;
            System.out.println("  FAIL: DIV sub-range " + r.begIdx() + "/" + r.count() + ", expected 4/4");
            return;
        }
        for (int i = 0; i < r.count(); i++) {
            check("sub-range", 4 + i, out[i]);
        }
    }

    static void floatOverload() {
        // The float[] overload widens each element to double before dividing,
        // so it divides the same two numbers -- every literal above is exactly
        // representable as a float.
        float[] n = new float[NUM.length], d = new float[DEN.length];
        for (int i = 0; i < NUM.length; i++) { n[i] = (float) NUM[i]; d[i] = (float) DEN[i]; }
        double[] out = new double[NUM.length];
        Core.DEFAULT.DIV(0, NUM.length - 1, n, d, out);
        for (int i = 0; i < NUM.length; i++) {
            check("float", i, out[i]);
        }
    }

    static void fillLoop() {
        // DIV_OpenImpl, not DIV_StepImpl: a separate transcription of the batch
        // body, and the only assertion that reaches it.
        double[] out = new double[NUM.length];
        Core.DIV_Stream s;
        OutRange r;
        try {
            s = Core.DEFAULT.DIV_OpenAndFill(NUM, DEN, out);
            r = s.outRange();
        } catch (RuntimeException e) {
            fail("DIV_OpenAndFill threw on a zero divisor: " + e);
            return;
        }
        checks++;
        if (r.begIdx() != 0 || r.count() != NUM.length) {
            failures++;
            System.out.println("  FAIL: DIV_OpenAndFill range " + r.begIdx() + "/" + r.count()
                               + ", expected 0/" + NUM.length);
            return;
        }
        for (int i = 0; i < NUM.length; i++) {
            check("fill", i, out[i]);
        }
        // The same loop with no output array: only the last row survives.
        check("open-last", NUM.length - 1, Core.DEFAULT.DIV_Open(NUM, DEN).value());
    }

    static void streamingTier() {
        double[] head0 = { NUM[0] }, head1 = { DEN[0] };
        Core.DIV_Stream s;
        try {
            s = Core.DEFAULT.DIV_Open(head0, head1);
        } catch (RuntimeException e) {
            fail("DIV_Open threw on a zero divisor: " + e);
            return;
        }
        check("open", 0, s.value());

        for (int i = 1; i < NUM.length; i++) {
            // A zero divisor is a FINITE input. The tier rejects non-finite
            // INPUTS; a guard widened to the OUTPUT would surface here and
            // nowhere else.
            double peeked, updated;
            try {
                peeked = s.peek(NUM[i], DEN[i]);
                updated = s.update(NUM[i], DEN[i]);
            } catch (RuntimeException e) {
                fail("stream rejected " + label(i) + ": " + e);
                return;
            }
            check("peek", i, peeked);
            check("update", i, updated);
            checks++;
            if (Double.doubleToRawLongBits(peeked) != Double.doubleToRawLongBits(updated)) {
                failures++;
                System.out.println("  FAIL: peek != update at " + label(i));
            }
        }
        checks++;
        if (s.outRange().count() != NUM.length) {
            failures++;
            System.out.println("  FAIL: stream committed " + s.outRange().count()
                               + " bars, expected " + NUM.length);
        }
    }

    static void nonFiniteBarStillRejected() {
        // The control for the case above: the tier's own contract is unchanged,
        // so "does not reject a zero divisor" cannot be read as "rejects
        // nothing".
        Core.DIV_Stream s = Core.DEFAULT.DIV_Open(new double[] { 1.0 }, new double[] { 2.0 });
        for (double bad : new double[] { Double.NaN, Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY }) {
            for (int slot = 0; slot < 2; slot++) {
                double a0 = slot == 0 ? bad : 1.0, a1 = slot == 0 ? 1.0 : bad;
                // peek as well as update: they carry the guard separately, so a
                // control on one of them leaves the other's removable.
                mustReject("peek", bad, () -> s.peek(a0, a1));
                mustReject("update", bad, () -> s.update(a0, a1));
            }
        }
    }

    private static void mustReject(String what, double bad, Runnable body) {
        checks++;
        try {
            body.run();
            failures++;
            System.out.println("  FAIL: " + what + " accepted a non-finite bar (" + bad + ")");
        } catch (TaLibArgumentException e) {
            if (e.retCode() != RetCode.BadParam) {
                failures++;
                System.out.println("  FAIL: " + what + " on " + bad + " gave " + e.retCode());
            }
        }
    }

    public static void main(String[] args) {
        batchFullRange();
        batchSubRange();
        floatOverload();
        fillLoop();
        streamingTier();
        nonFiniteBarStillRejected();

        // Green must also be non-vacuous: a leg that stopped running would
        // otherwise leave "ALL PASS" printing a smaller number nobody reads.
        final int n = NUM.length;
        final int expected = (1 + n)                    // batch, full range
                           + (1 + 4)                    // batch, sub-range
                           + n                          // the float[] overload
                           + (1 + n + 1)                // fill: range, rows, open's last
                           + (1 + 3 * (n - 1) + 1)      // open, peek/update/bits, outRange
                           + 12;                        // peek+update reject 3 x 2 slots
        if (failures == 0 && checks != expected) {
            System.out.println("DivZeroTest: took " + checks + " checks, expected " + expected);
            System.exit(1);
        }

        if (failures == 0) {
            System.out.println("DivZeroTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("DivZeroTest: " + failures + " of " + checks + " checks FAILED");
            System.exit(1);
        }
    }
}
