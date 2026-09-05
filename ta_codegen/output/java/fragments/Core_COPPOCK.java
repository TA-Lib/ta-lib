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
 *  090526 KL     First version (issue #362).
 */

   /**
    * Number of leading input bars {@link Core#COPPOCK} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInWMAPeriod Smoothing period for the ROC sum (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC1Period Short rate-of-change period (default 11; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC2Period Long rate-of-change period (default 14; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int COPPOCK_Lookback( int optInWMAPeriod, int optInROC1Period, int optInROC2Period )
   {
      if( optInWMAPeriod == Integer.MIN_VALUE ) {
         optInWMAPeriod = 10;
      } else if( optInWMAPeriod < 1 || optInWMAPeriod > 100000 ) {
         return -1;
      }
      if( optInROC1Period == Integer.MIN_VALUE ) {
         optInROC1Period = 11;
      } else if( optInROC1Period < 1 || optInROC1Period > 100000 ) {
         return -1;
      }
      if( optInROC2Period == Integer.MIN_VALUE ) {
         optInROC2Period = 14;
      } else if( optInROC2Period < 1 || optInROC2Period > 100000 ) {
         return -1;
      }
      /* The ROC sum needs max(p1,p2) prior bars before it exists at all, and the
       * WMA needs optInWMAPeriod values of that sum -> the first output sits at
       * max(p1,p2) + optInWMAPeriod - 1. The lookback keys off the MAX, which is
       * why optInROC1Period > optInROC2Period is accepted rather than rejected:
       * the formula is symmetric in the two ROCs (issue #362, decision 2).
       */
      if( optInROC1Period > optInROC2Period ) {
         return optInROC1Period + optInWMAPeriod - 1 ;
      }
      return optInROC2Period + optInWMAPeriod - 1 ;

   }
   RetCode COPPOCK_Impl( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInWMAPeriod,
                         int optInROC1Period,
                         int optInROC2Period,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int outIdx = 0;
      int inIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int q = 0;
      int rw = 0;
      int ringWalk = 0;
      int ringSize = 0;
      int barsSinceReseed = 0;
      int roc1Idx = 0;
      int roc2Idx = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double trailingValue = 0;
      double divider = 0;
      double base1 = 0;
      double base2 = 0;
      double roc1 = 0;
      double roc2 = 0;
      double[] sRing;
      int sRing_Idx = 0;
      int maxIdx_sRing = (50)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInWMAPeriod == Integer.MIN_VALUE ) {
         optInWMAPeriod = 10;
      } else if( optInWMAPeriod < 1 || optInWMAPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInROC1Period == Integer.MIN_VALUE ) {
         optInROC1Period = 11;
      } else if( optInROC1Period < 1 || optInROC1Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInROC2Period == Integer.MIN_VALUE ) {
         optInROC2Period = 14;
      } else if( optInROC2Period < 1 || optInROC2Period > 100000 ) {
         return RetCode.BadParam;
      }
      /* Coppock Curve: a WMA(optInWMAPeriod) of the SUM of two rates of change,
       * ROC(optInROC1Period) + ROC(optInROC2Period). The sum, not the mean:
       * every published definition sums them; Tulip's beta/copp.c averages and
       * therefore reads at exactly half this amplitude. A clean 2.000000x ratio
       * against Tulip is Tulip's variant, not a defect here (issue #362).
       *
       * The smoothed series is the inline expression
       *    S(j) = R(j,p1) + R(j,p2)
       *    R(j,p) = (inReal[j-p] != 0.0) ? ((inReal[j]/inReal[j-p])-1.0)*100.0
       *                                  : 0.0
       * -- TA_ROC's own zero guard included, so a zero price yields 0.0 exactly
       * where TA_ROC yields 0.0, never an inf that pollutes the window. Each
       * lagged denominator goes through its own trailing cursor advanced in
       * lock-step (TA_ROC's own shape) -- a parameter-sized lag subscript is
       * outside the stream classifier's index grammar.
       *
       * The WMA stage reproduces TA_WMA's recurrence verbatim -- the triangle
       * divider computed in double (#142), the periodSum/periodSub carry and the
       * 8*w re-anchor (#254) -- because anything short of verbatim breaks the
       * bit-exact composite differential against TA_ROC + TA_ROC + TA_WMA. S is
       * a DERIVED series that is never materialised, so, exactly as in TA_HMA's
       * outer stage, each S value is computed once and carried in a
       * (optInWMAPeriod-1)-slot ring: the trailing subtraction reads the
       * expiring slot and the re-anchor walks the ring, oldest first, weight
       * counting up from 1, then adds the current bar at weight w.
       */
      lookbackTotal = COPPOCK_Lookback(optInWMAPeriod, optInROC1Period, optInROC2Period);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Triangle divider in double: the int product w*(w+1) overflows int32 at
       * w >= 46341 (#142), exactly as in TA_WMA.
       */
      divider = (double)optInWMAPeriod * (optInWMAPeriod + 1) / 2.0;
      outIdx = 0;
      /* The S value computed at bar t expires optInWMAPeriod-1 bars later, so a
       * single-cursor ring of optInWMAPeriod-1 slots is enough: read the
       * expiring value, overwrite the slot, advance. At w == 1 the recurrence's
       * state is never consumed (see the identity output below), but the ring
       * still needs one slot for the reads to stay in bounds.
       */
      ringSize = optInWMAPeriod - 1;
      if( ringSize < 1 ) {
         ringSize = 1;
      }
      if( ringSize < 1 ) return RetCode.InternalError;
      sRing = new double[ringSize];
      maxIdx_sRing = (ringSize)-1;
      sRing_Idx = 0;
      /* At w == 1 the priming loop below never runs, so the first trailing read
       * would see an undefined slot; at w > 1 priming overwrites every slot.
       */
      sRing[0] = 0.0;
      /* One trailing cursor per ROC denominator, advanced in lock-step from the
       * priming scan onward.
       */
      inIdx = startIdx - (optInWMAPeriod - 1);
      roc1Idx = inIdx - optInROC1Period;
      roc2Idx = inIdx - optInROC2Period;
      /* Priming: the w-1 S values before the first output, oldest first with
       * the weight counting up from 1 -- TA_WMA's own priming order, which the
       * re-anchor below must reproduce. They also fill the ring.
       */
      periodSub = (double)0.0;
      periodSum = periodSub;
      i = 1;
      while( inIdx < startIdx ) {
         base1 = inReal[roc1Idx];
         roc1Idx += 1;
         base2 = inReal[roc2Idx];
         roc2Idx += 1;
         roc1 = (base1 != 0.0) ? (inReal[inIdx] / base1 - 1.0) * 100.0 : 0.0;
         roc2 = (base2 != 0.0) ? (inReal[inIdx] / base2 - 1.0) * 100.0 : 0.0;
         tempReal = roc1 + roc2;
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
         sRing[sRing_Idx] = tempReal;
         sRing_Idx++;
         if( sRing_Idx > maxIdx_sRing ) { sRing_Idx = 0; }
         inIdx += 1;
      }
      barsSinceReseed = 8 * optInWMAPeriod;
      trailingValue = 0.0;
      /* Tight loop for the requested range. */
      while( inIdx <= endIdx ) {
         base1 = inReal[roc1Idx];
         roc1Idx += 1;
         base2 = inReal[roc2Idx];
         roc2Idx += 1;
         roc1 = (base1 != 0.0) ? (inReal[inIdx] / base1 - 1.0) * 100.0 : 0.0;
         roc2 = (base2 != 0.0) ? (inReal[inIdx] / base2 - 1.0) * 100.0 : 0.0;
         tempReal = roc1 + roc2;
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInWMAPeriod;
         /* Re-anchor every 8*w bars: rebuild both totals from the window
          * itself -- TA_WMA's #254 fix, same single trigger, same interval.
          * The window lives in the ring (w-1 prior S values, sRing_Idx the
          * oldest) plus the current bar's tempReal at weight w.
          */
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 ) {
            barsSinceReseed = 8 * optInWMAPeriod;
            periodSub = (double)0.0;
            periodSum = (double)0.0;
            rw = 1;
            ringWalk = sRing_Idx;
            for( q = 0; q < ringSize; q += 1 ) {
               tempReal2 = sRing[ringWalk];
               periodSub += tempReal2;
               periodSum += tempReal2 * rw;
               rw += 1;
               ringWalk += 1;
               if( ringWalk >= ringSize ) {
                  ringWalk = 0;
               }
            }
            periodSub += tempReal;
            periodSum += tempReal * optInWMAPeriod;
         }
         /* Read the expiring S value BEFORE overwriting its slot with the
          * current one -- the ring is the aliasing-safe stand-in for TA_WMA's
          * "save the trailing value before the store" rule.
          */
         trailingValue = sRing[sRing_Idx];
         sRing[sRing_Idx] = tempReal;
         sRing_Idx++;
         if( sRing_Idx > maxIdx_sRing ) { sRing_Idx = 0; }
         /* Load-bearing, not a rounding nicety: keep it. WMA(1) is the identity
          * and TA_WMA ships an exact copy fast path, but the recurrence here is
          * off by a whole term at w == 1 -- ringSize clamps to 1, so the
          * read-before-write ring hands back the wrong trailing value. Deleting
          * this arm moves TA_SREF bar 16 at (1,11,14) from -11.311839169954585
          * to -6.4591709868291103.
          */
         if( optInWMAPeriod == 1 ) {
            outReal[outIdx] = tempReal;
         } else {
            outReal[outIdx] = periodSum / divider;
         }
         outIdx += 1;
         periodSum -= periodSub;
         inIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode COPPOCK_Impl( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInWMAPeriod,
                         int optInROC1Period,
                         int optInROC2Period,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int outIdx = 0;
      int inIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int q = 0;
      int rw = 0;
      int ringWalk = 0;
      int ringSize = 0;
      int barsSinceReseed = 0;
      int roc1Idx = 0;
      int roc2Idx = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double trailingValue = 0;
      double divider = 0;
      double base1 = 0;
      double base2 = 0;
      double roc1 = 0;
      double roc2 = 0;
      double[] sRing;
      int sRing_Idx = 0;
      int maxIdx_sRing = (50)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInWMAPeriod == Integer.MIN_VALUE ) {
         optInWMAPeriod = 10;
      } else if( optInWMAPeriod < 1 || optInWMAPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInROC1Period == Integer.MIN_VALUE ) {
         optInROC1Period = 11;
      } else if( optInROC1Period < 1 || optInROC1Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInROC2Period == Integer.MIN_VALUE ) {
         optInROC2Period = 14;
      } else if( optInROC2Period < 1 || optInROC2Period > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = COPPOCK_Lookback(optInWMAPeriod, optInROC1Period, optInROC2Period);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      divider = (double)optInWMAPeriod * (optInWMAPeriod + 1) / 2.0;
      outIdx = 0;
      ringSize = optInWMAPeriod - 1;
      if( ringSize < 1 ) {
         ringSize = 1;
      }
      if( ringSize < 1 ) return RetCode.InternalError;
      sRing = new double[ringSize];
      maxIdx_sRing = (ringSize)-1;
      sRing_Idx = 0;
      sRing[0] = 0.0;
      inIdx = startIdx - (optInWMAPeriod - 1);
      roc1Idx = inIdx - optInROC1Period;
      roc2Idx = inIdx - optInROC2Period;
      periodSub = (double)0.0;
      periodSum = periodSub;
      i = 1;
      while( inIdx < startIdx ) {
         base1 = (double)inReal[roc1Idx];
         roc1Idx += 1;
         base2 = (double)inReal[roc2Idx];
         roc2Idx += 1;
         roc1 = (base1 != 0.0) ? ((double)inReal[inIdx] / base1 - 1.0) * 100.0 : 0.0;
         roc2 = (base2 != 0.0) ? ((double)inReal[inIdx] / base2 - 1.0) * 100.0 : 0.0;
         tempReal = roc1 + roc2;
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
         sRing[sRing_Idx] = tempReal;
         sRing_Idx++;
         if( sRing_Idx > maxIdx_sRing ) { sRing_Idx = 0; }
         inIdx += 1;
      }
      barsSinceReseed = 8 * optInWMAPeriod;
      trailingValue = 0.0;
      while( inIdx <= endIdx ) {
         base1 = (double)inReal[roc1Idx];
         roc1Idx += 1;
         base2 = (double)inReal[roc2Idx];
         roc2Idx += 1;
         roc1 = (base1 != 0.0) ? ((double)inReal[inIdx] / base1 - 1.0) * 100.0 : 0.0;
         roc2 = (base2 != 0.0) ? ((double)inReal[inIdx] / base2 - 1.0) * 100.0 : 0.0;
         tempReal = roc1 + roc2;
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInWMAPeriod;
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 ) {
            barsSinceReseed = 8 * optInWMAPeriod;
            periodSub = (double)0.0;
            periodSum = (double)0.0;
            rw = 1;
            ringWalk = sRing_Idx;
            for( q = 0; q < ringSize; q += 1 ) {
               tempReal2 = sRing[ringWalk];
               periodSub += tempReal2;
               periodSum += tempReal2 * rw;
               rw += 1;
               ringWalk += 1;
               if( ringWalk >= ringSize ) {
                  ringWalk = 0;
               }
            }
            periodSub += tempReal;
            periodSum += tempReal * optInWMAPeriod;
         }
         trailingValue = sRing[sRing_Idx];
         sRing[sRing_Idx] = tempReal;
         sRing_Idx++;
         if( sRing_Idx > maxIdx_sRing ) { sRing_Idx = 0; }
         if( optInWMAPeriod == 1 ) {
            outReal[outIdx] = tempReal;
         } else {
            outReal[outIdx] = periodSum / divider;
         }
         outIdx += 1;
         periodSum -= periodSub;
         inIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Coppock Curve: Edwin S. "Sedge" Coppock's long-term momentum oscillator
    * (*Barron's*, originally published as the "Trendex Model"), computed as a
    * weighted moving average of the **sum** of two rates of change. Unbounded;
    * positive turns from below zero are the signal the indicator was designed
    * for (long-term buying opportunities on monthly index data).
    * <p><b>Formula</b>
    * <pre>{@code
    * `COPPOCK = WMA(ROC(optInROC1Period) + ROC(optInROC2Period), optInWMAPeriod)`
    * Each ROC carries [`ROC`](/functions/roc)'s own zero guard — a zero price `optInROC*Period` bars back yields 0.0 for that term, never an infinity. The two ROCs are **summed**, not averaged: every published definition sums them. (Tulip's `copp` averages, so it reads at exactly half this amplitude — a clean 2.0x ratio against Tulip is Tulip's variant, not a defect.)
    * The formula is symmetric in the two ROC periods and the lookback keys off their max, so `optInROC1Period > optInROC2Period` is accepted rather than rejected.
    * The classic defaults are 11/14/10 on monthly data. Wikipedia's daily-scale variant (231/294-bar ROC, 210-bar WMA) is a parameter choice reachable through this API, not a competing formula.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The single fused pass is bit-identical to running {@code ROC + ROC} into [{@code WMA}](/functions/wma).</li>
    * <li>First output at {@code max(optInROC1Period, optInROC2Period) + optInWMAPeriod - 1}. Not start-dependent: each output depends only on its finite trailing window.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#COPPOCK_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series (canonically a monthly close)
    * @param optInWMAPeriod Smoothing period for the ROC sum (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC1Period Short rate-of-change period (default 11; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC2Period Long rate-of-change period (default 14; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Coppock Curve value. Must hold at least
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
    */
   public OutRange COPPOCK( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInWMAPeriod,
                            int optInROC1Period,
                            int optInROC2Period,
                            double outReal[] )
   {
      requireIndexRange("COPPOCK", startIdx, endIdx);
      int guardStart = clampedStart("COPPOCK", startIdx, COPPOCK_Lookback(optInWMAPeriod, optInROC1Period, optInROC2Period));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("COPPOCK", "inReal", inReal, guardInLen);
      requireLength("COPPOCK", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = COPPOCK_Impl(startIdx, endIdx, inReal, optInWMAPeriod, optInROC1Period, optInROC2Period, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("COPPOCK", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Coppock Curve: Edwin S. "Sedge" Coppock's long-term momentum oscillator
    * (*Barron's*, originally published as the "Trendex Model"), computed as a
    * weighted moving average of the **sum** of two rates of change. Unbounded;
    * positive turns from below zero are the signal the indicator was designed
    * for (long-term buying opportunities on monthly index data).
    * <p><b>Formula</b>
    * <pre>{@code
    * `COPPOCK = WMA(ROC(optInROC1Period) + ROC(optInROC2Period), optInWMAPeriod)`
    * Each ROC carries [`ROC`](/functions/roc)'s own zero guard — a zero price `optInROC*Period` bars back yields 0.0 for that term, never an infinity. The two ROCs are **summed**, not averaged: every published definition sums them. (Tulip's `copp` averages, so it reads at exactly half this amplitude — a clean 2.0x ratio against Tulip is Tulip's variant, not a defect.)
    * The formula is symmetric in the two ROC periods and the lookback keys off their max, so `optInROC1Period > optInROC2Period` is accepted rather than rejected.
    * The classic defaults are 11/14/10 on monthly data. Wikipedia's daily-scale variant (231/294-bar ROC, 210-bar WMA) is a parameter choice reachable through this API, not a competing formula.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The single fused pass is bit-identical to running {@code ROC + ROC} into [{@code WMA}](/functions/wma).</li>
    * <li>First output at {@code max(optInROC1Period, optInROC2Period) + optInWMAPeriod - 1}. Not start-dependent: each output depends only on its finite trailing window.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#COPPOCK_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series (canonically a monthly close)
    * @param optInWMAPeriod Smoothing period for the ROC sum (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC1Period Short rate-of-change period (default 11; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC2Period Long rate-of-change period (default 14; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Coppock Curve value. Must hold at least
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
    */
   public OutRange COPPOCK( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInWMAPeriod,
                            int optInROC1Period,
                            int optInROC2Period,
                            double outReal[] )
   {
      requireIndexRange("COPPOCK", startIdx, endIdx);
      int guardStart = clampedStart("COPPOCK", startIdx, COPPOCK_Lookback(optInWMAPeriod, optInROC1Period, optInROC2Period));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("COPPOCK", "inReal", inReal, guardInLen);
      requireLength("COPPOCK", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = COPPOCK_Impl(startIdx, endIdx, inReal, optInWMAPeriod, optInROC1Period, optInROC2Period, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("COPPOCK", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live COPPOCK stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#COPPOCK} over the same series.
    * Open with {@link Core#coppockOpen}; there is no close — the handle is
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
   public static final class CoppockStream {
      Core core;
      int optInWMAPeriod;
      int optInROC1Period;
      int optInROC2Period;
      int ringSize;
      int barsSinceReseed;
      double periodSum;
      double periodSub;
      double trailingValue;
      double divider;
      int sRing_Idx;
      int maxIdx_sRing;
      int ringPos_roc1Idx;
      int ringCap_roc1Idx;
      double[] ring_roc1Idx_inReal;
      int ringPos_roc2Idx;
      int ringCap_roc2Idx;
      double[] ring_roc2Idx_inReal;
      int cbSize_sRing;
      double[] cb_sRing;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      CoppockStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#COPPOCK} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CoppockStream( CoppockStream other ) {
         this.core = other.core;
         this.optInWMAPeriod = other.optInWMAPeriod;
         this.optInROC1Period = other.optInROC1Period;
         this.optInROC2Period = other.optInROC2Period;
         this.ringSize = other.ringSize;
         this.barsSinceReseed = other.barsSinceReseed;
         this.periodSum = other.periodSum;
         this.periodSub = other.periodSub;
         this.trailingValue = other.trailingValue;
         this.divider = other.divider;
         this.sRing_Idx = other.sRing_Idx;
         this.maxIdx_sRing = other.maxIdx_sRing;
         this.ringPos_roc1Idx = other.ringPos_roc1Idx;
         this.ringCap_roc1Idx = other.ringCap_roc1Idx;
         this.ring_roc1Idx_inReal = other.ring_roc1Idx_inReal.clone();
         this.ringPos_roc2Idx = other.ringPos_roc2Idx;
         this.ringCap_roc2Idx = other.ringCap_roc2Idx;
         this.ring_roc2Idx_inReal = other.ring_roc2Idx_inReal.clone();
         this.cbSize_sRing = other.cbSize_sRing;
         this.cb_sRing = other.cb_sRing.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("COPPOCK update: BadParam", RetCode.BadParam);
         }
         core.coppockStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         requireArgument("COPPOCK updateAndFill", "inReal", inReal);
         requireArgument("COPPOCK updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("COPPOCK updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("COPPOCK updateAndFill: BadParam", RetCode.BadParam);
            }
            core.coppockStepImpl(this, inReal[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
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
            throw new TaLibArgumentException("COPPOCK peek: BadParam", RetCode.BadParam);
         CoppockStream sp = this;
         int q = 0;
         int rw = 0;
         int ringWalk = 0;
         double tempReal = 0.0;
         double tempReal2 = 0.0;
         double base1 = 0.0;
         double base2 = 0.0;
         double roc1 = 0.0;
         double roc2 = 0.0;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = 0.0;
         double periodSub = sp.periodSub;
         double periodSum = sp.periodSum;
         int sRing_Idx = sp.sRing_Idx;
         double trailingValue = sp.trailingValue;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         if( sp.ringCap_roc1Idx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         if( sp.ringCap_roc2Idx == 0 ) {
            pkSlot1 = 0;
            pkVal1 = inReal;
         }
         base1 = (sp.ringPos_roc1Idx != pkSlot0) ? sp.ring_roc1Idx_inReal[sp.ringPos_roc1Idx] : pkVal0;
         base2 = (sp.ringPos_roc2Idx != pkSlot1) ? sp.ring_roc2Idx_inReal[sp.ringPos_roc2Idx] : pkVal1;
         roc1 = (base1 != 0.0) ? (inReal / base1 - 1.0) * 100.0 : 0.0;
         roc2 = (base2 != 0.0) ? (inReal / base2 - 1.0) * 100.0 : 0.0;
         tempReal = roc1 + roc2;
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * sp.optInWMAPeriod;
         /* Re-anchor every 8*w bars: rebuild both totals from the window
          * itself -- TA_WMA's #254 fix, same single trigger, same interval.
          * The window lives in the ring (w-1 prior S values, sRing_Idx the
          * oldest) plus the current bar's tempReal at weight w.
          */
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 ) {
            barsSinceReseed = 8 * sp.optInWMAPeriod;
            periodSub = (double)0.0;
            periodSum = (double)0.0;
            rw = 1;
            ringWalk = sRing_Idx;
            for( q = 0; q < sp.ringSize; q += 1 ) {
               tempReal2 = sp.cb_sRing[ringWalk];
               periodSub += tempReal2;
               periodSum += tempReal2 * rw;
               rw += 1;
               ringWalk += 1;
               if( ringWalk >= sp.ringSize ) {
                  ringWalk = 0;
               }
            }
            periodSub += tempReal;
            periodSum += tempReal * sp.optInWMAPeriod;
         }
         /* Read the expiring S value BEFORE overwriting its slot with the
          * current one -- the ring is the aliasing-safe stand-in for TA_WMA's
          * "save the trailing value before the store" rule.
          */
         trailingValue = sp.cb_sRing[sRing_Idx];
         sRing_Idx = sRing_Idx + 1;
         if( sRing_Idx > sp.maxIdx_sRing ) {
            sRing_Idx = 0;
         }
         /* Load-bearing, not a rounding nicety: keep it. WMA(1) is the identity
          * and TA_WMA ships an exact copy fast path, but the recurrence here is
          * off by a whole term at w == 1 -- ringSize clamps to 1, so the
          * read-before-write ring hands back the wrong trailing value. Deleting
          * this arm moves TA_SREF bar 16 at (1,11,14) from -11.311839169954585
          * to -6.4591709868291103.
          */
         if( sp.optInWMAPeriod == 1 ) {
            cur_outReal = tempReal;
         } else {
            cur_outReal = periodSum / sp.divider;
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
      public CoppockStream clone() {
         return new CoppockStream(this);
      }
   }
   void coppockStepImpl( CoppockStream sp, double inReal )
   {
      int q = 0;
      int rw = 0;
      int ringWalk = 0;
      double tempReal = 0.0;
      double tempReal2 = 0.0;
      double base1 = 0.0;
      double base2 = 0.0;
      double roc1 = 0.0;
      double roc2 = 0.0;
      if( sp.ringCap_roc1Idx == 0 ) {
         sp.ring_roc1Idx_inReal[0] = inReal;
      }
      if( sp.ringCap_roc2Idx == 0 ) {
         sp.ring_roc2Idx_inReal[0] = inReal;
      }
      base1 = sp.ring_roc1Idx_inReal[sp.ringPos_roc1Idx];
      base2 = sp.ring_roc2Idx_inReal[sp.ringPos_roc2Idx];
      roc1 = (base1 != 0.0) ? (inReal / base1 - 1.0) * 100.0 : 0.0;
      roc2 = (base2 != 0.0) ? (inReal / base2 - 1.0) * 100.0 : 0.0;
      tempReal = roc1 + roc2;
      sp.periodSub += tempReal;
      sp.periodSub -= sp.trailingValue;
      sp.periodSum += tempReal * sp.optInWMAPeriod;
      /* Re-anchor every 8*w bars: rebuild both totals from the window
       * itself -- TA_WMA's #254 fix, same single trigger, same interval.
       * The window lives in the ring (w-1 prior S values, sRing_Idx the
       * oldest) plus the current bar's tempReal at weight w.
       */
      sp.barsSinceReseed -= 1;
      if( sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = 8 * sp.optInWMAPeriod;
         sp.periodSub = (double)0.0;
         sp.periodSum = (double)0.0;
         rw = 1;
         ringWalk = sp.sRing_Idx;
         for( q = 0; q < sp.ringSize; q += 1 ) {
            tempReal2 = sp.cb_sRing[ringWalk];
            sp.periodSub += tempReal2;
            sp.periodSum += tempReal2 * rw;
            rw += 1;
            ringWalk += 1;
            if( ringWalk >= sp.ringSize ) {
               ringWalk = 0;
            }
         }
         sp.periodSub += tempReal;
         sp.periodSum += tempReal * sp.optInWMAPeriod;
      }
      /* Read the expiring S value BEFORE overwriting its slot with the
       * current one -- the ring is the aliasing-safe stand-in for TA_WMA's
       * "save the trailing value before the store" rule.
       */
      sp.trailingValue = sp.cb_sRing[sp.sRing_Idx];
      sp.cb_sRing[sp.sRing_Idx] = tempReal;
      sp.sRing_Idx = sp.sRing_Idx + 1;
      if( sp.sRing_Idx > sp.maxIdx_sRing ) {
         sp.sRing_Idx = 0;
      }
      /* Load-bearing, not a rounding nicety: keep it. WMA(1) is the identity
       * and TA_WMA ships an exact copy fast path, but the recurrence here is
       * off by a whole term at w == 1 -- ringSize clamps to 1, so the
       * read-before-write ring hands back the wrong trailing value. Deleting
       * this arm moves TA_SREF bar 16 at (1,11,14) from -11.311839169954585
       * to -6.4591709868291103.
       */
      if( sp.optInWMAPeriod == 1 ) {
         sp.cur_outReal = tempReal;
      } else {
         sp.cur_outReal = sp.periodSum / sp.divider;
      }
      sp.periodSum -= sp.periodSub;
      sp.ring_roc1Idx_inReal[sp.ringPos_roc1Idx] = inReal;
      sp.ringPos_roc1Idx = sp.ringPos_roc1Idx + 1;
      if( sp.ringPos_roc1Idx >= sp.ringCap_roc1Idx ) {
         sp.ringPos_roc1Idx = 0;
      }
      sp.ring_roc2Idx_inReal[sp.ringPos_roc2Idx] = inReal;
      sp.ringPos_roc2Idx = sp.ringPos_roc2Idx + 1;
      if( sp.ringPos_roc2Idx >= sp.ringCap_roc2Idx ) {
         sp.ringPos_roc2Idx = 0;
      }
   }
   private RetCode coppockOpenImpl( CoppockStream sp, double inReal[], int startIdx, int optInWMAPeriod, int optInROC1Period, int optInROC2Period, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int inIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int q = 0;
      int rw = 0;
      int ringWalk = 0;
      int ringSize = 0;
      int barsSinceReseed = 0;
      int roc1Idx = 0;
      int roc2Idx = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double trailingValue = 0;
      double divider = 0;
      double base1 = 0;
      double base2 = 0;
      double roc1 = 0;
      double roc2 = 0;
      double[] sRing;
      int sRing_Idx = 0;
      int maxIdx_sRing = (50)-1;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInWMAPeriod == Integer.MIN_VALUE ) {
         optInWMAPeriod = 10;
      } else if( optInWMAPeriod < 1 || optInWMAPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInROC1Period == Integer.MIN_VALUE ) {
         optInROC1Period = 11;
      } else if( optInROC1Period < 1 || optInROC1Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInROC2Period == Integer.MIN_VALUE ) {
         optInROC2Period = 14;
      } else if( optInROC2Period < 1 || optInROC2Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Coppock Curve: a WMA(optInWMAPeriod) of the SUM of two rates of change,
       * ROC(optInROC1Period) + ROC(optInROC2Period). The sum, not the mean:
       * every published definition sums them; Tulip's beta/copp.c averages and
       * therefore reads at exactly half this amplitude. A clean 2.000000x ratio
       * against Tulip is Tulip's variant, not a defect here (issue #362).
       *
       * The smoothed series is the inline expression
       *    S(j) = R(j,p1) + R(j,p2)
       *    R(j,p) = (inReal[j-p] != 0.0) ? ((inReal[j]/inReal[j-p])-1.0)*100.0
       *                                  : 0.0
       * -- TA_ROC's own zero guard included, so a zero price yields 0.0 exactly
       * where TA_ROC yields 0.0, never an inf that pollutes the window. Each
       * lagged denominator goes through its own trailing cursor advanced in
       * lock-step (TA_ROC's own shape) -- a parameter-sized lag subscript is
       * outside the stream classifier's index grammar.
       *
       * The WMA stage reproduces TA_WMA's recurrence verbatim -- the triangle
       * divider computed in double (#142), the periodSum/periodSub carry and the
       * 8*w re-anchor (#254) -- because anything short of verbatim breaks the
       * bit-exact composite differential against TA_ROC + TA_ROC + TA_WMA. S is
       * a DERIVED series that is never materialised, so, exactly as in TA_HMA's
       * outer stage, each S value is computed once and carried in a
       * (optInWMAPeriod-1)-slot ring: the trailing subtraction reads the
       * expiring slot and the re-anchor walks the ring, oldest first, weight
       * counting up from 1, then adds the current bar at weight w.
       */
      lookbackTotal = COPPOCK_Lookback(optInWMAPeriod, optInROC1Period, optInROC2Period);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Triangle divider in double: the int product w*(w+1) overflows int32 at
       * w >= 46341 (#142), exactly as in TA_WMA.
       */
      divider = (double)optInWMAPeriod * (optInWMAPeriod + 1) / 2.0;
      outIdx = 0;
      /* The S value computed at bar t expires optInWMAPeriod-1 bars later, so a
       * single-cursor ring of optInWMAPeriod-1 slots is enough: read the
       * expiring value, overwrite the slot, advance. At w == 1 the recurrence's
       * state is never consumed (see the identity output below), but the ring
       * still needs one slot for the reads to stay in bounds.
       */
      ringSize = optInWMAPeriod - 1;
      if( ringSize < 1 ) {
         ringSize = 1;
      }
      if( ringSize < 1 ) return RetCode.InternalError;
      sRing = new double[ringSize];
      maxIdx_sRing = (ringSize)-1;
      sRing_Idx = 0;
      /* At w == 1 the priming loop below never runs, so the first trailing read
       * would see an undefined slot; at w > 1 priming overwrites every slot.
       */
      sRing[0] = 0.0;
      /* One trailing cursor per ROC denominator, advanced in lock-step from the
       * priming scan onward.
       */
      inIdx = startIdx - (optInWMAPeriod - 1);
      roc1Idx = inIdx - optInROC1Period;
      roc2Idx = inIdx - optInROC2Period;
      /* Priming: the w-1 S values before the first output, oldest first with
       * the weight counting up from 1 -- TA_WMA's own priming order, which the
       * re-anchor below must reproduce. They also fill the ring.
       */
      periodSub = (double)0.0;
      periodSum = periodSub;
      i = 1;
      while( inIdx < startIdx ) {
         base1 = inReal[roc1Idx];
         roc1Idx += 1;
         base2 = inReal[roc2Idx];
         roc2Idx += 1;
         roc1 = (base1 != 0.0) ? (inReal[inIdx] / base1 - 1.0) * 100.0 : 0.0;
         roc2 = (base2 != 0.0) ? (inReal[inIdx] / base2 - 1.0) * 100.0 : 0.0;
         tempReal = roc1 + roc2;
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
         sRing[sRing_Idx] = tempReal;
         sRing_Idx++;
         if( sRing_Idx > maxIdx_sRing ) { sRing_Idx = 0; }
         inIdx += 1;
      }
      barsSinceReseed = 8 * optInWMAPeriod;
      trailingValue = 0.0;
      /* Tight loop for the requested range. */
      while( inIdx <= endIdx ) {
         base1 = inReal[roc1Idx];
         roc1Idx += 1;
         base2 = inReal[roc2Idx];
         roc2Idx += 1;
         roc1 = (base1 != 0.0) ? (inReal[inIdx] / base1 - 1.0) * 100.0 : 0.0;
         roc2 = (base2 != 0.0) ? (inReal[inIdx] / base2 - 1.0) * 100.0 : 0.0;
         tempReal = roc1 + roc2;
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInWMAPeriod;
         /* Re-anchor every 8*w bars: rebuild both totals from the window
          * itself -- TA_WMA's #254 fix, same single trigger, same interval.
          * The window lives in the ring (w-1 prior S values, sRing_Idx the
          * oldest) plus the current bar's tempReal at weight w.
          */
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 ) {
            barsSinceReseed = 8 * optInWMAPeriod;
            periodSub = (double)0.0;
            periodSum = (double)0.0;
            rw = 1;
            ringWalk = sRing_Idx;
            for( q = 0; q < ringSize; q += 1 ) {
               tempReal2 = sRing[ringWalk];
               periodSub += tempReal2;
               periodSum += tempReal2 * rw;
               rw += 1;
               ringWalk += 1;
               if( ringWalk >= ringSize ) {
                  ringWalk = 0;
               }
            }
            periodSub += tempReal;
            periodSum += tempReal * optInWMAPeriod;
         }
         /* Read the expiring S value BEFORE overwriting its slot with the
          * current one -- the ring is the aliasing-safe stand-in for TA_WMA's
          * "save the trailing value before the store" rule.
          */
         trailingValue = sRing[sRing_Idx];
         sRing[sRing_Idx] = tempReal;
         sRing_Idx++;
         if( sRing_Idx > maxIdx_sRing ) { sRing_Idx = 0; }
         /* Load-bearing, not a rounding nicety: keep it. WMA(1) is the identity
          * and TA_WMA ships an exact copy fast path, but the recurrence here is
          * off by a whole term at w == 1 -- ringSize clamps to 1, so the
          * read-before-write ring hands back the wrong trailing value. Deleting
          * this arm moves TA_SREF bar 16 at (1,11,14) from -11.311839169954585
          * to -6.4591709868291103.
          */
         if( optInWMAPeriod == 1 ) {
            outReal[outIdx * outStride] = tempReal;
         } else {
            outReal[outIdx * outStride] = periodSum / divider;
         }
         outIdx += 1;
         periodSum -= periodSub;
         inIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_roc1Idx = inIdx - roc1Idx;
      if( cap_roc1Idx < 0 || cap_roc1Idx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_roc1Idx = (cap_roc1Idx > 0)? cap_roc1Idx : 1;
      double[] capRing_roc1Idx_inReal = new double[allocN_roc1Idx];
      System.arraycopy(inReal, historyLen - cap_roc1Idx, capRing_roc1Idx_inReal, 0, cap_roc1Idx);
      int cap_roc2Idx = inIdx - roc2Idx;
      if( cap_roc2Idx < 0 || cap_roc2Idx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_roc2Idx = (cap_roc2Idx > 0)? cap_roc2Idx : 1;
      double[] capRing_roc2Idx_inReal = new double[allocN_roc2Idx];
      System.arraycopy(inReal, historyLen - cap_roc2Idx, capRing_roc2Idx_inReal, 0, cap_roc2Idx);
      int capCb_sRing = maxIdx_sRing + 1;
      if( capCb_sRing > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInWMAPeriod = optInWMAPeriod;
      sp.optInROC1Period = optInROC1Period;
      sp.optInROC2Period = optInROC2Period;
      sp.ringSize = ringSize;
      sp.barsSinceReseed = barsSinceReseed;
      sp.periodSum = periodSum;
      sp.periodSub = periodSub;
      sp.trailingValue = trailingValue;
      sp.divider = divider;
      sp.sRing_Idx = sRing_Idx;
      sp.maxIdx_sRing = maxIdx_sRing;
      sp.ringPos_roc1Idx = 0;
      sp.ringCap_roc1Idx = cap_roc1Idx;
      sp.ring_roc1Idx_inReal = capRing_roc1Idx_inReal;
      sp.ringPos_roc2Idx = 0;
      sp.ringCap_roc2Idx = cap_roc2Idx;
      sp.ring_roc2Idx_inReal = capRing_roc2Idx_inReal;
      sp.cbSize_sRing = capCb_sRing;
      sp.cb_sRing = sRing;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* coppockOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CoppockStream coppockOpenAndFillInternal( double inReal[], int startIdx, int optInWMAPeriod, int optInROC1Period, int optInROC2Period, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CoppockStream sp = new CoppockStream(this);
      RetCode retCode = coppockOpenImpl(sp, inReal, startIdx, optInWMAPeriod, optInROC1Period, optInROC2Period, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("COPPOCK openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("COPPOCK openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("COPPOCK openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind coppockOpen (composition seam). */
   CoppockStream coppockOpenInternal( double inReal[], int startIdx, int optInWMAPeriod, int optInROC1Period, int optInROC2Period )
   {
      CoppockStream sp = new CoppockStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = coppockOpenImpl(sp, inReal, startIdx, optInWMAPeriod, optInROC1Period, optInROC2Period, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("COPPOCK open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("COPPOCK open: internal error", retCode);
      }
      throw new TaLibArgumentException("COPPOCK open: " + retCode, retCode);
   }
   /**
    * Open a live COPPOCK stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#COPPOCK} at that bar.
    * <p>The history must hold at least {@code COPPOCK_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CoppockStream coppockOpen( double inReal[], int optInWMAPeriod, int optInROC1Period, int optInROC2Period )
   {
      requireArgument("COPPOCK open", "inReal", inReal);
      requireHistory("COPPOCK open", inReal.length);
      return coppockOpenInternal(inReal, 0, optInWMAPeriod, optInROC1Period, optInROC2Period);
   }
   /**
    * {@link Core#coppockOpen} that also fills the output array(s) bit-identically
    * to {@link Core#COPPOCK} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CoppockStream#outRange()}.
    */
   public CoppockStream coppockOpenAndFill( double inReal[], int optInWMAPeriod, int optInROC1Period, int optInROC2Period, double outReal[] )
   {
      requireArgument("COPPOCK openAndFill", "inReal", inReal);
      requireHistory("COPPOCK openAndFill", inReal.length);
      int guardOutLen = openFillCount("COPPOCK openAndFill", inReal.length, COPPOCK_Lookback(optInWMAPeriod, optInROC1Period, optInROC2Period));
      requireLength("COPPOCK openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("COPPOCK openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return coppockOpenAndFillInternal(inReal, 0, optInWMAPeriod, optInROC1Period, optInROC2Period, outBegIdx, outNBElement, outReal);
   }
