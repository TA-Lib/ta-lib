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
 *  022705 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDL3STARSINSOUTH} consumes before
    * it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDL3STARSINSOUTH_Lookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      return Math.max(Math.max(ShadowVeryShort_avgPeriod, ShadowLong_avgPeriod), Math.max(BodyLong_avgPeriod, BodyShort_avgPeriod)) + 2 ;

   }
   RetCode CDL3STARSINSOUTH_Impl( int startIdx,
                                  int endIdx,
                                  double inOpen[],
                                  double inHigh[],
                                  double inLow[],
                                  double inClose[],
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  int outInteger[] )
   {
      double BodyLongPeriodTotal = 0;
      double BodyShortPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double[] ShadowVeryShortPeriodTotal = new double[2];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
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
      lookbackTotal = CDL3STARSINSOUTH_Lookback();
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
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortPeriodTotal[0] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      BodyShortPeriodTotal = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
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
       * - first candle: long black candle with long lower shadow
       * - second candle: smaller black candle that opens higher than prior close but within prior candle's range
       *   and trades lower than prior close but not lower than prior low and closes off of its low (it has a shadow)
       * - third candle: small black marubozu (or candle with very short shadows) engulfed by prior candle's range
       * The meanings of "long body", "short body", "very short shadow" are specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100): 3 stars in the south is always bullish;
       * the user should consider that 3 stars in the south is significant when it appears in downtrend, while this function
       * does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* 3rd black */
             Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long */
             (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* with long lower shadow */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) && /* 2nd: smaller candle */
             inOpen[i - 1] > inClose[i - 2] &&
             inOpen[i - 1] <= inHigh[i - 2] &&                           /* that opens higher but within 1st range */
             inLow[i - 1] < inClose[i - 2] &&                            /* and trades lower than 1st close */
             inLow[i - 1] >= inLow[i - 2] &&                             /* but not lower than 1st low */
             (((inClose[i - 1] >= inOpen[i - 1]) ? inOpen[i - 1] : inClose[i - 1]) - inLow[i - 1]) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* and has a lower shadow */
             Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd: small marubozu */
             (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             inLow[i] > inLow[i - 1] &&
             inHigh[i] < inHigh[i - 1] )                                 /* engulfed by prior candle's range */
         {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 2] - inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 2] - inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 2] - (((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inClose[BodyLongTrailingIdx - 2]) : (inOpen[BodyLongTrailingIdx - 2]))) + ((((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inOpen[BodyLongTrailingIdx - 2]) : (inClose[BodyLongTrailingIdx - 2])) - inLow[BodyLongTrailingIdx - 2])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - 2] - inOpen[ShadowLongTrailingIdx - 2])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx - 2] - inLow[ShadowLongTrailingIdx - 2]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx - 2] - (((inClose[ShadowLongTrailingIdx - 2]) >= (inOpen[ShadowLongTrailingIdx - 2])) ? (inClose[ShadowLongTrailingIdx - 2]) : (inOpen[ShadowLongTrailingIdx - 2]))) + ((((inClose[ShadowLongTrailingIdx - 2]) >= (inOpen[ShadowLongTrailingIdx - 2])) ? (inOpen[ShadowLongTrailingIdx - 2]) : (inClose[ShadowLongTrailingIdx - 2])) - inLow[ShadowLongTrailingIdx - 2])) : 0.0)));
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - (((inClose[ShadowVeryShortTrailingIdx - totIdx]) >= (inOpen[ShadowVeryShortTrailingIdx - totIdx])) ? (inClose[ShadowVeryShortTrailingIdx - totIdx]) : (inOpen[ShadowVeryShortTrailingIdx - totIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx - totIdx]) >= (inOpen[ShadowVeryShortTrailingIdx - totIdx])) ? (inOpen[ShadowVeryShortTrailingIdx - totIdx]) : (inClose[ShadowVeryShortTrailingIdx - totIdx])) - inLow[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDL3STARSINSOUTH_Impl( int startIdx,
                                  int endIdx,
                                  float inOpen[],
                                  float inHigh[],
                                  float inLow[],
                                  float inClose[],
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  int outInteger[] )
   {
      double BodyLongPeriodTotal = 0;
      double BodyShortPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double[] ShadowVeryShortPeriodTotal = new double[2];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDL3STARSINSOUTH_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortPeriodTotal[0] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      BodyShortPeriodTotal = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
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
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inOpen[i - 2] : (double)inClose[i - 2]) - (double)inLow[i - 2]) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) && (double)inOpen[i - 1] > (double)inClose[i - 2] && (double)inOpen[i - 1] <= (double)inHigh[i - 2] && (double)inLow[i - 1] < (double)inClose[i - 2] && (double)inLow[i - 1] >= (double)inLow[i - 2] && ((((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inOpen[i - 1] : (double)inClose[i - 1]) - (double)inLow[i - 1]) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (double)inLow[i] > (double)inLow[i - 1] && (double)inHigh[i] < (double)inHigh[i - 1] ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 2] - (double)inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 2] - (double)inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 2] - ((((double)inClose[BodyLongTrailingIdx - 2]) >= ((double)inOpen[BodyLongTrailingIdx - 2])) ? ((double)inClose[BodyLongTrailingIdx - 2]) : ((double)inOpen[BodyLongTrailingIdx - 2]))) + (((((double)inClose[BodyLongTrailingIdx - 2]) >= ((double)inOpen[BodyLongTrailingIdx - 2])) ? ((double)inOpen[BodyLongTrailingIdx - 2]) : ((double)inClose[BodyLongTrailingIdx - 2])) - (double)inLow[BodyLongTrailingIdx - 2])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowLongTrailingIdx - 2] - (double)inOpen[ShadowLongTrailingIdx - 2])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[ShadowLongTrailingIdx - 2] - (double)inLow[ShadowLongTrailingIdx - 2]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[ShadowLongTrailingIdx - 2] - ((((double)inClose[ShadowLongTrailingIdx - 2]) >= ((double)inOpen[ShadowLongTrailingIdx - 2])) ? ((double)inClose[ShadowLongTrailingIdx - 2]) : ((double)inOpen[ShadowLongTrailingIdx - 2]))) + (((((double)inClose[ShadowLongTrailingIdx - 2]) >= ((double)inOpen[ShadowLongTrailingIdx - 2])) ? ((double)inOpen[ShadowLongTrailingIdx - 2]) : ((double)inClose[ShadowLongTrailingIdx - 2])) - (double)inLow[ShadowLongTrailingIdx - 2])) : 0.0)));
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - ((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inClose[i - totIdx]) : ((double)inOpen[i - totIdx]))) + (((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inOpen[i - totIdx]) : ((double)inClose[i - totIdx])) - (double)inLow[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx - totIdx] - (double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx - totIdx] - (double)inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx - totIdx] - ((((double)inClose[ShadowVeryShortTrailingIdx - totIdx]) >= ((double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) ? ((double)inClose[ShadowVeryShortTrailingIdx - totIdx]) : ((double)inOpen[ShadowVeryShortTrailingIdx - totIdx]))) + (((((double)inClose[ShadowVeryShortTrailingIdx - totIdx]) >= ((double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) ? ((double)inOpen[ShadowVeryShortTrailingIdx - totIdx]) : ((double)inClose[ShadowVeryShortTrailingIdx - totIdx])) - (double)inLow[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx] - ((((double)inClose[BodyShortTrailingIdx]) >= ((double)inOpen[BodyShortTrailingIdx])) ? ((double)inClose[BodyShortTrailingIdx]) : ((double)inOpen[BodyShortTrailingIdx]))) + (((((double)inClose[BodyShortTrailingIdx]) >= ((double)inOpen[BodyShortTrailingIdx])) ? ((double)inOpen[BodyShortTrailingIdx]) : ((double)inClose[BodyShortTrailingIdx])) - (double)inLow[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A three-candle bullish reversal pattern of three consecutive black candles
    * that progressively shrink and stabilize: a long black candle with a long
    * lower shadow, a smaller black candle probing lower, then a small black
    * marubozu contained within the second candle's range. A hit signals a
    * bullish reversal; per the code comment it is meaningful in a downtrend,
    * but the function does not verify prior trend.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior downtrend the pattern classically assumes for significance.</li>
    * <li>Thomas Bulkowski's statistical study found this has the best reversal rate of the 103 candlestick patterns he tracked (86% bullish reversal) — but that rests on just 9 occurrences in 4.7 million candle lines, and its overall post-breakout performance ranks dead last, 103rd of 103. ([thepatternsite.com](https://thepatternsite.com/ThreeStarsSouth.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDL3STARSINSOUTH_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 on the bar where the pattern completes (always
    *        bullish), 0 otherwise. Never emits -100. Must hold at least
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
    * @see Core#CDL3BLACKCROWS
    * @see Core#CDLIDENTICAL3CROWS
    * @see Core#CDL3WHITESOLDIERS
    */
   public OutRange CDL3STARSINSOUTH( int startIdx,
                                     int endIdx,
                                     double inOpen[],
                                     double inHigh[],
                                     double inLow[],
                                     double inClose[],
                                     int outInteger[] )
   {
      requireIndexRange("CDL3STARSINSOUTH", startIdx, endIdx);
      int guardStart = clampedStart("CDL3STARSINSOUTH", startIdx, CDL3STARSINSOUTH_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDL3STARSINSOUTH", "inOpen", inOpen, guardInLen);
      requireLength("CDL3STARSINSOUTH", "inHigh", inHigh, guardInLen);
      requireLength("CDL3STARSINSOUTH", "inLow", inLow, guardInLen);
      requireLength("CDL3STARSINSOUTH", "inClose", inClose, guardInLen);
      requireLength("CDL3STARSINSOUTH", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDL3STARSINSOUTH_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDL3STARSINSOUTH", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A three-candle bullish reversal pattern of three consecutive black candles
    * that progressively shrink and stabilize: a long black candle with a long
    * lower shadow, a smaller black candle probing lower, then a small black
    * marubozu contained within the second candle's range. A hit signals a
    * bullish reversal; per the code comment it is meaningful in a downtrend,
    * but the function does not verify prior trend.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior downtrend the pattern classically assumes for significance.</li>
    * <li>Thomas Bulkowski's statistical study found this has the best reversal rate of the 103 candlestick patterns he tracked (86% bullish reversal) — but that rests on just 9 occurrences in 4.7 million candle lines, and its overall post-breakout performance ranks dead last, 103rd of 103. ([thepatternsite.com](https://thepatternsite.com/ThreeStarsSouth.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDL3STARSINSOUTH_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 on the bar where the pattern completes (always
    *        bullish), 0 otherwise. Never emits -100. Must hold at least
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
    * @see Core#CDL3BLACKCROWS
    * @see Core#CDLIDENTICAL3CROWS
    * @see Core#CDL3WHITESOLDIERS
    */
   public OutRange CDL3STARSINSOUTH( int startIdx,
                                     int endIdx,
                                     float inOpen[],
                                     float inHigh[],
                                     float inLow[],
                                     float inClose[],
                                     int outInteger[] )
   {
      requireIndexRange("CDL3STARSINSOUTH", startIdx, endIdx);
      int guardStart = clampedStart("CDL3STARSINSOUTH", startIdx, CDL3STARSINSOUTH_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDL3STARSINSOUTH", "inOpen", inOpen, guardInLen);
      requireLength("CDL3STARSINSOUTH", "inHigh", inHigh, guardInLen);
      requireLength("CDL3STARSINSOUTH", "inLow", inLow, guardInLen);
      requireLength("CDL3STARSINSOUTH", "inClose", inClose, guardInLen);
      requireLength("CDL3STARSINSOUTH", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDL3STARSINSOUTH_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDL3STARSINSOUTH", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDL3STARSINSOUTH stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDL3STARSINSOUTH} over the same series.
    * Open with {@link Core#CDL3STARSINSOUTH_Open}; there is no close — the handle is
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
   public static final class CDL3STARSINSOUTH_Stream {
      Core core;
      double BodyLongPeriodTotal;
      double BodyShortPeriodTotal;
      double ShadowLongPeriodTotal;
      double[] ShadowVeryShortPeriodTotal;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag1_inClose;
      double lag2_inClose;
      int ringPos_BodyLongTrailingIdx;
      int ringCap_BodyLongTrailingIdx;
      int ringLag_BodyLongTrailingIdx;
      double[] ring_BodyLongTrailingIdx_derived;
      int ringPos_BodyShortTrailingIdx;
      int ringCap_BodyShortTrailingIdx;
      double[] ring_BodyShortTrailingIdx_derived;
      int ringPos_ShadowLongTrailingIdx;
      int ringCap_ShadowLongTrailingIdx;
      int ringLag_ShadowLongTrailingIdx;
      double[] ring_ShadowLongTrailingIdx_derived;
      int ringPos_ShadowVeryShortTrailingIdx;
      int ringCap_ShadowVeryShortTrailingIdx;
      int ringLag_ShadowVeryShortTrailingIdx;
      double[] ring_ShadowVeryShortTrailingIdx_derived;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cs_BodyShort_rangeType;
      int cs_BodyShort_avgPeriod;
      double cs_BodyShort_factor;
      int cs_ShadowLong_rangeType;
      int cs_ShadowLong_avgPeriod;
      double cs_ShadowLong_factor;
      int cs_ShadowVeryShort_rangeType;
      int cs_ShadowVeryShort_avgPeriod;
      double cs_ShadowVeryShort_factor;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      CDL3STARSINSOUTH_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDL3STARSINSOUTH} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CDL3STARSINSOUTH_Stream( CDL3STARSINSOUTH_Stream other ) {
         this.core = other.core;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.BodyShortPeriodTotal = other.BodyShortPeriodTotal;
         this.ShadowLongPeriodTotal = other.ShadowLongPeriodTotal;
         this.ShadowVeryShortPeriodTotal = other.ShadowVeryShortPeriodTotal.clone();
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ringLag_BodyLongTrailingIdx = other.ringLag_BodyLongTrailingIdx;
         this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         this.ringPos_BodyShortTrailingIdx = other.ringPos_BodyShortTrailingIdx;
         this.ringCap_BodyShortTrailingIdx = other.ringCap_BodyShortTrailingIdx;
         this.ring_BodyShortTrailingIdx_derived = other.ring_BodyShortTrailingIdx_derived.clone();
         this.ringPos_ShadowLongTrailingIdx = other.ringPos_ShadowLongTrailingIdx;
         this.ringCap_ShadowLongTrailingIdx = other.ringCap_ShadowLongTrailingIdx;
         this.ringLag_ShadowLongTrailingIdx = other.ringLag_ShadowLongTrailingIdx;
         this.ring_ShadowLongTrailingIdx_derived = other.ring_ShadowLongTrailingIdx_derived.clone();
         this.ringPos_ShadowVeryShortTrailingIdx = other.ringPos_ShadowVeryShortTrailingIdx;
         this.ringCap_ShadowVeryShortTrailingIdx = other.ringCap_ShadowVeryShortTrailingIdx;
         this.ringLag_ShadowVeryShortTrailingIdx = other.ringLag_ShadowVeryShortTrailingIdx;
         this.ring_ShadowVeryShortTrailingIdx_derived = other.ring_ShadowVeryShortTrailingIdx_derived.clone();
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cs_ShadowLong_rangeType = other.cs_ShadowLong_rangeType;
         this.cs_ShadowLong_avgPeriod = other.cs_ShadowLong_avgPeriod;
         this.cs_ShadowLong_factor = other.cs_ShadowLong_factor;
         this.cs_ShadowVeryShort_rangeType = other.cs_ShadowVeryShort_rangeType;
         this.cs_ShadowVeryShort_avgPeriod = other.cs_ShadowVeryShort_avgPeriod;
         this.cs_ShadowVeryShort_factor = other.cs_ShadowVeryShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( CDL3STARSINSOUTH_Stream other ) {
         this.core = other.core;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.BodyShortPeriodTotal = other.BodyShortPeriodTotal;
         this.ShadowLongPeriodTotal = other.ShadowLongPeriodTotal;
         if( this.ShadowVeryShortPeriodTotal != null && this.ShadowVeryShortPeriodTotal.length == other.ShadowVeryShortPeriodTotal.length ) {
            System.arraycopy( other.ShadowVeryShortPeriodTotal, 0, this.ShadowVeryShortPeriodTotal, 0, other.ShadowVeryShortPeriodTotal.length );
         } else {
            this.ShadowVeryShortPeriodTotal = other.ShadowVeryShortPeriodTotal.clone();
         }
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
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
         this.ringPos_ShadowLongTrailingIdx = other.ringPos_ShadowLongTrailingIdx;
         this.ringCap_ShadowLongTrailingIdx = other.ringCap_ShadowLongTrailingIdx;
         this.ringLag_ShadowLongTrailingIdx = other.ringLag_ShadowLongTrailingIdx;
         if( this.ring_ShadowLongTrailingIdx_derived != null && this.ring_ShadowLongTrailingIdx_derived.length == other.ring_ShadowLongTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_ShadowLongTrailingIdx_derived, 0, this.ring_ShadowLongTrailingIdx_derived, 0, other.ring_ShadowLongTrailingIdx_derived.length );
         } else {
            this.ring_ShadowLongTrailingIdx_derived = other.ring_ShadowLongTrailingIdx_derived.clone();
         }
         this.ringPos_ShadowVeryShortTrailingIdx = other.ringPos_ShadowVeryShortTrailingIdx;
         this.ringCap_ShadowVeryShortTrailingIdx = other.ringCap_ShadowVeryShortTrailingIdx;
         this.ringLag_ShadowVeryShortTrailingIdx = other.ringLag_ShadowVeryShortTrailingIdx;
         if( this.ring_ShadowVeryShortTrailingIdx_derived != null && this.ring_ShadowVeryShortTrailingIdx_derived.length == other.ring_ShadowVeryShortTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_ShadowVeryShortTrailingIdx_derived, 0, this.ring_ShadowVeryShortTrailingIdx_derived, 0, other.ring_ShadowVeryShortTrailingIdx_derived.length );
         } else {
            this.ring_ShadowVeryShortTrailingIdx_derived = other.ring_ShadowVeryShortTrailingIdx_derived.clone();
         }
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cs_ShadowLong_rangeType = other.cs_ShadowLong_rangeType;
         this.cs_ShadowLong_avgPeriod = other.cs_ShadowLong_avgPeriod;
         this.cs_ShadowLong_factor = other.cs_ShadowLong_factor;
         this.cs_ShadowVeryShort_rangeType = other.cs_ShadowVeryShort_rangeType;
         this.cs_ShadowVeryShort_avgPeriod = other.cs_ShadowVeryShort_avgPeriod;
         this.cs_ShadowVeryShort_factor = other.cs_ShadowVeryShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDL3STARSINSOUTH_Stream> PEEK_SCRATCH = new ThreadLocal<>();

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
            throw new TaLibArgumentException("CDL3STARSINSOUTH update: BadParam", RetCode.BadParam);
         core.CDL3STARSINSOUTH_StepImpl(this, inOpen, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outInteger;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inOpen.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] ) {
         requireArgument("CDL3STARSINSOUTH updateAndFill", "inOpen", inOpen);
         requireArgument("CDL3STARSINSOUTH updateAndFill", "inHigh", inHigh);
         requireArgument("CDL3STARSINSOUTH updateAndFill", "inLow", inLow);
         requireArgument("CDL3STARSINSOUTH updateAndFill", "inClose", inClose);
         requireArgument("CDL3STARSINSOUTH updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDL3STARSINSOUTH updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("CDL3STARSINSOUTH updateAndFill: BadParam", RetCode.BadParam);
            core.CDL3STARSINSOUTH_StepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
            outInteger[i] = this.cur_outInteger;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
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
            throw new TaLibArgumentException("CDL3STARSINSOUTH peek: BadParam", RetCode.BadParam);
         CDL3STARSINSOUTH_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDL3STARSINSOUTH_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDL3STARSINSOUTH_StepImpl(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDL3STARSINSOUTH_Stream copy() {
         return new CDL3STARSINSOUTH_Stream(this);
      }
   }
   void CDL3STARSINSOUTH_StepImpl( CDL3STARSINSOUTH_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int totIdx = 0;
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      int BodyShort_rangeType = sp.cs_BodyShort_rangeType;
      int BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
      double BodyShort_factor = sp.cs_BodyShort_factor;
      int ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
      int ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
      double ShadowLong_factor = sp.cs_ShadowLong_factor;
      int ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
      int ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
      double ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
      sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( sp.ringCap_BodyShortTrailingIdx == 0 ) {
         sp.ring_BodyShortTrailingIdx_derived[0] = ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      }
      sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx] = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ring_ShadowVeryShortTrailingIdx_derived[sp.ringPos_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 &&                 /* 3rd black */
          Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long */
          (((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inOpen : sp.lag2_inClose) - sp.lag2_inLow) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowLong_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* with long lower shadow */
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) && /* 2nd: smaller candle */
          sp.lag1_inOpen > sp.lag2_inClose &&
          sp.lag1_inOpen <= sp.lag2_inHigh &&                           /* that opens higher but within 1st range */
          sp.lag1_inLow < sp.lag2_inClose &&                            /* and trades lower than 1st close */
          sp.lag1_inLow >= sp.lag2_inLow &&                             /* but not lower than 1st low */
          (((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inOpen : sp.lag1_inClose) - sp.lag1_inLow) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* and has a lower shadow */
          Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd: small marubozu */
          (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          inLow > sp.lag1_inLow &&
          inHigh < sp.lag1_inHigh )                                     /* engulfed by prior candle's range */
      {
         sp.cur_outInteger = 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx];
      sp.ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowLong_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0))) - sp.ring_ShadowLongTrailingIdx_derived[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - 2) % sp.ringCap_ShadowLongTrailingIdx];
      for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
         sp.ShadowVeryShortPeriodTotal[totIdx] = sp.ShadowVeryShortPeriodTotal[totIdx] + (sp.ring_ShadowVeryShortTrailingIdx_derived[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - totIdx >= sp.ringCap_ShadowVeryShortTrailingIdx) ? sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - totIdx - sp.ringCap_ShadowVeryShortTrailingIdx : sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - totIdx] - sp.ring_ShadowVeryShortTrailingIdx_derived[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx]);
      }
      sp.BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyShortTrailingIdx_derived[sp.ringPos_BodyShortTrailingIdx];
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
      sp.lag2_inClose = sp.lag1_inClose;
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
      sp.ringPos_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx + 1;
      if( sp.ringPos_ShadowLongTrailingIdx >= sp.ringCap_ShadowLongTrailingIdx ) {
         sp.ringPos_ShadowLongTrailingIdx = 0;
      }
      sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
      if( sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx ) {
         sp.ringPos_ShadowVeryShortTrailingIdx = 0;
      }
   }
   private RetCode CDL3STARSINSOUTH_OpenImpl( CDL3STARSINSOUTH_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double BodyLongPeriodTotal = 0;
      double BodyShortPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double[] ShadowVeryShortPeriodTotal = new double[2];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDL3STARSINSOUTH_Lookback();
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
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortPeriodTotal[0] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      BodyShortPeriodTotal = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
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
       * - first candle: long black candle with long lower shadow
       * - second candle: smaller black candle that opens higher than prior close but within prior candle's range
       *   and trades lower than prior close but not lower than prior low and closes off of its low (it has a shadow)
       * - third candle: small black marubozu (or candle with very short shadows) engulfed by prior candle's range
       * The meanings of "long body", "short body", "very short shadow" are specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100): 3 stars in the south is always bullish;
       * the user should consider that 3 stars in the south is significant when it appears in downtrend, while this function
       * does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* 3rd black */
             Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long */
             (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* with long lower shadow */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) && /* 2nd: smaller candle */
             inOpen[i - 1] > inClose[i - 2] &&
             inOpen[i - 1] <= inHigh[i - 2] &&                           /* that opens higher but within 1st range */
             inLow[i - 1] < inClose[i - 2] &&                            /* and trades lower than 1st close */
             inLow[i - 1] >= inLow[i - 2] &&                             /* but not lower than 1st low */
             (((inClose[i - 1] >= inOpen[i - 1]) ? inOpen[i - 1] : inClose[i - 1]) - inLow[i - 1]) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* and has a lower shadow */
             Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd: small marubozu */
             (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             inLow[i] > inLow[i - 1] &&
             inHigh[i] < inHigh[i - 1] )                                 /* engulfed by prior candle's range */
         {
            outInteger[outIdx++ * outStride] = 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 2] - inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 2] - inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 2] - (((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inClose[BodyLongTrailingIdx - 2]) : (inOpen[BodyLongTrailingIdx - 2]))) + ((((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inOpen[BodyLongTrailingIdx - 2]) : (inClose[BodyLongTrailingIdx - 2])) - inLow[BodyLongTrailingIdx - 2])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - 2] - inOpen[ShadowLongTrailingIdx - 2])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx - 2] - inLow[ShadowLongTrailingIdx - 2]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx - 2] - (((inClose[ShadowLongTrailingIdx - 2]) >= (inOpen[ShadowLongTrailingIdx - 2])) ? (inClose[ShadowLongTrailingIdx - 2]) : (inOpen[ShadowLongTrailingIdx - 2]))) + ((((inClose[ShadowLongTrailingIdx - 2]) >= (inOpen[ShadowLongTrailingIdx - 2])) ? (inOpen[ShadowLongTrailingIdx - 2]) : (inClose[ShadowLongTrailingIdx - 2])) - inLow[ShadowLongTrailingIdx - 2])) : 0.0)));
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - (((inClose[ShadowVeryShortTrailingIdx - totIdx]) >= (inOpen[ShadowVeryShortTrailingIdx - totIdx])) ? (inClose[ShadowVeryShortTrailingIdx - totIdx]) : (inOpen[ShadowVeryShortTrailingIdx - totIdx]))) + ((((inClose[ShadowVeryShortTrailingIdx - totIdx]) >= (inOpen[ShadowVeryShortTrailingIdx - totIdx])) ? (inOpen[ShadowVeryShortTrailingIdx - totIdx]) : (inClose[ShadowVeryShortTrailingIdx - totIdx])) - inLow[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - (((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inClose[BodyShortTrailingIdx]) : (inOpen[BodyShortTrailingIdx]))) + ((((inClose[BodyShortTrailingIdx]) >= (inOpen[BodyShortTrailingIdx])) ? (inOpen[BodyShortTrailingIdx]) : (inClose[BodyShortTrailingIdx])) - inLow[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      int cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 3;
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
      int capLag_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
      int cap_ShadowLongTrailingIdx = capLag_ShadowLongTrailingIdx + 3;
      if( capLag_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowLongTrailingIdx = (cap_ShadowLongTrailingIdx > 0)? cap_ShadowLongTrailingIdx : 1;
      double[] capRing_ShadowLongTrailingIdx_derived = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_derived[fillJ % cap_ShadowLongTrailingIdx] = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowLong_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowLong_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int capLag_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
      int cap_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx + 2;
      if( capLag_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowVeryShortTrailingIdx = (cap_ShadowVeryShortTrailingIdx > 0)? cap_ShadowVeryShortTrailingIdx : 1;
      double[] capRing_ShadowVeryShortTrailingIdx_derived = new double[allocN_ShadowVeryShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowVeryShortTrailingIdx_derived[fillJ % cap_ShadowVeryShortTrailingIdx] = ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.BodyShortPeriodTotal = BodyShortPeriodTotal;
      sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
      sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
      sp.ringPos_BodyShortTrailingIdx = 0;
      sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
      sp.ring_BodyShortTrailingIdx_derived = capRing_BodyShortTrailingIdx_derived;
      sp.ringPos_ShadowLongTrailingIdx = historyLen % cap_ShadowLongTrailingIdx;
      sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
      sp.ringLag_ShadowLongTrailingIdx = capLag_ShadowLongTrailingIdx;
      sp.ring_ShadowLongTrailingIdx_derived = capRing_ShadowLongTrailingIdx_derived;
      sp.ringPos_ShadowVeryShortTrailingIdx = historyLen % cap_ShadowVeryShortTrailingIdx;
      sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
      sp.ringLag_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx;
      sp.ring_ShadowVeryShortTrailingIdx_derived = capRing_ShadowVeryShortTrailingIdx_derived;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cs_ShadowLong_rangeType = ShadowLong_rangeType;
      sp.cs_ShadowLong_avgPeriod = ShadowLong_avgPeriod;
      sp.cs_ShadowLong_factor = ShadowLong_factor;
      sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
      sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
      sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* CDL3STARSINSOUTH_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDL3STARSINSOUTH_Stream CDL3STARSINSOUTH_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDL3STARSINSOUTH_Stream sp = new CDL3STARSINSOUTH_Stream(this);
      RetCode retCode = CDL3STARSINSOUTH_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDL3STARSINSOUTH openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDL3STARSINSOUTH openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDL3STARSINSOUTH openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind CDL3STARSINSOUTH_Open (composition seam). */
   CDL3STARSINSOUTH_Stream CDL3STARSINSOUTH_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDL3STARSINSOUTH_Stream sp = new CDL3STARSINSOUTH_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = CDL3STARSINSOUTH_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDL3STARSINSOUTH open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDL3STARSINSOUTH open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDL3STARSINSOUTH open: " + retCode, retCode);
   }
   /**
    * Open a live CDL3STARSINSOUTH stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDL3STARSINSOUTH} at that bar.
    * <p>The history must hold at least {@code CDL3STARSINSOUTH_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CDL3STARSINSOUTH_Stream CDL3STARSINSOUTH_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDL3STARSINSOUTH open", "inOpen", inOpen);
      requireHistory("CDL3STARSINSOUTH open", inOpen.length);
      requireArgument("CDL3STARSINSOUTH open", "inHigh", inHigh);
      requireArgument("CDL3STARSINSOUTH open", "inLow", inLow);
      requireArgument("CDL3STARSINSOUTH open", "inClose", inClose);
      requireHistoryLength("CDL3STARSINSOUTH open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDL3STARSINSOUTH open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDL3STARSINSOUTH open", "inClose", inClose.length, inOpen.length);
      return CDL3STARSINSOUTH_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDL3STARSINSOUTH_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDL3STARSINSOUTH} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CDL3STARSINSOUTH_Stream#outRange()}.
    */
   public CDL3STARSINSOUTH_Stream CDL3STARSINSOUTH_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDL3STARSINSOUTH openAndFill", "inOpen", inOpen);
      requireHistory("CDL3STARSINSOUTH openAndFill", inOpen.length);
      requireArgument("CDL3STARSINSOUTH openAndFill", "inHigh", inHigh);
      requireArgument("CDL3STARSINSOUTH openAndFill", "inLow", inLow);
      requireArgument("CDL3STARSINSOUTH openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDL3STARSINSOUTH openAndFill", inOpen.length, CDL3STARSINSOUTH_Lookback());
      requireHistoryLength("CDL3STARSINSOUTH openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDL3STARSINSOUTH openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDL3STARSINSOUTH openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDL3STARSINSOUTH openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDL3STARSINSOUTH openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return CDL3STARSINSOUTH_OpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
