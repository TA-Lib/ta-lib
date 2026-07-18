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

   public int cdlBreakawayLookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      return BodyLong_avgPeriod + 4 ;

   }
   public RetCode cdlBreakaway( int startIdx,
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
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlBreakawayLookback();
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
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black (white)
       * - second candle: black (white) day whose body gaps down (up)
       * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
       * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
       * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
       * The meaning of "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that breakaway is significant in a trend opposite to the last candle, while this
       * function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) && /* 1st, 2nd, 4th same color, 5th opposite */
             ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) &&
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
             (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i - 3], inClose[i - 3]) < Math.min(inOpen[i - 4], inClose[i - 4])) && inHigh[i - 2] < inHigh[i - 3] && inLow[i - 2] < inLow[i - 3] && inHigh[i - 1] < inHigh[i - 2] && inLow[i - 1] < inLow[i - 2] && inClose[i] > inOpen[i - 3] && inClose[i] < inClose[i - 4] || ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i - 3], inClose[i - 3]) > Math.max(inOpen[i - 4], inClose[i - 4])) && inHigh[i - 2] > inHigh[i - 3] && inLow[i - 2] > inLow[i - 3] && inHigh[i - 1] > inHigh[i - 2] && inLow[i - 1] > inLow[i - 2] && inClose[i] < inOpen[i - 3] && inClose[i] > inClose[i - 4]) ) /* when 1st is black: 2nd gaps down 3rd has lower high and low than 2nd 4th has lower high and low than 3rd 5th closes inside the gap when 1st is white: 2nd gaps up 3rd has higher high and low than 2nd 4th has higher high and low than 3rd 5th closes inside the gap */
         {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) - Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlBreakawayUnguarded( int startIdx,
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
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      lookbackTotal = cdlBreakawayLookback();
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
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) && ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) && Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i - 3], inClose[i - 3]) < Math.min(inOpen[i - 4], inClose[i - 4])) && inHigh[i - 2] < inHigh[i - 3] && inLow[i - 2] < inLow[i - 3] && inHigh[i - 1] < inHigh[i - 2] && inLow[i - 1] < inLow[i - 2] && inClose[i] > inOpen[i - 3] && inClose[i] < inClose[i - 4] || ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i - 3], inClose[i - 3]) > Math.max(inOpen[i - 4], inClose[i - 4])) && inHigh[i - 2] > inHigh[i - 3] && inLow[i - 2] > inLow[i - 3] && inHigh[i - 1] > inHigh[i - 2] && inLow[i - 1] > inLow[i - 2] && inClose[i] < inOpen[i - 3] && inClose[i] > inClose[i - 4]) ) {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) - Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlBreakaway( int startIdx,
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
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlBreakawayLookback();
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
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) && (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) && Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 0 - 1 && (Math.max((double)inOpen[i - 3], (double)inClose[i - 3]) < Math.min((double)inOpen[i - 4], (double)inClose[i - 4])) && (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] < (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] < (double)inLow[i - 2] && (double)inClose[i] > (double)inOpen[i - 3] && (double)inClose[i] < (double)inClose[i - 4] || (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) > Math.max((double)inOpen[i - 4], (double)inClose[i - 4])) && (double)inHigh[i - 2] > (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] > (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && (double)inClose[i] < (double)inOpen[i - 3] && (double)inClose[i] > (double)inClose[i - 4]) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) - Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlBreakawayUnguarded( int startIdx,
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
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      lookbackTotal = cdlBreakawayLookback();
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
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) && (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) && Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 0 - 1 && (Math.max((double)inOpen[i - 3], (double)inClose[i - 3]) < Math.min((double)inOpen[i - 4], (double)inClose[i - 4])) && (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] < (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] < (double)inLow[i - 2] && (double)inClose[i] > (double)inOpen[i - 3] && (double)inClose[i] < (double)inClose[i - 4] || (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) > Math.max((double)inOpen[i - 4], (double)inClose[i - 4])) && (double)inHigh[i - 2] > (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] > (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && (double)inClose[i] < (double)inOpen[i - 3] && (double)inClose[i] > (double)inClose[i - 4]) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) - Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CDLBREAKAWAY stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlBreakaway} over the same series.
    * Open with {@link Core#cdlBreakawayOpen}; there is no close — the handle is
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
   public static final class CdlBreakawayStream {
      final Core core;
      double BodyLongPeriodTotal;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag3_inOpen;
      double lag4_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag3_inHigh;
      double lag4_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag3_inLow;
      double lag4_inLow;
      double lag1_inClose;
      double lag2_inClose;
      double lag3_inClose;
      double lag4_inClose;
      int ringPos_BodyLongTrailingIdx;
      int ringCap_BodyLongTrailingIdx;
      int ringLag_BodyLongTrailingIdx;
      double[] ring_BodyLongTrailingIdx_inOpen;
      double[] ring_BodyLongTrailingIdx_inHigh;
      double[] ring_BodyLongTrailingIdx_inLow;
      double[] ring_BodyLongTrailingIdx_inClose;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cur_outInteger;

      CdlBreakawayStream( Core core ) { this.core = core; }

      CdlBreakawayStream( CdlBreakawayStream other ) {
         this.core = other.core;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag3_inOpen = other.lag3_inOpen;
         this.lag4_inOpen = other.lag4_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag3_inHigh = other.lag3_inHigh;
         this.lag4_inHigh = other.lag4_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag3_inLow = other.lag3_inLow;
         this.lag4_inLow = other.lag4_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.lag3_inClose = other.lag3_inClose;
         this.lag4_inClose = other.lag4_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ringLag_BodyLongTrailingIdx = other.ringLag_BodyLongTrailingIdx;
         this.ring_BodyLongTrailingIdx_inOpen = other.ring_BodyLongTrailingIdx_inOpen.clone();
         this.ring_BodyLongTrailingIdx_inHigh = other.ring_BodyLongTrailingIdx_inHigh.clone();
         this.ring_BodyLongTrailingIdx_inLow = other.ring_BodyLongTrailingIdx_inLow.clone();
         this.ring_BodyLongTrailingIdx_inClose = other.ring_BodyLongTrailingIdx_inClose.clone();
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cur_outInteger = other.cur_outInteger;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.cdlBreakawayStreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CdlBreakawayStream scratch = new CdlBreakawayStream(this);
         core.cdlBreakawayStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CdlBreakawayStream copy() {
         return new CdlBreakawayStream(this);
      }
   }
   void cdlBreakawayStreamStep( CdlBreakawayStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx] = inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] = inHigh;
      sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx] = inLow;
      sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] = inClose;
      if( ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) == ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) && /* 1st, 2nd, 4th same color, 5th opposite */
          ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) == ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) &&
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - ((inClose >= inOpen) ? 1 : 0 - 1) &&
          Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag4_inHigh - sp.lag4_inLow) - Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
          (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) == 0 - 1 && (Math.max(sp.lag3_inOpen, sp.lag3_inClose) < Math.min(sp.lag4_inOpen, sp.lag4_inClose)) && sp.lag2_inHigh < sp.lag3_inHigh && sp.lag2_inLow < sp.lag3_inLow && sp.lag1_inHigh < sp.lag2_inHigh && sp.lag1_inLow < sp.lag2_inLow && inClose > sp.lag3_inOpen && inClose < sp.lag4_inClose || ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) == 1 && (Math.min(sp.lag3_inOpen, sp.lag3_inClose) > Math.max(sp.lag4_inOpen, sp.lag4_inClose)) && sp.lag2_inHigh > sp.lag3_inHigh && sp.lag2_inLow > sp.lag3_inLow && sp.lag1_inHigh > sp.lag2_inHigh && sp.lag1_inLow > sp.lag2_inLow && inClose < sp.lag3_inOpen && inClose > sp.lag4_inClose) ) /* when 1st is black: 2nd gaps down 3rd has lower high and low than 2nd 4th has lower high and low than 3rd 5th closes inside the gap when 1st is white: 2nd gaps up 3rd has higher high and low than 2nd 4th has higher high and low than 3rd 5th closes inside the gap */
      {
         sp.cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag4_inHigh - sp.lag4_inLow) - Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(sp.ring_BodyLongTrailingIdx_inClose[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (sp.ring_BodyLongTrailingIdx_inHigh[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((sp.ring_BodyLongTrailingIdx_inHigh[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx]) - Math.abs(sp.ring_BodyLongTrailingIdx_inClose[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx])) : 0.0)));
      sp.lag4_inOpen = sp.lag3_inOpen;
      sp.lag3_inOpen = sp.lag2_inOpen;
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag4_inHigh = sp.lag3_inHigh;
      sp.lag3_inHigh = sp.lag2_inHigh;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag4_inLow = sp.lag3_inLow;
      sp.lag3_inLow = sp.lag2_inLow;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
      sp.lag4_inClose = sp.lag3_inClose;
      sp.lag3_inClose = sp.lag2_inClose;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
      sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx] = inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] = inHigh;
      sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx] = inLow;
      sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] = inClose;
      sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
      if( sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx ) {
         sp.ringPos_BodyLongTrailingIdx = 0;
      }
   }
   private RetCode cdlBreakawayOpenBody( CdlBreakawayStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int lastValue_outInteger = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlBreakawayLookback();
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
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black (white)
       * - second candle: black (white) day whose body gaps down (up)
       * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
       * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
       * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
       * The meaning of "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that breakaway is significant in a trend opposite to the last candle, while this
       * function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) && /* 1st, 2nd, 4th same color, 5th opposite */
             ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) &&
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
             (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i - 3], inClose[i - 3]) < Math.min(inOpen[i - 4], inClose[i - 4])) && inHigh[i - 2] < inHigh[i - 3] && inLow[i - 2] < inLow[i - 3] && inHigh[i - 1] < inHigh[i - 2] && inLow[i - 1] < inLow[i - 2] && inClose[i] > inOpen[i - 3] && inClose[i] < inClose[i - 4] || ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i - 3], inClose[i - 3]) > Math.max(inOpen[i - 4], inClose[i - 4])) && inHigh[i - 2] > inHigh[i - 3] && inLow[i - 2] > inLow[i - 3] && inHigh[i - 1] > inHigh[i - 2] && inLow[i - 1] > inLow[i - 2] && inClose[i] < inOpen[i - 3] && inClose[i] > inClose[i - 4]) ) /* when 1st is black: 2nd gaps down 3rd has lower high and low than 2nd 4th has lower high and low than 3rd 5th closes inside the gap when 1st is white: 2nd gaps up 3rd has higher high and low than 2nd 4th has higher high and low than 3rd 5th closes inside the gap */
         {
            lastValue_outInteger = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            lastValue_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) - Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      int cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 5;
      if( capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyLongTrailingIdx = (cap_BodyLongTrailingIdx > 0)? cap_BodyLongTrailingIdx : 1;
      double[] capRing_BodyLongTrailingIdx_inOpen = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_inOpen[fillJ % cap_BodyLongTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_BodyLongTrailingIdx_inHigh = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_inHigh[fillJ % cap_BodyLongTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_BodyLongTrailingIdx_inLow = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_inLow[fillJ % cap_BodyLongTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_BodyLongTrailingIdx_inClose = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_inClose[fillJ % cap_BodyLongTrailingIdx] = inClose[fillJ];
      }
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag3_inOpen = inOpen[historyLen - 3];
      sp.lag4_inOpen = inOpen[historyLen - 4];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag3_inHigh = inHigh[historyLen - 3];
      sp.lag4_inHigh = inHigh[historyLen - 4];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag3_inLow = inLow[historyLen - 3];
      sp.lag4_inLow = inLow[historyLen - 4];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.lag3_inClose = inClose[historyLen - 3];
      sp.lag4_inClose = inClose[historyLen - 4];
      sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_inOpen = capRing_BodyLongTrailingIdx_inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh = capRing_BodyLongTrailingIdx_inHigh;
      sp.ring_BodyLongTrailingIdx_inLow = capRing_BodyLongTrailingIdx_inLow;
      sp.ring_BodyLongTrailingIdx_inClose = capRing_BodyLongTrailingIdx_inClose;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlBreakawayOpenAndFillBody( CdlBreakawayStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
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
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlBreakawayLookback();
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
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black (white)
       * - second candle: black (white) day whose body gaps down (up)
       * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
       * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
       * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
       * The meaning of "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that breakaway is significant in a trend opposite to the last candle, while this
       * function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) && /* 1st, 2nd, 4th same color, 5th opposite */
             ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) &&
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
             (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i - 3], inClose[i - 3]) < Math.min(inOpen[i - 4], inClose[i - 4])) && inHigh[i - 2] < inHigh[i - 3] && inLow[i - 2] < inLow[i - 3] && inHigh[i - 1] < inHigh[i - 2] && inLow[i - 1] < inLow[i - 2] && inClose[i] > inOpen[i - 3] && inClose[i] < inClose[i - 4] || ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i - 3], inClose[i - 3]) > Math.max(inOpen[i - 4], inClose[i - 4])) && inHigh[i - 2] > inHigh[i - 3] && inLow[i - 2] > inLow[i - 3] && inHigh[i - 1] > inHigh[i - 2] && inLow[i - 1] > inLow[i - 2] && inClose[i] < inOpen[i - 3] && inClose[i] > inClose[i - 4]) ) /* when 1st is black: 2nd gaps down 3rd has lower high and low than 2nd 4th has lower high and low than 3rd 5th closes inside the gap when 1st is white: 2nd gaps up 3rd has higher high and low than 2nd 4th has higher high and low than 3rd 5th closes inside the gap */
         {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) - Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      int cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 5;
      if( capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyLongTrailingIdx = (cap_BodyLongTrailingIdx > 0)? cap_BodyLongTrailingIdx : 1;
      double[] capRing_BodyLongTrailingIdx_inOpen = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_inOpen[fillJ % cap_BodyLongTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_BodyLongTrailingIdx_inHigh = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_inHigh[fillJ % cap_BodyLongTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_BodyLongTrailingIdx_inLow = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_inLow[fillJ % cap_BodyLongTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_BodyLongTrailingIdx_inClose = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_inClose[fillJ % cap_BodyLongTrailingIdx] = inClose[fillJ];
      }
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag3_inOpen = inOpen[historyLen - 3];
      sp.lag4_inOpen = inOpen[historyLen - 4];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag3_inHigh = inHigh[historyLen - 3];
      sp.lag4_inHigh = inHigh[historyLen - 4];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag3_inLow = inLow[historyLen - 3];
      sp.lag4_inLow = inLow[historyLen - 4];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.lag3_inClose = inClose[historyLen - 3];
      sp.lag4_inClose = inClose[historyLen - 4];
      sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_inOpen = capRing_BodyLongTrailingIdx_inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh = capRing_BodyLongTrailingIdx_inHigh;
      sp.ring_BodyLongTrailingIdx_inLow = capRing_BodyLongTrailingIdx_inLow;
      sp.ring_BodyLongTrailingIdx_inClose = capRing_BodyLongTrailingIdx_inClose;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlBreakawayOpen (composition seam). */
   CdlBreakawayStream cdlBreakawayOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlBreakawayStream sp = new CdlBreakawayStream(this);
      RetCode retCode = cdlBreakawayOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLBREAKAWAY open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLBREAKAWAY open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLBREAKAWAY open: " + retCode);
   }
   /**
    * Open a live CDLBREAKAWAY stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlBreakaway} at that bar.
    * <p>The history must hold at least {@code cdlBreakawayLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlBreakawayStream cdlBreakawayOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return cdlBreakawayOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlBreakawayOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlBreakaway} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CdlBreakawayStream cdlBreakawayOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlBreakawayStream sp = new CdlBreakawayStream(this);
      RetCode retCode = cdlBreakawayOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLBREAKAWAY openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLBREAKAWAY openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLBREAKAWAY openAndFill: " + retCode);
   }
