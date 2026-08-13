/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  102404 AC   Creation
 *  040309 AC   Increased flexibility to allow real bodies matching
 *              on one end (Greg Morris - "Candlestick charting explained")
 */

   /**
    * Number of leading input bars {@link Core#CDLHARAMI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLHARAMI_Lookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      return Math.max(BodyShort_avgPeriod, BodyLong_avgPeriod) + 1 ;

   }
   RetCode CDLHARAMI_Internal( int startIdx,
                               int endIdx,
                               double inOpen[],
                               double inHigh[],
                               double inLow[],
                               double inClose[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               int outInteger[] )
   {
      double BodyShortPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyShortTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLHARAMI_Lookback();
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
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      BodyLongPeriodTotal = 0;
      BodyShortPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long white (black) real body
       * - second candle: short real body totally engulfed by the first
       * The meaning of "short" and "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish:
       * - 100 is returned when the first candle's real body begins before and ends after the second candle's real body
       * - 80 is returned when the two real bodies match on one end (Greg Morris contemplate this case in his book
       *   "Candlestick charting explained")
       * The user should consider that a harami is significant when it appears in a downtrend if bullish or
       * in an uptrend when bearish, while this function does not consider the trend
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            /* 1st: long */
            if( Math.abs(inClose[i] - inOpen[i]) <= ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
               /* 2nd: short */
               /* 2nd is engulfed by 1st */
               if( Math.max(inClose[i], inOpen[i]) < Math.max(inClose[i - 1], inOpen[i - 1]) && Math.min(inClose[i], inOpen[i]) > Math.min(inClose[i - 1], inOpen[i - 1]) ) {
                  outInteger[outIdx++] = (0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1)) * 100;
                  /* 2nd is engulfed by 1st
                   * (one end of real body can match;
                   * engulfing guaranteed by "long" and "short")
                   */
               } else if( Math.max(inClose[i], inOpen[i]) <= Math.max(inClose[i - 1], inOpen[i - 1]) && Math.min(inClose[i], inOpen[i]) >= Math.min(inClose[i - 1], inOpen[i - 1]) ) {
                  outInteger[outIdx++] = (0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1)) * 80;
               } else {
                  outInteger[outIdx++] = 0;
               }
            } else {
               outInteger[outIdx++] = 0;
            }
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) - Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLHARAMI_Internal( int startIdx,
                               int endIdx,
                               float inOpen[],
                               float inHigh[],
                               float inLow[],
                               float inClose[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               int outInteger[] )
   {
      double BodyShortPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyShortTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLHARAMI_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal = 0;
      BodyShortPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            if( Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
               if( Math.max((double)inClose[i], (double)inOpen[i]) < Math.max((double)inClose[i - 1], (double)inOpen[i - 1]) && Math.min((double)inClose[i], (double)inOpen[i]) > Math.min((double)inClose[i - 1], (double)inOpen[i - 1]) ) {
                  outInteger[outIdx++] = (0 - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1)) * 100;
               } else if( Math.max((double)inClose[i], (double)inOpen[i]) <= Math.max((double)inClose[i - 1], (double)inOpen[i - 1]) && Math.min((double)inClose[i], (double)inOpen[i]) >= Math.min((double)inClose[i - 1], (double)inOpen[i - 1]) ) {
                  outInteger[outIdx++] = (0 - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1)) * 80;
               } else {
                  outInteger[outIdx++] = 0;
               }
            } else {
               outInteger[outIdx++] = 0;
            }
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) - Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) - Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Two-candle pattern: a long real body followed by a short real body
    * contained within the first candle's real body. A reversal signal whose
    * direction is the opposite of the first candle's color. A hit signals a
    * potential reversal opposite the 1st candle: positive (black 1st) is
    * bullish, negative (white 1st) is bearish.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that the reversal signal assumes.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHARAMI_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100/+80 when the long 1st candle is black (bullish),
    *        -100/-80 when it is white (bearish), 0 otherwise; 80 when the two real
    *        bodies share an end, 100 when the 1st body strictly overhangs both ends of
    *        the 2nd. Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLHARAMICROSS
    * @see Core#CDLENGULFING
    */
   public OutRange CDLHARAMI( int startIdx,
                              int endIdx,
                              double inOpen[],
                              double inHigh[],
                              double inLow[],
                              double inClose[],
                              int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHARAMI_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHARAMI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Two-candle pattern: a long real body followed by a short real body
    * contained within the first candle's real body. A reversal signal whose
    * direction is the opposite of the first candle's color. A hit signals a
    * potential reversal opposite the 1st candle: positive (black 1st) is
    * bullish, negative (white 1st) is bearish.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that the reversal signal assumes.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHARAMI_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100/+80 when the long 1st candle is black (bullish),
    *        -100/-80 when it is white (bearish), 0 otherwise; 80 when the two real
    *        bodies share an end, 100 when the 1st body strictly overhangs both ends of
    *        the 2nd. Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLHARAMICROSS
    * @see Core#CDLENGULFING
    */
   public OutRange CDLHARAMI( int startIdx,
                              int endIdx,
                              float inOpen[],
                              float inHigh[],
                              float inLow[],
                              float inClose[],
                              int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHARAMI_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHARAMI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLHARAMI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLHARAMI} over the same series.
    * Open with {@link Core#CDLHARAMI_Open}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class CDLHARAMI_Stream {
      Core core;
      double BodyShortPeriodTotal;
      double BodyLongPeriodTotal;
      double lag1_inOpen;
      double lag1_inHigh;
      double lag1_inLow;
      double lag1_inClose;
      int ringPos_BodyLongTrailingIdx;
      int ringCap_BodyLongTrailingIdx;
      double[] ring_BodyLongTrailingIdx_inOpen;
      double[] ring_BodyLongTrailingIdx_inHigh;
      double[] ring_BodyLongTrailingIdx_inLow;
      double[] ring_BodyLongTrailingIdx_inClose;
      int ringPos_BodyShortTrailingIdx;
      int ringCap_BodyShortTrailingIdx;
      double[] ring_BodyShortTrailingIdx_inOpen;
      double[] ring_BodyShortTrailingIdx_inHigh;
      double[] ring_BodyShortTrailingIdx_inLow;
      double[] ring_BodyShortTrailingIdx_inClose;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cs_BodyShort_rangeType;
      int cs_BodyShort_avgPeriod;
      double cs_BodyShort_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLHARAMI_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLHARAMI_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLHARAMI_Stream( CDLHARAMI_Stream other ) {
         this.core = other.core;
         this.BodyShortPeriodTotal = other.BodyShortPeriodTotal;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ring_BodyLongTrailingIdx_inOpen = other.ring_BodyLongTrailingIdx_inOpen.clone();
         this.ring_BodyLongTrailingIdx_inHigh = other.ring_BodyLongTrailingIdx_inHigh.clone();
         this.ring_BodyLongTrailingIdx_inLow = other.ring_BodyLongTrailingIdx_inLow.clone();
         this.ring_BodyLongTrailingIdx_inClose = other.ring_BodyLongTrailingIdx_inClose.clone();
         this.ringPos_BodyShortTrailingIdx = other.ringPos_BodyShortTrailingIdx;
         this.ringCap_BodyShortTrailingIdx = other.ringCap_BodyShortTrailingIdx;
         this.ring_BodyShortTrailingIdx_inOpen = other.ring_BodyShortTrailingIdx_inOpen.clone();
         this.ring_BodyShortTrailingIdx_inHigh = other.ring_BodyShortTrailingIdx_inHigh.clone();
         this.ring_BodyShortTrailingIdx_inLow = other.ring_BodyShortTrailingIdx_inLow.clone();
         this.ring_BodyShortTrailingIdx_inClose = other.ring_BodyShortTrailingIdx_inClose.clone();
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDLHARAMI_Stream other ) {
         this.core = other.core;
         this.BodyShortPeriodTotal = other.BodyShortPeriodTotal;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         if( this.ring_BodyLongTrailingIdx_inOpen != null && this.ring_BodyLongTrailingIdx_inOpen.length == other.ring_BodyLongTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_inOpen, 0, this.ring_BodyLongTrailingIdx_inOpen, 0, other.ring_BodyLongTrailingIdx_inOpen.length );
         } else {
            this.ring_BodyLongTrailingIdx_inOpen = other.ring_BodyLongTrailingIdx_inOpen.clone();
         }
         if( this.ring_BodyLongTrailingIdx_inHigh != null && this.ring_BodyLongTrailingIdx_inHigh.length == other.ring_BodyLongTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_inHigh, 0, this.ring_BodyLongTrailingIdx_inHigh, 0, other.ring_BodyLongTrailingIdx_inHigh.length );
         } else {
            this.ring_BodyLongTrailingIdx_inHigh = other.ring_BodyLongTrailingIdx_inHigh.clone();
         }
         if( this.ring_BodyLongTrailingIdx_inLow != null && this.ring_BodyLongTrailingIdx_inLow.length == other.ring_BodyLongTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_inLow, 0, this.ring_BodyLongTrailingIdx_inLow, 0, other.ring_BodyLongTrailingIdx_inLow.length );
         } else {
            this.ring_BodyLongTrailingIdx_inLow = other.ring_BodyLongTrailingIdx_inLow.clone();
         }
         if( this.ring_BodyLongTrailingIdx_inClose != null && this.ring_BodyLongTrailingIdx_inClose.length == other.ring_BodyLongTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_inClose, 0, this.ring_BodyLongTrailingIdx_inClose, 0, other.ring_BodyLongTrailingIdx_inClose.length );
         } else {
            this.ring_BodyLongTrailingIdx_inClose = other.ring_BodyLongTrailingIdx_inClose.clone();
         }
         this.ringPos_BodyShortTrailingIdx = other.ringPos_BodyShortTrailingIdx;
         this.ringCap_BodyShortTrailingIdx = other.ringCap_BodyShortTrailingIdx;
         if( this.ring_BodyShortTrailingIdx_inOpen != null && this.ring_BodyShortTrailingIdx_inOpen.length == other.ring_BodyShortTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_BodyShortTrailingIdx_inOpen, 0, this.ring_BodyShortTrailingIdx_inOpen, 0, other.ring_BodyShortTrailingIdx_inOpen.length );
         } else {
            this.ring_BodyShortTrailingIdx_inOpen = other.ring_BodyShortTrailingIdx_inOpen.clone();
         }
         if( this.ring_BodyShortTrailingIdx_inHigh != null && this.ring_BodyShortTrailingIdx_inHigh.length == other.ring_BodyShortTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_BodyShortTrailingIdx_inHigh, 0, this.ring_BodyShortTrailingIdx_inHigh, 0, other.ring_BodyShortTrailingIdx_inHigh.length );
         } else {
            this.ring_BodyShortTrailingIdx_inHigh = other.ring_BodyShortTrailingIdx_inHigh.clone();
         }
         if( this.ring_BodyShortTrailingIdx_inLow != null && this.ring_BodyShortTrailingIdx_inLow.length == other.ring_BodyShortTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_BodyShortTrailingIdx_inLow, 0, this.ring_BodyShortTrailingIdx_inLow, 0, other.ring_BodyShortTrailingIdx_inLow.length );
         } else {
            this.ring_BodyShortTrailingIdx_inLow = other.ring_BodyShortTrailingIdx_inLow.clone();
         }
         if( this.ring_BodyShortTrailingIdx_inClose != null && this.ring_BodyShortTrailingIdx_inClose.length == other.ring_BodyShortTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_BodyShortTrailingIdx_inClose, 0, this.ring_BodyShortTrailingIdx_inClose, 0, other.ring_BodyShortTrailingIdx_inClose.length );
         } else {
            this.ring_BodyShortTrailingIdx_inClose = other.ring_BodyShortTrailingIdx_inClose.clone();
         }
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDLHARAMI_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.CDLHARAMI_StreamStep(this, inOpen, inHigh, inLow, inClose);
         return this.cur_outInteger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a scratch handle held per thread and
       * reused, so the copy allocates nothing after the first peek of this
       * indicator on this thread. That scratch is retained for the life of
       * the thread.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         CDLHARAMI_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLHARAMI_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLHARAMI_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
         return scratch.cur_outInteger;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public int value() {
         return this.cur_outInteger;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public CDLHARAMI_Stream copy() {
         return new CDLHARAMI_Stream(this);
      }
   }
   void CDLHARAMI_StreamStep( CDLHARAMI_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      int BodyShort_rangeType = sp.cs_BodyShort_rangeType;
      int BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
      double BodyShort_factor = sp.cs_BodyShort_factor;
      if( sp.ringCap_BodyLongTrailingIdx == 0 ) {
         sp.ring_BodyLongTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyLongTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyLongTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyLongTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_BodyShortTrailingIdx == 0 ) {
         sp.ring_BodyShortTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyShortTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyShortTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyShortTrailingIdx_inClose[0] = inClose;
      }
      if( Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
         /* 1st: long */
         if( Math.abs(inClose - inOpen) <= ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            /* 2nd: short */
            /* 2nd is engulfed by 1st */
            if( Math.max(inClose, inOpen) < Math.max(sp.lag1_inClose, sp.lag1_inOpen) && Math.min(inClose, inOpen) > Math.min(sp.lag1_inClose, sp.lag1_inOpen) ) {
               sp.cur_outInteger = (0 - ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1)) * 100;
               /* 2nd is engulfed by 1st
                * (one end of real body can match;
                * engulfing guaranteed by "long" and "short")
                */
            } else if( Math.max(inClose, inOpen) <= Math.max(sp.lag1_inClose, sp.lag1_inOpen) && Math.min(inClose, inOpen) >= Math.min(sp.lag1_inClose, sp.lag1_inOpen) ) {
               sp.cur_outInteger = (0 - ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1)) * 80;
            } else {
               sp.cur_outInteger = 0;
            }
         } else {
            sp.cur_outInteger = 0;
         }
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx]) - Math.abs(sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx])) : 0.0)));
      sp.BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(sp.ring_BodyShortTrailingIdx_inClose[sp.ringPos_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inOpen[sp.ringPos_BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (sp.ring_BodyShortTrailingIdx_inHigh[sp.ringPos_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inLow[sp.ringPos_BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((sp.ring_BodyShortTrailingIdx_inHigh[sp.ringPos_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inLow[sp.ringPos_BodyShortTrailingIdx]) - Math.abs(sp.ring_BodyShortTrailingIdx_inClose[sp.ringPos_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inOpen[sp.ringPos_BodyShortTrailingIdx])) : 0.0)));
      sp.lag1_inOpen = inOpen;
      sp.lag1_inHigh = inHigh;
      sp.lag1_inLow = inLow;
      sp.lag1_inClose = inClose;
      sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx] = inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] = inHigh;
      sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx] = inLow;
      sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] = inClose;
      sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
      if( sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx ) {
         sp.ringPos_BodyLongTrailingIdx = 0;
      }
      sp.ring_BodyShortTrailingIdx_inOpen[sp.ringPos_BodyShortTrailingIdx] = inOpen;
      sp.ring_BodyShortTrailingIdx_inHigh[sp.ringPos_BodyShortTrailingIdx] = inHigh;
      sp.ring_BodyShortTrailingIdx_inLow[sp.ringPos_BodyShortTrailingIdx] = inLow;
      sp.ring_BodyShortTrailingIdx_inClose[sp.ringPos_BodyShortTrailingIdx] = inClose;
      sp.ringPos_BodyShortTrailingIdx = sp.ringPos_BodyShortTrailingIdx + 1;
      if( sp.ringPos_BodyShortTrailingIdx >= sp.ringCap_BodyShortTrailingIdx ) {
         sp.ringPos_BodyShortTrailingIdx = 0;
      }
   }
   private RetCode CDLHARAMI_OpenCore( CDLHARAMI_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double BodyShortPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyShortTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLHARAMI_Lookback();
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
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      BodyLongPeriodTotal = 0;
      BodyShortPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long white (black) real body
       * - second candle: short real body totally engulfed by the first
       * The meaning of "short" and "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish:
       * - 100 is returned when the first candle's real body begins before and ends after the second candle's real body
       * - 80 is returned when the two real bodies match on one end (Greg Morris contemplate this case in his book
       *   "Candlestick charting explained")
       * The user should consider that a harami is significant when it appears in a downtrend if bullish or
       * in an uptrend when bearish, while this function does not consider the trend
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            /* 1st: long */
            if( Math.abs(inClose[i] - inOpen[i]) <= ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
               /* 2nd: short */
               /* 2nd is engulfed by 1st */
               if( Math.max(inClose[i], inOpen[i]) < Math.max(inClose[i - 1], inOpen[i - 1]) && Math.min(inClose[i], inOpen[i]) > Math.min(inClose[i - 1], inOpen[i - 1]) ) {
                  outInteger[outIdx++ * outStride] = (0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1)) * 100;
                  /* 2nd is engulfed by 1st
                   * (one end of real body can match;
                   * engulfing guaranteed by "long" and "short")
                   */
               } else if( Math.max(inClose[i], inOpen[i]) <= Math.max(inClose[i - 1], inOpen[i - 1]) && Math.min(inClose[i], inOpen[i]) >= Math.min(inClose[i - 1], inOpen[i - 1]) ) {
                  outInteger[outIdx++ * outStride] = (0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1)) * 80;
               } else {
                  outInteger[outIdx++ * outStride] = 0;
               }
            } else {
               outInteger[outIdx++ * outStride] = 0;
            }
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) - Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      if( cap_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyLongTrailingIdx = (cap_BodyLongTrailingIdx > 0)? cap_BodyLongTrailingIdx : 1;
      double[] capRing_BodyLongTrailingIdx_inOpen = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inOpen, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inHigh = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inHigh, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inLow = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inLow, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inClose = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inClose, 0, cap_BodyLongTrailingIdx);
      int cap_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
      if( cap_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyShortTrailingIdx = (cap_BodyShortTrailingIdx > 0)? cap_BodyShortTrailingIdx : 1;
      double[] capRing_BodyShortTrailingIdx_inOpen = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inOpen, 0, cap_BodyShortTrailingIdx);
      double[] capRing_BodyShortTrailingIdx_inHigh = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inHigh, 0, cap_BodyShortTrailingIdx);
      double[] capRing_BodyShortTrailingIdx_inLow = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inLow, 0, cap_BodyShortTrailingIdx);
      double[] capRing_BodyShortTrailingIdx_inClose = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inClose, 0, cap_BodyShortTrailingIdx);
      sp.BodyShortPeriodTotal = BodyShortPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_BodyLongTrailingIdx = 0;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_inOpen = capRing_BodyLongTrailingIdx_inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh = capRing_BodyLongTrailingIdx_inHigh;
      sp.ring_BodyLongTrailingIdx_inLow = capRing_BodyLongTrailingIdx_inLow;
      sp.ring_BodyLongTrailingIdx_inClose = capRing_BodyLongTrailingIdx_inClose;
      sp.ringPos_BodyShortTrailingIdx = 0;
      sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
      sp.ring_BodyShortTrailingIdx_inOpen = capRing_BodyShortTrailingIdx_inOpen;
      sp.ring_BodyShortTrailingIdx_inHigh = capRing_BodyShortTrailingIdx_inHigh;
      sp.ring_BodyShortTrailingIdx_inLow = capRing_BodyShortTrailingIdx_inLow;
      sp.ring_BodyShortTrailingIdx_inClose = capRing_BodyShortTrailingIdx_inClose;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CDLHARAMI_OpenBody( CDLHARAMI_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDLHARAMI_OpenCore( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDLHARAMI_OpenAndFillBody( CDLHARAMI_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDLHARAMI_OpenCore( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDLHARAMI_OpenAndFillInternalBody( CDLHARAMI_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDLHARAMI_OpenCore(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDLHARAMI_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLHARAMI_Stream CDLHARAMI_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLHARAMI_Stream sp = new CDLHARAMI_Stream(this);
      RetCode retCode = CDLHARAMI_OpenAndFillInternalBody(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLHARAMI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLHARAMI openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLHARAMI openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind CDLHARAMI_Open (composition seam). */
   CDLHARAMI_Stream CDLHARAMI_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLHARAMI_Stream sp = new CDLHARAMI_Stream(this);
      RetCode retCode = CDLHARAMI_OpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLHARAMI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLHARAMI open: internal error");
      }
      throw new IllegalArgumentException("CDLHARAMI open: " + retCode);
   }
   /**
    * Open a live CDLHARAMI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLHARAMI} at that bar.
    * <p>The history must hold at least {@code CDLHARAMI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLHARAMI_Stream CDLHARAMI_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLHARAMI_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLHARAMI_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLHARAMI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLHARAMI_Stream#fillRange()}.
    */
   public CDLHARAMI_Stream CDLHARAMI_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLHARAMI_Stream sp = new CDLHARAMI_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHARAMI_OpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLHARAMI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLHARAMI openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLHARAMI openAndFill: " + retCode);
   }
