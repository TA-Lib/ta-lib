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
 *  020605 AC   Creation
 */

   public int cdlRiseFall3MethodsLookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      return Math.max(BodyShort_avgPeriod, BodyLong_avgPeriod) + 4 ;

   }
   public RetCode cdlRiseFall3Methods( int startIdx,
                                       int endIdx,
                                       double inOpen[],
                                       double inHigh[],
                                       double inLow[],
                                       double inClose[],
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
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlRiseFall3MethodsLookback();
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
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long white (black) candlestick
       * - then: group of falling (rising) small real body candlesticks (commonly black (white)) that hold within
       *   the prior long candle's range: ideally they should be three but two or more than three are ok too
       * - final candle: long white (black) candle that opens above (below) the previous small candle's close
       *   and closes above (below) the first long candle's close
       * The meaning of "short" and "long" is specified with TA_SetCandleSettings; here only patterns with 3 small candles
       * are considered;
       * outInteger is positive (1 to 100) or negative (-1 to -100)
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 0 - ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) && /* white, 3 black, white  ||  black, 3 white, black */
             ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) &&
             ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) &&
             Math.min(inOpen[i - 3], inClose[i - 3]) < inHigh[i - 4] && /* 2nd to 4th hold within 1st: a part of the real body must be within 1st range */
             Math.max(inOpen[i - 3], inClose[i - 3]) > inLow[i - 4] &&
             Math.min(inOpen[i - 2], inClose[i - 2]) < inHigh[i - 4] &&
             Math.max(inOpen[i - 2], inClose[i - 2]) > inLow[i - 4] &&
             Math.min(inOpen[i - 1], inClose[i - 1]) < inHigh[i - 4] &&
             Math.max(inOpen[i - 1], inClose[i - 1]) > inLow[i - 4] &&
             inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) < inClose[i - 3] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && /* 2nd to 4th are falling (rising) */
             inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) < inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) &&
             inOpen[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) > inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && /* 5th opens above (below) the prior close */
             inClose[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) > inClose[i - 4] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && /* 5th closes above (below) the 1st close */
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
             Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++] = 100 * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1);
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
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0))));
         i += 1;
         BodyShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlRiseFall3MethodsUnguarded( int startIdx,
                                                int endIdx,
                                                double inOpen[],
                                                double inHigh[],
                                                double inLow[],
                                                double inClose[],
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
      lookbackTotal = cdlRiseFall3MethodsLookback();
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
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 0 - ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) && ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) && Math.min(inOpen[i - 3], inClose[i - 3]) < inHigh[i - 4] && Math.max(inOpen[i - 3], inClose[i - 3]) > inLow[i - 4] && Math.min(inOpen[i - 2], inClose[i - 2]) < inHigh[i - 4] && Math.max(inOpen[i - 2], inClose[i - 2]) > inLow[i - 4] && Math.min(inOpen[i - 1], inClose[i - 1]) < inHigh[i - 4] && Math.max(inOpen[i - 1], inClose[i - 1]) > inLow[i - 4] && inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) < inClose[i - 3] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) < inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && inOpen[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) > inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && inClose[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) > inClose[i - 4] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100 * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1);
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) - Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx - totIdx] - inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx - totIdx] - inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx - totIdx] - inLow[BodyShortTrailingIdx - totIdx]) - Math.abs(inClose[BodyShortTrailingIdx - totIdx] - inOpen[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0))));
         i += 1;
         BodyShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlRiseFall3Methods( int startIdx,
                                       int endIdx,
                                       float inOpen[],
                                       float inHigh[],
                                       float inLow[],
                                       float inClose[],
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
      lookbackTotal = cdlRiseFall3MethodsLookback();
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
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 0 - (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) && (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) && Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) < (double)inHigh[i - 4] && Math.max((double)inOpen[i - 3], (double)inClose[i - 3]) > (double)inLow[i - 4] && Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) < (double)inHigh[i - 4] && Math.max((double)inOpen[i - 2], (double)inClose[i - 2]) > (double)inLow[i - 4] && Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) < (double)inHigh[i - 4] && Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) > (double)inLow[i - 4] && (double)inClose[i - 2] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) < (double)inClose[i - 3] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && (double)inClose[i - 1] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) < (double)inClose[i - 2] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && (double)inOpen[i] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) > (double)inClose[i - 1] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && (double)inClose[i] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) > (double)inClose[i - 4] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100 * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1);
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) - Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx - totIdx] - (double)inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx - totIdx] - (double)inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx - totIdx] - (double)inLow[BodyShortTrailingIdx - totIdx]) - Math.abs((double)inClose[BodyShortTrailingIdx - totIdx] - (double)inOpen[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) - Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : 0.0))));
         i += 1;
         BodyShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlRiseFall3MethodsUnguarded( int startIdx,
                                                int endIdx,
                                                float inOpen[],
                                                float inHigh[],
                                                float inLow[],
                                                float inClose[],
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
      lookbackTotal = cdlRiseFall3MethodsLookback();
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
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 0 - (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) && (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) && Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) < (double)inHigh[i - 4] && Math.max((double)inOpen[i - 3], (double)inClose[i - 3]) > (double)inLow[i - 4] && Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) < (double)inHigh[i - 4] && Math.max((double)inOpen[i - 2], (double)inClose[i - 2]) > (double)inLow[i - 4] && Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) < (double)inHigh[i - 4] && Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) > (double)inLow[i - 4] && (double)inClose[i - 2] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) < (double)inClose[i - 3] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && (double)inClose[i - 1] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) < (double)inClose[i - 2] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && (double)inOpen[i] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) > (double)inClose[i - 1] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && (double)inClose[i] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) > (double)inClose[i - 4] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100 * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1);
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - (double)inLow[i - 4]) - Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) - Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx - totIdx] - (double)inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx - totIdx] - (double)inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx - totIdx] - (double)inLow[BodyShortTrailingIdx - totIdx]) - Math.abs((double)inClose[BodyShortTrailingIdx - totIdx] - (double)inOpen[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) - Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : 0.0))));
         i += 1;
         BodyShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CDLRISEFALL3METHODS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlRiseFall3Methods} over the same series.
    * Open with {@link Core#cdlRiseFall3MethodsOpen}; there is no close — the handle is
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
   public static final class CdlRiseFall3MethodsStream {
      final Core core;
      double[] BodyPeriodTotal;
      int totIdx;
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
      int ringPos_BodyShortTrailingIdx;
      int ringCap_BodyShortTrailingIdx;
      int ringLag_BodyShortTrailingIdx;
      double[] ring_BodyShortTrailingIdx_inOpen;
      double[] ring_BodyShortTrailingIdx_inHigh;
      double[] ring_BodyShortTrailingIdx_inLow;
      double[] ring_BodyShortTrailingIdx_inClose;
      int winPos_totIdx;
      int winCap_totIdx;
      double[] win_totIdx_inOpen;
      double[] win_totIdx_inHigh;
      double[] win_totIdx_inLow;
      double[] win_totIdx_inClose;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cs_BodyShort_rangeType;
      int cs_BodyShort_avgPeriod;
      double cs_BodyShort_factor;
      int cur_outInteger;

      CdlRiseFall3MethodsStream( Core core ) { this.core = core; }

      CdlRiseFall3MethodsStream( CdlRiseFall3MethodsStream other ) {
         this.core = other.core;
         this.BodyPeriodTotal = other.BodyPeriodTotal.clone();
         this.totIdx = other.totIdx;
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
         this.ringPos_BodyShortTrailingIdx = other.ringPos_BodyShortTrailingIdx;
         this.ringCap_BodyShortTrailingIdx = other.ringCap_BodyShortTrailingIdx;
         this.ringLag_BodyShortTrailingIdx = other.ringLag_BodyShortTrailingIdx;
         this.ring_BodyShortTrailingIdx_inOpen = other.ring_BodyShortTrailingIdx_inOpen.clone();
         this.ring_BodyShortTrailingIdx_inHigh = other.ring_BodyShortTrailingIdx_inHigh.clone();
         this.ring_BodyShortTrailingIdx_inLow = other.ring_BodyShortTrailingIdx_inLow.clone();
         this.ring_BodyShortTrailingIdx_inClose = other.ring_BodyShortTrailingIdx_inClose.clone();
         this.winPos_totIdx = other.winPos_totIdx;
         this.winCap_totIdx = other.winCap_totIdx;
         this.win_totIdx_inOpen = other.win_totIdx_inOpen.clone();
         this.win_totIdx_inHigh = other.win_totIdx_inHigh.clone();
         this.win_totIdx_inLow = other.win_totIdx_inLow.clone();
         this.win_totIdx_inClose = other.win_totIdx_inClose.clone();
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cur_outInteger = other.cur_outInteger;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.cdlRiseFall3MethodsStreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CdlRiseFall3MethodsStream scratch = new CdlRiseFall3MethodsStream(this);
         core.cdlRiseFall3MethodsStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CdlRiseFall3MethodsStream copy() {
         return new CdlRiseFall3MethodsStream(this);
      }
   }
   void cdlRiseFall3MethodsStreamStep( CdlRiseFall3MethodsStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      int BodyShort_rangeType = sp.cs_BodyShort_rangeType;
      int BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
      double BodyShort_factor = sp.cs_BodyShort_factor;
      sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx] = inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] = inHigh;
      sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx] = inLow;
      sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] = inClose;
      sp.ring_BodyShortTrailingIdx_inOpen[sp.ringPos_BodyShortTrailingIdx] = inOpen;
      sp.ring_BodyShortTrailingIdx_inHigh[sp.ringPos_BodyShortTrailingIdx] = inHigh;
      sp.ring_BodyShortTrailingIdx_inLow[sp.ringPos_BodyShortTrailingIdx] = inLow;
      sp.ring_BodyShortTrailingIdx_inClose[sp.ringPos_BodyShortTrailingIdx] = inClose;
      sp.win_totIdx_inOpen[sp.winPos_totIdx] = inOpen;
      sp.win_totIdx_inHigh[sp.winPos_totIdx] = inHigh;
      sp.win_totIdx_inLow[sp.winPos_totIdx] = inLow;
      sp.win_totIdx_inClose[sp.winPos_totIdx] = inClose;
      if( ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) == 0 - ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) && /* white, 3 black, white  ||  black, 3 white, black */
          ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) == ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) &&
          ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) &&
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - ((inClose >= inOpen) ? 1 : 0 - 1) &&
          Math.min(sp.lag3_inOpen, sp.lag3_inClose) < sp.lag4_inHigh && /* 2nd to 4th hold within 1st: a part of the real body must be within 1st range */
          Math.max(sp.lag3_inOpen, sp.lag3_inClose) > sp.lag4_inLow &&
          Math.min(sp.lag2_inOpen, sp.lag2_inClose) < sp.lag4_inHigh &&
          Math.max(sp.lag2_inOpen, sp.lag2_inClose) > sp.lag4_inLow &&
          Math.min(sp.lag1_inOpen, sp.lag1_inClose) < sp.lag4_inHigh &&
          Math.max(sp.lag1_inOpen, sp.lag1_inClose) > sp.lag4_inLow &&
          sp.lag2_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) < sp.lag3_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) && /* 2nd to 4th are falling (rising) */
          sp.lag1_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) < sp.lag2_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) &&
          inOpen * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) > sp.lag1_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) && /* 5th opens above (below) the prior close */
          inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) > sp.lag4_inClose * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) && /* 5th closes above (below) the 1st close */
          Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag4_inHigh - sp.lag4_inLow) - Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
          Math.abs(sp.lag3_inClose - sp.lag3_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((BodyShort_rangeType == 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((BodyShort_rangeType == 2) ? ((sp.lag3_inHigh - sp.lag3_inLow) - Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          Math.abs(sp.lag2_inClose - sp.lag2_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyShort_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyShort_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyShort_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyShort_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) )
      {
         sp.cur_outInteger = 100 * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1);
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyPeriodTotal[4] = sp.BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag4_inHigh - sp.lag4_inLow) - Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(sp.ring_BodyLongTrailingIdx_inClose[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (sp.ring_BodyLongTrailingIdx_inHigh[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((sp.ring_BodyLongTrailingIdx_inHigh[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx]) - Math.abs(sp.ring_BodyLongTrailingIdx_inClose[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx])) : 0.0))));
      for( sp.totIdx = 3; sp.totIdx >= 1; sp.totIdx -= 1 ) {
         sp.BodyPeriodTotal[sp.totIdx] = sp.BodyPeriodTotal[sp.totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : ((BodyShort_rangeType == 1) ? (sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) : ((BodyShort_rangeType == 2) ? ((sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) - Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(sp.ring_BodyShortTrailingIdx_inClose[(sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx - sp.totIdx) % sp.ringCap_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inOpen[(sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx - sp.totIdx) % sp.ringCap_BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (sp.ring_BodyShortTrailingIdx_inHigh[(sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx - sp.totIdx) % sp.ringCap_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inLow[(sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx - sp.totIdx) % sp.ringCap_BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((sp.ring_BodyShortTrailingIdx_inHigh[(sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx - sp.totIdx) % sp.ringCap_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inLow[(sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx - sp.totIdx) % sp.ringCap_BodyShortTrailingIdx]) - Math.abs(sp.ring_BodyShortTrailingIdx_inClose[(sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx - sp.totIdx) % sp.ringCap_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inOpen[(sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx - sp.totIdx) % sp.ringCap_BodyShortTrailingIdx])) : 0.0))));
      }
      sp.BodyPeriodTotal[0] = sp.BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(sp.ring_BodyLongTrailingIdx_inClose[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx) % sp.ringCap_BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (sp.ring_BodyLongTrailingIdx_inHigh[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx) % sp.ringCap_BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((sp.ring_BodyLongTrailingIdx_inHigh[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx) % sp.ringCap_BodyLongTrailingIdx]) - Math.abs(sp.ring_BodyLongTrailingIdx_inClose[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx) % sp.ringCap_BodyLongTrailingIdx])) : 0.0))));
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
      sp.ring_BodyShortTrailingIdx_inOpen[sp.ringPos_BodyShortTrailingIdx] = inOpen;
      sp.ring_BodyShortTrailingIdx_inHigh[sp.ringPos_BodyShortTrailingIdx] = inHigh;
      sp.ring_BodyShortTrailingIdx_inLow[sp.ringPos_BodyShortTrailingIdx] = inLow;
      sp.ring_BodyShortTrailingIdx_inClose[sp.ringPos_BodyShortTrailingIdx] = inClose;
      sp.ringPos_BodyShortTrailingIdx = sp.ringPos_BodyShortTrailingIdx + 1;
      if( sp.ringPos_BodyShortTrailingIdx >= sp.ringCap_BodyShortTrailingIdx ) {
         sp.ringPos_BodyShortTrailingIdx = 0;
      }
      sp.winPos_totIdx = sp.winPos_totIdx + 1;
      if( sp.winPos_totIdx >= sp.winCap_totIdx ) {
         sp.winPos_totIdx = 0;
      }
   }
   private RetCode cdlRiseFall3MethodsOpenBody( CdlRiseFall3MethodsStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      double[] BodyPeriodTotal = new double[5];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyShortTrailingIdx = 0;
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
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlRiseFall3MethodsLookback();
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
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long white (black) candlestick
       * - then: group of falling (rising) small real body candlesticks (commonly black (white)) that hold within
       *   the prior long candle's range: ideally they should be three but two or more than three are ok too
       * - final candle: long white (black) candle that opens above (below) the previous small candle's close
       *   and closes above (below) the first long candle's close
       * The meaning of "short" and "long" is specified with TA_SetCandleSettings; here only patterns with 3 small candles
       * are considered;
       * outInteger is positive (1 to 100) or negative (-1 to -100)
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 0 - ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) && /* white, 3 black, white  ||  black, 3 white, black */
             ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) &&
             ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) &&
             Math.min(inOpen[i - 3], inClose[i - 3]) < inHigh[i - 4] && /* 2nd to 4th hold within 1st: a part of the real body must be within 1st range */
             Math.max(inOpen[i - 3], inClose[i - 3]) > inLow[i - 4] &&
             Math.min(inOpen[i - 2], inClose[i - 2]) < inHigh[i - 4] &&
             Math.max(inOpen[i - 2], inClose[i - 2]) > inLow[i - 4] &&
             Math.min(inOpen[i - 1], inClose[i - 1]) < inHigh[i - 4] &&
             Math.max(inOpen[i - 1], inClose[i - 1]) > inLow[i - 4] &&
             inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) < inClose[i - 3] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && /* 2nd to 4th are falling (rising) */
             inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) < inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) &&
             inOpen[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) > inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && /* 5th opens above (below) the prior close */
             inClose[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) > inClose[i - 4] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && /* 5th closes above (below) the 1st close */
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
             Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            lastValue_outInteger = 100 * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1);
         } else {
            lastValue_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) - Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx - totIdx] - inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx - totIdx] - inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx - totIdx] - inLow[BodyShortTrailingIdx - totIdx]) - Math.abs(inClose[BodyShortTrailingIdx - totIdx] - inOpen[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0))));
         i += 1;
         BodyShortTrailingIdx += 1;
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
      int capLag_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
      int cap_BodyShortTrailingIdx = capLag_BodyShortTrailingIdx + 4;
      if( capLag_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyShortTrailingIdx = (cap_BodyShortTrailingIdx > 0)? cap_BodyShortTrailingIdx : 1;
      double[] capRing_BodyShortTrailingIdx_inOpen = new double[allocN_BodyShortTrailingIdx];
      for( int fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyShortTrailingIdx_inOpen[fillJ % cap_BodyShortTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_BodyShortTrailingIdx_inHigh = new double[allocN_BodyShortTrailingIdx];
      for( int fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyShortTrailingIdx_inHigh[fillJ % cap_BodyShortTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_BodyShortTrailingIdx_inLow = new double[allocN_BodyShortTrailingIdx];
      for( int fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyShortTrailingIdx_inLow[fillJ % cap_BodyShortTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_BodyShortTrailingIdx_inClose = new double[allocN_BodyShortTrailingIdx];
      for( int fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyShortTrailingIdx_inClose[fillJ % cap_BodyShortTrailingIdx] = inClose[fillJ];
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
      sp.BodyPeriodTotal = BodyPeriodTotal;
      sp.totIdx = totIdx;
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
      sp.ringPos_BodyShortTrailingIdx = historyLen % cap_BodyShortTrailingIdx;
      sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
      sp.ringLag_BodyShortTrailingIdx = capLag_BodyShortTrailingIdx;
      sp.ring_BodyShortTrailingIdx_inOpen = capRing_BodyShortTrailingIdx_inOpen;
      sp.ring_BodyShortTrailingIdx_inHigh = capRing_BodyShortTrailingIdx_inHigh;
      sp.ring_BodyShortTrailingIdx_inLow = capRing_BodyShortTrailingIdx_inLow;
      sp.ring_BodyShortTrailingIdx_inClose = capRing_BodyShortTrailingIdx_inClose;
      sp.winPos_totIdx = 0;
      sp.winCap_totIdx = cap_totIdx;
      sp.win_totIdx_inOpen = capWin_totIdx_inOpen;
      sp.win_totIdx_inHigh = capWin_totIdx_inHigh;
      sp.win_totIdx_inLow = capWin_totIdx_inLow;
      sp.win_totIdx_inClose = capWin_totIdx_inClose;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlRiseFall3MethodsOpenAndFillBody( CdlRiseFall3MethodsStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      double[] BodyPeriodTotal = new double[5];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyShortTrailingIdx = 0;
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
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlRiseFall3MethodsLookback();
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
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long white (black) candlestick
       * - then: group of falling (rising) small real body candlesticks (commonly black (white)) that hold within
       *   the prior long candle's range: ideally they should be three but two or more than three are ok too
       * - final candle: long white (black) candle that opens above (below) the previous small candle's close
       *   and closes above (below) the first long candle's close
       * The meaning of "short" and "long" is specified with TA_SetCandleSettings; here only patterns with 3 small candles
       * are considered;
       * outInteger is positive (1 to 100) or negative (-1 to -100)
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 0 - ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) && /* white, 3 black, white  ||  black, 3 white, black */
             ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) &&
             ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) &&
             Math.min(inOpen[i - 3], inClose[i - 3]) < inHigh[i - 4] && /* 2nd to 4th hold within 1st: a part of the real body must be within 1st range */
             Math.max(inOpen[i - 3], inClose[i - 3]) > inLow[i - 4] &&
             Math.min(inOpen[i - 2], inClose[i - 2]) < inHigh[i - 4] &&
             Math.max(inOpen[i - 2], inClose[i - 2]) > inLow[i - 4] &&
             Math.min(inOpen[i - 1], inClose[i - 1]) < inHigh[i - 4] &&
             Math.max(inOpen[i - 1], inClose[i - 1]) > inLow[i - 4] &&
             inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) < inClose[i - 3] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && /* 2nd to 4th are falling (rising) */
             inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) < inClose[i - 2] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) &&
             inOpen[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) > inClose[i - 1] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && /* 5th opens above (below) the prior close */
             inClose[i] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) > inClose[i - 4] * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) && /* 5th closes above (below) the 1st close */
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - inLow[i - 4]) - Math.abs(inClose[i - 4] - inOpen[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
             Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++] = 100 * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1);
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
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0))));
         i += 1;
         BodyShortTrailingIdx += 1;
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
      int capLag_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
      int cap_BodyShortTrailingIdx = capLag_BodyShortTrailingIdx + 4;
      if( capLag_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyShortTrailingIdx = (cap_BodyShortTrailingIdx > 0)? cap_BodyShortTrailingIdx : 1;
      double[] capRing_BodyShortTrailingIdx_inOpen = new double[allocN_BodyShortTrailingIdx];
      for( int fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyShortTrailingIdx_inOpen[fillJ % cap_BodyShortTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_BodyShortTrailingIdx_inHigh = new double[allocN_BodyShortTrailingIdx];
      for( int fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyShortTrailingIdx_inHigh[fillJ % cap_BodyShortTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_BodyShortTrailingIdx_inLow = new double[allocN_BodyShortTrailingIdx];
      for( int fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyShortTrailingIdx_inLow[fillJ % cap_BodyShortTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_BodyShortTrailingIdx_inClose = new double[allocN_BodyShortTrailingIdx];
      for( int fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyShortTrailingIdx_inClose[fillJ % cap_BodyShortTrailingIdx] = inClose[fillJ];
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
      sp.BodyPeriodTotal = BodyPeriodTotal;
      sp.totIdx = totIdx;
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
      sp.ringPos_BodyShortTrailingIdx = historyLen % cap_BodyShortTrailingIdx;
      sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
      sp.ringLag_BodyShortTrailingIdx = capLag_BodyShortTrailingIdx;
      sp.ring_BodyShortTrailingIdx_inOpen = capRing_BodyShortTrailingIdx_inOpen;
      sp.ring_BodyShortTrailingIdx_inHigh = capRing_BodyShortTrailingIdx_inHigh;
      sp.ring_BodyShortTrailingIdx_inLow = capRing_BodyShortTrailingIdx_inLow;
      sp.ring_BodyShortTrailingIdx_inClose = capRing_BodyShortTrailingIdx_inClose;
      sp.winPos_totIdx = 0;
      sp.winCap_totIdx = cap_totIdx;
      sp.win_totIdx_inOpen = capWin_totIdx_inOpen;
      sp.win_totIdx_inHigh = capWin_totIdx_inHigh;
      sp.win_totIdx_inLow = capWin_totIdx_inLow;
      sp.win_totIdx_inClose = capWin_totIdx_inClose;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlRiseFall3MethodsOpen (composition seam). */
   CdlRiseFall3MethodsStream cdlRiseFall3MethodsOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlRiseFall3MethodsStream sp = new CdlRiseFall3MethodsStream(this);
      RetCode retCode = cdlRiseFall3MethodsOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLRISEFALL3METHODS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLRISEFALL3METHODS open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLRISEFALL3METHODS open: " + retCode);
   }
   /**
    * Open a live CDLRISEFALL3METHODS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlRiseFall3Methods} at that bar.
    * <p>The history must hold at least {@code cdlRiseFall3MethodsLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlRiseFall3MethodsStream cdlRiseFall3MethodsOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return cdlRiseFall3MethodsOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlRiseFall3MethodsOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlRiseFall3Methods} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CdlRiseFall3MethodsStream cdlRiseFall3MethodsOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlRiseFall3MethodsStream sp = new CdlRiseFall3MethodsStream(this);
      RetCode retCode = cdlRiseFall3MethodsOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLRISEFALL3METHODS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLRISEFALL3METHODS openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLRISEFALL3METHODS openAndFill: " + retCode);
   }
