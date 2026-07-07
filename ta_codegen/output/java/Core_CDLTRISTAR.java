/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  CSB      Christopher Barnhouse
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  100204 AC      Creation
 *  051005 CSB,AC  Fix #1199526 for out-of-bound write in output.
 */

   public int cdlTristarLookback( )
   {
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      return BodyDoji_avgPeriod + 2 ;

   }
   public RetCode cdlTristar( int startIdx,
                              int endIdx,
                              double inOpen[],
                              double inHigh[],
                              double inLow[],
                              double inClose[],
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlTristarLookback();
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
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - 2 - BodyDoji_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - 3 consecutive doji days
       * - the second doji is a star
       * The meaning of "doji" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish
       */
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 2] - inOpen[i - 2]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: doji */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
             Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            /* 3rd: doji */
            outInteger[outIdx] = 0;
            if( (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && /* 2nd gaps up */
                Math.max(inOpen[i], inClose[i]) < Math.max(inOpen[i - 1], inClose[i - 1]) ) /* 3rd is not higher than 2nd */
            {
               outInteger[outIdx] = 0 - 100;
            }
            if( (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && /* 2nd gaps down */
                Math.min(inOpen[i], inClose[i]) > Math.min(inOpen[i - 1], inClose[i - 1]) ) /* 3rd is not lower than 2nd */
            {
               outInteger[outIdx] = 100;
            }
            outIdx += 1;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlTristarUnguarded( int startIdx,
                                       int endIdx,
                                       double inOpen[],
                                       double inHigh[],
                                       double inLow[],
                                       double inClose[],
                                       MInteger outBegIdx,
                                       MInteger outNBElement,
                                       int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      lookbackTotal = cdlTristarLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - 2 - BodyDoji_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 2] - inOpen[i - 2]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx] = 0;
            if( (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && Math.max(inOpen[i], inClose[i]) < Math.max(inOpen[i - 1], inClose[i - 1]) ) {
               outInteger[outIdx] = 0 - 100;
            }
            if( (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && Math.min(inOpen[i], inClose[i]) > Math.min(inOpen[i - 1], inClose[i - 1]) ) {
               outInteger[outIdx] = 100;
            }
            outIdx += 1;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlTristar( int startIdx,
                              int endIdx,
                              float inOpen[],
                              float inHigh[],
                              float inLow[],
                              float inClose[],
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlTristarLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - 2 - BodyDoji_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx] = 0;
            if( (Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) && Math.max((double)inOpen[i], (double)inClose[i]) < Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) ) {
               outInteger[outIdx] = 0 - 100;
            }
            if( (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2])) && Math.min((double)inOpen[i], (double)inClose[i]) > Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) ) {
               outInteger[outIdx] = 100;
            }
            outIdx += 1;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) - Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlTristarUnguarded( int startIdx,
                                       int endIdx,
                                       float inOpen[],
                                       float inHigh[],
                                       float inLow[],
                                       float inClose[],
                                       MInteger outBegIdx,
                                       MInteger outNBElement,
                                       int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      lookbackTotal = cdlTristarLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - 2 - BodyDoji_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx] = 0;
            if( (Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) && Math.max((double)inOpen[i], (double)inClose[i]) < Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) ) {
               outInteger[outIdx] = 0 - 100;
            }
            if( (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2])) && Math.min((double)inOpen[i], (double)inClose[i]) > Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) ) {
               outInteger[outIdx] = 100;
            }
            outIdx += 1;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) - Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
