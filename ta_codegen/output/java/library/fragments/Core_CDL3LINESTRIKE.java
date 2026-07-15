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
 *  121104 AC   Creation
 */

   public int cdl3LineStrikeLookback( )
   {
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      return Near_avgPeriod + 3 ;

   }
   public RetCode cdl3LineStrike( int startIdx,
                                  int endIdx,
                                  double inOpen[],
                                  double inHigh[],
                                  double inLow[],
                                  double inClose[],
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  int outInteger[] )
   {
      double[] NearPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdl3LineStrikeLookback();
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
      NearPeriodTotal[3] = 0;
      NearPeriodTotal[2] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - three white soldiers (three black crows): three white (black) candlesticks with consecutively higher (lower) closes,
       * each opening within or near the previous real body
       * - fourth candle: black (white) candle that opens above (below) prior candle's close and closes below (above)
       * the first candle's open
       * The meaning of "near" is specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that 3-line strike is significant when it appears in a trend in the same direction of
       * the first three candles, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) && /* three with same color */
             ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && /* 4th opposite color */
             inOpen[i - 2] >= Math.min(inOpen[i - 3], inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
             inOpen[i - 2] <= Math.max(inOpen[i - 3], inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i - 1] >= Math.min(inOpen[i - 2], inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
             inOpen[i - 1] <= Math.max(inOpen[i - 2], inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && inClose[i - 1] > inClose[i - 2] && inClose[i - 2] > inClose[i - 3] && inOpen[i] > inClose[i - 1] && inClose[i] < inOpen[i - 3] || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && inClose[i - 1] < inClose[i - 2] && inClose[i - 2] < inClose[i - 3] && inOpen[i] < inClose[i - 1] && inClose[i] > inOpen[i - 3]) ) /* if three white consecutive higher closes 4th opens above prior close 4th closes below 1st open if three black consecutive lower closes 4th opens below prior close 4th closes above 1st open */
         {
            outInteger[outIdx++] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 3; totIdx >= 2; totIdx -= 1 ) {
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) - Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdl3LineStrikeUnguarded( int startIdx,
                                           int endIdx,
                                           double inOpen[],
                                           double inHigh[],
                                           double inLow[],
                                           double inClose[],
                                           MInteger outBegIdx,
                                           MInteger outNBElement,
                                           int outInteger[] )
   {
      double[] NearPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      lookbackTotal = cdl3LineStrikeLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal[3] = 0;
      NearPeriodTotal[2] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && inOpen[i - 2] >= Math.min(inOpen[i - 3], inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && inOpen[i - 2] <= Math.max(inOpen[i - 3], inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && inOpen[i - 1] >= Math.min(inOpen[i - 2], inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && inOpen[i - 1] <= Math.max(inOpen[i - 2], inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && inClose[i - 1] > inClose[i - 2] && inClose[i - 2] > inClose[i - 3] && inOpen[i] > inClose[i - 1] && inClose[i] < inOpen[i - 3] || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && inClose[i - 1] < inClose[i - 2] && inClose[i - 2] < inClose[i - 3] && inOpen[i] < inClose[i - 1] && inClose[i] > inOpen[i - 3]) ) {
            outInteger[outIdx++] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 3; totIdx >= 2; totIdx -= 1 ) {
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) - Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdl3LineStrike( int startIdx,
                                  int endIdx,
                                  float inOpen[],
                                  float inHigh[],
                                  float inLow[],
                                  float inClose[],
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  int outInteger[] )
   {
      double[] NearPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdl3LineStrikeLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal[3] = 0;
      NearPeriodTotal[2] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (double)inOpen[i - 2] >= Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 2] <= Math.max((double)inOpen[i - 3], (double)inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] >= Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] <= Math.max((double)inOpen[i - 2], (double)inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (double)inClose[i - 1] > (double)inClose[i - 2] && (double)inClose[i - 2] > (double)inClose[i - 3] && (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] < (double)inOpen[i - 3] || (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (double)inClose[i - 1] < (double)inClose[i - 2] && (double)inClose[i - 2] < (double)inClose[i - 3] && (double)inOpen[i] < (double)inClose[i - 1] && (double)inClose[i] > (double)inOpen[i - 3]) ) {
            outInteger[outIdx++] = (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 3; totIdx >= 2; totIdx -= 1 ) {
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) - Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdl3LineStrikeUnguarded( int startIdx,
                                           int endIdx,
                                           float inOpen[],
                                           float inHigh[],
                                           float inLow[],
                                           float inClose[],
                                           MInteger outBegIdx,
                                           MInteger outNBElement,
                                           int outInteger[] )
   {
      double[] NearPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      lookbackTotal = cdl3LineStrikeLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal[3] = 0;
      NearPeriodTotal[2] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (double)inOpen[i - 2] >= Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 2] <= Math.max((double)inOpen[i - 3], (double)inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] >= Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] <= Math.max((double)inOpen[i - 2], (double)inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (double)inClose[i - 1] > (double)inClose[i - 2] && (double)inClose[i - 2] > (double)inClose[i - 3] && (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] < (double)inOpen[i - 3] || (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (double)inClose[i - 1] < (double)inClose[i - 2] && (double)inClose[i - 2] < (double)inClose[i - 3] && (double)inOpen[i] < (double)inClose[i - 1] && (double)inClose[i] > (double)inOpen[i - 3]) ) {
            outInteger[outIdx++] = (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 3; totIdx >= 2; totIdx -= 1 ) {
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) - Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
