/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092926 MF,CC  First version (issue #474).
 */

   /**
    * Number of leading input bars {@link Core#vidya} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod The EMA length whose alpha the CMO scales (default
    *        12; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInCMOPeriod Number of trailing price changes in the CMO (default
    *        9; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int vidyaLookback( int optInTimePeriod, int optInCMOPeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 12;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInCMOPeriod == Integer.MIN_VALUE ) {
         optInCMOPeriod = 9;
      } else if( optInCMOPeriod < 2 || optInCMOPeriod > 100000 ) {
         return -1;
      }
      if( optInTimePeriod == 1 ) {
         return this.unstablePeriod[FuncUnstId.VIDYA.ordinal()] ;
      }
      return optInCMOPeriod + this.unstablePeriod[FuncUnstId.VIDYA.ordinal()] ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#vidya}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod The EMA length whose alpha the CMO scales (default
    *        12; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInCMOPeriod Number of trailing price changes in the CMO (default
    *        9; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int vidyaDisplayShift( int optInTimePeriod, int optInCMOPeriod, int outputIdx )
   {
      if( vidyaLookback( optInTimePeriod, optInCMOPeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode vidyaImpl( int startIdx,
                      int endIdx,
                      double inReal[],
                      int optInTimePeriod,
                      int optInCMOPeriod,
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
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      double alpha = 0;
      double k = 0;
      double prevVIDYA = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 12;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInCMOPeriod == Integer.MIN_VALUE ) {
         optInCMOPeriod = 9;
      } else if( optInCMOPeriod < 2 || optInCMOPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* No smoothing at period 1: the output is a copy of the input, as MA gives
       * for every MAType. The unstable period still delays the first output.
       */
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.VIDYA.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.SUCCESS ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.SUCCESS ;
      }
      lookbackTotal = vidyaLookback(optInTimePeriod, optInCMOPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      alpha = 2.0 / (double)(optInTimePeriod + 1);
      /* The CMO below is TA_CMOU's loop, spelling and nullRun reset included, so
       * that from the first full window on VIDYA equals the composite of TA_CMOU
       * bit for bit. The recursion is seeded on the window's first price and
       * steps through the warm-up with the CMO of the changes seen so far. The
       * unstable period enters it that many bars before startIdx.
       */
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = inReal[today];
      trailingValue = prevValue;
      prevVIDYA = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      nullRun = 0;
      for( i = 0; i < optInCMOPeriod; i += 1 ) {
         today += 1;
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
         } else {
            k = 0.0;
         }
         prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
      }
      today += 1;
      /* Skip the unstable period: the whole computation, nothing written. */
      while( today <= startIdx ) {
         tempReal = inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInCMOPeriod ) {
            nullRun = optInCMOPeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
         } else {
            k = 0.0;
         }
         prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
         today += 1;
      }
      outReal[0] = prevVIDYA;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInCMOPeriod ) {
            nullRun = optInCMOPeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
         } else {
            k = 0.0;
         }
         prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
         outReal[outIdx++] = prevVIDYA;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode vidyaImpl( int startIdx,
                      int endIdx,
                      float inReal[],
                      int optInTimePeriod,
                      int optInCMOPeriod,
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
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      double alpha = 0;
      double k = 0;
      double prevVIDYA = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 12;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInCMOPeriod == Integer.MIN_VALUE ) {
         optInCMOPeriod = 9;
      } else if( optInCMOPeriod < 2 || optInCMOPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.VIDYA.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.SUCCESS ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.SUCCESS ;
      }
      lookbackTotal = vidyaLookback(optInTimePeriod, optInCMOPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      alpha = 2.0 / (double)(optInTimePeriod + 1);
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = (double)inReal[today];
      trailingValue = prevValue;
      prevVIDYA = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      nullRun = 0;
      for( i = 0; i < optInCMOPeriod; i += 1 ) {
         today += 1;
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
         } else {
            k = 0.0;
         }
         prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
      }
      today += 1;
      while( today <= startIdx ) {
         tempReal = (double)inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInCMOPeriod ) {
            nullRun = optInCMOPeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
         } else {
            k = 0.0;
         }
         prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
         today += 1;
      }
      outReal[0] = prevVIDYA;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInCMOPeriod ) {
            nullRun = optInCMOPeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
         } else {
            k = 0.0;
         }
         prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
         outReal[outIdx++] = prevVIDYA;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Variable Index Dynamic Average (Tushar Chande): an EMA whose smoothing
    * factor is scaled every bar by the absolute value of the Chande Momentum
    * Oscillator. It follows the price like an EMA in a one-way move and stops
    * moving when up and down moves balance, so it flattens out in
    * consolidations.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/vidya">ta-lib.org/functions/vidya</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input, consistent with {@code MA(period=1)} for every MAType.</li>
    * <li>Chande's 1992 article drives the same step with a ratio of standard deviations; this is his 1995 form, driven by the CMO.</li>
    * <li>The CMO is Chande's unsmoothed one. An implementation driven by a Wilder-smoothed CMO ({@code CMO}) computes a different line that does not converge to this one.</li>
    * <li>Being recursive, an output depends on how much history precedes it, and the seed's influence decays more slowly the closer the CMO stays to 0. Implementations that seed differently agree with this one only once that influence has decayed.</li>
    * <li>As an {@code MA} type, the one period is n and the CMO period is (3n + 2) / 4 in integer division, Chande's 12:9 ratio.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#vidyaLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Data on which to compute the average.
    * @param optInTimePeriod The EMA length whose alpha the CMO scales (default
    *        12; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInCMOPeriod Number of trailing price changes in the CMO (default
    *        9; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Variable Index Dynamic Average line. Must hold at least
    *        {@code endIdx - max(startIdx, vidyaLookback(...)) + 1} values, the count
    *        the call produces (none when that is not positive).
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
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
    * @see Core#cmou
    * @see Core#ema
    * @see Core#kama
    * @see Core#ma
    */
   public OutRange vidya( int startIdx,
                          int endIdx,
                          double inReal[],
                          int optInTimePeriod,
                          int optInCMOPeriod,
                          double outReal[] )
   {
      requireIndexRange("VIDYA", startIdx, endIdx);
      int guardStart = clampedStart("VIDYA", startIdx, vidyaLookback(optInTimePeriod, optInCMOPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VIDYA", "inReal", inReal, guardInLen);
      requireLength("VIDYA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = vidyaImpl(startIdx, endIdx, inReal, optInTimePeriod, optInCMOPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("VIDYA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Variable Index Dynamic Average (Tushar Chande): an EMA whose smoothing
    * factor is scaled every bar by the absolute value of the Chande Momentum
    * Oscillator. It follows the price like an EMA in a one-way move and stops
    * moving when up and down moves balance, so it flattens out in
    * consolidations.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/vidya">ta-lib.org/functions/vidya</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input, consistent with {@code MA(period=1)} for every MAType.</li>
    * <li>Chande's 1992 article drives the same step with a ratio of standard deviations; this is his 1995 form, driven by the CMO.</li>
    * <li>The CMO is Chande's unsmoothed one. An implementation driven by a Wilder-smoothed CMO ({@code CMO}) computes a different line that does not converge to this one.</li>
    * <li>Being recursive, an output depends on how much history precedes it, and the seed's influence decays more slowly the closer the CMO stays to 0. Implementations that seed differently agree with this one only once that influence has decayed.</li>
    * <li>As an {@code MA} type, the one period is n and the CMO period is (3n + 2) / 4 in integer division, Chande's 12:9 ratio.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#vidyaLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Data on which to compute the average.
    * @param optInTimePeriod The EMA length whose alpha the CMO scales (default
    *        12; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInCMOPeriod Number of trailing price changes in the CMO (default
    *        9; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Variable Index Dynamic Average line. Must hold at least
    *        {@code endIdx - max(startIdx, vidyaLookback(...)) + 1} values, the count
    *        the call produces (none when that is not positive).
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
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
    * @see Core#cmou
    * @see Core#ema
    * @see Core#kama
    * @see Core#ma
    */
   public OutRange vidya( int startIdx,
                          int endIdx,
                          float inReal[],
                          int optInTimePeriod,
                          int optInCMOPeriod,
                          double outReal[] )
   {
      requireIndexRange("VIDYA", startIdx, endIdx);
      int guardStart = clampedStart("VIDYA", startIdx, vidyaLookback(optInTimePeriod, optInCMOPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VIDYA", "inReal", inReal, guardInLen);
      requireLength("VIDYA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = vidyaImpl(startIdx, endIdx, inReal, optInTimePeriod, optInCMOPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("VIDYA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live VIDYA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#vidya} over the same series.
    * Open with {@link Core#vidyaOpen}; there is no close — the handle is
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
   public static final class VidyaStream {
      private Core core;
      private int optInTimePeriod;
      private int optInCMOPeriod;
      private int nullRun;
      private double upSum;
      private double downSum;
      private double prevValue;
      private double trailingValue;
      private double alpha;
      private double prevVIDYA;
      private int ringPos_trailingIdx;
      private int ringCap_trailingIdx;
      private double[] ring_trailingIdx_inReal;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private VidyaStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#vidya} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("VIDYA advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private VidyaStream( VidyaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInCMOPeriod = other.optInCMOPeriod;
         this.nullRun = other.nullRun;
         this.upSum = other.upSum;
         this.downSum = other.downSum;
         this.prevValue = other.prevValue;
         this.trailingValue = other.trailingValue;
         this.alpha = other.alpha;
         this.prevVIDYA = other.prevVIDYA;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
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
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("VIDYA update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("VIDYA update", "inReal");
         core.vidyaStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("VIDYA peek", "inReal");
         VidyaStream sp = this;
         double sum = 0.0;
         double diff = 0.0;
         double tempReal = 0.0;
         double k = 0.0;
         double cur_outReal = 0.0;
         double downSum = sp.downSum;
         int nullRun = sp.nullRun;
         double prevVIDYA = sp.prevVIDYA;
         double prevValue = sp.prevValue;
         double trailingValue = sp.trailingValue;
         double upSum = sp.upSum;
         if( sp.optInTimePeriod == 1 ) {
            cur_outReal = inReal;
            return cur_outReal ;
         }
         tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = inReal;
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= sp.optInCMOPeriod ) {
            nullRun = sp.optInCMOPeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            k = sp.alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
         } else {
            k = 0.0;
         }
         prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
         cur_outReal = prevVIDYA;
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
      public VidyaStream clone() {
         return new VidyaStream(this);
      }
   }
   private void vidyaStepImpl( VidyaStream sp, double inReal )
   {
      double sum = 0.0;
      double diff = 0.0;
      double tempReal = 0.0;
      double k = 0.0;
      int ringCapL_trailingIdx = 0;
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      diff = tempReal - sp.trailingValue;
      sp.trailingValue = tempReal;
      if( diff > 0.0 ) {
         sp.upSum -= diff;
      } else if( diff < 0.0 ) {
         sp.downSum += diff;
      }
      tempReal = inReal;
      diff = tempReal - sp.prevValue;
      sp.prevValue = tempReal;
      if( diff > 0.0 ) {
         sp.upSum += diff;
      } else if( diff < 0.0 ) {
         sp.downSum -= diff;
      }
      if( diff == 0.0 ) {
         sp.nullRun += 1;
      } else {
         sp.nullRun = 0;
      }
      if( sp.nullRun >= sp.optInCMOPeriod ) {
         sp.nullRun = sp.optInCMOPeriod;
         sp.upSum = 0.0;
         sp.downSum = 0.0;
      }
      sum = sp.upSum + sp.downSum;
      if( sum > 0.0 ) {
         k = sp.alpha * (Math.abs(100.0 * (sp.upSum - sp.downSum) / sum) / 100.0);
      } else {
         k = 0.0;
      }
      sp.prevVIDYA = Math.fma(sp.prevValue - sp.prevVIDYA, k, sp.prevVIDYA);
      sp.cur_outReal = sp.prevVIDYA;
      ringCapL_trailingIdx = sp.ringCap_trailingIdx;
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= ringCapL_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode vidyaOpenImpl( VidyaStream sp, double inReal[], int startIdx, int optInTimePeriod, int optInCMOPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int nullRun = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      double alpha = 0;
      double k = 0;
      double prevVIDYA = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 12;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInCMOPeriod == Integer.MIN_VALUE ) {
         optInCMOPeriod = 9;
      } else if( optInCMOPeriod < 2 || optInCMOPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( optInTimePeriod == 1 ) {
         int fillLb = vidyaLookback(optInTimePeriod, optInCMOPeriod);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.INSUFFICIENT_HISTORY;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInCMOPeriod = optInCMOPeriod;
         sp.nullRun = 0;
         sp.upSum = 0.0;
         sp.downSum = 0.0;
         sp.prevValue = 0.0;
         sp.trailingValue = 0.0;
         sp.alpha = 0.0;
         sp.prevVIDYA = 0.0;
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
         return RetCode.SUCCESS;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = vidyaLookback(optInTimePeriod, optInCMOPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      alpha = 2.0 / (double)(optInTimePeriod + 1);
      /* The CMO below is TA_CMOU's loop, spelling and nullRun reset included, so
       * that from the first full window on VIDYA equals the composite of TA_CMOU
       * bit for bit. The recursion is seeded on the window's first price and
       * steps through the warm-up with the CMO of the changes seen so far. The
       * unstable period enters it that many bars before startIdx.
       */
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = inReal[today];
      trailingValue = prevValue;
      prevVIDYA = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      nullRun = 0;
      for( i = 0; i < optInCMOPeriod; i += 1 ) {
         today += 1;
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
         } else {
            k = 0.0;
         }
         prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
      }
      today += 1;
      /* Skip the unstable period: the whole computation, nothing written. */
      while( today <= startIdx ) {
         tempReal = inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInCMOPeriod ) {
            nullRun = optInCMOPeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
         } else {
            k = 0.0;
         }
         prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
         today += 1;
      }
      outReal[0 * outStride] = prevVIDYA;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInCMOPeriod ) {
            nullRun = optInCMOPeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            k = alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
         } else {
            k = 0.0;
         }
         prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
         outReal[outIdx++ * outStride] = prevVIDYA;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 1 || cap_trailingIdx > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInCMOPeriod = optInCMOPeriod;
      sp.nullRun = nullRun;
      sp.upSum = upSum;
      sp.downSum = downSum;
      sp.prevValue = prevValue;
      sp.trailingValue = trailingValue;
      sp.alpha = alpha;
      sp.prevVIDYA = prevVIDYA;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* vidyaOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   VidyaStream vidyaOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, int optInCMOPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      VidyaStream sp = new VidyaStream(this);
      RetCode retCode = vidyaOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInCMOPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("VIDYA openAndFill", inReal.length, startIdx, vidyaLookback(optInTimePeriod, optInCMOPeriod));
      }
      throw streamFailure("VIDYA openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind vidyaOpen (composition seam). */
   VidyaStream vidyaOpenInternal( double inReal[], int startIdx, int optInTimePeriod, int optInCMOPeriod )
   {
      VidyaStream sp = new VidyaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = vidyaOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInCMOPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("VIDYA open", inReal.length, startIdx, vidyaLookback(optInTimePeriod, optInCMOPeriod));
      }
      throw streamFailure("VIDYA open", retCode);
   }
   /**
    * Open a live VIDYA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#vidya} at that bar.
    * <p>The history must hold at least {@code vidyaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public VidyaStream vidyaOpen( double inReal[], int optInTimePeriod, int optInCMOPeriod )
   {
      requireArgument("VIDYA open", "inReal", inReal);
      requireHistory("VIDYA open", inReal.length);
      return vidyaOpenInternal(inReal, 0, optInTimePeriod, optInCMOPeriod);
   }
   /**
    * {@link Core#vidyaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#vidya} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link VidyaStream#outRange()}.
    */
   public VidyaStream vidyaOpenAndFill( double inReal[], int optInTimePeriod, int optInCMOPeriod, double outReal[] )
   {
      requireArgument("VIDYA openAndFill", "inReal", inReal);
      requireHistory("VIDYA openAndFill", inReal.length);
      int guardOutLen = openFillCount("VIDYA openAndFill", inReal.length, vidyaLookback(optInTimePeriod, optInCMOPeriod));
      requireLength("VIDYA openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("VIDYA openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return vidyaOpenAndFillInternal(inReal, 0, optInTimePeriod, optInCMOPeriod, outBegIdx, outNBElement, outReal);
   }
   private double vidyaStepTape( VidyaStream sp, double[] tape, int tapeBase, int tapeMask, double inReal )
   {
      double sum = 0.0;
      double diff = 0.0;
      double tempReal = 0.0;
      double k = 0.0;
      tempReal = tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask];
      diff = tempReal - sp.trailingValue;
      sp.trailingValue = tempReal;
      if( diff > 0.0 ) {
         sp.upSum -= diff;
      } else if( diff < 0.0 ) {
         sp.downSum += diff;
      }
      tempReal = inReal;
      diff = tempReal - sp.prevValue;
      sp.prevValue = tempReal;
      if( diff > 0.0 ) {
         sp.upSum += diff;
      } else if( diff < 0.0 ) {
         sp.downSum -= diff;
      }
      if( diff == 0.0 ) {
         sp.nullRun += 1;
      } else {
         sp.nullRun = 0;
      }
      if( sp.nullRun >= sp.optInCMOPeriod ) {
         sp.nullRun = sp.optInCMOPeriod;
         sp.upSum = 0.0;
         sp.downSum = 0.0;
      }
      sum = sp.upSum + sp.downSum;
      if( sum > 0.0 ) {
         k = sp.alpha * (Math.abs(100.0 * (sp.upSum - sp.downSum) / sum) / 100.0);
      } else {
         k = 0.0;
      }
      sp.prevVIDYA = Math.fma(sp.prevValue - sp.prevVIDYA, k, sp.prevVIDYA);
      sp.cur_outReal = sp.prevVIDYA;
      sp.outRangeCount++;
      return sp.cur_outReal;
   }
   private double vidyaPeekTape( VidyaStream sp, double[] tape, int tapeBase, int tapeMask, double inReal )
   {
      double sum = 0.0;
      double diff = 0.0;
      double tempReal = 0.0;
      double k = 0.0;
      double cur_outReal = 0.0;
      double downSum = sp.downSum;
      int nullRun = sp.nullRun;
      double prevVIDYA = sp.prevVIDYA;
      double prevValue = sp.prevValue;
      double trailingValue = sp.trailingValue;
      double upSum = sp.upSum;
      tempReal = tape[(tapeBase - sp.ringCap_trailingIdx) & tapeMask];
      diff = tempReal - trailingValue;
      trailingValue = tempReal;
      if( diff > 0.0 ) {
         upSum -= diff;
      } else if( diff < 0.0 ) {
         downSum += diff;
      }
      tempReal = inReal;
      diff = tempReal - prevValue;
      prevValue = tempReal;
      if( diff > 0.0 ) {
         upSum += diff;
      } else if( diff < 0.0 ) {
         downSum -= diff;
      }
      if( diff == 0.0 ) {
         nullRun += 1;
      } else {
         nullRun = 0;
      }
      if( nullRun >= sp.optInCMOPeriod ) {
         nullRun = sp.optInCMOPeriod;
         upSum = 0.0;
         downSum = 0.0;
      }
      sum = upSum + downSum;
      if( sum > 0.0 ) {
         k = sp.alpha * (Math.abs(100.0 * (upSum - downSum) / sum) / 100.0);
      } else {
         k = 0.0;
      }
      prevVIDYA = Math.fma(prevValue - prevVIDYA, k, prevVIDYA);
      cur_outReal = prevVIDYA;
      return cur_outReal;
   }
   private int vidyaTapeDetach( VidyaStream sp )
   {
      int reach = 0;
      sp.ring_trailingIdx_inReal = new double[0];
      if( sp.ringCap_trailingIdx > reach ) {
         reach = sp.ringCap_trailingIdx;
      }
      return reach;
   }
