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
