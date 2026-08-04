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
 *  072626 MF,CC  First Version — the OutRange batch contract. Absorbs the
 *                non-vacuous cases of the retired junit CoreTest.
 */

package io.github.talib.test;

import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

/**
 * The batch API's contract: what {@link OutRange} means, and which misuses
 * throw what.
 *
 * <p>Junit-free by design (see {@link CoreApiTest}). Numerical correctness is
 * not this file's job — {@code ta_regtest --codegen} and {@code --xlang-hash}
 * prove that against the C reference for all 168 functions. What is tested here
 * is the surface a Java caller touches, which no cross-language harness sees.
 *
 * <p>It absorbs the two non-vacuous cases of the retired junit {@code CoreTest}
 * (a MAX call with known outputs, and the CMO {@code FLT_EPSILON} case that
 * pins the untouched tail of the output buffer). {@code CoreTest}'s other
 * methods — {@code testMFI}, {@code testHT}, {@code testMA_MAMA},
 * {@code test_MACD} — asserted nothing whatsoever (the last carried a literal
 * {@code // TODO Add tests of outputs}) and were dropped rather than ported.
 */
public class BatchApiTest {

    private static int failures = 0;
    private static int checks = 0;

    private static void check(boolean condition, String what) {
        checks++;
        if (!condition) {
            failures++;
            System.out.println("  FAIL: " + what);
        }
    }

    private static void checkThrows(Class<? extends RuntimeException> expected,
                                    Runnable body, String what) {
        checks++;
        try {
            body.run();
            failures++;
            System.out.println("  FAIL: " + what + " (no exception thrown)");
        } catch (RuntimeException e) {
            if (!expected.isInstance(e)) {
                failures++;
                System.out.println("  FAIL: " + what + " (threw " + e.getClass().getName() + ")");
            }
        }
    }

    private static double[] closes(int n) {
        double[] out = new double[n];
        for (int i = 0; i < n; i++) {
            out[i] = 100.0 + 10.0 * Math.sin(i / 7.0) + 3.0 * Math.cos(i / 3.0);
        }
        return out;
    }

    /* ---------------------------------------------------- ported from CoreTest */

    /** MAX over three bars with a period of 2 — known begIdx, count and values. */
    static void maxWithKnownOutputs() {
        double[] input = { 2.0, 1.2, 1.5 };
        double[] output = new double[3];

        OutRange r = Core.DEFAULT.max(0, 2, input, 2, output);

        check(r.begIdx() == 1, "MAX begIdx == 1 (got " + r.begIdx() + ")");
        check(r.count() == 2, "MAX count == 2 (got " + r.count() + ")");
        check(output[0] == 2.0, "MAX[0] == 2.0");
        check(output[1] == 1.5, "MAX[1] == 1.5");
        check(r.endIdx() == 3, "MAX endIdx == begIdx + count");
        check(!r.isEmpty(), "MAX range is not empty");
    }

    /** The first output lands exactly at the lookback. */
    static void begIdxEqualsLookback() {
        double[] in = closes(200);
        double[] out = new double[in.length];

        OutRange r = Core.DEFAULT.movingAverage(0, in.length - 1, in, 10, MAType.Sma, out);
        check(r.begIdx() == Core.DEFAULT.movingAverageLookback(10, MAType.Sma),
              "SMA begIdx == lookback");
        check(r.count() == in.length - r.begIdx(), "SMA count fills to the end");
    }

    /**
     * CMO over ±FLT_EPSILON data: the output starts at the lookback and nothing
     * past {@code count} is written. The untouched tail is the real assertion —
     * it catches a writer running past the range it reported.
     */
    static void cmoLeavesTheTailUntouched() {
        final double FLT_EPSILON = 1.192092896e-07;
        final double SENTINEL = -3e37;

        double[] in = new double[100];
        for (int i = 0; i < in.length; i++) {
            in[i] = ((i % 2) == 0 ? 1.0 : -1.0) * FLT_EPSILON;
        }
        double[] out = new double[100];
        java.util.Arrays.fill(out, SENTINEL);

        int lookback = Core.DEFAULT.cmoLookback(Integer.MIN_VALUE);
        OutRange r = Core.DEFAULT.cmo(0, in.length - 1, in, Integer.MIN_VALUE, out);

        check(r.begIdx() == lookback, "CMO begIdx == lookback");
        check(r.count() > 0, "CMO produced values (so the tail check is not vacuous)");
        boolean tailUntouched = true;
        for (int i = r.count(); i < out.length; i++) {
            if (out[i] != SENTINEL) {
                tailUntouched = false;
            }
        }
        check(tailUntouched, "CMO wrote nothing past count");
    }

    /* -------------------------------------------------- the OutRange contract */

