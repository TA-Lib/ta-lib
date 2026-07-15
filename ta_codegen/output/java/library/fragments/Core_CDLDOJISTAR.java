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
 *  100204 AC   Creation
 */

   public int cdlDojiStarLookback( )
   {
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      return Math.max(BodyDoji_avgPeriod, BodyLong_avgPeriod) + 1 ;

   }
   public RetCode cdlDojiStar( int startIdx,
                               int endIdx,
                               double inOpen[],
                               double inHigh[],
                               double inLow[],
                               double inClose[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
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
      lookbackTotal = cdlDojiStarLookback();
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
      BodyDojiPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long real body
       * - second candle: star (open gapping up in an uptrend or down in a downtrend) with a doji
       * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * it's defined bullish when the long candle is white and the star gaps up, bearish when the long candle
       * is black and the star gaps down; the user should consider that a doji star is bullish when it appears
       * in an uptrend and it's bearish when it appears in a downtrend, so to determine the bullishness or
       * bearishness of the pattern the trend must be analyzed
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
             Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
             (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i], inClose[i]) > Math.max(inOpen[i - 1], inClose[i - 1])) || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i], inClose[i]) < Math.min(inOpen[i - 1], inClose[i - 1]))) ) /* that gaps up if 1st is white or down if 1st is black */
         {
            outInteger[outIdx++] = (0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1)) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlDojiStarUnguarded( int startIdx,
                                        int endIdx,
                                        double inOpen[],
                                        double inHigh[],
                                        double inLow[],
                                        double inClose[],
                                        MInteger outBegIdx,
                                        MInteger outNBElement,
                                        int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      lookbackTotal = cdlDojiStarLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal = 0;
      BodyDojiPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i], inClose[i]) > Math.max(inOpen[i - 1], inClose[i - 1])) || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i], inClose[i]) < Math.min(inOpen[i - 1], inClose[i - 1]))) ) {
            outInteger[outIdx++] = (0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1)) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlDojiStar( int startIdx,
                               int endIdx,
                               float inOpen[],
                               float inHigh[],
                               float inLow[],
                               float inClose[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlDojiStarLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal = 0;
      BodyDojiPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i], (double)inClose[i]) > Math.max((double)inOpen[i - 1], (double)inClose[i - 1])) || (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (Math.max((double)inOpen[i], (double)inClose[i]) < Math.min((double)inOpen[i - 1], (double)inClose[i - 1]))) ) {
            outInteger[outIdx++] = (0 - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1)) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) - Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) - Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlDojiStarUnguarded( int startIdx,
                                        int endIdx,
                                        float inOpen[],
                                        float inHigh[],
                                        float inLow[],
                                        float inClose[],
                                        MInteger outBegIdx,
                                        MInteger outNBElement,
                                        int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      lookbackTotal = cdlDojiStarLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal = 0;
      BodyDojiPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i], (double)inClose[i]) > Math.max((double)inOpen[i - 1], (double)inClose[i - 1])) || (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (Math.max((double)inOpen[i], (double)inClose[i]) < Math.min((double)inOpen[i - 1], (double)inClose[i - 1]))) ) {
            outInteger[outIdx++] = (0 - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1)) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) - Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) - Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
