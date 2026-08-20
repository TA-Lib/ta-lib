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
    * Number of leading input bars {@link Core#CDLGAPSIDESIDEWHITE} consumes
    * before it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLGAPSIDESIDEWHITE_Lookback( )
   {
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      return Math.max(Near_avgPeriod, Equal_avgPeriod) + 2 ;

   }
   RetCode CDLGAPSIDESIDEWHITE_Impl( int startIdx,
                                     int endIdx,
                                     double inOpen[],
                                     double inHigh[],
                                     double inLow[],
                                     double inClose[],
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     int outInteger[] )
   {
      double NearPeriodTotal = 0;
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int NearTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLGAPSIDESIDEWHITE_Lookback();
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
      NearPeriodTotal = 0;
      EqualPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - upside or downside gap (between the bodies)
       * - first candle after the window: white candlestick
       * - second candle after the window: white candlestick with similar size (near the same) and about the same
       *   open (equal) of the previous candle
       * - the second candle does not close the window
       * The meaning of "near" and "equal" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) or negative (-1 to -100): the user should consider that upside
       * or downside gap side-by-side white lines is significant when it appears in a trend, while this function
       * does not consider the trend
       */
      outIdx = 0;
      do {
         if( ((Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && (Math.min(inOpen[i], inClose[i]) > Math.max(inOpen[i - 2], inClose[i - 2])) || (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && (Math.max(inOpen[i], inClose[i]) < Math.min(inOpen[i - 2], inClose[i - 2]))) && /* upside or downside gap between the 1st candle and both the next 2 candles */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && /* 2nd: white */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&         /* 3rd: white */
             Math.abs(inClose[i] - inOpen[i]) >= Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* same size 2 and 3 */
             Math.abs(inClose[i] - inOpen[i]) <= Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i] >= inOpen[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* same open 2 and 3 */
             inOpen[i] <= inOpen[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++] = (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) ? 100 : 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
         i += 1;
         NearTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLGAPSIDESIDEWHITE_Impl( int startIdx,
                                     int endIdx,
                                     float inOpen[],
                                     float inHigh[],
                                     float inLow[],
                                     float inClose[],
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     int outInteger[] )
   {
      double NearPeriodTotal = 0;
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int NearTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLGAPSIDESIDEWHITE_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal = 0;
      EqualPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) && (Math.min((double)inOpen[i], (double)inClose[i]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) || (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2])) && (Math.max((double)inOpen[i], (double)inClose[i]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2]))) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && Math.abs((double)inClose[i] - (double)inOpen[i]) >= Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) <= Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] >= (double)inOpen[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] <= (double)inOpen[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = (Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) ? 100 : 0 - 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 1] - (double)inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 1] - (double)inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 1] - ((((double)inClose[NearTrailingIdx - 1]) >= ((double)inOpen[NearTrailingIdx - 1])) ? ((double)inClose[NearTrailingIdx - 1]) : ((double)inOpen[NearTrailingIdx - 1]))) + (((((double)inClose[NearTrailingIdx - 1]) >= ((double)inOpen[NearTrailingIdx - 1])) ? ((double)inOpen[NearTrailingIdx - 1]) : ((double)inClose[NearTrailingIdx - 1])) - (double)inLow[NearTrailingIdx - 1])) : 0.0)));
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs((double)inClose[EqualTrailingIdx - 1] - (double)inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[EqualTrailingIdx - 1] - (double)inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[EqualTrailingIdx - 1] - ((((double)inClose[EqualTrailingIdx - 1]) >= ((double)inOpen[EqualTrailingIdx - 1])) ? ((double)inClose[EqualTrailingIdx - 1]) : ((double)inOpen[EqualTrailingIdx - 1]))) + (((((double)inClose[EqualTrailingIdx - 1]) >= ((double)inOpen[EqualTrailingIdx - 1])) ? ((double)inOpen[EqualTrailingIdx - 1]) : ((double)inClose[EqualTrailingIdx - 1])) - (double)inLow[EqualTrailingIdx - 1])) : 0.0)));
         i += 1;
         NearTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A three-candle pattern: a first candle followed by two white candles of
    * similar body size that both gap the same direction (up or down) from the
    * first candle's real body and open at about the same level. It is a
    * continuation signal whose sign reports the gap direction; the code does
    * not verify a prior trend.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend the continuation signal classically assumes.</li>
    * <li>Bulkowski's data shows the bullish form is rare (984 occurrences out of 4.7 million candle lines, frequency rank 73/103) but continues as labeled 66% of the time; the bearish form is rarer still (frequency rank 86/103) and its 56% continuation rate is "near random" — Bulkowski cautions the bearish sample is too thin to trust. ([thepatternsite.com](https://thepatternsite.com/SidebySideWhiteLinesBull.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLGAPSIDESIDEWHITE_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 for an up-gap (bullish continuation), -100 for a
    *        down-gap (bearish continuation), 0 when no pattern. Sign is set solely by
    *        the C2-vs-C1 gap direction (realbodygapup ? 100 : -100) Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#CDLTASUKIGAP
    * @see Core#CDLXSIDEGAP3METHODS
    */
   public OutRange CDLGAPSIDESIDEWHITE( int startIdx,
                                        int endIdx,
                                        double inOpen[],
                                        double inHigh[],
                                        double inLow[],
                                        double inClose[],
                                        int outInteger[] )
   {
      requireIndexRange("CDLGAPSIDESIDEWHITE", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, CDLGAPSIDESIDEWHITE_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLGAPSIDESIDEWHITE", "inOpen", inOpen, guardInLen);
      requireLength("CDLGAPSIDESIDEWHITE", "inHigh", inHigh, guardInLen);
      requireLength("CDLGAPSIDESIDEWHITE", "inLow", inLow, guardInLen);
      requireLength("CDLGAPSIDESIDEWHITE", "inClose", inClose, guardInLen);
      requireLength("CDLGAPSIDESIDEWHITE", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLGAPSIDESIDEWHITE_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLGAPSIDESIDEWHITE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A three-candle pattern: a first candle followed by two white candles of
    * similar body size that both gap the same direction (up or down) from the
    * first candle's real body and open at about the same level. It is a
    * continuation signal whose sign reports the gap direction; the code does
    * not verify a prior trend.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend the continuation signal classically assumes.</li>
    * <li>Bulkowski's data shows the bullish form is rare (984 occurrences out of 4.7 million candle lines, frequency rank 73/103) but continues as labeled 66% of the time; the bearish form is rarer still (frequency rank 86/103) and its 56% continuation rate is "near random" — Bulkowski cautions the bearish sample is too thin to trust. ([thepatternsite.com](https://thepatternsite.com/SidebySideWhiteLinesBull.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLGAPSIDESIDEWHITE_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 for an up-gap (bullish continuation), -100 for a
    *        down-gap (bearish continuation), 0 when no pattern. Sign is set solely by
    *        the C2-vs-C1 gap direction (realbodygapup ? 100 : -100) Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#CDLTASUKIGAP
    * @see Core#CDLXSIDEGAP3METHODS
    */
   public OutRange CDLGAPSIDESIDEWHITE( int startIdx,
                                        int endIdx,
                                        float inOpen[],
                                        float inHigh[],
                                        float inLow[],
                                        float inClose[],
                                        int outInteger[] )
   {
      requireIndexRange("CDLGAPSIDESIDEWHITE", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, CDLGAPSIDESIDEWHITE_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLGAPSIDESIDEWHITE", "inOpen", inOpen, guardInLen);
      requireLength("CDLGAPSIDESIDEWHITE", "inHigh", inHigh, guardInLen);
      requireLength("CDLGAPSIDESIDEWHITE", "inLow", inLow, guardInLen);
      requireLength("CDLGAPSIDESIDEWHITE", "inClose", inClose, guardInLen);
      requireLength("CDLGAPSIDESIDEWHITE", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLGAPSIDESIDEWHITE_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLGAPSIDESIDEWHITE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLGAPSIDESIDEWHITE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLGAPSIDESIDEWHITE} over the same series.
    * Open with {@link Core#CDLGAPSIDESIDEWHITE_Open}; there is no close — the handle is
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
   public static final class CDLGAPSIDESIDEWHITE_Stream {
      Core core;
      double NearPeriodTotal;
      double EqualPeriodTotal;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inHigh;
      double lag1_inLow;
      double lag1_inClose;
      double lag2_inClose;
      int ringPos_EqualTrailingIdx;
      int ringCap_EqualTrailingIdx;
      int ringLag_EqualTrailingIdx;
      double[] ring_EqualTrailingIdx_derived;
      int ringPos_NearTrailingIdx;
      int ringCap_NearTrailingIdx;
      int ringLag_NearTrailingIdx;
      double[] ring_NearTrailingIdx_derived;
      int cs_Equal_rangeType;
      int cs_Equal_avgPeriod;
      double cs_Equal_factor;
      int cs_Near_rangeType;
      int cs_Near_avgPeriod;
      double cs_Near_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLGAPSIDESIDEWHITE_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLGAPSIDESIDEWHITE_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLGAPSIDESIDEWHITE_Stream( CDLGAPSIDESIDEWHITE_Stream other ) {
         this.core = other.core;
         this.NearPeriodTotal = other.NearPeriodTotal;
         this.EqualPeriodTotal = other.EqualPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.ringPos_EqualTrailingIdx = other.ringPos_EqualTrailingIdx;
         this.ringCap_EqualTrailingIdx = other.ringCap_EqualTrailingIdx;
         this.ringLag_EqualTrailingIdx = other.ringLag_EqualTrailingIdx;
         this.ring_EqualTrailingIdx_derived = other.ring_EqualTrailingIdx_derived.clone();
         this.ringPos_NearTrailingIdx = other.ringPos_NearTrailingIdx;
         this.ringCap_NearTrailingIdx = other.ringCap_NearTrailingIdx;
         this.ringLag_NearTrailingIdx = other.ringLag_NearTrailingIdx;
         this.ring_NearTrailingIdx_derived = other.ring_NearTrailingIdx_derived.clone();
         this.cs_Equal_rangeType = other.cs_Equal_rangeType;
         this.cs_Equal_avgPeriod = other.cs_Equal_avgPeriod;
         this.cs_Equal_factor = other.cs_Equal_factor;
         this.cs_Near_rangeType = other.cs_Near_rangeType;
         this.cs_Near_avgPeriod = other.cs_Near_avgPeriod;
         this.cs_Near_factor = other.cs_Near_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDLGAPSIDESIDEWHITE_Stream other ) {
         this.core = other.core;
         this.NearPeriodTotal = other.NearPeriodTotal;
         this.EqualPeriodTotal = other.EqualPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.ringPos_EqualTrailingIdx = other.ringPos_EqualTrailingIdx;
         this.ringCap_EqualTrailingIdx = other.ringCap_EqualTrailingIdx;
         this.ringLag_EqualTrailingIdx = other.ringLag_EqualTrailingIdx;
         if( this.ring_EqualTrailingIdx_derived != null && this.ring_EqualTrailingIdx_derived.length == other.ring_EqualTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_EqualTrailingIdx_derived, 0, this.ring_EqualTrailingIdx_derived, 0, other.ring_EqualTrailingIdx_derived.length );
         } else {
            this.ring_EqualTrailingIdx_derived = other.ring_EqualTrailingIdx_derived.clone();
         }
         this.ringPos_NearTrailingIdx = other.ringPos_NearTrailingIdx;
         this.ringCap_NearTrailingIdx = other.ringCap_NearTrailingIdx;
         this.ringLag_NearTrailingIdx = other.ringLag_NearTrailingIdx;
         if( this.ring_NearTrailingIdx_derived != null && this.ring_NearTrailingIdx_derived.length == other.ring_NearTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_NearTrailingIdx_derived, 0, this.ring_NearTrailingIdx_derived, 0, other.ring_NearTrailingIdx_derived.length );
         } else {
            this.ring_NearTrailingIdx_derived = other.ring_NearTrailingIdx_derived.clone();
         }
         this.cs_Equal_rangeType = other.cs_Equal_rangeType;
         this.cs_Equal_avgPeriod = other.cs_Equal_avgPeriod;
         this.cs_Equal_factor = other.cs_Equal_factor;
         this.cs_Near_rangeType = other.cs_Near_rangeType;
         this.cs_Near_avgPeriod = other.cs_Near_avgPeriod;
         this.cs_Near_factor = other.cs_Near_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDLGAPSIDESIDEWHITE_Stream> PEEK_SCRATCH = new ThreadLocal<>();

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
            throw new TaLibArgumentException("CDLGAPSIDESIDEWHITE update: BadParam", RetCode.BadParam);
         core.CDLGAPSIDESIDEWHITE_StreamStep(this, inOpen, inHigh, inLow, inClose);
         return this.cur_outInteger;
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
            throw new TaLibArgumentException("CDLGAPSIDESIDEWHITE peek: BadParam", RetCode.BadParam);
         CDLGAPSIDESIDEWHITE_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLGAPSIDESIDEWHITE_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLGAPSIDESIDEWHITE_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLGAPSIDESIDEWHITE_Stream copy() {
         return new CDLGAPSIDESIDEWHITE_Stream(this);
      }
   }
   void CDLGAPSIDESIDEWHITE_StreamStep( CDLGAPSIDESIDEWHITE_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int Equal_rangeType = sp.cs_Equal_rangeType;
      int Equal_avgPeriod = sp.cs_Equal_avgPeriod;
      double Equal_factor = sp.cs_Equal_factor;
      int Near_rangeType = sp.cs_Near_rangeType;
      int Near_avgPeriod = sp.cs_Near_avgPeriod;
      double Near_factor = sp.cs_Near_factor;
      sp.ring_EqualTrailingIdx_derived[sp.ringPos_EqualTrailingIdx] = ((Equal_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType == 1) ? (inHigh - inLow) : ((Equal_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType == 1) ? (inHigh - inLow) : ((Near_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( ((Math.min(sp.lag1_inOpen, sp.lag1_inClose) > Math.max(sp.lag2_inOpen, sp.lag2_inClose)) && (Math.min(inOpen, inClose) > Math.max(sp.lag2_inOpen, sp.lag2_inClose)) || (Math.max(sp.lag1_inOpen, sp.lag1_inClose) < Math.min(sp.lag2_inOpen, sp.lag2_inClose)) && (Math.max(inOpen, inClose) < Math.min(sp.lag2_inOpen, sp.lag2_inClose))) && /* upside or downside gap between the 1st candle and both the next 2 candles */
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && /* 2nd: white */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 1 &&                 /* 3rd: white */
          Math.abs(inClose - inOpen) >= Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* same size 2 and 3 */
          Math.abs(inClose - inOpen) <= Math.abs(sp.lag1_inClose - sp.lag1_inOpen) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          inOpen >= sp.lag1_inOpen - ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* same open 2 and 3 */
          inOpen <= sp.lag1_inOpen + ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
      {
         sp.cur_outInteger = (Math.min(sp.lag1_inOpen, sp.lag1_inClose) > Math.max(sp.lag2_inOpen, sp.lag2_inClose)) ? 100 : 0 - 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_NearTrailingIdx_derived[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 1) % sp.ringCap_NearTrailingIdx];
      sp.EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_EqualTrailingIdx_derived[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx];
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag1_inHigh = inHigh;
      sp.lag1_inLow = inLow;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
      sp.ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx + 1;
      if( sp.ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx ) {
         sp.ringPos_EqualTrailingIdx = 0;
      }
      sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
      if( sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx ) {
         sp.ringPos_NearTrailingIdx = 0;
      }
   }
   private RetCode CDLGAPSIDESIDEWHITE_OpenPass( CDLGAPSIDESIDEWHITE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double NearPeriodTotal = 0;
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int NearTrailingIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLGAPSIDESIDEWHITE_Lookback();
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
      NearPeriodTotal = 0;
      EqualPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - upside or downside gap (between the bodies)
       * - first candle after the window: white candlestick
       * - second candle after the window: white candlestick with similar size (near the same) and about the same
       *   open (equal) of the previous candle
       * - the second candle does not close the window
       * The meaning of "near" and "equal" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) or negative (-1 to -100): the user should consider that upside
       * or downside gap side-by-side white lines is significant when it appears in a trend, while this function
       * does not consider the trend
       */
      outIdx = 0;
      do {
         if( ((Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && (Math.min(inOpen[i], inClose[i]) > Math.max(inOpen[i - 2], inClose[i - 2])) || (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && (Math.max(inOpen[i], inClose[i]) < Math.min(inOpen[i - 2], inClose[i - 2]))) && /* upside or downside gap between the 1st candle and both the next 2 candles */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && /* 2nd: white */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&         /* 3rd: white */
             Math.abs(inClose[i] - inOpen[i]) >= Math.abs(inClose[i - 1] - inOpen[i - 1]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* same size 2 and 3 */
             Math.abs(inClose[i] - inOpen[i]) <= Math.abs(inClose[i - 1] - inOpen[i - 1]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i] >= inOpen[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* same open 2 and 3 */
             inOpen[i] <= inOpen[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++ * outStride] = (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) ? 100 : 0 - 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
         i += 1;
         NearTrailingIdx += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_EqualTrailingIdx = i - EqualTrailingIdx;
      int cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 2;
      if( capLag_EqualTrailingIdx < 0 || cap_EqualTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_EqualTrailingIdx = (cap_EqualTrailingIdx > 0)? cap_EqualTrailingIdx : 1;
      double[] capRing_EqualTrailingIdx_derived = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_derived[fillJ % cap_EqualTrailingIdx] = ((Equal_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Equal_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Equal_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int capLag_NearTrailingIdx = i - NearTrailingIdx;
      int cap_NearTrailingIdx = capLag_NearTrailingIdx + 2;
      if( capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_NearTrailingIdx = (cap_NearTrailingIdx > 0)? cap_NearTrailingIdx : 1;
      double[] capRing_NearTrailingIdx_derived = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.NearPeriodTotal = NearPeriodTotal;
      sp.EqualPeriodTotal = EqualPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
      sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
      sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
      sp.ring_EqualTrailingIdx_derived = capRing_EqualTrailingIdx_derived;
      sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
      sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
      sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
      sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
      sp.cs_Equal_rangeType = Equal_rangeType;
      sp.cs_Equal_avgPeriod = Equal_avgPeriod;
      sp.cs_Equal_factor = Equal_factor;
      sp.cs_Near_rangeType = Near_rangeType;
      sp.cs_Near_avgPeriod = Near_avgPeriod;
      sp.cs_Near_factor = Near_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CDLGAPSIDESIDEWHITE_OpenImpl( CDLGAPSIDESIDEWHITE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDLGAPSIDESIDEWHITE_OpenPass( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDLGAPSIDESIDEWHITE_OpenAndFillImpl( CDLGAPSIDESIDEWHITE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDLGAPSIDESIDEWHITE_OpenPass( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDLGAPSIDESIDEWHITE_OpenAndFillInternalImpl( CDLGAPSIDESIDEWHITE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDLGAPSIDESIDEWHITE_OpenPass(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDLGAPSIDESIDEWHITE_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLGAPSIDESIDEWHITE_Stream CDLGAPSIDESIDEWHITE_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLGAPSIDESIDEWHITE_Stream sp = new CDLGAPSIDESIDEWHITE_Stream(this);
      RetCode retCode = CDLGAPSIDESIDEWHITE_OpenAndFillInternalImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLGAPSIDESIDEWHITE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLGAPSIDESIDEWHITE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLGAPSIDESIDEWHITE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind CDLGAPSIDESIDEWHITE_Open (composition seam). */
   CDLGAPSIDESIDEWHITE_Stream CDLGAPSIDESIDEWHITE_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLGAPSIDESIDEWHITE_Stream sp = new CDLGAPSIDESIDEWHITE_Stream(this);
      RetCode retCode = CDLGAPSIDESIDEWHITE_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLGAPSIDESIDEWHITE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLGAPSIDESIDEWHITE open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLGAPSIDESIDEWHITE open: " + retCode, retCode);
   }
   /**
    * Open a live CDLGAPSIDESIDEWHITE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLGAPSIDESIDEWHITE} at that bar.
    * <p>The history must hold at least {@code CDLGAPSIDESIDEWHITE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLGAPSIDESIDEWHITE_Stream CDLGAPSIDESIDEWHITE_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLGAPSIDESIDEWHITE_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLGAPSIDESIDEWHITE_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLGAPSIDESIDEWHITE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLGAPSIDESIDEWHITE_Stream#fillRange()}.
    */
   public CDLGAPSIDESIDEWHITE_Stream CDLGAPSIDESIDEWHITE_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLGAPSIDESIDEWHITE_Stream sp = new CDLGAPSIDESIDEWHITE_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLGAPSIDESIDEWHITE_OpenAndFillImpl(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLGAPSIDESIDEWHITE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLGAPSIDESIDEWHITE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLGAPSIDESIDEWHITE openAndFill: " + retCode, retCode);
   }
