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
 *  022005 AC   Creation
 */

   public int cdlMatHoldLookback( double optInPenetration )
   {
      if( optInPenetration == -4e37 ) {
         optInPenetration = 5e-1;
      } else if( optInPenetration < 0e0 || optInPenetration > 1.7976931348623157e308 ) {
         return -1;
      }
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      return Math.max(BodyShort_avgPeriod, BodyLong_avgPeriod) + 4 ;

   }
   public RetCode cdlMatHold( int startIdx,
                              int endIdx,
                              double inOpen[],
                              double inHigh[],
                              double inLow[],
                              double inClose[],
                              double optInPenetration,
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              int outInteger[] )
   {
      double[] BodyPeriodTotal = new double[5];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyShortTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInPenetration == -4e37 ) {
         optInPenetration = 5e-1;
      } else if( optInPenetration < 0e0 || optInPenetration > 1.7976931348623157e308 ) {
         return RetCode.BadParam;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlMatHoldLookback(optInPenetration);
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
      BodyPeriodTotal[4] = 0;
      BodyPeriodTotal[3] = 0;
      BodyPeriodTotal[2] = 0;
      BodyPeriodTotal[1] = 0;
      BodyPeriodTotal[0] = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[3] = BodyPeriodTotal[3] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)));
         BodyPeriodTotal[2] = BodyPeriodTotal[2] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         BodyPeriodTotal[1] = BodyPeriodTotal[1] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long white candle
       * - upside gap between the first and the second bodies
       * - second candle: small black candle
       * - third and fourth candles: falling small real body candlesticks (commonly black) that hold within the long
       *   white candle's body and are higher than the reaction days of the rising three methods
       * - fifth candle: white candle that opens above the previous small candle's close and closes higher than the
       *   high of the highest reaction day
       * The meaning of "short" and "long" is specified with TA_SetCandleSettings;
       * "hold within" means "a part of the real body must be within";
       * optInPenetration is the maximum percentage of the first white body the reaction days can penetrate (it is
       * to specify how much the reaction days should be "higher than the reaction days of the rising three methods")
       * outInteger is positive (1 to 100): mat hold is always bullish
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 1 &&     /* white, black, 2 black or white, white */
             ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == 0 - 1 &&
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&
             (Math.min(inOpen[i - 3], inClose[i - 3]) > Math.max(inOpen[i - 4], inClose[i - 4])) && /* upside gap 1st to 2nd */
             Math.min(inOpen[i - 2], inClose[i - 2]) < inClose[i - 4] && /* 3rd to 4th hold within 1st: a part of the real body must be within 1st real body */
             Math.min(inOpen[i - 1], inClose[i - 1]) < inClose[i - 4] &&
             Math.min(inOpen[i - 2], inClose[i - 2]) > inClose[i - 4] - Math.abs(inClose[i - 4] - inOpen[i - 4]) * optInPenetration && /* reaction days penetrate first body less than optInPenetration percent */
             Math.min(inOpen[i - 1], inClose[i - 1]) > inClose[i - 4] - Math.abs(inClose[i - 4] - inOpen[i - 4]) * optInPenetration &&
             Math.max(inClose[i - 2], inOpen[i - 2]) < inOpen[i - 3] &&  /* 2nd to 4th are falling */
             Math.max(inClose[i - 1], inOpen[i - 1]) < Math.max(inClose[i - 2], inOpen[i - 2]) &&
             inOpen[i] > inClose[i - 1] &&                               /* 5th opens above the prior close */
             inClose[i] > Math.max(Math.max(inHigh[i - 3], inHigh[i - 2]), inHigh[i - 1]) && /* 5th closes above the highest high of the reaction days */
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small */
             Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) - Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx - totIdx] - inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx - totIdx] - inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx - totIdx] - inLow[BodyShortTrailingIdx - totIdx]) - Math.abs(inClose[BodyShortTrailingIdx - totIdx] - inOpen[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         BodyShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlMatHoldUnguarded( int startIdx,
                                       int endIdx,
                                       double inOpen[],
                                       double inHigh[],
                                       double inLow[],
                                       double inClose[],
                                       double optInPenetration,
                                       MInteger outBegIdx,
                                       MInteger outNBElement,
                                       int outInteger[] )
   {
      double[] BodyPeriodTotal = new double[5];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyShortTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      lookbackTotal = cdlMatHoldLookback(optInPenetration);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal[4] = 0;
      BodyPeriodTotal[3] = 0;
      BodyPeriodTotal[2] = 0;
      BodyPeriodTotal[1] = 0;
      BodyPeriodTotal[0] = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[3] = BodyPeriodTotal[3] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)));
         BodyPeriodTotal[2] = BodyPeriodTotal[2] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         BodyPeriodTotal[1] = BodyPeriodTotal[1] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 1 && ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i - 3], inClose[i - 3]) > Math.max(inOpen[i - 4], inClose[i - 4])) && Math.min(inOpen[i - 2], inClose[i - 2]) < inClose[i - 4] && Math.min(inOpen[i - 1], inClose[i - 1]) < inClose[i - 4] && Math.min(inOpen[i - 2], inClose[i - 2]) > inClose[i - 4] - Math.abs(inClose[i - 4] - inOpen[i - 4]) * optInPenetration && Math.min(inOpen[i - 1], inClose[i - 1]) > inClose[i - 4] - Math.abs(inClose[i - 4] - inOpen[i - 4]) * optInPenetration && Math.max(inClose[i - 2], inOpen[i - 2]) < inOpen[i - 3] && Math.max(inClose[i - 1], inOpen[i - 1]) < Math.max(inClose[i - 2], inOpen[i - 2]) && inOpen[i] > inClose[i - 1] && inClose[i] > Math.max(Math.max(inHigh[i - 3], inHigh[i - 2]), inHigh[i - 1]) && Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) - Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx - totIdx] - inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx - totIdx] - inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx - totIdx] - inLow[BodyShortTrailingIdx - totIdx]) - Math.abs(inClose[BodyShortTrailingIdx - totIdx] - inOpen[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         BodyShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlMatHold( int startIdx,
                              int endIdx,
                              float inOpen[],
                              float inHigh[],
                              float inLow[],
                              float inClose[],
                              double optInPenetration,
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              int outInteger[] )
   {
      double[] BodyPeriodTotal = new double[5];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyShortTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInPenetration == -4e37 ) {
         optInPenetration = 5e-1;
      } else if( optInPenetration < 0e0 || optInPenetration > 1.7976931348623157e308 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = cdlMatHoldLookback(optInPenetration);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal[4] = 0;
      BodyPeriodTotal[3] = 0;
      BodyPeriodTotal[2] = 0;
      BodyPeriodTotal[1] = 0;
      BodyPeriodTotal[0] = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[3] = BodyPeriodTotal[3] + ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)));
         BodyPeriodTotal[2] = BodyPeriodTotal[2] + ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         BodyPeriodTotal[1] = BodyPeriodTotal[1] + ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) > Math.max((double)inOpen[i - 4], (double)inClose[i - 4])) && Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) < (double)inClose[i - 4] && Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) < (double)inClose[i - 4] && Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) > (double)inClose[i - 4] - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) * optInPenetration && Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > (double)inClose[i - 4] - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) * optInPenetration && Math.max((double)inClose[i - 2], (double)inOpen[i - 2]) < (double)inOpen[i - 3] && Math.max((double)inClose[i - 1], (double)inOpen[i - 1]) < Math.max((double)inClose[i - 2], (double)inOpen[i - 2]) && (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] > Math.max(Math.max((double)inHigh[i - 3], (double)inHigh[i - 2]), (double)inHigh[i - 1]) && Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) - Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx - totIdx] - (double)inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx - totIdx] - (double)inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx - totIdx] - (double)inLow[BodyShortTrailingIdx - totIdx]) - Math.abs((double)inClose[BodyShortTrailingIdx - totIdx] - (double)inOpen[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         BodyShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlMatHoldUnguarded( int startIdx,
                                       int endIdx,
                                       float inOpen[],
                                       float inHigh[],
                                       float inLow[],
                                       float inClose[],
                                       double optInPenetration,
                                       MInteger outBegIdx,
                                       MInteger outNBElement,
                                       int outInteger[] )
   {
      double[] BodyPeriodTotal = new double[5];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyShortTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      lookbackTotal = cdlMatHoldLookback(optInPenetration);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal[4] = 0;
      BodyPeriodTotal[3] = 0;
      BodyPeriodTotal[2] = 0;
      BodyPeriodTotal[1] = 0;
      BodyPeriodTotal[0] = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[3] = BodyPeriodTotal[3] + ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)));
         BodyPeriodTotal[2] = BodyPeriodTotal[2] + ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         BodyPeriodTotal[1] = BodyPeriodTotal[1] + ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) > Math.max((double)inOpen[i - 4], (double)inClose[i - 4])) && Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) < (double)inClose[i - 4] && Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) < (double)inClose[i - 4] && Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) > (double)inClose[i - 4] - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) * optInPenetration && Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > (double)inClose[i - 4] - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) * optInPenetration && Math.max((double)inClose[i - 2], (double)inOpen[i - 2]) < (double)inOpen[i - 3] && Math.max((double)inClose[i - 1], (double)inOpen[i - 1]) < Math.max((double)inClose[i - 2], (double)inOpen[i - 2]) && (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] > Math.max(Math.max((double)inHigh[i - 3], (double)inHigh[i - 2]), (double)inHigh[i - 1]) && Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) - Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx - totIdx] - (double)inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx - totIdx] - (double)inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx - totIdx] - (double)inLow[BodyShortTrailingIdx - totIdx]) - Math.abs((double)inClose[BodyShortTrailingIdx - totIdx] - (double)inOpen[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         BodyShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
