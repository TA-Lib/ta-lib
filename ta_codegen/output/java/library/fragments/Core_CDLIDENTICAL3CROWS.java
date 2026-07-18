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
 *  103104 AC   Creation
 */

   public int cdlIdentical3CrowsLookback( )
   {
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      return Math.max(ShadowVeryShort_avgPeriod, Equal_avgPeriod) + 2 ;

   }
   public RetCode cdlIdentical3Crows( int startIdx,
                                      int endIdx,
                                      double inOpen[],
                                      double inHigh[],
                                      double inLow[],
                                      double inClose[],
                                      MInteger outBegIdx,
                                      MInteger outNBElement,
                                      int outInteger[] )
   {
      double[] ShadowVeryShortPeriodTotal = new double[3];
      double[] EqualPeriodTotal = new double[3];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlIdentical3CrowsLookback();
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
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortPeriodTotal[0] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      EqualPeriodTotal[2] = 0;
      EqualPeriodTotal[1] = 0;
      EqualPeriodTotal[0] = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal[2] = EqualPeriodTotal[2] + ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         EqualPeriodTotal[1] = EqualPeriodTotal[1] + ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - three consecutive and declining black candlesticks
       * - each candle must have no or very short lower shadow
       * - each candle after the first must open at or very close to the prior candle's close
       * The meaning of "very short" is specified with TA_SetCandleSettings;
       * the meaning of "very close" is specified with TA_SetCandleSettings (Equal);
       * outInteger is negative (-1 to -100): identical three crows is always bearish;
       * the user should consider that identical 3 crows is significant when it appears after a mature advance or at high levels,
       * while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
             (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
             (((inClose[i - 1] >= inOpen[i - 1]) ? inOpen[i - 1] : inClose[i - 1]) - inLow[i - 1]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* 3rd black */
             (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
             inClose[i - 2] > inClose[i - 1] &&                          /* three declining */
             inClose[i - 1] > inClose[i] &&
             inOpen[i - 1] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd black opens very close to 1st close */
             inOpen[i - 1] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd black opens very close to 2nd close */
             inOpen[i] >= inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            EqualPeriodTotal[totIdx] = EqualPeriodTotal[totIdx] + (((Equal_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Equal_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Equal_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - totIdx] - inOpen[EqualTrailingIdx - totIdx])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - totIdx] - inLow[EqualTrailingIdx - totIdx]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - totIdx] - inLow[EqualTrailingIdx - totIdx]) - Math.abs(inClose[EqualTrailingIdx - totIdx] - inOpen[EqualTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlIdentical3CrowsUnguarded( int startIdx,
                                               int endIdx,
                                               double inOpen[],
                                               double inHigh[],
                                               double inLow[],
                                               double inClose[],
                                               MInteger outBegIdx,
                                               MInteger outNBElement,
                                               int outInteger[] )
   {
      double[] ShadowVeryShortPeriodTotal = new double[3];
      double[] EqualPeriodTotal = new double[3];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      lookbackTotal = cdlIdentical3CrowsLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortPeriodTotal[0] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      EqualPeriodTotal[2] = 0;
      EqualPeriodTotal[1] = 0;
      EqualPeriodTotal[0] = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal[2] = EqualPeriodTotal[2] + ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         EqualPeriodTotal[1] = EqualPeriodTotal[1] + ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((inClose[i - 1] >= inOpen[i - 1]) ? inOpen[i - 1] : inClose[i - 1]) - inLow[i - 1]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && inClose[i - 2] > inClose[i - 1] && inClose[i - 1] > inClose[i] && inOpen[i - 1] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && inOpen[i - 1] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && inOpen[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && inOpen[i] >= inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            EqualPeriodTotal[totIdx] = EqualPeriodTotal[totIdx] + (((Equal_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Equal_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Equal_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - totIdx] - inOpen[EqualTrailingIdx - totIdx])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - totIdx] - inLow[EqualTrailingIdx - totIdx]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - totIdx] - inLow[EqualTrailingIdx - totIdx]) - Math.abs(inClose[EqualTrailingIdx - totIdx] - inOpen[EqualTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlIdentical3Crows( int startIdx,
                                      int endIdx,
                                      float inOpen[],
                                      float inHigh[],
                                      float inLow[],
                                      float inClose[],
                                      MInteger outBegIdx,
                                      MInteger outNBElement,
                                      int outInteger[] )
   {
      double[] ShadowVeryShortPeriodTotal = new double[3];
      double[] EqualPeriodTotal = new double[3];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlIdentical3CrowsLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortPeriodTotal[0] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      EqualPeriodTotal[2] = 0;
      EqualPeriodTotal[1] = 0;
      EqualPeriodTotal[0] = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal[2] = EqualPeriodTotal[2] + ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         EqualPeriodTotal[1] = EqualPeriodTotal[1] + ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && ((((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inOpen[i - 2] : (double)inClose[i - 2]) - (double)inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inOpen[i - 1] : (double)inClose[i - 1]) - (double)inLow[i - 1]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (double)inClose[i - 2] > (double)inClose[i - 1] && (double)inClose[i - 1] > (double)inClose[i] && (double)inOpen[i - 1] <= (double)inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] >= (double)inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] <= (double)inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] >= (double)inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx - totIdx] - (double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx - totIdx] - (double)inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx - totIdx] - (double)inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs((double)inClose[ShadowVeryShortTrailingIdx - totIdx] - (double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            EqualPeriodTotal[totIdx] = EqualPeriodTotal[totIdx] + (((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs((double)inClose[EqualTrailingIdx - totIdx] - (double)inOpen[EqualTrailingIdx - totIdx])) : ((Equal_rangeType == 1) ? ((double)inHigh[EqualTrailingIdx - totIdx] - (double)inLow[EqualTrailingIdx - totIdx]) : ((Equal_rangeType == 2) ? (((double)inHigh[EqualTrailingIdx - totIdx] - (double)inLow[EqualTrailingIdx - totIdx]) - Math.abs((double)inClose[EqualTrailingIdx - totIdx] - (double)inOpen[EqualTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlIdentical3CrowsUnguarded( int startIdx,
                                               int endIdx,
                                               float inOpen[],
                                               float inHigh[],
                                               float inLow[],
                                               float inClose[],
                                               MInteger outBegIdx,
                                               MInteger outNBElement,
                                               int outInteger[] )
   {
      double[] ShadowVeryShortPeriodTotal = new double[3];
      double[] EqualPeriodTotal = new double[3];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      lookbackTotal = cdlIdentical3CrowsLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortPeriodTotal[0] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      EqualPeriodTotal[2] = 0;
      EqualPeriodTotal[1] = 0;
      EqualPeriodTotal[0] = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal[2] = EqualPeriodTotal[2] + ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         EqualPeriodTotal[1] = EqualPeriodTotal[1] + ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && ((((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inOpen[i - 2] : (double)inClose[i - 2]) - (double)inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inOpen[i - 1] : (double)inClose[i - 1]) - (double)inLow[i - 1]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (double)inClose[i - 2] > (double)inClose[i - 1] && (double)inClose[i - 1] > (double)inClose[i] && (double)inOpen[i - 1] <= (double)inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] >= (double)inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] <= (double)inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] >= (double)inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx - totIdx] - (double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx - totIdx] - (double)inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx - totIdx] - (double)inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs((double)inClose[ShadowVeryShortTrailingIdx - totIdx] - (double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            EqualPeriodTotal[totIdx] = EqualPeriodTotal[totIdx] + (((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs((double)inClose[EqualTrailingIdx - totIdx] - (double)inOpen[EqualTrailingIdx - totIdx])) : ((Equal_rangeType == 1) ? ((double)inHigh[EqualTrailingIdx - totIdx] - (double)inLow[EqualTrailingIdx - totIdx]) : ((Equal_rangeType == 2) ? (((double)inHigh[EqualTrailingIdx - totIdx] - (double)inLow[EqualTrailingIdx - totIdx]) - Math.abs((double)inClose[EqualTrailingIdx - totIdx] - (double)inOpen[EqualTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CDLIDENTICAL3CROWS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlIdentical3Crows} over the same series.
    * Open with {@link Core#cdlIdentical3CrowsOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class CdlIdentical3CrowsStream {
      final Core core;
      double[] ShadowVeryShortPeriodTotal;
      double[] EqualPeriodTotal;
      int totIdx;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag1_inClose;
      double lag2_inClose;
      int ringPos_EqualTrailingIdx;
      int ringCap_EqualTrailingIdx;
      int ringLag_EqualTrailingIdx;
      double[] ring_EqualTrailingIdx_inOpen;
      double[] ring_EqualTrailingIdx_inHigh;
      double[] ring_EqualTrailingIdx_inLow;
      double[] ring_EqualTrailingIdx_inClose;
      int ringPos_ShadowVeryShortTrailingIdx;
      int ringCap_ShadowVeryShortTrailingIdx;
      int ringLag_ShadowVeryShortTrailingIdx;
      double[] ring_ShadowVeryShortTrailingIdx_inOpen;
      double[] ring_ShadowVeryShortTrailingIdx_inHigh;
      double[] ring_ShadowVeryShortTrailingIdx_inLow;
      double[] ring_ShadowVeryShortTrailingIdx_inClose;
      int winPos_totIdx;
      int winCap_totIdx;
      double[] win_totIdx_inOpen;
      double[] win_totIdx_inHigh;
      double[] win_totIdx_inLow;
      double[] win_totIdx_inClose;
      int cs_Equal_rangeType;
      int cs_Equal_avgPeriod;
      double cs_Equal_factor;
      int cs_ShadowVeryShort_rangeType;
      int cs_ShadowVeryShort_avgPeriod;
      double cs_ShadowVeryShort_factor;
      int cur_outInteger;

      CdlIdentical3CrowsStream( Core core ) { this.core = core; }

      CdlIdentical3CrowsStream( CdlIdentical3CrowsStream other ) {
         this.core = other.core;
         this.ShadowVeryShortPeriodTotal = other.ShadowVeryShortPeriodTotal.clone();
         this.EqualPeriodTotal = other.EqualPeriodTotal.clone();
         this.totIdx = other.totIdx;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.ringPos_EqualTrailingIdx = other.ringPos_EqualTrailingIdx;
         this.ringCap_EqualTrailingIdx = other.ringCap_EqualTrailingIdx;
         this.ringLag_EqualTrailingIdx = other.ringLag_EqualTrailingIdx;
         this.ring_EqualTrailingIdx_inOpen = other.ring_EqualTrailingIdx_inOpen.clone();
         this.ring_EqualTrailingIdx_inHigh = other.ring_EqualTrailingIdx_inHigh.clone();
         this.ring_EqualTrailingIdx_inLow = other.ring_EqualTrailingIdx_inLow.clone();
         this.ring_EqualTrailingIdx_inClose = other.ring_EqualTrailingIdx_inClose.clone();
         this.ringPos_ShadowVeryShortTrailingIdx = other.ringPos_ShadowVeryShortTrailingIdx;
         this.ringCap_ShadowVeryShortTrailingIdx = other.ringCap_ShadowVeryShortTrailingIdx;
         this.ringLag_ShadowVeryShortTrailingIdx = other.ringLag_ShadowVeryShortTrailingIdx;
         this.ring_ShadowVeryShortTrailingIdx_inOpen = other.ring_ShadowVeryShortTrailingIdx_inOpen.clone();
         this.ring_ShadowVeryShortTrailingIdx_inHigh = other.ring_ShadowVeryShortTrailingIdx_inHigh.clone();
         this.ring_ShadowVeryShortTrailingIdx_inLow = other.ring_ShadowVeryShortTrailingIdx_inLow.clone();
         this.ring_ShadowVeryShortTrailingIdx_inClose = other.ring_ShadowVeryShortTrailingIdx_inClose.clone();
         this.winPos_totIdx = other.winPos_totIdx;
         this.winCap_totIdx = other.winCap_totIdx;
         this.win_totIdx_inOpen = other.win_totIdx_inOpen.clone();
         this.win_totIdx_inHigh = other.win_totIdx_inHigh.clone();
         this.win_totIdx_inLow = other.win_totIdx_inLow.clone();
         this.win_totIdx_inClose = other.win_totIdx_inClose.clone();
         this.cs_Equal_rangeType = other.cs_Equal_rangeType;
         this.cs_Equal_avgPeriod = other.cs_Equal_avgPeriod;
         this.cs_Equal_factor = other.cs_Equal_factor;
         this.cs_ShadowVeryShort_rangeType = other.cs_ShadowVeryShort_rangeType;
         this.cs_ShadowVeryShort_avgPeriod = other.cs_ShadowVeryShort_avgPeriod;
         this.cs_ShadowVeryShort_factor = other.cs_ShadowVeryShort_factor;
         this.cur_outInteger = other.cur_outInteger;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.cdlIdentical3CrowsStreamStep(this, inOpen, inHigh, inLow, inClose);
         return this.cur_outInteger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         CdlIdentical3CrowsStream scratch = new CdlIdentical3CrowsStream(this);
         core.cdlIdentical3CrowsStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CdlIdentical3CrowsStream copy() {
         return new CdlIdentical3CrowsStream(this);
      }
   }
   void cdlIdentical3CrowsStreamStep( CdlIdentical3CrowsStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int Equal_rangeType = sp.cs_Equal_rangeType;
      int Equal_avgPeriod = sp.cs_Equal_avgPeriod;
      double Equal_factor = sp.cs_Equal_factor;
      int ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
      int ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
      double ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
      sp.ring_EqualTrailingIdx_inOpen[sp.ringPos_EqualTrailingIdx] = inOpen;
      sp.ring_EqualTrailingIdx_inHigh[sp.ringPos_EqualTrailingIdx] = inHigh;
      sp.ring_EqualTrailingIdx_inLow[sp.ringPos_EqualTrailingIdx] = inLow;
      sp.ring_EqualTrailingIdx_inClose[sp.ringPos_EqualTrailingIdx] = inClose;
      sp.ring_ShadowVeryShortTrailingIdx_inOpen[sp.ringPos_ShadowVeryShortTrailingIdx] = inOpen;
      sp.ring_ShadowVeryShortTrailingIdx_inHigh[sp.ringPos_ShadowVeryShortTrailingIdx] = inHigh;
      sp.ring_ShadowVeryShortTrailingIdx_inLow[sp.ringPos_ShadowVeryShortTrailingIdx] = inLow;
      sp.ring_ShadowVeryShortTrailingIdx_inClose[sp.ringPos_ShadowVeryShortTrailingIdx] = inClose;
      sp.win_totIdx_inOpen[sp.winPos_totIdx] = inOpen;
      sp.win_totIdx_inHigh[sp.winPos_totIdx] = inHigh;
      sp.win_totIdx_inLow[sp.winPos_totIdx] = inLow;
      sp.win_totIdx_inClose[sp.winPos_totIdx] = inClose;
      if( ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
          (((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inOpen : sp.lag2_inClose) - sp.lag2_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowVeryShort_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
          (((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inOpen : sp.lag1_inClose) - sp.lag1_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 &&                 /* 3rd black */
          (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
          sp.lag2_inClose > sp.lag1_inClose &&                          /* three declining */
          sp.lag1_inClose > inClose &&
          sp.lag1_inOpen <= sp.lag2_inClose + ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd black opens very close to 1st close */
          sp.lag1_inOpen >= sp.lag2_inClose - ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) &&
          inOpen <= sp.lag1_inClose + ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd black opens very close to 2nd close */
          inOpen >= sp.lag1_inClose - ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
      {
         sp.cur_outInteger = 0 - 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for( sp.totIdx = 2; sp.totIdx >= 0; sp.totIdx -= 1 ) {
         sp.ShadowVeryShortPeriodTotal[sp.totIdx] = sp.ShadowVeryShortPeriodTotal[sp.totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) - Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.ring_ShadowVeryShortTrailingIdx_inClose[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inOpen[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (sp.ring_ShadowVeryShortTrailingIdx_inHigh[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inLow[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((sp.ring_ShadowVeryShortTrailingIdx_inHigh[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inLow[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx]) - Math.abs(sp.ring_ShadowVeryShortTrailingIdx_inClose[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inOpen[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx])) : 0.0))));
      }
      for( sp.totIdx = 2; sp.totIdx >= 1; sp.totIdx -= 1 ) {
         sp.EqualPeriodTotal[sp.totIdx] = sp.EqualPeriodTotal[sp.totIdx] + (((Equal_rangeType == 0) ? (Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : ((Equal_rangeType == 1) ? (sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) : ((Equal_rangeType == 2) ? ((sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) - Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(sp.ring_EqualTrailingIdx_inClose[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - sp.totIdx) % sp.ringCap_EqualTrailingIdx] - sp.ring_EqualTrailingIdx_inOpen[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - sp.totIdx) % sp.ringCap_EqualTrailingIdx])) : ((Equal_rangeType == 1) ? (sp.ring_EqualTrailingIdx_inHigh[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - sp.totIdx) % sp.ringCap_EqualTrailingIdx] - sp.ring_EqualTrailingIdx_inLow[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - sp.totIdx) % sp.ringCap_EqualTrailingIdx]) : ((Equal_rangeType == 2) ? ((sp.ring_EqualTrailingIdx_inHigh[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - sp.totIdx) % sp.ringCap_EqualTrailingIdx] - sp.ring_EqualTrailingIdx_inLow[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - sp.totIdx) % sp.ringCap_EqualTrailingIdx]) - Math.abs(sp.ring_EqualTrailingIdx_inClose[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - sp.totIdx) % sp.ringCap_EqualTrailingIdx] - sp.ring_EqualTrailingIdx_inOpen[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - sp.totIdx) % sp.ringCap_EqualTrailingIdx])) : 0.0))));
      }
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
      sp.ring_EqualTrailingIdx_inOpen[sp.ringPos_EqualTrailingIdx] = inOpen;
      sp.ring_EqualTrailingIdx_inHigh[sp.ringPos_EqualTrailingIdx] = inHigh;
      sp.ring_EqualTrailingIdx_inLow[sp.ringPos_EqualTrailingIdx] = inLow;
      sp.ring_EqualTrailingIdx_inClose[sp.ringPos_EqualTrailingIdx] = inClose;
      sp.ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx + 1;
      if( sp.ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx ) {
         sp.ringPos_EqualTrailingIdx = 0;
      }
      sp.ring_ShadowVeryShortTrailingIdx_inOpen[sp.ringPos_ShadowVeryShortTrailingIdx] = inOpen;
      sp.ring_ShadowVeryShortTrailingIdx_inHigh[sp.ringPos_ShadowVeryShortTrailingIdx] = inHigh;
      sp.ring_ShadowVeryShortTrailingIdx_inLow[sp.ringPos_ShadowVeryShortTrailingIdx] = inLow;
      sp.ring_ShadowVeryShortTrailingIdx_inClose[sp.ringPos_ShadowVeryShortTrailingIdx] = inClose;
      sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
      if( sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx ) {
         sp.ringPos_ShadowVeryShortTrailingIdx = 0;
      }
      sp.winPos_totIdx = sp.winPos_totIdx + 1;
      if( sp.winPos_totIdx >= sp.winCap_totIdx ) {
         sp.winPos_totIdx = 0;
      }
   }
   private RetCode cdlIdentical3CrowsOpenBody( CdlIdentical3CrowsStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      double[] ShadowVeryShortPeriodTotal = new double[3];
      double[] EqualPeriodTotal = new double[3];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int lastValue_outInteger = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlIdentical3CrowsLookback();
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
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortPeriodTotal[0] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      EqualPeriodTotal[2] = 0;
      EqualPeriodTotal[1] = 0;
      EqualPeriodTotal[0] = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal[2] = EqualPeriodTotal[2] + ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         EqualPeriodTotal[1] = EqualPeriodTotal[1] + ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - three consecutive and declining black candlesticks
       * - each candle must have no or very short lower shadow
       * - each candle after the first must open at or very close to the prior candle's close
       * The meaning of "very short" is specified with TA_SetCandleSettings;
       * the meaning of "very close" is specified with TA_SetCandleSettings (Equal);
       * outInteger is negative (-1 to -100): identical three crows is always bearish;
       * the user should consider that identical 3 crows is significant when it appears after a mature advance or at high levels,
       * while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
             (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
             (((inClose[i - 1] >= inOpen[i - 1]) ? inOpen[i - 1] : inClose[i - 1]) - inLow[i - 1]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* 3rd black */
             (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
             inClose[i - 2] > inClose[i - 1] &&                          /* three declining */
             inClose[i - 1] > inClose[i] &&
             inOpen[i - 1] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd black opens very close to 1st close */
             inOpen[i - 1] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd black opens very close to 2nd close */
             inOpen[i] >= inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            lastValue_outInteger = 0 - 100;
         } else {
            lastValue_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            EqualPeriodTotal[totIdx] = EqualPeriodTotal[totIdx] + (((Equal_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Equal_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Equal_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - totIdx] - inOpen[EqualTrailingIdx - totIdx])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - totIdx] - inLow[EqualTrailingIdx - totIdx]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - totIdx] - inLow[EqualTrailingIdx - totIdx]) - Math.abs(inClose[EqualTrailingIdx - totIdx] - inOpen[EqualTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_EqualTrailingIdx = i - EqualTrailingIdx;
      int cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 3;
      if( capLag_EqualTrailingIdx < 0 || cap_EqualTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_EqualTrailingIdx = (cap_EqualTrailingIdx > 0)? cap_EqualTrailingIdx : 1;
      double[] capRing_EqualTrailingIdx_inOpen = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_inOpen[fillJ % cap_EqualTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_EqualTrailingIdx_inHigh = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_inHigh[fillJ % cap_EqualTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_EqualTrailingIdx_inLow = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_inLow[fillJ % cap_EqualTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_EqualTrailingIdx_inClose = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_inClose[fillJ % cap_EqualTrailingIdx] = inClose[fillJ];
      }
      int capLag_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
      int cap_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx + 3;
      if( capLag_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowVeryShortTrailingIdx = (cap_ShadowVeryShortTrailingIdx > 0)? cap_ShadowVeryShortTrailingIdx : 1;
      double[] capRing_ShadowVeryShortTrailingIdx_inOpen = new double[allocN_ShadowVeryShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowVeryShortTrailingIdx_inOpen[fillJ % cap_ShadowVeryShortTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_ShadowVeryShortTrailingIdx_inHigh = new double[allocN_ShadowVeryShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowVeryShortTrailingIdx_inHigh[fillJ % cap_ShadowVeryShortTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_ShadowVeryShortTrailingIdx_inLow = new double[allocN_ShadowVeryShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowVeryShortTrailingIdx_inLow[fillJ % cap_ShadowVeryShortTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_ShadowVeryShortTrailingIdx_inClose = new double[allocN_ShadowVeryShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowVeryShortTrailingIdx_inClose[fillJ % cap_ShadowVeryShortTrailingIdx] = inClose[fillJ];
      }
      int cap_totIdx = (int)(3);
      if( cap_totIdx < 1 || cap_totIdx > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_totIdx_inOpen = new double[cap_totIdx];
      System.arraycopy(inOpen, historyLen - cap_totIdx, capWin_totIdx_inOpen, 0, cap_totIdx);
      double[] capWin_totIdx_inHigh = new double[cap_totIdx];
      System.arraycopy(inHigh, historyLen - cap_totIdx, capWin_totIdx_inHigh, 0, cap_totIdx);
      double[] capWin_totIdx_inLow = new double[cap_totIdx];
      System.arraycopy(inLow, historyLen - cap_totIdx, capWin_totIdx_inLow, 0, cap_totIdx);
      double[] capWin_totIdx_inClose = new double[cap_totIdx];
      System.arraycopy(inClose, historyLen - cap_totIdx, capWin_totIdx_inClose, 0, cap_totIdx);
      sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
      sp.EqualPeriodTotal = EqualPeriodTotal;
      sp.totIdx = totIdx;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
      sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
      sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
      sp.ring_EqualTrailingIdx_inOpen = capRing_EqualTrailingIdx_inOpen;
      sp.ring_EqualTrailingIdx_inHigh = capRing_EqualTrailingIdx_inHigh;
      sp.ring_EqualTrailingIdx_inLow = capRing_EqualTrailingIdx_inLow;
      sp.ring_EqualTrailingIdx_inClose = capRing_EqualTrailingIdx_inClose;
      sp.ringPos_ShadowVeryShortTrailingIdx = historyLen % cap_ShadowVeryShortTrailingIdx;
      sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
      sp.ringLag_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx;
      sp.ring_ShadowVeryShortTrailingIdx_inOpen = capRing_ShadowVeryShortTrailingIdx_inOpen;
      sp.ring_ShadowVeryShortTrailingIdx_inHigh = capRing_ShadowVeryShortTrailingIdx_inHigh;
      sp.ring_ShadowVeryShortTrailingIdx_inLow = capRing_ShadowVeryShortTrailingIdx_inLow;
      sp.ring_ShadowVeryShortTrailingIdx_inClose = capRing_ShadowVeryShortTrailingIdx_inClose;
      sp.winPos_totIdx = 0;
      sp.winCap_totIdx = cap_totIdx;
      sp.win_totIdx_inOpen = capWin_totIdx_inOpen;
      sp.win_totIdx_inHigh = capWin_totIdx_inHigh;
      sp.win_totIdx_inLow = capWin_totIdx_inLow;
      sp.win_totIdx_inClose = capWin_totIdx_inClose;
      sp.cs_Equal_rangeType = Equal_rangeType;
      sp.cs_Equal_avgPeriod = Equal_avgPeriod;
      sp.cs_Equal_factor = Equal_factor;
      sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
      sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
      sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlIdentical3CrowsOpenAndFillBody( CdlIdentical3CrowsStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      double[] ShadowVeryShortPeriodTotal = new double[3];
      double[] EqualPeriodTotal = new double[3];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlIdentical3CrowsLookback();
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
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortPeriodTotal[0] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      EqualPeriodTotal[2] = 0;
      EqualPeriodTotal[1] = 0;
      EqualPeriodTotal[0] = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         ShadowVeryShortPeriodTotal[0] = ShadowVeryShortPeriodTotal[0] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal[2] = EqualPeriodTotal[2] + ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         EqualPeriodTotal[1] = EqualPeriodTotal[1] + ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - three consecutive and declining black candlesticks
       * - each candle must have no or very short lower shadow
       * - each candle after the first must open at or very close to the prior candle's close
       * The meaning of "very short" is specified with TA_SetCandleSettings;
       * the meaning of "very close" is specified with TA_SetCandleSettings (Equal);
       * outInteger is negative (-1 to -100): identical three crows is always bearish;
       * the user should consider that identical 3 crows is significant when it appears after a mature advance or at high levels,
       * while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
             (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
             (((inClose[i - 1] >= inOpen[i - 1]) ? inOpen[i - 1] : inClose[i - 1]) - inLow[i - 1]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* 3rd black */
             (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[0] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short lower shadow */
             inClose[i - 2] > inClose[i - 1] &&                          /* three declining */
             inClose[i - 1] > inClose[i] &&
             inOpen[i - 1] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd black opens very close to 1st close */
             inOpen[i - 1] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[2] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd black opens very close to 2nd close */
             inOpen[i] >= inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal[1] / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            EqualPeriodTotal[totIdx] = EqualPeriodTotal[totIdx] + (((Equal_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Equal_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Equal_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - totIdx] - inOpen[EqualTrailingIdx - totIdx])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - totIdx] - inLow[EqualTrailingIdx - totIdx]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - totIdx] - inLow[EqualTrailingIdx - totIdx]) - Math.abs(inClose[EqualTrailingIdx - totIdx] - inOpen[EqualTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_EqualTrailingIdx = i - EqualTrailingIdx;
      int cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 3;
      if( capLag_EqualTrailingIdx < 0 || cap_EqualTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_EqualTrailingIdx = (cap_EqualTrailingIdx > 0)? cap_EqualTrailingIdx : 1;
      double[] capRing_EqualTrailingIdx_inOpen = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_inOpen[fillJ % cap_EqualTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_EqualTrailingIdx_inHigh = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_inHigh[fillJ % cap_EqualTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_EqualTrailingIdx_inLow = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_inLow[fillJ % cap_EqualTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_EqualTrailingIdx_inClose = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_inClose[fillJ % cap_EqualTrailingIdx] = inClose[fillJ];
      }
      int capLag_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
      int cap_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx + 3;
      if( capLag_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowVeryShortTrailingIdx = (cap_ShadowVeryShortTrailingIdx > 0)? cap_ShadowVeryShortTrailingIdx : 1;
      double[] capRing_ShadowVeryShortTrailingIdx_inOpen = new double[allocN_ShadowVeryShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowVeryShortTrailingIdx_inOpen[fillJ % cap_ShadowVeryShortTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_ShadowVeryShortTrailingIdx_inHigh = new double[allocN_ShadowVeryShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowVeryShortTrailingIdx_inHigh[fillJ % cap_ShadowVeryShortTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_ShadowVeryShortTrailingIdx_inLow = new double[allocN_ShadowVeryShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowVeryShortTrailingIdx_inLow[fillJ % cap_ShadowVeryShortTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_ShadowVeryShortTrailingIdx_inClose = new double[allocN_ShadowVeryShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowVeryShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowVeryShortTrailingIdx_inClose[fillJ % cap_ShadowVeryShortTrailingIdx] = inClose[fillJ];
      }
      int cap_totIdx = (int)(3);
      if( cap_totIdx < 1 || cap_totIdx > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_totIdx_inOpen = new double[cap_totIdx];
      System.arraycopy(inOpen, historyLen - cap_totIdx, capWin_totIdx_inOpen, 0, cap_totIdx);
      double[] capWin_totIdx_inHigh = new double[cap_totIdx];
      System.arraycopy(inHigh, historyLen - cap_totIdx, capWin_totIdx_inHigh, 0, cap_totIdx);
      double[] capWin_totIdx_inLow = new double[cap_totIdx];
      System.arraycopy(inLow, historyLen - cap_totIdx, capWin_totIdx_inLow, 0, cap_totIdx);
      double[] capWin_totIdx_inClose = new double[cap_totIdx];
      System.arraycopy(inClose, historyLen - cap_totIdx, capWin_totIdx_inClose, 0, cap_totIdx);
      sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
      sp.EqualPeriodTotal = EqualPeriodTotal;
      sp.totIdx = totIdx;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
      sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
      sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
      sp.ring_EqualTrailingIdx_inOpen = capRing_EqualTrailingIdx_inOpen;
      sp.ring_EqualTrailingIdx_inHigh = capRing_EqualTrailingIdx_inHigh;
      sp.ring_EqualTrailingIdx_inLow = capRing_EqualTrailingIdx_inLow;
      sp.ring_EqualTrailingIdx_inClose = capRing_EqualTrailingIdx_inClose;
      sp.ringPos_ShadowVeryShortTrailingIdx = historyLen % cap_ShadowVeryShortTrailingIdx;
      sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
      sp.ringLag_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx;
      sp.ring_ShadowVeryShortTrailingIdx_inOpen = capRing_ShadowVeryShortTrailingIdx_inOpen;
      sp.ring_ShadowVeryShortTrailingIdx_inHigh = capRing_ShadowVeryShortTrailingIdx_inHigh;
      sp.ring_ShadowVeryShortTrailingIdx_inLow = capRing_ShadowVeryShortTrailingIdx_inLow;
      sp.ring_ShadowVeryShortTrailingIdx_inClose = capRing_ShadowVeryShortTrailingIdx_inClose;
      sp.winPos_totIdx = 0;
      sp.winCap_totIdx = cap_totIdx;
      sp.win_totIdx_inOpen = capWin_totIdx_inOpen;
      sp.win_totIdx_inHigh = capWin_totIdx_inHigh;
      sp.win_totIdx_inLow = capWin_totIdx_inLow;
      sp.win_totIdx_inClose = capWin_totIdx_inClose;
      sp.cs_Equal_rangeType = Equal_rangeType;
      sp.cs_Equal_avgPeriod = Equal_avgPeriod;
      sp.cs_Equal_factor = Equal_factor;
      sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
      sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
      sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlIdentical3CrowsOpen (composition seam). */
   CdlIdentical3CrowsStream cdlIdentical3CrowsOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlIdentical3CrowsStream sp = new CdlIdentical3CrowsStream(this);
      RetCode retCode = cdlIdentical3CrowsOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLIDENTICAL3CROWS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLIDENTICAL3CROWS open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLIDENTICAL3CROWS open: " + retCode);
   }
   /**
    * Open a live CDLIDENTICAL3CROWS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlIdentical3Crows} at that bar.
    * <p>The history must hold at least {@code cdlIdentical3CrowsLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlIdentical3CrowsStream cdlIdentical3CrowsOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return cdlIdentical3CrowsOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlIdentical3CrowsOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlIdentical3Crows} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CdlIdentical3CrowsStream cdlIdentical3CrowsOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlIdentical3CrowsStream sp = new CdlIdentical3CrowsStream(this);
      RetCode retCode = cdlIdentical3CrowsOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLIDENTICAL3CROWS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLIDENTICAL3CROWS openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLIDENTICAL3CROWS openAndFill: " + retCode);
   }