    /**
     * The invariant this whole error model hangs on: a valid range shorter than
     * the lookback is a SUCCESS with no values, never an exception. Matches C's
     * {@code TA_SUCCESS} + {@code outNBElement == 0}.
     */
    static void shortRangeIsAnEmptySuccessNotAnException() {
        double[] in = closes(10);
        double[] out = new double[10];

        check(Core.DEFAULT.smaLookback(30) > 9,
              "the 30-period lookback really does exceed this 10-bar range");

        OutRange r = Core.DEFAULT.sma(0, in.length - 1, in, 30, out);
        check(r.count() == 0, "too-short range yields count == 0");
        check(r.isEmpty(), "too-short range is isEmpty()");
        check(r.begIdx() == 0, "empty range reports begIdx 0");
    }

    /** Misuse mapping. Note what is absent: no code path returns a RetCode. */
    static void misuseThrowsTheDocumentedException() {
        final double[] in = closes(100);
        final double[] out = new double[100];

        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.sma(-1, 50, in, 10, out), "negative startIdx -> IndexOutOfBounds");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.sma(0, -1, in, 10, out), "negative endIdx -> IndexOutOfBounds");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.sma(50, 10, in, 10, out), "endIdx < startIdx -> IndexOutOfBounds");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.sma(0, 50, in, 0, out), "period below range -> IllegalArgument");
        // The cast is required, not incidental: `null` alone is ambiguous between
        // the double[] and float[] overloads. Real callers pass a typed array.
        checkThrows(NullPointerException.class,
            () -> Core.DEFAULT.sma(0, 50, (double[]) null, 10, out),
            "null input -> NullPointerException");

        // Two outputs sharing one array has no correct answer (issue #108).
        final double[] shared = new double[100];
        final double[] third = new double[100];
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.bbands(0, 50, in, 20, 2.0, 2.0, MAType.Sma, shared, shared, third),
            "aliased output arrays -> IllegalArgument");
    }

    /**
     * The public surface exposes no unguarded tier. Pinned by reflection over the
     * shipped Core rather than by a string assertion on generator output.
     */
    static void noUnguardedTierOnThePublicSurface() {
        int leaked = 0;
        for (java.lang.reflect.Method m : Core.class.getMethods()) {
            if (m.getName().endsWith("Unguarded") || m.getName().endsWith("UnguardedInternal")) {
                leaked++;
            }
        }
        check(leaked == 0, "no Unguarded method survives on the public Core surface");
        // Non-vacuity: the reflection actually sees the surface it is asserting over.
        int sma = 0;
        for (java.lang.reflect.Method m : Core.class.getMethods()) {
            if (m.getName().equals("sma")) {
                sma++;
            }
        }
        check(sma >= 2, "reflection sees the sma overloads it is filtering over");
    }

    /** The float overload adopts the identical shape (C's TA_S_* parity). */
    static void floatOverloadHasTheSameShape() {
        double[] in = closes(100);
        float[] inF = new float[100];
        for (int i = 0; i < in.length; i++) {
            inF[i] = (float) in[i];
        }
        double[] outD = new double[100];
        double[] outF = new double[100];

        OutRange rd = Core.DEFAULT.sma(0, in.length - 1, in, 10, outD);
        OutRange rf = Core.DEFAULT.sma(0, inF.length - 1, inF, 10, outF);

        check(rd.equals(rf), "float overload reports the same OutRange");
        check(rf.count() > 0, "float overload produced values");
    }

    /** OutRange is a value type: equality by components, usable as a key. */
    static void outRangeValueSemantics() {
        check(new OutRange(3, 7).equals(new OutRange(3, 7)), "OutRange equals by value");
        check(!new OutRange(3, 7).equals(new OutRange(3, 8)), "OutRange distinguishes count");
        check(new OutRange(3, 7).hashCode() == new OutRange(3, 7).hashCode(),
              "OutRange hashCode agrees with equals");
        check(OutRange.EMPTY.isEmpty() && OutRange.EMPTY.count() == 0, "OutRange.EMPTY");
        check(new OutRange(3, 7).toString().contains("begIdx"), "OutRange toString names components");
    }

    public static void main(String[] args) {
        maxWithKnownOutputs();
        begIdxEqualsLookback();
        cmoLeavesTheTailUntouched();
        shortRangeIsAnEmptySuccessNotAnException();
        misuseThrowsTheDocumentedException();
        noUnguardedTierOnThePublicSurface();
        floatOverloadHasTheSameShape();
        outRangeValueSemantics();

        if (failures == 0) {
            System.out.println("BatchApiTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("BatchApiTest: " + failures + " of " + checks + " checks FAILED");
            System.exit(1);
        }
    }
}
