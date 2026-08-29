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

   /**
    * Number of leading input bars {@link Core#CDLRISEFALL3METHODS} consumes
    * before it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLRISEFALL3METHODS_Lookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      return Math.max(BodyShort_avgPeriod, BodyLong_avgPeriod) + 4 ;

   }
   RetCode CDLRISEFALL3METHODS_Impl( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLRISEFALL3METHODS_Lookback();
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
         BodyPeriodTotal[3] = BodyPeriodTotal[3] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
         BodyPeriodTotal[2] = BodyPeriodTotal[2] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         BodyPeriodTotal[1] = BodyPeriodTotal[1] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
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
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
             Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++] = 100 * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1);
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx - totIdx] - inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx - totIdx] - inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx - totIdx] - (((inClose[BodyShortTrailingIdx - totIdx]) >= (inOpen[BodyShortTrailingIdx - totIdx])) ? (inClose[BodyShortTrailingIdx - totIdx]) : (inOpen[BodyShortTrailingIdx - totIdx]))) + ((((inClose[BodyShortTrailingIdx - totIdx]) >= (inOpen[BodyShortTrailingIdx - totIdx])) ? (inOpen[BodyShortTrailingIdx - totIdx]) : (inClose[BodyShortTrailingIdx - totIdx])) - inLow[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0))));
         i += 1;
         BodyShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLRISEFALL3METHODS_Impl( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLRISEFALL3METHODS_Lookback();
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
         BodyPeriodTotal[3] = BodyPeriodTotal[3] + ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 3] - ((((double)inClose[i - 3]) >= ((double)inOpen[i - 3])) ? ((double)inClose[i - 3]) : ((double)inOpen[i - 3]))) + (((((double)inClose[i - 3]) >= ((double)inOpen[i - 3])) ? ((double)inOpen[i - 3]) : ((double)inClose[i - 3])) - (double)inLow[i - 3])) : 0.0)));
         BodyPeriodTotal[2] = BodyPeriodTotal[2] + ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
         BodyPeriodTotal[1] = BodyPeriodTotal[1] + ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - ((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inClose[i - 4]) : ((double)inOpen[i - 4]))) + (((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inOpen[i - 4]) : ((double)inClose[i - 4])) - (double)inLow[i - 4])) : 0.0)));
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 0 - (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) && (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) && Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) < (double)inHigh[i - 4] && Math.max((double)inOpen[i - 3], (double)inClose[i - 3]) > (double)inLow[i - 4] && Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) < (double)inHigh[i - 4] && Math.max((double)inOpen[i - 2], (double)inClose[i - 2]) > (double)inLow[i - 4] && Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) < (double)inHigh[i - 4] && Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) > (double)inLow[i - 4] && (double)inClose[i - 2] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) < (double)inClose[i - 3] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && (double)inClose[i - 1] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) < (double)inClose[i - 2] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && (double)inOpen[i] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) > (double)inClose[i - 1] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && (double)inClose[i] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) > (double)inClose[i - 4] * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) && Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - ((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inClose[i - 4]) : ((double)inOpen[i - 4]))) + (((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inOpen[i - 4]) : ((double)inClose[i - 4])) - (double)inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 3] - ((((double)inClose[i - 3]) >= ((double)inOpen[i - 3])) ? ((double)inClose[i - 3]) : ((double)inOpen[i - 3]))) + (((((double)inClose[i - 3]) >= ((double)inOpen[i - 3])) ? ((double)inOpen[i - 3]) : ((double)inClose[i - 3])) - (double)inLow[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100 * (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1);
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - ((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inClose[i - 4]) : ((double)inOpen[i - 4]))) + (((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inOpen[i - 4]) : ((double)inClose[i - 4])) - (double)inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 4] - ((((double)inClose[BodyLongTrailingIdx - 4]) >= ((double)inOpen[BodyLongTrailingIdx - 4])) ? ((double)inClose[BodyLongTrailingIdx - 4]) : ((double)inOpen[BodyLongTrailingIdx - 4]))) + (((((double)inClose[BodyLongTrailingIdx - 4]) >= ((double)inOpen[BodyLongTrailingIdx - 4])) ? ((double)inOpen[BodyLongTrailingIdx - 4]) : ((double)inClose[BodyLongTrailingIdx - 4])) - (double)inLow[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - ((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inClose[i - totIdx]) : ((double)inOpen[i - totIdx]))) + (((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inOpen[i - totIdx]) : ((double)inClose[i - totIdx])) - (double)inLow[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx - totIdx] - (double)inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx - totIdx] - (double)inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx - totIdx] - ((((double)inClose[BodyShortTrailingIdx - totIdx]) >= ((double)inOpen[BodyShortTrailingIdx - totIdx])) ? ((double)inClose[BodyShortTrailingIdx - totIdx]) : ((double)inOpen[BodyShortTrailingIdx - totIdx]))) + (((((double)inClose[BodyShortTrailingIdx - totIdx]) >= ((double)inOpen[BodyShortTrailingIdx - totIdx])) ? ((double)inOpen[BodyShortTrailingIdx - totIdx]) : ((double)inClose[BodyShortTrailingIdx - totIdx])) - (double)inLow[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx] - ((((double)inClose[BodyLongTrailingIdx]) >= ((double)inOpen[BodyLongTrailingIdx])) ? ((double)inClose[BodyLongTrailingIdx]) : ((double)inOpen[BodyLongTrailingIdx]))) + (((((double)inClose[BodyLongTrailingIdx]) >= ((double)inOpen[BodyLongTrailingIdx])) ? ((double)inOpen[BodyLongTrailingIdx]) : ((double)inClose[BodyLongTrailingIdx])) - (double)inLow[BodyLongTrailingIdx])) : 0.0))));
         i += 1;
         BodyShortTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A five-candle continuation pattern: a long candle, three small
    * counter-color candles that stay partly within the first candle's high-low
    * range, then a long same-color candle that resumes the trend. Bullish
    * (rising) or bearish (falling) continuation signal.
    * <p><b>Notes</b>
    * <ul>
    * <li>Only the three-small-candle variant is detected; the classic pattern allowing two or more small candles is not supported.</li>
    * <li>The middle candles need only partially overlap the first candle's range, not be fully contained within it.</li>
    * <li>The prior trend the continuation reading assumes is not verified.</li>
    * <li>Bulkowski's testing found Rising Three Methods continues 74% of the time (102 examples out of 4.7M candle lines) and Falling Three Methods continues 71% of the time (just 64 examples) — both act as classically labeled, but Bulkowski flags the samples as too thin to trust: Falling Three Methods is so rare he omitted its statistics from his book entirely. ([thepatternsite.com](https://thepatternsite.com/Rising3Methods.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLRISEFALL3METHODS_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when candle 1 is white (rising/bullish
    *        continuation), -100 when candle 1 is black (falling/bearish continuation),
    *        0 otherwise. Sign = 100 * color of candle 1. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#CDLXSIDEGAP3METHODS
    * @see Core#CDL3INSIDE
    * @see Core#CDL3OUTSIDE
    */
   public OutRange CDLRISEFALL3METHODS( int startIdx,
                                        int endIdx,
                                        double inOpen[],
                                        double inHigh[],
                                        double inLow[],
                                        double inClose[],
                                        int outInteger[] )
   {
      requireIndexRange("CDLRISEFALL3METHODS", startIdx, endIdx);
      int guardStart = clampedStart("CDLRISEFALL3METHODS", startIdx, CDLRISEFALL3METHODS_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLRISEFALL3METHODS", "inOpen", inOpen, guardInLen);
      requireLength("CDLRISEFALL3METHODS", "inHigh", inHigh, guardInLen);
      requireLength("CDLRISEFALL3METHODS", "inLow", inLow, guardInLen);
      requireLength("CDLRISEFALL3METHODS", "inClose", inClose, guardInLen);
      requireLength("CDLRISEFALL3METHODS", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLRISEFALL3METHODS_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLRISEFALL3METHODS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A five-candle continuation pattern: a long candle, three small
    * counter-color candles that stay partly within the first candle's high-low
    * range, then a long same-color candle that resumes the trend. Bullish
    * (rising) or bearish (falling) continuation signal.
    * <p><b>Notes</b>
    * <ul>
    * <li>Only the three-small-candle variant is detected; the classic pattern allowing two or more small candles is not supported.</li>
    * <li>The middle candles need only partially overlap the first candle's range, not be fully contained within it.</li>
    * <li>The prior trend the continuation reading assumes is not verified.</li>
    * <li>Bulkowski's testing found Rising Three Methods continues 74% of the time (102 examples out of 4.7M candle lines) and Falling Three Methods continues 71% of the time (just 64 examples) — both act as classically labeled, but Bulkowski flags the samples as too thin to trust: Falling Three Methods is so rare he omitted its statistics from his book entirely. ([thepatternsite.com](https://thepatternsite.com/Rising3Methods.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLRISEFALL3METHODS_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when candle 1 is white (rising/bullish
    *        continuation), -100 when candle 1 is black (falling/bearish continuation),
    *        0 otherwise. Sign = 100 * color of candle 1. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#CDLXSIDEGAP3METHODS
    * @see Core#CDL3INSIDE
    * @see Core#CDL3OUTSIDE
    */
   public OutRange CDLRISEFALL3METHODS( int startIdx,
                                        int endIdx,
                                        float inOpen[],
                                        float inHigh[],
                                        float inLow[],
                                        float inClose[],
                                        int outInteger[] )
   {
      requireIndexRange("CDLRISEFALL3METHODS", startIdx, endIdx);
      int guardStart = clampedStart("CDLRISEFALL3METHODS", startIdx, CDLRISEFALL3METHODS_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLRISEFALL3METHODS", "inOpen", inOpen, guardInLen);
      requireLength("CDLRISEFALL3METHODS", "inHigh", inHigh, guardInLen);
      requireLength("CDLRISEFALL3METHODS", "inLow", inLow, guardInLen);
      requireLength("CDLRISEFALL3METHODS", "inClose", inClose, guardInLen);
      requireLength("CDLRISEFALL3METHODS", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLRISEFALL3METHODS_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLRISEFALL3METHODS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLRISEFALL3METHODS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLRISEFALL3METHODS} over the same series.
    * Open with {@link Core#cdlrisefall3methodsOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class Cdlrisefall3methodsStream {
      Core core;
      double[] BodyPeriodTotal;
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
      double[] ring_BodyLongTrailingIdx_derived;
      int ringPos_BodyShortTrailingIdx;
      int ringCap_BodyShortTrailingIdx;
      int ringLag_BodyShortTrailingIdx;
      double[] ring_BodyShortTrailingIdx_derived;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cs_BodyShort_rangeType;
      int cs_BodyShort_avgPeriod;
      double cs_BodyShort_factor;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      Cdlrisefall3methodsStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLRISEFALL3METHODS} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      Cdlrisefall3methodsStream( Cdlrisefall3methodsStream other ) {
         this.core = other.core;
         this.BodyPeriodTotal = other.BodyPeriodTotal.clone();
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
         this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         this.ringPos_BodyShortTrailingIdx = other.ringPos_BodyShortTrailingIdx;
         this.ringCap_BodyShortTrailingIdx = other.ringCap_BodyShortTrailingIdx;
         this.ringLag_BodyShortTrailingIdx = other.ringLag_BodyShortTrailingIdx;
         this.ring_BodyShortTrailingIdx_derived = other.ring_BodyShortTrailingIdx_derived.clone();
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( Cdlrisefall3methodsStream other ) {
         this.core = other.core;
         if( this.BodyPeriodTotal != null && this.BodyPeriodTotal.length == other.BodyPeriodTotal.length ) {
            System.arraycopy( other.BodyPeriodTotal, 0, this.BodyPeriodTotal, 0, other.BodyPeriodTotal.length );
         } else {
            this.BodyPeriodTotal = other.BodyPeriodTotal.clone();
         }
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
         if( this.ring_BodyLongTrailingIdx_derived != null && this.ring_BodyLongTrailingIdx_derived.length == other.ring_BodyLongTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_derived, 0, this.ring_BodyLongTrailingIdx_derived, 0, other.ring_BodyLongTrailingIdx_derived.length );
         } else {
            this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         }
         this.ringPos_BodyShortTrailingIdx = other.ringPos_BodyShortTrailingIdx;
         this.ringCap_BodyShortTrailingIdx = other.ringCap_BodyShortTrailingIdx;
         this.ringLag_BodyShortTrailingIdx = other.ringLag_BodyShortTrailingIdx;
         if( this.ring_BodyShortTrailingIdx_derived != null && this.ring_BodyShortTrailingIdx_derived.length == other.ring_BodyShortTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_BodyShortTrailingIdx_derived, 0, this.ring_BodyShortTrailingIdx_derived, 0, other.ring_BodyShortTrailingIdx_derived.length );
         } else {
            this.ring_BodyShortTrailingIdx_derived = other.ring_BodyShortTrailingIdx_derived.clone();
         }
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<Cdlrisefall3methodsStream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the handle is left exactly as it was —
       * the stream stays usable, so skip the bar or re-open on a clean
       * history. This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDLRISEFALL3METHODS update: BadParam", RetCode.BadParam);
         core.cdlrisefall3methodsStepImpl(this, inOpen, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outInteger;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inOpen.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] ) {
         requireArgument("CDLRISEFALL3METHODS updateAndFill", "inOpen", inOpen);
         requireArgument("CDLRISEFALL3METHODS updateAndFill", "inHigh", inHigh);
         requireArgument("CDLRISEFALL3METHODS updateAndFill", "inLow", inLow);
         requireArgument("CDLRISEFALL3METHODS updateAndFill", "inClose", inClose);
         requireArgument("CDLRISEFALL3METHODS updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLRISEFALL3METHODS updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("CDLRISEFALL3METHODS updateAndFill: BadParam", RetCode.BadParam);
            core.cdlrisefall3methodsStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
            outInteger[i] = this.cur_outInteger;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a scratch handle held per thread and
       * reused, so the copy allocates nothing after the first peek of this
       * indicator on this thread. That scratch is retained for the life of
       * the thread.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDLRISEFALL3METHODS peek: BadParam", RetCode.BadParam);
         Cdlrisefall3methodsStream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new Cdlrisefall3methodsStream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.cdlrisefall3methodsStepImpl(scratch, inOpen, inHigh, inLow, inClose);
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
      public Cdlrisefall3methodsStream copy() {
         return new Cdlrisefall3methodsStream(this);
      }
   }
   void cdlrisefall3methodsStepImpl( Cdlrisefall3methodsStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int totIdx = 0;
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      int BodyShort_rangeType = sp.cs_BodyShort_rangeType;
      int BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
      double BodyShort_factor = sp.cs_BodyShort_factor;
      sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ring_BodyShortTrailingIdx_derived[sp.ringPos_BodyShortTrailingIdx] = ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
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
          Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
          Math.abs(sp.lag3_inClose - sp.lag3_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((BodyShort_rangeType == 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((BodyShort_rangeType == 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          Math.abs(sp.lag2_inClose - sp.lag2_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyShort_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyShort_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyShort_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyShort_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) )
      {
         sp.cur_outInteger = 100 * ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1);
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyPeriodTotal[4] = sp.BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx]);
      for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
         sp.BodyPeriodTotal[totIdx] = sp.BodyPeriodTotal[totIdx] + (sp.ring_BodyShortTrailingIdx_derived[(sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - totIdx >= sp.ringCap_BodyShortTrailingIdx) ? sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - totIdx - sp.ringCap_BodyShortTrailingIdx : sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - totIdx] - sp.ring_BodyShortTrailingIdx_derived[(sp.ringPos_BodyShortTrailingIdx + sp.ringCap_BodyShortTrailingIdx - sp.ringLag_BodyShortTrailingIdx - totIdx) % sp.ringCap_BodyShortTrailingIdx]);
      }
      sp.BodyPeriodTotal[0] = sp.BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx) % sp.ringCap_BodyLongTrailingIdx]);
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
      sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
      if( sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx ) {
         sp.ringPos_BodyLongTrailingIdx = 0;
      }
      sp.ringPos_BodyShortTrailingIdx = sp.ringPos_BodyShortTrailingIdx + 1;
      if( sp.ringPos_BodyShortTrailingIdx >= sp.ringCap_BodyShortTrailingIdx ) {
         sp.ringPos_BodyShortTrailingIdx = 0;
      }
   }
   private RetCode cdlrisefall3methodsOpenImpl( Cdlrisefall3methodsStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
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
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
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
      lookbackTotal = CDLRISEFALL3METHODS_Lookback();
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
         return RetCode.InsufficientHistory ;
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
         BodyPeriodTotal[3] = BodyPeriodTotal[3] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
         BodyPeriodTotal[2] = BodyPeriodTotal[2] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         BodyPeriodTotal[1] = BodyPeriodTotal[1] + ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
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
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[4] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long, then 3 small, 5th long */
             Math.abs(inClose[i - 3] - inOpen[i - 3]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[3] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[2] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 1] - inOpen[i - 1]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal[1] / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++ * outStride] = 100 * ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1);
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal[4] = BodyPeriodTotal[4] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0))));
         for( totIdx = 3; totIdx >= 1; totIdx -= 1 ) {
            BodyPeriodTotal[totIdx] = BodyPeriodTotal[totIdx] + (((BodyShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx - totIdx] - inOpen[BodyShortTrailingIdx - totIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx - totIdx] - inLow[BodyShortTrailingIdx - totIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx - totIdx] - (((inClose[BodyShortTrailingIdx - totIdx]) >= (inOpen[BodyShortTrailingIdx - totIdx])) ? (inClose[BodyShortTrailingIdx - totIdx]) : (inOpen[BodyShortTrailingIdx - totIdx]))) + ((((inClose[BodyShortTrailingIdx - totIdx]) >= (inOpen[BodyShortTrailingIdx - totIdx])) ? (inOpen[BodyShortTrailingIdx - totIdx]) : (inClose[BodyShortTrailingIdx - totIdx])) - inLow[BodyShortTrailingIdx - totIdx])) : 0.0))));
         }
         BodyPeriodTotal[0] = BodyPeriodTotal[0] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - (((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inClose[BodyLongTrailingIdx]) : (inOpen[BodyLongTrailingIdx]))) + ((((inClose[BodyLongTrailingIdx]) >= (inOpen[BodyLongTrailingIdx])) ? (inOpen[BodyLongTrailingIdx]) : (inClose[BodyLongTrailingIdx])) - inLow[BodyLongTrailingIdx])) : 0.0))));
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
      double[] capRing_BodyLongTrailingIdx_derived = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int capLag_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
      int cap_BodyShortTrailingIdx = capLag_BodyShortTrailingIdx + 4;
      if( capLag_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyShortTrailingIdx = (cap_BodyShortTrailingIdx > 0)? cap_BodyShortTrailingIdx : 1;
      double[] capRing_BodyShortTrailingIdx_derived = new double[allocN_BodyShortTrailingIdx];
      for( int fillJ = historyLen - cap_BodyShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyShortTrailingIdx_derived[fillJ % cap_BodyShortTrailingIdx] = ((BodyShort_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.BodyPeriodTotal = BodyPeriodTotal;
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
      sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
      sp.ringPos_BodyShortTrailingIdx = historyLen % cap_BodyShortTrailingIdx;
      sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
      sp.ringLag_BodyShortTrailingIdx = capLag_BodyShortTrailingIdx;
      sp.ring_BodyShortTrailingIdx_derived = capRing_BodyShortTrailingIdx_derived;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cdlrisefall3methodsOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   Cdlrisefall3methodsStream cdlrisefall3methodsOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      Cdlrisefall3methodsStream sp = new Cdlrisefall3methodsStream(this);
      RetCode retCode = cdlrisefall3methodsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLRISEFALL3METHODS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLRISEFALL3METHODS openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLRISEFALL3METHODS openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cdlrisefall3methodsOpen (composition seam). */
   Cdlrisefall3methodsStream cdlrisefall3methodsOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      Cdlrisefall3methodsStream sp = new Cdlrisefall3methodsStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdlrisefall3methodsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLRISEFALL3METHODS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLRISEFALL3METHODS open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLRISEFALL3METHODS open: " + retCode, retCode);
   }
   /**
    * Open a live CDLRISEFALL3METHODS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLRISEFALL3METHODS} at that bar.
    * <p>The history must hold at least {@code CDLRISEFALL3METHODS_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public Cdlrisefall3methodsStream cdlrisefall3methodsOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLRISEFALL3METHODS open", "inOpen", inOpen);
      requireHistory("CDLRISEFALL3METHODS open", inOpen.length);
      requireArgument("CDLRISEFALL3METHODS open", "inHigh", inHigh);
      requireArgument("CDLRISEFALL3METHODS open", "inLow", inLow);
      requireArgument("CDLRISEFALL3METHODS open", "inClose", inClose);
      requireHistoryLength("CDLRISEFALL3METHODS open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLRISEFALL3METHODS open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLRISEFALL3METHODS open", "inClose", inClose.length, inOpen.length);
      return cdlrisefall3methodsOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlrisefall3methodsOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CDLRISEFALL3METHODS} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link Cdlrisefall3methodsStream#outRange()}.
    */
   public Cdlrisefall3methodsStream cdlrisefall3methodsOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLRISEFALL3METHODS openAndFill", "inOpen", inOpen);
      requireHistory("CDLRISEFALL3METHODS openAndFill", inOpen.length);
      requireArgument("CDLRISEFALL3METHODS openAndFill", "inHigh", inHigh);
      requireArgument("CDLRISEFALL3METHODS openAndFill", "inLow", inLow);
      requireArgument("CDLRISEFALL3METHODS openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLRISEFALL3METHODS openAndFill", inOpen.length, CDLRISEFALL3METHODS_Lookback());
      requireHistoryLength("CDLRISEFALL3METHODS openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLRISEFALL3METHODS openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLRISEFALL3METHODS openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLRISEFALL3METHODS openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLRISEFALL3METHODS openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdlrisefall3methodsOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
