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
/**** Streaming API *****/

   /**
    * A live CDLADVANCEBLOCK stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlAdvanceBlock} over the same series.
    * Open with {@link Core#cdlAdvanceBlockOpen}; there is no close — the handle is
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
   public static final class CdlAdvanceBlockStream {
      final Core core;
      double[] ShadowShortPeriodTotal;
      double[] ShadowLongPeriodTotal;
      double[] NearPeriodTotal;
      double[] FarPeriodTotal;
      double BodyLongPeriodTotal;
      int totIdx;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag1_inClose;
      double lag2_inClose;
      int ringPos_BodyLongTrailingIdx;
      int ringCap_BodyLongTrailingIdx;
      int ringLag_BodyLongTrailingIdx;
      double[] ring_BodyLongTrailingIdx_inOpen;
      double[] ring_BodyLongTrailingIdx_inHigh;
      double[] ring_BodyLongTrailingIdx_inLow;
      double[] ring_BodyLongTrailingIdx_inClose;
      int ringPos_FarTrailingIdx;
      int ringCap_FarTrailingIdx;
      int ringLag_FarTrailingIdx;
      double[] ring_FarTrailingIdx_inOpen;
      double[] ring_FarTrailingIdx_inHigh;
      double[] ring_FarTrailingIdx_inLow;
      double[] ring_FarTrailingIdx_inClose;
      int ringPos_NearTrailingIdx;
      int ringCap_NearTrailingIdx;
      int ringLag_NearTrailingIdx;
      double[] ring_NearTrailingIdx_inOpen;
      double[] ring_NearTrailingIdx_inHigh;
      double[] ring_NearTrailingIdx_inLow;
      double[] ring_NearTrailingIdx_inClose;
      int ringPos_ShadowLongTrailingIdx;
      int ringCap_ShadowLongTrailingIdx;
      int ringLag_ShadowLongTrailingIdx;
      double[] ring_ShadowLongTrailingIdx_inOpen;
      double[] ring_ShadowLongTrailingIdx_inHigh;
      double[] ring_ShadowLongTrailingIdx_inLow;
      double[] ring_ShadowLongTrailingIdx_inClose;
      int ringPos_ShadowShortTrailingIdx;
      int ringCap_ShadowShortTrailingIdx;
      int ringLag_ShadowShortTrailingIdx;
      double[] ring_ShadowShortTrailingIdx_inOpen;
      double[] ring_ShadowShortTrailingIdx_inHigh;
      double[] ring_ShadowShortTrailingIdx_inLow;
      double[] ring_ShadowShortTrailingIdx_inClose;
      int winPos_totIdx;
      int winCap_totIdx;
      double[] win_totIdx_inOpen;
      double[] win_totIdx_inHigh;
      double[] win_totIdx_inLow;
      double[] win_totIdx_inClose;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cs_Far_rangeType;
      int cs_Far_avgPeriod;
      double cs_Far_factor;
      int cs_Near_rangeType;
      int cs_Near_avgPeriod;
      double cs_Near_factor;
      int cs_ShadowLong_rangeType;
      int cs_ShadowLong_avgPeriod;
      double cs_ShadowLong_factor;
      int cs_ShadowShort_rangeType;
      int cs_ShadowShort_avgPeriod;
      double cs_ShadowShort_factor;
      int cur_outInteger;

      CdlAdvanceBlockStream( Core core ) { this.core = core; }

      CdlAdvanceBlockStream( CdlAdvanceBlockStream other ) {
         this.core = other.core;
         this.ShadowShortPeriodTotal = other.ShadowShortPeriodTotal.clone();
         this.ShadowLongPeriodTotal = other.ShadowLongPeriodTotal.clone();
         this.NearPeriodTotal = other.NearPeriodTotal.clone();
         this.FarPeriodTotal = other.FarPeriodTotal.clone();
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.totIdx = other.totIdx;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ringLag_BodyLongTrailingIdx = other.ringLag_BodyLongTrailingIdx;
         this.ring_BodyLongTrailingIdx_inOpen = other.ring_BodyLongTrailingIdx_inOpen.clone();
         this.ring_BodyLongTrailingIdx_inHigh = other.ring_BodyLongTrailingIdx_inHigh.clone();
         this.ring_BodyLongTrailingIdx_inLow = other.ring_BodyLongTrailingIdx_inLow.clone();
         this.ring_BodyLongTrailingIdx_inClose = other.ring_BodyLongTrailingIdx_inClose.clone();
         this.ringPos_FarTrailingIdx = other.ringPos_FarTrailingIdx;
         this.ringCap_FarTrailingIdx = other.ringCap_FarTrailingIdx;
         this.ringLag_FarTrailingIdx = other.ringLag_FarTrailingIdx;
         this.ring_FarTrailingIdx_inOpen = other.ring_FarTrailingIdx_inOpen.clone();
         this.ring_FarTrailingIdx_inHigh = other.ring_FarTrailingIdx_inHigh.clone();
         this.ring_FarTrailingIdx_inLow = other.ring_FarTrailingIdx_inLow.clone();
         this.ring_FarTrailingIdx_inClose = other.ring_FarTrailingIdx_inClose.clone();
         this.ringPos_NearTrailingIdx = other.ringPos_NearTrailingIdx;
         this.ringCap_NearTrailingIdx = other.ringCap_NearTrailingIdx;
         this.ringLag_NearTrailingIdx = other.ringLag_NearTrailingIdx;
         this.ring_NearTrailingIdx_inOpen = other.ring_NearTrailingIdx_inOpen.clone();
         this.ring_NearTrailingIdx_inHigh = other.ring_NearTrailingIdx_inHigh.clone();
         this.ring_NearTrailingIdx_inLow = other.ring_NearTrailingIdx_inLow.clone();
         this.ring_NearTrailingIdx_inClose = other.ring_NearTrailingIdx_inClose.clone();
         this.ringPos_ShadowLongTrailingIdx = other.ringPos_ShadowLongTrailingIdx;
         this.ringCap_ShadowLongTrailingIdx = other.ringCap_ShadowLongTrailingIdx;
         this.ringLag_ShadowLongTrailingIdx = other.ringLag_ShadowLongTrailingIdx;
         this.ring_ShadowLongTrailingIdx_inOpen = other.ring_ShadowLongTrailingIdx_inOpen.clone();
         this.ring_ShadowLongTrailingIdx_inHigh = other.ring_ShadowLongTrailingIdx_inHigh.clone();
         this.ring_ShadowLongTrailingIdx_inLow = other.ring_ShadowLongTrailingIdx_inLow.clone();
         this.ring_ShadowLongTrailingIdx_inClose = other.ring_ShadowLongTrailingIdx_inClose.clone();
         this.ringPos_ShadowShortTrailingIdx = other.ringPos_ShadowShortTrailingIdx;
         this.ringCap_ShadowShortTrailingIdx = other.ringCap_ShadowShortTrailingIdx;
         this.ringLag_ShadowShortTrailingIdx = other.ringLag_ShadowShortTrailingIdx;
         this.ring_ShadowShortTrailingIdx_inOpen = other.ring_ShadowShortTrailingIdx_inOpen.clone();
         this.ring_ShadowShortTrailingIdx_inHigh = other.ring_ShadowShortTrailingIdx_inHigh.clone();
         this.ring_ShadowShortTrailingIdx_inLow = other.ring_ShadowShortTrailingIdx_inLow.clone();
         this.ring_ShadowShortTrailingIdx_inClose = other.ring_ShadowShortTrailingIdx_inClose.clone();
         this.winPos_totIdx = other.winPos_totIdx;
         this.winCap_totIdx = other.winCap_totIdx;
         this.win_totIdx_inOpen = other.win_totIdx_inOpen.clone();
         this.win_totIdx_inHigh = other.win_totIdx_inHigh.clone();
         this.win_totIdx_inLow = other.win_totIdx_inLow.clone();
         this.win_totIdx_inClose = other.win_totIdx_inClose.clone();
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_Far_rangeType = other.cs_Far_rangeType;
         this.cs_Far_avgPeriod = other.cs_Far_avgPeriod;
         this.cs_Far_factor = other.cs_Far_factor;
         this.cs_Near_rangeType = other.cs_Near_rangeType;
         this.cs_Near_avgPeriod = other.cs_Near_avgPeriod;
         this.cs_Near_factor = other.cs_Near_factor;
         this.cs_ShadowLong_rangeType = other.cs_ShadowLong_rangeType;
         this.cs_ShadowLong_avgPeriod = other.cs_ShadowLong_avgPeriod;
         this.cs_ShadowLong_factor = other.cs_ShadowLong_factor;
         this.cs_ShadowShort_rangeType = other.cs_ShadowShort_rangeType;
         this.cs_ShadowShort_avgPeriod = other.cs_ShadowShort_avgPeriod;
         this.cs_ShadowShort_factor = other.cs_ShadowShort_factor;
         this.cur_outInteger = other.cur_outInteger;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.cdlAdvanceBlockStreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CdlAdvanceBlockStream scratch = new CdlAdvanceBlockStream(this);
         core.cdlAdvanceBlockStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CdlAdvanceBlockStream copy() {
         return new CdlAdvanceBlockStream(this);
      }
   }
   void cdlAdvanceBlockStreamStep( CdlAdvanceBlockStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      int Far_rangeType = sp.cs_Far_rangeType;
      int Far_avgPeriod = sp.cs_Far_avgPeriod;
      double Far_factor = sp.cs_Far_factor;
      int Near_rangeType = sp.cs_Near_rangeType;
      int Near_avgPeriod = sp.cs_Near_avgPeriod;
      double Near_factor = sp.cs_Near_factor;
      int ShadowLong_rangeType = sp.cs_ShadowLong_rangeType;
      int ShadowLong_avgPeriod = sp.cs_ShadowLong_avgPeriod;
      double ShadowLong_factor = sp.cs_ShadowLong_factor;
      int ShadowShort_rangeType = sp.cs_ShadowShort_rangeType;
      int ShadowShort_avgPeriod = sp.cs_ShadowShort_avgPeriod;
      double ShadowShort_factor = sp.cs_ShadowShort_factor;
      sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx] = inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] = inHigh;
      sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx] = inLow;
      sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] = inClose;
      sp.ring_FarTrailingIdx_inOpen[sp.ringPos_FarTrailingIdx] = inOpen;
      sp.ring_FarTrailingIdx_inHigh[sp.ringPos_FarTrailingIdx] = inHigh;
      sp.ring_FarTrailingIdx_inLow[sp.ringPos_FarTrailingIdx] = inLow;
      sp.ring_FarTrailingIdx_inClose[sp.ringPos_FarTrailingIdx] = inClose;
      sp.ring_NearTrailingIdx_inOpen[sp.ringPos_NearTrailingIdx] = inOpen;
      sp.ring_NearTrailingIdx_inHigh[sp.ringPos_NearTrailingIdx] = inHigh;
      sp.ring_NearTrailingIdx_inLow[sp.ringPos_NearTrailingIdx] = inLow;
      sp.ring_NearTrailingIdx_inClose[sp.ringPos_NearTrailingIdx] = inClose;
      sp.ring_ShadowLongTrailingIdx_inOpen[sp.ringPos_ShadowLongTrailingIdx] = inOpen;
      sp.ring_ShadowLongTrailingIdx_inHigh[sp.ringPos_ShadowLongTrailingIdx] = inHigh;
      sp.ring_ShadowLongTrailingIdx_inLow[sp.ringPos_ShadowLongTrailingIdx] = inLow;
      sp.ring_ShadowLongTrailingIdx_inClose[sp.ringPos_ShadowLongTrailingIdx] = inClose;
      sp.ring_ShadowShortTrailingIdx_inOpen[sp.ringPos_ShadowShortTrailingIdx] = inOpen;
      sp.ring_ShadowShortTrailingIdx_inHigh[sp.ringPos_ShadowShortTrailingIdx] = inHigh;
      sp.ring_ShadowShortTrailingIdx_inLow[sp.ringPos_ShadowShortTrailingIdx] = inLow;
      sp.ring_ShadowShortTrailingIdx_inClose[sp.ringPos_ShadowShortTrailingIdx] = inClose;
      sp.win_totIdx_inOpen[sp.winPos_totIdx] = inOpen;
      sp.win_totIdx_inHigh[sp.winPos_totIdx] = inHigh;
      sp.win_totIdx_inLow[sp.winPos_totIdx] = inLow;
      sp.win_totIdx_inClose[sp.winPos_totIdx] = inClose;
      if( ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 1 && /* 1st white */
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && /* 2nd white */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 1 &&                 /* 3rd white */
          inClose > sp.lag1_inClose &&
          sp.lag1_inClose > sp.lag2_inClose &&                      /* consecutive higher closes */
          sp.lag1_inOpen > sp.lag2_inOpen &&                        /* 2nd opens within/near 1st real body */
          sp.lag1_inOpen <= sp.lag2_inClose + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          inOpen > sp.lag1_inOpen &&                                /* 3rd opens within/near 2nd real body */
          inOpen <= sp.lag1_inClose + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
          (sp.lag2_inHigh - ((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inClose : sp.lag2_inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowShort_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowShort_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          (Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) - ((Far_factor * (((Far_avgPeriod != 0) ? (sp.FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Far_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Far_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Far_factor * (((Far_avgPeriod != 0) ? (sp.FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Far_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Far_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) && ((inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) || (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowShort_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowShort_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) || Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (sp.ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) ) /* 1st: short upper shadow ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance 3 far smaller than 2 advance blocked with the 3rd ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
      {
         sp.cur_outInteger = 0 - 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for( sp.totIdx = 2; sp.totIdx >= 0; sp.totIdx -= 1 ) {
         sp.ShadowShortPeriodTotal[sp.totIdx] = sp.ShadowShortPeriodTotal[sp.totIdx] + (((ShadowShort_rangeType == 0) ? (Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : ((ShadowShort_rangeType == 1) ? (sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) : ((ShadowShort_rangeType == 2) ? ((sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) - Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(sp.ring_ShadowShortTrailingIdx_inClose[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - sp.ringLag_ShadowShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowShortTrailingIdx] - sp.ring_ShadowShortTrailingIdx_inOpen[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - sp.ringLag_ShadowShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowShortTrailingIdx])) : ((ShadowShort_rangeType == 1) ? (sp.ring_ShadowShortTrailingIdx_inHigh[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - sp.ringLag_ShadowShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowShortTrailingIdx] - sp.ring_ShadowShortTrailingIdx_inLow[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - sp.ringLag_ShadowShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowShortTrailingIdx]) : ((ShadowShort_rangeType == 2) ? ((sp.ring_ShadowShortTrailingIdx_inHigh[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - sp.ringLag_ShadowShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowShortTrailingIdx] - sp.ring_ShadowShortTrailingIdx_inLow[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - sp.ringLag_ShadowShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowShortTrailingIdx]) - Math.abs(sp.ring_ShadowShortTrailingIdx_inClose[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - sp.ringLag_ShadowShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowShortTrailingIdx] - sp.ring_ShadowShortTrailingIdx_inOpen[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - sp.ringLag_ShadowShortTrailingIdx - sp.totIdx) % sp.ringCap_ShadowShortTrailingIdx])) : 0.0))));
      }
      for( sp.totIdx = 1; sp.totIdx >= 0; sp.totIdx -= 1 ) {
         sp.ShadowLongPeriodTotal[sp.totIdx] = sp.ShadowLongPeriodTotal[sp.totIdx] + (((ShadowLong_rangeType == 0) ? (Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : ((ShadowLong_rangeType == 1) ? (sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) : ((ShadowLong_rangeType == 2) ? ((sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) - Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(sp.ring_ShadowLongTrailingIdx_inClose[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - sp.totIdx) % sp.ringCap_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inOpen[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - sp.totIdx) % sp.ringCap_ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (sp.ring_ShadowLongTrailingIdx_inHigh[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - sp.totIdx) % sp.ringCap_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inLow[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - sp.totIdx) % sp.ringCap_ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((sp.ring_ShadowLongTrailingIdx_inHigh[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - sp.totIdx) % sp.ringCap_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inLow[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - sp.totIdx) % sp.ringCap_ShadowLongTrailingIdx]) - Math.abs(sp.ring_ShadowLongTrailingIdx_inClose[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - sp.totIdx) % sp.ringCap_ShadowLongTrailingIdx] - sp.ring_ShadowLongTrailingIdx_inOpen[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - sp.totIdx) % sp.ringCap_ShadowLongTrailingIdx])) : 0.0))));
      }
      for( sp.totIdx = 2; sp.totIdx >= 1; sp.totIdx -= 1 ) {
         sp.FarPeriodTotal[sp.totIdx] = sp.FarPeriodTotal[sp.totIdx] + (((Far_rangeType == 0) ? (Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : ((Far_rangeType == 1) ? (sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) : ((Far_rangeType == 2) ? ((sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) - Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : 0.0))) - ((Far_rangeType == 0) ? (Math.abs(sp.ring_FarTrailingIdx_inClose[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - sp.totIdx) % sp.ringCap_FarTrailingIdx] - sp.ring_FarTrailingIdx_inOpen[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - sp.totIdx) % sp.ringCap_FarTrailingIdx])) : ((Far_rangeType == 1) ? (sp.ring_FarTrailingIdx_inHigh[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - sp.totIdx) % sp.ringCap_FarTrailingIdx] - sp.ring_FarTrailingIdx_inLow[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - sp.totIdx) % sp.ringCap_FarTrailingIdx]) : ((Far_rangeType == 2) ? ((sp.ring_FarTrailingIdx_inHigh[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - sp.totIdx) % sp.ringCap_FarTrailingIdx] - sp.ring_FarTrailingIdx_inLow[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - sp.totIdx) % sp.ringCap_FarTrailingIdx]) - Math.abs(sp.ring_FarTrailingIdx_inClose[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - sp.totIdx) % sp.ringCap_FarTrailingIdx] - sp.ring_FarTrailingIdx_inOpen[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - sp.totIdx) % sp.ringCap_FarTrailingIdx])) : 0.0))));
         sp.NearPeriodTotal[sp.totIdx] = sp.NearPeriodTotal[sp.totIdx] + (((Near_rangeType == 0) ? (Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : ((Near_rangeType == 1) ? (sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) : ((Near_rangeType == 2) ? ((sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) - Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(sp.ring_NearTrailingIdx_inClose[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inOpen[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx])) : ((Near_rangeType == 1) ? (sp.ring_NearTrailingIdx_inHigh[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inLow[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx]) : ((Near_rangeType == 2) ? ((sp.ring_NearTrailingIdx_inHigh[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inLow[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx]) - Math.abs(sp.ring_NearTrailingIdx_inClose[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inOpen[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx])) : 0.0))));
      }
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(sp.ring_BodyLongTrailingIdx_inClose[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (sp.ring_BodyLongTrailingIdx_inHigh[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((sp.ring_BodyLongTrailingIdx_inHigh[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx]) - Math.abs(sp.ring_BodyLongTrailingIdx_inClose[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx])) : 0.0)));
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
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
      sp.ring_FarTrailingIdx_inOpen[sp.ringPos_FarTrailingIdx] = inOpen;
      sp.ring_FarTrailingIdx_inHigh[sp.ringPos_FarTrailingIdx] = inHigh;
      sp.ring_FarTrailingIdx_inLow[sp.ringPos_FarTrailingIdx] = inLow;
      sp.ring_FarTrailingIdx_inClose[sp.ringPos_FarTrailingIdx] = inClose;
      sp.ringPos_FarTrailingIdx = sp.ringPos_FarTrailingIdx + 1;
      if( sp.ringPos_FarTrailingIdx >= sp.ringCap_FarTrailingIdx ) {
         sp.ringPos_FarTrailingIdx = 0;
      }
      sp.ring_NearTrailingIdx_inOpen[sp.ringPos_NearTrailingIdx] = inOpen;
      sp.ring_NearTrailingIdx_inHigh[sp.ringPos_NearTrailingIdx] = inHigh;
      sp.ring_NearTrailingIdx_inLow[sp.ringPos_NearTrailingIdx] = inLow;
      sp.ring_NearTrailingIdx_inClose[sp.ringPos_NearTrailingIdx] = inClose;
      sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
      if( sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx ) {
         sp.ringPos_NearTrailingIdx = 0;
      }
      sp.ring_ShadowLongTrailingIdx_inOpen[sp.ringPos_ShadowLongTrailingIdx] = inOpen;
      sp.ring_ShadowLongTrailingIdx_inHigh[sp.ringPos_ShadowLongTrailingIdx] = inHigh;
      sp.ring_ShadowLongTrailingIdx_inLow[sp.ringPos_ShadowLongTrailingIdx] = inLow;
      sp.ring_ShadowLongTrailingIdx_inClose[sp.ringPos_ShadowLongTrailingIdx] = inClose;
      sp.ringPos_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx + 1;
      if( sp.ringPos_ShadowLongTrailingIdx >= sp.ringCap_ShadowLongTrailingIdx ) {
         sp.ringPos_ShadowLongTrailingIdx = 0;
      }
      sp.ring_ShadowShortTrailingIdx_inOpen[sp.ringPos_ShadowShortTrailingIdx] = inOpen;
      sp.ring_ShadowShortTrailingIdx_inHigh[sp.ringPos_ShadowShortTrailingIdx] = inHigh;
      sp.ring_ShadowShortTrailingIdx_inLow[sp.ringPos_ShadowShortTrailingIdx] = inLow;
      sp.ring_ShadowShortTrailingIdx_inClose[sp.ringPos_ShadowShortTrailingIdx] = inClose;
      sp.ringPos_ShadowShortTrailingIdx = sp.ringPos_ShadowShortTrailingIdx + 1;
      if( sp.ringPos_ShadowShortTrailingIdx >= sp.ringCap_ShadowShortTrailingIdx ) {
         sp.ringPos_ShadowShortTrailingIdx = 0;
      }
      sp.winPos_totIdx = sp.winPos_totIdx + 1;
      if( sp.winPos_totIdx >= sp.winCap_totIdx ) {
         sp.winPos_totIdx = 0;
      }
   }
   private RetCode cdlAdvanceBlockOpenBody( CdlAdvanceBlockStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
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
         return RetCode.OutOfRangeEndIndex ;
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
            lastValue_outInteger = 0 - 100;
         } else {
            lastValue_outInteger = 0;
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
      /* Capture the live batch state into the handle. */
      int capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      int cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 3;
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
      int capLag_FarTrailingIdx = i - FarTrailingIdx;
      int cap_FarTrailingIdx = capLag_FarTrailingIdx + 3;
      if( capLag_FarTrailingIdx < 0 || cap_FarTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_FarTrailingIdx = (cap_FarTrailingIdx > 0)? cap_FarTrailingIdx : 1;
      double[] capRing_FarTrailingIdx_inOpen = new double[allocN_FarTrailingIdx];
      for( int fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_FarTrailingIdx_inOpen[fillJ % cap_FarTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_FarTrailingIdx_inHigh = new double[allocN_FarTrailingIdx];
      for( int fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_FarTrailingIdx_inHigh[fillJ % cap_FarTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_FarTrailingIdx_inLow = new double[allocN_FarTrailingIdx];
      for( int fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_FarTrailingIdx_inLow[fillJ % cap_FarTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_FarTrailingIdx_inClose = new double[allocN_FarTrailingIdx];
      for( int fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_FarTrailingIdx_inClose[fillJ % cap_FarTrailingIdx] = inClose[fillJ];
      }
      int capLag_NearTrailingIdx = i - NearTrailingIdx;
      int cap_NearTrailingIdx = capLag_NearTrailingIdx + 3;
      if( capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_NearTrailingIdx = (cap_NearTrailingIdx > 0)? cap_NearTrailingIdx : 1;
      double[] capRing_NearTrailingIdx_inOpen = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_inOpen[fillJ % cap_NearTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_NearTrailingIdx_inHigh = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_inHigh[fillJ % cap_NearTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_NearTrailingIdx_inLow = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_inLow[fillJ % cap_NearTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_NearTrailingIdx_inClose = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_inClose[fillJ % cap_NearTrailingIdx] = inClose[fillJ];
      }
      int capLag_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
      int cap_ShadowLongTrailingIdx = capLag_ShadowLongTrailingIdx + 3;
      if( capLag_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowLongTrailingIdx = (cap_ShadowLongTrailingIdx > 0)? cap_ShadowLongTrailingIdx : 1;
      double[] capRing_ShadowLongTrailingIdx_inOpen = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_inOpen[fillJ % cap_ShadowLongTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_ShadowLongTrailingIdx_inHigh = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_inHigh[fillJ % cap_ShadowLongTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_ShadowLongTrailingIdx_inLow = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_inLow[fillJ % cap_ShadowLongTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_ShadowLongTrailingIdx_inClose = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_inClose[fillJ % cap_ShadowLongTrailingIdx] = inClose[fillJ];
      }
      int capLag_ShadowShortTrailingIdx = i - ShadowShortTrailingIdx;
      int cap_ShadowShortTrailingIdx = capLag_ShadowShortTrailingIdx + 3;
      if( capLag_ShadowShortTrailingIdx < 0 || cap_ShadowShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowShortTrailingIdx = (cap_ShadowShortTrailingIdx > 0)? cap_ShadowShortTrailingIdx : 1;
      double[] capRing_ShadowShortTrailingIdx_inOpen = new double[allocN_ShadowShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowShortTrailingIdx_inOpen[fillJ % cap_ShadowShortTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_ShadowShortTrailingIdx_inHigh = new double[allocN_ShadowShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowShortTrailingIdx_inHigh[fillJ % cap_ShadowShortTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_ShadowShortTrailingIdx_inLow = new double[allocN_ShadowShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowShortTrailingIdx_inLow[fillJ % cap_ShadowShortTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_ShadowShortTrailingIdx_inClose = new double[allocN_ShadowShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowShortTrailingIdx_inClose[fillJ % cap_ShadowShortTrailingIdx] = inClose[fillJ];
      }
      int cap_totIdx = (int)(3);
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
      sp.ShadowShortPeriodTotal = ShadowShortPeriodTotal;
      sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
      sp.NearPeriodTotal = NearPeriodTotal;
      sp.FarPeriodTotal = FarPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.totIdx = totIdx;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_inOpen = capRing_BodyLongTrailingIdx_inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh = capRing_BodyLongTrailingIdx_inHigh;
      sp.ring_BodyLongTrailingIdx_inLow = capRing_BodyLongTrailingIdx_inLow;
      sp.ring_BodyLongTrailingIdx_inClose = capRing_BodyLongTrailingIdx_inClose;
      sp.ringPos_FarTrailingIdx = historyLen % cap_FarTrailingIdx;
      sp.ringCap_FarTrailingIdx = cap_FarTrailingIdx;
      sp.ringLag_FarTrailingIdx = capLag_FarTrailingIdx;
      sp.ring_FarTrailingIdx_inOpen = capRing_FarTrailingIdx_inOpen;
      sp.ring_FarTrailingIdx_inHigh = capRing_FarTrailingIdx_inHigh;
      sp.ring_FarTrailingIdx_inLow = capRing_FarTrailingIdx_inLow;
      sp.ring_FarTrailingIdx_inClose = capRing_FarTrailingIdx_inClose;
      sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
      sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
      sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
      sp.ring_NearTrailingIdx_inOpen = capRing_NearTrailingIdx_inOpen;
      sp.ring_NearTrailingIdx_inHigh = capRing_NearTrailingIdx_inHigh;
      sp.ring_NearTrailingIdx_inLow = capRing_NearTrailingIdx_inLow;
      sp.ring_NearTrailingIdx_inClose = capRing_NearTrailingIdx_inClose;
      sp.ringPos_ShadowLongTrailingIdx = historyLen % cap_ShadowLongTrailingIdx;
      sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
      sp.ringLag_ShadowLongTrailingIdx = capLag_ShadowLongTrailingIdx;
      sp.ring_ShadowLongTrailingIdx_inOpen = capRing_ShadowLongTrailingIdx_inOpen;
      sp.ring_ShadowLongTrailingIdx_inHigh = capRing_ShadowLongTrailingIdx_inHigh;
      sp.ring_ShadowLongTrailingIdx_inLow = capRing_ShadowLongTrailingIdx_inLow;
      sp.ring_ShadowLongTrailingIdx_inClose = capRing_ShadowLongTrailingIdx_inClose;
      sp.ringPos_ShadowShortTrailingIdx = historyLen % cap_ShadowShortTrailingIdx;
      sp.ringCap_ShadowShortTrailingIdx = cap_ShadowShortTrailingIdx;
      sp.ringLag_ShadowShortTrailingIdx = capLag_ShadowShortTrailingIdx;
      sp.ring_ShadowShortTrailingIdx_inOpen = capRing_ShadowShortTrailingIdx_inOpen;
      sp.ring_ShadowShortTrailingIdx_inHigh = capRing_ShadowShortTrailingIdx_inHigh;
      sp.ring_ShadowShortTrailingIdx_inLow = capRing_ShadowShortTrailingIdx_inLow;
      sp.ring_ShadowShortTrailingIdx_inClose = capRing_ShadowShortTrailingIdx_inClose;
      sp.winPos_totIdx = 0;
      sp.winCap_totIdx = cap_totIdx;
      sp.win_totIdx_inOpen = capWin_totIdx_inOpen;
      sp.win_totIdx_inHigh = capWin_totIdx_inHigh;
      sp.win_totIdx_inLow = capWin_totIdx_inLow;
      sp.win_totIdx_inClose = capWin_totIdx_inClose;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_Far_rangeType = Far_rangeType;
      sp.cs_Far_avgPeriod = Far_avgPeriod;
      sp.cs_Far_factor = Far_factor;
      sp.cs_Near_rangeType = Near_rangeType;
      sp.cs_Near_avgPeriod = Near_avgPeriod;
      sp.cs_Near_factor = Near_factor;
      sp.cs_ShadowLong_rangeType = ShadowLong_rangeType;
      sp.cs_ShadowLong_avgPeriod = ShadowLong_avgPeriod;
      sp.cs_ShadowLong_factor = ShadowLong_factor;
      sp.cs_ShadowShort_rangeType = ShadowShort_rangeType;
      sp.cs_ShadowShort_avgPeriod = ShadowShort_avgPeriod;
      sp.cs_ShadowShort_factor = ShadowShort_factor;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlAdvanceBlockOpenAndFillBody( CdlAdvanceBlockStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
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
         return RetCode.OutOfRangeEndIndex ;
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
      /* Capture the live batch state into the handle. */
      int capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      int cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 3;
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
      int capLag_FarTrailingIdx = i - FarTrailingIdx;
      int cap_FarTrailingIdx = capLag_FarTrailingIdx + 3;
      if( capLag_FarTrailingIdx < 0 || cap_FarTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_FarTrailingIdx = (cap_FarTrailingIdx > 0)? cap_FarTrailingIdx : 1;
      double[] capRing_FarTrailingIdx_inOpen = new double[allocN_FarTrailingIdx];
      for( int fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_FarTrailingIdx_inOpen[fillJ % cap_FarTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_FarTrailingIdx_inHigh = new double[allocN_FarTrailingIdx];
      for( int fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_FarTrailingIdx_inHigh[fillJ % cap_FarTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_FarTrailingIdx_inLow = new double[allocN_FarTrailingIdx];
      for( int fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_FarTrailingIdx_inLow[fillJ % cap_FarTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_FarTrailingIdx_inClose = new double[allocN_FarTrailingIdx];
      for( int fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_FarTrailingIdx_inClose[fillJ % cap_FarTrailingIdx] = inClose[fillJ];
      }
      int capLag_NearTrailingIdx = i - NearTrailingIdx;
      int cap_NearTrailingIdx = capLag_NearTrailingIdx + 3;
      if( capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_NearTrailingIdx = (cap_NearTrailingIdx > 0)? cap_NearTrailingIdx : 1;
      double[] capRing_NearTrailingIdx_inOpen = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_inOpen[fillJ % cap_NearTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_NearTrailingIdx_inHigh = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_inHigh[fillJ % cap_NearTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_NearTrailingIdx_inLow = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_inLow[fillJ % cap_NearTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_NearTrailingIdx_inClose = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_inClose[fillJ % cap_NearTrailingIdx] = inClose[fillJ];
      }
      int capLag_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
      int cap_ShadowLongTrailingIdx = capLag_ShadowLongTrailingIdx + 3;
      if( capLag_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowLongTrailingIdx = (cap_ShadowLongTrailingIdx > 0)? cap_ShadowLongTrailingIdx : 1;
      double[] capRing_ShadowLongTrailingIdx_inOpen = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_inOpen[fillJ % cap_ShadowLongTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_ShadowLongTrailingIdx_inHigh = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_inHigh[fillJ % cap_ShadowLongTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_ShadowLongTrailingIdx_inLow = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_inLow[fillJ % cap_ShadowLongTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_ShadowLongTrailingIdx_inClose = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_inClose[fillJ % cap_ShadowLongTrailingIdx] = inClose[fillJ];
      }
      int capLag_ShadowShortTrailingIdx = i - ShadowShortTrailingIdx;
      int cap_ShadowShortTrailingIdx = capLag_ShadowShortTrailingIdx + 3;
      if( capLag_ShadowShortTrailingIdx < 0 || cap_ShadowShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowShortTrailingIdx = (cap_ShadowShortTrailingIdx > 0)? cap_ShadowShortTrailingIdx : 1;
      double[] capRing_ShadowShortTrailingIdx_inOpen = new double[allocN_ShadowShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowShortTrailingIdx_inOpen[fillJ % cap_ShadowShortTrailingIdx] = inOpen[fillJ];
      }
      double[] capRing_ShadowShortTrailingIdx_inHigh = new double[allocN_ShadowShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowShortTrailingIdx_inHigh[fillJ % cap_ShadowShortTrailingIdx] = inHigh[fillJ];
      }
      double[] capRing_ShadowShortTrailingIdx_inLow = new double[allocN_ShadowShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowShortTrailingIdx_inLow[fillJ % cap_ShadowShortTrailingIdx] = inLow[fillJ];
      }
      double[] capRing_ShadowShortTrailingIdx_inClose = new double[allocN_ShadowShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowShortTrailingIdx_inClose[fillJ % cap_ShadowShortTrailingIdx] = inClose[fillJ];
      }
      int cap_totIdx = (int)(3);
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
      sp.ShadowShortPeriodTotal = ShadowShortPeriodTotal;
      sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
      sp.NearPeriodTotal = NearPeriodTotal;
      sp.FarPeriodTotal = FarPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.totIdx = totIdx;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_inOpen = capRing_BodyLongTrailingIdx_inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh = capRing_BodyLongTrailingIdx_inHigh;
      sp.ring_BodyLongTrailingIdx_inLow = capRing_BodyLongTrailingIdx_inLow;
      sp.ring_BodyLongTrailingIdx_inClose = capRing_BodyLongTrailingIdx_inClose;
      sp.ringPos_FarTrailingIdx = historyLen % cap_FarTrailingIdx;
      sp.ringCap_FarTrailingIdx = cap_FarTrailingIdx;
      sp.ringLag_FarTrailingIdx = capLag_FarTrailingIdx;
      sp.ring_FarTrailingIdx_inOpen = capRing_FarTrailingIdx_inOpen;
      sp.ring_FarTrailingIdx_inHigh = capRing_FarTrailingIdx_inHigh;
      sp.ring_FarTrailingIdx_inLow = capRing_FarTrailingIdx_inLow;
      sp.ring_FarTrailingIdx_inClose = capRing_FarTrailingIdx_inClose;
      sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
      sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
      sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
      sp.ring_NearTrailingIdx_inOpen = capRing_NearTrailingIdx_inOpen;
      sp.ring_NearTrailingIdx_inHigh = capRing_NearTrailingIdx_inHigh;
      sp.ring_NearTrailingIdx_inLow = capRing_NearTrailingIdx_inLow;
      sp.ring_NearTrailingIdx_inClose = capRing_NearTrailingIdx_inClose;
      sp.ringPos_ShadowLongTrailingIdx = historyLen % cap_ShadowLongTrailingIdx;
      sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
      sp.ringLag_ShadowLongTrailingIdx = capLag_ShadowLongTrailingIdx;
      sp.ring_ShadowLongTrailingIdx_inOpen = capRing_ShadowLongTrailingIdx_inOpen;
      sp.ring_ShadowLongTrailingIdx_inHigh = capRing_ShadowLongTrailingIdx_inHigh;
      sp.ring_ShadowLongTrailingIdx_inLow = capRing_ShadowLongTrailingIdx_inLow;
      sp.ring_ShadowLongTrailingIdx_inClose = capRing_ShadowLongTrailingIdx_inClose;
      sp.ringPos_ShadowShortTrailingIdx = historyLen % cap_ShadowShortTrailingIdx;
      sp.ringCap_ShadowShortTrailingIdx = cap_ShadowShortTrailingIdx;
      sp.ringLag_ShadowShortTrailingIdx = capLag_ShadowShortTrailingIdx;
      sp.ring_ShadowShortTrailingIdx_inOpen = capRing_ShadowShortTrailingIdx_inOpen;
      sp.ring_ShadowShortTrailingIdx_inHigh = capRing_ShadowShortTrailingIdx_inHigh;
      sp.ring_ShadowShortTrailingIdx_inLow = capRing_ShadowShortTrailingIdx_inLow;
      sp.ring_ShadowShortTrailingIdx_inClose = capRing_ShadowShortTrailingIdx_inClose;
      sp.winPos_totIdx = 0;
      sp.winCap_totIdx = cap_totIdx;
      sp.win_totIdx_inOpen = capWin_totIdx_inOpen;
      sp.win_totIdx_inHigh = capWin_totIdx_inHigh;
      sp.win_totIdx_inLow = capWin_totIdx_inLow;
      sp.win_totIdx_inClose = capWin_totIdx_inClose;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_Far_rangeType = Far_rangeType;
      sp.cs_Far_avgPeriod = Far_avgPeriod;
      sp.cs_Far_factor = Far_factor;
      sp.cs_Near_rangeType = Near_rangeType;
      sp.cs_Near_avgPeriod = Near_avgPeriod;
      sp.cs_Near_factor = Near_factor;
      sp.cs_ShadowLong_rangeType = ShadowLong_rangeType;
      sp.cs_ShadowLong_avgPeriod = ShadowLong_avgPeriod;
      sp.cs_ShadowLong_factor = ShadowLong_factor;
      sp.cs_ShadowShort_rangeType = ShadowShort_rangeType;
      sp.cs_ShadowShort_avgPeriod = ShadowShort_avgPeriod;
      sp.cs_ShadowShort_factor = ShadowShort_factor;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlAdvanceBlockOpen (composition seam). */
   CdlAdvanceBlockStream cdlAdvanceBlockOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlAdvanceBlockStream sp = new CdlAdvanceBlockStream(this);
      RetCode retCode = cdlAdvanceBlockOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLADVANCEBLOCK open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLADVANCEBLOCK open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLADVANCEBLOCK open: " + retCode);
   }
   /**
    * Open a live CDLADVANCEBLOCK stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlAdvanceBlock} at that bar.
    * <p>The history must hold at least {@code cdlAdvanceBlockLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlAdvanceBlockStream cdlAdvanceBlockOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return cdlAdvanceBlockOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlAdvanceBlockOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlAdvanceBlock} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CdlAdvanceBlockStream cdlAdvanceBlockOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlAdvanceBlockStream sp = new CdlAdvanceBlockStream(this);
      RetCode retCode = cdlAdvanceBlockOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLADVANCEBLOCK openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLADVANCEBLOCK openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLADVANCEBLOCK openAndFill: " + retCode);
   }
