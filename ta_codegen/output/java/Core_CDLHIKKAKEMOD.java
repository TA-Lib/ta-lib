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
 *  122605 AC   Creation
 */

   public int cdlHikkakeModLookback( )
   {
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      return Math.max(1, Near_avgPeriod) + 5 ;

   }
   public RetCode cdlHikkakeMod( int startIdx,
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
      int patternIdx = 0;
      int patternResult = 0;
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
      lookbackTotal = cdlHikkakeModLookback();
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
      NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx - 3 ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      patternIdx = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         /* copy here the pattern recognition code below */
         if( inHigh[i - 2] < inHigh[i - 3] &&
             inLow[i - 2] > inLow[i - 3] &&   /* 2nd: lower high and higher low than 1st */
             inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 3rd: lower high and higher low than 2nd */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] && inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1] && inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 4th: lower high and lower low (bull) 2nd: close near the low (bear) 4th: higher high and higher low (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
         } else if( i <= patternIdx + 3 &&
             (patternResult > 0 && inClose[i] > inHigh[patternIdx - 1] || patternResult < 0 && inClose[i] < inLow[patternIdx - 1]) ) /* search for confirmation if modified hikkake was no more than 3 bars ago close higher than the high of 3rd close lower than the low of 3rd */
         {
            patternIdx = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) - Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle
       * - second candle: candle with range less than first candle and close near the bottom (near the top)
       * - third candle: lower high and higher low than 2nd
       * - fourth candle: lower high and lower low (higher high and higher low) than 3rd
       * outInteger[hikkake bar] is positive (1 to 100) or negative (-1 to -100) meaning bullish or bearish hikkake
       * Confirmation could come in the next 3 days with:
       * - a day that closes higher than the high (lower than the low) of the 3rd candle
       * outInteger[confirmationbar] is equal to 100 + the bullish hikkake result or -100 - the bearish hikkake result
       * Note: if confirmation and a new hikkake come at the same bar, only the new hikkake is reported (the new hikkake
       * overwrites the confirmation of the old hikkake);
       * the user should consider that modified hikkake is a reversal pattern, while hikkake could be both a reversal
       * or a continuation pattern, so bullish (bearish) modified hikkake is significant when appearing in a downtrend
       * (uptrend)
       */
      outIdx = 0;
      do {
         if( inHigh[i - 2] < inHigh[i - 3] &&
             inLow[i - 2] > inLow[i - 3] &&   /* 2nd: lower high and higher low than 1st */
             inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 3rd: lower high and higher low than 2nd */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] && inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1] && inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 4th: lower high and lower low (bull) 2nd: close near the low (bear) 4th: higher high and higher low (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
            outInteger[outIdx++] = patternResult;
         } else if( i <= patternIdx + 3 &&
             (patternResult > 0 && inClose[i] > inHigh[patternIdx - 1] || patternResult < 0 && inClose[i] < inLow[patternIdx - 1]) ) /* search for confirmation if modified hikkake was no more than 3 bars ago close higher than the high of 3rd close lower than the low of 3rd */
         {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternIdx = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) - Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlHikkakeModUnguarded( int startIdx,
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
      int patternIdx = 0;
      int patternResult = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      lookbackTotal = cdlHikkakeModLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx - 3 ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      patternIdx = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( inHigh[i - 2] < inHigh[i - 3] && inLow[i - 2] > inLow[i - 3] && inHigh[i - 1] < inHigh[i - 2] && inLow[i - 1] > inLow[i - 2] && (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] && inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1] && inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && inClose[i] > inHigh[patternIdx - 1] || patternResult < 0 && inClose[i] < inLow[patternIdx - 1]) ) {
            patternIdx = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) - Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( inHigh[i - 2] < inHigh[i - 3] && inLow[i - 2] > inLow[i - 3] && inHigh[i - 1] < inHigh[i - 2] && inLow[i - 1] > inLow[i - 2] && (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] && inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1] && inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
            outInteger[outIdx++] = patternResult;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && inClose[i] > inHigh[patternIdx - 1] || patternResult < 0 && inClose[i] < inLow[patternIdx - 1]) ) {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternIdx = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) - Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlHikkakeMod( int startIdx,
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
      int patternIdx = 0;
      int patternResult = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlHikkakeModLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx - 3 ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      patternIdx = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] && (double)inClose[i - 2] <= (double)inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1] && (double)inClose[i - 2] >= (double)inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && (double)inClose[i] > (double)inHigh[patternIdx - 1] || patternResult < 0 && (double)inClose[i] < (double)inLow[patternIdx - 1]) ) {
            patternIdx = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) - Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] && (double)inClose[i - 2] <= (double)inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1] && (double)inClose[i - 2] >= (double)inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
            outInteger[outIdx++] = patternResult;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && (double)inClose[i] > (double)inHigh[patternIdx - 1] || patternResult < 0 && (double)inClose[i] < (double)inLow[patternIdx - 1]) ) {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternIdx = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) - Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlHikkakeModUnguarded( int startIdx,
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
      int patternIdx = 0;
      int patternResult = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      lookbackTotal = cdlHikkakeModLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx - 3 ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      patternIdx = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] && (double)inClose[i - 2] <= (double)inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1] && (double)inClose[i - 2] >= (double)inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && (double)inClose[i] > (double)inHigh[patternIdx - 1] || patternResult < 0 && (double)inClose[i] < (double)inLow[patternIdx - 1]) ) {
            patternIdx = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) - Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] && (double)inClose[i - 2] <= (double)inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1] && (double)inClose[i - 2] >= (double)inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
            outInteger[outIdx++] = patternResult;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && (double)inClose[i] > (double)inHigh[patternIdx - 1] || patternResult < 0 && (double)inClose[i] < (double)inLow[patternIdx - 1]) ) {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternIdx = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) - Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
