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
 *  103004 AC   Creation
 */

   public int cdlInvertedHammerLookback( )
   {
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      return Math.max(Math.max(BodyShort_avgPeriod, ShadowLong_avgPeriod), ShadowVeryShort_avgPeriod) + 1 ;

   }
   public RetCode cdlInvertedHammer( int startIdx,
                                     int endIdx,
                                     double inOpen[],
                                     double inHigh[],
                                     double inLow[],
                                     double inClose[],
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
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
      lookbackTotal = cdlInvertedHammerLookback();
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
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - small real body
       * - long upper shadow
       * - no, or very short, lower shadow
       * - gap down
       * The meaning of "short", "very short" and "long" is specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100): inverted hammer is always bullish;
       * the user should consider that an inverted hammer must appear in a downtrend, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( (Math.max(inOpen[i], inClose[i]) < Math.min(inOpen[i - 1], inClose[i - 1])) && /* gap down */
             Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* small rb */
             (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long upper shadow */
             (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            /* very short lower shadow */
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) - Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlInvertedHammerUnguarded( int startIdx,
                                              int endIdx,
                                              double inOpen[],
                                              double inHigh[],
                                              double inLow[],
                                              double inClose[],
                                              MInteger outBegIdx,
                                              MInteger outNBElement,
                                              int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      lookbackTotal = cdlInvertedHammerLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( (Math.max(inOpen[i], inClose[i]) < Math.min(inOpen[i - 1], inClose[i - 1])) && Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) - Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlInvertedHammer( int startIdx,
                                     int endIdx,
                                     float inOpen[],
                                     float inHigh[],
                                     float inLow[],
                                     float inClose[],
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlInvertedHammerLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( (Math.max((double)inOpen[i], (double)inClose[i]) < Math.min((double)inOpen[i - 1], (double)inClose[i - 1])) && Math.abs((double)inClose[i] - (double)inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) - Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowLongTrailingIdx] - (double)inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[ShadowLongTrailingIdx] - (double)inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[ShadowLongTrailingIdx] - (double)inLow[ShadowLongTrailingIdx]) - Math.abs((double)inClose[ShadowLongTrailingIdx] - (double)inOpen[ShadowLongTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx] - (double)inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx] - (double)inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx] - (double)inLow[ShadowVeryShortTrailingIdx]) - Math.abs((double)inClose[ShadowVeryShortTrailingIdx] - (double)inOpen[ShadowVeryShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlInvertedHammerUnguarded( int startIdx,
                                              int endIdx,
                                              float inOpen[],
                                              float inHigh[],
                                              float inLow[],
                                              float inClose[],
                                              MInteger outBegIdx,
                                              MInteger outNBElement,
                                              int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      lookbackTotal = cdlInvertedHammerLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( (Math.max((double)inOpen[i], (double)inClose[i]) < Math.min((double)inOpen[i - 1], (double)inClose[i - 1])) && Math.abs((double)inClose[i] - (double)inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) - Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowLongTrailingIdx] - (double)inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[ShadowLongTrailingIdx] - (double)inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[ShadowLongTrailingIdx] - (double)inLow[ShadowLongTrailingIdx]) - Math.abs((double)inClose[ShadowLongTrailingIdx] - (double)inOpen[ShadowLongTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowVeryShortTrailingIdx] - (double)inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? ((double)inHigh[ShadowVeryShortTrailingIdx] - (double)inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? (((double)inHigh[ShadowVeryShortTrailingIdx] - (double)inLow[ShadowVeryShortTrailingIdx]) - Math.abs((double)inClose[ShadowVeryShortTrailingIdx] - (double)inOpen[ShadowVeryShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CDLINVERTEDHAMMER stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlInvertedHammer} over the same series.
    * Open with {@link Core#cdlInvertedHammerOpen}; there is no close — the handle is
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
   public static final class CdlInvertedHammerStream {
      final Core core;
      double BodyPeriodTotal;
      double ShadowLongPeriodTotal;
      double ShadowVeryShortPeriodTotal;
      double lag1_inOpen;
      double lag1_inClose;
      int ringPos_BodyTrailingIdx;
      int ringCap_BodyTrailingIdx;
      double[] ring_BodyTrailingIdx_inOpen;
      double[] ring_BodyTrailingIdx_inHigh;
      double[] ring_BodyTrailingIdx_inLow;
      double[] ring_BodyTrailingIdx_inClose;
      int ringPos_ShadowLongTrailingIdx;
      int ringCap_ShadowLongTrailingIdx;
      double[] ring_ShadowLongTrailingIdx_inOpen;
      double[] ring_ShadowLongTrailingIdx_inHigh;
      double[] ring_ShadowLongTrailingIdx_inLow;
      double[] ring_ShadowLongTrailingIdx_inClose;
      int ringPos_ShadowVeryShortTrailingIdx;
      int ringCap_ShadowVeryShortTrailingIdx;
      double[] ring_ShadowVeryShortTrailingIdx_inOpen;
      double[] ring_ShadowVeryShortTrailingIdx_inHigh;
      double[] ring_ShadowVeryShortTrailingIdx_inLow;
      double[] ring_ShadowVeryShortTrailingIdx_inClose;
      int cs_BodyShort_rangeType;
      int cs_BodyShort_avgPeriod;
      double cs_BodyShort_factor;
      int cs_ShadowLong_rangeType;
      int cs_ShadowLong_avgPeriod;
      double cs_ShadowLong_factor;
      int cs_ShadowVeryShort_rangeType;
      int cs_ShadowVeryShort_avgPeriod;
      double cs_ShadowVeryShort_factor;
      int cur_outInteger;

      CdlInvertedHammerStream( Core core ) { this.core = core; }

      CdlInvertedHammerStream( CdlInvertedHammerStream other ) {
         this.core = other.core;
         this.BodyPeriodTotal = other.BodyPeriodTotal;
         this.ShadowLongPeriodTotal = other.ShadowLongPeriodTotal;
         this.ShadowVeryShortPeriodTotal = other.ShadowVeryShortPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyTrailingIdx = other.ringPos_BodyTrailingIdx;
         this.ringCap_BodyTrailingIdx = other.ringCap_BodyTrailingIdx;
         this.ring_BodyTrailingIdx_inOpen = other.ring_BodyTrailingIdx_inOpen.clone();
         this.ring_BodyTrailingIdx_inHigh = other.ring_BodyTrailingIdx_inHigh.clone();
         this.ring_BodyTrailingIdx_inLow = other.ring_BodyTrailingIdx_inLow.clone();
         this.ring_BodyTrailingIdx_inClose = other.ring_BodyTrailingIdx_inClose.clone();
         this.ringPos_ShadowLongTrailingIdx = other.ringPos_ShadowLongTrailingIdx;
         this.ringCap_ShadowLongTrailingIdx = other.ringCap_ShadowLongTrailingIdx;
         this.ring_ShadowLongTrailingIdx_inOpen = other.ring_ShadowLongTrailingIdx_inOpen.clone();
         this.ring_ShadowLongTrailingIdx_inHigh = other.ring_ShadowLongTrailingIdx_inHigh.clone();
         this.ring_ShadowLongTrailingIdx_inLow = other.ring_ShadowLongTrailingIdx_inLow.clone();
         this.ring_ShadowLongTrailingIdx_inClose = other.ring_ShadowLongTrailingIdx_inClose.clone();
         this.ringPos_ShadowVeryShortTrailingIdx = other.ringPos_ShadowVeryShortTrailingIdx;
         this.ringCap_ShadowVeryShortTrailingIdx = other.ringCap_ShadowVeryShortTrailingIdx;
         this.ring_ShadowVeryShortTrailingIdx_inOpen = other.ring_ShadowVeryShortTrailingIdx_inOpen.clone();
         this.ring_ShadowVeryShortTrailingIdx_inHigh = other.ring_ShadowVeryShortTrailingIdx_inHigh.clone();
         this.ring_ShadowVeryShortTrailingIdx_inLow = other.ring_ShadowVeryShortTrailingIdx_inLow.clone();
         this.ring_ShadowVeryShortTrailingIdx_inClose = other.ring_ShadowVeryShortTrailingIdx_inClose.clone();
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cs_ShadowLong_rangeType = other.cs_ShadowLong_rangeType;
         this.cs_ShadowLong_avgPeriod = other.cs_ShadowLong_avgPeriod;
         this.cs_ShadowLong_factor = other.cs_ShadowLong_factor;
         this.cs_ShadowVeryShort_rangeType = other.cs_ShadowVeryShort_rangeType;
         this.cs_ShadowVeryShort_avgPeriod = other.cs_ShadowVeryShort_avgPeriod;
         this.cs_ShadowVeryShort_factor = other.cs_ShadowVeryShort_factor;
         this.cur_outInteger = other.cur_outInteger;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.cdlInvertedHammerStreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CdlInvertedHammerStream scratch = new CdlInvertedHammerStream(this);
         core.cdlInvertedHammerStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CdlInvertedHammerStream copy() {
         return new CdlInvertedHammerStream(this);
      }
   }
   void cdlInvertedHammerStreamStep( CdlInvertedHammerStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyShort_rangeType = sp.cs_BodyShort_rangeType;
      int BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
      double BodyShort_factor = sp.cs_BodyShort_factor;
      int ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
      int ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
      double ShadowLong_factor = sp.cs_ShadowLong_factor;
      int ShadowVeryShort_rangeType = sp.cs_ShadowVeryShort_rangeType;
      int ShadowVeryShort_avgPeriod = sp.cs_ShadowVeryShort_avgPeriod;
      double ShadowVeryShort_factor = sp.cs_ShadowVeryShort_factor;
      if( sp.ringCap_BodyTrailingIdx == 0 ) {
         sp.ring_BodyTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_ShadowLongTrailingIdx == 0 ) {
         sp.ring_ShadowLongTrailingIdx_inOpen[0] = inOpen;
         sp.ring_ShadowLongTrailingIdx_inHigh[0] = inHigh;
         sp.ring_ShadowLongTrailingIdx_inLow[0] = inLow;
         sp.ring_ShadowLongTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_ShadowVeryShortTrailingIdx == 0 ) {
         sp.ring_ShadowVeryShortTrailingIdx_inOpen[0] = inOpen;
         sp.ring_ShadowVeryShortTrailingIdx_inHigh[0] = inHigh;
         sp.ring_ShadowVeryShortTrailingIdx_inLow[0] = inLow;
         sp.ring_ShadowVeryShortTrailingIdx_inClose[0] = inClose;
      }
      if( (Math.max(inOpen, inClose) < Math.min(sp.lag1_inOpen, sp.lag1_inClose)) && /* gap down */
          Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* small rb */
          (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (sp.ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long upper shadow */
          (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (sp.ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) )
      {
         /* very short lower shadow */
         sp.cur_outInteger = 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? (sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx]) - Math.abs(sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx])) : 0.0)));
      sp.ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(sp.ring_ShadowLongTrailingIdx_inClose[sp.ringPos_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inOpen[sp.ringPos_ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (sp.ring_ShadowLongTrailingIdx_inHigh[sp.ringPos_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inLow[sp.ringPos_ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((sp.ring_ShadowLongTrailingIdx_inHigh[sp.ringPos_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inLow[sp.ringPos_ShadowLongTrailingIdx]) - Math.abs(sp.ring_ShadowLongTrailingIdx_inClose[sp.ringPos_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inOpen[sp.ringPos_ShadowLongTrailingIdx])) : 0.0)));
      sp.ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(sp.ring_ShadowVeryShortTrailingIdx_inClose[sp.ringPos_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inOpen[sp.ringPos_ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (sp.ring_ShadowVeryShortTrailingIdx_inHigh[sp.ringPos_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inLow[sp.ringPos_ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((sp.ring_ShadowVeryShortTrailingIdx_inHigh[sp.ringPos_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inLow[sp.ringPos_ShadowVeryShortTrailingIdx]) - Math.abs(sp.ring_ShadowVeryShortTrailingIdx_inClose[sp.ringPos_ShadowVeryShortTrailingIdx] - sp.ring_ShadowVeryShortTrailingIdx_inOpen[sp.ringPos_ShadowVeryShortTrailingIdx])) : 0.0)));
      sp.lag1_inOpen = inOpen;
      sp.lag1_inClose = inClose;
      sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx] = inOpen;
      sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] = inHigh;
      sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx] = inLow;
      sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] = inClose;
      sp.ringPos_BodyTrailingIdx = sp.ringPos_BodyTrailingIdx + 1;
      if( sp.ringPos_BodyTrailingIdx >= sp.ringCap_BodyTrailingIdx ) {
         sp.ringPos_BodyTrailingIdx = 0;
      }
      sp.ring_ShadowLongTrailingIdx_inOpen[sp.ringPos_ShadowLongTrailingIdx] = inOpen;
      sp.ring_ShadowLongTrailingIdx_inHigh[sp.ringPos_ShadowLongTrailingIdx] = inHigh;
      sp.ring_ShadowLongTrailingIdx_inLow[sp.ringPos_ShadowLongTrailingIdx] = inLow;
      sp.ring_ShadowLongTrailingIdx_inClose[sp.ringPos_ShadowLongTrailingIdx] = inClose;
      sp.ringPos_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx + 1;
      if( sp.ringPos_ShadowLongTrailingIdx >= sp.ringCap_ShadowLongTrailingIdx ) {
         sp.ringPos_ShadowLongTrailingIdx = 0;
      }
      sp.ring_ShadowVeryShortTrailingIdx_inOpen[sp.ringPos_ShadowVeryShortTrailingIdx] = inOpen;
      sp.ring_ShadowVeryShortTrailingIdx_inHigh[sp.ringPos_ShadowVeryShortTrailingIdx] = inHigh;
      sp.ring_ShadowVeryShortTrailingIdx_inLow[sp.ringPos_ShadowVeryShortTrailingIdx] = inLow;
      sp.ring_ShadowVeryShortTrailingIdx_inClose[sp.ringPos_ShadowVeryShortTrailingIdx] = inClose;
      sp.ringPos_ShadowVeryShortTrailingIdx = sp.ringPos_ShadowVeryShortTrailingIdx + 1;
      if( sp.ringPos_ShadowVeryShortTrailingIdx >= sp.ringCap_ShadowVeryShortTrailingIdx ) {
         sp.ringPos_ShadowVeryShortTrailingIdx = 0;
      }
   }
   private RetCode cdlInvertedHammerOpenBody( CdlInvertedHammerStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      double BodyPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int lastValue_outInteger = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlInvertedHammerLookback();
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
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - small real body
       * - long upper shadow
       * - no, or very short, lower shadow
       * - gap down
       * The meaning of "short", "very short" and "long" is specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100): inverted hammer is always bullish;
       * the user should consider that an inverted hammer must appear in a downtrend, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( (Math.max(inOpen[i], inClose[i]) < Math.min(inOpen[i - 1], inClose[i - 1])) && /* gap down */
             Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* small rb */
             (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long upper shadow */
             (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            /* very short lower shadow */
            lastValue_outInteger = 100;
         } else {
            lastValue_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) - Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_BodyTrailingIdx = i - BodyTrailingIdx;
      if( cap_BodyTrailingIdx < 0 || cap_BodyTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyTrailingIdx = (cap_BodyTrailingIdx > 0)? cap_BodyTrailingIdx : 1;
      double[] capRing_BodyTrailingIdx_inOpen = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inOpen, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inHigh = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inHigh, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inLow = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inLow, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inClose = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inClose, 0, cap_BodyTrailingIdx);
      int cap_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
      if( cap_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowLongTrailingIdx = (cap_ShadowLongTrailingIdx > 0)? cap_ShadowLongTrailingIdx : 1;
      double[] capRing_ShadowLongTrailingIdx_inOpen = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inOpen, 0, cap_ShadowLongTrailingIdx);
      double[] capRing_ShadowLongTrailingIdx_inHigh = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inHigh, 0, cap_ShadowLongTrailingIdx);
      double[] capRing_ShadowLongTrailingIdx_inLow = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inLow, 0, cap_ShadowLongTrailingIdx);
      double[] capRing_ShadowLongTrailingIdx_inClose = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inClose, 0, cap_ShadowLongTrailingIdx);
      int cap_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
      if( cap_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowVeryShortTrailingIdx = (cap_ShadowVeryShortTrailingIdx > 0)? cap_ShadowVeryShortTrailingIdx : 1;
      double[] capRing_ShadowVeryShortTrailingIdx_inOpen = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inOpen, 0, cap_ShadowVeryShortTrailingIdx);
      double[] capRing_ShadowVeryShortTrailingIdx_inHigh = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inHigh, 0, cap_ShadowVeryShortTrailingIdx);
      double[] capRing_ShadowVeryShortTrailingIdx_inLow = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inLow, 0, cap_ShadowVeryShortTrailingIdx);
      double[] capRing_ShadowVeryShortTrailingIdx_inClose = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inClose, 0, cap_ShadowVeryShortTrailingIdx);
      sp.BodyPeriodTotal = BodyPeriodTotal;
      sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
      sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_BodyTrailingIdx = 0;
      sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
      sp.ring_BodyTrailingIdx_inOpen = capRing_BodyTrailingIdx_inOpen;
      sp.ring_BodyTrailingIdx_inHigh = capRing_BodyTrailingIdx_inHigh;
      sp.ring_BodyTrailingIdx_inLow = capRing_BodyTrailingIdx_inLow;
      sp.ring_BodyTrailingIdx_inClose = capRing_BodyTrailingIdx_inClose;
      sp.ringPos_ShadowLongTrailingIdx = 0;
      sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
      sp.ring_ShadowLongTrailingIdx_inOpen = capRing_ShadowLongTrailingIdx_inOpen;
      sp.ring_ShadowLongTrailingIdx_inHigh = capRing_ShadowLongTrailingIdx_inHigh;
      sp.ring_ShadowLongTrailingIdx_inLow = capRing_ShadowLongTrailingIdx_inLow;
      sp.ring_ShadowLongTrailingIdx_inClose = capRing_ShadowLongTrailingIdx_inClose;
      sp.ringPos_ShadowVeryShortTrailingIdx = 0;
      sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
      sp.ring_ShadowVeryShortTrailingIdx_inOpen = capRing_ShadowVeryShortTrailingIdx_inOpen;
      sp.ring_ShadowVeryShortTrailingIdx_inHigh = capRing_ShadowVeryShortTrailingIdx_inHigh;
      sp.ring_ShadowVeryShortTrailingIdx_inLow = capRing_ShadowVeryShortTrailingIdx_inLow;
      sp.ring_ShadowVeryShortTrailingIdx_inClose = capRing_ShadowVeryShortTrailingIdx_inClose;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cs_ShadowLong_rangeType = ShadowLong_rangeType;
      sp.cs_ShadowLong_avgPeriod = ShadowLong_avgPeriod;
      sp.cs_ShadowLong_factor = ShadowLong_factor;
      sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
      sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
      sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlInvertedHammerOpenAndFillBody( CdlInvertedHammerStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      double ShadowLongPeriodTotal = 0;
      double ShadowVeryShortPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int ShadowVeryShortTrailingIdx = 0;
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
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowVeryShort_rangeType = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].rangeType.ordinal();
      int ShadowVeryShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].avgPeriod;
      double ShadowVeryShort_factor = this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlInvertedHammerLookback();
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
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowLongPeriodTotal = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      ShadowVeryShortPeriodTotal = 0;
      ShadowVeryShortTrailingIdx = startIdx - ShadowVeryShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowVeryShortTrailingIdx;
      while( i < startIdx ) {
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - small real body
       * - long upper shadow
       * - no, or very short, lower shadow
       * - gap down
       * The meaning of "short", "very short" and "long" is specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100): inverted hammer is always bullish;
       * the user should consider that an inverted hammer must appear in a downtrend, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( (Math.max(inOpen[i], inClose[i]) < Math.min(inOpen[i - 1], inClose[i - 1])) && /* gap down */
             Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && /* small rb */
             (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long upper shadow */
             (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowVeryShort_factor * (((ShadowVeryShort_avgPeriod != 0) ? (ShadowVeryShortPeriodTotal / ShadowVeryShort_avgPeriod) : ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowVeryShort_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            /* very short lower shadow */
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal += ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) - Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : 0.0)));
         ShadowVeryShortPeriodTotal += ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowVeryShort_rangeType == 0) ? (Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : ((ShadowVeryShort_rangeType == 1) ? (inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) : ((ShadowVeryShort_rangeType == 2) ? ((inHigh[ShadowVeryShortTrailingIdx] - inLow[ShadowVeryShortTrailingIdx]) - Math.abs(inClose[ShadowVeryShortTrailingIdx] - inOpen[ShadowVeryShortTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         ShadowVeryShortTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_BodyTrailingIdx = i - BodyTrailingIdx;
      if( cap_BodyTrailingIdx < 0 || cap_BodyTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyTrailingIdx = (cap_BodyTrailingIdx > 0)? cap_BodyTrailingIdx : 1;
      double[] capRing_BodyTrailingIdx_inOpen = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inOpen, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inHigh = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inHigh, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inLow = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inLow, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inClose = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inClose, 0, cap_BodyTrailingIdx);
      int cap_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
      if( cap_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowLongTrailingIdx = (cap_ShadowLongTrailingIdx > 0)? cap_ShadowLongTrailingIdx : 1;
      double[] capRing_ShadowLongTrailingIdx_inOpen = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inOpen, 0, cap_ShadowLongTrailingIdx);
      double[] capRing_ShadowLongTrailingIdx_inHigh = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inHigh, 0, cap_ShadowLongTrailingIdx);
      double[] capRing_ShadowLongTrailingIdx_inLow = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inLow, 0, cap_ShadowLongTrailingIdx);
      double[] capRing_ShadowLongTrailingIdx_inClose = new double[allocN_ShadowLongTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_ShadowLongTrailingIdx, capRing_ShadowLongTrailingIdx_inClose, 0, cap_ShadowLongTrailingIdx);
      int cap_ShadowVeryShortTrailingIdx = i - ShadowVeryShortTrailingIdx;
      if( cap_ShadowVeryShortTrailingIdx < 0 || cap_ShadowVeryShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowVeryShortTrailingIdx = (cap_ShadowVeryShortTrailingIdx > 0)? cap_ShadowVeryShortTrailingIdx : 1;
      double[] capRing_ShadowVeryShortTrailingIdx_inOpen = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inOpen, 0, cap_ShadowVeryShortTrailingIdx);
      double[] capRing_ShadowVeryShortTrailingIdx_inHigh = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inHigh, 0, cap_ShadowVeryShortTrailingIdx);
      double[] capRing_ShadowVeryShortTrailingIdx_inLow = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inLow, 0, cap_ShadowVeryShortTrailingIdx);
      double[] capRing_ShadowVeryShortTrailingIdx_inClose = new double[allocN_ShadowVeryShortTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_ShadowVeryShortTrailingIdx, capRing_ShadowVeryShortTrailingIdx_inClose, 0, cap_ShadowVeryShortTrailingIdx);
      sp.BodyPeriodTotal = BodyPeriodTotal;
      sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
      sp.ShadowVeryShortPeriodTotal = ShadowVeryShortPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_BodyTrailingIdx = 0;
      sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
      sp.ring_BodyTrailingIdx_inOpen = capRing_BodyTrailingIdx_inOpen;
      sp.ring_BodyTrailingIdx_inHigh = capRing_BodyTrailingIdx_inHigh;
      sp.ring_BodyTrailingIdx_inLow = capRing_BodyTrailingIdx_inLow;
      sp.ring_BodyTrailingIdx_inClose = capRing_BodyTrailingIdx_inClose;
      sp.ringPos_ShadowLongTrailingIdx = 0;
      sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
      sp.ring_ShadowLongTrailingIdx_inOpen = capRing_ShadowLongTrailingIdx_inOpen;
      sp.ring_ShadowLongTrailingIdx_inHigh = capRing_ShadowLongTrailingIdx_inHigh;
      sp.ring_ShadowLongTrailingIdx_inLow = capRing_ShadowLongTrailingIdx_inLow;
      sp.ring_ShadowLongTrailingIdx_inClose = capRing_ShadowLongTrailingIdx_inClose;
      sp.ringPos_ShadowVeryShortTrailingIdx = 0;
      sp.ringCap_ShadowVeryShortTrailingIdx = cap_ShadowVeryShortTrailingIdx;
      sp.ring_ShadowVeryShortTrailingIdx_inOpen = capRing_ShadowVeryShortTrailingIdx_inOpen;
      sp.ring_ShadowVeryShortTrailingIdx_inHigh = capRing_ShadowVeryShortTrailingIdx_inHigh;
      sp.ring_ShadowVeryShortTrailingIdx_inLow = capRing_ShadowVeryShortTrailingIdx_inLow;
      sp.ring_ShadowVeryShortTrailingIdx_inClose = capRing_ShadowVeryShortTrailingIdx_inClose;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cs_ShadowLong_rangeType = ShadowLong_rangeType;
      sp.cs_ShadowLong_avgPeriod = ShadowLong_avgPeriod;
      sp.cs_ShadowLong_factor = ShadowLong_factor;
      sp.cs_ShadowVeryShort_rangeType = ShadowVeryShort_rangeType;
      sp.cs_ShadowVeryShort_avgPeriod = ShadowVeryShort_avgPeriod;
      sp.cs_ShadowVeryShort_factor = ShadowVeryShort_factor;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlInvertedHammerOpen (composition seam). */
   CdlInvertedHammerStream cdlInvertedHammerOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlInvertedHammerStream sp = new CdlInvertedHammerStream(this);
      RetCode retCode = cdlInvertedHammerOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLINVERTEDHAMMER open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLINVERTEDHAMMER open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLINVERTEDHAMMER open: " + retCode);
   }
   /**
    * Open a live CDLINVERTEDHAMMER stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlInvertedHammer} at that bar.
    * <p>The history must hold at least {@code cdlInvertedHammerLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlInvertedHammerStream cdlInvertedHammerOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return cdlInvertedHammerOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlInvertedHammerOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlInvertedHammer} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CdlInvertedHammerStream cdlInvertedHammerOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlInvertedHammerStream sp = new CdlInvertedHammerStream(this);
      RetCode retCode = cdlInvertedHammerOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLINVERTEDHAMMER openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLINVERTEDHAMMER openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLINVERTEDHAMMER openAndFill: " + retCode);
   }
