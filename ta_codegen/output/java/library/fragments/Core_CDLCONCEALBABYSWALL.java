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

   public int cdlConcealBabysWallLookback( )
   {
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      return ShadowVeryShort_avgPeriod + 3 ;

   }
   public RetCode cdlConcealBabysWall( int startIdx,
                                       int endIdx,
                                       double inOpen[],
                                       double inHigh[],
                                       double inLow[],
                                       double inClose[],
                                       MInteger outBegIdx,
                                       MInteger outNBElement,
                                       int outInteger[] )
   {
      double[] ShadowVeryShortPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
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
      lookbackTotal = cdlConcealBabysWallLookback();
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
      ShadowVeryShortPeriodTotal[3] = 0;
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[3] = ShadowVeryShortPeriodTotal[3] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)));
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: black marubozu (very short shadows)
       * - second candle: black marubozu (very short shadows)
       * - third candle: black candle that opens gapping down but has an upper shadow that extends into the prior body
       * - fourth candle: black candle that completely engulfs the third candle, including the shadows
       * The meanings of "very short shadow" are specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100): concealing baby swallow is always bullish;
       * the user should consider that concealing baby swallow is significant when it appears in downtrend, while
       * this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
             ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 3rd black */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* 4th black */
             (((inClose[i - 3] >= inOpen[i - 3]) ? inOpen[i - 3] : inClose[i - 3]) - inLow[i - 3]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: marubozu */
             (inHigh[i - 3] - ((inClose[i - 3] >= inOpen[i - 3]) ? inClose[i - 3] : inOpen[i - 3])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: marubozu */
             (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && /* 3rd: opens gapping down */
             (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* and HAS an upper shadow */
             inHigh[i - 1] > inClose[i - 2] &&                           /* that extends into the prior body */
             inHigh[i] > inHigh[i - 1] &&
             inLow[i] < inLow[i - 1] )                                   /* 4th: engulfs the 3rd including the shadows */
         {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlConcealBabysWallUnguarded( int startIdx,
                                                int endIdx,
                                                double inOpen[],
                                                double inHigh[],
                                                double inLow[],
                                                double inClose[],
                                                MInteger outBegIdx,
                                                MInteger outNBElement,
                                                int outInteger[] )
   {
      double[] ShadowVeryShortPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      lookbackTotal = cdlConcealBabysWallLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      ShadowVeryShortPeriodTotal[3] = 0;
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[3] = ShadowVeryShortPeriodTotal[3] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)));
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (((inClose[i - 3] >= inOpen[i - 3]) ? inOpen[i - 3] : inClose[i - 3]) - inLow[i - 3]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i - 3] - ((inClose[i - 3] >= inOpen[i - 3]) ? inClose[i - 3] : inOpen[i - 3])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && inHigh[i - 1] > inClose[i - 2] && inHigh[i] > inHigh[i - 1] && inLow[i] < inLow[i - 1] ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlConcealBabysWall( int startIdx,
                                       int endIdx,
                                       float inOpen[],
                                       float inHigh[],
                                       float inLow[],
                                       float inClose[],
                                       MInteger outBegIdx,
                                       MInteger outNBElement,
                                       int outInteger[] )
   {
      double[] ShadowVeryShortPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlConcealBabysWallLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      ShadowVeryShortPeriodTotal[3] = 0;
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[3] = ShadowVeryShortPeriodTotal[3] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)));
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && ((((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? (double)inOpen[i - 3] : (double)inClose[i - 3]) - (double)inLow[i - 3]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i - 3] - (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? (double)inClose[i - 3] : (double)inOpen[i - 3])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inOpen[i - 2] : (double)inClose[i - 2]) - (double)inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i - 2] - (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inClose[i - 2] : (double)inOpen[i - 2])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2])) && ((double)inHigh[i - 1] - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inClose[i - 1] : (double)inOpen[i - 1])) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (double)inHigh[i - 1] > (double)inClose[i - 2] && (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx - totIdx] - (double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx - totIdx] - (double)inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx - totIdx] - (double)inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs((double)inClose[ShadowVeryShortTrailingIdx - totIdx] - (double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlConcealBabysWallUnguarded( int startIdx,
                                                int endIdx,
                                                float inOpen[],
                                                float inHigh[],
                                                float inLow[],
                                                float inClose[],
                                                MInteger outBegIdx,
                                                MInteger outNBElement,
                                                int outInteger[] )
   {
      double[] ShadowVeryShortPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      lookbackTotal = cdlConcealBabysWallLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      ShadowVeryShortPeriodTotal[3] = 0;
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[3] = ShadowVeryShortPeriodTotal[3] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)));
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && ((((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? (double)inOpen[i - 3] : (double)inClose[i - 3]) - (double)inLow[i - 3]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i - 3] - (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? (double)inClose[i - 3] : (double)inOpen[i - 3])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inOpen[i - 2] : (double)inClose[i - 2]) - (double)inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i - 2] - (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inClose[i - 2] : (double)inOpen[i - 2])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2])) && ((double)inHigh[i - 1] - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inClose[i - 1] : (double)inOpen[i - 1])) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (double)inHigh[i - 1] > (double)inClose[i - 2] && (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx - totIdx] - (double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx - totIdx] - (double)inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx - totIdx] - (double)inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs((double)inClose[ShadowVeryShortTrailingIdx - totIdx] - (double)inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CDLCONCEALBABYSWALL stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlConcealBabysWall} over the same series.
    * Open with {@link Core#cdlConcealBabysWallOpen}; there is no close — the handle is
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
   public static final class CdlConcealBabysWallStream {
      final Core core;
      double[] ShadowVeryShortPeriodTotal;
      int totIdx;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag3_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag3_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag3_inLow;
      double lag1_inClose;
      double lag2_inClose;
      double lag3_inClose;
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
      int cs_ShadowVeryShort_rangeType;
      int cs_ShadowVeryShort_avgPeriod;
      double cs_ShadowVeryShort_factor;
      int cur_outInteger;

      CdlConcealBabysWallStream( Core core ) { this.core = core; }

      CdlConcealBabysWallStream( CdlConcealBabysWallStream other ) {
         this.core = other.core;
         this.ShadowVeryShortPeriodTotal = other.ShadowVeryShortPeriodTotal.clone();
         this.totIdx = other.totIdx;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag3_inOpen = other.lag3_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag3_inHigh = other.lag3_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag3_inLow = other.lag3_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.lag3_inClose = other.lag3_inClose;
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
         core.cdlConcealBabysWallStreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CdlConcealBabysWallStream scratch = new CdlConcealBabysWallStream(this);
         core.cdlConcealBabysWallStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CdlConcealBabysWallStream copy() {
         return new CdlConcealBabysWallStream(this);
      }
   }
   void cdlConcealBabysWallStreamStep( CdlConcealBabysWallStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
      int ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
      double ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
      sp.ring_ShadowVeryShortTrailingIdx_inOpen[sp.ringPos_ShadowVeryShortTrailingIdx] = inOpen;
      sp.ring_ShadowVeryShortTrailingIdx_inHigh[sp.ringPos_ShadowVeryShortTrailingIdx] = inHigh;
      sp.ring_ShadowVeryShortTrailingIdx_inLow[sp.ringPos_ShadowVeryShortTrailingIdx] = inLow;
      sp.ring_ShadowVeryShortTrailingIdx_inClose[sp.ringPos_ShadowVeryShortTrailingIdx] = inClose;
      sp.win_totIdx_inOpen[sp.winPos_totIdx] = inOpen;
      sp.win_totIdx_inHigh[sp.winPos_totIdx] = inHigh;
      sp.win_totIdx_inLow[sp.winPos_totIdx] = inLow;
      sp.win_totIdx_inClose[sp.winPos_totIdx] = inClose;
      if( ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
          ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* 3rd black */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 &&                 /* 4th black */
          (((sp.lag3_inClose >= sp.lag3_inOpen) ? sp.lag3_inOpen : sp.lag3_inClose) - sp.lag3_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((ShadowVeryShort_rangeType == 2) ? ((sp.lag3_inHigh - sp.lag3_inLow) - Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: marubozu */
          (sp.lag3_inHigh - ((sp.lag3_inClose >= sp.lag3_inOpen) ? sp.lag3_inClose : sp.lag3_inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((ShadowVeryShort_rangeType == 2) ? ((sp.lag3_inHigh - sp.lag3_inLow) - Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          (((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inOpen : sp.lag2_inClose) - sp.lag2_inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowVeryShort_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: marubozu */
          (sp.lag2_inHigh - ((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inClose : sp.lag2_inOpen)) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowVeryShort_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          (Math.max(sp.lag1_inOpen, sp.lag1_inClose) < Math.min(sp.lag2_inOpen, sp.lag2_inClose)) && /* 3rd: opens gapping down */
          (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowVeryShort_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* and HAS an upper shadow */
          sp.lag1_inHigh > sp.lag2_inClose &&                           /* that extends into the prior body */
          inHigh > sp.lag1_inHigh &&
          inLow < sp.lag1_inLow )                                       /* 4th: engulfs the 3rd including the shadows */
      {
         sp.cur_outInteger = 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for( sp.totIdx = 3; sp.totIdx >= 1; sp.totIdx -= 1 ) {
         sp.ShadowVeryShortPeriodTotal[sp.totIdx] = sp.ShadowVeryShortPeriodTotal[sp.totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) - Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.ring_ShadowVeryShortTrailingIdx_inClose[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inOpen[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (sp.ring_ShadowVeryShortTrailingIdx_inHigh[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inLow[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((sp.ring_ShadowVeryShortTrailingIdx_inHigh[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inLow[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx]) - Math.abs(sp.ring_ShadowVeryShortTrailingIdx_inClose[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inOpen[(sp.ringPos_ShadowVeryShortTrailingIdx + sp.ringCap_ShadowVeryShortTrailingIdx - sp.ringLag_ShadowVeryShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowVeryShortTrailingIdx])) : 0.0))));
      }
      sp.lag3_inOpen = sp.lag2_inOpen;
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag3_inHigh = sp.lag2_inHigh;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag3_inLow = sp.lag2_inLow;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
      sp.lag3_inClose = sp.lag2_inClose;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
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
   private RetCode cdlConcealBabysWallOpenBody( CdlConcealBabysWallStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      double[] ShadowVeryShortPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int lastValue_outInteger = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlConcealBabysWallLookback();
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
      ShadowVeryShortPeriodTotal[3] = 0;
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[3] = ShadowVeryShortPeriodTotal[3] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)));
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: black marubozu (very short shadows)
       * - second candle: black marubozu (very short shadows)
       * - third candle: black candle that opens gapping down but has an upper shadow that extends into the prior body
       * - fourth candle: black candle that completely engulfs the third candle, including the shadows
       * The meanings of "very short shadow" are specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100): concealing baby swallow is always bullish;
       * the user should consider that concealing baby swallow is significant when it appears in downtrend, while
       * this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
             ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 3rd black */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* 4th black */
             (((inClose[i - 3] >= inOpen[i - 3]) ? inOpen[i - 3] : inClose[i - 3]) - inLow[i - 3]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: marubozu */
             (inHigh[i - 3] - ((inClose[i - 3] >= inOpen[i - 3]) ? inClose[i - 3] : inOpen[i - 3])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: marubozu */
             (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && /* 3rd: opens gapping down */
             (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* and HAS an upper shadow */
             inHigh[i - 1] > inClose[i - 2] &&                           /* that extends into the prior body */
             inHigh[i] > inHigh[i - 1] &&
             inLow[i] < inLow[i - 1] )                                   /* 4th: engulfs the 3rd including the shadows */
         {
            lastValue_outInteger = 100;
         } else {
            lastValue_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
      int cap_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx + 4;
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
      int cap_totIdx = (int)(4);
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
      sp.totIdx = totIdx;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag3_inOpen = inOpen[historyLen - 3];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag3_inHigh = inHigh[historyLen - 3];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag3_inLow = inLow[historyLen - 3];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.lag3_inClose = inClose[historyLen - 3];
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
      sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
      sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
      sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlConcealBabysWallOpenAndFillBody( CdlConcealBabysWallStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      double[] ShadowVeryShortPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
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
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlConcealBabysWallLookback();
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
      ShadowVeryShortPeriodTotal[3] = 0;
      ShadowVeryShortPeriodTotal[2] = 0;
      ShadowVeryShortPeriodTotal[1] = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal[3] = ShadowVeryShortPeriodTotal[3] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)));
         ShadowVeryShortPeriodTotal[2] = ShadowVeryShortPeriodTotal[2] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         ShadowVeryShortPeriodTotal[1] = ShadowVeryShortPeriodTotal[1] + ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: black marubozu (very short shadows)
       * - second candle: black marubozu (very short shadows)
       * - third candle: black candle that opens gapping down but has an upper shadow that extends into the prior body
       * - fourth candle: black candle that completely engulfs the third candle, including the shadows
       * The meanings of "very short shadow" are specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100): concealing baby swallow is always bullish;
       * the user should consider that concealing baby swallow is significant when it appears in downtrend, while
       * this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == 0 - 1 && /* 1st black */
             ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* 2nd black */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 3rd black */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* 4th black */
             (((inClose[i - 3] >= inOpen[i - 3]) ? inOpen[i - 3] : inClose[i - 3]) - inLow[i - 3]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: marubozu */
             (inHigh[i - 3] - ((inClose[i - 3] >= inOpen[i - 3]) ? inClose[i - 3] : inOpen[i - 3])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[3] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (((inClose[i - 2] >= inOpen[i - 2]) ? inOpen[i - 2] : inClose[i - 2]) - inLow[i - 2]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: marubozu */
             (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[2] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && /* 3rd: opens gapping down */
             (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal[1] / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* and HAS an upper shadow */
             inHigh[i - 1] > inClose[i - 2] &&                           /* that extends into the prior body */
             inHigh[i] > inHigh[i - 1] &&
             inLow[i] < inLow[i - 1] )                                   /* 4th: engulfs the 3rd including the shadows */
         {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            ShadowVeryShortPeriodTotal[totIdx] = ShadowVeryShortPeriodTotal[totIdx] + (((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - totIdx] - inLow[ShadowVeryShortTrailingIdx - totIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx - totIdx] - inOpen[ShadowVeryShortTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
      int cap_ShadowVeryShortTrailingIdx = capLag_ShadowVeryShortTrailingIdx + 4;
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
      int cap_totIdx = (int)(4);
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
      sp.totIdx = totIdx;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag3_inOpen = inOpen[historyLen - 3];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag3_inHigh = inHigh[historyLen - 3];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag3_inLow = inLow[historyLen - 3];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.lag3_inClose = inClose[historyLen - 3];
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
      sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
      sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
      sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlConcealBabysWallOpen (composition seam). */
   CdlConcealBabysWallStream cdlConcealBabysWallOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlConcealBabysWallStream sp = new CdlConcealBabysWallStream(this);
      RetCode retCode = cdlConcealBabysWallOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLCONCEALBABYSWALL open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLCONCEALBABYSWALL open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLCONCEALBABYSWALL open: " + retCode);
   }
   /**
    * Open a live CDLCONCEALBABYSWALL stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlConcealBabysWall} at that bar.
    * <p>The history must hold at least {@code cdlConcealBabysWallLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlConcealBabysWallStream cdlConcealBabysWallOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return cdlConcealBabysWallOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlConcealBabysWallOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlConcealBabysWall} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CdlConcealBabysWallStream cdlConcealBabysWallOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlConcealBabysWallStream sp = new CdlConcealBabysWallStream(this);
      RetCode retCode = cdlConcealBabysWallOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLCONCEALBABYSWALL openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLCONCEALBABYSWALL openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLCONCEALBABYSWALL openAndFill: " + retCode);
   }
