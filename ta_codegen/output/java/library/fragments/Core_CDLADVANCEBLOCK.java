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
 *  120404 AC   Creation
 */

   public int cdlAdvanceBlockLookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Far_rangeType = this.candleSettings[CandleSettingType.Far.ordinal()].rangeType.ordinal();
      int Far_avgPeriod = this.candleSettings[CandleSettingType.Far.ordinal()].avgPeriod;
      double Far_factor = this.candleSettings[CandleSettingType.Far.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      return Math.max(Math.max(Math.max(ShadowLong_avgPeriod, ShadowShort_avgPeriod), Math.max(Far_avgPeriod, Near_avgPeriod)), BodyLong_avgPeriod) + 2 ;

   }
   public RetCode cdlAdvanceBlock( int startIdx,
                                   int endIdx,
                                   double inOpen[],
                                   double inHigh[],
                                   double inLow[],
                                   double inClose[],
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   int outInteger[] )
   {
      double[] ShadowShortPeriodTotal = new double[3];
      double[] ShadowLongPeriodTotal = new double[2];
      double[] NearPeriodTotal = new double[3];
      double[] FarPeriodTotal = new double[3];
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int ShadowShortTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int FarTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Far_rangeType = this.candleSettings[CandleSettingType.Far.ordinal()].rangeType.ordinal();
      int Far_avgPeriod = this.candleSettings[CandleSettingType.Far.ordinal()].avgPeriod;
      double Far_factor = this.candleSettings[CandleSettingType.Far.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
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
      lookbackTotal = cdlAdvanceBlockLookback();
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
      ShadowShortPeriodTotal[2] = 0;
      ShadowShortPeriodTotal[1] = 0;
      ShadowShortPeriodTotal[0] = 0;
      ShadowShortTrailingIdx = startIdx - ShadowShort_avgPeriod;
      ShadowLongPeriodTotal[1] = 0;
      ShadowLongPeriodTotal[0] = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      NearPeriodTotal[2] = 0;
      NearPeriodTotal[1] = 0;
      NearPeriodTotal[0] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      FarPeriodTotal[2] = 0;
      FarPeriodTotal[1] = 0;
      FarPeriodTotal[0] = 0;
      FarTrailingIdx = startIdx - Far_avgPeriod;
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = ShadowShortTrailingIdx;
      while( i < startIdx ) {
         ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = FarTrailingIdx;
      while( i < startIdx ) {
         FarPeriodTotal[2] = FarPeriodTotal[2] + ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         FarPeriodTotal[1] = FarPeriodTotal[1] + ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - three white candlesticks with consecutively higher closes
       * - each candle opens within or near the previous white real body
       * - first candle: long white with no or very short upper shadow (a short shadow is accepted too for more flexibility)
       * - second and third candles, or only third candle, show signs of weakening: progressively smaller white real bodies
       * and/or relatively long upper shadows; see below for specific conditions
       * The meanings of "long body", "short shadow", "far" and "near" are specified with TA_SetCandleSettings;
       * outInteger is negative (-1 to -100): advance block is always bearish;
       * the user should consider that advance block is significant when it appears in uptrend, while this function
       * does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && /* 1st white */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && /* 2nd white */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&         /* 3rd white */
             inClose[i] > inClose[i - 1] &&
             inClose[i - 1] > inClose[i - 2] &&                      /* consecutive higher closes */
             inOpen[i - 1] > inOpen[i - 2] &&                        /* 2nd opens within/near 1st real body */
             inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i] > inOpen[i - 1] &&                            /* 3rd opens within/near 2nd real body */
             inOpen[i] <= inClose[i - 1] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
             (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) && ((inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) || (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) ) /* 1st: short upper shadow ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance 3 far smaller than 2 advance blocked with the 3rd ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
         {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowShortPeriodTotal[totIdx] = ShadowShortPeriodTotal[totIdx] + (((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - totIdx] - inOpen[ShadowShortTrailingIdx - totIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowShortTrailingIdx - totIdx] - inLow[ShadowShortTrailingIdx - totIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowShortTrailingIdx - totIdx] - inLow[ShadowShortTrailingIdx - totIdx]) - Math.abs(inClose[ShadowShortTrailingIdx - totIdx] - inOpen[ShadowShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            ShadowLongPeriodTotal[totIdx] = ShadowLongPeriodTotal[totIdx] + (((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - totIdx] - inOpen[ShadowLongTrailingIdx - totIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx - totIdx] - inLow[ShadowLongTrailingIdx - totIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx - totIdx] - inLow[ShadowLongTrailingIdx - totIdx]) - Math.abs(inClose[ShadowLongTrailingIdx - totIdx] - inOpen[ShadowLongTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            FarPeriodTotal[totIdx] = FarPeriodTotal[totIdx] + (((Far_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Far_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Far_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Far_rangeType == 0) ? (Math.abs(inClose[FarTrailingIdx - totIdx] - inOpen[FarTrailingIdx - totIdx])) : ((Far_rangeType == 1) ? (inHigh[FarTrailingIdx - totIdx] - inLow[FarTrailingIdx - totIdx]) : ((Far_rangeType == 2) ? ((inHigh[FarTrailingIdx - totIdx] - inLow[FarTrailingIdx - totIdx]) - Math.abs(inClose[FarTrailingIdx - totIdx] - inOpen[FarTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) - Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 2] - inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 2] - inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 2] - inLow[BodyLongTrailingIdx - 2]) - Math.abs(inClose[BodyLongTrailingIdx - 2] - inOpen[BodyLongTrailingIdx - 2])) : 0.0)));
         i += 1;
         ShadowShortTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         NearTrailingIdx += 1;
         FarTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlAdvanceBlockUnguarded( int startIdx,
                                            int endIdx,
                                            double inOpen[],
                                            double inHigh[],
                                            double inLow[],
                                            double inClose[],
                                            MInteger outBegIdx,
                                            MInteger outNBElement,
                                            int outInteger[] )
   {
      double[] ShadowShortPeriodTotal = new double[3];
      double[] ShadowLongPeriodTotal = new double[2];
      double[] NearPeriodTotal = new double[3];
      double[] FarPeriodTotal = new double[3];
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int ShadowShortTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int FarTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Far_rangeType = this.candleSettings[CandleSettingType.Far.ordinal()].rangeType.ordinal();
      int Far_avgPeriod = this.candleSettings[CandleSettingType.Far.ordinal()].avgPeriod;
      double Far_factor = this.candleSettings[CandleSettingType.Far.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      lookbackTotal = cdlAdvanceBlockLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      ShadowShortPeriodTotal[2] = 0;
      ShadowShortPeriodTotal[1] = 0;
      ShadowShortPeriodTotal[0] = 0;
      ShadowShortTrailingIdx = startIdx - ShadowShort_avgPeriod;
      ShadowLongPeriodTotal[1] = 0;
      ShadowLongPeriodTotal[0] = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      NearPeriodTotal[2] = 0;
      NearPeriodTotal[1] = 0;
      NearPeriodTotal[0] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      FarPeriodTotal[2] = 0;
      FarPeriodTotal[1] = 0;
      FarPeriodTotal[0] = 0;
      FarTrailingIdx = startIdx - Far_avgPeriod;
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = ShadowShortTrailingIdx;
      while( i < startIdx ) {
         ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = FarTrailingIdx;
      while( i < startIdx ) {
         FarPeriodTotal[2] = FarPeriodTotal[2] + ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         FarPeriodTotal[1] = FarPeriodTotal[1] + ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inClose[i] > inClose[i - 1] && inClose[i - 1] > inClose[i - 2] && inOpen[i - 1] > inOpen[i - 2] && inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && inOpen[i] > inOpen[i - 1] && inOpen[i] <= inClose[i - 1] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) && ((inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) || (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowShortPeriodTotal[totIdx] = ShadowShortPeriodTotal[totIdx] + (((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - totIdx] - inOpen[ShadowShortTrailingIdx - totIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowShortTrailingIdx - totIdx] - inLow[ShadowShortTrailingIdx - totIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowShortTrailingIdx - totIdx] - inLow[ShadowShortTrailingIdx - totIdx]) - Math.abs(inClose[ShadowShortTrailingIdx - totIdx] - inOpen[ShadowShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            ShadowLongPeriodTotal[totIdx] = ShadowLongPeriodTotal[totIdx] + (((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - totIdx] - inOpen[ShadowLongTrailingIdx - totIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx - totIdx] - inLow[ShadowLongTrailingIdx - totIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx - totIdx] - inLow[ShadowLongTrailingIdx - totIdx]) - Math.abs(inClose[ShadowLongTrailingIdx - totIdx] - inOpen[ShadowLongTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            FarPeriodTotal[totIdx] = FarPeriodTotal[totIdx] + (((Far_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Far_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Far_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Far_rangeType == 0) ? (Math.abs(inClose[FarTrailingIdx - totIdx] - inOpen[FarTrailingIdx - totIdx])) : ((Far_rangeType == 1) ? (inHigh[FarTrailingIdx - totIdx] - inLow[FarTrailingIdx - totIdx]) : ((Far_rangeType == 2) ? ((inHigh[FarTrailingIdx - totIdx] - inLow[FarTrailingIdx - totIdx]) - Math.abs(inClose[FarTrailingIdx - totIdx] - inOpen[FarTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) - Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 2] - inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 2] - inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 2] - inLow[BodyLongTrailingIdx - 2]) - Math.abs(inClose[BodyLongTrailingIdx - 2] - inOpen[BodyLongTrailingIdx - 2])) : 0.0)));
         i += 1;
         ShadowShortTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         NearTrailingIdx += 1;
         FarTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlAdvanceBlock( int startIdx,
                                   int endIdx,
                                   float inOpen[],
                                   float inHigh[],
                                   float inLow[],
                                   float inClose[],
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   int outInteger[] )
   {
      double[] ShadowShortPeriodTotal = new double[3];
      double[] ShadowLongPeriodTotal = new double[2];
      double[] NearPeriodTotal = new double[3];
      double[] FarPeriodTotal = new double[3];
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int ShadowShortTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int FarTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Far_rangeType = this.candleSettings[CandleSettingType.Far.ordinal()].rangeType.ordinal();
      int Far_avgPeriod = this.candleSettings[CandleSettingType.Far.ordinal()].avgPeriod;
      double Far_factor = this.candleSettings[CandleSettingType.Far.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlAdvanceBlockLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      ShadowShortPeriodTotal[2] = 0;
      ShadowShortPeriodTotal[1] = 0;
      ShadowShortPeriodTotal[0] = 0;
      ShadowShortTrailingIdx = startIdx - ShadowShort_avgPeriod;
      ShadowLongPeriodTotal[1] = 0;
      ShadowLongPeriodTotal[0] = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      NearPeriodTotal[2] = 0;
      NearPeriodTotal[1] = 0;
      NearPeriodTotal[0] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      FarPeriodTotal[2] = 0;
      FarPeriodTotal[1] = 0;
      FarPeriodTotal[0] = 0;
      FarTrailingIdx = startIdx - Far_avgPeriod;
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = ShadowShortTrailingIdx;
      while( i < startIdx ) {
         ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = FarTrailingIdx;
      while( i < startIdx ) {
         FarPeriodTotal[2] = FarPeriodTotal[2] + ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         FarPeriodTotal[1] = FarPeriodTotal[1] + ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inClose[i] > (double)inClose[i - 1] && (double)inClose[i - 1] > (double)inClose[i - 2] && (double)inOpen[i - 1] > (double)inOpen[i - 2] && (double)inOpen[i - 1] <= (double)inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] > (double)inOpen[i - 1] && (double)inOpen[i] <= (double)inClose[i - 1] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i - 2] - (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inClose[i - 2] : (double)inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) && (((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) || ((double)inHigh[i - 1] - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inClose[i - 1] : (double)inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowShortPeriodTotal[totIdx] = ShadowShortPeriodTotal[totIdx] + (((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowShortTrailingIdx - totIdx] - (double)inOpen[ShadowShortTrailingIdx - totIdx])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[ShadowShortTrailingIdx - totIdx] - (double)inLow[ShadowShortTrailingIdx - totIdx]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[ShadowShortTrailingIdx - totIdx] - (double)inLow[ShadowShortTrailingIdx - totIdx]) - Math.abs((double)inClose[ShadowShortTrailingIdx - totIdx] - (double)inOpen[ShadowShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            ShadowLongPeriodTotal[totIdx] = ShadowLongPeriodTotal[totIdx] + (((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowLongTrailingIdx - totIdx] - (double)inOpen[ShadowLongTrailingIdx - totIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[ShadowLongTrailingIdx - totIdx] - (double)inLow[ShadowLongTrailingIdx - totIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[ShadowLongTrailingIdx - totIdx] - (double)inLow[ShadowLongTrailingIdx - totIdx]) - Math.abs((double)inClose[ShadowLongTrailingIdx - totIdx] - (double)inOpen[ShadowLongTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            FarPeriodTotal[totIdx] = FarPeriodTotal[totIdx] + (((Far_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Far_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Far_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Far_rangeType == 0) ? (Math.abs((double)inClose[FarTrailingIdx - totIdx] - (double)inOpen[FarTrailingIdx - totIdx])) : ((Far_rangeType == 1) ? ((double)inHigh[FarTrailingIdx - totIdx] - (double)inLow[FarTrailingIdx - totIdx]) : ((Far_rangeType == 2) ? (((double)inHigh[FarTrailingIdx - totIdx] - (double)inLow[FarTrailingIdx - totIdx]) - Math.abs((double)inClose[FarTrailingIdx - totIdx] - (double)inOpen[FarTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) - Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 2] - (double)inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 2] - (double)inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 2] - (double)inLow[BodyLongTrailingIdx - 2]) - Math.abs((double)inClose[BodyLongTrailingIdx - 2] - (double)inOpen[BodyLongTrailingIdx - 2])) : 0.0)));
         i += 1;
         ShadowShortTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         NearTrailingIdx += 1;
         FarTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlAdvanceBlockUnguarded( int startIdx,
                                            int endIdx,
                                            float inOpen[],
                                            float inHigh[],
                                            float inLow[],
                                            float inClose[],
                                            MInteger outBegIdx,
                                            MInteger outNBElement,
                                            int outInteger[] )
   {
      double[] ShadowShortPeriodTotal = new double[3];
      double[] ShadowLongPeriodTotal = new double[2];
      double[] NearPeriodTotal = new double[3];
      double[] FarPeriodTotal = new double[3];
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int BodyLongTrailingIdx = 0;
      int ShadowShortTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int FarTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int Far_rangeType = this.candleSettings[CandleSettingType.Far.ordinal()].rangeType.ordinal();
      int Far_avgPeriod = this.candleSettings[CandleSettingType.Far.ordinal()].avgPeriod;
      double Far_factor = this.candleSettings[CandleSettingType.Far.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.ShadowLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      lookbackTotal = cdlAdvanceBlockLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      ShadowShortPeriodTotal[2] = 0;
      ShadowShortPeriodTotal[1] = 0;
      ShadowShortPeriodTotal[0] = 0;
      ShadowShortTrailingIdx = startIdx - ShadowShort_avgPeriod;
      ShadowLongPeriodTotal[1] = 0;
      ShadowLongPeriodTotal[0] = 0;
      ShadowLongTrailingIdx = startIdx - ShadowLong_avgPeriod;
      NearPeriodTotal[2] = 0;
      NearPeriodTotal[1] = 0;
      NearPeriodTotal[0] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      FarPeriodTotal[2] = 0;
      FarPeriodTotal[1] = 0;
      FarPeriodTotal[0] = 0;
      FarTrailingIdx = startIdx - Far_avgPeriod;
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = ShadowShortTrailingIdx;
      while( i < startIdx ) {
         ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = FarTrailingIdx;
      while( i < startIdx ) {
         FarPeriodTotal[2] = FarPeriodTotal[2] + ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         FarPeriodTotal[1] = FarPeriodTotal[1] + ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inClose[i] > (double)inClose[i - 1] && (double)inClose[i - 1] > (double)inClose[i - 2] && (double)inOpen[i - 1] > (double)inOpen[i - 2] && (double)inOpen[i - 1] <= (double)inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] > (double)inOpen[i - 1] && (double)inOpen[i] <= (double)inClose[i - 1] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i - 2] - (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inClose[i - 2] : (double)inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) && (((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) || ((double)inHigh[i - 1] - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inClose[i - 1] : (double)inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowShortPeriodTotal[totIdx] = ShadowShortPeriodTotal[totIdx] + (((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowShortTrailingIdx - totIdx] - (double)inOpen[ShadowShortTrailingIdx - totIdx])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[ShadowShortTrailingIdx - totIdx] - (double)inLow[ShadowShortTrailingIdx - totIdx]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[ShadowShortTrailingIdx - totIdx] - (double)inLow[ShadowShortTrailingIdx - totIdx]) - Math.abs((double)inClose[ShadowShortTrailingIdx - totIdx] - (double)inOpen[ShadowShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            ShadowLongPeriodTotal[totIdx] = ShadowLongPeriodTotal[totIdx] + (((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowLongTrailingIdx - totIdx] - (double)inOpen[ShadowLongTrailingIdx - totIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[ShadowLongTrailingIdx - totIdx] - (double)inLow[ShadowLongTrailingIdx - totIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[ShadowLongTrailingIdx - totIdx] - (double)inLow[ShadowLongTrailingIdx - totIdx]) - Math.abs((double)inClose[ShadowLongTrailingIdx - totIdx] - (double)inOpen[ShadowLongTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            FarPeriodTotal[totIdx] = FarPeriodTotal[totIdx] + (((Far_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Far_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Far_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Far_rangeType == 0) ? (Math.abs((double)inClose[FarTrailingIdx - totIdx] - (double)inOpen[FarTrailingIdx - totIdx])) : ((Far_rangeType == 1) ? ((double)inHigh[FarTrailingIdx - totIdx] - (double)inLow[FarTrailingIdx - totIdx]) : ((Far_rangeType == 2) ? (((double)inHigh[FarTrailingIdx - totIdx] - (double)inLow[FarTrailingIdx - totIdx]) - Math.abs((double)inClose[FarTrailingIdx - totIdx] - (double)inOpen[FarTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) - Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 2] - (double)inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 2] - (double)inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 2] - (double)inLow[BodyLongTrailingIdx - 2]) - Math.abs((double)inClose[BodyLongTrailingIdx - 2] - (double)inOpen[BodyLongTrailingIdx - 2])) : 0.0)));
         i += 1;
         ShadowShortTrailingIdx += 1;
         ShadowLongTrailingIdx += 1;
         NearTrailingIdx += 1;
         FarTrailingIdx += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
