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
   RetCode CDL3LINESTRIKE_Internal( int startIdx,
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
         NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
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
             inOpen[i - 2] >= Math.min(inOpen[i - 3], inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
             inOpen[i - 2] <= Math.max(inOpen[i - 3], inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i - 1] >= Math.min(inOpen[i - 2], inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
             inOpen[i - 1] <= Math.max(inOpen[i - 2], inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
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
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) - Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDL3LINESTRIKE_Internal( int startIdx,
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
         NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (double)inOpen[i - 2] >= Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 2] <= Math.max((double)inOpen[i - 3], (double)inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 3] - (double)inLow[i - 3]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 3] - (double)inLow[i - 3]) - Math.abs((double)inClose[i - 3] - (double)inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] >= Math.min((double)inOpen[i - 2], (double)inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i - 1] <= Math.max((double)inOpen[i - 2], (double)inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (double)inClose[i - 1] > (double)inClose[i - 2] && (double)inClose[i - 2] > (double)inClose[i - 3] && (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] < (double)inOpen[i - 3] || (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (double)inClose[i - 1] < (double)inClose[i - 2] && (double)inClose[i - 2] < (double)inClose[i - 3] && (double)inOpen[i] < (double)inClose[i - 1] && (double)inClose[i] > (double)inOpen[i - 3]) ) {
            outInteger[outIdx++] = (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 3; totIdx >= 2; totIdx -= 1 ) {
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) - Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - totIdx] - (double)inLow[NearTrailingIdx - totIdx]) - Math.abs((double)inClose[NearTrailingIdx - totIdx] - (double)inOpen[NearTrailingIdx - totIdx])) : 0.0))));
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
    * signal keyed to the color of the first three candles. +100 = three-white
    * (bullish) strike, -100 = three-black (bearish) strike; traditionally read
    * as significant only inside a trend matching the first three candles.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the surrounding trend the pattern classically assumes for significance.</li>
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDL3LINESTRIKE_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
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
    * signal keyed to the color of the first three candles. +100 = three-white
    * (bullish) strike, -100 = three-black (bearish) strike; traditionally read
    * as significant only inside a trend matching the first three candles.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the surrounding trend the pattern classically assumes for significance.</li>
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDL3LINESTRIKE_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDL3LINESTRIKE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDL3LINESTRIKE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDL3LINESTRIKE} over the same series.
    * Open with {@link Core#CDL3LINESTRIKE_Open}; there is no close — the handle is
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
   public static final class CDL3LINESTRIKE_Stream {
      Core core;
      double[] NearPeriodTotal;
      int totIdx;
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
      double[] ring_NearTrailingIdx_inOpen;
      double[] ring_NearTrailingIdx_inHigh;
      double[] ring_NearTrailingIdx_inLow;
      double[] ring_NearTrailingIdx_inClose;
      int winPos_totIdx;
      int winCap_totIdx;
      double[] win_totIdx_inOpen;
      double[] win_totIdx_inHigh;
      double[] win_totIdx_inLow;
      double[] win_totIdx_inClose;
      int cs_Near_rangeType;
      int cs_Near_avgPeriod;
      double cs_Near_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDL3LINESTRIKE_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDL3LINESTRIKE_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDL3LINESTRIKE_Stream( CDL3LINESTRIKE_Stream other ) {
         this.core = other.core;
         this.NearPeriodTotal = other.NearPeriodTotal.clone();
         this.totIdx = other.totIdx;
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
         this.ring_NearTrailingIdx_inOpen = other.ring_NearTrailingIdx_inOpen.clone();
         this.ring_NearTrailingIdx_inHigh = other.ring_NearTrailingIdx_inHigh.clone();
         this.ring_NearTrailingIdx_inLow = other.ring_NearTrailingIdx_inLow.clone();
         this.ring_NearTrailingIdx_inClose = other.ring_NearTrailingIdx_inClose.clone();
         this.winPos_totIdx = other.winPos_totIdx;
         this.winCap_totIdx = other.winCap_totIdx;
         this.win_totIdx_inOpen = other.win_totIdx_inOpen.clone();
         this.win_totIdx_inHigh = other.win_totIdx_inHigh.clone();
         this.win_totIdx_inLow = other.win_totIdx_inLow.clone();
         this.win_totIdx_inClose = other.win_totIdx_inClose.clone();
         this.cs_Near_rangeType = other.cs_Near_rangeType;
         this.cs_Near_avgPeriod = other.cs_Near_avgPeriod;
         this.cs_Near_factor = other.cs_Near_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDL3LINESTRIKE_Stream other ) {
         this.core = other.core;
         if( this.NearPeriodTotal != null && this.NearPeriodTotal.length == other.NearPeriodTotal.length ) {
            System.arraycopy( other.NearPeriodTotal, 0, this.NearPeriodTotal, 0, other.NearPeriodTotal.length );
         } else {
            this.NearPeriodTotal = other.NearPeriodTotal.clone();
         }
         this.totIdx = other.totIdx;
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
         if( this.ring_NearTrailingIdx_inOpen != null && this.ring_NearTrailingIdx_inOpen.length == other.ring_NearTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_NearTrailingIdx_inOpen, 0, this.ring_NearTrailingIdx_inOpen, 0, other.ring_NearTrailingIdx_inOpen.length );
         } else {
            this.ring_NearTrailingIdx_inOpen = other.ring_NearTrailingIdx_inOpen.clone();
         }
         if( this.ring_NearTrailingIdx_inHigh != null && this.ring_NearTrailingIdx_inHigh.length == other.ring_NearTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_NearTrailingIdx_inHigh, 0, this.ring_NearTrailingIdx_inHigh, 0, other.ring_NearTrailingIdx_inHigh.length );
         } else {
            this.ring_NearTrailingIdx_inHigh = other.ring_NearTrailingIdx_inHigh.clone();
         }
         if( this.ring_NearTrailingIdx_inLow != null && this.ring_NearTrailingIdx_inLow.length == other.ring_NearTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_NearTrailingIdx_inLow, 0, this.ring_NearTrailingIdx_inLow, 0, other.ring_NearTrailingIdx_inLow.length );
         } else {
            this.ring_NearTrailingIdx_inLow = other.ring_NearTrailingIdx_inLow.clone();
         }
         if( this.ring_NearTrailingIdx_inClose != null && this.ring_NearTrailingIdx_inClose.length == other.ring_NearTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_NearTrailingIdx_inClose, 0, this.ring_NearTrailingIdx_inClose, 0, other.ring_NearTrailingIdx_inClose.length );
         } else {
            this.ring_NearTrailingIdx_inClose = other.ring_NearTrailingIdx_inClose.clone();
         }
         this.winPos_totIdx = other.winPos_totIdx;
         this.winCap_totIdx = other.winCap_totIdx;
         if( this.win_totIdx_inOpen != null && this.win_totIdx_inOpen.length == other.win_totIdx_inOpen.length ) {
            System.arraycopy( other.win_totIdx_inOpen, 0, this.win_totIdx_inOpen, 0, other.win_totIdx_inOpen.length );
         } else {
            this.win_totIdx_inOpen = other.win_totIdx_inOpen.clone();
         }
         if( this.win_totIdx_inHigh != null && this.win_totIdx_inHigh.length == other.win_totIdx_inHigh.length ) {
            System.arraycopy( other.win_totIdx_inHigh, 0, this.win_totIdx_inHigh, 0, other.win_totIdx_inHigh.length );
         } else {
            this.win_totIdx_inHigh = other.win_totIdx_inHigh.clone();
         }
         if( this.win_totIdx_inLow != null && this.win_totIdx_inLow.length == other.win_totIdx_inLow.length ) {
            System.arraycopy( other.win_totIdx_inLow, 0, this.win_totIdx_inLow, 0, other.win_totIdx_inLow.length );
         } else {
            this.win_totIdx_inLow = other.win_totIdx_inLow.clone();
         }
         if( this.win_totIdx_inClose != null && this.win_totIdx_inClose.length == other.win_totIdx_inClose.length ) {
            System.arraycopy( other.win_totIdx_inClose, 0, this.win_totIdx_inClose, 0, other.win_totIdx_inClose.length );
         } else {
            this.win_totIdx_inClose = other.win_totIdx_inClose.clone();
         }
         this.cs_Near_rangeType = other.cs_Near_rangeType;
         this.cs_Near_avgPeriod = other.cs_Near_avgPeriod;
         this.cs_Near_factor = other.cs_Near_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDL3LINESTRIKE_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.CDL3LINESTRIKE_StreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CDL3LINESTRIKE_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDL3LINESTRIKE_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDL3LINESTRIKE_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDL3LINESTRIKE_Stream copy() {
         return new CDL3LINESTRIKE_Stream(this);
      }
   }
   void CDL3LINESTRIKE_StreamStep( CDL3LINESTRIKE_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int Near_rangeType = sp.cs_Near_rangeType;
      int Near_avgPeriod = sp.cs_Near_avgPeriod;
      double Near_factor = sp.cs_Near_factor;
      sp.ring_NearTrailingIdx_inOpen[sp.ringPos_NearTrailingIdx] = inOpen;
      sp.ring_NearTrailingIdx_inHigh[sp.ringPos_NearTrailingIdx] = inHigh;
      sp.ring_NearTrailingIdx_inLow[sp.ringPos_NearTrailingIdx] = inLow;
      sp.ring_NearTrailingIdx_inClose[sp.ringPos_NearTrailingIdx] = inClose;
      sp.win_totIdx_inOpen[sp.winPos_totIdx] = inOpen;
      sp.win_totIdx_inHigh[sp.winPos_totIdx] = inHigh;
      sp.win_totIdx_inLow[sp.winPos_totIdx] = inLow;
      sp.win_totIdx_inClose[sp.winPos_totIdx] = inClose;
      if( ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) == ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) && /* three with same color */
          ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) &&
          ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) && /* 4th opposite color */
          sp.lag2_inOpen >= Math.min(sp.lag3_inOpen, sp.lag3_inClose) - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType == 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType == 2) ? ((sp.lag3_inHigh - sp.lag3_inLow) - Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
          sp.lag2_inOpen <= Math.max(sp.lag3_inOpen, sp.lag3_inClose) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : ((Near_rangeType == 1) ? (sp.lag3_inHigh - sp.lag3_inLow) : ((Near_rangeType == 2) ? ((sp.lag3_inHigh - sp.lag3_inLow) - Math.abs(sp.lag3_inClose - sp.lag3_inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          sp.lag1_inOpen >= Math.min(sp.lag2_inOpen, sp.lag2_inClose) - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
          sp.lag1_inOpen <= Math.max(sp.lag2_inOpen, sp.lag2_inClose) + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
          (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && sp.lag1_inClose > sp.lag2_inClose && sp.lag2_inClose > sp.lag3_inClose && inOpen > sp.lag1_inClose && inClose < sp.lag3_inOpen || ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && sp.lag1_inClose < sp.lag2_inClose && sp.lag2_inClose < sp.lag3_inClose && inOpen < sp.lag1_inClose && inClose > sp.lag3_inOpen) ) /* if three white consecutive higher closes 4th opens above prior close 4th closes below 1st open if three black consecutive lower closes 4th opens below prior close 4th closes above 1st open */
      {
         sp.cur_outInteger = ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for( sp.totIdx = 3; sp.totIdx >= 2; sp.totIdx -= 1 ) {
         sp.NearPeriodTotal[sp.totIdx] = sp.NearPeriodTotal[sp.totIdx] + (((Near_rangeType == 0) ? (Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : ((Near_rangeType == 1) ? (sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) : ((Near_rangeType == 2) ? ((sp.win_totIdx_inHigh[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inLow[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx]) - Math.abs(sp.win_totIdx_inClose[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx] - sp.win_totIdx_inOpen[(sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx >= sp.winCap_totIdx) ? sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx - sp.winCap_totIdx : sp.winPos_totIdx + sp.winCap_totIdx - sp.totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(sp.ring_NearTrailingIdx_inClose[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inOpen[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx])) : ((Near_rangeType == 1) ? (sp.ring_NearTrailingIdx_inHigh[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inLow[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx]) : ((Near_rangeType == 2) ? ((sp.ring_NearTrailingIdx_inHigh[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inLow[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx]) - Math.abs(sp.ring_NearTrailingIdx_inClose[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inOpen[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - sp.totIdx) % sp.ringCap_NearTrailingIdx])) : 0.0))));
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
      sp.ring_NearTrailingIdx_inOpen[sp.ringPos_NearTrailingIdx] = inOpen;
      sp.ring_NearTrailingIdx_inHigh[sp.ringPos_NearTrailingIdx] = inHigh;
      sp.ring_NearTrailingIdx_inLow[sp.ringPos_NearTrailingIdx] = inLow;
      sp.ring_NearTrailingIdx_inClose[sp.ringPos_NearTrailingIdx] = inClose;
      sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
      if( sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx ) {
         sp.ringPos_NearTrailingIdx = 0;
      }
      sp.winPos_totIdx = sp.winPos_totIdx + 1;
      if( sp.winPos_totIdx >= sp.winCap_totIdx ) {
         sp.winPos_totIdx = 0;
      }
   }
   private RetCode CDL3LINESTRIKE_OpenCore( CDL3LINESTRIKE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double[] NearPeriodTotal = new double[4];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      NearPeriodTotal[3] = 0;
      NearPeriodTotal[2] = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal[3] = NearPeriodTotal[3] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)));
         NearPeriodTotal[2] = NearPeriodTotal[2] + ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
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
             inOpen[i - 2] >= Math.min(inOpen[i - 3], inClose[i - 3]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd opens within/near 1st rb */
             inOpen[i - 2] <= Math.max(inOpen[i - 3], inClose[i - 3]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[3] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 3] - inOpen[i - 3])) : ((Near_rangeType == 1) ? (inHigh[i - 3] - inLow[i - 3]) : ((Near_rangeType == 2) ? ((inHigh[i - 3] - inLow[i - 3]) - Math.abs(inClose[i - 3] - inOpen[i - 3])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
             inOpen[i - 1] >= Math.min(inOpen[i - 2], inClose[i - 2]) - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) && /* 3rd opens within/near 2nd rb */
             inOpen[i - 1] <= Math.max(inOpen[i - 2], inClose[i - 2]) + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal[2] / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) &&
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
            NearPeriodTotal[totIdx] = NearPeriodTotal[totIdx] + (((Near_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((Near_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[i - totIdx] - inLow[i - totIdx]) - Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - totIdx] - inLow[NearTrailingIdx - totIdx]) - Math.abs(inClose[NearTrailingIdx - totIdx] - inOpen[NearTrailingIdx - totIdx])) : 0.0))));
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
      int cap_totIdx = (int)(4);
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
      sp.NearPeriodTotal = NearPeriodTotal;
      sp.totIdx = totIdx;
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
      sp.ring_NearTrailingIdx_inOpen = capRing_NearTrailingIdx_inOpen;
      sp.ring_NearTrailingIdx_inHigh = capRing_NearTrailingIdx_inHigh;
      sp.ring_NearTrailingIdx_inLow = capRing_NearTrailingIdx_inLow;
      sp.ring_NearTrailingIdx_inClose = capRing_NearTrailingIdx_inClose;
      sp.winPos_totIdx = 0;
      sp.winCap_totIdx = cap_totIdx;
      sp.win_totIdx_inOpen = capWin_totIdx_inOpen;
      sp.win_totIdx_inHigh = capWin_totIdx_inHigh;
      sp.win_totIdx_inLow = capWin_totIdx_inLow;
      sp.win_totIdx_inClose = capWin_totIdx_inClose;
      sp.cs_Near_rangeType = Near_rangeType;
      sp.cs_Near_avgPeriod = Near_avgPeriod;
      sp.cs_Near_factor = Near_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CDL3LINESTRIKE_OpenBody( CDL3LINESTRIKE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDL3LINESTRIKE_OpenCore( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDL3LINESTRIKE_OpenAndFillBody( CDL3LINESTRIKE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDL3LINESTRIKE_OpenCore( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDL3LINESTRIKE_OpenAndFillInternalBody( CDL3LINESTRIKE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDL3LINESTRIKE_OpenCore(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDL3LINESTRIKE_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDL3LINESTRIKE_Stream CDL3LINESTRIKE_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDL3LINESTRIKE_Stream sp = new CDL3LINESTRIKE_Stream(this);
      RetCode retCode = CDL3LINESTRIKE_OpenAndFillInternalBody(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDL3LINESTRIKE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDL3LINESTRIKE openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDL3LINESTRIKE openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind CDL3LINESTRIKE_Open (composition seam). */
   CDL3LINESTRIKE_Stream CDL3LINESTRIKE_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDL3LINESTRIKE_Stream sp = new CDL3LINESTRIKE_Stream(this);
      RetCode retCode = CDL3LINESTRIKE_OpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDL3LINESTRIKE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDL3LINESTRIKE open: internal error");
      }
      throw new IllegalArgumentException("CDL3LINESTRIKE open: " + retCode);
   }
   /**
    * Open a live CDL3LINESTRIKE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDL3LINESTRIKE} at that bar.
    * <p>The history must hold at least {@code CDL3LINESTRIKE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDL3LINESTRIKE_Stream CDL3LINESTRIKE_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDL3LINESTRIKE_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDL3LINESTRIKE_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDL3LINESTRIKE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDL3LINESTRIKE_Stream#fillRange()}.
    */
   public CDL3LINESTRIKE_Stream CDL3LINESTRIKE_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDL3LINESTRIKE_Stream sp = new CDL3LINESTRIKE_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDL3LINESTRIKE_OpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDL3LINESTRIKE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDL3LINESTRIKE openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDL3LINESTRIKE openAndFill: " + retCode);
   }
