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
 *  121204 AC   Creation
 */

   public int cdlThrustingLookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      return Math.max(Equal_avgPeriod, BodyLong_avgPeriod) + 1 ;

   }
   public RetCode cdlThrusting( int startIdx,
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
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
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
      lookbackTotal = cdlThrustingLookback();
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
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black candle
       * - second candle: white candle with open below previous day low and close into previous day body under the midpoint;
       * to differentiate it from in-neck the close should not be equal to the black candle's close
       * The meaning of "equal" is specified with TA_SetCandleSettings
       * outInteger is negative (-1 to -100): thrusting pattern is always bearish
       * the user should consider that the thrusting pattern is significant when it appears in a downtrend and it could be
       * even bullish "when coming in an uptrend or occurring twice within several days" (Steve Nison says), while this
       * function does not consider the trend
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 1st: black */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&             /* 2nd: white */
             inOpen[i] < inLow[i - 1] &&                                 /* open below prior low */
             inClose[i] > inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* close into prior body */
             inClose[i] <= Math.fma(Math.abs(inClose[i - 1] - inOpen[i - 1]), 0.5, inClose[i - 1]) ) /* under the midpoint */
         {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) - Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : 0.0)));
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) - Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlThrustingUnguarded( int startIdx,
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
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      lookbackTotal = cdlThrustingLookback();
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
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inOpen[i] < inLow[i - 1] && inClose[i] > inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && inClose[i] <= Math.fma(Math.abs(inClose[i - 1] - inOpen[i - 1]), 0.5, inClose[i - 1]) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) - Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : 0.0)));
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) - Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlThrusting( int startIdx,
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
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlThrustingLookback();
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
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inOpen[i] < (double)inLow[i - 1] && (double)inClose[i] > (double)inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inClose[i] <= Math.fma(Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]), 0.5, (double)inClose[i - 1]) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs((double)inClose[EqualTrailingIdx - 1] - (double)inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[EqualTrailingIdx - 1] - (double)inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[EqualTrailingIdx - 1] - (double)inLow[EqualTrailingIdx - 1]) - Math.abs((double)inClose[EqualTrailingIdx - 1] - (double)inOpen[EqualTrailingIdx - 1])) : 0.0)));
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 1] - (double)inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 1] - (double)inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 1] - (double)inLow[BodyLongTrailingIdx - 1]) - Math.abs((double)inClose[BodyLongTrailingIdx - 1] - (double)inOpen[BodyLongTrailingIdx - 1])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlThrustingUnguarded( int startIdx,
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
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      lookbackTotal = cdlThrustingLookback();
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
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inOpen[i] < (double)inLow[i - 1] && (double)inClose[i] > (double)inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inClose[i] <= Math.fma(Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]), 0.5, (double)inClose[i - 1]) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs((double)inClose[EqualTrailingIdx - 1] - (double)inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[EqualTrailingIdx - 1] - (double)inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[EqualTrailingIdx - 1] - (double)inLow[EqualTrailingIdx - 1]) - Math.abs((double)inClose[EqualTrailingIdx - 1] - (double)inOpen[EqualTrailingIdx - 1])) : 0.0)));
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 1] - (double)inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 1] - (double)inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 1] - (double)inLow[BodyLongTrailingIdx - 1]) - Math.abs((double)inClose[BodyLongTrailingIdx - 1] - (double)inOpen[BodyLongTrailingIdx - 1])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CDLTHRUSTING stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlThrusting} over the same series.
    * Open with {@link Core#cdlThrustingOpen}; there is no close — the handle is
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
   public static final class CdlThrustingStream {
      final Core core;
      double EqualPeriodTotal;
      double BodyLongPeriodTotal;
      double lag1_inOpen;
      double lag1_inHigh;
      double lag1_inLow;
      double lag1_inClose;
      int ringPos_BodyLongTrailingIdx;
      int ringCap_BodyLongTrailingIdx;
      int ringLag_BodyLongTrailingIdx;
      double[] ring_BodyLongTrailingIdx_inOpen;
      double[] ring_BodyLongTrailingIdx_inHigh;
      double[] ring_BodyLongTrailingIdx_inLow;
      double[] ring_BodyLongTrailingIdx_inClose;
      int ringPos_EqualTrailingIdx;
      int ringCap_EqualTrailingIdx;
      int ringLag_EqualTrailingIdx;
      double[] ring_EqualTrailingIdx_inOpen;
      double[] ring_EqualTrailingIdx_inHigh;
      double[] ring_EqualTrailingIdx_inLow;
      double[] ring_EqualTrailingIdx_inClose;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cs_Equal_rangeType;
      int cs_Equal_avgPeriod;
      double cs_Equal_factor;
      int cur_outInteger;

      CdlThrustingStream( Core core ) { this.core = core; }

      CdlThrustingStream( CdlThrustingStream other ) {
         this.core = other.core;
         this.EqualPeriodTotal = other.EqualPeriodTotal;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ringLag_BodyLongTrailingIdx = other.ringLag_BodyLongTrailingIdx;
         this.ring_BodyLongTrailingIdx_inOpen = other.ring_BodyLongTrailingIdx_inOpen.clone();
         this.ring_BodyLongTrailingIdx_inHigh = other.ring_BodyLongTrailingIdx_inHigh.clone();
         this.ring_BodyLongTrailingIdx_inLow = other.ring_BodyLongTrailingIdx_inLow.clone();
         this.ring_BodyLongTrailingIdx_inClose = other.ring_BodyLongTrailingIdx_inClose.clone();
         this.ringPos_EqualTrailingIdx = other.ringPos_EqualTrailingIdx;
         this.ringCap_EqualTrailingIdx = other.ringCap_EqualTrailingIdx;
         this.ringLag_EqualTrailingIdx = other.ringLag_EqualTrailingIdx;
         this.ring_EqualTrailingIdx_inOpen = other.ring_EqualTrailingIdx_inOpen.clone();
         this.ring_EqualTrailingIdx_inHigh = other.ring_EqualTrailingIdx_inHigh.clone();
         this.ring_EqualTrailingIdx_inLow = other.ring_EqualTrailingIdx_inLow.clone();
         this.ring_EqualTrailingIdx_inClose = other.ring_EqualTrailingIdx_inClose.clone();
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_Equal_rangeType = other.cs_Equal_rangeType;
         this.cs_Equal_avgPeriod = other.cs_Equal_avgPeriod;
         this.cs_Equal_factor = other.cs_Equal_factor;
         this.cur_outInteger = other.cur_outInteger;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.cdlThrustingStreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CdlThrustingStream scratch = new CdlThrustingStream(this);
         core.cdlThrustingStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CdlThrustingStream copy() {
         return new CdlThrustingStream(this);
      }
   }
   void cdlThrustingStreamStep( CdlThrustingStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      int Equal_rangeType = sp.cs_Equal_rangeType;
      int Equal_avgPeriod = sp.cs_Equal_avgPeriod;
      double Equal_factor = sp.cs_Equal_factor;
      sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx] = inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] = inHigh;
      sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx] = inLow;
      sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] = inClose;
      sp.ring_EqualTrailingIdx_inOpen[sp.ringPos_EqualTrailingIdx] = inOpen;
      sp.ring_EqualTrailingIdx_inHigh[sp.ringPos_EqualTrailingIdx] = inHigh;
      sp.ring_EqualTrailingIdx_inLow[sp.ringPos_EqualTrailingIdx] = inLow;
      sp.ring_EqualTrailingIdx_inClose[sp.ringPos_EqualTrailingIdx] = inClose;
      if( ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* 1st: black */
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 1 &&                     /* 2nd: white */
          inOpen < sp.lag1_inLow &&                                     /* open below prior low */
          inClose > sp.lag1_inClose + ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* close into prior body */
          inClose <= Math.fma(Math.abs(sp.lag1_inClose - sp.lag1_inOpen), 0.5, sp.lag1_inClose) ) /* under the midpoint */
      {
         sp.cur_outInteger = 0 - 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(sp.ring_EqualTrailingIdx_inClose[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx] - sp.ring_EqualTrailingIdx_inOpen[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx])) : ((Equal_rangeType == 1) ? (sp.ring_EqualTrailingIdx_inHigh[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx] - sp.ring_EqualTrailingIdx_inLow[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx]) : ((Equal_rangeType == 2) ? ((sp.ring_EqualTrailingIdx_inHigh[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx] - sp.ring_EqualTrailingIdx_inLow[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx]) - Math.abs(sp.ring_EqualTrailingIdx_inClose[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx] - sp.ring_EqualTrailingIdx_inOpen[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx])) : 0.0)));
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(sp.ring_BodyLongTrailingIdx_inClose[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (sp.ring_BodyLongTrailingIdx_inHigh[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((sp.ring_BodyLongTrailingIdx_inHigh[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx]) - Math.abs(sp.ring_BodyLongTrailingIdx_inClose[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 1) % sp.ringCap_BodyLongTrailingIdx])) : 0.0)));
      sp.lag1_inOpen = inOpen;
      sp.lag1_inHigh = inHigh;
      sp.lag1_inLow = inLow;
      sp.lag1_inClose = inClose;
      sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx] = inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] = inHigh;
      sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx] = inLow;
      sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] = inClose;
      sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
      if( sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx ) {
         sp.ringPos_BodyLongTrailingIdx = 0;
      }
      sp.ring_EqualTrailingIdx_inOpen[sp.ringPos_EqualTrailingIdx] = inOpen;
      sp.ring_EqualTrailingIdx_inHigh[sp.ringPos_EqualTrailingIdx] = inHigh;
      sp.ring_EqualTrailingIdx_inLow[sp.ringPos_EqualTrailingIdx] = inLow;
      sp.ring_EqualTrailingIdx_inClose[sp.ringPos_EqualTrailingIdx] = inClose;
      sp.ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx + 1;
      if( sp.ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx ) {
         sp.ringPos_EqualTrailingIdx = 0;
      }
   }
   private RetCode cdlThrustingOpenBody( CdlThrustingStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      double EqualPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
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
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlThrustingLookback();
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
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black candle
       * - second candle: white candle with open below previous day low and close into previous day body under the midpoint;
       * to differentiate it from in-neck the close should not be equal to the black candle's close
       * The meaning of "equal" is specified with TA_SetCandleSettings
       * outInteger is negative (-1 to -100): thrusting pattern is always bearish
       * the user should consider that the thrusting pattern is significant when it appears in a downtrend and it could be
       * even bullish "when coming in an uptrend or occurring twice within several days" (Steve Nison says), while this
       * function does not consider the trend
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 1st: black */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&             /* 2nd: white */
             inOpen[i] < inLow[i - 1] &&                                 /* open below prior low */
             inClose[i] > inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* close into prior body */
             inClose[i] <= Math.fma(Math.abs(inClose[i - 1] - inOpen[i - 1]), 0.5, inClose[i - 1]) ) /* under the midpoint */
         {
            lastValue_outInteger = 0 - 100;
         } else {
            lastValue_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) - Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : 0.0)));
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) - Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      int cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 2;
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
      int capLag_EqualTrailingIdx = i - EqualTrailingIdx;
      int cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 2;
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
      sp.EqualPeriodTotal = EqualPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_inOpen = capRing_BodyLongTrailingIdx_inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh = capRing_BodyLongTrailingIdx_inHigh;
      sp.ring_BodyLongTrailingIdx_inLow = capRing_BodyLongTrailingIdx_inLow;
      sp.ring_BodyLongTrailingIdx_inClose = capRing_BodyLongTrailingIdx_inClose;
      sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
      sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
      sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
      sp.ring_EqualTrailingIdx_inOpen = capRing_EqualTrailingIdx_inOpen;
      sp.ring_EqualTrailingIdx_inHigh = capRing_EqualTrailingIdx_inHigh;
      sp.ring_EqualTrailingIdx_inLow = capRing_EqualTrailingIdx_inLow;
      sp.ring_EqualTrailingIdx_inClose = capRing_EqualTrailingIdx_inClose;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_Equal_rangeType = Equal_rangeType;
      sp.cs_Equal_avgPeriod = Equal_avgPeriod;
      sp.cs_Equal_factor = Equal_factor;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlThrustingOpenAndFillBody( CdlThrustingStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      double EqualPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
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
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlThrustingLookback();
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
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black candle
       * - second candle: white candle with open below previous day low and close into previous day body under the midpoint;
       * to differentiate it from in-neck the close should not be equal to the black candle's close
       * The meaning of "equal" is specified with TA_SetCandleSettings
       * outInteger is negative (-1 to -100): thrusting pattern is always bearish
       * the user should consider that the thrusting pattern is significant when it appears in a downtrend and it could be
       * even bullish "when coming in an uptrend or occurring twice within several days" (Steve Nison says), while this
       * function does not consider the trend
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 1st: black */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&             /* 2nd: white */
             inOpen[i] < inLow[i - 1] &&                                 /* open below prior low */
             inClose[i] > inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* close into prior body */
             inClose[i] <= Math.fma(Math.abs(inClose[i - 1] - inOpen[i - 1]), 0.5, inClose[i - 1]) ) /* under the midpoint */
         {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) - Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : 0.0)));
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 1] - inLow[BodyLongTrailingIdx - 1]) - Math.abs(inClose[BodyLongTrailingIdx - 1] - inOpen[BodyLongTrailingIdx - 1])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      int cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 2;
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
      int capLag_EqualTrailingIdx = i - EqualTrailingIdx;
      int cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 2;
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
      sp.EqualPeriodTotal = EqualPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_inOpen = capRing_BodyLongTrailingIdx_inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh = capRing_BodyLongTrailingIdx_inHigh;
      sp.ring_BodyLongTrailingIdx_inLow = capRing_BodyLongTrailingIdx_inLow;
      sp.ring_BodyLongTrailingIdx_inClose = capRing_BodyLongTrailingIdx_inClose;
      sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
      sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
      sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
      sp.ring_EqualTrailingIdx_inOpen = capRing_EqualTrailingIdx_inOpen;
      sp.ring_EqualTrailingIdx_inHigh = capRing_EqualTrailingIdx_inHigh;
      sp.ring_EqualTrailingIdx_inLow = capRing_EqualTrailingIdx_inLow;
      sp.ring_EqualTrailingIdx_inClose = capRing_EqualTrailingIdx_inClose;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_Equal_rangeType = Equal_rangeType;
      sp.cs_Equal_avgPeriod = Equal_avgPeriod;
      sp.cs_Equal_factor = Equal_factor;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlThrustingOpen (composition seam). */
   CdlThrustingStream cdlThrustingOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlThrustingStream sp = new CdlThrustingStream(this);
      RetCode retCode = cdlThrustingOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLTHRUSTING open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLTHRUSTING open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLTHRUSTING open: " + retCode);
   }
   /**
    * Open a live CDLTHRUSTING stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlThrusting} at that bar.
    * <p>The history must hold at least {@code cdlThrustingLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlThrustingStream cdlThrustingOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return cdlThrustingOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlThrustingOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlThrusting} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CdlThrustingStream cdlThrustingOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlThrustingStream sp = new CdlThrustingStream(this);
      RetCode retCode = cdlThrustingOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLTHRUSTING openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLTHRUSTING openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLTHRUSTING openAndFill: " + retCode);
   }
