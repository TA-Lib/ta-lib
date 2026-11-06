/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120404 AC     Creation
 *  100426 MF,CC  Remove the outInteger comment, which misstated the values
 *  103126 MF,CC  Nest the cheap, selective pre-check conjuncts in an outer if
 *  110526 MF,CC  Carry the ranges of the previous bars instead of recomputing them
 */

   /**
    * Number of leading input bars {@link Core#cdladvanceblock} consumes before
    * it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int cdladvanceblockLookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].factor;
      int Far_rangeType = this.candleSettings[CandleSettingType.FAR.ordinal()].rangeType.ordinal();
      int Far_avgPeriod = this.candleSettings[CandleSettingType.FAR.ordinal()].avgPeriod;
      double Far_factor = this.candleSettings[CandleSettingType.FAR.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.NEAR.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.NEAR.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.NEAR.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].factor;
      return Math.max(Math.max(Math.max(ShadowLong_avgPeriod, ShadowShort_avgPeriod), Math.max(Far_avgPeriod, Near_avgPeriod)), BodyLong_avgPeriod) + 2 ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#cdladvanceblock}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int cdladvanceblockDisplayShift( int outputIdx )
   {
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode cdladvanceblockImpl( int startIdx,
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
      double ShadowShortPeriodTotalRange0 = 0;
      double ShadowShortPeriodTotalTrailRange0 = 0;
      double ShadowShortPeriodTotalRange1 = 0;
      double ShadowShortPeriodTotalTrailRange1 = 0;
      double ShadowShortPeriodTotalRange2 = 0;
      double ShadowShortPeriodTotalTrailRange2 = 0;
      double ShadowLongPeriodTotalRange0 = 0;
      double ShadowLongPeriodTotalTrailRange0 = 0;
      double ShadowLongPeriodTotalRange1 = 0;
      double ShadowLongPeriodTotalTrailRange1 = 0;
      double FarPeriodTotalRange1 = 0;
      double FarPeriodTotalTrailRange1 = 0;
      double FarPeriodTotalRange2 = 0;
      double FarPeriodTotalTrailRange2 = 0;
      double NearPeriodTotalRange1 = 0;
      double NearPeriodTotalTrailRange1 = 0;
      double NearPeriodTotalRange2 = 0;
      double NearPeriodTotalTrailRange2 = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
      int ShadowShortTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int FarTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].factor;
      int Far_rangeType = this.candleSettings[CandleSettingType.FAR.ordinal()].rangeType.ordinal();
      int Far_avgPeriod = this.candleSettings[CandleSettingType.FAR.ordinal()].avgPeriod;
      double Far_factor = this.candleSettings[CandleSettingType.FAR.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.NEAR.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.NEAR.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.NEAR.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdladvanceblockLookback();
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
         return RetCode.SUCCESS ;
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
       * The meanings of "long body", "short shadow", "far" and "near" are specified with TA_SetCandleSettings
       */
      outIdx = 0;
      ShadowShortPeriodTotalRange1 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
      ShadowShortPeriodTotalTrailRange1 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - 1] - inOpen[ShadowShortTrailingIdx - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowShortTrailingIdx - 1] - inLow[ShadowShortTrailingIdx - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowShortTrailingIdx - 1] - (((inClose[ShadowShortTrailingIdx - 1]) >= (inOpen[ShadowShortTrailingIdx - 1])) ? (inClose[ShadowShortTrailingIdx - 1]) : (inOpen[ShadowShortTrailingIdx - 1]))) + ((((inClose[ShadowShortTrailingIdx - 1]) >= (inOpen[ShadowShortTrailingIdx - 1])) ? (inOpen[ShadowShortTrailingIdx - 1]) : (inClose[ShadowShortTrailingIdx - 1])) - inLow[ShadowShortTrailingIdx - 1])) : 0.0)));
      ShadowShortPeriodTotalRange2 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
      ShadowShortPeriodTotalTrailRange2 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - 2] - inOpen[ShadowShortTrailingIdx - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowShortTrailingIdx - 2] - inLow[ShadowShortTrailingIdx - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowShortTrailingIdx - 2] - (((inClose[ShadowShortTrailingIdx - 2]) >= (inOpen[ShadowShortTrailingIdx - 2])) ? (inClose[ShadowShortTrailingIdx - 2]) : (inOpen[ShadowShortTrailingIdx - 2]))) + ((((inClose[ShadowShortTrailingIdx - 2]) >= (inOpen[ShadowShortTrailingIdx - 2])) ? (inOpen[ShadowShortTrailingIdx - 2]) : (inClose[ShadowShortTrailingIdx - 2])) - inLow[ShadowShortTrailingIdx - 2])) : 0.0)));
      ShadowLongPeriodTotalRange1 = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
      ShadowLongPeriodTotalTrailRange1 = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - 1] - inOpen[ShadowLongTrailingIdx - 1])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx - 1] - inLow[ShadowLongTrailingIdx - 1]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx - 1] - (((inClose[ShadowLongTrailingIdx - 1]) >= (inOpen[ShadowLongTrailingIdx - 1])) ? (inClose[ShadowLongTrailingIdx - 1]) : (inOpen[ShadowLongTrailingIdx - 1]))) + ((((inClose[ShadowLongTrailingIdx - 1]) >= (inOpen[ShadowLongTrailingIdx - 1])) ? (inOpen[ShadowLongTrailingIdx - 1]) : (inClose[ShadowLongTrailingIdx - 1])) - inLow[ShadowLongTrailingIdx - 1])) : 0.0)));
      FarPeriodTotalRange2 = ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
      FarPeriodTotalTrailRange2 = ((Far_rangeType == 0) ? (Math.abs(inClose[FarTrailingIdx - 2] - inOpen[FarTrailingIdx - 2])) : ((Far_rangeType == 1) ? (inHigh[FarTrailingIdx - 2] - inLow[FarTrailingIdx - 2]) : ((Far_rangeType == 2) ? ((inHigh[FarTrailingIdx - 2] - (((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inClose[FarTrailingIdx - 2]) : (inOpen[FarTrailingIdx - 2]))) + ((((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inOpen[FarTrailingIdx - 2]) : (inClose[FarTrailingIdx - 2])) - inLow[FarTrailingIdx - 2])) : 0.0)));
      NearPeriodTotalRange2 = ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
      NearPeriodTotalTrailRange2 = ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) == 1 && /* 1st white */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) == 1 && /* 2nd white */
             ((inClose[i] >= inOpen[i]) ? 1 : -1) == 1 &&         /* 3rd white */
             inClose[i] > inClose[i - 1] &&
             inClose[i - 1] > inClose[i - 2] &&                   /* consecutive higher closes */
             inOpen[i - 1] > inOpen[i - 2] &&                     /* 2nd opens within/near 1st real body */
             inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i] > inOpen[i - 1] &&                         /* 3rd opens within/near 2nd real body */
             inOpen[i] <= inClose[i - 1] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
             (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: short upper shadow */
             (Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && /* ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance */
               Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ||
              Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || /* 3 far smaller than 2 advance blocked with the 3rd */
              Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && /* ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows */
               Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) &&
               ((inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ||
                (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) ||
              Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && /* ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
               (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) )
         {
            outInteger[outIdx++] = -100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         ShadowShortPeriodTotalRange0 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         ShadowShortPeriodTotalTrailRange0 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowShortTrailingIdx] - inOpen[ShadowShortTrailingIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowShortTrailingIdx] - inLow[ShadowShortTrailingIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowShortTrailingIdx] - (((inClose[ShadowShortTrailingIdx]) >= (inOpen[ShadowShortTrailingIdx])) ? (inClose[ShadowShortTrailingIdx]) : (inOpen[ShadowShortTrailingIdx]))) + ((((inClose[ShadowShortTrailingIdx]) >= (inOpen[ShadowShortTrailingIdx])) ? (inOpen[ShadowShortTrailingIdx]) : (inClose[ShadowShortTrailingIdx])) - inLow[ShadowShortTrailingIdx])) : 0.0)));
         ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + (ShadowShortPeriodTotalRange2 - ShadowShortPeriodTotalTrailRange2);
         ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + (ShadowShortPeriodTotalRange1 - ShadowShortPeriodTotalTrailRange1);
         ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + (ShadowShortPeriodTotalRange0 - ShadowShortPeriodTotalTrailRange0);
         ShadowShortPeriodTotalRange2 = ShadowShortPeriodTotalRange1;
         ShadowShortPeriodTotalTrailRange2 = ShadowShortPeriodTotalTrailRange1;
         ShadowShortPeriodTotalRange1 = ShadowShortPeriodTotalRange0;
         ShadowShortPeriodTotalTrailRange1 = ShadowShortPeriodTotalTrailRange0;
         ShadowLongPeriodTotalRange0 = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         ShadowLongPeriodTotalTrailRange0 = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx] - (((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inClose[ShadowLongTrailingIdx]) : (inOpen[ShadowLongTrailingIdx]))) + ((((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inOpen[ShadowLongTrailingIdx]) : (inClose[ShadowLongTrailingIdx])) - inLow[ShadowLongTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + (ShadowLongPeriodTotalRange1 - ShadowLongPeriodTotalTrailRange1);
         ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + (ShadowLongPeriodTotalRange0 - ShadowLongPeriodTotalTrailRange0);
         ShadowLongPeriodTotalRange1 = ShadowLongPeriodTotalRange0;
         ShadowLongPeriodTotalTrailRange1 = ShadowLongPeriodTotalTrailRange0;
         FarPeriodTotalRange1 = ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         FarPeriodTotalTrailRange1 = ((Far_rangeType == 0) ? (Math.abs(inClose[FarTrailingIdx - 1] - inOpen[FarTrailingIdx - 1])) : ((Far_rangeType == 1) ? (inHigh[FarTrailingIdx - 1] - inLow[FarTrailingIdx - 1]) : ((Far_rangeType == 2) ? ((inHigh[FarTrailingIdx - 1] - (((inClose[FarTrailingIdx - 1]) >= (inOpen[FarTrailingIdx - 1])) ? (inClose[FarTrailingIdx - 1]) : (inOpen[FarTrailingIdx - 1]))) + ((((inClose[FarTrailingIdx - 1]) >= (inOpen[FarTrailingIdx - 1])) ? (inOpen[FarTrailingIdx - 1]) : (inClose[FarTrailingIdx - 1])) - inLow[FarTrailingIdx - 1])) : 0.0)));
         FarPeriodTotal[2] = FarPeriodTotal[2] + (FarPeriodTotalRange2 - FarPeriodTotalTrailRange2);
         FarPeriodTotal[1] = FarPeriodTotal[1] + (FarPeriodTotalRange1 - FarPeriodTotalTrailRange1);
         FarPeriodTotalRange2 = FarPeriodTotalRange1;
         FarPeriodTotalTrailRange2 = FarPeriodTotalTrailRange1;
         NearPeriodTotalRange1 = ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         NearPeriodTotalTrailRange1 = ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + (NearPeriodTotalRange2 - NearPeriodTotalTrailRange2);
         NearPeriodTotal[1] = NearPeriodTotal[1] + (NearPeriodTotalRange1 - NearPeriodTotalTrailRange1);
         NearPeriodTotalRange2 = NearPeriodTotalRange1;
         NearPeriodTotalTrailRange2 = NearPeriodTotalTrailRange1;
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
      return RetCode.SUCCESS ;
   }
   RetCode cdladvanceblockImpl( int startIdx,
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
      double ShadowShortPeriodTotalRange0 = 0;
      double ShadowShortPeriodTotalTrailRange0 = 0;
      double ShadowShortPeriodTotalRange1 = 0;
      double ShadowShortPeriodTotalTrailRange1 = 0;
      double ShadowShortPeriodTotalRange2 = 0;
      double ShadowShortPeriodTotalTrailRange2 = 0;
      double ShadowLongPeriodTotalRange0 = 0;
      double ShadowLongPeriodTotalTrailRange0 = 0;
      double ShadowLongPeriodTotalRange1 = 0;
      double ShadowLongPeriodTotalTrailRange1 = 0;
      double FarPeriodTotalRange1 = 0;
      double FarPeriodTotalTrailRange1 = 0;
      double FarPeriodTotalRange2 = 0;
      double FarPeriodTotalTrailRange2 = 0;
      double NearPeriodTotalRange1 = 0;
      double NearPeriodTotalTrailRange1 = 0;
      double NearPeriodTotalRange2 = 0;
      double NearPeriodTotalTrailRange2 = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
      int ShadowShortTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int FarTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].factor;
      int Far_rangeType = this.candleSettings[CandleSettingType.FAR.ordinal()].rangeType.ordinal();
      int Far_avgPeriod = this.candleSettings[CandleSettingType.FAR.ordinal()].avgPeriod;
      double Far_factor = this.candleSettings[CandleSettingType.FAR.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.NEAR.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.NEAR.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.NEAR.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      lookbackTotal = cdladvanceblockLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
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
      ShadowShortPeriodTotalRange1 = ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
      ShadowShortPeriodTotalTrailRange1 = ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowShortTrailingIdx - 1] - (double)inOpen[ShadowShortTrailingIdx - 1])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[ShadowShortTrailingIdx - 1] - (double)inLow[ShadowShortTrailingIdx - 1]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[ShadowShortTrailingIdx - 1] - ((((double)inClose[ShadowShortTrailingIdx - 1]) >= ((double)inOpen[ShadowShortTrailingIdx - 1])) ? ((double)inClose[ShadowShortTrailingIdx - 1]) : ((double)inOpen[ShadowShortTrailingIdx - 1]))) + (((((double)inClose[ShadowShortTrailingIdx - 1]) >= ((double)inOpen[ShadowShortTrailingIdx - 1])) ? ((double)inOpen[ShadowShortTrailingIdx - 1]) : ((double)inClose[ShadowShortTrailingIdx - 1])) - (double)inLow[ShadowShortTrailingIdx - 1])) : 0.0)));
      ShadowShortPeriodTotalRange2 = ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
      ShadowShortPeriodTotalTrailRange2 = ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowShortTrailingIdx - 2] - (double)inOpen[ShadowShortTrailingIdx - 2])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[ShadowShortTrailingIdx - 2] - (double)inLow[ShadowShortTrailingIdx - 2]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[ShadowShortTrailingIdx - 2] - ((((double)inClose[ShadowShortTrailingIdx - 2]) >= ((double)inOpen[ShadowShortTrailingIdx - 2])) ? ((double)inClose[ShadowShortTrailingIdx - 2]) : ((double)inOpen[ShadowShortTrailingIdx - 2]))) + (((((double)inClose[ShadowShortTrailingIdx - 2]) >= ((double)inOpen[ShadowShortTrailingIdx - 2])) ? ((double)inOpen[ShadowShortTrailingIdx - 2]) : ((double)inClose[ShadowShortTrailingIdx - 2])) - (double)inLow[ShadowShortTrailingIdx - 2])) : 0.0)));
      ShadowLongPeriodTotalRange1 = ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
      ShadowLongPeriodTotalTrailRange1 = ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowLongTrailingIdx - 1] - (double)inOpen[ShadowLongTrailingIdx - 1])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[ShadowLongTrailingIdx - 1] - (double)inLow[ShadowLongTrailingIdx - 1]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[ShadowLongTrailingIdx - 1] - ((((double)inClose[ShadowLongTrailingIdx - 1]) >= ((double)inOpen[ShadowLongTrailingIdx - 1])) ? ((double)inClose[ShadowLongTrailingIdx - 1]) : ((double)inOpen[ShadowLongTrailingIdx - 1]))) + (((((double)inClose[ShadowLongTrailingIdx - 1]) >= ((double)inOpen[ShadowLongTrailingIdx - 1])) ? ((double)inOpen[ShadowLongTrailingIdx - 1]) : ((double)inClose[ShadowLongTrailingIdx - 1])) - (double)inLow[ShadowLongTrailingIdx - 1])) : 0.0)));
      FarPeriodTotalRange2 = ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
      FarPeriodTotalTrailRange2 = ((Far_rangeType == 0) ? (Math.abs((double)inClose[FarTrailingIdx - 2] - (double)inOpen[FarTrailingIdx - 2])) : ((Far_rangeType == 1) ? ((double)inHigh[FarTrailingIdx - 2] - (double)inLow[FarTrailingIdx - 2]) : ((Far_rangeType == 2) ? (((double)inHigh[FarTrailingIdx - 2] - ((((double)inClose[FarTrailingIdx - 2]) >= ((double)inOpen[FarTrailingIdx - 2])) ? ((double)inClose[FarTrailingIdx - 2]) : ((double)inOpen[FarTrailingIdx - 2]))) + (((((double)inClose[FarTrailingIdx - 2]) >= ((double)inOpen[FarTrailingIdx - 2])) ? ((double)inOpen[FarTrailingIdx - 2]) : ((double)inClose[FarTrailingIdx - 2])) - (double)inLow[FarTrailingIdx - 2])) : 0.0)));
      NearPeriodTotalRange2 = ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
      NearPeriodTotalTrailRange2 = ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 2] - ((((double)inClose[NearTrailingIdx - 2]) >= ((double)inOpen[NearTrailingIdx - 2])) ? ((double)inClose[NearTrailingIdx - 2]) : ((double)inOpen[NearTrailingIdx - 2]))) + (((((double)inClose[NearTrailingIdx - 2]) >= ((double)inOpen[NearTrailingIdx - 2])) ? ((double)inOpen[NearTrailingIdx - 2]) : ((double)inClose[NearTrailingIdx - 2])) - (double)inLow[NearTrailingIdx - 2])) : 0.0)));
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : -1) == 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : -1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : -1) == 1 && (double)inClose[i] > (double)inClose[i - 1] && (double)inClose[i - 1] > (double)inClose[i - 2] && (double)inOpen[i - 1] > (double)inOpen[i - 2] && (double)inOpen[i - 1] <= (double)inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] > (double)inOpen[i - 1] && (double)inOpen[i] <= (double)inClose[i - 1] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i - 2] - (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? (double)inClose[i - 2] : (double)inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) < Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) && (((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) || ((double)inHigh[i - 1] - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? (double)inClose[i - 1] : (double)inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) || Math.abs((double)inClose[i] - (double)inOpen[i]) < Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) ) {
            outInteger[outIdx++] = -100;
         } else {
            outInteger[outIdx++] = 0;
         }
         ShadowShortPeriodTotalRange0 = ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         ShadowShortPeriodTotalTrailRange0 = ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowShortTrailingIdx] - (double)inOpen[ShadowShortTrailingIdx])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[ShadowShortTrailingIdx] - (double)inLow[ShadowShortTrailingIdx]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[ShadowShortTrailingIdx] - ((((double)inClose[ShadowShortTrailingIdx]) >= ((double)inOpen[ShadowShortTrailingIdx])) ? ((double)inClose[ShadowShortTrailingIdx]) : ((double)inOpen[ShadowShortTrailingIdx]))) + (((((double)inClose[ShadowShortTrailingIdx]) >= ((double)inOpen[ShadowShortTrailingIdx])) ? ((double)inOpen[ShadowShortTrailingIdx]) : ((double)inClose[ShadowShortTrailingIdx])) - (double)inLow[ShadowShortTrailingIdx])) : 0.0)));
         ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + (ShadowShortPeriodTotalRange2 - ShadowShortPeriodTotalTrailRange2);
         ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + (ShadowShortPeriodTotalRange1 - ShadowShortPeriodTotalTrailRange1);
         ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + (ShadowShortPeriodTotalRange0 - ShadowShortPeriodTotalTrailRange0);
         ShadowShortPeriodTotalRange2 = ShadowShortPeriodTotalRange1;
         ShadowShortPeriodTotalTrailRange2 = ShadowShortPeriodTotalTrailRange1;
         ShadowShortPeriodTotalRange1 = ShadowShortPeriodTotalRange0;
         ShadowShortPeriodTotalTrailRange1 = ShadowShortPeriodTotalTrailRange0;
         ShadowLongPeriodTotalRange0 = ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         ShadowLongPeriodTotalTrailRange0 = ((ShadowLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowLongTrailingIdx] - (double)inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? ((double)inHigh[ShadowLongTrailingIdx] - (double)inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? (((double)inHigh[ShadowLongTrailingIdx] - ((((double)inClose[ShadowLongTrailingIdx]) >= ((double)inOpen[ShadowLongTrailingIdx])) ? ((double)inClose[ShadowLongTrailingIdx]) : ((double)inOpen[ShadowLongTrailingIdx]))) + (((((double)inClose[ShadowLongTrailingIdx]) >= ((double)inOpen[ShadowLongTrailingIdx])) ? ((double)inOpen[ShadowLongTrailingIdx]) : ((double)inClose[ShadowLongTrailingIdx])) - (double)inLow[ShadowLongTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + (ShadowLongPeriodTotalRange1 - ShadowLongPeriodTotalTrailRange1);
         ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + (ShadowLongPeriodTotalRange0 - ShadowLongPeriodTotalTrailRange0);
         ShadowLongPeriodTotalRange1 = ShadowLongPeriodTotalRange0;
         ShadowLongPeriodTotalTrailRange1 = ShadowLongPeriodTotalTrailRange0;
         FarPeriodTotalRange1 = ((Far_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Far_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Far_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         FarPeriodTotalTrailRange1 = ((Far_rangeType == 0) ? (Math.abs((double)inClose[FarTrailingIdx - 1] - (double)inOpen[FarTrailingIdx - 1])) : ((Far_rangeType == 1) ? ((double)inHigh[FarTrailingIdx - 1] - (double)inLow[FarTrailingIdx - 1]) : ((Far_rangeType == 2) ? (((double)inHigh[FarTrailingIdx - 1] - ((((double)inClose[FarTrailingIdx - 1]) >= ((double)inOpen[FarTrailingIdx - 1])) ? ((double)inClose[FarTrailingIdx - 1]) : ((double)inOpen[FarTrailingIdx - 1]))) + (((((double)inClose[FarTrailingIdx - 1]) >= ((double)inOpen[FarTrailingIdx - 1])) ? ((double)inOpen[FarTrailingIdx - 1]) : ((double)inClose[FarTrailingIdx - 1])) - (double)inLow[FarTrailingIdx - 1])) : 0.0)));
         FarPeriodTotal[2] = FarPeriodTotal[2] + (FarPeriodTotalRange2 - FarPeriodTotalTrailRange2);
         FarPeriodTotal[1] = FarPeriodTotal[1] + (FarPeriodTotalRange1 - FarPeriodTotalTrailRange1);
         FarPeriodTotalRange2 = FarPeriodTotalRange1;
         FarPeriodTotalTrailRange2 = FarPeriodTotalTrailRange1;
         NearPeriodTotalRange1 = ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         NearPeriodTotalTrailRange1 = ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 1] - (double)inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 1] - (double)inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 1] - ((((double)inClose[NearTrailingIdx - 1]) >= ((double)inOpen[NearTrailingIdx - 1])) ? ((double)inClose[NearTrailingIdx - 1]) : ((double)inOpen[NearTrailingIdx - 1]))) + (((((double)inClose[NearTrailingIdx - 1]) >= ((double)inOpen[NearTrailingIdx - 1])) ? ((double)inOpen[NearTrailingIdx - 1]) : ((double)inClose[NearTrailingIdx - 1])) - (double)inLow[NearTrailingIdx - 1])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + (NearPeriodTotalRange2 - NearPeriodTotalTrailRange2);
         NearPeriodTotal[1] = NearPeriodTotal[1] + (NearPeriodTotalRange1 - NearPeriodTotalTrailRange1);
         NearPeriodTotalRange2 = NearPeriodTotalRange1;
         NearPeriodTotalTrailRange2 = NearPeriodTotalTrailRange1;
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
      return RetCode.SUCCESS ;
   }
   /**
    * Three-candle bearish reversal pattern: three white candles with
    * consecutively higher closes whose advance weakens (progressively smaller
    * bodies and/or lengthening upper shadows). Signals that an uptrend's
    * advance is being blocked. A hit (-100) is bearish: the advance is
    * stalling/blocked; meaningful mainly within an existing uptrend.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cdladvanceblock">ta-lib.org/functions/cdladvanceblock</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior uptrend the pattern classically assumes for significance.</li>
    * <li>Although classically read as a bearish reversal, Bulkowski's testing found the Advance Block actually acts as a bullish continuation 64% of the time. (<a href="https://thepatternsite.com/AdvanceBlock.html">thepatternsite.com</a>)</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#cdladvanceblockLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger -100 on a detected pattern (always bearish), 0
    *        otherwise; never emits +100. Must hold at least
    *        {@code endIdx - max(startIdx, cdladvanceblockLookback(...)) + 1} values,
    *        and never be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#cdl3whitesoldiers
    * @see Core#cdlstalledpattern
    */
   public OutRange cdladvanceblock( int startIdx,
                                    int endIdx,
                                    double inOpen[],
                                    double inHigh[],
                                    double inLow[],
                                    double inClose[],
                                    int outInteger[] )
   {
      requireIndexRange("CDLADVANCEBLOCK", startIdx, endIdx);
      int guardStart = clampedStart("CDLADVANCEBLOCK", startIdx, cdladvanceblockLookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLADVANCEBLOCK", "inOpen", inOpen, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inHigh", inHigh, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inLow", inLow, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inClose", inClose, guardInLen);
      requireLength("CDLADVANCEBLOCK", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cdladvanceblockImpl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.SUCCESS ) {
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
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cdladvanceblock">ta-lib.org/functions/cdladvanceblock</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior uptrend the pattern classically assumes for significance.</li>
    * <li>Although classically read as a bearish reversal, Bulkowski's testing found the Advance Block actually acts as a bullish continuation 64% of the time. (<a href="https://thepatternsite.com/AdvanceBlock.html">thepatternsite.com</a>)</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#cdladvanceblockLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger -100 on a detected pattern (always bearish), 0
    *        otherwise; never emits +100. Must hold at least
    *        {@code endIdx - max(startIdx, cdladvanceblockLookback(...)) + 1} values,
    *        and never be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#cdl3whitesoldiers
    * @see Core#cdlstalledpattern
    */
   public OutRange cdladvanceblock( int startIdx,
                                    int endIdx,
                                    float inOpen[],
                                    float inHigh[],
                                    float inLow[],
                                    float inClose[],
                                    int outInteger[] )
   {
      requireIndexRange("CDLADVANCEBLOCK", startIdx, endIdx);
      int guardStart = clampedStart("CDLADVANCEBLOCK", startIdx, cdladvanceblockLookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLADVANCEBLOCK", "inOpen", inOpen, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inHigh", inHigh, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inLow", inLow, guardInLen);
      requireLength("CDLADVANCEBLOCK", "inClose", inClose, guardInLen);
      requireLength("CDLADVANCEBLOCK", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cdladvanceblockImpl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CDLADVANCEBLOCK", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLADVANCEBLOCK stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdladvanceblock} over the same series.
    * Open with {@link Core#cdladvanceblockOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class CdladvanceblockStream {
      private Core core;
      private double[] ShadowShortPeriodTotal;
      private double[] ShadowLongPeriodTotal;
      private double[] NearPeriodTotal;
      private double[] FarPeriodTotal;
      private double ShadowShortPeriodTotalRange1;
      private double ShadowShortPeriodTotalTrailRange1;
      private double ShadowShortPeriodTotalRange2;
      private double ShadowShortPeriodTotalTrailRange2;
      private double ShadowLongPeriodTotalRange1;
      private double ShadowLongPeriodTotalTrailRange1;
      private double FarPeriodTotalRange2;
      private double FarPeriodTotalTrailRange2;
      private double NearPeriodTotalRange2;
      private double NearPeriodTotalTrailRange2;
      private double BodyLongPeriodTotal;
      private double lag1_inOpen;
      private double lag2_inOpen;
      private double lag1_inHigh;
      private double lag2_inHigh;
      private double lag1_inLow;
      private double lag2_inLow;
      private double lag1_inClose;
      private double lag2_inClose;
      private int ringPos_BodyLongTrailingIdx;
      private int ringCap_BodyLongTrailingIdx;
      private int ringLag_BodyLongTrailingIdx;
      private double[] ring_BodyLongTrailingIdx_derived;
      private int ringPos_FarTrailingIdx;
      private int ringCap_FarTrailingIdx;
      private int ringLag_FarTrailingIdx;
      private double[] ring_FarTrailingIdx_derived;
      private int ringPos_NearTrailingIdx;
      private int ringCap_NearTrailingIdx;
      private int ringLag_NearTrailingIdx;
      private double[] ring_NearTrailingIdx_derived;
      private int ringPos_ShadowLongTrailingIdx;
      private int ringCap_ShadowLongTrailingIdx;
      private double[] ring_ShadowLongTrailingIdx_derived;
      private int ringPos_ShadowShortTrailingIdx;
      private int ringCap_ShadowShortTrailingIdx;
      private double[] ring_ShadowShortTrailingIdx_derived;
      private int cs_BodyLong_rangeType;
      private int cs_BodyLong_avgPeriod;
      private double cs_BodyLong_factor;
      private int cs_Far_rangeType;
      private int cs_Far_avgPeriod;
      private double cs_Far_factor;
      private int cs_Near_rangeType;
      private int cs_Near_avgPeriod;
      private double cs_Near_factor;
      private int cs_ShadowLong_rangeType;
      private int cs_ShadowLong_avgPeriod;
      private double cs_ShadowLong_factor;
      private int cs_ShadowShort_rangeType;
      private int cs_ShadowShort_avgPeriod;
      private double cs_ShadowShort_factor;
      private int cur_outInteger;
      private int outRangeBegIdx;
      private int outRangeCount;

      private CdladvanceblockStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#cdladvanceblock} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("CDLADVANCEBLOCK advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private CdladvanceblockStream( CdladvanceblockStream other ) {
         this.core = other.core;
         this.ShadowShortPeriodTotal = other.ShadowShortPeriodTotal.clone();
         this.ShadowLongPeriodTotal = other.ShadowLongPeriodTotal.clone();
         this.NearPeriodTotal = other.NearPeriodTotal.clone();
         this.FarPeriodTotal = other.FarPeriodTotal.clone();
         this.ShadowShortPeriodTotalRange1 = other.ShadowShortPeriodTotalRange1;
         this.ShadowShortPeriodTotalTrailRange1 = other.ShadowShortPeriodTotalTrailRange1;
         this.ShadowShortPeriodTotalRange2 = other.ShadowShortPeriodTotalRange2;
         this.ShadowShortPeriodTotalTrailRange2 = other.ShadowShortPeriodTotalTrailRange2;
         this.ShadowLongPeriodTotalRange1 = other.ShadowLongPeriodTotalRange1;
         this.ShadowLongPeriodTotalTrailRange1 = other.ShadowLongPeriodTotalTrailRange1;
         this.FarPeriodTotalRange2 = other.FarPeriodTotalRange2;
         this.FarPeriodTotalTrailRange2 = other.FarPeriodTotalTrailRange2;
         this.NearPeriodTotalRange2 = other.NearPeriodTotalRange2;
         this.NearPeriodTotalTrailRange2 = other.NearPeriodTotalTrailRange2;
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
         this.ring_ShadowLongTrailingIdx_derived = other.ring_ShadowLongTrailingIdx_derived.clone();
         this.ringPos_ShadowShortTrailingIdx = other.ringPos_ShadowShortTrailingIdx;
         this.ringCap_ShadowShortTrailingIdx = other.ringCap_ShadowShortTrailingIdx;
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

      /**
       * Commit one closed bar, returning the new current value.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("CDLADVANCEBLOCK update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("CDLADVANCEBLOCK update", !Double.isFinite(inOpen) ? "inOpen" : !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.cdladvanceblockStepImpl(this, inOpen, inHigh, inLow, inClose);
         this.outRangeCount++;
         return this.cur_outInteger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("CDLADVANCEBLOCK peek", !Double.isFinite(inOpen) ? "inOpen" : !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         CdladvanceblockStream sp = this;
         int cur_outInteger = 0;
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
         if( ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) == 1 && /* 1st white */
             ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) == 1 && /* 2nd white */
             ((inClose >= inOpen) ? 1 : -1) == 1 &&                 /* 3rd white */
             inClose > sp.lag1_inClose &&
             sp.lag1_inClose > sp.lag2_inClose &&                   /* consecutive higher closes */
             sp.lag1_inOpen > sp.lag2_inOpen &&                     /* 2nd opens within/near 1st real body */
             sp.lag1_inOpen <= sp.lag2_inClose + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen > sp.lag1_inOpen &&                             /* 3rd opens within/near 2nd real body */
             inOpen <= sp.lag1_inClose + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
             (sp.lag2_inHigh - ((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inClose : sp.lag2_inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowShort_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowShort_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: short upper shadow */
             (Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) - ((Far_factor * (((Far_avgPeriod != 0) ? (sp.FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Far_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Far_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && /* ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance */
               Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ||
              Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Far_factor * (((Far_avgPeriod != 0) ? (sp.FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Far_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Far_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || /* 3 far smaller than 2 advance blocked with the 3rd */
              Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && /* ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows */
               Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) &&
               ((inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ||
                (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowShort_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowShort_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) ||
              Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && /* ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
               (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (sp.ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) )
         {
            cur_outInteger = -100;
         } else {
            cur_outInteger = 0;
         }
         return cur_outInteger;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public int value() {
         return this.cur_outInteger;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public CdladvanceblockStream clone() {
         return new CdladvanceblockStream(this);
      }
   }
   private void cdladvanceblockStepImpl( CdladvanceblockStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      double ShadowShortPeriodTotalRange0 = 0.0;
      double ShadowShortPeriodTotalTrailRange0 = 0.0;
      double ShadowLongPeriodTotalRange0 = 0.0;
      double ShadowLongPeriodTotalTrailRange0 = 0.0;
      double FarPeriodTotalRange1 = 0.0;
      double FarPeriodTotalTrailRange1 = 0.0;
      double NearPeriodTotalRange1 = 0.0;
      double NearPeriodTotalTrailRange1 = 0.0;
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
      if( sp.ringCap_ShadowLongTrailingIdx == 0 ) {
         sp.ring_ShadowLongTrailingIdx_derived[0] = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      }
      if( sp.ringCap_ShadowShortTrailingIdx == 0 ) {
         sp.ring_ShadowShortTrailingIdx_derived[0] = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      }
      if( ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : -1) == 1 && /* 1st white */
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : -1) == 1 && /* 2nd white */
          ((inClose >= inOpen) ? 1 : -1) == 1 &&                 /* 3rd white */
          inClose > sp.lag1_inClose &&
          sp.lag1_inClose > sp.lag2_inClose &&                   /* consecutive higher closes */
          sp.lag1_inOpen > sp.lag2_inOpen &&                     /* 2nd opens within/near 1st real body */
          sp.lag1_inOpen <= sp.lag2_inClose + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          inOpen > sp.lag1_inOpen &&                             /* 3rd opens within/near 2nd real body */
          inOpen <= sp.lag1_inClose + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          Math.abs(sp.lag2_inClose - sp.lag2_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
          (sp.lag2_inHigh - ((sp.lag2_inClose >= sp.lag2_inOpen) ? sp.lag2_inClose : sp.lag2_inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((ShadowShort_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((ShadowShort_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: short upper shadow */
          (Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) - ((Far_factor * (((Far_avgPeriod != 0) ? (sp.FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Far_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Far_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && /* ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance */
            Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ||
           Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Far_factor * (((Far_avgPeriod != 0) ? (sp.FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Far_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Far_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || /* 3 far smaller than 2 advance blocked with the 3rd */
           Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && /* ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows */
            Math.abs(sp.lag1_inClose - sp.lag1_inOpen) < Math.abs(sp.lag2_inClose - sp.lag2_inOpen) &&
            ((inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ||
             (sp.lag1_inHigh - ((sp.lag1_inClose >= sp.lag1_inOpen) ? sp.lag1_inClose : sp.lag1_inOpen)) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((ShadowShort_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((ShadowShort_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) ||
           Math.abs(inClose - inOpen) < Math.abs(sp.lag1_inClose - sp.lag1_inOpen) && /* ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
            (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (sp.ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) )
      {
         sp.cur_outInteger = -100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      ShadowShortPeriodTotalRange0 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      ShadowShortPeriodTotalTrailRange0 = sp.ring_ShadowShortTrailingIdx_derived[sp.ringPos_ShadowShortTrailingIdx];
      sp.ShadowShortPeriodTotal[2] = sp.ShadowShortPeriodTotal[2] + (sp.ShadowShortPeriodTotalRange2 - sp.ShadowShortPeriodTotalTrailRange2);
      sp.ShadowShortPeriodTotal[1] = sp.ShadowShortPeriodTotal[1] + (sp.ShadowShortPeriodTotalRange1 - sp.ShadowShortPeriodTotalTrailRange1);
      sp.ShadowShortPeriodTotal[0] = sp.ShadowShortPeriodTotal[0] + (ShadowShortPeriodTotalRange0 - ShadowShortPeriodTotalTrailRange0);
      sp.ShadowShortPeriodTotalRange2 = sp.ShadowShortPeriodTotalRange1;
      sp.ShadowShortPeriodTotalTrailRange2 = sp.ShadowShortPeriodTotalTrailRange1;
      sp.ShadowShortPeriodTotalRange1 = ShadowShortPeriodTotalRange0;
      sp.ShadowShortPeriodTotalTrailRange1 = ShadowShortPeriodTotalTrailRange0;
      ShadowLongPeriodTotalRange0 = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      ShadowLongPeriodTotalTrailRange0 = sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx];
      sp.ShadowLongPeriodTotal[1] = sp.ShadowLongPeriodTotal[1] + (sp.ShadowLongPeriodTotalRange1 - sp.ShadowLongPeriodTotalTrailRange1);
      sp.ShadowLongPeriodTotal[0] = sp.ShadowLongPeriodTotal[0] + (ShadowLongPeriodTotalRange0 - ShadowLongPeriodTotalTrailRange0);
      sp.ShadowLongPeriodTotalRange1 = ShadowLongPeriodTotalRange0;
      sp.ShadowLongPeriodTotalTrailRange1 = ShadowLongPeriodTotalTrailRange0;
      FarPeriodTotalRange1 = ((Far_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Far_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Far_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)));
      FarPeriodTotalTrailRange1 = sp.ring_FarTrailingIdx_derived[(sp.ringPos_FarTrailingIdx + sp.ringCap_FarTrailingIdx - sp.ringLag_FarTrailingIdx - 1) % sp.ringCap_FarTrailingIdx];
      sp.FarPeriodTotal[2] = sp.FarPeriodTotal[2] + (sp.FarPeriodTotalRange2 - sp.FarPeriodTotalTrailRange2);
      sp.FarPeriodTotal[1] = sp.FarPeriodTotal[1] + (FarPeriodTotalRange1 - FarPeriodTotalTrailRange1);
      sp.FarPeriodTotalRange2 = FarPeriodTotalRange1;
      sp.FarPeriodTotalTrailRange2 = FarPeriodTotalTrailRange1;
      NearPeriodTotalRange1 = ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)));
      NearPeriodTotalTrailRange1 = sp.ring_NearTrailingIdx_derived[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 1) % sp.ringCap_NearTrailingIdx];
      sp.NearPeriodTotal[2] = sp.NearPeriodTotal[2] + (sp.NearPeriodTotalRange2 - sp.NearPeriodTotalTrailRange2);
      sp.NearPeriodTotal[1] = sp.NearPeriodTotal[1] + (NearPeriodTotalRange1 - NearPeriodTotalTrailRange1);
      sp.NearPeriodTotalRange2 = NearPeriodTotalRange1;
      sp.NearPeriodTotalTrailRange2 = NearPeriodTotalTrailRange1;
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
      sp.ring_ShadowLongTrailingIdx_derived[sp.ringPos_ShadowLongTrailingIdx] = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ringPos_ShadowLongTrailingIdx = sp.ringPos_ShadowLongTrailingIdx + 1;
      if( sp.ringPos_ShadowLongTrailingIdx >= sp.ringCap_ShadowLongTrailingIdx ) {
         sp.ringPos_ShadowLongTrailingIdx = 0;
      }
      sp.ring_ShadowShortTrailingIdx_derived[sp.ringPos_ShadowShortTrailingIdx] = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ringPos_ShadowShortTrailingIdx = sp.ringPos_ShadowShortTrailingIdx + 1;
      if( sp.ringPos_ShadowShortTrailingIdx >= sp.ringCap_ShadowShortTrailingIdx ) {
         sp.ringPos_ShadowShortTrailingIdx = 0;
      }
   }
   private RetCode cdladvanceblockOpenImpl( CdladvanceblockStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double[] ShadowShortPeriodTotal = new double[3];
      double[] ShadowLongPeriodTotal = new double[2];
      double[] NearPeriodTotal = new double[3];
      double[] FarPeriodTotal = new double[3];
      double ShadowShortPeriodTotalRange0 = 0;
      double ShadowShortPeriodTotalTrailRange0 = 0;
      double ShadowShortPeriodTotalRange1 = 0;
      double ShadowShortPeriodTotalTrailRange1 = 0;
      double ShadowShortPeriodTotalRange2 = 0;
      double ShadowShortPeriodTotalTrailRange2 = 0;
      double ShadowLongPeriodTotalRange0 = 0;
      double ShadowLongPeriodTotalTrailRange0 = 0;
      double ShadowLongPeriodTotalRange1 = 0;
      double ShadowLongPeriodTotalTrailRange1 = 0;
      double FarPeriodTotalRange1 = 0;
      double FarPeriodTotalTrailRange1 = 0;
      double FarPeriodTotalRange2 = 0;
      double FarPeriodTotalTrailRange2 = 0;
      double NearPeriodTotalRange1 = 0;
      double NearPeriodTotalTrailRange1 = 0;
      double NearPeriodTotalRange2 = 0;
      double NearPeriodTotalTrailRange2 = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
      int ShadowShortTrailingIdx = 0;
      int ShadowLongTrailingIdx = 0;
      int NearTrailingIdx = 0;
      int FarTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx >= historyLen ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BODY_LONG.ordinal()].factor;
      int Far_rangeType = this.candleSettings[CandleSettingType.FAR.ordinal()].rangeType.ordinal();
      int Far_avgPeriod = this.candleSettings[CandleSettingType.FAR.ordinal()].avgPeriod;
      double Far_factor = this.candleSettings[CandleSettingType.FAR.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.NEAR.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.NEAR.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.NEAR.ordinal()].factor;
      int ShadowLong_rangeType = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].rangeType.ordinal();
      int ShadowLong_avgPeriod = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].avgPeriod;
      double ShadowLong_factor = this.candleSettings[CandleSettingType.SHADOW_LONG.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.SHADOW_SHORT.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdladvanceblockLookback();
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
         return RetCode.INSUFFICIENT_HISTORY ;
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
       * The meanings of "long body", "short shadow", "far" and "near" are specified with TA_SetCandleSettings
       */
      outIdx = 0;
      ShadowShortPeriodTotalRange1 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
      ShadowShortPeriodTotalTrailRange1 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - 1] - inOpen[ShadowShortTrailingIdx - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowShortTrailingIdx - 1] - inLow[ShadowShortTrailingIdx - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowShortTrailingIdx - 1] - (((inClose[ShadowShortTrailingIdx - 1]) >= (inOpen[ShadowShortTrailingIdx - 1])) ? (inClose[ShadowShortTrailingIdx - 1]) : (inOpen[ShadowShortTrailingIdx - 1]))) + ((((inClose[ShadowShortTrailingIdx - 1]) >= (inOpen[ShadowShortTrailingIdx - 1])) ? (inOpen[ShadowShortTrailingIdx - 1]) : (inClose[ShadowShortTrailingIdx - 1])) - inLow[ShadowShortTrailingIdx - 1])) : 0.0)));
      ShadowShortPeriodTotalRange2 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
      ShadowShortPeriodTotalTrailRange2 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowShortTrailingIdx - 2] - inOpen[ShadowShortTrailingIdx - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowShortTrailingIdx - 2] - inLow[ShadowShortTrailingIdx - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowShortTrailingIdx - 2] - (((inClose[ShadowShortTrailingIdx - 2]) >= (inOpen[ShadowShortTrailingIdx - 2])) ? (inClose[ShadowShortTrailingIdx - 2]) : (inOpen[ShadowShortTrailingIdx - 2]))) + ((((inClose[ShadowShortTrailingIdx - 2]) >= (inOpen[ShadowShortTrailingIdx - 2])) ? (inOpen[ShadowShortTrailingIdx - 2]) : (inClose[ShadowShortTrailingIdx - 2])) - inLow[ShadowShortTrailingIdx - 2])) : 0.0)));
      ShadowLongPeriodTotalRange1 = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
      ShadowLongPeriodTotalTrailRange1 = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx - 1] - inOpen[ShadowLongTrailingIdx - 1])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx - 1] - inLow[ShadowLongTrailingIdx - 1]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx - 1] - (((inClose[ShadowLongTrailingIdx - 1]) >= (inOpen[ShadowLongTrailingIdx - 1])) ? (inClose[ShadowLongTrailingIdx - 1]) : (inOpen[ShadowLongTrailingIdx - 1]))) + ((((inClose[ShadowLongTrailingIdx - 1]) >= (inOpen[ShadowLongTrailingIdx - 1])) ? (inOpen[ShadowLongTrailingIdx - 1]) : (inClose[ShadowLongTrailingIdx - 1])) - inLow[ShadowLongTrailingIdx - 1])) : 0.0)));
      FarPeriodTotalRange2 = ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
      FarPeriodTotalTrailRange2 = ((Far_rangeType == 0) ? (Math.abs(inClose[FarTrailingIdx - 2] - inOpen[FarTrailingIdx - 2])) : ((Far_rangeType == 1) ? (inHigh[FarTrailingIdx - 2] - inLow[FarTrailingIdx - 2]) : ((Far_rangeType == 2) ? ((inHigh[FarTrailingIdx - 2] - (((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inClose[FarTrailingIdx - 2]) : (inOpen[FarTrailingIdx - 2]))) + ((((inClose[FarTrailingIdx - 2]) >= (inOpen[FarTrailingIdx - 2])) ? (inOpen[FarTrailingIdx - 2]) : (inClose[FarTrailingIdx - 2])) - inLow[FarTrailingIdx - 2])) : 0.0)));
      NearPeriodTotalRange2 = ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
      NearPeriodTotalTrailRange2 = ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : -1) == 1 && /* 1st white */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : -1) == 1 && /* 2nd white */
             ((inClose[i] >= inOpen[i]) ? 1 : -1) == 1 &&         /* 3rd white */
             inClose[i] > inClose[i - 1] &&
             inClose[i - 1] > inClose[i - 2] &&                   /* consecutive higher closes */
             inOpen[i - 1] > inOpen[i - 2] &&                     /* 2nd opens within/near 1st real body */
             inOpen[i - 1] <= inClose[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i] > inOpen[i - 1] &&                         /* 3rd opens within/near 2nd real body */
             inOpen[i] <= inClose[i - 1] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             Math.abs(inClose[i - 2] - inOpen[i - 2]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
             (inHigh[i - 2] - ((inClose[i - 2] >= inOpen[i - 2]) ? inClose[i - 2] : inOpen[i - 2])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[2] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: short upper shadow */
             (Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[2] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Far_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Far_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) && /* ( 2 far smaller than 1 && 3 not longer than 2 ) advance blocked with the 2nd, 3rd must not carry on the advance */
               Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[1] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ||
              Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Far_factor * (((Far_avgPeriod != 0) ? (FarPeriodTotal[1] / Far_avgPeriod) : ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Far_rangeType == 2) ? 2.0 : 1.0)))) || /* 3 far smaller than 2 advance blocked with the 3rd */
              Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && /* ( 3 smaller than 2 && 2 smaller than 1 && (3 or 2 not short upper shadow) ) advance blocked with progressively smaller real bodies and some upper shadows */
               Math.abs(inClose[i - 1] - inOpen[i - 1]) < Math.abs(inClose[i - 2] - inOpen[i - 2]) &&
               ((inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[0] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ||
                (inHigh[i - 1] - ((inClose[i - 1] >= inOpen[i - 1]) ? inClose[i - 1] : inOpen[i - 1])) > ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowShortPeriodTotal[1] / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((ShadowShort_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0))))) ||
              Math.abs(inClose[i] - inOpen[i]) < Math.abs(inClose[i - 1] - inOpen[i - 1]) && /* ( 3 smaller than 2 && 3 long upper shadow ) advance blocked with 3rd candle's long upper shadow and smaller body */
               (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowLong_factor * (((ShadowLong_avgPeriod != 0) ? (ShadowLongPeriodTotal[0] / ShadowLong_avgPeriod) : ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowLong_rangeType == 2) ? 2.0 : 1.0))))) )
         {
            outInteger[outIdx++ * outStride] = -100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         ShadowShortPeriodTotalRange0 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         ShadowShortPeriodTotalTrailRange0 = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowShortTrailingIdx] - inOpen[ShadowShortTrailingIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowShortTrailingIdx] - inLow[ShadowShortTrailingIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowShortTrailingIdx] - (((inClose[ShadowShortTrailingIdx]) >= (inOpen[ShadowShortTrailingIdx])) ? (inClose[ShadowShortTrailingIdx]) : (inOpen[ShadowShortTrailingIdx]))) + ((((inClose[ShadowShortTrailingIdx]) >= (inOpen[ShadowShortTrailingIdx])) ? (inOpen[ShadowShortTrailingIdx]) : (inClose[ShadowShortTrailingIdx])) - inLow[ShadowShortTrailingIdx])) : 0.0)));
         ShadowShortPeriodTotal[2] = ShadowShortPeriodTotal[2] + (ShadowShortPeriodTotalRange2 - ShadowShortPeriodTotalTrailRange2);
         ShadowShortPeriodTotal[1] = ShadowShortPeriodTotal[1] + (ShadowShortPeriodTotalRange1 - ShadowShortPeriodTotalTrailRange1);
         ShadowShortPeriodTotal[0] = ShadowShortPeriodTotal[0] + (ShadowShortPeriodTotalRange0 - ShadowShortPeriodTotalTrailRange0);
         ShadowShortPeriodTotalRange2 = ShadowShortPeriodTotalRange1;
         ShadowShortPeriodTotalTrailRange2 = ShadowShortPeriodTotalTrailRange1;
         ShadowShortPeriodTotalRange1 = ShadowShortPeriodTotalRange0;
         ShadowShortPeriodTotalTrailRange1 = ShadowShortPeriodTotalTrailRange0;
         ShadowLongPeriodTotalRange0 = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         ShadowLongPeriodTotalTrailRange0 = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[ShadowLongTrailingIdx] - inOpen[ShadowLongTrailingIdx])) : ((ShadowLong_rangeType == 1) ? (inHigh[ShadowLongTrailingIdx] - inLow[ShadowLongTrailingIdx]) : ((ShadowLong_rangeType == 2) ? ((inHigh[ShadowLongTrailingIdx] - (((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inClose[ShadowLongTrailingIdx]) : (inOpen[ShadowLongTrailingIdx]))) + ((((inClose[ShadowLongTrailingIdx]) >= (inOpen[ShadowLongTrailingIdx])) ? (inOpen[ShadowLongTrailingIdx]) : (inClose[ShadowLongTrailingIdx])) - inLow[ShadowLongTrailingIdx])) : 0.0)));
         ShadowLongPeriodTotal[1] = ShadowLongPeriodTotal[1] + (ShadowLongPeriodTotalRange1 - ShadowLongPeriodTotalTrailRange1);
         ShadowLongPeriodTotal[0] = ShadowLongPeriodTotal[0] + (ShadowLongPeriodTotalRange0 - ShadowLongPeriodTotalTrailRange0);
         ShadowLongPeriodTotalRange1 = ShadowLongPeriodTotalRange0;
         ShadowLongPeriodTotalTrailRange1 = ShadowLongPeriodTotalTrailRange0;
         FarPeriodTotalRange1 = ((Far_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Far_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Far_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         FarPeriodTotalTrailRange1 = ((Far_rangeType == 0) ? (Math.abs(inClose[FarTrailingIdx - 1] - inOpen[FarTrailingIdx - 1])) : ((Far_rangeType == 1) ? (inHigh[FarTrailingIdx - 1] - inLow[FarTrailingIdx - 1]) : ((Far_rangeType == 2) ? ((inHigh[FarTrailingIdx - 1] - (((inClose[FarTrailingIdx - 1]) >= (inOpen[FarTrailingIdx - 1])) ? (inClose[FarTrailingIdx - 1]) : (inOpen[FarTrailingIdx - 1]))) + ((((inClose[FarTrailingIdx - 1]) >= (inOpen[FarTrailingIdx - 1])) ? (inOpen[FarTrailingIdx - 1]) : (inClose[FarTrailingIdx - 1])) - inLow[FarTrailingIdx - 1])) : 0.0)));
         FarPeriodTotal[2] = FarPeriodTotal[2] + (FarPeriodTotalRange2 - FarPeriodTotalTrailRange2);
         FarPeriodTotal[1] = FarPeriodTotal[1] + (FarPeriodTotalRange1 - FarPeriodTotalTrailRange1);
         FarPeriodTotalRange2 = FarPeriodTotalRange1;
         FarPeriodTotalTrailRange2 = FarPeriodTotalTrailRange1;
         NearPeriodTotalRange1 = ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         NearPeriodTotalTrailRange1 = ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + (NearPeriodTotalRange2 - NearPeriodTotalTrailRange2);
         NearPeriodTotal[1] = NearPeriodTotal[1] + (NearPeriodTotalRange1 - NearPeriodTotalTrailRange1);
         NearPeriodTotalRange2 = NearPeriodTotalRange1;
         NearPeriodTotalTrailRange2 = NearPeriodTotalTrailRange1;
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
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_BodyLongTrailingIdx = (cap_BodyLongTrailingIdx > 0)? cap_BodyLongTrailingIdx : 1;
      double[] capRing_BodyLongTrailingIdx_derived = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int capLag_FarTrailingIdx = i - FarTrailingIdx;
      int cap_FarTrailingIdx = capLag_FarTrailingIdx + 2;
      if( capLag_FarTrailingIdx < 0 || cap_FarTrailingIdx > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_FarTrailingIdx = (cap_FarTrailingIdx > 0)? cap_FarTrailingIdx : 1;
      double[] capRing_FarTrailingIdx_derived = new double[allocN_FarTrailingIdx];
      for( int fillJ = historyLen - cap_FarTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_FarTrailingIdx_derived[fillJ % cap_FarTrailingIdx] = ((Far_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Far_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Far_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int capLag_NearTrailingIdx = i - NearTrailingIdx;
      int cap_NearTrailingIdx = capLag_NearTrailingIdx + 2;
      if( capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_NearTrailingIdx = (cap_NearTrailingIdx > 0)? cap_NearTrailingIdx : 1;
      double[] capRing_NearTrailingIdx_derived = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int cap_ShadowLongTrailingIdx = i - ShadowLongTrailingIdx;
      if( cap_ShadowLongTrailingIdx < 0 || cap_ShadowLongTrailingIdx > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_ShadowLongTrailingIdx = (cap_ShadowLongTrailingIdx > 0)? cap_ShadowLongTrailingIdx : 1;
      double[] capRing_ShadowLongTrailingIdx_derived = new double[allocN_ShadowLongTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowLongTrailingIdx_derived[fillJ - (historyLen - cap_ShadowLongTrailingIdx)] = ((ShadowLong_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowLong_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowLong_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int cap_ShadowShortTrailingIdx = i - ShadowShortTrailingIdx;
      if( cap_ShadowShortTrailingIdx < 0 || cap_ShadowShortTrailingIdx > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_ShadowShortTrailingIdx = (cap_ShadowShortTrailingIdx > 0)? cap_ShadowShortTrailingIdx : 1;
      double[] capRing_ShadowShortTrailingIdx_derived = new double[allocN_ShadowShortTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowShortTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowShortTrailingIdx_derived[fillJ - (historyLen - cap_ShadowShortTrailingIdx)] = ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowShort_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowShort_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.ShadowShortPeriodTotal = ShadowShortPeriodTotal;
      sp.ShadowLongPeriodTotal = ShadowLongPeriodTotal;
      sp.NearPeriodTotal = NearPeriodTotal;
      sp.FarPeriodTotal = FarPeriodTotal;
      sp.ShadowShortPeriodTotalRange1 = ShadowShortPeriodTotalRange1;
      sp.ShadowShortPeriodTotalTrailRange1 = ShadowShortPeriodTotalTrailRange1;
      sp.ShadowShortPeriodTotalRange2 = ShadowShortPeriodTotalRange2;
      sp.ShadowShortPeriodTotalTrailRange2 = ShadowShortPeriodTotalTrailRange2;
      sp.ShadowLongPeriodTotalRange1 = ShadowLongPeriodTotalRange1;
      sp.ShadowLongPeriodTotalTrailRange1 = ShadowLongPeriodTotalTrailRange1;
      sp.FarPeriodTotalRange2 = FarPeriodTotalRange2;
      sp.FarPeriodTotalTrailRange2 = FarPeriodTotalTrailRange2;
      sp.NearPeriodTotalRange2 = NearPeriodTotalRange2;
      sp.NearPeriodTotalTrailRange2 = NearPeriodTotalTrailRange2;
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
      sp.ringPos_ShadowLongTrailingIdx = 0;
      sp.ringCap_ShadowLongTrailingIdx = cap_ShadowLongTrailingIdx;
      sp.ring_ShadowLongTrailingIdx_derived = capRing_ShadowLongTrailingIdx_derived;
      sp.ringPos_ShadowShortTrailingIdx = 0;
      sp.ringCap_ShadowShortTrailingIdx = cap_ShadowShortTrailingIdx;
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
      return RetCode.SUCCESS;
   }
   /* cdladvanceblockOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CdladvanceblockStream cdladvanceblockOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdladvanceblockStream sp = new CdladvanceblockStream(this);
      RetCode retCode = cdladvanceblockOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CDLADVANCEBLOCK openAndFill", inOpen.length, startIdx, cdladvanceblockLookback());
      }
      throw streamFailure("CDLADVANCEBLOCK openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind cdladvanceblockOpen (composition seam). */
   CdladvanceblockStream cdladvanceblockOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdladvanceblockStream sp = new CdladvanceblockStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdladvanceblockOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CDLADVANCEBLOCK open", inOpen.length, startIdx, cdladvanceblockLookback());
      }
      throw streamFailure("CDLADVANCEBLOCK open", retCode);
   }
   /**
    * Open a live CDLADVANCEBLOCK stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdladvanceblock} at that bar.
    * <p>The history must hold at least {@code cdladvanceblockLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown, an empty history included. A null argument throws
    * {@link IllegalArgumentException}, ahead of everything above.
    */
   public CdladvanceblockStream cdladvanceblockOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLADVANCEBLOCK open", "inOpen", inOpen);
      requireHistory("CDLADVANCEBLOCK open", inOpen.length);
      requireArgument("CDLADVANCEBLOCK open", "inHigh", inHigh);
      requireArgument("CDLADVANCEBLOCK open", "inLow", inLow);
      requireArgument("CDLADVANCEBLOCK open", "inClose", inClose);
      requireHistoryLength("CDLADVANCEBLOCK open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLADVANCEBLOCK open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLADVANCEBLOCK open", "inClose", inClose.length, inOpen.length);
      return cdladvanceblockOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdladvanceblockOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdladvanceblock} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CdladvanceblockStream#outRange()}.
    */
   public CdladvanceblockStream cdladvanceblockOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLADVANCEBLOCK openAndFill", "inOpen", inOpen);
      requireHistory("CDLADVANCEBLOCK openAndFill", inOpen.length);
      requireArgument("CDLADVANCEBLOCK openAndFill", "inHigh", inHigh);
      requireArgument("CDLADVANCEBLOCK openAndFill", "inLow", inLow);
      requireArgument("CDLADVANCEBLOCK openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLADVANCEBLOCK openAndFill", inOpen.length, cdladvanceblockLookback());
      requireHistoryLength("CDLADVANCEBLOCK openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLADVANCEBLOCK openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLADVANCEBLOCK openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLADVANCEBLOCK openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw streamFailure("CDLADVANCEBLOCK openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdladvanceblockOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
