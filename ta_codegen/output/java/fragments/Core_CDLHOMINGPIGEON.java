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
 *  032005 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLHOMINGPIGEON} consumes before
    * it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLHOMINGPIGEON_Lookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      return Math.max(BodyShort_avgPeriod, BodyLong_avgPeriod) + 1 ;

   }
   RetCode CDLHOMINGPIGEON_Impl( int startIdx,
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
      lookbackTotal = CDLHOMINGPIGEON_Lookback();
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
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black candle
       * - second candle: short black real body completely inside the previous day's body
       * The meaning of "short" and "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100): homing pigeon is always bullish;
       * the user should consider that homing pigeon is significant when it appears in a downtrend,
       * while this function does not consider the trend
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* 2nd black */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
             Math.abs(inClose[i] - inOpen[i]) <= ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd short */
             inOpen[i] < inOpen[i - 1] &&                                /* 2nd engulfed by 1st */
             inClose[i] > inClose[i - 1] )
         {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 1] - (((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inClose[BodyLongTrailingIdx - 1]) : (inOpen[BodyLongTrailingIdx - 1]))) + ((((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inOpen[BodyLongTrailingIdx - 1]) : (inClose[BodyLongTrailingIdx - 1])) - inLow[BodyLongTrailingIdx - 1])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLHOMINGPIGEON_Impl( int startIdx,
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
      lookbackTotal = CDLHOMINGPIGEON_Lookback();
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
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] < (double)inOpen[i - 1] && (double)inClose[i] > (double)inClose[i - 1] ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 1] - (double)inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 1] - (double)inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 1] - ((((double)inClose[BodyLongTrailingIdx - 1]) >= ((double)inOpen[BodyLongTrailingIdx - 1])) ? ((double)inClose[BodyLongTrailingIdx - 1]) : ((double)inOpen[BodyLongTrailingIdx - 1]))) + (((((double)inClose[BodyLongTrailingIdx - 1]) >= ((double)inOpen[BodyLongTrailingIdx - 1])) ? ((double)inOpen[BodyLongTrailingIdx - 1]) : ((double)inClose[BodyLongTrailingIdx - 1])) - (double)inLow[BodyLongTrailingIdx - 1])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx] - ((((double)inClose[BodyShortTrailingIdx]) >= ((double)inOpen[BodyShortTrailingIdx])) ? ((double)inClose[BodyShortTrailingIdx]) : ((double)inOpen[BodyShortTrailingIdx]))) + (((((double)inClose[BodyShortTrailingIdx]) >= ((double)inOpen[BodyShortTrailingIdx])) ? ((double)inOpen[BodyShortTrailingIdx]) : ((double)inClose[BodyShortTrailingIdx])) - (double)inLow[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Two-candle pattern: a long black candle followed by a small black candle
    * whose real body sits inside the prior body. A hit signals a bullish
    * reversal, most meaningful in a downtrend, which the code does not verify.
    * <p><b>Formula</b>
    * <pre>{@code
    * Two candles at i-1 and i. Both black: close[i-1] < open[i-1] and close[i] < open[i]. First body long: realbody[i-1] > BodyLong average. Second body short: realbody[i] <= BodyShort average. Second body contained by first: open[i] < open[i-1] and close[i] > close[i-1].
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the preceding downtrend that the bullish reversal classically assumes.</li>
    * <li>Despite the bullish-reversal label, Bulkowski's testing found this behaves as a bearish continuation 56% of the time — "near random" by his own description — though its overall post-breakout performance rank (21st of 103) is comparatively strong. ([thepatternsite.com](https://thepatternsite.com/HomingPigeon.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHOMINGPIGEON_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the pattern is detected, 0 otherwise. Never
    *        emits -100 (always bullish) Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#CDLHARAMI
    * @see Core#CDLMATCHINGLOW
    */
   public OutRange CDLHOMINGPIGEON( int startIdx,
                                    int endIdx,
                                    double inOpen[],
                                    double inHigh[],
                                    double inLow[],
                                    double inClose[],
                                    int outInteger[] )
   {
      requireIndexRange("CDLHOMINGPIGEON", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, CDLHOMINGPIGEON_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLHOMINGPIGEON", "inOpen", inOpen, guardInLen);
      requireLength("CDLHOMINGPIGEON", "inHigh", inHigh, guardInLen);
      requireLength("CDLHOMINGPIGEON", "inLow", inLow, guardInLen);
      requireLength("CDLHOMINGPIGEON", "inClose", inClose, guardInLen);
      requireLength("CDLHOMINGPIGEON", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHOMINGPIGEON_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHOMINGPIGEON", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Two-candle pattern: a long black candle followed by a small black candle
    * whose real body sits inside the prior body. A hit signals a bullish
    * reversal, most meaningful in a downtrend, which the code does not verify.
    * <p><b>Formula</b>
    * <pre>{@code
    * Two candles at i-1 and i. Both black: close[i-1] < open[i-1] and close[i] < open[i]. First body long: realbody[i-1] > BodyLong average. Second body short: realbody[i] <= BodyShort average. Second body contained by first: open[i] < open[i-1] and close[i] > close[i-1].
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the preceding downtrend that the bullish reversal classically assumes.</li>
    * <li>Despite the bullish-reversal label, Bulkowski's testing found this behaves as a bearish continuation 56% of the time — "near random" by his own description — though its overall post-breakout performance rank (21st of 103) is comparatively strong. ([thepatternsite.com](https://thepatternsite.com/HomingPigeon.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHOMINGPIGEON_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the pattern is detected, 0 otherwise. Never
    *        emits -100 (always bullish) Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#CDLHARAMI
    * @see Core#CDLMATCHINGLOW
    */
   public OutRange CDLHOMINGPIGEON( int startIdx,
                                    int endIdx,
                                    float inOpen[],
                                    float inHigh[],
                                    float inLow[],
                                    float inClose[],
                                    int outInteger[] )
   {
      requireIndexRange("CDLHOMINGPIGEON", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, CDLHOMINGPIGEON_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLHOMINGPIGEON", "inOpen", inOpen, guardInLen);
      requireLength("CDLHOMINGPIGEON", "inHigh", inHigh, guardInLen);
      requireLength("CDLHOMINGPIGEON", "inLow", inLow, guardInLen);
      requireLength("CDLHOMINGPIGEON", "inClose", inClose, guardInLen);
      requireLength("CDLHOMINGPIGEON", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHOMINGPIGEON_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHOMINGPIGEON", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLHOMINGPIGEON stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLHOMINGPIGEON} over the same series.
    * Open with {@link Core#CDLHOMINGPIGEON_Open}; there is no close — the handle is
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
   public static final class CDLHOMINGPIGEON_Stream {
      Core core;
      double BodyShortPeriodTotal;
      double BodyLongPeriodTotal;
      double lag1_inOpen;
      double lag1_inHigh;
      double lag1_inLow;
      double lag1_inClose;
      int ringPos_BodyLongTrailingIdx;
      int ringCap_BodyLongTrailingIdx;
      int ringLag_BodyLongTrailingIdx;
      double[] ring_BodyLongTrailingIdx_derived;
      int ringPos_BodyShortTrailingIdx;
      int ringCap_BodyShortTrailingIdx;
      double[] ring_BodyShortTrailingIdx_derived;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cs_BodyShort_rangeType;
      int cs_BodyShort_avgPeriod;
      double cs_BodyShort_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLHOMINGPIGEON_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLHOMINGPIGEON_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLHOMINGPIGEON_Stream( CDLHOMINGPIGEON_Stream other ) {
         this.core = other.core;
         this.BodyShortPeriodTotal = other.BodyShortPeriodTotal;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ringLag_BodyLongTrailingIdx = other.ringLag_BodyLongTrailingIdx;
         this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         this.ringPos_BodyShortTrailingIdx = other.ringPos_BodyShortTrailingIdx;
         this.ringCap_BodyShortTrailingIdx = other.ringCap_BodyShortTrailingIdx;
         this.ring_BodyShortTrailingIdx_derived = other.ring_BodyShortTrailingIdx_derived.clone();
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDLHOMINGPIGEON_Stream other ) {
         this.core = other.core;
         this.BodyShortPeriodTotal = other.BodyShortPeriodTotal;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ringLag_BodyLongTrailingIdx = other.ringLag_BodyLongTrailingIdx;
         if( this.ring_BodyLongTrailingIdx_derived != null && this.ring_BodyLongTrailingIdx_derived.length == other.ring_BodyLongTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_derived, 0, this.ring_BodyLongTrailingIdx_derived, 0, other.ring_BodyLongTrailingIdx_derived.length );
         } else {
            this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         }
         this.ringPos_BodyShortTrailingIdx = other.ringPos_BodyShortTrailingIdx;
         this.ringCap_BodyShortTrailingIdx = other.ringCap_BodyShortTrailingIdx;
         if( this.ring_BodyShortTrailingIdx_derived != null && this.ring_BodyShortTrailingIdx_derived.length == other.ring_BodyShortTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_BodyShortTrailingIdx_derived, 0, this.ring_BodyShortTrailingIdx_derived, 0, other.ring_BodyShortTrailingIdx_derived.length );
         } else {
            this.ring_BodyShortTrailingIdx_derived = other.ring_BodyShortTrailingIdx_derived.clone();
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
      private static final ThreadLocal<CDLHOMINGPIGEON_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the handle is left exactly as it was —
       * the stream stays usable, so skip the bar or re-open on a clean
       * history. This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDLHOMINGPIGEON update: BadParam", RetCode.BadParam);
         core.CDLHOMINGPIGEON_StreamStep(this, inOpen, inHigh, inLow, inClose);
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
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDLHOMINGPIGEON peek: BadParam", RetCode.BadParam);
         CDLHOMINGPIGEON_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLHOMINGPIGEON_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLHOMINGPIGEON_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLHOMINGPIGEON_Stream copy() {
         return new CDLHOMINGPIGEON_Stream(this);
      }
   }
   void CDLHOMINGPIGEON_StreamStep( CDLHOMINGPIGEON_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      int BodyShort_rangeType = sp.cs_BodyShort_rangeType;
      int BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
      double BodyShort_factor = sp.cs_BodyShort_factor;
      sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( sp.ringCap_BodyShortTrailingIdx == 0 ) {
         sp.ring_BodyShortTrailingIdx_derived[0] = ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      }
      if( ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 &&                 /* 2nd black */
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
          Math.abs(inClose - inOpen) <= ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd short */
          inOpen < sp.lag1_inOpen &&                                    /* 2nd engulfed by 1st */
          inClose > sp.lag1_inClose )
      {
         sp.cur_outInteger = 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx];
      sp.BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyShortTrailingIdx_derived[sp.ringPos_BodyShortTrailingIdx];
      sp.lag1_inOpen = inOpen;
      sp.lag1_inHigh = inHigh;
      sp.lag1_inLow = inLow;
      sp.lag1_inClose = inClose;
      sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
      if( sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx ) {
         sp.ringPos_BodyLongTrailingIdx = 0;
      }
      sp.ring_BodyShortTrailingIdx_derived[sp.ringPos_BodyShortTrailingIdx] = ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ringPos_BodyShortTrailingIdx = sp.ringPos_BodyShortTrailingIdx + 1;
      if( sp.ringPos_BodyShortTrailingIdx >= sp.ringCap_BodyShortTrailingIdx ) {
         sp.ringPos_BodyShortTrailingIdx = 0;
      }
   }
   private RetCode CDLHOMINGPIGEON_OpenPass( CDLHOMINGPIGEON_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
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
      lookbackTotal = CDLHOMINGPIGEON_Lookback();
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
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      BodyLongPeriodTotal = 0;
      BodyShortPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black candle
       * - second candle: short black real body completely inside the previous day's body
       * The meaning of "short" and "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100): homing pigeon is always bullish;
       * the user should consider that homing pigeon is significant when it appears in a downtrend,
       * while this function does not consider the trend
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* 2nd black */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
             Math.abs(inClose[i] - inOpen[i]) <= ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd short */
             inOpen[i] < inOpen[i - 1] &&                                /* 2nd engulfed by 1st */
             inClose[i] > inClose[i - 1] )
         {
            outInteger[outIdx++ * outStride] = 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 1] - (((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inClose[BodyLongTrailingIdx - 1]) : (inOpen[BodyLongTrailingIdx - 1]))) + ((((inClose[BodyLongTrailingIdx - 1]) >= (inOpen[BodyLongTrailingIdx - 1])) ? (inOpen[BodyLongTrailingIdx - 1]) : (inClose[BodyLongTrailingIdx - 1])) - inLow[BodyLongTrailingIdx - 1])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      int cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 2;
      if( capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyLongTrailingIdx = (cap_BodyLongTrailingIdx > 0)? cap_BodyLongTrailingIdx : 1;
      double[] capRing_BodyLongTrailingIdx_derived = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int cap_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
      if( cap_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyShortTrailingIdx = (cap_BodyShortTrailingIdx > 0)? cap_BodyShortTrailingIdx : 1;
      double[] capRing_BodyShortTrailingIdx_derived = new double[allocN_BodyShortTrailingIdx];
      for( int fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyShortTrailingIdx_derived[fillJ - (historyLen - cap_BodyShortTrailingIdx)] = ((BodyShort_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.BodyShortPeriodTotal = BodyShortPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
      sp.ringPos_BodyShortTrailingIdx = 0;
      sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
      sp.ring_BodyShortTrailingIdx_derived = capRing_BodyShortTrailingIdx_derived;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CDLHOMINGPIGEON_OpenImpl( CDLHOMINGPIGEON_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDLHOMINGPIGEON_OpenPass( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDLHOMINGPIGEON_OpenAndFillImpl( CDLHOMINGPIGEON_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDLHOMINGPIGEON_OpenPass( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDLHOMINGPIGEON_OpenAndFillInternalImpl( CDLHOMINGPIGEON_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDLHOMINGPIGEON_OpenPass(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDLHOMINGPIGEON_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLHOMINGPIGEON_Stream CDLHOMINGPIGEON_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLHOMINGPIGEON_Stream sp = new CDLHOMINGPIGEON_Stream(this);
      RetCode retCode = CDLHOMINGPIGEON_OpenAndFillInternalImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLHOMINGPIGEON openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLHOMINGPIGEON openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLHOMINGPIGEON openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind CDLHOMINGPIGEON_Open (composition seam). */
   CDLHOMINGPIGEON_Stream CDLHOMINGPIGEON_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLHOMINGPIGEON_Stream sp = new CDLHOMINGPIGEON_Stream(this);
      RetCode retCode = CDLHOMINGPIGEON_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLHOMINGPIGEON open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLHOMINGPIGEON open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLHOMINGPIGEON open: " + retCode, retCode);
   }
   /**
    * Open a live CDLHOMINGPIGEON stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLHOMINGPIGEON} at that bar.
    * <p>The history must hold at least {@code CDLHOMINGPIGEON_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLHOMINGPIGEON_Stream CDLHOMINGPIGEON_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLHOMINGPIGEON_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLHOMINGPIGEON_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLHOMINGPIGEON} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLHOMINGPIGEON_Stream#fillRange()}.
    */
   public CDLHOMINGPIGEON_Stream CDLHOMINGPIGEON_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLHOMINGPIGEON_Stream sp = new CDLHOMINGPIGEON_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHOMINGPIGEON_OpenAndFillImpl(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLHOMINGPIGEON openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLHOMINGPIGEON openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLHOMINGPIGEON openAndFill: " + retCode, retCode);
   }
