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
 *  081826 MF,CC  Array-argument checks (#172 C2).
 *  082526 MF,CC  Declinable outputs and distinct empty ones (#262).
 */

package io.github.talib.test;

import io.github.talib.Core;
import io.github.talib.InsufficientHistoryException;
import io.github.talib.MAType;
import io.github.talib.OutRange;
import io.github.talib.RetCode;
import io.github.talib.TaLibArgumentException;
import io.github.talib.TaLibFailure;

import java.util.Arrays;
import java.util.EnumSet;
import java.util.Set;

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

    /* The streaming-opener cases below are counted apart from the batch ones:
     * sharing a counter would let a deleted streaming case hide behind a batch
     * one, and the floors in main() are what make a deletion loud. */
    private static int s1Reject = 0;
    private static int s4Reject = 0;
    private static int s4Accept = 0;
    private static int s5Reject = 0;
    /** Rule B6a at the opener — a declined output, counted apart from S5's. */
    private static int b6aOpen = 0;
    /** Rule U6a at {@code updateAndFill} — counted apart from the opener's. */
    private static int u6aFill = 0;

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

    /**
     * Same, plus the message has to name the things a caller needs to fix it.
     * A check that throws the right type with a useless message is half a check:
     * the length rejections all share one exception class, so the message is the
     * only thing that says <i>which</i> array and by how much.
     */
    private static void checkThrows(Class<? extends RuntimeException> expected,
                                    Runnable body, String what, String... needles) {
        checks++;
        try {
            body.run();
            failures++;
            System.out.println("  FAIL: " + what + " (no exception thrown)");
            return;
        } catch (RuntimeException e) {
            if (!expected.isInstance(e)) {
                failures++;
                System.out.println("  FAIL: " + what + " (threw " + e.getClass().getName()
                    + ": " + e.getMessage() + ")");
                return;
            }
            String msg = String.valueOf(e.getMessage());
            for (String needle : needles) {
                if (!msg.contains(needle)) {
                    failures++;
                    System.out.println("  FAIL: " + what + " (message \"" + msg
                        + "\" omits \"" + needle + "\")");
                    return;
                }
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

        OutRange r = Core.DEFAULT.MAX(0, 2, input, 2, output);

        check(r.begIdx() == 1, "MAX begIdx == 1 (got " + r.begIdx() + ")");
        check(r.count() == 2, "MAX count == 2 (got " + r.count() + ")");
        check(output[0] == 2.0, "MAX[0] == 2.0");
        check(output[1] == 1.5, "MAX[1] == 1.5");
        check(!r.isEmpty(), "MAX range is not empty");
    }

    /** The first output lands exactly at the lookback. */
    static void begIdxEqualsLookback() {
        double[] in = closes(200);
        double[] out = new double[in.length];

        OutRange r = Core.DEFAULT.MA(0, in.length - 1, in, 10, MAType.SMA, out);
        check(r.begIdx() == Core.DEFAULT.MA_Lookback(10, MAType.SMA),
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

        int lookback = Core.DEFAULT.CMO_Lookback(Integer.MIN_VALUE);
        OutRange r = Core.DEFAULT.CMO(0, in.length - 1, in, Integer.MIN_VALUE, out);

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

        check(Core.DEFAULT.SMA_Lookback(30) > 9,
              "the 30-period lookback really does exceed this 10-bar range");

        OutRange r = Core.DEFAULT.SMA(0, in.length - 1, in, 30, out);
        check(r.count() == 0, "too-short range yields count == 0");
        check(r.isEmpty(), "too-short range is isEmpty()");
        check(r.begIdx() == 0, "empty range reports begIdx 0");
    }

    /** Misuse mapping. Note what is absent: no code path returns a RetCode. */
    static void misuseThrowsTheDocumentedException() {
        final double[] in = closes(100);
        final double[] out = new double[100];

        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(-1, 50, in, 10, out), "negative startIdx -> IndexOutOfBounds");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(0, -1, in, 10, out), "negative endIdx -> IndexOutOfBounds");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(50, 10, in, 10, out), "endIdx < startIdx -> IndexOutOfBounds");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 50, in, 0, out), "period below range -> IllegalArgument");
        // The cast is required, not incidental: `null` alone is ambiguous between
        // the double[] and float[] overloads. Real callers pass a typed array.
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 50, (double[]) null, 10, out),
            "null input -> IllegalArgument");

        // Two outputs sharing one array has no correct answer (issue #108).
        final double[] shared = new double[100];
        final double[] third = new double[100];
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.BBANDS(0, 50, in, 20, 2.0, 2.0, MAType.SMA, shared, shared, third),
            "aliased output arrays -> IllegalArgument");
    }

    /* ------------------------------------------- array-argument checks (#172 C2) */

    /*
     * The four misuses in issue #172's C2 table. Each one used to be an
     * ArrayIndexOutOfBoundsException (or a JVM helpful-NPE) raised from deep
     * inside the algorithm, after the output buffer was already partly written
     * and with no OutRange to say how far the call had got. C cannot do better —
     * it is handed bare pointers and has no sizes — but Java arrays carry their
     * length.
     *
     * The bound is the one the Rust backend asserts and the cross-language
     * harness verifies: an input must reach endIdx, and an output must hold the
     * values actually PRODUCED — endIdx - max(startIdx, lookback) + 1, which for
     * a range starting below the lookback is shorter than the range. The exact
     * boundary is pinned in bothSidesOfTheOutputBound() below; without that pair
     * every assertion here would still pass against a bound that was merely
     * "some number", which is the vacuity this file's whole method is against.
     */

    /** Requesting more than the input holds names the input, not a bar index. */
    static void undersizedInputIsRejected() {
        final double[] in = closes(200);
        final double[] out = new double[501];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 500, in, 10, out),
            "endIdx past the input end -> IllegalArgument",
            "SMA", "inReal", "200", "501");
    }

    /** Two input series of different lengths: the short one is named. */
    static void mismatchedInputLengthsAreRejected() {
        final double[] longer = closes(200);
        final double[] shorter = closes(50);
        final double[] out = new double[200];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.ADD(0, 199, longer, shorter, out),
            "mismatched input lengths -> IllegalArgument",
            "ADD", "inReal1", "50", "200");
        // Same call with the legs the right way round is the control: nothing
        // about ADD's shape makes it throw.
        check(Core.DEFAULT.ADD(0, 49, longer, shorter, out).count() == 50,
              "ADD over the range both legs cover succeeds");
    }

    /** An undersized output names the output and both sizes. */
    static void undersizedOutputIsRejected() {
        final double[] in = closes(200);
        final double[] out = new double[3];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 10, out),
            "undersized output -> IllegalArgument",
            "SMA", "outReal", "3", "191");
    }

    /**
     * A null array is named where it was passed, not where it was first read.
     * The casts are required, not incidental: {@code null} alone is ambiguous
     * between the double[] and float[] overloads.
     */
    static void nullArraysAreNamed() {
        final double[] in = closes(200);
        final double[] out = new double[200];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, (double[]) null, 10, out),
            "null input names it", "SMA", "inReal");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 10, (double[]) null),
            "null output names it", "SMA", "outReal");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, (float[]) null, 10, out),
            "null float input names it", "SMA", "inReal");
        // An argument that does not exist is a bug however little of it would
        // have been read: the null check outlives the lookback short-circuit
        // that switches the LENGTH check off.
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 9, in, 30, (double[]) null),
            "null output on a range that produces nothing is still rejected",
            "SMA", "outReal");
    }

    /**
     * The exact boundary, from both sides. This is what makes every other length
     * assertion in this section mean something: an emitter that required
     * {@code endIdx - startIdx + 1} instead of the produced count would still
     * reject all the calls above, and would fail here.
     */
    static void bothSidesOfTheOutputBound() {
        double[] in = closes(200);
        int lookback = Core.DEFAULT.SMA_Lookback(10);
        int produced = 199 - lookback + 1;

        check(produced == 191, "the produced count really is 191 (got " + produced + ")");
        check(produced < 200, "the produced count is shorter than the requested range");

        OutRange r = Core.DEFAULT.SMA(0, 199, in, 10, new double[produced]);
        check(r.count() == produced, "an exactly-sized output is accepted and filled");

        final double[] oneShort = new double[produced - 1];
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 10, oneShort),
            "one element short of the produced count -> IllegalArgument",
            "outReal", String.valueOf(produced - 1), String.valueOf(produced));
    }

    /**
     * The complaint C2 actually makes: not that the call fails, but that it
     * fails halfway through, having already scribbled on the caller's buffer.
     */
    static void aRejectedCallWritesNothing() {
        final double SENTINEL = -3e37;
        double[] in = closes(200);
        double[] out = new double[3];
        Arrays.fill(out, SENTINEL);

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 10, out), "undersized output throws");

        boolean untouched = true;
        for (double v : out) {
            if (v != SENTINEL) {
                untouched = false;
            }
        }
        check(untouched, "a rejected call left the output buffer untouched");
        // Non-vacuity: the same buffer IS writable by a call that is accepted, so
        // the assertion above is about the rejection and not about the sentinel.
        Core.DEFAULT.SMA(0, 2, in, 1, out);
        check(out[0] != SENTINEL, "the sentinel is overwritten by a call that runs");
    }

    /**
     * The length check does not pre-empt the index and parameter rules. Every
     * case here is BOTH a bad argument and an undersized buffer, and the buffer
     * is the last thing the specification looks at, so it must not be the
     * diagnosis.
     */
    static void theLengthCheckDoesNotPreEmpt() {
        final double[] in = closes(200);
        final double[] tiny = new double[3];

        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(50, 10, in, 10, tiny),
            "endIdx < startIdx still -> IndexOutOfBounds", "endIdx");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(-1, 199, in, 10, tiny),
            "negative startIdx still -> IndexOutOfBounds", "startIdx");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 0, tiny),
            "out-of-range period still -> the parameter message", "bad parameter");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(0, Core.MAX_INDEX + 1, in, 10, tiny),
            "endIdx above MAX_INDEX still -> IndexOutOfBounds", "endIdx");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(Core.MAX_INDEX + 5, Core.MAX_INDEX + 9, in, 10, tiny),
            "startIdx above MAX_INDEX still -> IndexOutOfBounds", "startIdx");
    }

    /**
     * A valid range shorter than the lookback produces nothing, so it reads
     * nothing and any output length is fine — including none. This is the
     * {@code _assertStart > endIdx ||} short-circuit in front of the Rust
     * asserts; without it the length check would throw exactly where
     * {@link #shortRangeIsAnEmptySuccessNotAnException} promises a success.
     * It applies to the OUTPUT bound only — see
     * {@link #anEndIdxPastTheInputIsRejectedEvenProducingNothing}.
     */
    static void aRangeThatProducesNothingChecksNoLength() {
        double[] in = closes(10);

        check(Core.DEFAULT.SMA_Lookback(30) > 9, "the 30-period lookback exceeds this range");
        OutRange r = Core.DEFAULT.SMA(0, 9, in, 30, new double[0]);
        check(r.count() == 0, "a zero-length output is fine when nothing is produced");
    }

    /**
     * The INPUT bound survives the short-range escape that switches the output
     * bound off: an {@code endIdx} past the end of the supplied series is a caller
     * bug in every range, including one that produces no values.
     *
     * <p>C answers this call with {@code TA_SUCCESS} and a zero count — but only
     * because it has no array size to check against, and an empty {@link OutRange}
     * reads as "not enough data yet" rather than "your endIdx is past the end of
     * your own array". This is the one bound where Java checks more than C and
     * Rust; it is a diagnostic, not a safety net.
     *
     * <p>These three functions are here for a reason: APO, PPO and PVO used to run
     * their fast MA — whose lookback is smaller than their own — over the whole
     * requested range before discovering the range was too short, reading the
     * caller's input to build a result the empty slow MA then discarded. They now
     * return before touching anything, and {@code NoPhantomIoTest} pins that for
     * every core. This test pins the wrapper's half of it.
     */
    static void anEndIdxPastTheInputIsRejectedEvenProducingNothing() {
        final double[] in = closes(24);
        final double[] wide = closes(25);
        final double[] vol = new double[24];
        java.util.Arrays.fill(vol, 1000.0);
        final double[] volWide = new double[25];
        java.util.Arrays.fill(volWide, 1000.0);

        check(Core.DEFAULT.APO_Lookback(12, 26, MAType.EMA) > 24,
              "APO's lookback really does exceed this range (so nothing is produced)");

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.APO(0, 24, in, 12, 26, MAType.EMA, new double[0]),
            "APO: endIdx past the input, producing nothing", "APO", "inReal", "24", "25");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.PPO(0, 24, in, 12, 26, MAType.EMA, new double[0]),
            "PPO likewise", "PPO", "inReal", "24", "25");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.PVO(0, 24, vol, 12, 26, MAType.EMA, new double[0]),
            "PVO likewise", "PVO", "inVolume", "24", "25");

        // The controls. An input that DOES reach endIdx is an empty success again,
        // with a zero-length output — so the fix tightened the input bound only, and
        // did not turn the documented empty success into an error.
        check(Core.DEFAULT.APO(0, 24, wide, 12, 26, MAType.EMA, new double[0]).count() == 0,
              "APO with an input reaching endIdx is an empty success, zero-length output");
        check(Core.DEFAULT.PVO(0, 24, volWide, 12, 26, MAType.EMA, new double[0]).count() == 0,
              "PVO likewise");
        check(Core.DEFAULT.SMA(0, 24, wide, 26, new double[0]).count() == 0,
              "and a function that reads nothing is unaffected");
    }

    /** Every output is checked on its own, and named on its own. */
    static void eachOutputIsCheckedSeparately() {
        final double[] in = closes(200);
        int produced = 199 - Core.DEFAULT.MACD_Lookback(12, 26, 9) + 1;
        check(produced > 0, "MACD produces values over this range");

        final double[] big1 = new double[200];
        final double[] big2 = new double[200];
        final double[] small = new double[produced - 1];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.MACD(0, 199, in, 12, 26, 9, small, big1, big2),
            "short first output is named", "outMACD", String.valueOf(produced));
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.MACD(0, 199, in, 12, 26, 9, big1, small, big2),
            "short second output is named", "outMACDSignal", String.valueOf(produced));
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.MACD(0, 199, in, 12, 26, 9, big1, big2, small),
            "short third output is named", "outMACDHist", String.valueOf(produced));
        check(Core.DEFAULT.MACD(0, 199, in, 12, 26, 9, big1, big2,
                                new double[produced]).count() == produced,
              "three exactly-sized outputs are accepted");
    }

    /** The {@code int[]} outputs the candlestick patterns write are checked too. */
    static void integerOutputsAreChecked() {
        final double[] o = closes(200);
        final double[] h = closes(200);
        final double[] l = closes(200);
        final double[] c = closes(200);

        // Both sizes, not just the one allocated: the int[] and float[] overloads of
        // requireLength are two-line delegates, and forwarding a WRONG required count
        // is the only mutation of this guard the double[] assertions cannot see.
        int produced = 199 - Core.DEFAULT.CDLDOJI_Lookback() + 1;
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.CDLDOJI(0, 199, o, h, l, c, new int[3]),
            "short int[] output -> IllegalArgument",
            "CDLDOJI", "outInteger", "3", String.valueOf(produced));
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.CDLDOJI(0, 199, o, h, l, c, (int[]) null),
            "null int[] output -> IllegalArgument", "outInteger");
    }

    /** The float overload carries the identical checks, on its own array type. */
    static void floatOverloadIsCheckedToo() {
        double[] in = closes(200);
        final float[] inF = new float[200];
        for (int i = 0; i < in.length; i++) {
            inF[i] = (float) in[i];
        }
        final double[] tiny = new double[3];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, inF, 10, tiny),
            "float overload: undersized output", "SMA", "outReal", "191");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 500, inF, 10, new double[501]),
            "float overload: endIdx past the input end", "SMA", "inReal", "200", "501");
        check(Core.DEFAULT.SMA(0, 199, inF, 10, new double[191]).count() == 191,
              "float overload accepts an exactly-sized output");
    }

    /**
     * A leg the algorithm never indexes is checked like any other (#260). Four
     * candlestick patterns declare an OHLC input they do not read — CDL3OUTSIDE's
     * high and low among them — and Rust, Java and C# used to exempt exactly
     * those while C's NULL checks covered them, so the identical call was
     * {@code TA_BAD_PARAM} in C and a success here. A declared input must be
     * supplied; that rule now needs no exception list.
     *
     * <p>Both spellings of "not supplied", since the exemption dropped both: an
     * empty array (B5, naming the two lengths) and a null one (B4). The control
     * is the leg next to it, which IS read and was never exempt.
     */
    static void anUnreadLegIsCheckedLikeAnyOther() {
        final double[] real = closes(200);
        final double[] empty = new double[0];
        final int[] out = new int[200];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.CDL3OUTSIDE(0, 199, real, empty, empty, real, out),
            "an empty high leg the body never reads", "inHigh", "0", "200");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.CDL3OUTSIDE(0, 199, real, real, (double[]) null, real, out),
            "a null low leg the body never reads", "inLow", "null");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.CDLHIKKAKE(0, 199, empty, real, real, real, out),
            "CDLHIKKAKE's open leg, the other shape of the same exemption",
            "inOpen", "0", "200");

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.CDL3OUTSIDE(0, 199, empty, real, real, real, out),
            "the open leg, which IS read, is still checked", "inOpen", "0", "200");
        // Non-vacuity: every leg supplied and sized is the success these reject.
        check(Core.DEFAULT.CDL3OUTSIDE(0, 199, real, real, real, real, out).count() > 0,
              "CDL3OUTSIDE runs when every declared leg is supplied");
    }

    /**
     * The metadata path reaches the same guard: {@link io.github.talib.metadata.Dispatch}
     * calls the public wrappers, so an undersized buffer handed to a ParamHolder
     * fails the same way rather than crashing inside the algorithm.
     */
    static void theMetadataPathIsGuardedToo() {
        double[] in = closes(200);
        io.github.talib.metadata.ParamHolder h =
            io.github.talib.metadata.Functions.byName("SMA").newCall(Core.DEFAULT);
        h.setInput(0, in).setOptInput(0, 10).setOutput(0, new double[3]);

        checkThrows(IllegalArgumentException.class,
            () -> h.call(0, 199), "ParamHolder.call is guarded too", "SMA", "outReal");

        // Control: the same holder with a big enough output runs.
        h.setOutput(0, new double[200]);
        check(h.call(0, 199).count() == 191, "the same call with a sized output succeeds");
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
            if (m.getName().equals("SMA")) {
                sma++;
            }
        }
        check(sma >= 2, "reflection sees the SMA overloads it is filtering over");
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

        OutRange rd = Core.DEFAULT.SMA(0, in.length - 1, in, 10, outD);
        OutRange rf = Core.DEFAULT.SMA(0, inF.length - 1, inF, 10, outF);

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

    /**
     * Every failure carries the code C would have returned, and the mapping back
     * is TOTAL and LOSSLESS.
     *
     * <p>Total: every exception the public API raises implements
     * {@link TaLibFailure}, including the two conditions C cannot detect (an
     * absent argument, a buffer too short) and the raw JVM types those used to
     * be. Anything not covered leaves a caller with a thrown object it cannot
     * classify, which is the state this replaced.
     *
     * <p>Lossless: no two codes share one thrown representation. That is the
     * half the exception TYPES cannot carry —
     * {@link IndexOutOfBoundsException} serves both out-of-range index codes and
     * {@link IllegalStateException} serves both library-side ones — so a check
     * on the type alone would pass with the two arms of {@code failure()}
     * swapped.
     */
    static void everyFailureCarriesItsCode() {
        final double[] in = closes(200);
        final double[] out = new double[200];

        // Lossless, the pair the type cannot separate.
        checkCode(RetCode.OutOfRangeStartIndex,
            () -> Core.DEFAULT.SMA(-1, 50, in, 10, out), "negative startIdx carries OutOfRangeStartIndex");
        checkCode(RetCode.OutOfRangeEndIndex,
            () -> Core.DEFAULT.SMA(50, 10, in, 10, out), "endIdx < startIdx carries OutOfRangeEndIndex");

        // The rest of the batch tier's vocabulary.
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.SMA(0, 50, in, 0, out), "an out-of-range period carries BadParam");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.MACD(0, 199, in, 12, 26, 9, out, out, new double[200]),
            "two outputs sharing one array carries BadParam");

        // The two conditions C has no code for. They report the code C answers
        // for an absent argument it CAN detect, so the mapping stays total.
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.SMA(0, 199, (double[]) null, 10, out), "a null input carries BadParam");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.SMA(0, 199, in, 10, new double[3]), "a short output carries BadParam");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.MA(0, 199, in, 10, null, out), "a null enum carries BadParam");

        // Streaming's one recoverable condition, which is why it has a code.
        checkCode(RetCode.InsufficientHistory,
            () -> Core.DEFAULT.smaOpen(Arrays.copyOf(in, Core.DEFAULT.SMA_Lookback(30)), 30),
            "a short history carries InsufficientHistory");

        // ...and the REST of the streaming tier, which is a separate reject
        // ladder from the batch one. Totality is a property of every failure the
        // library raises, not of the tier someone happened to convert first.
        checkCode(RetCode.OutOfRangeStartIndex,
            () -> Core.DEFAULT.smaOpen(new double[0], 30),
            "an empty history carries OutOfRangeStartIndex");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.smaOpen(in, 0),
            "an out-of-range period on a stream open carries BadParam");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.bbandsOpenAndFill(in, 20, 2.0, 2.0, MAType.SMA, out, out, new double[200]),
            "aliased OpenAndFill outputs carry BadParam");

        // ...and it is still an InsufficientHistoryException, so an existing
        // catch keeps working.
        checkThrows(InsufficientHistoryException.class,
            () -> Core.DEFAULT.smaOpen(Arrays.copyOf(in, Core.DEFAULT.SMA_Lookback(30)), 30),
            "a short history is still typed");

        // The numbers the cross-language harness compares. Hardcoded, because
        // asking the enum for its own value would prove nothing.
        check(RetCode.Success.asCInt() == 0, "Success is 0");
        check(RetCode.BadParam.asCInt() == 2, "BadParam is 2");
        check(RetCode.AllocErr.asCInt() == 3, "AllocErr is 3");
        check(RetCode.OutOfRangeStartIndex.asCInt() == 12, "OutOfRangeStartIndex is 12");
        check(RetCode.OutOfRangeEndIndex.asCInt() == 13, "OutOfRangeEndIndex is 13");
        check(RetCode.InsufficientHistory.asCInt() == 17, "InsufficientHistory is 17");
        check(RetCode.InternalError.asCInt() == 5000, "InternalError is 5000");

        // Non-vacuity: the cases above have to REACH every code the batch and
        // streaming tiers can produce, or a member could stop being emitted
        // anywhere and nothing here would move. AllocErr and InternalError are
        // the two exceptions -- one is unreachable in Java (#178) and the other
        // needs a corrupted CIRCBUF size -- so they are named rather than
        // silently excluded.
        Set<RetCode> expected = EnumSet.of(
            RetCode.OutOfRangeStartIndex, RetCode.OutOfRangeEndIndex,
            RetCode.BadParam, RetCode.InsufficientHistory);
        check(seenCodes.equals(expected),
              "the probes reached exactly " + expected + " (got " + seenCodes + ")");
    }

    private static final Set<RetCode> seenCodes = EnumSet.noneOf(RetCode.class);

    /** The call must throw, the throw must carry a code, and it must be this one. */
    private static void checkCode(RetCode expected, Runnable body, String what) {
        checks++;
        try {
            body.run();
            failures++;
            System.out.println("  FAIL: " + what + " (no exception thrown)");
        } catch (RuntimeException e) {
            if (!(e instanceof TaLibFailure)) {
                failures++;
                System.out.println("  FAIL: " + what + " (" + e.getClass().getName()
                    + " carries no RetCode)");
                return;
            }
            RetCode got = ((TaLibFailure) e).retCode();
            if (got != expected) {
                failures++;
                System.out.println("  FAIL: " + what + " (carried " + got + ")");
                return;
            }
            seenCodes.add(got);
        }
    }

    /**
     * The index rules are evaluated BEFORE the presence check: the
     * specification lists B1 and B2 ahead of B4. Every case here is BOTH faults
     * at once; only the order decides which is reported.
     */
    static void anIndexFaultOutranksAnAbsentArgument() {
        final double[] in = closes(200);
        final double[] out = new double[200];

        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(-1, 50, (double[]) null, 10, out),
            "a negative startIdx outranks a null input", "startIdx");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(50, 10, in, 10, (double[]) null),
            "endIdx < startIdx outranks a null output", "endIdx");
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.SMA(0, Core.MAX_INDEX + 1, (double[]) null, 10, out),
            "an endIdx above MAX_INDEX outranks a null input", "endIdx");

        // The control, and what makes the three above about ORDER rather than
        // about the null check having been deleted: with the indices valid, the
        // null IS the diagnosis.
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, (double[]) null, 10, out),
            "a valid range still reports the null", "SMA", "inReal");
    }

    /**
     * A bad optional parameter outranks an absent or undersized buffer: the
     * specification lists B3 ahead of B4 and B5.
     *
     * <p>The parameter rule is the one every backend can express, so putting it
     * first is what makes a multi-fault call report the same condition in all
     * four. The buffer rules are not: a Rust slice and a C# span cannot be
     * absent, and C has no sizes to check.
     */
    static void aBadParameterOutranksAnAbsentBuffer() {
        final double[] in = closes(200);
        final double[] out = new double[200];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, (double[]) null, 0, out),
            "a bad period outranks a null input", "bad parameter");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, in, 0, (double[]) null),
            "a bad period outranks a null output", "bad parameter");

        // The control: with the period valid, the buffer IS the diagnosis. Without
        // it the two above would pass against a wrapper that had simply stopped
        // checking buffers.
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.SMA(0, 199, (double[]) null, 10, out),
            "a valid period still reports the null", "SMA", "inReal");
    }

    /**
     * A null enum parameter is rejected as a parameter outside its domain,
     * naming the function and the parameter. Left to itself it reaches the
     * {@code switch} inside the function's own {@code _Lookback} and surfaces as
     * a bare {@link NullPointerException} naming neither. Java is the only
     * backend where this is expressible at all.
     */
    static void aNullEnumIsNamed() {
        final double[] in = closes(200);
        final double[] out = new double[200];

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.MA(0, 199, in, 10, null, out),
            "a null enum names the function and the parameter", "MA", "optInMAType");
        checkCode(RetCode.BadParam,
            () -> Core.DEFAULT.MA(0, 199, in, 10, null, out),
            "a null enum carries BadParam");
        // ...and neither outranks the index rules.
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.MA(-1, 199, in, 10, null, out),
            "a negative startIdx outranks a null enum", "startIdx");
    }

    /**
     * Rule B6a: an output the .yaml marks {@code nullable} may be declined with
     * {@code null}, and declining it changes nothing about the output that was
     * asked for. MAMA's {@code outFAMA} is the only one in the corpus.
     *
     * <p>Acceptance alone would not test this. A body that stopped computing
     * FAMA, or took a different path without it, would be accepted here just the
     * same — so the declining call has to reproduce the supplied one bit for
     * bit, and leave everything above its own count untouched (rule N2).
     *
     * <p>No cross-language gate can see any of it: the JSON-RPC servers bind
     * every declared output, so a wrapper that went back to requiring
     * {@code outFAMA} stays green in {@code --codegen} and {@code --xlang-hash}
     * alike.
     */
    static void aNullableOutputMayBeDeclined() {
        final double[] in = closes(252);
        final double[] mamaRef = new double[252];
        final double[] famaRef = new double[252];
        OutRange ref = Core.DEFAULT.MAMA(0, 251, in, 0.5, 0.05, mamaRef, famaRef);
        check(ref.count() > 0, "the reference call produces values");

        final double CANARY = -1.2345678901234e300;
        double[] mama = new double[252];
        Arrays.fill(mama, CANARY);
        OutRange r = Core.DEFAULT.MAMA(0, 251, in, 0.5, 0.05, mama, null);

        check(r.begIdx() == ref.begIdx() && r.count() == ref.count(),
            "declining outFAMA leaves the reported range alone");
        boolean same = true;
        for (int i = 0; i < ref.count(); i++) {
            same &= Double.doubleToRawLongBits(mama[i]) == Double.doubleToRawLongBits(mamaRef[i]);
        }
        check(same, "declining outFAMA leaves outMAMA bit-identical");
        boolean untouched = true;
        for (int i = r.count(); i < mama.length; i++) {
            untouched &= Double.doubleToRawLongBits(mama[i]) == Double.doubleToRawLongBits(CANARY);
        }
        check(untouched, "the declining call writes nothing past its own count");

        // The supplied output only has to hold the produced count; the declined
        // one has no size to hold at all.
        Core.DEFAULT.MAMA(0, 251, in, 0.5, 0.05, new double[ref.count()], null);

        // Controls, so the acceptance above is about the FLAG and not about
        // MAMA having stopped checking its outputs.
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.MAMA(0, 251, in, 0.5, 0.05, null, famaRef),
            "the non-nullable output is still required", "MAMA", "outMAMA");
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.MAMA(0, 251, in, 0.5, 0.05, mamaRef, new double[1]),
            "a SUPPLIED nullable output is still length-checked", "MAMA", "outFAMA");
    }

    /**
     * Appendix D item 11: three separately allocated zero-length outputs are
     * three distinct buffers, and a range shorter than the lookback produces
     * nothing, so the call is a success with an empty range (rule N1).
     *
     * <p>Java always accepted it — two arrays are the same object or disjoint,
     * so its guard is reference equality and complete. It is here as the
     * cross-language anchor: C# and Rust rejected the same call until #262, and
     * this is the shape they now have to agree with.
     */
    static void distinctEmptyOutputsAreNotAliases() {
        final double[] in = closes(252);
        final int period = 253;
        check(Core.DEFAULT.ACCBANDS_Lookback(period) > 251,
            "the probe needs a lookback past the range, or it proves nothing");

        OutRange r = Core.DEFAULT.ACCBANDS(0, 251, in, in, in, period,
            new double[0], new double[0], new double[0]);
        check(r.count() == 0, "a sub-lookback range needs no output space");

        // Control: the same three empty arrays on a range that DOES produce
        // values are still rejected, so this is about the count and not about
        // the bound having gone away.
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.ACCBANDS(0, 251, in, in, in, 20,
                new double[0], new double[0], new double[0]),
            "an output that has to hold values is still bounded", "ACCBANDS");
        // And a REAL alias of two outputs is still rejected.
        double[] shared = new double[252];
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.ACCBANDS(0, 251, in, in, in, 20,
                shared, shared, new double[252]),
            "two outputs that are one array are still rejected", "ACCBANDS");
    }

    /**
     * Rule S4 is B4 plus the handle, over the same argument shapes, so the
     * streaming openers are driven from here rather than from a suite of their
     * own — as C's own S4 block rides along inside its batch-argument test.
     *
     * <p>Java expresses fewer of the shapes than C does: the handle is the
     * RETURN value and the range out-parameters live on it, so what is left is
     * the declared inputs and, for {@code openAndFill}, the outputs. Until #268
     * none of them was checked — {@code inReal.length} was read straight off a
     * null array and a null output faulted inside the fill loop, both with a raw
     * JVM exception naming neither the function nor the argument.
     *
     * <p>{@code outFAMA} is here rather than among the controls even though it
     * is declared {@code nullable}: unlike C's, this fill guards no output write,
     * so declining one has never worked in the streaming tier. Naming it is the
     * whole change — the call was already rejected.
     */
    static void streamingOpenersCheckTheirArguments() {
        final double[] in = closes(252);
        final double[] out = new double[252];
        final double[] out2 = new double[252];
        final int[] outI = new int[252];
        final double[] periods = new double[252];
        Arrays.fill(periods, 5.0);

        // B4's shapes, through the openers.
        streamRejects(() -> Core.DEFAULT.smaOpen(null, 30),
            "smaOpen(inReal=null)", "SMA open", "inReal");
        streamRejects(() -> Core.DEFAULT.smaOpenAndFill(null, 30, out),
            "smaOpenAndFill(inReal=null)", "SMA openAndFill", "inReal");
        streamRejects(() -> Core.DEFAULT.smaOpenAndFill(in, 30, null),
            "smaOpenAndFill(outReal=null)", "SMA openAndFill", "outReal");
        // A candlestick leg the body never indexes is still a declared input (#260).
        streamRejects(() -> Core.DEFAULT.cdl3outsideOpen(in, null, in, in),
            "cdl3outsideOpen(inHigh=null)", "CDL3OUTSIDE open", "inHigh");
        streamRejects(() -> Core.DEFAULT.cdl3outsideOpenAndFill(in, in, null, in, outI),
            "cdl3outsideOpenAndFill(inLow=null)", "CDL3OUTSIDE openAndFill", "inLow");
        streamRejects(() -> Core.DEFAULT.cdldojiOpen(in, in, in, null),
            "cdldojiOpen(inClose=null)", "CDLDOJI open", "inClose");
        // Multi-input, multi-output.
        streamRejects(() -> Core.DEFAULT.stochOpen(in, in, null, 5, 3, MAType.SMA, 3, MAType.SMA),
            "stochOpen(inClose=null)", "STOCH open", "inClose");
        streamRejects(() -> Core.DEFAULT.stochOpenAndFill(in, in, in, 5, 3, MAType.SMA, 3,
                MAType.SMA, out, null),
            "stochOpenAndFill(outSlowD=null)", "STOCH openAndFill", "outSlowD");
        // The two hand-rolled tiers: the dispatch and the period bank.
        streamRejects(() -> Core.DEFAULT.maOpen(null, 30, MAType.EMA),
            "maOpen(inReal=null)", "MA open", "inReal");
        streamRejects(() -> Core.DEFAULT.mavpOpen(in, null, 2, 30, MAType.SMA),
            "mavpOpen(inPeriods=null)", "MAVP open", "inPeriods");
        streamRejects(() -> Core.DEFAULT.mavpOpenAndFill(in, periods, 2, 30, MAType.SMA, null),
            "mavpOpenAndFill(outReal=null)", "MAVP openAndFill", "outReal");
        // A nullable output may be DECLINED at the opener, exactly as in the
        // batch tier (rule B6a) and as C has always allowed: `null` is not an
        // absent argument here, it is an answer. Proved below, in
        // `aDeclinedFillOutputIsStillComputed`, that declining changes nothing
        // but the write.

        aDeclinedFillOutputIsStillComputed(in);

        // Rule S3 ahead of the buffer rules, and the one shape that can tell
        // `openFillCount`'s raise from the flooring it replaced: with a rejected
        // parameter AND an absent output, flooring the `-1` lookback to 0 let
        // the output be reported (S4), where the fault is the parameter.
        streamRejects(() -> Core.DEFAULT.smaOpenAndFill(in, 0, null),
            "a bad parameter outranks an absent output", "SMA openAndFill", "bad parameter");
        s5Reject++;

        // Controls: the same calls with every argument supplied still open.
        streamAccepts(() -> Core.DEFAULT.smaOpen(in, 30), "smaOpen");
        streamAccepts(() -> Core.DEFAULT.smaOpenAndFill(in, 30, out), "smaOpenAndFill");
        streamAccepts(() -> Core.DEFAULT.cdl3outsideOpen(in, in, in, in), "cdl3outsideOpen");
        streamAccepts(() -> Core.DEFAULT.stochOpenAndFill(in, in, in, 5, 3, MAType.SMA, 3,
                MAType.SMA, out, out2), "stochOpenAndFill");
        streamAccepts(() -> Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, out, out2),
            "mamaOpenAndFill");
    }

    /**
     * Rule S1, and its order. An opener is a batch call over
     * {@code [0, historyLen - 1]}, so an empty history is B1's condition read on
     * that range — the implied {@code startIdx} of 0 names no bar — and answers
     * B1's code.
     *
     * <p>The order is the part worth a case of its own: the third call below is
     * BOTH an empty history and an absent output, and the empty history is what
     * it has to report. A null HISTORY is the one thing that cannot outrank —
     * a length is not readable from an array that is not there — which is the
     * last case, and the reason this rule is stated as "ahead of every presence
     * check" rather than "first".
     */
    static void anEmptyHistoryOutranksAnAbsentArgument() {
        final double[] empty = new double[0];
        final double[] out = new double[252];

        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.smaOpen(empty, 30),
            "an empty history is an index fault", "SMA open");
        s1Reject++;
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.smaOpenAndFill(empty, 30, out),
            "an empty history is an index fault on the fill too", "SMA openAndFill");
        s1Reject++;
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.smaOpenAndFill(empty, 30, null),
            "an empty history outranks a null output", "SMA openAndFill");
        s1Reject++;
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.cdldojiOpen(empty, empty, empty, empty),
            "a candlestick reaches it through four legs", "CDLDOJI open");
        s1Reject++;
        // The leg that is NOT the history is an ordinary argument, so it is
        // checked after the pair — the same call reports the empty history in C.
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.cdldojiOpen(empty, null, empty, empty),
            "an empty history outranks a null leg", "CDLDOJI open");
        s1Reject++;
        checkThrows(IndexOutOfBoundsException.class,
            () -> Core.DEFAULT.maOpen(empty, 30, MAType.EMA),
            "the dispatch tier answers it too", "MA open");
        s1Reject++;

        // The code, not just the type: one IndexOutOfBoundsException serves both
        // index rules, so the type alone cannot say which fired.
        check(codeOf(() -> Core.DEFAULT.smaOpen(empty, 30)) == RetCode.OutOfRangeStartIndex,
            "an empty history carries OutOfRangeStartIndex");

        // A history of exactly one bar is inside the domain: that is S7's
        // business, and this is what keeps the cases above about EMPTY.
        checkThrows(InsufficientHistoryException.class,
            () -> Core.DEFAULT.smaOpen(new double[1], 30),
            "a one-bar history reaches the warm-up check");

        // The exception: a null history is an absent argument, because its
        // length is what the rule above is about.
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.smaOpen(null, 30),
            "a null history is an absent argument, not an empty one",
            "SMA open", "inReal");
    }

    /** The code an opener's rejection carries, or null if it did not reject. */
    private static RetCode codeOf(Runnable body) {
        try {
            body.run();
            return null;
        } catch (RuntimeException e) {
            return (e instanceof TaLibFailure) ? ((TaLibFailure) e).retCode() : null;
        }
    }

    private static void streamRejects(Runnable body, String what, String... needles) {
        s4Reject++;
        checkThrows(IllegalArgumentException.class, body, what, needles);
    }

    /**
     * Rule B6a at the opener: {@code outFAMA} may be declined with {@code null},
     * and declining changes nothing but the write.
     *
     * <p>Non-vacuous in three directions. The supplied run is the oracle, so a
     * fill that stopped computing FAMA when it is declined — the easy way to
     * "support" this — fails on {@code value()}, which the handle caches from
     * the same expression the guarded store writes. The declined run must still
     * reject an undersized {@code outMAMA}, so the conditional bound cannot have
     * been dropped wholesale. And the supplied-but-undersized {@code outFAMA} is
     * still rejected, so "declinable" did not become "unchecked".
     */
    private static void aDeclinedFillOutputIsStillComputed(double[] in) {
        int lb = Core.DEFAULT.MAMA_Lookback(0.5, 0.05);
        int produced = in.length - lb;

        double[] refMama = new double[produced];
        double[] refFama = new double[produced];
        Core.MamaStream both =
            Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, refMama, refFama);

        double[] soloMama = new double[produced];
        Core.MamaStream declined =
            Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, soloMama, null);

        b6aOpen++;
        check(java.util.Arrays.equals(refMama, soloMama),
            "declining outFAMA leaves outMAMA bit-identical");
        check(both.outRange().begIdx() == declined.outRange().begIdx()
                && both.outRange().count() == declined.outRange().count(),
            "declining outFAMA leaves the reported range unchanged");
        b6aOpen++;
        check(Double.doubleToRawLongBits(both.value().fama())
                == Double.doubleToRawLongBits(declined.value().fama()),
            "a declined outFAMA is still computed: the handle reports it");
        b6aOpen++;
        check(Double.doubleToRawLongBits(refFama[produced - 1])
                == Double.doubleToRawLongBits(declined.value().fama()),
            "and it is the value the supplied run wrote last");
        b6aOpen++;

        // Declining one output does not disarm the other's bound, nor its own
        // when it IS supplied.
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced - 1], null),
            "an undersized outMAMA is still rejected when outFAMA is declined",
            "outMAMA");
        b6aOpen++;
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced],
                    new double[produced - 1]),
            "a supplied outFAMA is still bounded", "outFAMA");
        b6aOpen++;
    }

    /**
     * Rule U6a: {@code updateAndFill} declines a nullable output exactly as the
     * opener does, and the choice is the CALL's — the four open/fill
     * combinations are all accepted and all compute the same numbers.
     *
     * <p>Non-vacuous in the same directions as the opener's probe, plus the one
     * this rule adds. The supplied run is the oracle, and the comparison a
     * backend that stopped computing FAMA cannot satisfy is the handle's own
     * {@code value()} after the fill, not the arrays. The mixed combinations are
     * the point of the rule: declining at {@code openAndFill} and supplying here
     * — and the reverse — must be as ordinary as either matching pair. A
     * declining call must still bound {@code outMAMA}, and still bound
     * {@code outFAMA} where it IS supplied.
     */
    private static final double U6A_CANARY = -1.2345678901234e300;

    private static double[] canaryFilled(int n) {
        double[] a = new double[n];
        java.util.Arrays.fill(a, U6A_CANARY);
        return a;
    }

    private static boolean wasWritten(double[] a) {
        for (double v : a) {
            if (v == U6A_CANARY) {
                return false;
            }
        }
        return true;
    }

    private static void aDeclinedOutputAtUpdateAndFillIsAPropertyOfTheCall() {
        final double[] in = closes(252);
        final int produced = in.length - Core.DEFAULT.MAMA_Lookback(0.5, 0.05);
        final double[] bars = new double[8];
        for (int i = 0; i < bars.length; i++) {
            bars[i] = in[in.length - 1] + 1.0 + i * 0.25;
        }

        // The oracle: supplied at open, supplied here.
        Core.MamaStream oracle =
            Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced], new double[produced]);
        // Canary-filled, not zero-filled: comparing two arrays the fill never
        // wrote would otherwise pass on their shared initial value, which is
        // exactly the break the supplied/supplied leg below is meant to catch.
        double[] refMama = canaryFilled(bars.length);
        double[] refFama = canaryFilled(bars.length);
        oracle.updateAndFill(bars, refMama, refFama);
        u6aFill++;
        check(wasWritten(refMama) && wasWritten(refFama), "the oracle fill wrote both outputs");
        long oracleFama = Double.doubleToRawLongBits(oracle.value().fama());
        long oracleMama = Double.doubleToRawLongBits(oracle.value().mama());

        for (boolean declinedAtOpen : new boolean[] { false, true }) {
            String what = declinedAtOpen ? "declined at open" : "supplied at open";

            Core.MamaStream h = declinedAtOpen
                ? Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced], null)
                : Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced],
                        new double[produced]);
            double[] mama = canaryFilled(bars.length);
            h.updateAndFill(bars, mama, null);
            u6aFill++;
            check(wasWritten(mama) && java.util.Arrays.equals(refMama, mama),
                what + ", declined here: outMAMA");
            u6aFill++;
            check(h.outRange().begIdx() == oracle.outRange().begIdx()
                    && h.outRange().count() == oracle.outRange().count(),
                what + ", declined here: the range");
            // The state, not the write: FAMA feeds the next bar.
            u6aFill++;
            check(Double.doubleToRawLongBits(h.value().fama()) == oracleFama,
                what + ", declined here: a declined outFAMA is still computed");
            u6aFill++;
            check(Double.doubleToRawLongBits(h.value().mama()) == oracleMama,
                what + ", declined here: the handle's outMAMA");

            Core.MamaStream h2 = declinedAtOpen
                ? Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced], null)
                : Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced],
                        new double[produced]);
            double[] mama2 = canaryFilled(bars.length);
            double[] fama2 = canaryFilled(bars.length);
            h2.updateAndFill(bars, mama2, fama2);
            u6aFill++;
            check(wasWritten(mama2) && wasWritten(fama2)
                    && java.util.Arrays.equals(refMama, mama2)
                    && java.util.Arrays.equals(refFama, fama2),
                what + ", supplied here: both outputs");
        }

        // "May differ again on the NEXT call" — the sentence the whole rule rests
        // on. One handle, three fills, alternating; each has to agree with an
        // oracle driven the same way with everything supplied.
        Core.MamaStream alt =
            Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced], new double[produced]);
        Core.MamaStream altRef =
            Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced], new double[produced]);
        boolean[] plan = { true, false, true };
        for (int k = 0; k < plan.length; k++) {
            double[] leg = new double[bars.length];
            for (int i = 0; i < bars.length; i++) {
                leg[i] = bars[i] + k;
            }
            double[] wantM = canaryFilled(leg.length);
            double[] wantF = canaryFilled(leg.length);
            altRef.updateAndFill(leg, wantM, wantF);
            double[] gotM = canaryFilled(leg.length);
            double[] gotF = canaryFilled(leg.length);
            if (plan[k]) {
                alt.updateAndFill(leg, gotM, null);
            } else {
                alt.updateAndFill(leg, gotM, gotF);
                u6aFill++;
                check(wasWritten(gotF) && java.util.Arrays.equals(wantF, gotF),
                    "alternating leg " + k + ": outFAMA");
            }
            u6aFill++;
            check(wasWritten(gotM) && java.util.Arrays.equals(wantM, gotM)
                    && alt.outRange().count() == altRef.outRange().count(),
                "alternating leg " + k + ": outMAMA and the range");
        }
        u6aFill++;
        check(Double.doubleToRawLongBits(alt.value().fama())
                == Double.doubleToRawLongBits(altRef.value().fama()),
            "alternating the declined set left the handle's FAMA identical");

        // A DECLINED output is not an absent one: the required arrays are still
        // rule U2, and the fault has to be the documented exception naming the
        // argument, not the raw NullPointerException reading a length off a null
        // array used to produce.
        Core.MamaStream named =
            Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced], new double[produced]);
        checkThrows(TaLibArgumentException.class,
            () -> named.updateAndFill(bars, null, new double[bars.length]),
            "an absent required output names itself", "MAMA updateAndFill", "outMAMA");
        u6aFill++;
        checkThrows(TaLibArgumentException.class,
            () -> named.updateAndFill(null, new double[bars.length], null),
            "an absent input series names itself", "MAMA updateAndFill", "inReal");
        u6aFill++;

        // Declining one output disarms neither the other's bound nor its own
        // where it IS supplied, and a rejected fill commits nothing.
        Core.MamaStream guarded =
            Core.DEFAULT.mamaOpenAndFill(in, 0.5, 0.05, new double[produced], new double[produced]);
        int before = guarded.outRange().count();
        checkThrows(IllegalArgumentException.class,
            () -> guarded.updateAndFill(bars, new double[bars.length - 1], null),
            "an undersized outMAMA is still rejected when outFAMA is declined");
        u6aFill++;
        checkThrows(IllegalArgumentException.class,
            () -> guarded.updateAndFill(bars, new double[bars.length],
                    new double[bars.length - 1]),
            "a supplied outFAMA is still bounded");
        u6aFill++;
        check(guarded.outRange().count() == before, "a rejected fill commits nothing");
        u6aFill++;
    }

    private static void streamAccepts(Runnable body, String what) {
        s4Accept++;
        checks++;
        try {
            body.run();
        } catch (RuntimeException e) {
            failures++;
            System.out.println("  FAIL: " + what + " was rejected (" + e + ")");
        }
    }


    /**
     * Rule S5, from both sides. The bound is {@code historyLen - lookback} — the
     * count the fill actually writes, not the width of the history — so an
     * exactly-sized output has to be ACCEPTED and one element shorter REJECTED.
     * Only the pair pins the arithmetic: a bound of {@code historyLen} would
     * reject the first, and no bound at all would accept the second.
     *
     * <p>Until #268's follow-up an undersized output faulted inside the fill
     * with an {@link ArrayIndexOutOfBoundsException}, the buffer already partly
     * written and no {@link OutRange} to say how far it got.
     */
    static void theFillOutputBoundFromBothSides() {
        final double[] in = closes(252);
        final int lookback = Core.DEFAULT.SMA_Lookback(30);
        final int produced = in.length - lookback;

        check(lookback == 29, "the probe needs a lookback it can be one short of");
        check(produced < in.length, "the produced count is shorter than the history");

        double[] exact = new double[produced];
        Core.SmaStream h = Core.DEFAULT.smaOpenAndFill(in, 30, exact);
        check(h.outRange().begIdx() == lookback, "the fill starts at the lookback");
        check(h.outRange().count() == produced, "the fill wrote exactly the bound");

        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.smaOpenAndFill(in, 30, new double[produced - 1]),
            "one element short of the produced count -> IllegalArgument",
            "SMA openAndFill", "outReal", String.valueOf(produced - 1), String.valueOf(produced));
        s5Reject++;

        // A rejected fill writes nothing — the check is ahead of the numerics,
        // not a report from inside them.
        final double[] shortOut = new double[produced - 1];
        Arrays.fill(shortOut, -3e37);
        try {
            Core.DEFAULT.smaOpenAndFill(in, 30, shortOut);
            check(false, "expected the undersized fill to be rejected");
        } catch (IllegalArgumentException expected) {
            boolean untouched = true;
            for (double v : shortOut) {
                if (v != -3e37) { untouched = false; }
            }
            check(untouched, "a rejected fill leaves the output as it found it");
        }

        // An oversized output is legal and bit-identical: the bound is a
        // minimum, which is what says the exact case above was not luck.
        double[] roomy = new double[in.length];
        Core.DEFAULT.smaOpenAndFill(in, 30, roomy);
        boolean same = true;
        for (int i = 0; i < produced; i++) {
            if (Double.doubleToRawLongBits(exact[i]) != Double.doubleToRawLongBits(roomy[i])) {
                same = false;
            }
        }
        check(same, "the exactly-sized fill is bit-identical to the roomy one");
    }

    /**
     * The same bound on the tiers that hand-roll their own fill — the dispatch
     * tier (including its identity arm, whose lookback is 0), the period bank,
     * and a composed multi-output, whose sub-calls fill scratch of
     * their own rather than the caller's arrays. Each output is bounded separately.
     */
    static void theFillOutputBoundHoldsOnEveryTier() {
        final double[] in = closes(252);
        final double[] periods = new double[252];
        Arrays.fill(periods, 5.0);

        for (int[] arm : new int[][] { {30, 0}, {1, 0} }) {
            final int period = arm[0];
            final int lb = Core.DEFAULT.MA_Lookback(period, MAType.EMA);
            final int produced = in.length - lb;
            Core.DEFAULT.maOpenAndFill(in, period, MAType.EMA, new double[produced]);
            checkThrows(IllegalArgumentException.class,
                () -> Core.DEFAULT.maOpenAndFill(in, period, MAType.EMA, new double[produced - 1]),
                "MA one short of the bound", "MA openAndFill", "outReal");
            s5Reject++;
        }

        final int mavpLb = Core.DEFAULT.MAVP_Lookback(2, 30, MAType.SMA);
        final int mavpProduced = in.length - mavpLb;
        Core.DEFAULT.mavpOpenAndFill(in, periods, 2, 30, MAType.SMA, new double[mavpProduced]);
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.mavpOpenAndFill(in, periods, 2, 30, MAType.SMA,
                new double[mavpProduced - 1]),
            "MAVP one short of the bound", "MAVP openAndFill", "outReal");
        s5Reject++;

        final int bbLb = Core.DEFAULT.BBANDS_Lookback(20, 2.0, 2.0, MAType.SMA);
        final int bbProduced = in.length - bbLb;
        Core.DEFAULT.bbandsOpenAndFill(in, 20, 2.0, 2.0, MAType.SMA,
            new double[bbProduced], new double[bbProduced], new double[bbProduced]);
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.bbandsOpenAndFill(in, 20, 2.0, 2.0, MAType.SMA,
                new double[bbProduced], new double[bbProduced], new double[bbProduced - 1]),
            "each output is bounded separately", "BBANDS openAndFill", "outRealLowerBand");
        s5Reject++;

        // A history too short to produce anything is still S7, whatever the
        // output holds: the bound floors at zero rather than going negative.
        checkThrows(InsufficientHistoryException.class,
            () -> Core.DEFAULT.smaOpenAndFill(Arrays.copyOf(in, 29), 30, new double[0]),
            "a short history reaches the warm-up check, not the capacity one");

        // A null enum is a parameter outside its domain, named — it reaches the
        // lookback call the bound is derived from.
        // Rule S3, not S5 — a null enum with a full-length output. It belongs
        // here because the enum check has to precede the lookback call the S5
        // bound is derived from, but it is not one of S5's cases and is not
        // counted as one.
        checkThrows(IllegalArgumentException.class,
            () -> Core.DEFAULT.maOpenAndFill(in, 30, null, new double[in.length]),
            "a null enum at the opener is named", "MA openAndFill", "optInMAType");
    }

    public static void main(String[] args) {
        maxWithKnownOutputs();
        begIdxEqualsLookback();
        cmoLeavesTheTailUntouched();
        shortRangeIsAnEmptySuccessNotAnException();
        misuseThrowsTheDocumentedException();
        undersizedInputIsRejected();
        mismatchedInputLengthsAreRejected();
        undersizedOutputIsRejected();
        nullArraysAreNamed();
        bothSidesOfTheOutputBound();
        aRejectedCallWritesNothing();
        theLengthCheckDoesNotPreEmpt();
        aRangeThatProducesNothingChecksNoLength();
        anEndIdxPastTheInputIsRejectedEvenProducingNothing();
        eachOutputIsCheckedSeparately();
        integerOutputsAreChecked();
        floatOverloadIsCheckedToo();
        anUnreadLegIsCheckedLikeAnyOther();
        theMetadataPathIsGuardedToo();
        noUnguardedTierOnThePublicSurface();
        floatOverloadHasTheSameShape();
        outRangeValueSemantics();
        everyFailureCarriesItsCode();
        anIndexFaultOutranksAnAbsentArgument();
        aBadParameterOutranksAnAbsentBuffer();
        aNullEnumIsNamed();
        aNullableOutputMayBeDeclined();
        distinctEmptyOutputsAreNotAliases();
        streamingOpenersCheckTheirArguments();
        anEmptyHistoryOutranksAnAbsentArgument();
        theFillOutputBoundFromBothSides();
        theFillOutputBoundHoldsOnEveryTier();
        aDeclinedOutputAtUpdateAndFillIsAPropertyOfTheCall();

        // Literal floors, not derived from the calls above: a count computed
        // from the cases would move with a deleted one and still "pass".
        // s4Reject is 11, not 12: the twelfth was `mamaOpenAndFill(outFAMA=null)`,
        // which is no longer an absent argument but a declined output — rule B6a,
        // and it has its own counter and its own probe.
        if (s4Reject < 11 || s4Accept < 5 || s1Reject < 6 || s5Reject < 5 || b6aOpen < 6
                || u6aFill < 21) {
            failures++;
            System.out.println("  FAIL: the streaming-opener gate ran fewer checks"
                + " than it was written with");
        }

        if (failures == 0) {
            System.out.println("BatchApiTest: ALL PASS (" + checks + " checks)");
        } else {
            System.out.println("BatchApiTest: " + failures + " of " + checks + " checks FAILED");
            System.exit(1);
        }
    }
}
