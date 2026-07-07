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
 *  120804 AC   Creation
 */

   public int cdlStalledPatternLookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      return Math.max(Math.max(BodyLong_avgPeriod, BodyShort_avgPeriod), Math.max(ShadowVeryShort_avgPeriod, Near_avgPeriod)) + 2 ;

   }
   public RetCode cdlStalledPattern( int startIdx,
                                     int endIdx,
                                     double inOpen[],
                                     double inHigh[],
                                     double inLow[],
                                     double inClose[],
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     int outInteger[] )
   {
      double[] BodyLongPeriodTotal = new double[3];
      double[] NearPeriodTotal = new double[3];
      double BodyShortPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlStalledPatternLookback();
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
      BodyLongPeriodTotal[2] = 0;
      BodyLongPeriodTotal[1] = 0;
      BodyLongPeriodTotal[0] = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      BodyShortPeriodTotal = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      NearPeriodTotal[2] = 0;
      NearPeriodTotal[1] = 0;
      NearPeriodTotal[0] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal[2] = BodyLongPeriodTotal[2] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - three white candlesticks with consecutively higher closes
       * - first candle: long white
       * - second candle: long white with no or very short upper shadow opening within or near the previous white real body
       * and closing higher than the prior candle
       * - third candle: small white that gaps away or "rides on the shoulder" of the prior long real body (= it's at
       * the upper end of the prior real body)
       * The meanings of "long", "very short", "short", "near" are specified with TA_SetCandleSettings;
       * outInteger is negative (-1 to -100): stalled pattern is always bearish;
       * the user should consider that stalled pattern is significant when it appears in uptrend, while this function
       * does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && /* 1st white */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && /* 2nd white */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&         /* 3rd white */
             inClose[i] > inClose[i - 1] &&
             inClose[i - 1] > inClose[i - 2] &&                      /* consecutive higher closes */
             Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[2] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: long real body */
             (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && /* very short upper shadow */
             inOpen[i - 1] > inOpen[i - 2] &&                        /* opens within/near 1st real body */
             inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd: small real body */
             inOpen[i] >= inClose[i - 1] - Math.abs(inClose[i] - inOpen[i]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) /* rides on the shoulder of 2nd real body */
         {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            BodyLongPeriodTotal[totIdx] = BodyLongPeriodTotal[totIdx] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - totIdx] - inOpen[BodyLongTrailingIdx - totIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - totIdx] - inLow[BodyLongTrailingIdx - totIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - totIdx] - inLow[BodyLongTrailingIdx - totIdx]) - Math.abs(inClose[BodyLongTrailingIdx - totIdx] - inOpen[BodyLongTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) - Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) - Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) - Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlStalledPatternUnguarded( int startIdx,
                                              int endIdx,
                                              double inOpen[],
                                              double inHigh[],
                                              double inLow[],
                                              double inClose[],
                                              MInteger outBegIdx,
                                              MInteger outNBElement,
                                              int outInteger[] )
   {
      double[] BodyLongPeriodTotal = new double[3];
      double[] NearPeriodTotal = new double[3];
      double BodyShortPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      lookbackTotal = cdlStalledPatternLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal[2] = 0;
      BodyLongPeriodTotal[1] = 0;
      BodyLongPeriodTotal[0] = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      BodyShortPeriodTotal = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      NearPeriodTotal[2] = 0;
      NearPeriodTotal[1] = 0;
      NearPeriodTotal[0] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal[2] = BodyLongPeriodTotal[2] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inClose[i] > inClose[i - 1] && inClose[i - 1] > inClose[i - 2] && Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[2] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && inOpen[i - 1] > inOpen[i - 2] && inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && inOpen[i] >= inClose[i - 1] - Math.abs(inClose[i] - inOpen[i]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            BodyLongPeriodTotal[totIdx] = BodyLongPeriodTotal[totIdx] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - totIdx] - inOpen[BodyLongTrailingIdx - totIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - totIdx] - inLow[BodyLongTrailingIdx - totIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - totIdx] - inLow[BodyLongTrailingIdx - totIdx]) - Math.abs(inClose[BodyLongTrailingIdx - totIdx] - inOpen[BodyLongTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) - Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyShortTrailingIdx] - inLow[BodyShortTrailingIdx]) - Math.abs(inClose[BodyShortTrailingIdx] - inOpen[BodyShortTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx - 1] - inLow[ShadowVeryShortTrailingIdx - 1]) - Math.abs(inClose[ShadowVeryShortTrailingIdx - 1] - inOpen[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlStalledPattern( int startIdx,
                                     int endIdx,
                                     float inOpen[],
                                     float inHigh[],
                                     float inLow[],
                                     float inClose[],
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     int outInteger[] )
   {
      double[] BodyLongPeriodTotal = new double[3];
      double[] NearPeriodTotal = new double[3];
      double BodyShortPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlStalledPatternLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal[2] = 0;
      BodyLongPeriodTotal[1] = 0;
      BodyLongPeriodTotal[0] = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      BodyShortPeriodTotal = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      NearPeriodTotal[2] = 0;
      NearPeriodTotal[1] = 0;
      NearPeriodTotal[0] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal[2] = BodyLongPeriodTotal[2] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inClose[i] > (double)inClose[i - 1] && (double)inClose[i - 1] > (double)inClose[i - 2] && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[2] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i - 1] - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inClose[i - 1] : (double)inOpen[i - 1])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] > (double)inOpen[i - 2] && (double)inOpen[i - 1] <= (double)inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] >= (double)inClose[i - 1] - Math.abs((double)inClose[i] - (double)inOpen[i]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            BodyLongPeriodTotal[totIdx] = BodyLongPeriodTotal[totIdx] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - totIdx] - (double)inOpen[BodyLongTrailingIdx - totIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - totIdx] - (double)inLow[BodyLongTrailingIdx - totIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - totIdx] - (double)inLow[BodyLongTrailingIdx - totIdx]) - Math.abs((double)inClose[BodyLongTrailingIdx - totIdx] - (double)inOpen[BodyLongTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) - Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) - Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx - 1] - (double)inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx - 1] - (double)inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx - 1] - (double)inLow[ShadowVeryShortTrailingIdx - 1]) - Math.abs((double)inClose[ShadowVeryShortTrailingIdx - 1] - (double)inOpen[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlStalledPatternUnguarded( int startIdx,
                                              int endIdx,
                                              float inOpen[],
                                              float inHigh[],
                                              float inLow[],
                                              float inClose[],
                                              MInteger outBegIdx,
                                              MInteger outNBElement,
                                              int outInteger[] )
   {
      double[] BodyLongPeriodTotal = new double[3];
      double[] NearPeriodTotal = new double[3];
      double BodyShortPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int BodyShortTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      lookbackTotal = cdlStalledPatternLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal[2] = 0;
      BodyLongPeriodTotal[1] = 0;
      BodyLongPeriodTotal[0] = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      BodyShortPeriodTotal = 0;
      BodyShortTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      NearPeriodTotal[2] = 0;
      NearPeriodTotal[1] = 0;
      NearPeriodTotal[0] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal[2] = BodyLongPeriodTotal[2] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyShortTrailingIdx;
      while( i < startIdx ) {
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inClose[i] > (double)inClose[i - 1] && (double)inClose[i - 1] > (double)inClose[i - 2] && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[2] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i - 1] - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inClose[i - 1] : (double)inOpen[i - 1])) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] > (double)inOpen[i - 2] && (double)inOpen[i - 1] <= (double)inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyShortPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] >= (double)inClose[i - 1] - Math.abs((double)inClose[i] - (double)inOpen[i]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            BodyLongPeriodTotal[totIdx] = BodyLongPeriodTotal[totIdx] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - totIdx] - (double)inOpen[BodyLongTrailingIdx - totIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - totIdx] - (double)inLow[BodyLongTrailingIdx - totIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - totIdx] - (double)inLow[BodyLongTrailingIdx - totIdx]) - Math.abs((double)inClose[BodyLongTrailingIdx - totIdx] - (double)inOpen[BodyLongTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) - Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyShortPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyShortTrailingIdx] - (double)inLow[BodyShortTrailingIdx]) - Math.abs((double)inClose[BodyShortTrailingIdx] - (double)inOpen[BodyShortTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx - 1] - (double)inOpen[ShadowVeryShortTrailingIdx - 1])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx - 1] - (double)inLow[ShadowVeryShortTrailingIdx - 1]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx - 1] - (double)inLow[ShadowVeryShortTrailingIdx - 1]) - Math.abs((double)inClose[ShadowVeryShortTrailingIdx - 1] - (double)inOpen[ShadowVeryShortTrailingIdx - 1])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyShortTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
