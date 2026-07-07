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
 *  071704 AC   Creation
 */

   public int cdlLongLineLookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      return Math.max(BodyLong_avgPeriod, ShadowShort_avgPeriod) ;

   }
   public RetCode cdlLongLine( int startIdx,
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
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlLongLineLookback();
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
      BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - long real body
       * - short upper and lower shadow
       * The meaning of "long" and "short" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when white (bullish), negative (-1 to -100) when black (bearish)
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) - Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlLongLineUnguarded( int startIdx,
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
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      lookbackTotal = cdlLongLineLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) - Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlLongLine( int startIdx,
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
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlLongLineLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i] - (double)inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) - Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowTrailingIdx] - (double)inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[ShadowTrailingIdx] - (double)inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[ShadowTrailingIdx] - (double)inLow[ShadowTrailingIdx]) - Math.abs((double)inClose[ShadowTrailingIdx] - (double)inOpen[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlLongLineUnguarded( int startIdx,
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
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      lookbackTotal = cdlLongLineLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i] - (double)inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) - Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowTrailingIdx] - (double)inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[ShadowTrailingIdx] - (double)inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[ShadowTrailingIdx] - (double)inLow[ShadowTrailingIdx]) - Math.abs((double)inClose[ShadowTrailingIdx] - (double)inOpen[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
