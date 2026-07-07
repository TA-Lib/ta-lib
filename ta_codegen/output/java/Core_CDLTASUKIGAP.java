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
 *  011605 AC   Creation
 */

   public int cdlTasukiGapLookback( )
   {
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      return Near_avgPeriod + 2 ;

   }
   public RetCode cdlTasukiGap( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                int outInteger[] )
   {
      double NearPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
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
      lookbackTotal = cdlTasukiGapLookback();
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
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - upside (downside) gap
       * - first candle after the window: white (black) candlestick
       * - second candle: black (white) candlestick that opens within the previous real body and closes under (above)
       *   the previous real body inside the gap
       * - the size of two real bodies should be near the same
       * The meaning of "near" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that tasuki gap is significant when it appears in a trend, while this function does
       * not consider it
       */
      outIdx = 0;
      do {
         if( (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && inOpen[i] < inClose[i - 1] && inOpen[i] > inOpen[i - 1] && inClose[i] < inOpen[i - 1] && inClose[i] > Math.max(inClose[i - 2], inOpen[i - 2]) && Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inOpen[i] < inOpen[i - 1] && inOpen[i] > inClose[i - 1] && inClose[i] > inOpen[i - 1] && inClose[i] < Math.min(inClose[i - 2], inOpen[i - 2]) && Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            /* upside gap */
            /* 1st: white */
            /* 2nd: black */
            /* that opens within the white rb */
            /* and closes under the white rb */
            /* inside the gap */
            /* size of 2 rb near the same */
            /* downside gap */
            /* 1st: black */
            /* 2nd: white */
            /* that opens within the black rb */
            /* and closes above the black rb */
            /* inside the gap */
            /* size of 2 rb near the same */
            outInteger[outIdx++] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) - Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : 0.0)));
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlTasukiGapUnguarded( int startIdx,
                                         int endIdx,
                                         double inOpen[],
                                         double inHigh[],
                                         double inLow[],
                                         double inClose[],
                                         MInteger outBegIdx,
                                         MInteger outNBElement,
                                         int outInteger[] )
   {
      double NearPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      lookbackTotal = cdlTasukiGapLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && inOpen[i] < inClose[i - 1] && inOpen[i] > inOpen[i - 1] && inClose[i] < inOpen[i - 1] && inClose[i] > Math.max(inClose[i - 2], inOpen[i - 2]) && Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inOpen[i] < inOpen[i - 1] && inOpen[i] > inClose[i - 1] && inClose[i] > inOpen[i - 1] && inClose[i] < Math.min(inClose[i - 2], inOpen[i - 2]) && Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) - Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : 0.0)));
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlTasukiGap( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                int outInteger[] )
   {
      double NearPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
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
      lookbackTotal = cdlTasukiGapLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (double)inOpen[i] < (double)inClose[i - 1] && (double)inOpen[i] > (double)inOpen[i - 1] && (double)inClose[i] < (double)inOpen[i - 1] && (double)inClose[i] > Math.max((double)inClose[i - 2], (double)inOpen[i - 2]) && Math.abs(Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - Math.abs((double)inClose[i] - (double)inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2])) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inOpen[i] < (double)inOpen[i - 1] && (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] > (double)inOpen[i - 1] && (double)inClose[i] < Math.min((double)inClose[i - 2], (double)inOpen[i - 2]) && Math.abs(Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - Math.abs((double)inClose[i] - (double)inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 1] - (double)inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 1] - (double)inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 1] - (double)inLow[NearTrailingIdx - 1]) - Math.abs((double)inClose[NearTrailingIdx - 1] - (double)inOpen[NearTrailingIdx - 1])) : 0.0)));
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlTasukiGapUnguarded( int startIdx,
                                         int endIdx,
                                         float inOpen[],
                                         float inHigh[],
                                         float inLow[],
                                         float inClose[],
                                         MInteger outBegIdx,
                                         MInteger outNBElement,
                                         int outInteger[] )
   {
      double NearPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      lookbackTotal = cdlTasukiGapLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (double)inOpen[i] < (double)inClose[i - 1] && (double)inOpen[i] > (double)inOpen[i - 1] && (double)inClose[i] < (double)inOpen[i - 1] && (double)inClose[i] > Math.max((double)inClose[i - 2], (double)inOpen[i - 2]) && Math.abs(Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - Math.abs((double)inClose[i] - (double)inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2])) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inOpen[i] < (double)inOpen[i - 1] && (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] > (double)inOpen[i - 1] && (double)inClose[i] < Math.min((double)inClose[i - 2], (double)inOpen[i - 2]) && Math.abs(Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - Math.abs((double)inClose[i] - (double)inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 1] - (double)inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 1] - (double)inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 1] - (double)inLow[NearTrailingIdx - 1]) - Math.abs((double)inClose[NearTrailingIdx - 1] - (double)inOpen[NearTrailingIdx - 1])) : 0.0)));
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
