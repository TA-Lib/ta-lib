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
    * Number of leading input bars {@link Core#CDLSEPARATINGLINES} consumes
    * before it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLSEPARATINGLINES_Lookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      return Math.max(Math.max(ShadowVeryShort_avgPeriod, BodyLong_avgPeriod), Equal_avgPeriod) + 1 ;

   }
   RetCode CDLSEPARATINGLINES_Impl( int startIdx,
                                    int endIdx,
                                    double inOpen[],
                                    double inHigh[],
                                    double inLow[],
                                    double inClose[],
                                    MInteger outBegIdx,
                                    MInteger outNBElement,
                                    int outInteger[] )
   {
      double ShadowVeryShortPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
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
      lookbackTotal = CDLSEPARATINGLINES_Lookback();
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
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: black (white) candle
       * - second candle: bullish (bearish) belt hold with the same open as the prior candle
       * The meaning of "long body" and "very short shadow" of the belt hold is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that separating lines is significant when coming in a trend and the belt hold has
       * the same direction of the trend, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) && /* opposite candles */
             inOpen[i] <= inOpen[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* same open */
             inOpen[i] >= inOpen[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* belt hold: long body */
             (((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) || ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0))))) ) /* with no lower shadow if bullish with no upper shadow if bearish */
         {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLSEPARATINGLINES_Impl( int startIdx,
                                    int endIdx,
                                    float inOpen[],
                                    float inHigh[],
                                    float inLow[],
                                    float inClose[],
                                    MInteger outBegIdx,
                                    MInteger outNBElement,
                                    int outInteger[] )
   {
      double ShadowVeryShortPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLSEPARATINGLINES_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) && (double)inOpen[i] <= (double)inOpen[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] >= (double)inOpen[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) || (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0))))) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx] - (double)inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx] - (double)inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx] - ((((double)inClose[ShadowVeryShortTrailingIdx]) >= ((double)inOpen[ShadowVeryShortTrailingIdx])) ? ((double)inClose[ShadowVeryShortTrailingIdx]) : ((double)inOpen[ShadowVeryShortTrailingIdx]))) + (((((double)inClose[ShadowVeryShortTrailingIdx]) >= ((double)inOpen[ShadowVeryShortTrailingIdx])) ? ((double)inOpen[ShadowVeryShortTrailingIdx]) : ((double)inClose[ShadowVeryShortTrailingIdx])) - (double)inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx] - ((((double)inClose[BodyLongTrailingIdx]) >= ((double)inOpen[BodyLongTrailingIdx])) ? ((double)inClose[BodyLongTrailingIdx]) : ((double)inOpen[BodyLongTrailingIdx]))) + (((((double)inClose[BodyLongTrailingIdx]) >= ((double)inOpen[BodyLongTrailingIdx])) ? ((double)inOpen[BodyLongTrailingIdx]) : ((double)inClose[BodyLongTrailingIdx])) - (double)inLow[BodyLongTrailingIdx])) : 0.0)));
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs((double)inClose[EqualTrailingIdx - 1] - (double)inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[EqualTrailingIdx - 1] - (double)inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[EqualTrailingIdx - 1] - ((((double)inClose[EqualTrailingIdx - 1]) >= ((double)inOpen[EqualTrailingIdx - 1])) ? ((double)inClose[EqualTrailingIdx - 1]) : ((double)inOpen[EqualTrailingIdx - 1]))) + (((((double)inClose[EqualTrailingIdx - 1]) >= ((double)inOpen[EqualTrailingIdx - 1])) ? ((double)inOpen[EqualTrailingIdx - 1]) : ((double)inClose[EqualTrailingIdx - 1])) - (double)inLow[EqualTrailingIdx - 1])) : 0.0)));
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A two-candle continuation pattern: the second candle opposes the first in
    * color, opens at the same price as the first, and is a long-bodied belt
    * hold. Bullish (white second candle) or bearish (black second candle)
    * continuation signal.
    * <p><b>Formula</b>
    * <pre>{@code
    * Two consecutive candles i-1, i: (1) opposite colors: color(i-1) == -color(i); (2) same open: open[i-1]-Equal_avg <= open[i] <= open[i-1]+Equal_avg; (3) long body: realbody(i) > BodyLong_avg; (4) belt hold: if i is white, lowershadow(i) < ShadowVeryShort_avg; if i is black, uppershadow(i) < ShadowVeryShort_avg.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A prior trend is not verified, nor that the pattern aligns with it.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLSEPARATINGLINES_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 for a bullish (white second candle) hit, -100 for a
    *        bearish (black second candle) hit, 0 otherwise. Must hold at least
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
    * @see Core#CDLBELTHOLD
    */
   public OutRange CDLSEPARATINGLINES( int startIdx,
                                       int endIdx,
                                       double inOpen[],
                                       double inHigh[],
                                       double inLow[],
                                       double inClose[],
                                       int outInteger[] )
   {
      requireIndexRange("CDLSEPARATINGLINES", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, CDLSEPARATINGLINES_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLSEPARATINGLINES", "inOpen", inOpen, guardInLen);
      requireLength("CDLSEPARATINGLINES", "inHigh", inHigh, guardInLen);
      requireLength("CDLSEPARATINGLINES", "inLow", inLow, guardInLen);
      requireLength("CDLSEPARATINGLINES", "inClose", inClose, guardInLen);
      requireLength("CDLSEPARATINGLINES", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLSEPARATINGLINES_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLSEPARATINGLINES", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A two-candle continuation pattern: the second candle opposes the first in
    * color, opens at the same price as the first, and is a long-bodied belt
    * hold. Bullish (white second candle) or bearish (black second candle)
    * continuation signal.
    * <p><b>Formula</b>
    * <pre>{@code
    * Two consecutive candles i-1, i: (1) opposite colors: color(i-1) == -color(i); (2) same open: open[i-1]-Equal_avg <= open[i] <= open[i-1]+Equal_avg; (3) long body: realbody(i) > BodyLong_avg; (4) belt hold: if i is white, lowershadow(i) < ShadowVeryShort_avg; if i is black, uppershadow(i) < ShadowVeryShort_avg.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A prior trend is not verified, nor that the pattern aligns with it.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLSEPARATINGLINES_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 for a bullish (white second candle) hit, -100 for a
    *        bearish (black second candle) hit, 0 otherwise. Must hold at least
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
    * @see Core#CDLBELTHOLD
    */
   public OutRange CDLSEPARATINGLINES( int startIdx,
                                       int endIdx,
                                       float inOpen[],
                                       float inHigh[],
                                       float inLow[],
                                       float inClose[],
                                       int outInteger[] )
   {
      requireIndexRange("CDLSEPARATINGLINES", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, CDLSEPARATINGLINES_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLSEPARATINGLINES", "inOpen", inOpen, guardInLen);
      requireLength("CDLSEPARATINGLINES", "inHigh", inHigh, guardInLen);
      requireLength("CDLSEPARATINGLINES", "inLow", inLow, guardInLen);
      requireLength("CDLSEPARATINGLINES", "inClose", inClose, guardInLen);
      requireLength("CDLSEPARATINGLINES", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLSEPARATINGLINES_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLSEPARATINGLINES", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLSEPARATINGLINES stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLSEPARATINGLINES} over the same series.
    * Open with {@link Core#CDLSEPARATINGLINES_Open}; there is no close — the handle is
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
   public static final class CDLSEPARATINGLINES_Stream {
      Core core;
      double ShadowVeryShortPeriodTotal;
      double BodyLongPeriodTotal;
      double EqualPeriodTotal;
      double lag1_inOpen;
      double lag1_inHigh;
      double lag1_inLow;
      double lag1_inClose;
      int ringPos_BodyLongTrailingIdx;
      int ringCap_BodyLongTrailingIdx;
      double[] ring_BodyLongTrailingIdx_derived;
      int ringPos_EqualTrailingIdx;
      int ringCap_EqualTrailingIdx;
      int ringLag_EqualTrailingIdx;
      double[] ring_EqualTrailingIdx_derived;
      int ringPos_ShadowVeryShortTrailingIdx;
      int ringCap_ShadowVeryShortTrailingIdx;
      double[] ring_ShadowVeryShortTrailingIdx_derived;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cs_Equal_rangeType;
      int cs_Equal_avgPeriod;
      double cs_Equal_factor;
      int cs_ShadowVeryShort_rangeType;
      int cs_ShadowVeryShort_avgPeriod;
      double cs_ShadowVeryShort_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLSEPARATINGLINES_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLSEPARATINGLINES_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLSEPARATINGLINES_Stream( CDLSEPARATINGLINES_Stream other ) {
         this.core = other.core;
         this.ShadowVeryShortPeriodTotal = other.ShadowVeryShortPeriodTotal;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.EqualPeriodTotal = other.EqualPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         this.ringPos_EqualTrailingIdx = other.ringPos_EqualTrailingIdx;
         this.ringCap_EqualTrailingIdx = other.ringCap_EqualTrailingIdx;
         this.ringLag_EqualTrailingIdx = other.ringLag_EqualTrailingIdx;
         this.ring_EqualTrailingIdx_derived = other.ring_EqualTrailingIdx_derived.clone();
         this.ringPos_ShadowVeryShortTrailingIdx = other.ringPos_ShadowVeryShortTrailingIdx;
         this.ringCap_ShadowVeryShortTrailingIdx = other.ringCap_ShadowVeryShortTrailingIdx;
         this.ring_ShadowVeryShortTrailingIdx_derived = other.ring_ShadowVeryShortTrailingIdx_derived.clone();
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_Equal_rangeType = other.cs_Equal_rangeType;
         this.cs_Equal_avgPeriod = other.cs_Equal_avgPeriod;
         this.cs_Equal_factor = other.cs_Equal_factor;
         this.cs_ShadowVeryShort_rangeType = other.cs_ShadowVeryShort_rangeType;
         this.cs_ShadowVeryShort_avgPeriod = other.cs_ShadowVeryShort_avgPeriod;
         this.cs_ShadowVeryShort_factor = other.cs_ShadowVeryShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDLSEPARATINGLINES_Stream other ) {
         this.core = other.core;
         this.ShadowVeryShortPeriodTotal = other.ShadowVeryShortPeriodTotal;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.EqualPeriodTotal = other.EqualPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         if( this.ring_BodyLongTrailingIdx_derived != null && this.ring_BodyLongTrailingIdx_derived.length == other.ring_BodyLongTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_derived, 0, this.ring_BodyLongTrailingIdx_derived, 0, other.ring_BodyLongTrailingIdx_derived.length );
         } else {
            this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         }
         this.ringPos_EqualTrailingIdx = other.ringPos_EqualTrailingIdx;
         this.ringCap_EqualTrailingIdx = other.ringCap_EqualTrailingIdx;
         this.ringLag_EqualTrailingIdx = other.ringLag_EqualTrailingIdx;
         if( this.ring_EqualTrailingIdx_derived != null && this.ring_EqualTrailingIdx_derived.length == other.ring_EqualTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_EqualTrailingIdx_derived, 0, this.ring_EqualTrailingIdx_derived, 0, other.ring_EqualTrailingIdx_derived.length );
         } else {
            this.ring_EqualTrailingIdx_derived = other.ring_EqualTrailingIdx_derived.clone();
         }
         this.ringPos_ShadowVeryShortTrailingIdx = other.ringPos_ShadowVeryShortTrailingIdx;
         this.ringCap_ShadowVeryShortTrailingIdx = other.ringCap_ShadowVeryShortTrailingIdx;
         if( this.ring_ShadowVeryShortTrailingIdx_derived != null && this.ring_ShadowVeryShortTrailingIdx_derived.length == other.ring_ShadowVeryShortTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_ShadowVeryShortTrailingIdx_derived, 0, this.ring_ShadowVeryShortTrailingIdx_derived, 0, other.ring_ShadowVeryShortTrailingIdx_derived.length );
         } else {
            this.ring_ShadowVeryShortTrailingIdx_derived = other.ring_ShadowVeryShortTrailingIdx_derived.clone();
         }
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_Equal_rangeType = other.cs_Equal_rangeType;
         this.cs_Equal_avgPeriod = other.cs_Equal_avgPeriod;
         this.cs_Equal_factor = other.cs_Equal_factor;
         this.cs_ShadowVeryShort_rangeType = other.cs_ShadowVeryShort_rangeType;
         this.cs_ShadowVeryShort_avgPeriod = other.cs_ShadowVeryShort_avgPeriod;
         this.cs_ShadowVeryShort_factor = other.cs_ShadowVeryShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDLSEPARATINGLINES_Stream> PEEK_SCRATCH = new ThreadLocal<>();

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
            throw new TaLibArgumentException("CDLSEPARATINGLINES update: BadParam", RetCode.BadParam);
         core.CDLSEPARATINGLINES_StreamStep(this, inOpen, inHigh, inLow, inClose);
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
            throw new TaLibArgumentException("CDLSEPARATINGLINES peek: BadParam", RetCode.BadParam);
         CDLSEPARATINGLINES_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLSEPARATINGLINES_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLSEPARATINGLINES_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLSEPARATINGLINES_Stream copy() {
         return new CDLSEPARATINGLINES_Stream(this);
      }
   }
   void CDLSEPARATINGLINES_StreamStep( CDLSEPARATINGLINES_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      int Equal_rangeType = sp.cs_Equal_rangeType;
      int Equal_avgPeriod = sp.cs_Equal_avgPeriod;
      double Equal_factor = sp.cs_Equal_factor;
      int ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
      int ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
      double ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
      if( sp.ringCap_BodyLongTrailingIdx == 0 ) {
         sp.ring_BodyLongTrailingIdx_derived[0] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      }
      sp.ring_EqualTrailingIdx_derived[sp.ringPos_EqualTrailingIdx] = ((Equal_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType == 1) ? (inHigh - inLow) : ((Equal_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( sp.ringCap_ShadowVeryShortTrailingIdx == 0 ) {
         sp.ring_ShadowVeryShortTrailingIdx_derived[0] = ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      }
      if( ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - ((inClose >= inOpen) ? 1 : 0 - 1) && /* opposite candles */
          inOpen <= sp.lag1_inOpen + ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* same open */
          inOpen >= sp.lag1_inOpen - ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) &&
          Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* belt hold: long body */
          (((inClose >= inOpen) ? 1 : 0 - 1) == 1 && (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) || ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0))))) ) /* with no lower shadow if bullish with no upper shadow if bearish */
      {
         sp.cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx];
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx];
      sp.EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_EqualTrailingIdx_derived[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx];
      sp.lag1_inOpen = inOpen;
      sp.lag1_inHigh = inHigh;
      sp.lag1_inLow = inLow;
      sp.lag1_inClose = inClose;
      sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
      if( sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx ) {
         sp.ringPos_BodyLongTrailingIdx = 0;
      }
      sp.ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx + 1;
      if( sp.ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx ) {
         sp.ringPos_EqualTrailingIdx = 0;
      }
      sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
      if( sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx ) {
         sp.ringPos_ShadowVeryShortTrailingIdx = 0;
      }
   }
   private RetCode CDLSEPARATINGLINES_OpenPass( CDLSEPARATINGLINES_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double ShadowVeryShortPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int EqualTrailingIdx = 0;
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
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLSEPARATINGLINES_Lookback();
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
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: black (white) candle
       * - second candle: bullish (bearish) belt hold with the same open as the prior candle
       * The meaning of "long body" and "very short shadow" of the belt hold is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that separating lines is significant when coming in a trend and the belt hold has
       * the same direction of the trend, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) && /* opposite candles */
             inOpen[i] <= inOpen[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* same open */
             inOpen[i] >= inOpen[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* belt hold: long body */
             (((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) || ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0))))) ) /* with no lower shadow if bullish with no upper shadow if bearish */
         {
            outInteger[outIdx++ * outStride] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - (((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inClose[ShadowVeryShortTrailingIdx]) : (inOpen[ShadowVeryShortTrailingIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx]) >= (inOpen[ShadowVeryShortTrailingIdx])) ? (inOpen[ShadowVeryShortTrailingIdx]) : (inClose[ShadowVeryShortTrailingIdx])) - inLow[ShadowVeryShortTrailingIdx])) : 0.0)));
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0)));
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
         EqualTrailingIdx += 1;
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
      double[] capRing_BodyLongTrailingIdx_derived = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_derived[fillJ - (historyLen - cap_BodyLongTrailingIdx)] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int capLag_EqualTrailingIdx = i - EqualTrailingIdx;
      int cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 2;
      if( capLag_EqualTrailingIdx < 0 || cap_EqualTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_EqualTrailingIdx = (cap_EqualTrailingIdx > 0)? cap_EqualTrailingIdx : 1;
      double[] capRing_EqualTrailingIdx_derived = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_derived[fillJ % cap_EqualTrailingIdx] = ((Equal_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Equal_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Equal_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int cap_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
      if( cap_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowVeryShortTrailingIdx = (cap_ShadowVeryShortTrailingIdx > 0)? cap_ShadowVeryShortTrailingIdx : 1;
      double[] capRing_ShadowVeryShortTrailingIdx_derived = new double[allocN_ShadowVeryShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowVeryShortTrailingIdx_derived[fillJ - (historyLen - cap_ShadowVeryShortTrailingIdx)] = ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.EqualPeriodTotal = EqualPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_BodyLongTrailingIdx = 0;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
      sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
      sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
      sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
      sp.ring_EqualTrailingIdx_derived = capRing_EqualTrailingIdx_derived;
      sp.ringPos_ShadowVeryShortTrailingIdx = 0;
      sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
      sp.ring_ShadowVeryShortTrailingIdx_derived = capRing_ShadowVeryShortTrailingIdx_derived;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_Equal_rangeType = Equal_rangeType;
      sp.cs_Equal_avgPeriod = Equal_avgPeriod;
      sp.cs_Equal_factor = Equal_factor;
      sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
      sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
      sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CDLSEPARATINGLINES_OpenImpl( CDLSEPARATINGLINES_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDLSEPARATINGLINES_OpenPass( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDLSEPARATINGLINES_OpenAndFillImpl( CDLSEPARATINGLINES_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDLSEPARATINGLINES_OpenPass( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDLSEPARATINGLINES_OpenAndFillInternalImpl( CDLSEPARATINGLINES_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDLSEPARATINGLINES_OpenPass(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDLSEPARATINGLINES_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLSEPARATINGLINES_Stream CDLSEPARATINGLINES_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLSEPARATINGLINES_Stream sp = new CDLSEPARATINGLINES_Stream(this);
      RetCode retCode = CDLSEPARATINGLINES_OpenAndFillInternalImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLSEPARATINGLINES openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLSEPARATINGLINES openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLSEPARATINGLINES openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind CDLSEPARATINGLINES_Open (composition seam). */
   CDLSEPARATINGLINES_Stream CDLSEPARATINGLINES_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLSEPARATINGLINES_Stream sp = new CDLSEPARATINGLINES_Stream(this);
      RetCode retCode = CDLSEPARATINGLINES_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLSEPARATINGLINES open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLSEPARATINGLINES open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLSEPARATINGLINES open: " + retCode, retCode);
   }
   /**
    * Open a live CDLSEPARATINGLINES stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLSEPARATINGLINES} at that bar.
    * <p>The history must hold at least {@code CDLSEPARATINGLINES_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLSEPARATINGLINES_Stream CDLSEPARATINGLINES_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLSEPARATINGLINES_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLSEPARATINGLINES_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLSEPARATINGLINES} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLSEPARATINGLINES_Stream#fillRange()}.
    */
   public CDLSEPARATINGLINES_Stream CDLSEPARATINGLINES_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLSEPARATINGLINES_Stream sp = new CDLSEPARATINGLINES_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLSEPARATINGLINES_OpenAndFillImpl(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLSEPARATINGLINES openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLSEPARATINGLINES openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLSEPARATINGLINES openAndFill: " + retCode, retCode);
   }
