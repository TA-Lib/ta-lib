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

   /**
    * Number of leading input bars {@link Core#CDLADVANCEBLOCK} consumes before
    * it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLADVANCEBLOCK_Lookback( )
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
   RetCode CDLADVANCEBLOCK_Impl( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLADVANCEBLOCK_Lookback();
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
         ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = FarTrailingIdx;
      while( i < startIdx ) {
         FarPeriodTotal[2] = FarPeriodTotal[2] + ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         FarPeriodTotal[1] = FarPeriodTotal[1] + ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
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
             inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i] > inOpen[i - 1] &&                            /* 3rd opens within/near 2nd real body */
             inOpen[i] <= inClose[i - 1] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
             (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) && ((inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) || (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) ) /* 1st: short upper shadow ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance 3 far smaller than 2 advance blocked with the 3rd ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
         {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowShortPeriodTotal[totIdx] = ShadowShortPeriodTotal[totIdx] + (((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - totIdx] - inOpen[ShadowShortTrailingIdx - totIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowShortTrailingIdx - totIdx] - inLow[ShadowShortTrailingIdx - totIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowShortTrailingIdx - totIdx] - (((inClose[ShadowShortTrailingIdx - totIdx]) >= (inOpen[ShadowShortTrailingIdx - totIdx])) ? (inClose[ShadowShortTrailingIdx - totIdx]) : (inOpen[ShadowShortTrailingIdx - totIdx]))) + ((((inClose[ShadowShortTrailingIdx - totIdx]) >= (inOpen[ShadowShortTrailingIdx - totIdx])) ? (inOpen[ShadowShortTrailingIdx - totIdx]) : (inClose[ShadowShortTrailingIdx - totIdx])) - inLow[ShadowShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            ShadowLongPeriodTotal[totIdx] = ShadowLongPeriodTotal[totIdx] + (((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - totIdx] - inOpen[ShadowLongTrailingIdx - totIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx - totIdx] - inLow[ShadowLongTrailingIdx - totIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx - totIdx] - (((inClose[ShadowLongTrailingIdx - totIdx]) >= (inOpen[ShadowLongTrailingIdx - totIdx])) ? (inClose[ShadowLongTrailingIdx - totIdx]) : (inOpen[ShadowLongTrailingIdx - totIdx]))) + ((((inClose[ShadowLongTrailingIdx - totIdx]) >= (inOpen[ShadowLongTrailingIdx - totIdx])) ? (inOpen[ShadowLongTrailingIdx - totIdx]) : (inClose[ShadowLongTrailingIdx - totIdx])) - inLow[ShadowLongTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            FarPeriodTotal[totIdx] = FarPeriodTotal[totIdx] + (((Far_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Far_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Far_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Far_rangeType == 0) ? (Math.abs(inClose[FarTrailingIdx - totIdx] - inOpen[FarTrailingIdx - totIdx])) : ((Far_rangeType == 1) ? (inHigh[FarTrailingIdx - totIdx] - inLow[FarTrailingIdx - totIdx]) : ((Far_rangeType == 2) ? ((inHigh[FarTrailingIdx - totIdx] - (((inClose[FarTrailingIdx - totIdx]) >= (inOpen[FarTrailingIdx - totIdx])) ? (inClose[FarTrailingIdx - totIdx]) : (inOpen[FarTrailingIdx - totIdx]))) + ((((inClose[FarTrailingIdx - totIdx]) >= (inOpen[FarTrailingIdx - totIdx])) ? (inOpen[FarTrailingIdx - totIdx]) : (inClose[FarTrailingIdx - totIdx])) - inLow[FarTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - (((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inClose[NearTrailingIdx - totIdx]) : (inOpen[NearTrailingIdx - totIdx]))) + ((((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inOpen[NearTrailingIdx - totIdx]) : (inClose[NearTrailingIdx - totIdx])) - inLow[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 2] - inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 2] - inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 2] - (((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inClose[BodyLongTrailingIdx - 2]) : (inOpen[BodyLongTrailingIdx - 2]))) + ((((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inOpen[BodyLongTrailingIdx - 2]) : (inClose[BodyLongTrailingIdx - 2])) - inLow[BodyLongTrailingIdx - 2])) : 0.0)));
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
   RetCode CDLADVANCEBLOCK_Impl( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLADVANCEBLOCK_Lookback();
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
         ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
         ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = FarTrailingIdx;
      while( i < startIdx ) {
         FarPeriodTotal[2] = FarPeriodTotal[2] + ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
         FarPeriodTotal[1] = FarPeriodTotal[1] + ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inClose[i] > (double)inClose[i - 1] && (double)inClose[i - 1] > (double)inClose[i - 2] && (double)inOpen[i - 1] > (double)inOpen[i - 2] && (double)inOpen[i - 1] <= (double)inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] > (double)inOpen[i - 1] && (double)inOpen[i] <= (double)inClose[i - 1] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i - 2] - (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inClose[i - 2] : (double)inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) && (((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) || ((double)inHigh[i - 1] - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inClose[i - 1] : (double)inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) ) {
            outInteger[outIdx++] = 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowShortPeriodTotal[totIdx] = ShadowShortPeriodTotal[totIdx] + (((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - totIdx] - ((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inClose[i - totIdx]) : ((double)inOpen[i - totIdx]))) + (((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inOpen[i - totIdx]) : ((double)inClose[i - totIdx])) - (double)inLow[i - totIdx])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowShortTrailingIdx - totIdx] - (double)inOpen[ShadowShortTrailingIdx - totIdx])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[ShadowShortTrailingIdx - totIdx] - (double)inLow[ShadowShortTrailingIdx - totIdx]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[ShadowShortTrailingIdx - totIdx] - ((((double)inClose[ShadowShortTrailingIdx - totIdx]) >= ((double)inOpen[ShadowShortTrailingIdx - totIdx])) ? ((double)inClose[ShadowShortTrailingIdx - totIdx]) : ((double)inOpen[ShadowShortTrailingIdx - totIdx]))) + (((((double)inClose[ShadowShortTrailingIdx - totIdx]) >= ((double)inOpen[ShadowShortTrailingIdx - totIdx])) ? ((double)inOpen[ShadowShortTrailingIdx - totIdx]) : ((double)inClose[ShadowShortTrailingIdx - totIdx])) - (double)inLow[ShadowShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            ShadowLongPeriodTotal[totIdx] = ShadowLongPeriodTotal[totIdx] + (((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i - totIdx] - ((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inClose[i - totIdx]) : ((double)inOpen[i - totIdx]))) + (((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inOpen[i - totIdx]) : ((double)inClose[i - totIdx])) - (double)inLow[i - totIdx])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowLongTrailingIdx - totIdx] - (double)inOpen[ShadowLongTrailingIdx - totIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[ShadowLongTrailingIdx - totIdx] - (double)inLow[ShadowLongTrailingIdx - totIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[ShadowLongTrailingIdx - totIdx] - ((((double)inClose[ShadowLongTrailingIdx - totIdx]) >= ((double)inOpen[ShadowLongTrailingIdx - totIdx])) ? ((double)inClose[ShadowLongTrailingIdx - totIdx]) : ((double)inOpen[ShadowLongTrailingIdx - totIdx]))) + (((((double)inClose[ShadowLongTrailingIdx - totIdx]) >= ((double)inOpen[ShadowLongTrailingIdx - totIdx])) ? ((double)inOpen[ShadowLongTrailingIdx - totIdx]) : ((double)inClose[ShadowLongTrailingIdx - totIdx])) - (double)inLow[ShadowLongTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            FarPeriodTotal[totIdx] = FarPeriodTotal[totIdx] + (((Far_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Far_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Far_rangeType == 2) ? (((double)inHigh[i - totIdx] - ((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inClose[i - totIdx]) : ((double)inOpen[i - totIdx]))) + (((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inOpen[i - totIdx]) : ((double)inClose[i - totIdx])) - (double)inLow[i - totIdx])) : 0.0))) - ((Far_rangeType == 0) ? (Math.abs((double)inClose[FarTrailingIdx - totIdx] - (double)inOpen[FarTrailingIdx - totIdx])) : ((Far_rangeType == 1) ? ((double)inHigh[FarTrailingIdx - totIdx] - (double)inLow[FarTrailingIdx - totIdx]) : ((Far_rangeType == 2) ? (((double)inHigh[FarTrailingIdx - totIdx] - ((((double)inClose[FarTrailingIdx - totIdx]) >= ((double)inOpen[FarTrailingIdx - totIdx])) ? ((double)inClose[FarTrailingIdx - totIdx]) : ((double)inOpen[FarTrailingIdx - totIdx]))) + (((((double)inClose[FarTrailingIdx - totIdx]) >= ((double)inOpen[FarTrailingIdx - totIdx])) ? ((double)inOpen[FarTrailingIdx - totIdx]) : ((double)inClose[FarTrailingIdx - totIdx])) - (double)inLow[FarTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[i - totIdx] - ((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inClose[i - totIdx]) : ((double)inOpen[i - totIdx]))) + (((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inOpen[i - totIdx]) : ((double)inClose[i - totIdx])) - (double)inLow[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - totIdx] - ((((double)inClose[NearTrailingIdx - totIdx]) >= ((double)inOpen[NearTrailingIdx - totIdx])) ? ((double)inClose[NearTrailingIdx - totIdx]) : ((double)inOpen[NearTrailingIdx - totIdx]))) + (((((double)inClose[NearTrailingIdx - totIdx]) >= ((double)inOpen[NearTrailingIdx - totIdx])) ? ((double)inOpen[NearTrailingIdx - totIdx]) : ((double)inClose[NearTrailingIdx - totIdx])) - (double)inLow[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 2] - (double)inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 2] - (double)inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 2] - ((((double)inClose[BodyLongTrailingIdx - 2]) >= ((double)inOpen[BodyLongTrailingIdx - 2])) ? ((double)inClose[BodyLongTrailingIdx - 2]) : ((double)inOpen[BodyLongTrailingIdx - 2]))) + (((((double)inClose[BodyLongTrailingIdx - 2]) >= ((double)inOpen[BodyLongTrailingIdx - 2])) ? ((double)inOpen[BodyLongTrailingIdx - 2]) : ((double)inClose[BodyLongTrailingIdx - 2])) - (double)inLow[BodyLongTrailingIdx - 2])) : 0.0)));
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
   /**
    * Three-candle bearish reversal pattern: three white candles with
    * consecutively higher closes whose advance weakens (progressively smaller
    * bodies and/or lengthening upper shadows). Signals that an uptrend's
    * advance is being blocked. A hit (-100) is bearish: the advance is
    * stalling/blocked; meaningful mainly within an existing uptrend.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior uptrend the pattern classically assumes for significance.</li>
    * <li>Although classically read as a bearish reversal, Bulkowski's testing found the Advance Block actually acts as a bullish continuation 64% of the time. ([thepatternsite.com](https://thepatternsite.com/AdvanceBlock.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLADVANCEBLOCK_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger -100 on a detected pattern (always bearish), 0
    *        otherwise; never emits +100. Must hold at least
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
    * @see Core#CDL3WHITESOLDIERS
    * @see Core#CDLSTALLEDPATTERN
    */
   public OutRange CDLADVANCEBLOCK( int startIdx,
                                    int endIdx,
                                    double inOpen[],
                                    double inHigh[],
                                    double inLow[],
                                    double inClose[],
                                    int outInteger[] )
   {
      requireIndexRange("CDLADVANCEBLOCK", startIdx, endIdx);
      int guardStart = clampedStart("CDLADVANCEBLOCK", startIdx, CDLADVANCEBLOCK_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLADVANCEBLOCK", "inOpen", inOpen, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inHigh", inHigh, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inLow", inLow, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inClose", inClose, guardInLen);
      requireLength("CDLADVANCEBLOCK", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLADVANCEBLOCK_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLADVANCEBLOCK", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Three-candle bearish reversal pattern: three white candles with
    * consecutively higher closes whose advance weakens (progressively smaller
    * bodies and/or lengthening upper shadows). Signals that an uptrend's
    * advance is being blocked. A hit (-100) is bearish: the advance is
    * stalling/blocked; meaningful mainly within an existing uptrend.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior uptrend the pattern classically assumes for significance.</li>
    * <li>Although classically read as a bearish reversal, Bulkowski's testing found the Advance Block actually acts as a bullish continuation 64% of the time. ([thepatternsite.com](https://thepatternsite.com/AdvanceBlock.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLADVANCEBLOCK_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger -100 on a detected pattern (always bearish), 0
    *        otherwise; never emits +100. Must hold at least
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
    * @see Core#CDL3WHITESOLDIERS
    * @see Core#CDLSTALLEDPATTERN
    */
   public OutRange CDLADVANCEBLOCK( int startIdx,
                                    int endIdx,
                                    float inOpen[],
                                    float inHigh[],
                                    float inLow[],
                                    float inClose[],
                                    int outInteger[] )
   {
      requireIndexRange("CDLADVANCEBLOCK", startIdx, endIdx);
      int guardStart = clampedStart("CDLADVANCEBLOCK", startIdx, CDLADVANCEBLOCK_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLADVANCEBLOCK", "inOpen", inOpen, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inHigh", inHigh, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inLow", inLow, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inClose", inClose, guardInLen);
      requireLength("CDLADVANCEBLOCK", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLADVANCEBLOCK_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLADVANCEBLOCK", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLADVANCEBLOCK stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLADVANCEBLOCK} over the same series.
    * Open with {@link Core#CDLADVANCEBLOCK_Open}; there is no close — the handle is
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
   public static final class CDLADVANCEBLOCK_Stream {
      Core core;
      double[] ShadowShortPeriodTotal;
      double[] ShadowLongPeriodTotal;
      double[] NearPeriodTotal;
      double[] FarPeriodTotal;
      double BodyLongPeriodTotal;
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
      double[] ring_BodyLongTrailingIdx_derived;
      int ringPos_FarTrailingIdx;
      int ringCap_FarTrailingIdx;
      int ringLag_FarTrailingIdx;
      double[] ring_FarTrailingIdx_derived;
      int ringPos_NearTrailingIdx;
      int ringCap_NearTrailingIdx;
      int ringLag_NearTrailingIdx;
      double[] ring_NearTrailingIdx_derived;
      int ringPos_ShadowLongTrailingIdx;
      int ringCap_ShadowLongTrailingIdx;
      int ringLag_ShadowLongTrailingIdx;
      double[] ring_ShadowLongTrailingIdx_derived;
      int ringPos_ShadowShortTrailingIdx;
      int ringCap_ShadowShortTrailingIdx;
      int ringLag_ShadowShortTrailingIdx;
      double[] ring_ShadowShortTrailingIdx_derived;
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
      int outRangeBegIdx;
      int outRangeCount;

      CDLADVANCEBLOCK_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLADVANCEBLOCK} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CDLADVANCEBLOCK_Stream( CDLADVANCEBLOCK_Stream other ) {
         this.core = other.core;
         this.ShadowShortPeriodTotal = other.ShadowShortPeriodTotal.clone();
         this.ShadowLongPeriodTotal = other.ShadowLongPeriodTotal.clone();
         this.NearPeriodTotal = other.NearPeriodTotal.clone();
         this.FarPeriodTotal = other.FarPeriodTotal.clone();
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
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
         this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         this.ringPos_FarTrailingIdx = other.ringPos_FarTrailingIdx;
         this.ringCap_FarTrailingIdx = other.ringCap_FarTrailingIdx;
         this.ringLag_FarTrailingIdx = other.ringLag_FarTrailingIdx;
         this.ring_FarTrailingIdx_derived = other.ring_FarTrailingIdx_derived.clone();
         this.ringPos_NearTrailingIdx = other.ringPos_NearTrailingIdx;
         this.ringCap_NearTrailingIdx = other.ringCap_NearTrailingIdx;
         this.ringLag_NearTrailingIdx = other.ringLag_NearTrailingIdx;
         this.ring_NearTrailingIdx_derived = other.ring_NearTrailingIdx_derived.clone();
         this.ringPos_ShadowLongTrailingIdx = other.ringPos_ShadowLongTrailingIdx;
         this.ringCap_ShadowLongTrailingIdx = other.ringCap_ShadowLongTrailingIdx;
         this.ringLag_ShadowLongTrailingIdx = other.ringLag_ShadowLongTrailingIdx;
         this.ring_ShadowLongTrailingIdx_derived = other.ring_ShadowLongTrailingIdx_derived.clone();
         this.ringPos_ShadowShortTrailingIdx = other.ringPos_ShadowShortTrailingIdx;
         this.ringCap_ShadowShortTrailingIdx = other.ringCap_ShadowShortTrailingIdx;
         this.ringLag_ShadowShortTrailingIdx = other.ringLag_ShadowShortTrailingIdx;
         this.ring_ShadowShortTrailingIdx_derived = other.ring_ShadowShortTrailingIdx_derived.clone();
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
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( CDLADVANCEBLOCK_Stream other ) {
         this.core = other.core;
         if( this.ShadowShortPeriodTotal != null && this.ShadowShortPeriodTotal.length == other.ShadowShortPeriodTotal.length ) {
            System.arraycopy( other.ShadowShortPeriodTotal, 0, this.ShadowShortPeriodTotal, 0, other.ShadowShortPeriodTotal.length );
         } else {
            this.ShadowShortPeriodTotal = other.ShadowShortPeriodTotal.clone();
         }
         if( this.ShadowLongPeriodTotal != null && this.ShadowLongPeriodTotal.length == other.ShadowLongPeriodTotal.length ) {
            System.arraycopy( other.ShadowLongPeriodTotal, 0, this.ShadowLongPeriodTotal, 0, other.ShadowLongPeriodTotal.length );
         } else {
            this.ShadowLongPeriodTotal = other.ShadowLongPeriodTotal.clone();
         }
         if( this.NearPeriodTotal != null && this.NearPeriodTotal.length == other.NearPeriodTotal.length ) {
            System.arraycopy( other.NearPeriodTotal, 0, this.NearPeriodTotal, 0, other.NearPeriodTotal.length );
         } else {
            this.NearPeriodTotal = other.NearPeriodTotal.clone();
         }
         if( this.FarPeriodTotal != null && this.FarPeriodTotal.length == other.FarPeriodTotal.length ) {
            System.arraycopy( other.FarPeriodTotal, 0, this.FarPeriodTotal, 0, other.FarPeriodTotal.length );
         } else {
            this.FarPeriodTotal = other.FarPeriodTotal.clone();
         }
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
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
         if( this.ring_BodyLongTrailingIdx_derived != null && this.ring_BodyLongTrailingIdx_derived.length == other.ring_BodyLongTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_derived, 0, this.ring_BodyLongTrailingIdx_derived, 0, other.ring_BodyLongTrailingIdx_derived.length );
         } else {
            this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         }
         this.ringPos_FarTrailingIdx = other.ringPos_FarTrailingIdx;
         this.ringCap_FarTrailingIdx = other.ringCap_FarTrailingIdx;
         this.ringLag_FarTrailingIdx = other.ringLag_FarTrailingIdx;
         if( this.ring_FarTrailingIdx_derived != null && this.ring_FarTrailingIdx_derived.length == other.ring_FarTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_FarTrailingIdx_derived, 0, this.ring_FarTrailingIdx_derived, 0, other.ring_FarTrailingIdx_derived.length );
         } else {
            this.ring_FarTrailingIdx_derived = other.ring_FarTrailingIdx_derived.clone();
         }
         this.ringPos_NearTrailingIdx = other.ringPos_NearTrailingIdx;
         this.ringCap_NearTrailingIdx = other.ringCap_NearTrailingIdx;
         this.ringLag_NearTrailingIdx = other.ringLag_NearTrailingIdx;
         if( this.ring_NearTrailingIdx_derived != null && this.ring_NearTrailingIdx_derived.length == other.ring_NearTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_NearTrailingIdx_derived, 0, this.ring_NearTrailingIdx_derived, 0, other.ring_NearTrailingIdx_derived.length );
         } else {
            this.ring_NearTrailingIdx_derived = other.ring_NearTrailingIdx_derived.clone();
         }
         this.ringPos_ShadowLongTrailingIdx = other.ringPos_ShadowLongTrailingIdx;
         this.ringCap_ShadowLongTrailingIdx = other.ringCap_ShadowLongTrailingIdx;
         this.ringLag_ShadowLongTrailingIdx = other.ringLag_ShadowLongTrailingIdx;
         if( this.ring_ShadowLongTrailingIdx_derived != null && this.ring_ShadowLongTrailingIdx_derived.length == other.ring_ShadowLongTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_ShadowLongTrailingIdx_derived, 0, this.ring_ShadowLongTrailingIdx_derived, 0, other.ring_ShadowLongTrailingIdx_derived.length );
         } else {
            this.ring_ShadowLongTrailingIdx_derived = other.ring_ShadowLongTrailingIdx_derived.clone();
         }
         this.ringPos_ShadowShortTrailingIdx = other.ringPos_ShadowShortTrailingIdx;
         this.ringCap_ShadowShortTrailingIdx = other.ringCap_ShadowShortTrailingIdx;
         this.ringLag_ShadowShortTrailingIdx = other.ringLag_ShadowShortTrailingIdx;
         if( this.ring_ShadowShortTrailingIdx_derived != null && this.ring_ShadowShortTrailingIdx_derived.length == other.ring_ShadowShortTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_ShadowShortTrailingIdx_derived, 0, this.ring_ShadowShortTrailingIdx_derived, 0, other.ring_ShadowShortTrailingIdx_derived.length );
         } else {
            this.ring_ShadowShortTrailingIdx_derived = other.ring_ShadowShortTrailingIdx_derived.clone();
         }
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
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDLADVANCEBLOCK_Stream> PEEK_SCRATCH = new ThreadLocal<>();

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
            throw new TaLibArgumentException("CDLADVANCEBLOCK update: BadParam", RetCode.BadParam);
         core.CDLADVANCEBLOCK_StepImpl(this, inOpen, inHigh, inLow, inClose);
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
         requireArgument("CDLADVANCEBLOCK updateAndFill", "inOpen", inOpen);
         requireArgument("CDLADVANCEBLOCK updateAndFill", "inHigh", inHigh);
         requireArgument("CDLADVANCEBLOCK updateAndFill", "inLow", inLow);
         requireArgument("CDLADVANCEBLOCK updateAndFill", "inClose", inClose);
         requireArgument("CDLADVANCEBLOCK updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLADVANCEBLOCK updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("CDLADVANCEBLOCK updateAndFill: BadParam", RetCode.BadParam);
            core.CDLADVANCEBLOCK_StepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
            throw new TaLibArgumentException("CDLADVANCEBLOCK peek: BadParam", RetCode.BadParam);
         CDLADVANCEBLOCK_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLADVANCEBLOCK_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLADVANCEBLOCK_StepImpl(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLADVANCEBLOCK_Stream copy() {
         return new CDLADVANCEBLOCK_Stream(this);
      }
   }
   void CDLADVANCEBLOCK_StepImpl( CDLADVANCEBLOCK_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int totIdx = 0;
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
      sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ring_FarTrailingIdx_derived[sp.ringPos_FarTrailingIdx] = ((Far_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Far_rangeType == 1) ? (inHigh - inLow) : ((Far_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType == 1) ? (inHigh - inLow) : ((Near_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx] = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ring_ShadowShortTrailingIdx_derived[sp.ringPos_ShadowShortTrailingIdx] = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 1 && /* 1st white */
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && /* 2nd white */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 1 &&                 /* 3rd white */
          inClose > sp.lag1_inClose &&
          sp.lag1_inClose > sp.lag2_inClose &&                      /* consecutive higher closes */
          sp.lag1_inOpen > sp.lag2_inOpen &&                        /* 2nd opens within/near 1st real body */
          sp.lag1_inOpen <= sp.lag2_inClose + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          inOpen > sp.lag1_inOpen &&                                /* 3rd opens within/near 2nd real body */
          inOpen <= sp.lag1_inClose + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
          (sp.lag2_inHigh - ((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inClose : sp.lag2_inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowShort_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowShort_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) &&
          (Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) - ((Far_factor * (((Far_avgPeriod != 0) ? (sp.FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Far_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Far_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Far_factor * (((Far_avgPeriod != 0) ? (sp.FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Far_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Far_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) && ((inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) || (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowShort_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowShort_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) || Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (sp.ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) ) /* 1st: short upper shadow ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance 3 far smaller than 2 advance blocked with the 3rd ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
      {
         sp.cur_outInteger = 0 - 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
         sp.ShadowShortPeriodTotal[totIdx] = sp.ShadowShortPeriodTotal[totIdx] + (sp.ring_ShadowShortTrailingIdx_derived[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - totIdx >= sp.ringCap_ShadowShortTrailingIdx) ? sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - totIdx - sp.ringCap_ShadowShortTrailingIdx : sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - totIdx] - sp.ring_ShadowShortTrailingIdx_derived[(sp.ringPos_ShadowShortTrailingIdx + sp.ringCap_ShadowShortTrailingIdx - sp.ringLag_ShadowShortTrailingIdx - totIdx) % sp.ringCap_ShadowShortTrailingIdx]);
      }
      for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
         sp.ShadowLongPeriodTotal[totIdx] = sp.ShadowLongPeriodTotal[totIdx] + (sp.ring_ShadowLongTrailingIdx_derived[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - totIdx >= sp.ringCap_ShadowLongTrailingIdx) ? sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - totIdx - sp.ringCap_ShadowLongTrailingIdx : sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - totIdx] - sp.ring_ShadowLongTrailingIdx_derived[(sp.ringPos_ShadowLongTrailingIdx + sp.ringCap_ShadowLongTrailingIdx - sp.ringLag_ShadowLongTrailingIdx - totIdx) % sp.ringCap_ShadowLongTrailingIdx]);
      }
      for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
         sp.FarPeriodTotal[totIdx] = sp.FarPeriodTotal[totIdx] + (sp.ring_FarTrailingIdx_derived[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - totIdx >= sp.ringCap_FarTrailingIdx) ? sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - totIdx - sp.ringCap_FarTrailingIdx : sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - totIdx] - sp.ring_FarTrailingIdx_derived[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - totIdx) % sp.ringCap_FarTrailingIdx]);
         sp.NearPeriodTotal[totIdx] = sp.NearPeriodTotal[totIdx] + (sp.ring_NearTrailingIdx_derived[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx >= sp.ringCap_NearTrailingIdx) ? sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx - sp.ringCap_NearTrailingIdx : sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx] - sp.ring_NearTrailingIdx_derived[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - totIdx) % sp.ringCap_NearTrailingIdx]);
      }
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 2) % sp.ringCap_BodyLongTrailingIdx];
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
      sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
      if( sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx ) {
         sp.ringPos_BodyLongTrailingIdx = 0;
      }
      sp.ringPos_FarTrailingIdx = sp.ringPos_FarTrailingIdx + 1;
      if( sp.ringPos_FarTrailingIdx >= sp.ringCap_FarTrailingIdx ) {
         sp.ringPos_FarTrailingIdx = 0;
      }
      sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
      if( sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx ) {
         sp.ringPos_NearTrailingIdx = 0;
      }
      sp.ringPos_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx + 1;
      if( sp.ringPos_ShadowLongTrailingIdx >= sp.ringCap_ShadowLongTrailingIdx ) {
         sp.ringPos_ShadowLongTrailingIdx = 0;
      }
      sp.ringPos_ShadowShortTrailingIdx = sp.ringPos_ShadowShortTrailingIdx + 1;
      if( sp.ringPos_ShadowShortTrailingIdx >= sp.ringCap_ShadowShortTrailingIdx ) {
         sp.ringPos_ShadowShortTrailingIdx = 0;
      }
   }
   private RetCode CDLADVANCEBLOCK_OpenImpl( CDLADVANCEBLOCK_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
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
      lookbackTotal = CDLADVANCEBLOCK_Lookback();
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
         ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = ShadowLongTrailingIdx;
      while( i < startIdx ) {
         ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         NearPeriodTotal[1] = NearPeriodTotal[1] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = FarTrailingIdx;
      while( i < startIdx ) {
         FarPeriodTotal[2] = FarPeriodTotal[2] + ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         FarPeriodTotal[1] = FarPeriodTotal[1] + ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
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
             inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i] > inOpen[i - 1] &&                            /* 3rd opens within/near 2nd real body */
             inOpen[i] <= inClose[i - 1] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
             (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) &&
             (Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) && ((inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) || (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) || Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) ) /* 1st: short upper shadow ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance 3 far smaller than 2 advance blocked with the 3rd ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
         {
            outInteger[outIdx++ * outStride] = 0 - 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 2; totIdx >= 0; totIdx -= 1 ) {
            ShadowShortPeriodTotal[totIdx] = ShadowShortPeriodTotal[totIdx] + (((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - totIdx] - inOpen[ShadowShortTrailingIdx - totIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowShortTrailingIdx - totIdx] - inLow[ShadowShortTrailingIdx - totIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowShortTrailingIdx - totIdx] - (((inClose[ShadowShortTrailingIdx - totIdx]) >= (inOpen[ShadowShortTrailingIdx - totIdx])) ? (inClose[ShadowShortTrailingIdx - totIdx]) : (inOpen[ShadowShortTrailingIdx - totIdx]))) + ((((inClose[ShadowShortTrailingIdx - totIdx]) >= (inOpen[ShadowShortTrailingIdx - totIdx])) ? (inOpen[ShadowShortTrailingIdx - totIdx]) : (inClose[ShadowShortTrailingIdx - totIdx])) - inLow[ShadowShortTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            ShadowLongPeriodTotal[totIdx] = ShadowLongPeriodTotal[totIdx] + (((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - totIdx] - inOpen[ShadowLongTrailingIdx - totIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx - totIdx] - inLow[ShadowLongTrailingIdx - totIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx - totIdx] - (((inClose[ShadowLongTrailingIdx - totIdx]) >= (inOpen[ShadowLongTrailingIdx - totIdx])) ? (inClose[ShadowLongTrailingIdx - totIdx]) : (inOpen[ShadowLongTrailingIdx - totIdx]))) + ((((inClose[ShadowLongTrailingIdx - totIdx]) >= (inOpen[ShadowLongTrailingIdx - totIdx])) ? (inOpen[ShadowLongTrailingIdx - totIdx]) : (inClose[ShadowLongTrailingIdx - totIdx])) - inLow[ShadowLongTrailingIdx - totIdx])) : 0.0))));
         }
         for( totIdx = 2; totIdx >= 1; totIdx -= 1 ) {
            FarPeriodTotal[totIdx] = FarPeriodTotal[totIdx] + (((Far_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Far_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Far_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Far_rangeType == 0) ? (Math.abs(inClose[FarTrailingIdx - totIdx] - inOpen[FarTrailingIdx - totIdx])) : ((Far_rangeType == 1) ? (inHigh[FarTrailingIdx - totIdx] - inLow[FarTrailingIdx - totIdx]) : ((Far_rangeType == 2) ? ((inHigh[FarTrailingIdx - totIdx] - (((inClose[FarTrailingIdx - totIdx]) >= (inOpen[FarTrailingIdx - totIdx])) ? (inClose[FarTrailingIdx - totIdx]) : (inOpen[FarTrailingIdx - totIdx]))) + ((((inClose[FarTrailingIdx - totIdx]) >= (inOpen[FarTrailingIdx - totIdx])) ? (inOpen[FarTrailingIdx - totIdx]) : (inClose[FarTrailingIdx - totIdx])) - inLow[FarTrailingIdx - totIdx])) : 0.0))));
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - (((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inClose[NearTrailingIdx - totIdx]) : (inOpen[NearTrailingIdx - totIdx]))) + ((((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inOpen[NearTrailingIdx - totIdx]) : (inClose[NearTrailingIdx - totIdx])) - inLow[NearTrailingIdx - totIdx])) : 0.0))));
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 2] - inOpen[BodyLongTrailingIdx - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 2] - inLow[BodyLongTrailingIdx - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 2] - (((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inClose[BodyLongTrailingIdx - 2]) : (inOpen[BodyLongTrailingIdx - 2]))) + ((((inClose[BodyLongTrailingIdx - 2]) >= (inOpen[BodyLongTrailingIdx - 2])) ? (inOpen[BodyLongTrailingIdx - 2]) : (inClose[BodyLongTrailingIdx - 2])) - inLow[BodyLongTrailingIdx - 2])) : 0.0)));
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
      double[] capRing_BodyLongTrailingIdx_derived = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int capLag_FarTrailingIdx = i - FarTrailingIdx;
      int cap_FarTrailingIdx = capLag_FarTrailingIdx + 3;
      if( capLag_FarTrailingIdx < 0 || cap_FarTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_FarTrailingIdx = (cap_FarTrailingIdx > 0)? cap_FarTrailingIdx : 1;
      double[] capRing_FarTrailingIdx_derived = new double[allocN_FarTrailingIdx];
      for( int fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_FarTrailingIdx_derived[fillJ % cap_FarTrailingIdx] = ((Far_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Far_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Far_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int capLag_NearTrailingIdx = i - NearTrailingIdx;
      int cap_NearTrailingIdx = capLag_NearTrailingIdx + 3;
      if( capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_NearTrailingIdx = (cap_NearTrailingIdx > 0)? cap_NearTrailingIdx : 1;
      double[] capRing_NearTrailingIdx_derived = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int capLag_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
      int cap_ShadowLongTrailingIdx = capLag_ShadowLongTrailingIdx + 3;
      if( capLag_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowLongTrailingIdx = (cap_ShadowLongTrailingIdx > 0)? cap_ShadowLongTrailingIdx : 1;
      double[] capRing_ShadowLongTrailingIdx_derived = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_derived[fillJ % cap_ShadowLongTrailingIdx] = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowLong_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowLong_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int capLag_ShadowShortTrailingIdx = i - ShadowShortTrailingIdx;
      int cap_ShadowShortTrailingIdx = capLag_ShadowShortTrailingIdx + 3;
      if( capLag_ShadowShortTrailingIdx < 0 || cap_ShadowShortTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowShortTrailingIdx = (cap_ShadowShortTrailingIdx > 0)? cap_ShadowShortTrailingIdx : 1;
      double[] capRing_ShadowShortTrailingIdx_derived = new double[allocN_ShadowShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowShortTrailingIdx_derived[fillJ % cap_ShadowShortTrailingIdx] = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowShort_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowShort_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.ShadowShortPeriodTotal = ShadowShortPeriodTotal;
      sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
      sp.NearPeriodTotal = NearPeriodTotal;
      sp.FarPeriodTotal = FarPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
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
      sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
      sp.ringPos_FarTrailingIdx = historyLen % cap_FarTrailingIdx;
      sp.ringCap_FarTrailingIdx = cap_FarTrailingIdx;
      sp.ringLag_FarTrailingIdx = capLag_FarTrailingIdx;
      sp.ring_FarTrailingIdx_derived = capRing_FarTrailingIdx_derived;
      sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
      sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
      sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
      sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
      sp.ringPos_ShadowLongTrailingIdx = historyLen % cap_ShadowLongTrailingIdx;
      sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
      sp.ringLag_ShadowLongTrailingIdx = capLag_ShadowLongTrailingIdx;
      sp.ring_ShadowLongTrailingIdx_derived = capRing_ShadowLongTrailingIdx_derived;
      sp.ringPos_ShadowShortTrailingIdx = historyLen % cap_ShadowShortTrailingIdx;
      sp.ringCap_ShadowShortTrailingIdx = cap_ShadowShortTrailingIdx;
      sp.ringLag_ShadowShortTrailingIdx = capLag_ShadowShortTrailingIdx;
      sp.ring_ShadowShortTrailingIdx_derived = capRing_ShadowShortTrailingIdx_derived;
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
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* CDLADVANCEBLOCK_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLADVANCEBLOCK_Stream CDLADVANCEBLOCK_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLADVANCEBLOCK_Stream sp = new CDLADVANCEBLOCK_Stream(this);
      RetCode retCode = CDLADVANCEBLOCK_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLADVANCEBLOCK openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLADVANCEBLOCK openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLADVANCEBLOCK openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind CDLADVANCEBLOCK_Open (composition seam). */
   CDLADVANCEBLOCK_Stream CDLADVANCEBLOCK_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLADVANCEBLOCK_Stream sp = new CDLADVANCEBLOCK_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = CDLADVANCEBLOCK_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLADVANCEBLOCK open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLADVANCEBLOCK open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLADVANCEBLOCK open: " + retCode, retCode);
   }
   /**
    * Open a live CDLADVANCEBLOCK stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLADVANCEBLOCK} at that bar.
    * <p>The history must hold at least {@code CDLADVANCEBLOCK_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CDLADVANCEBLOCK_Stream CDLADVANCEBLOCK_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLADVANCEBLOCK open", "inOpen", inOpen);
      requireHistory("CDLADVANCEBLOCK open", inOpen.length);
      requireArgument("CDLADVANCEBLOCK open", "inHigh", inHigh);
      requireArgument("CDLADVANCEBLOCK open", "inLow", inLow);
      requireArgument("CDLADVANCEBLOCK open", "inClose", inClose);
      requireHistoryLength("CDLADVANCEBLOCK open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLADVANCEBLOCK open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLADVANCEBLOCK open", "inClose", inClose.length, inOpen.length);
      return CDLADVANCEBLOCK_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLADVANCEBLOCK_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLADVANCEBLOCK} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CDLADVANCEBLOCK_Stream#outRange()}.
    */
   public CDLADVANCEBLOCK_Stream CDLADVANCEBLOCK_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLADVANCEBLOCK openAndFill", "inOpen", inOpen);
      requireHistory("CDLADVANCEBLOCK openAndFill", inOpen.length);
      requireArgument("CDLADVANCEBLOCK openAndFill", "inHigh", inHigh);
      requireArgument("CDLADVANCEBLOCK openAndFill", "inLow", inLow);
      requireArgument("CDLADVANCEBLOCK openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLADVANCEBLOCK openAndFill", inOpen.length, CDLADVANCEBLOCK_Lookback());
      requireHistoryLength("CDLADVANCEBLOCK openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLADVANCEBLOCK openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLADVANCEBLOCK openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLADVANCEBLOCK openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLADVANCEBLOCK openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return CDLADVANCEBLOCK_OpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
