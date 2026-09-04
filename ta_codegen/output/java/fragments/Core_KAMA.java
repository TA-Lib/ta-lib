/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  062704 MF     Fix limit case to avoid divid by zero (or by
 *                a value close to zero induce by the imprecision
 *                of floating points).
 *  070226 MF,CC  Allow period of 1: output is a copy of the input,
 *                consistent with TA_MA (issues #48, #59). The natural
 *                KAMA math at period=1 would be a fixed-alpha EMA
 *                (efficiency ratio is always 1), which would disagree
 *                with TA_MA's period-1 copy, so identity is explicit.
 *  082326 MF,CC  Fix #253. Recognize a flat window by counting bars and drop
 *                the fixed TA_IS_ZERO band beside the efficiency ratio, which
 *                forced the fastest adaptation on any instrument quoted small
 *                enough to fall under it.
 */

   /**
    * Number of leading input bars {@link Core#KAMA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Lookback window for the efficiency ratio (default
    *        30; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int KAMA_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInTimePeriod == 1 ) {
         return this.unstablePeriod[FuncUnstId.KAMA.ordinal()] ;
      }
      return optInTimePeriod + this.unstablePeriod[FuncUnstId.KAMA.ordinal()] ;

   }
   RetCode KAMA_Impl( int startIdx,
                      int endIdx,
                      double inReal[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      int nullRun = 0;
      double trailingValue = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      /* Default return values */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). The unstable period
       * still delays the first output for API consistency.
       */
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.KAMA.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.Success ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.KAMA.ordinal()];
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Initialize the variables by going through
       * the lookback period.
       */
      sumROC1 = 0.0;
      /* Consecutive 1-day changes of exactly zero, counted so that a flat window
       * can be recognized exactly (the shape #244 needed for MFI). sumROC1 cannot
       * answer that question itself once the window starts sliding: it is
       * maintained by add-then-subtract, so a window that has gone flat leaves it
       * holding rounding residue of arbitrary sign rather than zero, and the
       * efficiency ratio then divides that residue into itself.
       */
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
      /* At this point sumROC1 represent the
       * summation of the 1-day price difference
       * over the (optInTimePeriod-1)
       */
      /* Calculate the first KAMA */
      /* The yesterday price is used here as the previous KAMA. */
      prevKAMA = inReal[today - 1];
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      /* Save the trailing value. Do this because inReal
       * and outReal can be pointers to the same buffer.
       */
      trailingValue = tempReal2;
      /* Calculate the efficiency ratio.
       *
       * The only threshold is `sumROC1 <= periodROC`, and it is scale-consistent:
       * both sides carry the quote unit. The fixed TA_IS_ZERO band that used to
       * sit beside it was not -- it declared the window flat, and forced the
       * fastest adaptation, for every window of an instrument quoted below it
       * (issue #253). A genuinely flat window is now recognized by the exact bar
       * count above instead.
       */
      if( sumROC1 <= periodROC ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      /* Calculate the smoothing constant */
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      /* Calculate the KAMA like an EMA, using the
       * smoothing constant as the adaptive factor.
       */
      prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      /* 'today' keep track of where the processing is within the
       * input.
       */
      /* Skip the unstable period. Do the whole processing
       * needed for KAMA, but do not write it in the output.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Once a whole window of flat bars has gone by, every 1-day change it
          * spans is exactly zero, so the sum is known to be exactly zero and the
          * residue can be dropped. That is what lets the efficiency ratio be
          * decided by `sumROC1 <= periodROC` alone: a window that flat has
          * periodROC == 0 too, so the test is 0 <= 0 and the ratio is 1.
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
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      /* Write the first value. */
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      /* Do the KAMA calculation for the requested range. */
      while( today <= endIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Once a whole window of flat bars has gone by, every 1-day change it
          * spans is exactly zero, so the sum is known to be exactly zero and the
          * residue can be dropped. That is what lets the efficiency ratio be
          * decided by `sumROC1 <= periodROC` alone: a window that flat has
          * periodROC == 0 too, so the test is 0 <= 0 and the ratio is 1.
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
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode KAMA_Impl( int startIdx,
                      int endIdx,
                      float inReal[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      int nullRun = 0;
      double trailingValue = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.KAMA.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.Success ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.KAMA.ordinal()];
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
      prevKAMA = (double)inReal[today - 1];
      tempReal = (double)inReal[today];
      tempReal2 = (double)inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      trailingValue = tempReal2;
      if( sumROC1 <= periodROC ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
      while( today <= startIdx ) {
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
         if( sumROC1 <= periodROC ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
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
         if( sumROC1 <= periodROC ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Kaufman Adaptive Moving Average: an EMA whose smoothing factor adapts each
    * bar to an efficiency ratio (directional move vs. total volatility). Reacts
    * fast in trends and smooths in ranging markets. Flat KAMA =
    * non-trending/ranging market. KAMA tracking price closely = efficient
    * trend.
    * <p><b>Formula</b>
    * <pre>{@code
    * ER = |price[t] - price[t-period]| / sum(|price[i]-price[i-1]|, last period bars)
    * SC = (ER*(2/3 - 2/31) + 2/31)^2
    * KAMA[t] = KAMA[t-1] + SC*(price[t] - KAMA[t-1])
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input, consistent with {@code MA(period=1)} for every MAType. (The natural KAMA math at period 1 would degenerate to a fixed-alpha EMA because the efficiency ratio is always 1, so the copy is made explicit.) Allowed since 0.6.5.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#KAMA_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series.
    * @param optInTimePeriod Lookback window for the efficiency ratio (default
    *        30; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Adaptive moving average line. Must hold at least
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
    * @see Core#MAMA
    * @see Core#EMA
    * @see Core#MA
    */
   public OutRange KAMA( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("KAMA", startIdx, endIdx);
      int guardStart = clampedStart("KAMA", startIdx, KAMA_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KAMA", "inReal", inReal, guardInLen);
      requireLength("KAMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = KAMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("KAMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Kaufman Adaptive Moving Average: an EMA whose smoothing factor adapts each
    * bar to an efficiency ratio (directional move vs. total volatility). Reacts
    * fast in trends and smooths in ranging markets. Flat KAMA =
    * non-trending/ranging market. KAMA tracking price closely = efficient
    * trend.
    * <p><b>Formula</b>
    * <pre>{@code
    * ER = |price[t] - price[t-period]| / sum(|price[i]-price[i-1]|, last period bars)
    * SC = (ER*(2/3 - 2/31) + 2/31)^2
    * KAMA[t] = KAMA[t-1] + SC*(price[t] - KAMA[t-1])
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input, consistent with {@code MA(period=1)} for every MAType. (The natural KAMA math at period 1 would degenerate to a fixed-alpha EMA because the efficiency ratio is always 1, so the copy is made explicit.) Allowed since 0.6.5.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#KAMA_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series.
    * @param optInTimePeriod Lookback window for the efficiency ratio (default
    *        30; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Adaptive moving average line. Must hold at least
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
    * @see Core#MAMA
    * @see Core#EMA
    * @see Core#MA
    */
   public OutRange KAMA( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("KAMA", startIdx, endIdx);
      int guardStart = clampedStart("KAMA", startIdx, KAMA_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KAMA", "inReal", inReal, guardInLen);
      requireLength("KAMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = KAMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("KAMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live KAMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#KAMA} over the same series.
    * Open with {@link Core#kamaOpen}; there is no close — the handle is
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
   public static final class KamaStream {
      Core core;
      int optInTimePeriod;
      double constMax;
      double constDiff;
      double sumROC1;
      double prevKAMA;
      int nullRun;
      double trailingValue;
      double lag1_inReal;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      KamaStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#KAMA} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      KamaStream( KamaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.constMax = other.constMax;
         this.constDiff = other.constDiff;
         this.sumROC1 = other.sumROC1;
         this.prevKAMA = other.prevKAMA;
         this.nullRun = other.nullRun;
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
            throw new TaLibArgumentException("KAMA update: BadParam", RetCode.BadParam);
         }
         core.kamaStepImpl(this, inReal);
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
         requireArgument("KAMA updateAndFill", "inReal", inReal);
         requireArgument("KAMA updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("KAMA updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("KAMA updateAndFill: BadParam", RetCode.BadParam);
            }
            core.kamaStepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("KAMA peek: BadParam", RetCode.BadParam);
         KamaStream sp = this;
         double tempReal = 0.0;
         double tempReal2 = 0.0;
         double periodROC = 0.0;
         double cur_outReal = 0.0;
         int nullRun = sp.nullRun;
         double prevKAMA = sp.prevKAMA;
         double sumROC1 = sp.sumROC1;
         double trailingValue = sp.trailingValue;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( sp.optInTimePeriod == 1 ) {
            cur_outReal = inReal;
            return cur_outReal ;
         }
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         tempReal = inReal;
         tempReal2 = (sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal0;
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
         /* Once a whole window of flat bars has gone by, every 1-day change it
          * spans is exactly zero, so the sum is known to be exactly zero and the
          * residue can be dropped. That is what lets the efficiency ratio be
          * decided by `sumROC1 <= periodROC` alone: a window that flat has
          * periodROC == 0 too, so the test is 0 <= 0 and the ratio is 1.
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
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, sp.constDiff, sp.constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal - prevKAMA, tempReal, prevKAMA);
         cur_outReal = prevKAMA;
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
      public KamaStream clone() {
         return new KamaStream(this);
      }
   }
   void kamaStepImpl( KamaStream sp, double inReal )
   {
      double tempReal = 0.0;
      double tempReal2 = 0.0;
      double periodROC = 0.0;
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      tempReal = inReal;
      tempReal2 = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      periodROC = tempReal - tempReal2;
      /* Adjust sumROC1:
       *  - Remove trailing ROC1
       *  - Add new ROC1
       */
      sp.sumROC1 -= Math.abs(sp.trailingValue - tempReal2);
      sp.sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
      /* Once a whole window of flat bars has gone by, every 1-day change it
       * spans is exactly zero, so the sum is known to be exactly zero and the
       * residue can be dropped. That is what lets the efficiency ratio be
       * decided by `sumROC1 <= periodROC` alone: a window that flat has
       * periodROC == 0 too, so the test is 0 <= 0 and the ratio is 1.
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
      /* Save the trailing value. Do this because inReal
       * and outReal can be pointers to the same buffer.
       */
      sp.trailingValue = tempReal2;
      /* Calculate the efficiency ratio */
      if( sp.sumROC1 <= periodROC ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sp.sumROC1);
      }
      /* Calculate the smoothing constant */
      tempReal = Math.fma(tempReal, sp.constDiff, sp.constMax);
      tempReal *= tempReal;
      /* Calculate the KAMA like an EMA, using the
       * smoothing constant as the adaptive factor.
       */
      sp.prevKAMA = Math.fma(inReal - sp.prevKAMA, tempReal, sp.prevKAMA);
      sp.cur_outReal = sp.prevKAMA;
      sp.lag1_inReal = inReal;
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode kamaOpenImpl( KamaStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      int nullRun = 0;
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
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( optInTimePeriod == 1 ) {
         int fillLb = KAMA_Lookback(optInTimePeriod);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.constMax = 0.0;
         sp.constDiff = 0.0;
         sp.sumROC1 = 0.0;
         sp.prevKAMA = 0.0;
         sp.nullRun = 0;
         sp.trailingValue = 0.0;
         sp.lag1_inReal = 0.0;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = 0;
         sp.ring_trailingIdx_inReal = new double[1];
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         if( outStride == 0 ) {
            outReal[0] = inReal[historyLen - 1];
         } else {
            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
               outReal[fillIdx] = inReal[fillLb + fillIdx];
            }
         }
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      /* Default return values */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.KAMA.ordinal()];
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Initialize the variables by going through
       * the lookback period.
       */
      sumROC1 = 0.0;
      /* Consecutive 1-day changes of exactly zero, counted so that a flat window
       * can be recognized exactly (the shape #244 needed for MFI). sumROC1 cannot
       * answer that question itself once the window starts sliding: it is
       * maintained by add-then-subtract, so a window that has gone flat leaves it
       * holding rounding residue of arbitrary sign rather than zero, and the
       * efficiency ratio then divides that residue into itself.
       */
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
      /* At this point sumROC1 represent the
       * summation of the 1-day price difference
       * over the (optInTimePeriod-1)
       */
      /* Calculate the first KAMA */
      /* The yesterday price is used here as the previous KAMA. */
      prevKAMA = inReal[today - 1];
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      /* Save the trailing value. Do this because inReal
       * and outReal can be pointers to the same buffer.
       */
      trailingValue = tempReal2;
      /* Calculate the efficiency ratio.
       *
       * The only threshold is `sumROC1 <= periodROC`, and it is scale-consistent:
       * both sides carry the quote unit. The fixed TA_IS_ZERO band that used to
       * sit beside it was not -- it declared the window flat, and forced the
       * fastest adaptation, for every window of an instrument quoted below it
       * (issue #253). A genuinely flat window is now recognized by the exact bar
       * count above instead.
       */
      if( sumROC1 <= periodROC ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      /* Calculate the smoothing constant */
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      /* Calculate the KAMA like an EMA, using the
       * smoothing constant as the adaptive factor.
       */
      prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      /* 'today' keep track of where the processing is within the
       * input.
       */
      /* Skip the unstable period. Do the whole processing
       * needed for KAMA, but do not write it in the output.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Once a whole window of flat bars has gone by, every 1-day change it
          * spans is exactly zero, so the sum is known to be exactly zero and the
          * residue can be dropped. That is what lets the efficiency ratio be
          * decided by `sumROC1 <= periodROC` alone: a window that flat has
          * periodROC == 0 too, so the test is 0 <= 0 and the ratio is 1.
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
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      /* Write the first value. */
      outReal[0 * outStride] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      /* Do the KAMA calculation for the requested range. */
      while( today <= endIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Once a whole window of flat bars has gone by, every 1-day change it
          * spans is exactly zero, so the sum is known to be exactly zero and the
          * residue can be dropped. That is what lets the efficiency ratio be
          * decided by `sumROC1 <= periodROC` alone: a window that flat has
          * periodROC == 0 too, so the test is 0 <= 0 and the ratio is 1.
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
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++ * outStride] = prevKAMA;
      }
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
      sp.constMax = constMax;
      sp.constDiff = constDiff;
      sp.sumROC1 = sumROC1;
      sp.prevKAMA = prevKAMA;
      sp.nullRun = nullRun;
      sp.trailingValue = trailingValue;
      sp.lag1_inReal = inReal[historyLen - 1];
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* kamaOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   KamaStream kamaOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      KamaStream sp = new KamaStream(this);
      RetCode retCode = kamaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("KAMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("KAMA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("KAMA openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind kamaOpen (composition seam). */
   KamaStream kamaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      KamaStream sp = new KamaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = kamaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("KAMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("KAMA open: internal error", retCode);
      }
      throw new TaLibArgumentException("KAMA open: " + retCode, retCode);
   }
   /**
    * Open a live KAMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#KAMA} at that bar.
    * <p>The history must hold at least {@code KAMA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public KamaStream kamaOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("KAMA open", "inReal", inReal);
      requireHistory("KAMA open", inReal.length);
      return kamaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#kamaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#KAMA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link KamaStream#outRange()}.
    */
   public KamaStream kamaOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("KAMA openAndFill", "inReal", inReal);
      requireHistory("KAMA openAndFill", inReal.length);
      int guardOutLen = openFillCount("KAMA openAndFill", inReal.length, KAMA_Lookback(optInTimePeriod));
      requireLength("KAMA openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("KAMA openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return kamaOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
