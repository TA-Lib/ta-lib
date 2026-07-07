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
 *  032005 AC   Creation
 */

   public int cdlStickSandwichLookback( )
   {
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      return Equal_avgPeriod + 2 ;

   }
   public RetCode cdlStickSandwich( int startIdx,
                                    int endIdx,
                                    double inOpen[],
                                    double inHigh[],
                                    double inLow[],
                                    double inClose[],
                                    MInteger outBegIdx,
                                    MInteger outNBElement,
                                    int outInteger[] )
   {
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlStickSandwichLookback();
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
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: black candle
       * - second candle: white candle that trades only above the prior close (low > prior close)
       * - third candle: black candle with the close equal to the first candle's close
       * The meaning of "equal" is specified with TA_SetCandleSettings
       * outInteger is always positive (1 to 100): stick sandwich is always bullish;
       * the user should consider that stick sandwich is significant when coming in a downtrend,
       * while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* first black */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 &&     /* second white */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* third black */
             inLow[i - 1] > inClose[i - 2] &&                            /* 2nd low > prior close */
             inClose[i] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st and 3rd same close */
             inClose[i] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 2] - inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 2] - inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 2] - inLow[EqualTrailingIdx - 2]) - Math.abs(inClose[EqualTrailingIdx - 2] - inOpen[EqualTrailingIdx - 2])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlStickSandwichUnguarded( int startIdx,
                                             int endIdx,
                                             double inOpen[],
                                             double inHigh[],
                                             double inLow[],
                                             double inClose[],
                                             MInteger outBegIdx,
                                             MInteger outNBElement,
                                             int outInteger[] )
   {
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      lookbackTotal = cdlStickSandwichLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && inLow[i - 1] > inClose[i - 2] && inClose[i] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && inClose[i] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 2] - inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 2] - inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 2] - inLow[EqualTrailingIdx - 2]) - Math.abs(inClose[EqualTrailingIdx - 2] - inOpen[EqualTrailingIdx - 2])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlStickSandwich( int startIdx,
                                    int endIdx,
                                    float inOpen[],
                                    float inHigh[],
                                    float inLow[],
                                    float inClose[],
                                    MInteger outBegIdx,
                                    MInteger outNBElement,
                                    int outInteger[] )
   {
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlStickSandwichLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (double)inLow[i - 1] > (double)inClose[i - 2] && (double)inClose[i] <= (double)inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inClose[i] >= (double)inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs((double)inClose[EqualTrailingIdx - 2] - (double)inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[EqualTrailingIdx - 2] - (double)inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[EqualTrailingIdx - 2] - (double)inLow[EqualTrailingIdx - 2]) - Math.abs((double)inClose[EqualTrailingIdx - 2] - (double)inOpen[EqualTrailingIdx - 2])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlStickSandwichUnguarded( int startIdx,
                                             int endIdx,
                                             float inOpen[],
                                             float inHigh[],
                                             float inLow[],
                                             float inClose[],
                                             MInteger outBegIdx,
                                             MInteger outNBElement,
                                             int outInteger[] )
   {
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      lookbackTotal = cdlStickSandwichLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (double)inLow[i - 1] > (double)inClose[i - 2] && (double)inClose[i] <= (double)inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inClose[i] >= (double)inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs((double)inClose[EqualTrailingIdx - 2] - (double)inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[EqualTrailingIdx - 2] - (double)inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[EqualTrailingIdx - 2] - (double)inLow[EqualTrailingIdx - 2]) - Math.abs((double)inClose[EqualTrailingIdx - 2] - (double)inOpen[EqualTrailingIdx - 2])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
