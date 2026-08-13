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
 *  011505 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLTAKURI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLTAKURI_Lookback( )
   {
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      return Math.max(Math.max(BodyDoji_avgPeriod, ShadowVeryShort_avgPeriod), ShadowVeryLong_avgPeriod) ;

   }
   RetCode CDLTAKURI_Internal( int startIdx,
                               int endIdx,
                               double inOpen[],
                               double inHigh[],
                               double inLow[],
                               double inClose[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      double ShadowVeryLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int ShadowVeryLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLTAKURI_Lookback();
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
      BodyDojiPeriodTotal = 0;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      ShadowVeryLongPeriodTotal = 0;
      ShadowVeryLongTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryLongTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       *
       * Must have:
       * - doji body
       * - open and close at the high of the day = no or very short upper shadow
       * - very long lower shadow
       * The meaning of "doji", "very short" and "very long" is specified with TA_SetCandleSettings
       * outInteger is always positive (1 to 100) but this does not mean it is bullish: takuri must be considered
       * relatively to the trend
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowVeryLongPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : 0.0)));
         ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[ShadowVeryLongTrailingIdx] - inOpen[ShadowVeryLongTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[ShadowVeryLongTrailingIdx] - inLow[ShadowVeryLongTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[ShadowVeryLongTrailingIdx] - inLow[ShadowVeryLongTrailingIdx]) - Math.abs(inClose[ShadowVeryLongTrailingIdx] - inOpen[ShadowVeryLongTrailingIdx])) : 0.0)));
         i += 1;
         BodyDojiTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
         ShadowVeryLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLTAKURI_Internal( int startIdx,
                               int endIdx,
                               float inOpen[],
                               float inHigh[],
                               float inLow[],
                               float inClose[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      double ShadowVeryLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int ShadowVeryLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLTAKURI_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyDojiPeriodTotal = 0;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      ShadowVeryLongPeriodTotal = 0;
      ShadowVeryLongTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryLongTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowVeryLongPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) - Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx] - (double)inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx] - (double)inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx] - (double)inLow[ShadowVeryShortTrailingIdx]) - Math.abs((double)inClose[ShadowVeryShortTrailingIdx] - (double)inOpen[ShadowVeryShortTrailingIdx])) : 0.0)));
         ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryLongTrailingIdx] - (double)inOpen[ShadowVeryLongTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[ShadowVeryLongTrailingIdx] - (double)inLow[ShadowVeryLongTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[ShadowVeryLongTrailingIdx] - (double)inLow[ShadowVeryLongTrailingIdx]) - Math.abs((double)inClose[ShadowVeryLongTrailingIdx] - (double)inOpen[ShadowVeryLongTrailingIdx])) : 0.0)));
         i += 1;
         BodyDojiTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
         ShadowVeryLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Single-candle pattern: a doji whose open and close sit at the high
    * (no/very short upper shadow) with a very long lower shadow, i.e. a
    * dragonfly doji with an exceptionally long lower shadow. Emitted as a
    * positive signal, but its directional meaning depends on the prevailing
    * trend, which the code does not check. A hit marks a takuri
    * (dragonfly-doji) line; a potential reversal only when read against the
    * trend (typically a bottom/bullish reversal after a downtrend), which the
    * code itself does not verify.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLTAKURI_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the takuri pattern is detected, 0 otherwise.
    *        Never negative; the positive sign is a convention and does not by itself
    *        imply bullishness. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLDRAGONFLYDOJI
    * @see Core#CDLDOJI
    * @see Core#CDLHAMMER
    * @see Core#CDLGRAVESTONEDOJI
    */
   public OutRange CDLTAKURI( int startIdx,
                              int endIdx,
                              double inOpen[],
                              double inHigh[],
                              double inLow[],
                              double inClose[],
                              int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLTAKURI_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLTAKURI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Single-candle pattern: a doji whose open and close sit at the high
    * (no/very short upper shadow) with a very long lower shadow, i.e. a
    * dragonfly doji with an exceptionally long lower shadow. Emitted as a
    * positive signal, but its directional meaning depends on the prevailing
    * trend, which the code does not check. A hit marks a takuri
    * (dragonfly-doji) line; a potential reversal only when read against the
    * trend (typically a bottom/bullish reversal after a downtrend), which the
    * code itself does not verify.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLTAKURI_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the takuri pattern is detected, 0 otherwise.
    *        Never negative; the positive sign is a convention and does not by itself
    *        imply bullishness. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLDRAGONFLYDOJI
    * @see Core#CDLDOJI
    * @see Core#CDLHAMMER
    * @see Core#CDLGRAVESTONEDOJI
    */
   public OutRange CDLTAKURI( int startIdx,
                              int endIdx,
                              float inOpen[],
                              float inHigh[],
                              float inLow[],
                              float inClose[],
                              int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLTAKURI_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLTAKURI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLTAKURI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLTAKURI} over the same series.
    * Open with {@link Core#CDLTAKURI_Open}; there is no close — the handle is
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
   public static final class CDLTAKURI_Stream {
      Core core;
      double BodyDojiPeriodTotal;
      double ShadowVeryShortPeriodTotal;
      double ShadowVeryLongPeriodTotal;
      int ringPos_BodyDojiTrailingIdx;
      int ringCap_BodyDojiTrailingIdx;
      double[] ring_BodyDojiTrailingIdx_inOpen;
      double[] ring_BodyDojiTrailingIdx_inHigh;
      double[] ring_BodyDojiTrailingIdx_inLow;
      double[] ring_BodyDojiTrailingIdx_inClose;
      int ringPos_ShadowVeryLongTrailingIdx;
      int ringCap_ShadowVeryLongTrailingIdx;
      double[] ring_ShadowVeryLongTrailingIdx_inOpen;
      double[] ring_ShadowVeryLongTrailingIdx_inHigh;
      double[] ring_ShadowVeryLongTrailingIdx_inLow;
      double[] ring_ShadowVeryLongTrailingIdx_inClose;
      int ringPos_ShadowVeryShortTrailingIdx;
      int ringCap_ShadowVeryShortTrailingIdx;
      double[] ring_ShadowVeryShortTrailingIdx_inOpen;
      double[] ring_ShadowVeryShortTrailingIdx_inHigh;
      double[] ring_ShadowVeryShortTrailingIdx_inLow;
      double[] ring_ShadowVeryShortTrailingIdx_inClose;
      int cs_BodyDoji_rangeType;
      int cs_BodyDoji_avgPeriod;
      double cs_BodyDoji_factor;
      int cs_ShadowVeryLong_rangeType;
      int cs_ShadowVeryLong_avgPeriod;
      double cs_ShadowVeryLong_factor;
      int cs_ShadowVeryShort_rangeType;
      int cs_ShadowVeryShort_avgPeriod;
      double cs_ShadowVeryShort_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLTAKURI_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLTAKURI_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLTAKURI_Stream( CDLTAKURI_Stream other ) {
         this.core = other.core;
         this.BodyDojiPeriodTotal = other.BodyDojiPeriodTotal;
         this.ShadowVeryShortPeriodTotal = other.ShadowVeryShortPeriodTotal;
         this.ShadowVeryLongPeriodTotal = other.ShadowVeryLongPeriodTotal;
         this.ringPos_BodyDojiTrailingIdx = other.ringPos_BodyDojiTrailingIdx;
         this.ringCap_BodyDojiTrailingIdx = other.ringCap_BodyDojiTrailingIdx;
         this.ring_BodyDojiTrailingIdx_inOpen = other.ring_BodyDojiTrailingIdx_inOpen.clone();
         this.ring_BodyDojiTrailingIdx_inHigh = other.ring_BodyDojiTrailingIdx_inHigh.clone();
         this.ring_BodyDojiTrailingIdx_inLow = other.ring_BodyDojiTrailingIdx_inLow.clone();
         this.ring_BodyDojiTrailingIdx_inClose = other.ring_BodyDojiTrailingIdx_inClose.clone();
         this.ringPos_ShadowVeryLongTrailingIdx = other.ringPos_ShadowVeryLongTrailingIdx;
         this.ringCap_ShadowVeryLongTrailingIdx = other.ringCap_ShadowVeryLongTrailingIdx;
         this.ring_ShadowVeryLongTrailingIdx_inOpen = other.ring_ShadowVeryLongTrailingIdx_inOpen.clone();
         this.ring_ShadowVeryLongTrailingIdx_inHigh = other.ring_ShadowVeryLongTrailingIdx_inHigh.clone();
         this.ring_ShadowVeryLongTrailingIdx_inLow = other.ring_ShadowVeryLongTrailingIdx_inLow.clone();
         this.ring_ShadowVeryLongTrailingIdx_inClose = other.ring_ShadowVeryLongTrailingIdx_inClose.clone();
         this.ringPos_ShadowVeryShortTrailingIdx = other.ringPos_ShadowVeryShortTrailingIdx;
         this.ringCap_ShadowVeryShortTrailingIdx = other.ringCap_ShadowVeryShortTrailingIdx;
         this.ring_ShadowVeryShortTrailingIdx_inOpen = other.ring_ShadowVeryShortTrailingIdx_inOpen.clone();
         this.ring_ShadowVeryShortTrailingIdx_inHigh = other.ring_ShadowVeryShortTrailingIdx_inHigh.clone();
         this.ring_ShadowVeryShortTrailingIdx_inLow = other.ring_ShadowVeryShortTrailingIdx_inLow.clone();
         this.ring_ShadowVeryShortTrailingIdx_inClose = other.ring_ShadowVeryShortTrailingIdx_inClose.clone();
         this.cs_BodyDoji_rangeType = other.cs_BodyDoji_rangeType;
         this.cs_BodyDoji_avgPeriod = other.cs_BodyDoji_avgPeriod;
         this.cs_BodyDoji_factor = other.cs_BodyDoji_factor;
         this.cs_ShadowVeryLong_rangeType = other.cs_ShadowVeryLong_rangeType;
         this.cs_ShadowVeryLong_avgPeriod = other.cs_ShadowVeryLong_avgPeriod;
         this.cs_ShadowVeryLong_factor = other.cs_ShadowVeryLong_factor;
         this.cs_ShadowVeryShort_rangeType = other.cs_ShadowVeryShort_rangeType;
         this.cs_ShadowVeryShort_avgPeriod = other.cs_ShadowVeryShort_avgPeriod;
         this.cs_ShadowVeryShort_factor = other.cs_ShadowVeryShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDLTAKURI_Stream other ) {
         this.core = other.core;
         this.BodyDojiPeriodTotal = other.BodyDojiPeriodTotal;
         this.ShadowVeryShortPeriodTotal = other.ShadowVeryShortPeriodTotal;
         this.ShadowVeryLongPeriodTotal = other.ShadowVeryLongPeriodTotal;
         this.ringPos_BodyDojiTrailingIdx = other.ringPos_BodyDojiTrailingIdx;
         this.ringCap_BodyDojiTrailingIdx = other.ringCap_BodyDojiTrailingIdx;
         if( this.ring_BodyDojiTrailingIdx_inOpen != null && this.ring_BodyDojiTrailingIdx_inOpen.length == other.ring_BodyDojiTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inOpen, 0, this.ring_BodyDojiTrailingIdx_inOpen, 0, other.ring_BodyDojiTrailingIdx_inOpen.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inOpen = other.ring_BodyDojiTrailingIdx_inOpen.clone();
         }
         if( this.ring_BodyDojiTrailingIdx_inHigh != null && this.ring_BodyDojiTrailingIdx_inHigh.length == other.ring_BodyDojiTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inHigh, 0, this.ring_BodyDojiTrailingIdx_inHigh, 0, other.ring_BodyDojiTrailingIdx_inHigh.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inHigh = other.ring_BodyDojiTrailingIdx_inHigh.clone();
         }
         if( this.ring_BodyDojiTrailingIdx_inLow != null && this.ring_BodyDojiTrailingIdx_inLow.length == other.ring_BodyDojiTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inLow, 0, this.ring_BodyDojiTrailingIdx_inLow, 0, other.ring_BodyDojiTrailingIdx_inLow.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inLow = other.ring_BodyDojiTrailingIdx_inLow.clone();
         }
         if( this.ring_BodyDojiTrailingIdx_inClose != null && this.ring_BodyDojiTrailingIdx_inClose.length == other.ring_BodyDojiTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inClose, 0, this.ring_BodyDojiTrailingIdx_inClose, 0, other.ring_BodyDojiTrailingIdx_inClose.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inClose = other.ring_BodyDojiTrailingIdx_inClose.clone();
         }
         this.ringPos_ShadowVeryLongTrailingIdx = other.ringPos_ShadowVeryLongTrailingIdx;
         this.ringCap_ShadowVeryLongTrailingIdx = other.ringCap_ShadowVeryLongTrailingIdx;
         if( this.ring_ShadowVeryLongTrailingIdx_inOpen != null && this.ring_ShadowVeryLongTrailingIdx_inOpen.length == other.ring_ShadowVeryLongTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_ShadowVeryLongTrailingIdx_inOpen, 0, this.ring_ShadowVeryLongTrailingIdx_inOpen, 0, other.ring_ShadowVeryLongTrailingIdx_inOpen.length );
         } else {
            this.ring_ShadowVeryLongTrailingIdx_inOpen = other.ring_ShadowVeryLongTrailingIdx_inOpen.clone();
         }
         if( this.ring_ShadowVeryLongTrailingIdx_inHigh != null && this.ring_ShadowVeryLongTrailingIdx_inHigh.length == other.ring_ShadowVeryLongTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_ShadowVeryLongTrailingIdx_inHigh, 0, this.ring_ShadowVeryLongTrailingIdx_inHigh, 0, other.ring_ShadowVeryLongTrailingIdx_inHigh.length );
         } else {
            this.ring_ShadowVeryLongTrailingIdx_inHigh = other.ring_ShadowVeryLongTrailingIdx_inHigh.clone();
         }
         if( this.ring_ShadowVeryLongTrailingIdx_inLow != null && this.ring_ShadowVeryLongTrailingIdx_inLow.length == other.ring_ShadowVeryLongTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_ShadowVeryLongTrailingIdx_inLow, 0, this.ring_ShadowVeryLongTrailingIdx_inLow, 0, other.ring_ShadowVeryLongTrailingIdx_inLow.length );
         } else {
            this.ring_ShadowVeryLongTrailingIdx_inLow = other.ring_ShadowVeryLongTrailingIdx_inLow.clone();
         }
         if( this.ring_ShadowVeryLongTrailingIdx_inClose != null && this.ring_ShadowVeryLongTrailingIdx_inClose.length == other.ring_ShadowVeryLongTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_ShadowVeryLongTrailingIdx_inClose, 0, this.ring_ShadowVeryLongTrailingIdx_inClose, 0, other.ring_ShadowVeryLongTrailingIdx_inClose.length );
         } else {
            this.ring_ShadowVeryLongTrailingIdx_inClose = other.ring_ShadowVeryLongTrailingIdx_inClose.clone();
         }
         this.ringPos_ShadowVeryShortTrailingIdx = other.ringPos_ShadowVeryShortTrailingIdx;
         this.ringCap_ShadowVeryShortTrailingIdx = other.ringCap_ShadowVeryShortTrailingIdx;
         if( this.ring_ShadowVeryShortTrailingIdx_inOpen != null && this.ring_ShadowVeryShortTrailingIdx_inOpen.length == other.ring_ShadowVeryShortTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_ShadowVeryShortTrailingIdx_inOpen, 0, this.ring_ShadowVeryShortTrailingIdx_inOpen, 0, other.ring_ShadowVeryShortTrailingIdx_inOpen.length );
         } else {
            this.ring_ShadowVeryShortTrailingIdx_inOpen = other.ring_ShadowVeryShortTrailingIdx_inOpen.clone();
         }
         if( this.ring_ShadowVeryShortTrailingIdx_inHigh != null && this.ring_ShadowVeryShortTrailingIdx_inHigh.length == other.ring_ShadowVeryShortTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_ShadowVeryShortTrailingIdx_inHigh, 0, this.ring_ShadowVeryShortTrailingIdx_inHigh, 0, other.ring_ShadowVeryShortTrailingIdx_inHigh.length );
         } else {
            this.ring_ShadowVeryShortTrailingIdx_inHigh = other.ring_ShadowVeryShortTrailingIdx_inHigh.clone();
         }
         if( this.ring_ShadowVeryShortTrailingIdx_inLow != null && this.ring_ShadowVeryShortTrailingIdx_inLow.length == other.ring_ShadowVeryShortTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_ShadowVeryShortTrailingIdx_inLow, 0, this.ring_ShadowVeryShortTrailingIdx_inLow, 0, other.ring_ShadowVeryShortTrailingIdx_inLow.length );
         } else {
            this.ring_ShadowVeryShortTrailingIdx_inLow = other.ring_ShadowVeryShortTrailingIdx_inLow.clone();
         }
         if( this.ring_ShadowVeryShortTrailingIdx_inClose != null && this.ring_ShadowVeryShortTrailingIdx_inClose.length == other.ring_ShadowVeryShortTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_ShadowVeryShortTrailingIdx_inClose, 0, this.ring_ShadowVeryShortTrailingIdx_inClose, 0, other.ring_ShadowVeryShortTrailingIdx_inClose.length );
         } else {
            this.ring_ShadowVeryShortTrailingIdx_inClose = other.ring_ShadowVeryShortTrailingIdx_inClose.clone();
         }
         this.cs_BodyDoji_rangeType = other.cs_BodyDoji_rangeType;
         this.cs_BodyDoji_avgPeriod = other.cs_BodyDoji_avgPeriod;
         this.cs_BodyDoji_factor = other.cs_BodyDoji_factor;
         this.cs_ShadowVeryLong_rangeType = other.cs_ShadowVeryLong_rangeType;
         this.cs_ShadowVeryLong_avgPeriod = other.cs_ShadowVeryLong_avgPeriod;
         this.cs_ShadowVeryLong_factor = other.cs_ShadowVeryLong_factor;
         this.cs_ShadowVeryShort_rangeType = other.cs_ShadowVeryShort_rangeType;
         this.cs_ShadowVeryShort_avgPeriod = other.cs_ShadowVeryShort_avgPeriod;
         this.cs_ShadowVeryShort_factor = other.cs_ShadowVeryShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDLTAKURI_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.CDLTAKURI_StreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CDLTAKURI_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLTAKURI_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLTAKURI_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLTAKURI_Stream copy() {
         return new CDLTAKURI_Stream(this);
      }
   }
   void CDLTAKURI_StreamStep( CDLTAKURI_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
      int BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
      double BodyDoji_factor = sp.cs_BodyDoji_factor;
      int ShadowVeryLong_rangeType = sp.cs_ShadowVeryLong_rangeType;
      int ShadowVeryLong_avgPeriod = sp.cs_ShadowVeryLong_avgPeriod;
      double ShadowVeryLong_factor = sp.cs_ShadowVeryLong_factor;
      int ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
      int ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
      double ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
      if( sp.ringCap_BodyDojiTrailingIdx == 0 ) {
         sp.ring_BodyDojiTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyDojiTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyDojiTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyDojiTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_ShadowVeryLongTrailingIdx == 0 ) {
         sp.ring_ShadowVeryLongTrailingIdx_inOpen[0] = inOpen;
         sp.ring_ShadowVeryLongTrailingIdx_inHigh[0] = inHigh;
         sp.ring_ShadowVeryLongTrailingIdx_inLow[0] = inLow;
         sp.ring_ShadowVeryLongTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_ShadowVeryShortTrailingIdx == 0 ) {
         sp.ring_ShadowVeryShortTrailingIdx_inOpen[0] = inOpen;
         sp.ring_ShadowVeryShortTrailingIdx_inHigh[0] = inHigh;
         sp.ring_ShadowVeryShortTrailingIdx_inLow[0] = inLow;
         sp.ring_ShadowVeryShortTrailingIdx_inClose[0] = inClose;
      }
      if( Math.abs(inClose - inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType == 1) ? (inHigh - inLow) : ((BodyDoji_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (sp.ShadowVeryLongPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
         sp.cur_outInteger = 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType == 1) ? (inHigh - inLow) : ((BodyDoji_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx]) - Math.abs(sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx])) : 0.0)));
      sp.ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.ring_ShadowVeryShortTrailingIdx_inClose[sp.ringPos_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inOpen[sp.ringPos_ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (sp.ring_ShadowVeryShortTrailingIdx_inHigh[sp.ringPos_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inLow[sp.ringPos_ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((sp.ring_ShadowVeryShortTrailingIdx_inHigh[sp.ringPos_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inLow[sp.ringPos_ShadowVeryShortTrailingIdx]) - Math.abs(sp.ring_ShadowVeryShortTrailingIdx_inClose[sp.ringPos_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inOpen[sp.ringPos_ShadowVeryShortTrailingIdx])) : 0.0)));
      sp.ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs(sp.ring_ShadowVeryLongTrailingIdx_inClose[sp.ringPos_ShadowVeryLongTrailingIdx] - sp.ring_ShadowVeryLongTrailingIdx_inOpen[sp.ringPos_ShadowVeryLongTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? (sp.ring_ShadowVeryLongTrailingIdx_inHigh[sp.ringPos_ShadowVeryLongTrailingIdx] - sp.ring_ShadowVeryLongTrailingIdx_inLow[sp.ringPos_ShadowVeryLongTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? ((sp.ring_ShadowVeryLongTrailingIdx_inHigh[sp.ringPos_ShadowVeryLongTrailingIdx] - sp.ring_ShadowVeryLongTrailingIdx_inLow[sp.ringPos_ShadowVeryLongTrailingIdx]) - Math.abs(sp.ring_ShadowVeryLongTrailingIdx_inClose[sp.ringPos_ShadowVeryLongTrailingIdx] - sp.ring_ShadowVeryLongTrailingIdx_inOpen[sp.ringPos_ShadowVeryLongTrailingIdx])) : 0.0)));
      sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx] = inOpen;
      sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] = inHigh;
      sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx] = inLow;
      sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] = inClose;
      sp.ringPos_BodyDojiTrailingIdx = sp.ringPos_BodyDojiTrailingIdx + 1;
      if( sp.ringPos_BodyDojiTrailingIdx >= sp.ringCap_BodyDojiTrailingIdx ) {
         sp.ringPos_BodyDojiTrailingIdx = 0;
      }
      sp.ring_ShadowVeryLongTrailingIdx_inOpen[sp.ringPos_ShadowVeryLongTrailingIdx] = inOpen;
      sp.ring_ShadowVeryLongTrailingIdx_inHigh[sp.ringPos_ShadowVeryLongTrailingIdx] = inHigh;
      sp.ring_ShadowVeryLongTrailingIdx_inLow[sp.ringPos_ShadowVeryLongTrailingIdx] = inLow;
      sp.ring_ShadowVeryLongTrailingIdx_inClose[sp.ringPos_ShadowVeryLongTrailingIdx] = inClose;
      sp.ringPos_ShadowVeryLongTrailingIdx = sp.ringPos_ShadowVeryLongTrailingIdx + 1;
      if( sp.ringPos_ShadowVeryLongTrailingIdx >= sp.ringCap_ShadowVeryLongTrailingIdx ) {
         sp.ringPos_ShadowVeryLongTrailingIdx = 0;
      }
      sp.ring_ShadowVeryShortTrailingIdx_inOpen[sp.ringPos_ShadowVeryShortTrailingIdx] = inOpen;
      sp.ring_ShadowVeryShortTrailingIdx_inHigh[sp.ringPos_ShadowVeryShortTrailingIdx] = inHigh;
      sp.ring_ShadowVeryShortTrailingIdx_inLow[sp.ringPos_ShadowVeryShortTrailingIdx] = inLow;
      sp.ring_ShadowVeryShortTrailingIdx_inClose[sp.ringPos_ShadowVeryShortTrailingIdx] = inClose;
      sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
      if( sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx ) {
         sp.ringPos_ShadowVeryShortTrailingIdx = 0;
      }
   }
   private RetCode CDLTAKURI_OpenCore( CDLTAKURI_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double BodyDojiPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      double ShadowVeryLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int ShadowVeryLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLTAKURI_Lookback();
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
      BodyDojiPeriodTotal = 0;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      ShadowVeryLongPeriodTotal = 0;
      ShadowVeryLongTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryLongTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       *
       * Must have:
       * - doji body
       * - open and close at the high of the day = no or very short upper shadow
       * - very long lower shadow
       * The meaning of "doji", "very short" and "very long" is specified with TA_SetCandleSettings
       * outInteger is always positive (1 to 100) but this does not mean it is bullish: takuri must be considered
       * relatively to the trend
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowVeryLongPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++ * outStride] = 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : 0.0)));
         ShadowVeryLongPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[ShadowVeryLongTrailingIdx] - inOpen[ShadowVeryLongTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[ShadowVeryLongTrailingIdx] - inLow[ShadowVeryLongTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[ShadowVeryLongTrailingIdx] - inLow[ShadowVeryLongTrailingIdx]) - Math.abs(inClose[ShadowVeryLongTrailingIdx] - inOpen[ShadowVeryLongTrailingIdx])) : 0.0)));
         i += 1;
         BodyDojiTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
         ShadowVeryLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_BodyDojiTrailingIdx = i - BodyDojiTrailingIdx;
      if( cap_BodyDojiTrailingIdx < 0 || cap_BodyDojiTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyDojiTrailingIdx = (cap_BodyDojiTrailingIdx > 0)? cap_BodyDojiTrailingIdx : 1;
      double[] capRing_BodyDojiTrailingIdx_inOpen = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inOpen, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inHigh = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inHigh, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inLow = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inLow, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inClose = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inClose, 0, cap_BodyDojiTrailingIdx);
      int cap_ShadowVeryLongTrailingIdx = i - ShadowVeryLongTrailingIdx;
      if( cap_ShadowVeryLongTrailingIdx < 0 || cap_ShadowVeryLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowVeryLongTrailingIdx = (cap_ShadowVeryLongTrailingIdx > 0)? cap_ShadowVeryLongTrailingIdx : 1;
      double[] capRing_ShadowVeryLongTrailingIdx_inOpen = new double[allocN_ShadowVeryLongTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_ShadowVeryLongTrailingIdx, capRing_ShadowVeryLongTrailingIdx_inOpen, 0, cap_ShadowVeryLongTrailingIdx);
      double[] capRing_ShadowVeryLongTrailingIdx_inHigh = new double[allocN_ShadowVeryLongTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_ShadowVeryLongTrailingIdx, capRing_ShadowVeryLongTrailingIdx_inHigh, 0, cap_ShadowVeryLongTrailingIdx);
      double[] capRing_ShadowVeryLongTrailingIdx_inLow = new double[allocN_ShadowVeryLongTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_ShadowVeryLongTrailingIdx, capRing_ShadowVeryLongTrailingIdx_inLow, 0, cap_ShadowVeryLongTrailingIdx);
      double[] capRing_ShadowVeryLongTrailingIdx_inClose = new double[allocN_ShadowVeryLongTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_ShadowVeryLongTrailingIdx, capRing_ShadowVeryLongTrailingIdx_inClose, 0, cap_ShadowVeryLongTrailingIdx);
      int cap_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
      if( cap_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowVeryShortTrailingIdx = (cap_ShadowVeryShortTrailingIdx > 0)? cap_ShadowVeryShortTrailingIdx : 1;
      double[] capRing_ShadowVeryShortTrailingIdx_inOpen = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inOpen, 0, cap_ShadowVeryShortTrailingIdx);
      double[] capRing_ShadowVeryShortTrailingIdx_inHigh = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inHigh, 0, cap_ShadowVeryShortTrailingIdx);
      double[] capRing_ShadowVeryShortTrailingIdx_inLow = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inLow, 0, cap_ShadowVeryShortTrailingIdx);
      double[] capRing_ShadowVeryShortTrailingIdx_inClose = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inClose, 0, cap_ShadowVeryShortTrailingIdx);
      sp.BodyDojiPeriodTotal = BodyDojiPeriodTotal;
      sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
      sp.ShadowVeryLongPeriodTotal = ShadowVeryLongPeriodTotal;
      sp.ringPos_BodyDojiTrailingIdx = 0;
      sp.ringCap_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx;
      sp.ring_BodyDojiTrailingIdx_inOpen = capRing_BodyDojiTrailingIdx_inOpen;
      sp.ring_BodyDojiTrailingIdx_inHigh = capRing_BodyDojiTrailingIdx_inHigh;
      sp.ring_BodyDojiTrailingIdx_inLow = capRing_BodyDojiTrailingIdx_inLow;
      sp.ring_BodyDojiTrailingIdx_inClose = capRing_BodyDojiTrailingIdx_inClose;
      sp.ringPos_ShadowVeryLongTrailingIdx = 0;
      sp.ringCap_ShadowVeryLongTrailingIdx = cap_ShadowVeryLongTrailingIdx;
      sp.ring_ShadowVeryLongTrailingIdx_inOpen = capRing_ShadowVeryLongTrailingIdx_inOpen;
      sp.ring_ShadowVeryLongTrailingIdx_inHigh = capRing_ShadowVeryLongTrailingIdx_inHigh;
      sp.ring_ShadowVeryLongTrailingIdx_inLow = capRing_ShadowVeryLongTrailingIdx_inLow;
      sp.ring_ShadowVeryLongTrailingIdx_inClose = capRing_ShadowVeryLongTrailingIdx_inClose;
      sp.ringPos_ShadowVeryShortTrailingIdx = 0;
      sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
      sp.ring_ShadowVeryShortTrailingIdx_inOpen = capRing_ShadowVeryShortTrailingIdx_inOpen;
      sp.ring_ShadowVeryShortTrailingIdx_inHigh = capRing_ShadowVeryShortTrailingIdx_inHigh;
      sp.ring_ShadowVeryShortTrailingIdx_inLow = capRing_ShadowVeryShortTrailingIdx_inLow;
      sp.ring_ShadowVeryShortTrailingIdx_inClose = capRing_ShadowVeryShortTrailingIdx_inClose;
      sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
      sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
      sp.cs_BodyDoji_factor = BodyDoji_factor;
      sp.cs_ShadowVeryLong_rangeType = ShadowVeryLong_rangeType;
      sp.cs_ShadowVeryLong_avgPeriod = ShadowVeryLong_avgPeriod;
      sp.cs_ShadowVeryLong_factor = ShadowVeryLong_factor;
      sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
      sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
      sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CDLTAKURI_OpenBody( CDLTAKURI_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDLTAKURI_OpenCore( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDLTAKURI_OpenAndFillBody( CDLTAKURI_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDLTAKURI_OpenCore( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDLTAKURI_OpenAndFillInternalBody( CDLTAKURI_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDLTAKURI_OpenCore(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDLTAKURI_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLTAKURI_Stream CDLTAKURI_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLTAKURI_Stream sp = new CDLTAKURI_Stream(this);
      RetCode retCode = CDLTAKURI_OpenAndFillInternalBody(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLTAKURI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLTAKURI openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLTAKURI openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind CDLTAKURI_Open (composition seam). */
   CDLTAKURI_Stream CDLTAKURI_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLTAKURI_Stream sp = new CDLTAKURI_Stream(this);
      RetCode retCode = CDLTAKURI_OpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLTAKURI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLTAKURI open: internal error");
      }
      throw new IllegalArgumentException("CDLTAKURI open: " + retCode);
   }
   /**
    * Open a live CDLTAKURI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLTAKURI} at that bar.
    * <p>The history must hold at least {@code CDLTAKURI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLTAKURI_Stream CDLTAKURI_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLTAKURI_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLTAKURI_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLTAKURI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLTAKURI_Stream#fillRange()}.
    */
   public CDLTAKURI_Stream CDLTAKURI_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLTAKURI_Stream sp = new CDLTAKURI_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLTAKURI_OpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLTAKURI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLTAKURI openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLTAKURI openAndFill: " + retCode);
   }
