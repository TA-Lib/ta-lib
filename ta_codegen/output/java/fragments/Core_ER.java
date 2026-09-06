/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090626 KL     First version (issue #350).
 */

   /**
    * Number of leading input bars {@link Core#ER} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of one-bar changes in the path sum
    *        ({@code KAMA}'s {@code optInTimePeriod} is the same window, under its own
    *        default) (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ER_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* P one-bar changes need P+1 prices: first output at index P. */
      return optInTimePeriod ;

   }
   RetCode ER_Impl( int startIdx,
                    int endIdx,
                    double inReal[],
                    int optInTimePeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int nullRun = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double trailingValue = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Kaufman Efficiency Ratio (Perry J. Kaufman, Smarter Trading, 1995):
       * net directional movement over the period divided by the total path
       * travelled,
       *
       *   ER[t] = |c[t] - c[t-P]| / SUM(k = t-P+1 .. t) |c[k] - c[k-1]|
       *
       * The ratio is in [0,1] in exact arithmetic, but sumROC1 drifts, so the
       * clamp to 1.0 is what makes the declared range a bound rather than a
       * hope -- and in kama.c what keeps its recurrence a convex combination.
       *
       * This is a lift of TA_KAMA's inner efficiency ratio (kama.c) so the
       * two stay bit-identical -- the KAMA-reconstruction differential in
       * test_composite2.c exists to keep it that way. Three more guards are
       * load-bearing and shared with kama.c:
       *
       *   - `sumROC1 <= periodROC` pins the ratio to exactly 1.0 where FP
       *     would give 1.0000000000000002, on up-moves. It compares against
       *     the SIGNED numerator, so it is false for every down move: it
       *     bounds nothing on its own, which is what the clamp is for.
       *   - a genuinely flat window is recognized by COUNTING exactly-zero
       *     one-bar changes (nullRun >= P forces sumROC1 to 0.0, purging the
       *     running sum's rounding residue), after which `0 <= 0` pins the
       *     0/0 to 1.0 -- never NaN (#112). This is kama.c's #253 form; the
       *     absolute TA_IS_ZERO band it replaced fails the QUOTE-UNIT/SCALE
       *     gate (ER is homogeneous of degree 0, and a fixed 1e-14 met a
       *     price-carrying sum).
       *
       * The third is the denominator test (#385): a subtract-then-add sum can
       * reach 0.0, or below it, on a window that is not flat. The clamp
       * subsumes it numerically, so it is held by the structural sweep over
       * divisors, not by any value test.
       *
       * The subtract-then-add update order matches TA_SUM's recurrence,
       * which is what makes the composite differential bit-exact. The
       * trailing value is cached one iteration ahead, which is what keeps
       * outReal == inReal aliasing safe.
       */
      lookbackTotal = ER_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Prime the path sum over the optInTimePeriod one-bar changes ending
       * at the first output bar's predecessor.
       */
      sumROC1 = 0.0;
      nullRun = 0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         tempReal -= inReal[today];
         sumROC1 += Math.abs(tempReal);
         if( tempReal == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
      }
      /* First output: today == startIdx. */
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      trailingValue = tempReal2;
      /* A fully flat priming window sums to an exact 0.0 (no residue yet), so
       * `0 <= 0` already answers 1.0 here without the nullRun purge. The
       * denominator test below is unreachable at this site -- a priming sum only
       * ever has non-negative terms added to it -- and is written anyway so both
       * sites read as one rule.
       */
      if( sumROC1 <= 0.0 || sumROC1 <= periodROC ) {
         outReal[0] = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
         if( tempReal > 1.0 ) {
            tempReal = 1.0;
         }
         outReal[0] = tempReal;
      }
      outIdx = 1;
      today += 1;
      while( today <= endIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Subtract-then-add, TA_SUM's own order. */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Once a whole window of one-bar changes is exactly zero, the sum's
          * only content is rounding residue from the subtract/add carry --
          * purge it so the flat window is recognized exactly (kama.c #253).
          */
         if( tempReal - inReal[today - 1] == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            sumROC1 = 0.0;
         }
         /* Save the trailing value: outReal may alias inReal, and the next
          * iteration's subtraction needs the ORIGINAL bar, not the slot the
          * write below may have clobbered.
          */
         trailingValue = tempReal2;
         if( sumROC1 <= 0.0 || sumROC1 <= periodROC ) {
            outReal[outIdx++] = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
            if( tempReal > 1.0 ) {
               tempReal = 1.0;
            }
            outReal[outIdx++] = tempReal;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode ER_Impl( int startIdx,
                    int endIdx,
                    float inReal[],
                    int optInTimePeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int nullRun = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double trailingValue = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = ER_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumROC1 = 0.0;
      nullRun = 0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = (double)inReal[today++];
         tempReal -= (double)inReal[today];
         sumROC1 += Math.abs(tempReal);
         if( tempReal == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
      }
      tempReal = (double)inReal[today];
      tempReal2 = (double)inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      trailingValue = tempReal2;
      if( sumROC1 <= 0.0 || sumROC1 <= periodROC ) {
         outReal[0] = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
         if( tempReal > 1.0 ) {
            tempReal = 1.0;
         }
         outReal[0] = tempReal;
      }
      outIdx = 1;
      today += 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today];
         tempReal2 = (double)inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - (double)inReal[today - 1]);
         if( tempReal - (double)inReal[today - 1] == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            sumROC1 = 0.0;
         }
         trailingValue = tempReal2;
         if( sumROC1 <= 0.0 || sumROC1 <= periodROC ) {
            outReal[outIdx++] = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
            if( tempReal > 1.0 ) {
               tempReal = 1.0;
            }
            outReal[outIdx++] = tempReal;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Kaufman Efficiency Ratio (also searched as "KER"): Perry Kaufman's noise
    * measure from *Smarter Trading* (1995) — the net directional movement over
    * the period divided by the total path travelled to get there. 1.0 is a
    * perfectly efficient (straight-line) move; values near 0 are churn. This is
    * exactly the efficiency ratio [{@code KAMA}](/functions/kama) computes
    * internally to set its adaptive smoothing constant, exposed standalone and
    * kept bit-identical to it.
    * <p><b>Formula</b>
    * <pre>{@code
    * `ER[t] = |close[t] − close[t−P]| / Σ |close[k] − close[k−1]|` over the same `P` bars.
    * Two guards, both shared with `KAMA`: a ratio that floating point would nudge just above 1.0 on a straight-line advance is pinned to exactly 1.0, and a dead-flat window (0/0) also reports 1.0 — a flat market therefore reads as "perfectly efficient", which is `KAMA`'s own convention and what keeps the two reconstructible from each other.
    * The output is a hard 0..1 — the net move can never exceed the path travelled.
    * TC2000 documents a signed ×100 variant (−100..+100); the absolute 0..1 form here is the author's, StockCharts', LEAN's, backtrader's and pandas-ta's.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>First output at index {@code P} ({@code P} one-bar changes need {@code P+1} prices). No unstable period, not start-dependent.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ER_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series (canonically close)
    * @param optInTimePeriod Number of one-bar changes in the path sum
    *        ({@code KAMA}'s {@code optInTimePeriod} is the same window, under its own
    *        default) (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param outReal Efficiency ratio. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#KAMA
    * @see Core#MAMA
    * @see Core#STDDEV
    * @see Core#VHF
    */
   public OutRange ER( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       double outReal[] )
   {
      requireIndexRange("ER", startIdx, endIdx);
      int guardStart = clampedStart("ER", startIdx, ER_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ER", "inReal", inReal, guardInLen);
      requireLength("ER", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ER_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ER", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Kaufman Efficiency Ratio (also searched as "KER"): Perry Kaufman's noise
    * measure from *Smarter Trading* (1995) — the net directional movement over
    * the period divided by the total path travelled to get there. 1.0 is a
    * perfectly efficient (straight-line) move; values near 0 are churn. This is
    * exactly the efficiency ratio [{@code KAMA}](/functions/kama) computes
    * internally to set its adaptive smoothing constant, exposed standalone and
    * kept bit-identical to it.
    * <p><b>Formula</b>
    * <pre>{@code
    * `ER[t] = |close[t] − close[t−P]| / Σ |close[k] − close[k−1]|` over the same `P` bars.
    * Two guards, both shared with `KAMA`: a ratio that floating point would nudge just above 1.0 on a straight-line advance is pinned to exactly 1.0, and a dead-flat window (0/0) also reports 1.0 — a flat market therefore reads as "perfectly efficient", which is `KAMA`'s own convention and what keeps the two reconstructible from each other.
    * The output is a hard 0..1 — the net move can never exceed the path travelled.
    * TC2000 documents a signed ×100 variant (−100..+100); the absolute 0..1 form here is the author's, StockCharts', LEAN's, backtrader's and pandas-ta's.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>First output at index {@code P} ({@code P} one-bar changes need {@code P+1} prices). No unstable period, not start-dependent.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ER_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series (canonically close)
    * @param optInTimePeriod Number of one-bar changes in the path sum
    *        ({@code KAMA}'s {@code optInTimePeriod} is the same window, under its own
    *        default) (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param outReal Efficiency ratio. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#KAMA
    * @see Core#MAMA
    * @see Core#STDDEV
    * @see Core#VHF
    */
   public OutRange ER( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       double outReal[] )
   {
      requireIndexRange("ER", startIdx, endIdx);
      int guardStart = clampedStart("ER", startIdx, ER_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ER", "inReal", inReal, guardInLen);
      requireLength("ER", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ER_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ER", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ER stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ER} over the same series.
    * Open with {@link Core#erOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class ErStream {
      Core core;
      int optInTimePeriod;
      int nullRun;
      double sumROC1;
      double trailingValue;
      double lag1_inReal;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      ErStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ER} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       */
      public void advance() { if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++; }

      ErStream( ErStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.nullRun = other.nullRun;
         this.sumROC1 = other.sumROC1;
         this.trailingValue = other.trailingValue;
         this.lag1_inReal = other.lag1_inReal;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("ER update: BadParam", RetCode.BadParam);
         core.erStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("ER peek: BadParam", RetCode.BadParam);
         ErStream sp = this;
         double periodROC = 0.0;
         double tempReal = 0.0;
         double tempReal2 = 0.0;
         double cur_outReal = 0.0;
         int nullRun = sp.nullRun;
         double sumROC1 = sp.sumROC1;
         double trailingValue = sp.trailingValue;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         tempReal = inReal;
         tempReal2 = (sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal0;
         periodROC = tempReal - tempReal2;
         /* Subtract-then-add, TA_SUM's own order. */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
         /* Once a whole window of one-bar changes is exactly zero, the sum's
          * only content is rounding residue from the subtract/add carry --
          * purge it so the flat window is recognized exactly (kama.c #253).
          */
         if( tempReal - sp.lag1_inReal == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= sp.optInTimePeriod ) {
            nullRun = sp.optInTimePeriod;
            sumROC1 = 0.0;
         }
         /* Save the trailing value: outReal may alias inReal, and the next
          * iteration's subtraction needs the ORIGINAL bar, not the slot the
          * write below may have clobbered.
          */
         trailingValue = tempReal2;
         if( sumROC1 <= 0.0 || sumROC1 <= periodROC ) {
            cur_outReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
            if( tempReal > 1.0 ) {
               tempReal = 1.0;
            }
            cur_outReal = tempReal;
         }
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public ErStream clone() {
         return new ErStream(this);
      }
   }
   void erStepImpl( ErStream sp, double inReal )
   {
      double periodROC = 0.0;
      double tempReal = 0.0;
      double tempReal2 = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      tempReal = inReal;
      tempReal2 = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      periodROC = tempReal - tempReal2;
      /* Subtract-then-add, TA_SUM's own order. */
      sp.sumROC1 -= Math.abs(sp.trailingValue - tempReal2);
      sp.sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
      /* Once a whole window of one-bar changes is exactly zero, the sum's
       * only content is rounding residue from the subtract/add carry --
       * purge it so the flat window is recognized exactly (kama.c #253).
       */
      if( tempReal - sp.lag1_inReal == 0.0 ) {
         sp.nullRun += 1;
      } else {
         sp.nullRun = 0;
      }
      if( sp.nullRun >= sp.optInTimePeriod ) {
         sp.nullRun = sp.optInTimePeriod;
         sp.sumROC1 = 0.0;
      }
      /* Save the trailing value: outReal may alias inReal, and the next
       * iteration's subtraction needs the ORIGINAL bar, not the slot the
       * write below may have clobbered.
       */
      sp.trailingValue = tempReal2;
      if( sp.sumROC1 <= 0.0 || sp.sumROC1 <= periodROC ) {
         sp.cur_outReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sp.sumROC1);
         if( tempReal > 1.0 ) {
            tempReal = 1.0;
         }
         sp.cur_outReal = tempReal;
      }
      sp.lag1_inReal = inReal;
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode erOpenImpl( ErStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int nullRun = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double trailingValue = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Kaufman Efficiency Ratio (Perry J. Kaufman, Smarter Trading, 1995):
       * net directional movement over the period divided by the total path
       * travelled,
       *
       *   ER[t] = |c[t] - c[t-P]| / SUM(k = t-P+1 .. t) |c[k] - c[k-1]|
       *
       * The ratio is in [0,1] in exact arithmetic, but sumROC1 drifts, so the
       * clamp to 1.0 is what makes the declared range a bound rather than a
       * hope -- and in kama.c what keeps its recurrence a convex combination.
       *
       * This is a lift of TA_KAMA's inner efficiency ratio (kama.c) so the
       * two stay bit-identical -- the KAMA-reconstruction differential in
       * test_composite2.c exists to keep it that way. Three more guards are
       * load-bearing and shared with kama.c:
       *
       *   - `sumROC1 <= periodROC` pins the ratio to exactly 1.0 where FP
       *     would give 1.0000000000000002, on up-moves. It compares against
       *     the SIGNED numerator, so it is false for every down move: it
       *     bounds nothing on its own, which is what the clamp is for.
       *   - a genuinely flat window is recognized by COUNTING exactly-zero
       *     one-bar changes (nullRun >= P forces sumROC1 to 0.0, purging the
       *     running sum's rounding residue), after which `0 <= 0` pins the
       *     0/0 to 1.0 -- never NaN (#112). This is kama.c's #253 form; the
       *     absolute TA_IS_ZERO band it replaced fails the QUOTE-UNIT/SCALE
       *     gate (ER is homogeneous of degree 0, and a fixed 1e-14 met a
       *     price-carrying sum).
       *
       * The third is the denominator test (#385): a subtract-then-add sum can
       * reach 0.0, or below it, on a window that is not flat. The clamp
       * subsumes it numerically, so it is held by the structural sweep over
       * divisors, not by any value test.
       *
       * The subtract-then-add update order matches TA_SUM's recurrence,
       * which is what makes the composite differential bit-exact. The
       * trailing value is cached one iteration ahead, which is what keeps
       * outReal == inReal aliasing safe.
       */
      lookbackTotal = ER_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Prime the path sum over the optInTimePeriod one-bar changes ending
       * at the first output bar's predecessor.
       */
      sumROC1 = 0.0;
      nullRun = 0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         tempReal -= inReal[today];
         sumROC1 += Math.abs(tempReal);
         if( tempReal == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
      }
      /* First output: today == startIdx. */
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      trailingValue = tempReal2;
      /* A fully flat priming window sums to an exact 0.0 (no residue yet), so
       * `0 <= 0` already answers 1.0 here without the nullRun purge. The
       * denominator test below is unreachable at this site -- a priming sum only
       * ever has non-negative terms added to it -- and is written anyway so both
       * sites read as one rule.
       */
      if( sumROC1 <= 0.0 || sumROC1 <= periodROC ) {
         outReal[0 * outStride] = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
         if( tempReal > 1.0 ) {
            tempReal = 1.0;
         }
         outReal[0 * outStride] = tempReal;
      }
      outIdx = 1;
      today += 1;
      while( today <= endIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Subtract-then-add, TA_SUM's own order. */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Once a whole window of one-bar changes is exactly zero, the sum's
          * only content is rounding residue from the subtract/add carry --
          * purge it so the flat window is recognized exactly (kama.c #253).
          */
         if( tempReal - inReal[today - 1] == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            sumROC1 = 0.0;
         }
         /* Save the trailing value: outReal may alias inReal, and the next
          * iteration's subtraction needs the ORIGINAL bar, not the slot the
          * write below may have clobbered.
          */
         trailingValue = tempReal2;
         if( sumROC1 <= 0.0 || sumROC1 <= periodROC ) {
            outReal[outIdx++ * outStride] = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
            if( tempReal > 1.0 ) {
               tempReal = 1.0;
            }
            outReal[outIdx++ * outStride] = tempReal;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.nullRun = nullRun;
      sp.sumROC1 = sumROC1;
      sp.trailingValue = trailingValue;
      sp.lag1_inReal = inReal[historyLen - 1];
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* erOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   ErStream erOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      ErStream sp = new ErStream(this);
      RetCode retCode = erOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ER openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ER openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ER openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind erOpen (composition seam). */
   ErStream erOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      ErStream sp = new ErStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = erOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ER open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ER open: internal error", retCode);
      }
      throw new TaLibArgumentException("ER open: " + retCode, retCode);
   }
   /**
    * Open a live ER stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ER} at that bar.
    * <p>The history must hold at least {@code ER_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public ErStream erOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("ER open", "inReal", inReal);
      requireHistory("ER open", inReal.length);
      return erOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#erOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ER} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link ErStream#outRange()}.
    */
   public ErStream erOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("ER openAndFill", "inReal", inReal);
      requireHistory("ER openAndFill", inReal.length);
      int guardOutLen = openFillCount("ER openAndFill", inReal.length, ER_Lookback(optInTimePeriod));
      requireLength("ER openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("ER openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return erOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
