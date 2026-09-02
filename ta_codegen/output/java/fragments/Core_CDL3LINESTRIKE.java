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
 *  121104 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDL3LINESTRIKE} consumes before
    * it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDL3LINESTRIKE_Lookback( )
   {
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      return Near_avgPeriod + 3 ;

   }
   RetCode CDL3LINESTRIKE_Impl( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                int outInteger[] )
   {
      double[] NearPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
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
      lookbackTotal = CDL3LINESTRIKE_Lookback();
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
      NearPeriodTotal[3] = 0;
      NearPeriodTotal[2] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - three white soldiers (three black crows): three white (black) candlesticks with consecutively higher (lower) closes,
       * each opening within or near the previous real body
       * - fourth candle: black (white) candle that opens above (below) prior candle's close and closes below (above)
       * the first candle's open
       * The meaning of "near" is specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that 3-line strike is significant when it appears in a trend in the same direction of
       * the first three candles, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) && /* three with same color */
             ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && /* 4th opposite color */
             inOpen[i - 2] >= Math.min(inOpen[i - 3], inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
             inOpen[i - 2] <= Math.max(inOpen[i - 3], inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i - 1] >= Math.min(inOpen[i - 2], inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
             inOpen[i - 1] <= Math.max(inOpen[i - 2], inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && inClose[i - 1] > inClose[i - 2] && inClose[i - 2] > inClose[i - 3] && inOpen[i] > inClose[i - 1] && inClose[i] < inOpen[i - 3] || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && inClose[i - 1] < inClose[i - 2] && inClose[i - 2] < inClose[i - 3] && inOpen[i] < inClose[i - 1] && inClose[i] > inOpen[i - 3]) ) /* if three white consecutive higher closes 4th opens above prior close 4th closes below 1st open if three black consecutive lower closes 4th opens below prior close 4th closes above 1st open */
         {
            outInteger[outIdx++] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 3; totIdx >= 2; totIdx -= 1 ) {
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - (((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inClose[NearTrailingIdx - totIdx]) : (inOpen[NearTrailingIdx - totIdx]))) + ((((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inOpen[NearTrailingIdx - totIdx]) : (inClose[NearTrailingIdx - totIdx])) - inLow[NearTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDL3LINESTRIKE_Impl( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                int outInteger[] )
   {
      double[] NearPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDL3LINESTRIKE_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal[3] = 0;
      NearPeriodTotal[2] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - ((((double)inClose[i - 3]) >= ((double)inOpen[i - 3])) ? ((double)inClose[i - 3]) : ((double)inOpen[i - 3]))) + (((((double)inClose[i - 3]) >= ((double)inOpen[i - 3])) ? ((double)inOpen[i - 3]) : ((double)inClose[i - 3])) - (double)inLow[i - 3])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (double)inOpen[i - 2] >= Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - ((((double)inClose[i - 3]) >= ((double)inOpen[i - 3])) ? ((double)inClose[i - 3]) : ((double)inOpen[i - 3]))) + (((((double)inClose[i - 3]) >= ((double)inOpen[i - 3])) ? ((double)inOpen[i - 3]) : ((double)inClose[i - 3])) - (double)inLow[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 2] <= Math.max((double)inOpen[i - 3], (double)inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - ((((double)inClose[i - 3]) >= ((double)inOpen[i - 3])) ? ((double)inClose[i - 3]) : ((double)inOpen[i - 3]))) + (((((double)inClose[i - 3]) >= ((double)inOpen[i - 3])) ? ((double)inOpen[i - 3]) : ((double)inClose[i - 3])) - (double)inLow[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] >= Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] <= Math.max((double)inOpen[i - 2], (double)inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (double)inClose[i - 1] > (double)inClose[i - 2] && (double)inClose[i - 2] > (double)inClose[i - 3] && (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] < (double)inOpen[i - 3] || (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (double)inClose[i - 1] < (double)inClose[i - 2] && (double)inClose[i - 2] < (double)inClose[i - 3] && (double)inOpen[i] < (double)inClose[i - 1] && (double)inClose[i] > (double)inOpen[i - 3]) ) {
            outInteger[outIdx++] = (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 3; totIdx >= 2; totIdx -= 1 ) {
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[i - totIdx] - ((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inClose[i - totIdx]) : ((double)inOpen[i - totIdx]))) + (((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inOpen[i - totIdx]) : ((double)inClose[i - totIdx])) - (double)inLow[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - totIdx] - ((((double)inClose[NearTrailingIdx - totIdx]) >= ((double)inOpen[NearTrailingIdx - totIdx])) ? ((double)inClose[NearTrailingIdx - totIdx]) : ((double)inOpen[NearTrailingIdx - totIdx]))) + (((((double)inClose[NearTrailingIdx - totIdx]) >= ((double)inOpen[NearTrailingIdx - totIdx])) ? ((double)inOpen[NearTrailingIdx - totIdx]) : ((double)inClose[NearTrailingIdx - totIdx])) - (double)inLow[NearTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A four-candle pattern: three same-color candles with consecutively higher
    * (or lower) closes, each opening within or near the prior real body, then a
    * fourth opposite-color candle that opens beyond the third close and closes
    * past the first candle's open. TA-Lib emits a signed continuation-style
    * signal keyed to the color of the first three candles, traditionally read
    * as significant only inside a trend matching those three candles.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the surrounding trend the pattern classically assumes for significance.</li>
    * <li>TA-Lib's sign follows the classic continuation reading. Thomas Bulkowski's statistical study of the pattern (*Encyclopedia of Candlestick Charts*) found the opposite in practice — it acted as a reversal far more often than a continuation — so traders who follow his research read this pattern's signal in the opposite direction from what its sign here suggests.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDL3LINESTRIKE_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 for a white (rising) three-line strike, -100 for a
    *        black (falling) three-line strike, 0 otherwise. Sign is the color of the
    *        first three candles: candlecolor(i-1)*100. Must hold at least
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
    * @see Core#CDL3BLACKCROWS
    */
   public OutRange CDL3LINESTRIKE( int startIdx,
                                   int endIdx,
                                   double inOpen[],
                                   double inHigh[],
                                   double inLow[],
                                   double inClose[],
                                   int outInteger[] )
   {
      requireIndexRange("CDL3LINESTRIKE", startIdx, endIdx);
      int guardStart = clampedStart("CDL3LINESTRIKE", startIdx, CDL3LINESTRIKE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDL3LINESTRIKE", "inOpen", inOpen, guardInLen);
      requireLength("CDL3LINESTRIKE", "inHigh", inHigh, guardInLen);
      requireLength("CDL3LINESTRIKE", "inLow", inLow, guardInLen);
      requireLength("CDL3LINESTRIKE", "inClose", inClose, guardInLen);
      requireLength("CDL3LINESTRIKE", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDL3LINESTRIKE_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDL3LINESTRIKE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A four-candle pattern: three same-color candles with consecutively higher
    * (or lower) closes, each opening within or near the prior real body, then a
    * fourth opposite-color candle that opens beyond the third close and closes
    * past the first candle's open. TA-Lib emits a signed continuation-style
    * signal keyed to the color of the first three candles, traditionally read
    * as significant only inside a trend matching those three candles.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the surrounding trend the pattern classically assumes for significance.</li>
    * <li>TA-Lib's sign follows the classic continuation reading. Thomas Bulkowski's statistical study of the pattern (*Encyclopedia of Candlestick Charts*) found the opposite in practice — it acted as a reversal far more often than a continuation — so traders who follow his research read this pattern's signal in the opposite direction from what its sign here suggests.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDL3LINESTRIKE_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 for a white (rising) three-line strike, -100 for a
    *        black (falling) three-line strike, 0 otherwise. Sign is the color of the
    *        first three candles: candlecolor(i-1)*100. Must hold at least
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
    * @see Core#CDL3BLACKCROWS
    */
   public OutRange CDL3LINESTRIKE( int startIdx,
                                   int endIdx,
                                   float inOpen[],
                                   float inHigh[],
                                   float inLow[],
                                   float inClose[],
                                   int outInteger[] )
   {
      requireIndexRange("CDL3LINESTRIKE", startIdx, endIdx);
      int guardStart = clampedStart("CDL3LINESTRIKE", startIdx, CDL3LINESTRIKE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDL3LINESTRIKE", "inOpen", inOpen, guardInLen);
      requireLength("CDL3LINESTRIKE", "inHigh", inHigh, guardInLen);
      requireLength("CDL3LINESTRIKE", "inLow", inLow, guardInLen);
      requireLength("CDL3LINESTRIKE", "inClose", inClose, guardInLen);
      requireLength("CDL3LINESTRIKE", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDL3LINESTRIKE_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDL3LINESTRIKE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDL3LINESTRIKE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDL3LINESTRIKE} over the same series.
    * Open with {@link Core#cdl3linestrikeOpen}; there is no close — the handle is
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
   public static final class Cdl3linestrikeStream {
      Core core;
      double[] NearPeriodTotal;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag3_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag3_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag3_inLow;
      double lag1_inClose;
      double lag2_inClose;
      double lag3_inClose;
      int ringPos_NearTrailingIdx;
      int ringCap_NearTrailingIdx;
      int ringLag_NearTrailingIdx;
      double[] ring_NearTrailingIdx_derived;
      int cs_Near_rangeType;
      int cs_Near_avgPeriod;
      double cs_Near_factor;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      Cdl3linestrikeStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDL3LINESTRIKE} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      Cdl3linestrikeStream( Cdl3linestrikeStream other ) {
         this.core = other.core;
         this.NearPeriodTotal = other.NearPeriodTotal.clone();
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag3_inOpen = other.lag3_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag3_inHigh = other.lag3_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag3_inLow = other.lag3_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.lag3_inClose = other.lag3_inClose;
         this.ringPos_NearTrailingIdx = other.ringPos_NearTrailingIdx;
         this.ringCap_NearTrailingIdx = other.ringCap_NearTrailingIdx;
         this.ringLag_NearTrailingIdx = other.ringLag_NearTrailingIdx;
         this.ring_NearTrailingIdx_derived = other.ring_NearTrailingIdx_derived.clone();
         this.cs_Near_rangeType = other.cs_Near_rangeType;
         this.cs_Near_avgPeriod = other.cs_Near_avgPeriod;
         this.cs_Near_factor = other.cs_Near_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("CDL3LINESTRIKE update: BadParam", RetCode.BadParam);
         }
         core.cdl3linestrikeStepImpl(this, inOpen, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outInteger;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inOpen.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] ) {
         requireArgument("CDL3LINESTRIKE updateAndFill", "inOpen", inOpen);
         requireArgument("CDL3LINESTRIKE updateAndFill", "inHigh", inHigh);
         requireArgument("CDL3LINESTRIKE updateAndFill", "inLow", inLow);
         requireArgument("CDL3LINESTRIKE updateAndFill", "inClose", inClose);
         requireArgument("CDL3LINESTRIKE updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDL3LINESTRIKE updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CDL3LINESTRIKE updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cdl3linestrikeStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
            outInteger[i] = this.cur_outInteger;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDL3LINESTRIKE peek: BadParam", RetCode.BadParam);
         Cdl3linestrikeStream sp = this;
         int cur_outInteger = sp.cur_outInteger;
         int Near_rangeType = sp.cs_Near_rangeType;
         int Near_avgPeriod = sp.cs_Near_avgPeriod;
         double Near_factor = sp.cs_Near_factor;
         if( ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) == ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) && /* three with same color */
             ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) &&
             ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) && /* 4th opposite color */
             sp.lag2_inOpen >= Math.min(sp.lag3_inOpen, sp.lag3_inClose) - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType == 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType == 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
             sp.lag2_inOpen <= Math.max(sp.lag3_inOpen, sp.lag3_inClose) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType == 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType == 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             sp.lag1_inOpen >= Math.min(sp.lag2_inOpen, sp.lag2_inClose) - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
             sp.lag1_inOpen <= Math.max(sp.lag2_inOpen, sp.lag2_inClose) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && sp.lag1_inClose > sp.lag2_inClose && sp.lag2_inClose > sp.lag3_inClose && inOpen > sp.lag1_inClose && inClose < sp.lag3_inOpen || ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && sp.lag1_inClose < sp.lag2_inClose && sp.lag2_inClose < sp.lag3_inClose && inOpen < sp.lag1_inClose && inClose > sp.lag3_inOpen) ) /* if three white consecutive higher closes 4th opens above prior close 4th closes below 1st open if three black consecutive lower closes 4th opens below prior close 4th closes above 1st open */
         {
            cur_outInteger = ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) * 100;
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
      public Cdl3linestrikeStream clone() {
         return new Cdl3linestrikeStream(this);
      }
   }
   void cdl3linestrikeStepImpl( Cdl3linestrikeStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int totIdx = 0;
      int Near_rangeType = sp.cs_Near_rangeType;
      int Near_avgPeriod = sp.cs_Near_avgPeriod;
      double Near_factor = sp.cs_Near_factor;
      sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType == 1) ? (inHigh - inLow) : ((Near_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) == ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) && /* three with same color */
          ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) &&
          ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) && /* 4th opposite color */
          sp.lag2_inOpen >= Math.min(sp.lag3_inOpen, sp.lag3_inClose) - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType == 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType == 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
          sp.lag2_inOpen <= Math.max(sp.lag3_inOpen, sp.lag3_inClose) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType == 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType == 2) ? ((sp.lag3_inHigh - (((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inClose) : (sp.lag3_inOpen))) + ((((sp.lag3_inClose) >= (sp.lag3_inOpen)) ? (sp.lag3_inOpen) : (sp.lag3_inClose)) - sp.lag3_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          sp.lag1_inOpen >= Math.min(sp.lag2_inOpen, sp.lag2_inClose) - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
          sp.lag1_inOpen <= Math.max(sp.lag2_inOpen, sp.lag2_inClose) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && sp.lag1_inClose > sp.lag2_inClose && sp.lag2_inClose > sp.lag3_inClose && inOpen > sp.lag1_inClose && inClose < sp.lag3_inOpen || ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && sp.lag1_inClose < sp.lag2_inClose && sp.lag2_inClose < sp.lag3_inClose && inOpen < sp.lag1_inClose && inClose > sp.lag3_inOpen) ) /* if three white consecutive higher closes 4th opens above prior close 4th closes below 1st open if three black consecutive lower closes 4th opens below prior close 4th closes above 1st open */
      {
         sp.cur_outInteger = ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for( totIdx = 3; totIdx >= 2; totIdx -= 1 ) {
         sp.NearPeriodTotal[totIdx] = sp.NearPeriodTotal[totIdx] + (sp.ring_NearTrailingIdx_derived[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx >= sp.ringCap_NearTrailingIdx) ? sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx - sp.ringCap_NearTrailingIdx : sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - totIdx] - sp.ring_NearTrailingIdx_derived[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - totIdx) % sp.ringCap_NearTrailingIdx]);
      }
      sp.lag3_inOpen = sp.lag2_inOpen;
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag3_inHigh = sp.lag2_inHigh;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag3_inLow = sp.lag2_inLow;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
      sp.lag3_inClose = sp.lag2_inClose;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
      sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
      if( sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx ) {
         sp.ringPos_NearTrailingIdx = 0;
      }
   }
   private RetCode cdl3linestrikeOpenImpl( Cdl3linestrikeStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double[] NearPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int NearTrailingIdx = 0;
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
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDL3LINESTRIKE_Lookback();
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
      NearPeriodTotal[3] = 0;
      NearPeriodTotal[2] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - three white soldiers (three black crows): three white (black) candlesticks with consecutively higher (lower) closes,
       * each opening within or near the previous real body
       * - fourth candle: black (white) candle that opens above (below) prior candle's close and closes below (above)
       * the first candle's open
       * The meaning of "near" is specified with TA_SetCandleSettings;
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that 3-line strike is significant when it appears in a trend in the same direction of
       * the first three candles, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) && /* three with same color */
             ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && /* 4th opposite color */
             inOpen[i - 2] >= Math.min(inOpen[i - 3], inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
             inOpen[i - 2] <= Math.max(inOpen[i - 3], inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - (((inClose[i - 3]) >= (inOpen[i - 3])) ? (inClose[i - 3]) : (inOpen[i - 3]))) + ((((inClose[i - 3]) >= (inOpen[i - 3])) ? (inOpen[i - 3]) : (inClose[i - 3])) - inLow[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i - 1] >= Math.min(inOpen[i - 2], inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
             inOpen[i - 1] <= Math.max(inOpen[i - 2], inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && inClose[i - 1] > inClose[i - 2] && inClose[i - 2] > inClose[i - 3] && inOpen[i] > inClose[i - 1] && inClose[i] < inOpen[i - 3] || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && inClose[i - 1] < inClose[i - 2] && inClose[i - 2] < inClose[i - 3] && inOpen[i] < inClose[i - 1] && inClose[i] > inOpen[i - 3]) ) /* if three white consecutive higher closes 4th opens above prior close 4th closes below 1st open if three black consecutive lower closes 4th opens below prior close 4th closes above 1st open */
         {
            outInteger[outIdx++ * outStride] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 3; totIdx >= 2; totIdx -= 1 ) {
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - (((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inClose[NearTrailingIdx - totIdx]) : (inOpen[NearTrailingIdx - totIdx]))) + ((((inClose[NearTrailingIdx - totIdx]) >= (inOpen[NearTrailingIdx - totIdx])) ? (inOpen[NearTrailingIdx - totIdx]) : (inClose[NearTrailingIdx - totIdx])) - inLow[NearTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_NearTrailingIdx = i - NearTrailingIdx;
      int cap_NearTrailingIdx = capLag_NearTrailingIdx + 4;
      if( capLag_NearTrailingIdx < 0 || cap_NearTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_NearTrailingIdx = (cap_NearTrailingIdx > 0)? cap_NearTrailingIdx : 1;
      double[] capRing_NearTrailingIdx_derived = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.NearPeriodTotal = NearPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag3_inOpen = inOpen[historyLen - 3];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag3_inHigh = inHigh[historyLen - 3];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag3_inLow = inLow[historyLen - 3];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.lag3_inClose = inClose[historyLen - 3];
      sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
      sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
      sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
      sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
      sp.cs_Near_rangeType = Near_rangeType;
      sp.cs_Near_avgPeriod = Near_avgPeriod;
      sp.cs_Near_factor = Near_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cdl3linestrikeOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   Cdl3linestrikeStream cdl3linestrikeOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      Cdl3linestrikeStream sp = new Cdl3linestrikeStream(this);
      RetCode retCode = cdl3linestrikeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDL3LINESTRIKE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDL3LINESTRIKE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDL3LINESTRIKE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cdl3linestrikeOpen (composition seam). */
   Cdl3linestrikeStream cdl3linestrikeOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      Cdl3linestrikeStream sp = new Cdl3linestrikeStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdl3linestrikeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDL3LINESTRIKE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDL3LINESTRIKE open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDL3LINESTRIKE open: " + retCode, retCode);
   }
   /**
    * Open a live CDL3LINESTRIKE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDL3LINESTRIKE} at that bar.
    * <p>The history must hold at least {@code CDL3LINESTRIKE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public Cdl3linestrikeStream cdl3linestrikeOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDL3LINESTRIKE open", "inOpen", inOpen);
      requireHistory("CDL3LINESTRIKE open", inOpen.length);
      requireArgument("CDL3LINESTRIKE open", "inHigh", inHigh);
      requireArgument("CDL3LINESTRIKE open", "inLow", inLow);
      requireArgument("CDL3LINESTRIKE open", "inClose", inClose);
      requireHistoryLength("CDL3LINESTRIKE open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDL3LINESTRIKE open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDL3LINESTRIKE open", "inClose", inClose.length, inOpen.length);
      return cdl3linestrikeOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdl3linestrikeOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CDL3LINESTRIKE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link Cdl3linestrikeStream#outRange()}.
    */
   public Cdl3linestrikeStream cdl3linestrikeOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDL3LINESTRIKE openAndFill", "inOpen", inOpen);
      requireHistory("CDL3LINESTRIKE openAndFill", inOpen.length);
      requireArgument("CDL3LINESTRIKE openAndFill", "inHigh", inHigh);
      requireArgument("CDL3LINESTRIKE openAndFill", "inLow", inLow);
      requireArgument("CDL3LINESTRIKE openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDL3LINESTRIKE openAndFill", inOpen.length, CDL3LINESTRIKE_Lookback());
      requireHistoryLength("CDL3LINESTRIKE openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDL3LINESTRIKE openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDL3LINESTRIKE openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDL3LINESTRIKE openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDL3LINESTRIKE openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdl3linestrikeOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
