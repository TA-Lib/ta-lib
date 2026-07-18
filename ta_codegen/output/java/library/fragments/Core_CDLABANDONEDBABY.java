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
 *  102304 AC   Creation
 */

   public int cdlAbandonedBabyLookback( double optInPenetration )
   {
      if( optInPenetration == -4e37 ) {
         optInPenetration = 3e-1;
      } else if( optInPenetration < 0e0 || optInPenetration > 1.7976931348623157e308 ) {
         return -1;
      }
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      return Math.max(Math.max(BodyDoji_avgPeriod, BodyLong_avgPeriod), BodyShort_avgPeriod) + 2 ;

   }
   public RetCode cdlAbandonedBaby( int startIdx,
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
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      double BodyShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
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
         optInPenetration = 3e-1;
      } else if( optInPenetration < 0e0 || optInPenetration > 1.7976931348623157e308 ) {
         return RetCode.BadParam;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlAbandonedBabyLookback(optInPenetration);
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
      BodyShortPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - 1 - BodyDoji_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long white (black) real body
       * - second candle: doji
       * - third candle: black (white) real body that moves well within the first candle's real body
       * - upside (downside) gap between the first candle and the doji (the shadows of the two candles don't touch)
       * - downside (upside) gap between the doji and the third candle (the shadows of the two candles don't touch)
       * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
       * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
       * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
       * it to be relatively long
       * outInteger is positive (1 to 100) when it's an abandoned baby bottom or negative (-1 to -100) when it's
       * an abandoned baby top; the user should consider that an abandoned baby is significant when it appears in
       * an uptrend or downtrend, while this function does not consider the trend
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
             Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
             (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && inClose[i] < inClose[i - 2] - Math.abs(inClose[i - 2] - inOpen[i - 2]) * optInPenetration && (inLow[i - 1] > inHigh[i - 2]) && (inHigh[i] < inLow[i - 1]) || ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inClose[i] > Math.fma(Math.abs(inClose[i - 2] - inOpen[i - 2]), optInPenetration, inClose[i - 2]) && (inHigh[i - 1] < inLow[i - 2]) && (inLow[i] > inHigh[i - 1])) ) /* 1st white 3rd black 3rd closes well within 1st rb upside gap between 1st and 2nd downside gap between 2nd and 3rd 1st black 3rd white 3rd closes well within 1st rb downside gap between 1st and 2nd upside gap between 2nd and 3rd */
         {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) - Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlAbandonedBabyUnguarded( int startIdx,
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
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      double BodyShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      lookbackTotal = cdlAbandonedBabyLookback(optInPenetration);
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
      BodyShortPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - 1 - BodyDoji_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && inClose[i] < inClose[i - 2] - Math.abs(inClose[i - 2] - inOpen[i - 2]) * optInPenetration && (inLow[i - 1] > inHigh[i - 2]) && (inHigh[i] < inLow[i - 1]) || ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inClose[i] > Math.fma(Math.abs(inClose[i - 2] - inOpen[i - 2]), optInPenetration, inClose[i - 2]) && (inHigh[i - 1] < inLow[i - 2]) && (inLow[i] > inHigh[i - 1])) ) {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) - Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlAbandonedBaby( int startIdx,
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
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      double BodyShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
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
         optInPenetration = 3e-1;
      } else if( optInPenetration < 0e0 || optInPenetration > 1.7976931348623157e308 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = cdlAbandonedBabyLookback(optInPenetration);
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
      BodyShortPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - 1 - BodyDoji_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (double)inClose[i] < (double)inClose[i - 2] - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) * optInPenetration && ((double)inLow[i - 1] > (double)inHigh[i - 2]) && ((double)inHigh[i] < (double)inLow[i - 1]) || (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inClose[i] > Math.fma(Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]), optInPenetration, (double)inClose[i - 2]) && ((double)inHigh[i - 1] < (double)inLow[i - 2]) && ((double)inLow[i] > (double)inHigh[i - 1])) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) - Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) - Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) - Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlAbandonedBabyUnguarded( int startIdx,
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
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      double BodyShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      lookbackTotal = cdlAbandonedBabyLookback(optInPenetration);
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
      BodyShortPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - 1 - BodyDoji_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (double)inClose[i] < (double)inClose[i - 2] - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) * optInPenetration && ((double)inLow[i - 1] > (double)inHigh[i - 2]) && ((double)inHigh[i] < (double)inLow[i - 1]) || (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inClose[i] > Math.fma(Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]), optInPenetration, (double)inClose[i - 2]) && ((double)inHigh[i - 1] < (double)inLow[i - 2]) && ((double)inLow[i] > (double)inHigh[i - 1])) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) - Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) - Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) - Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CDLABANDONEDBABY stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlAbandonedBaby} over the same series.
    * Open with {@link Core#cdlAbandonedBabyOpen}; there is no close — the handle is
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
   public static final class CdlAbandonedBabyStream {
      final Core core;
      double optInPenetration;
      double BodyDojiPeriodTotal;
      double BodyLongPeriodTotal;
      double BodyShortPeriodTotal;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag1_inClose;
      double lag2_inClose;
      int ringPos_BodyDojiTrailingIdx;
      int ringCap_BodyDojiTrailingIdx;
      double[] ring_BodyDojiTrailingIdx_inOpen;
      double[] ring_BodyDojiTrailingIdx_inHigh;
      double[] ring_BodyDojiTrailingIdx_inLow;
      double[] ring_BodyDojiTrailingIdx_inClose;
      int ringPos_BodyLongTrailingIdx;
      int ringCap_BodyLongTrailingIdx;
      double[] ring_BodyLongTrailingIdx_inOpen;
      double[] ring_BodyLongTrailingIdx_inHigh;
      double[] ring_BodyLongTrailingIdx_inLow;
      double[] ring_BodyLongTrailingIdx_inClose;
      int ringPos_BodyShortTrailingIdx;
      int ringCap_BodyShortTrailingIdx;
      double[] ring_BodyShortTrailingIdx_inOpen;
      double[] ring_BodyShortTrailingIdx_inHigh;
      double[] ring_BodyShortTrailingIdx_inLow;
      double[] ring_BodyShortTrailingIdx_inClose;
      int cs_BodyDoji_rangeType;
      int cs_BodyDoji_avgPeriod;
      double cs_BodyDoji_factor;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cs_BodyShort_rangeType;
      int cs_BodyShort_avgPeriod;
      double cs_BodyShort_factor;
      int cur_outInteger;

      CdlAbandonedBabyStream( Core core ) { this.core = core; }

      CdlAbandonedBabyStream( CdlAbandonedBabyStream other ) {
         this.core = other.core;
         this.optInPenetration = other.optInPenetration;
         this.BodyDojiPeriodTotal = other.BodyDojiPeriodTotal;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.BodyShortPeriodTotal = other.BodyShortPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.ringPos_BodyDojiTrailingIdx = other.ringPos_BodyDojiTrailingIdx;
         this.ringCap_BodyDojiTrailingIdx = other.ringCap_BodyDojiTrailingIdx;
         this.ring_BodyDojiTrailingIdx_inOpen = other.ring_BodyDojiTrailingIdx_inOpen.clone();
         this.ring_BodyDojiTrailingIdx_inHigh = other.ring_BodyDojiTrailingIdx_inHigh.clone();
         this.ring_BodyDojiTrailingIdx_inLow = other.ring_BodyDojiTrailingIdx_inLow.clone();
         this.ring_BodyDojiTrailingIdx_inClose = other.ring_BodyDojiTrailingIdx_inClose.clone();
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ring_BodyLongTrailingIdx_inOpen = other.ring_BodyLongTrailingIdx_inOpen.clone();
         this.ring_BodyLongTrailingIdx_inHigh = other.ring_BodyLongTrailingIdx_inHigh.clone();
         this.ring_BodyLongTrailingIdx_inLow = other.ring_BodyLongTrailingIdx_inLow.clone();
         this.ring_BodyLongTrailingIdx_inClose = other.ring_BodyLongTrailingIdx_inClose.clone();
         this.ringPos_BodyShortTrailingIdx = other.ringPos_BodyShortTrailingIdx;
         this.ringCap_BodyShortTrailingIdx = other.ringCap_BodyShortTrailingIdx;
         this.ring_BodyShortTrailingIdx_inOpen = other.ring_BodyShortTrailingIdx_inOpen.clone();
         this.ring_BodyShortTrailingIdx_inHigh = other.ring_BodyShortTrailingIdx_inHigh.clone();
         this.ring_BodyShortTrailingIdx_inLow = other.ring_BodyShortTrailingIdx_inLow.clone();
         this.ring_BodyShortTrailingIdx_inClose = other.ring_BodyShortTrailingIdx_inClose.clone();
         this.cs_BodyDoji_rangeType = other.cs_BodyDoji_rangeType;
         this.cs_BodyDoji_avgPeriod = other.cs_BodyDoji_avgPeriod;
         this.cs_BodyDoji_factor = other.cs_BodyDoji_factor;
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
         core.cdlAbandonedBabyStreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CdlAbandonedBabyStream scratch = new CdlAbandonedBabyStream(this);
         core.cdlAbandonedBabyStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CdlAbandonedBabyStream copy() {
         return new CdlAbandonedBabyStream(this);
      }
   }
   void cdlAbandonedBabyStreamStep( CdlAbandonedBabyStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
      int BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
      double BodyDoji_factor = sp.cs_BodyDoji_factor;
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      int BodyShort_rangeType = sp.cs_BodyShort_rangeType;
      int BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
      double BodyShort_factor = sp.cs_BodyShort_factor;
      if( sp.ringCap_BodyDojiTrailingIdx == 0 ) {
         sp.ring_BodyDojiTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyDojiTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyDojiTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyDojiTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_BodyLongTrailingIdx == 0 ) {
         sp.ring_BodyLongTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyLongTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyLongTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyLongTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_BodyShortTrailingIdx == 0 ) {
         sp.ring_BodyShortTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyShortTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyShortTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyShortTrailingIdx_inClose[0] = inClose;
      }
      if( Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long */
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyDoji_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyDoji_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
          Math.abs(inClose - inOpen) > ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
          (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 1 && ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 && inClose < sp.lag2_inClose - Math.abs(sp.lag2_inClose - sp.lag2_inOpen) * sp.optInPenetration && (sp.lag1_inLow > sp.lag2_inHigh) && (inHigh < sp.lag1_inLow) || ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 0 - 1 && ((inClose >= inOpen) ? 1 : 0 - 1) == 1 && inClose > Math.fma(Math.abs(sp.lag2_inClose - sp.lag2_inOpen), sp.optInPenetration, sp.lag2_inClose) && (sp.lag1_inHigh < sp.lag2_inLow) && (inLow > sp.lag1_inHigh)) ) /* 1st white 3rd black 3rd closes well within 1st rb upside gap between 1st and 2nd downside gap between 2nd and 3rd 1st black 3rd white 3rd closes well within 1st rb downside gap between 1st and 2nd upside gap between 2nd and 3rd */
      {
         sp.cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx]) - Math.abs(sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx])) : 0.0)));
      sp.BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyDoji_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyDoji_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx]) - Math.abs(sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx])) : 0.0)));
      sp.BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(sp.ring_BodyShortTrailingIdx_inClose[sp.ringPos_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inOpen[sp.ringPos_BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (sp.ring_BodyShortTrailingIdx_inHigh[sp.ringPos_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inLow[sp.ringPos_BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((sp.ring_BodyShortTrailingIdx_inHigh[sp.ringPos_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inLow[sp.ringPos_BodyShortTrailingIdx]) - Math.abs(sp.ring_BodyShortTrailingIdx_inClose[sp.ringPos_BodyShortTrailingIdx] - sp.ring_BodyShortTrailingIdx_inOpen[sp.ringPos_BodyShortTrailingIdx])) : 0.0)));
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
      sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx] = inOpen;
      sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] = inHigh;
      sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx] = inLow;
      sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] = inClose;
      sp.ringPos_BodyDojiTrailingIdx = sp.ringPos_BodyDojiTrailingIdx + 1;
      if( sp.ringPos_BodyDojiTrailingIdx >= sp.ringCap_BodyDojiTrailingIdx ) {
         sp.ringPos_BodyDojiTrailingIdx = 0;
      }
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
   }
   private RetCode cdlAbandonedBabyOpenBody( CdlAbandonedBabyStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, double optInPenetration )
   {
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      double BodyShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int lastValue_outInteger = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( optInPenetration == -4e37 ) {
         optInPenetration = 3e-1;
      } else if( optInPenetration < 0e0 || optInPenetration > 1.7976931348623157e308 ) {
         return RetCode.BadParam;
      }
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlAbandonedBabyLookback(optInPenetration);
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
      BodyDojiPeriodTotal = 0;
      BodyShortPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - 1 - BodyDoji_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long white (black) real body
       * - second candle: doji
       * - third candle: black (white) real body that moves well within the first candle's real body
       * - upside (downside) gap between the first candle and the doji (the shadows of the two candles don't touch)
       * - downside (upside) gap between the doji and the third candle (the shadows of the two candles don't touch)
       * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
       * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
       * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
       * it to be relatively long
       * outInteger is positive (1 to 100) when it's an abandoned baby bottom or negative (-1 to -100) when it's
       * an abandoned baby top; the user should consider that an abandoned baby is significant when it appears in
       * an uptrend or downtrend, while this function does not consider the trend
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
             Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
             (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && inClose[i] < inClose[i - 2] - Math.abs(inClose[i - 2] - inOpen[i - 2]) * optInPenetration && (inLow[i - 1] > inHigh[i - 2]) && (inHigh[i] < inLow[i - 1]) || ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inClose[i] > Math.fma(Math.abs(inClose[i - 2] - inOpen[i - 2]), optInPenetration, inClose[i - 2]) && (inHigh[i - 1] < inLow[i - 2]) && (inLow[i] > inHigh[i - 1])) ) /* 1st white 3rd black 3rd closes well within 1st rb upside gap between 1st and 2nd downside gap between 2nd and 3rd 1st black 3rd white 3rd closes well within 1st rb downside gap between 1st and 2nd upside gap between 2nd and 3rd */
         {
            lastValue_outInteger = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            lastValue_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) - Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_BodyDojiTrailingIdx = i - BodyDojiTrailingIdx;
      if( cap_BodyDojiTrailingIdx < 0 || cap_BodyDojiTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyDojiTrailingIdx = (cap_BodyDojiTrailingIdx > 0)? cap_BodyDojiTrailingIdx : 1;
      double[] capRing_BodyDojiTrailingIdx_inOpen = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inOpen, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inHigh = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inHigh, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inLow = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inLow, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inClose = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inClose, 0, cap_BodyDojiTrailingIdx);
      int cap_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      if( cap_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyLongTrailingIdx = (cap_BodyLongTrailingIdx > 0)? cap_BodyLongTrailingIdx : 1;
      double[] capRing_BodyLongTrailingIdx_inOpen = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inOpen, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inHigh = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inHigh, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inLow = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inLow, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inClose = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inClose, 0, cap_BodyLongTrailingIdx);
      int cap_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
      if( cap_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyShortTrailingIdx = (cap_BodyShortTrailingIdx > 0)? cap_BodyShortTrailingIdx : 1;
      double[] capRing_BodyShortTrailingIdx_inOpen = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inOpen, 0, cap_BodyShortTrailingIdx);
      double[] capRing_BodyShortTrailingIdx_inHigh = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inHigh, 0, cap_BodyShortTrailingIdx);
      double[] capRing_BodyShortTrailingIdx_inLow = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inLow, 0, cap_BodyShortTrailingIdx);
      double[] capRing_BodyShortTrailingIdx_inClose = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inClose, 0, cap_BodyShortTrailingIdx);
      sp.optInPenetration = optInPenetration;
      sp.BodyDojiPeriodTotal = BodyDojiPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.BodyShortPeriodTotal = BodyShortPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_BodyDojiTrailingIdx = 0;
      sp.ringCap_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx;
      sp.ring_BodyDojiTrailingIdx_inOpen = capRing_BodyDojiTrailingIdx_inOpen;
      sp.ring_BodyDojiTrailingIdx_inHigh = capRing_BodyDojiTrailingIdx_inHigh;
      sp.ring_BodyDojiTrailingIdx_inLow = capRing_BodyDojiTrailingIdx_inLow;
      sp.ring_BodyDojiTrailingIdx_inClose = capRing_BodyDojiTrailingIdx_inClose;
      sp.ringPos_BodyLongTrailingIdx = 0;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_inOpen = capRing_BodyLongTrailingIdx_inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh = capRing_BodyLongTrailingIdx_inHigh;
      sp.ring_BodyLongTrailingIdx_inLow = capRing_BodyLongTrailingIdx_inLow;
      sp.ring_BodyLongTrailingIdx_inClose = capRing_BodyLongTrailingIdx_inClose;
      sp.ringPos_BodyShortTrailingIdx = 0;
      sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
      sp.ring_BodyShortTrailingIdx_inOpen = capRing_BodyShortTrailingIdx_inOpen;
      sp.ring_BodyShortTrailingIdx_inHigh = capRing_BodyShortTrailingIdx_inHigh;
      sp.ring_BodyShortTrailingIdx_inLow = capRing_BodyShortTrailingIdx_inLow;
      sp.ring_BodyShortTrailingIdx_inClose = capRing_BodyShortTrailingIdx_inClose;
      sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
      sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
      sp.cs_BodyDoji_factor = BodyDoji_factor;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlAbandonedBabyOpenAndFillBody( CdlAbandonedBabyStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], double optInPenetration, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      double BodyShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( optInPenetration == -4e37 ) {
         optInPenetration = 3e-1;
      } else if( optInPenetration < 0e0 || optInPenetration > 1.7976931348623157e308 ) {
         return RetCode.BadParam;
      }
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlAbandonedBabyLookback(optInPenetration);
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
      BodyDojiPeriodTotal = 0;
      BodyShortPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 2 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - 1 - BodyDoji_avgPeriod;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long white (black) real body
       * - second candle: doji
       * - third candle: black (white) real body that moves well within the first candle's real body
       * - upside (downside) gap between the first candle and the doji (the shadows of the two candles don't touch)
       * - downside (upside) gap between the doji and the third candle (the shadows of the two candles don't touch)
       * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
       * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
       * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
       * it to be relatively long
       * outInteger is positive (1 to 100) when it's an abandoned baby bottom or negative (-1 to -100) when it's
       * an abandoned baby top; the user should consider that an abandoned baby is significant when it appears in
       * an uptrend or downtrend, while this function does not consider the trend
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
             Math.abs(inClose[i] - inOpen[i]) > ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd: longer than short */
             (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && inClose[i] < inClose[i - 2] - Math.abs(inClose[i - 2] - inOpen[i - 2]) * optInPenetration && (inLow[i - 1] > inHigh[i - 2]) && (inHigh[i] < inLow[i - 1]) || ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inClose[i] > Math.fma(Math.abs(inClose[i - 2] - inOpen[i - 2]), optInPenetration, inClose[i - 2]) && (inHigh[i - 1] < inLow[i - 2]) && (inLow[i] > inHigh[i - 1])) ) /* 1st white 3rd black 3rd closes well within 1st rb upside gap between 1st and 2nd downside gap between 2nd and 3rd 1st black 3rd white 3rd closes well within 1st rb downside gap between 1st and 2nd upside gap between 2nd and 3rd */
         {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) - Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_BodyDojiTrailingIdx = i - BodyDojiTrailingIdx;
      if( cap_BodyDojiTrailingIdx < 0 || cap_BodyDojiTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyDojiTrailingIdx = (cap_BodyDojiTrailingIdx > 0)? cap_BodyDojiTrailingIdx : 1;
      double[] capRing_BodyDojiTrailingIdx_inOpen = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inOpen, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inHigh = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inHigh, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inLow = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inLow, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inClose = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inClose, 0, cap_BodyDojiTrailingIdx);
      int cap_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      if( cap_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyLongTrailingIdx = (cap_BodyLongTrailingIdx > 0)? cap_BodyLongTrailingIdx : 1;
      double[] capRing_BodyLongTrailingIdx_inOpen = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inOpen, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inHigh = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inHigh, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inLow = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inLow, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inClose = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inClose, 0, cap_BodyLongTrailingIdx);
      int cap_BodyShortTrailingIdx = i - BodyShortTrailingIdx;
      if( cap_BodyShortTrailingIdx < 0 || cap_BodyShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyShortTrailingIdx = (cap_BodyShortTrailingIdx > 0)? cap_BodyShortTrailingIdx : 1;
      double[] capRing_BodyShortTrailingIdx_inOpen = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inOpen, 0, cap_BodyShortTrailingIdx);
      double[] capRing_BodyShortTrailingIdx_inHigh = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inHigh, 0, cap_BodyShortTrailingIdx);
      double[] capRing_BodyShortTrailingIdx_inLow = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inLow, 0, cap_BodyShortTrailingIdx);
      double[] capRing_BodyShortTrailingIdx_inClose = new double[allocN_BodyShortTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyShortTrailingIdx, capRing_BodyShortTrailingIdx_inClose, 0, cap_BodyShortTrailingIdx);
      sp.optInPenetration = optInPenetration;
      sp.BodyDojiPeriodTotal = BodyDojiPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.BodyShortPeriodTotal = BodyShortPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_BodyDojiTrailingIdx = 0;
      sp.ringCap_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx;
      sp.ring_BodyDojiTrailingIdx_inOpen = capRing_BodyDojiTrailingIdx_inOpen;
      sp.ring_BodyDojiTrailingIdx_inHigh = capRing_BodyDojiTrailingIdx_inHigh;
      sp.ring_BodyDojiTrailingIdx_inLow = capRing_BodyDojiTrailingIdx_inLow;
      sp.ring_BodyDojiTrailingIdx_inClose = capRing_BodyDojiTrailingIdx_inClose;
      sp.ringPos_BodyLongTrailingIdx = 0;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_inOpen = capRing_BodyLongTrailingIdx_inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh = capRing_BodyLongTrailingIdx_inHigh;
      sp.ring_BodyLongTrailingIdx_inLow = capRing_BodyLongTrailingIdx_inLow;
      sp.ring_BodyLongTrailingIdx_inClose = capRing_BodyLongTrailingIdx_inClose;
      sp.ringPos_BodyShortTrailingIdx = 0;
      sp.ringCap_BodyShortTrailingIdx = cap_BodyShortTrailingIdx;
      sp.ring_BodyShortTrailingIdx_inOpen = capRing_BodyShortTrailingIdx_inOpen;
      sp.ring_BodyShortTrailingIdx_inHigh = capRing_BodyShortTrailingIdx_inHigh;
      sp.ring_BodyShortTrailingIdx_inLow = capRing_BodyShortTrailingIdx_inLow;
      sp.ring_BodyShortTrailingIdx_inClose = capRing_BodyShortTrailingIdx_inClose;
      sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
      sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
      sp.cs_BodyDoji_factor = BodyDoji_factor;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlAbandonedBabyOpen (composition seam). */
   CdlAbandonedBabyStream cdlAbandonedBabyOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, double optInPenetration )
   {
      CdlAbandonedBabyStream sp = new CdlAbandonedBabyStream(this);
      RetCode retCode = cdlAbandonedBabyOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx, optInPenetration);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLABANDONEDBABY open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLABANDONEDBABY open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLABANDONEDBABY open: " + retCode);
   }
   /**
    * Open a live CDLABANDONEDBABY stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlAbandonedBaby} at that bar.
    * <p>The history must hold at least {@code cdlAbandonedBabyLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlAbandonedBabyStream cdlAbandonedBabyOpen( double inOpen[], double inHigh[], double inLow[], double inClose[], double optInPenetration )
   {
      return cdlAbandonedBabyOpenInternal(inOpen, inHigh, inLow, inClose, 0, optInPenetration);
   }
   /**
    * {@link Core#cdlAbandonedBabyOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlAbandonedBaby} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CdlAbandonedBabyStream cdlAbandonedBabyOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], double optInPenetration, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlAbandonedBabyStream sp = new CdlAbandonedBabyStream(this);
      RetCode retCode = cdlAbandonedBabyOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, optInPenetration, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLABANDONEDBABY openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLABANDONEDBABY openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLABANDONEDBABY openAndFill: " + retCode);
   }
