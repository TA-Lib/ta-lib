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
 *  011505 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLBREAKAWAY} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLBREAKAWAY_Lookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      return BodyLong_avgPeriod + 4 ;

   }
   RetCode CDLBREAKAWAY_Impl( int startIdx,
                              int endIdx,
                              double inOpen[],
                              double inHigh[],
                              double inLow[],
                              double inClose[],
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              int outInteger[] )
   {
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLBREAKAWAY_Lookback();
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
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black (white)
       * - second candle: black (white) day whose body gaps down (up)
       * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
       * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
       * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
       * The meaning of "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that breakaway is significant in a trend opposite to the last candle, while this
       * function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) && /* 1st, 2nd, 4th same color, 5th opposite */
             ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) &&
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
             (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 0 - 1 && /* when 1st is black: */
               (Math.max(inOpen[i - 3], inClose[i - 3]) < Math.min(inOpen[i - 4], inClose[i - 4])) && /* 2nd gaps down */
               inHigh[i - 2] < inHigh[i - 3] &&
               inLow[i - 2] < inLow[i - 3] &&                             /* 3rd has lower high and low than 2nd */
               inHigh[i - 1] < inHigh[i - 2] &&
               inLow[i - 1] < inLow[i - 2] &&                             /* 4th has lower high and low than 3rd */
               inClose[i] > inOpen[i - 3] &&
               inClose[i] < inClose[i - 4] ||                             /* 5th closes inside the gap */
              ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 1 &&     /* when 1st is white: */
               (Math.min(inOpen[i - 3], inClose[i - 3]) > Math.max(inOpen[i - 4], inClose[i - 4])) && /* 2nd gaps up */
               inHigh[i - 2] > inHigh[i - 3] &&
               inLow[i - 2] > inLow[i - 3] &&                             /* 3rd has higher high and low than 2nd */
               inHigh[i - 1] > inHigh[i - 2] &&
               inLow[i - 1] > inLow[i - 2] &&                             /* 4th has higher high and low than 3rd */
               inClose[i] < inOpen[i - 3] &&
               inClose[i] > inClose[i - 4]) )                             /* 5th closes inside the gap */
         {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLBREAKAWAY_Impl( int startIdx,
                              int endIdx,
                              float inOpen[],
                              float inHigh[],
                              float inLow[],
                              float inClose[],
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              int outInteger[] )
   {
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLBREAKAWAY_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - ((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inClose[i - 4]) : ((double)inOpen[i - 4]))) + (((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inOpen[i - 4]) : ((double)inClose[i - 4])) - (double)inLow[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) && (((double)inClose[i - 3] >= (double)inOpen[i - 3]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) && Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - ((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inClose[i - 4]) : ((double)inOpen[i - 4]))) + (((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inOpen[i - 4]) : ((double)inClose[i - 4])) - (double)inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 0 - 1 && (Math.max((double)inOpen[i - 3], (double)inClose[i - 3]) < Math.min((double)inOpen[i - 4], (double)inClose[i - 4])) && (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] < (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] < (double)inLow[i - 2] && (double)inClose[i] > (double)inOpen[i - 3] && (double)inClose[i] < (double)inClose[i - 4] || (((double)inClose[i - 4] >= (double)inOpen[i - 4]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i - 3], (double)inClose[i - 3]) > Math.max((double)inOpen[i - 4], (double)inClose[i - 4])) && (double)inHigh[i - 2] > (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] > (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && (double)inClose[i] < (double)inOpen[i - 3] && (double)inClose[i] > (double)inClose[i - 4]) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 4] - (double)inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 4] - (double)inLow[i - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 4] - ((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inClose[i - 4]) : ((double)inOpen[i - 4]))) + (((((double)inClose[i - 4]) >= ((double)inOpen[i - 4])) ? ((double)inOpen[i - 4]) : ((double)inClose[i - 4])) - (double)inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - 4] - (double)inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - 4] - (double)inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - 4] - ((((double)inClose[BodyLongTrailingIdx - 4]) >= ((double)inOpen[BodyLongTrailingIdx - 4])) ? ((double)inClose[BodyLongTrailingIdx - 4]) : ((double)inOpen[BodyLongTrailingIdx - 4]))) + (((((double)inClose[BodyLongTrailingIdx - 4]) >= ((double)inOpen[BodyLongTrailingIdx - 4])) ? ((double)inOpen[BodyLongTrailingIdx - 4]) : ((double)inClose[BodyLongTrailingIdx - 4])) - (double)inLow[BodyLongTrailingIdx - 4])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A five-candle reversal pattern: a long first candle, a same-colored second
    * candle that gaps away from it by its real body, two more candles extending
    * the move, and an opposite-colored fifth candle that closes back inside the
    * gap. Emits a bullish signal (bottom reversal) or bearish signal (top
    * reversal).
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend the pattern classically assumes (a breakaway matters most against a preceding move).</li>
    * <li>Bulkowski's data shows a directional asymmetry TA-Lib's symmetric output doesn't capture: bullish Breakaway reverses only 59% of the time ("near random"), while bearish Breakaway reverses 63% of the time overall. ([thepatternsite.com](https://thepatternsite.com/BullBreakaway.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLBREAKAWAY_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the fifth candle is white (bullish breakaway),
    *        -100 when it is black (bearish breakaway), 0 otherwise. Must hold at least
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
    * @see Core#CDLGAPSIDESIDEWHITE
    * @see Core#CDLRISEFALL3METHODS
    * @see Core#CDL3LINESTRIKE
    */
   public OutRange CDLBREAKAWAY( int startIdx,
                                 int endIdx,
                                 double inOpen[],
                                 double inHigh[],
                                 double inLow[],
                                 double inClose[],
                                 int outInteger[] )
   {
      requireIndexRange("CDLBREAKAWAY", startIdx, endIdx);
      int guardStart = clampedStart("CDLBREAKAWAY", startIdx, CDLBREAKAWAY_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLBREAKAWAY", "inOpen", inOpen, guardInLen);
      requireLength("CDLBREAKAWAY", "inHigh", inHigh, guardInLen);
      requireLength("CDLBREAKAWAY", "inLow", inLow, guardInLen);
      requireLength("CDLBREAKAWAY", "inClose", inClose, guardInLen);
      requireLength("CDLBREAKAWAY", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLBREAKAWAY_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLBREAKAWAY", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A five-candle reversal pattern: a long first candle, a same-colored second
    * candle that gaps away from it by its real body, two more candles extending
    * the move, and an opposite-colored fifth candle that closes back inside the
    * gap. Emits a bullish signal (bottom reversal) or bearish signal (top
    * reversal).
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend the pattern classically assumes (a breakaway matters most against a preceding move).</li>
    * <li>Bulkowski's data shows a directional asymmetry TA-Lib's symmetric output doesn't capture: bullish Breakaway reverses only 59% of the time ("near random"), while bearish Breakaway reverses 63% of the time overall. ([thepatternsite.com](https://thepatternsite.com/BullBreakaway.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLBREAKAWAY_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the fifth candle is white (bullish breakaway),
    *        -100 when it is black (bearish breakaway), 0 otherwise. Must hold at least
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
    * @see Core#CDLGAPSIDESIDEWHITE
    * @see Core#CDLRISEFALL3METHODS
    * @see Core#CDL3LINESTRIKE
    */
   public OutRange CDLBREAKAWAY( int startIdx,
                                 int endIdx,
                                 float inOpen[],
                                 float inHigh[],
                                 float inLow[],
                                 float inClose[],
                                 int outInteger[] )
   {
      requireIndexRange("CDLBREAKAWAY", startIdx, endIdx);
      int guardStart = clampedStart("CDLBREAKAWAY", startIdx, CDLBREAKAWAY_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLBREAKAWAY", "inOpen", inOpen, guardInLen);
      requireLength("CDLBREAKAWAY", "inHigh", inHigh, guardInLen);
      requireLength("CDLBREAKAWAY", "inLow", inLow, guardInLen);
      requireLength("CDLBREAKAWAY", "inClose", inClose, guardInLen);
      requireLength("CDLBREAKAWAY", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLBREAKAWAY_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLBREAKAWAY", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLBREAKAWAY stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLBREAKAWAY} over the same series.
    * Open with {@link Core#cdlbreakawayOpen}; there is no close — the handle is
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
   public static final class CdlbreakawayStream {
      Core core;
      double BodyLongPeriodTotal;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag3_inOpen;
      double lag4_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag3_inHigh;
      double lag4_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag3_inLow;
      double lag4_inLow;
      double lag1_inClose;
      double lag2_inClose;
      double lag3_inClose;
      double lag4_inClose;
      int ringPos_BodyLongTrailingIdx;
      int ringCap_BodyLongTrailingIdx;
      int ringLag_BodyLongTrailingIdx;
      double[] ring_BodyLongTrailingIdx_derived;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      CdlbreakawayStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLBREAKAWAY} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CdlbreakawayStream( CdlbreakawayStream other ) {
         this.core = other.core;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag3_inOpen = other.lag3_inOpen;
         this.lag4_inOpen = other.lag4_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag3_inHigh = other.lag3_inHigh;
         this.lag4_inHigh = other.lag4_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag3_inLow = other.lag3_inLow;
         this.lag4_inLow = other.lag4_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.lag3_inClose = other.lag3_inClose;
         this.lag4_inClose = other.lag4_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ringLag_BodyLongTrailingIdx = other.ringLag_BodyLongTrailingIdx;
         this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
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
            throw new TaLibArgumentException("CDLBREAKAWAY update: BadParam", RetCode.BadParam);
         }
         core.cdlbreakawayStepImpl(this, inOpen, inHigh, inLow, inClose);
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
         requireArgument("CDLBREAKAWAY updateAndFill", "inOpen", inOpen);
         requireArgument("CDLBREAKAWAY updateAndFill", "inHigh", inHigh);
         requireArgument("CDLBREAKAWAY updateAndFill", "inLow", inLow);
         requireArgument("CDLBREAKAWAY updateAndFill", "inClose", inClose);
         requireArgument("CDLBREAKAWAY updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLBREAKAWAY updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CDLBREAKAWAY updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cdlbreakawayStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
            throw new TaLibArgumentException("CDLBREAKAWAY peek: BadParam", RetCode.BadParam);
         CdlbreakawayStream sp = this;
         int cur_outInteger = 0;
         int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
         int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
         double BodyLong_factor = sp.cs_BodyLong_factor;
         if( ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) == ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) && /* 1st, 2nd, 4th same color, 5th opposite */
             ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) == ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) &&
             ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - ((inClose >= inOpen) ? 1 : 0 - 1) &&
             Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
             (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* when 1st is black: */
               (Math.max(sp.lag3_inOpen, sp.lag3_inClose) < Math.min(sp.lag4_inOpen, sp.lag4_inClose)) && /* 2nd gaps down */
               sp.lag2_inHigh < sp.lag3_inHigh &&
               sp.lag2_inLow < sp.lag3_inLow &&                             /* 3rd has lower high and low than 2nd */
               sp.lag1_inHigh < sp.lag2_inHigh &&
               sp.lag1_inLow < sp.lag2_inLow &&                             /* 4th has lower high and low than 3rd */
               inClose > sp.lag3_inOpen &&
               inClose < sp.lag4_inClose ||                                 /* 5th closes inside the gap */
              ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) == 1 &&     /* when 1st is white: */
               (Math.min(sp.lag3_inOpen, sp.lag3_inClose) > Math.max(sp.lag4_inOpen, sp.lag4_inClose)) && /* 2nd gaps up */
               sp.lag2_inHigh > sp.lag3_inHigh &&
               sp.lag2_inLow > sp.lag3_inLow &&                             /* 3rd has higher high and low than 2nd */
               sp.lag1_inHigh > sp.lag2_inHigh &&
               sp.lag1_inLow > sp.lag2_inLow &&                             /* 4th has higher high and low than 3rd */
               inClose < sp.lag3_inOpen &&
               inClose > sp.lag4_inClose) )                                 /* 5th closes inside the gap */
         {
            cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 100;
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
      public CdlbreakawayStream clone() {
         return new CdlbreakawayStream(this);
      }
   }
   void cdlbreakawayStepImpl( CdlbreakawayStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) == ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) && /* 1st, 2nd, 4th same color, 5th opposite */
          ((sp.lag3_inClose >= sp.lag3_inOpen) ? 1 : 0 - 1) == ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) &&
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - ((inClose >= inOpen) ? 1 : 0 - 1) &&
          Math.abs(sp.lag4_inClose - sp.lag4_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
          (((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* when 1st is black: */
            (Math.max(sp.lag3_inOpen, sp.lag3_inClose) < Math.min(sp.lag4_inOpen, sp.lag4_inClose)) && /* 2nd gaps down */
            sp.lag2_inHigh < sp.lag3_inHigh &&
            sp.lag2_inLow < sp.lag3_inLow &&                             /* 3rd has lower high and low than 2nd */
            sp.lag1_inHigh < sp.lag2_inHigh &&
            sp.lag1_inLow < sp.lag2_inLow &&                             /* 4th has lower high and low than 3rd */
            inClose > sp.lag3_inOpen &&
            inClose < sp.lag4_inClose ||                                 /* 5th closes inside the gap */
           ((sp.lag4_inClose >= sp.lag4_inOpen) ? 1 : 0 - 1) == 1 &&     /* when 1st is white: */
            (Math.min(sp.lag3_inOpen, sp.lag3_inClose) > Math.max(sp.lag4_inOpen, sp.lag4_inClose)) && /* 2nd gaps up */
            sp.lag2_inHigh > sp.lag3_inHigh &&
            sp.lag2_inLow > sp.lag3_inLow &&                             /* 3rd has higher high and low than 2nd */
            sp.lag1_inHigh > sp.lag2_inHigh &&
            sp.lag1_inLow > sp.lag2_inLow &&                             /* 4th has higher high and low than 3rd */
            inClose < sp.lag3_inOpen &&
            inClose > sp.lag4_inClose) )                                 /* 5th closes inside the gap */
      {
         sp.cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag4_inClose - sp.lag4_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag4_inHigh - sp.lag4_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag4_inHigh - (((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inClose) : (sp.lag4_inOpen))) + ((((sp.lag4_inClose) >= (sp.lag4_inOpen)) ? (sp.lag4_inOpen) : (sp.lag4_inClose)) - sp.lag4_inLow)) : 0.0))) - sp.ring_BodyLongTrailingIdx_derived[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - 4) % sp.ringCap_BodyLongTrailingIdx];
      sp.lag4_inOpen = sp.lag3_inOpen;
      sp.lag3_inOpen = sp.lag2_inOpen;
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag4_inHigh = sp.lag3_inHigh;
      sp.lag3_inHigh = sp.lag2_inHigh;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag4_inLow = sp.lag3_inLow;
      sp.lag3_inLow = sp.lag2_inLow;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
      sp.lag4_inClose = sp.lag3_inClose;
      sp.lag3_inClose = sp.lag2_inClose;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
      sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
      if( sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx ) {
         sp.ringPos_BodyLongTrailingIdx = 0;
      }
   }
   private RetCode cdlbreakawayOpenImpl( CdlbreakawayStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyLongTrailingIdx = 0;
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
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLBREAKAWAY_Lookback();
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
      BodyLongPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black (white)
       * - second candle: black (white) day whose body gaps down (up)
       * - third candle: black or white day with lower (higher) high and lower (higher) low than prior candle's
       * - fourth candle: black (white) day with lower (higher) high and lower (higher) low than prior candle's
       * - fifth candle: white (black) day that closes inside the gap, erasing the prior 3 days
       * The meaning of "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that breakaway is significant in a trend opposite to the last candle, while this
       * function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) && /* 1st, 2nd, 4th same color, 5th opposite */
             ((inClose[i - 3] >= inOpen[i - 3]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) &&
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) &&
             Math.abs(inClose[i - 4] - inOpen[i - 4]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st long */
             (((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 0 - 1 && /* when 1st is black: */
               (Math.max(inOpen[i - 3], inClose[i - 3]) < Math.min(inOpen[i - 4], inClose[i - 4])) && /* 2nd gaps down */
               inHigh[i - 2] < inHigh[i - 3] &&
               inLow[i - 2] < inLow[i - 3] &&                             /* 3rd has lower high and low than 2nd */
               inHigh[i - 1] < inHigh[i - 2] &&
               inLow[i - 1] < inLow[i - 2] &&                             /* 4th has lower high and low than 3rd */
               inClose[i] > inOpen[i - 3] &&
               inClose[i] < inClose[i - 4] ||                             /* 5th closes inside the gap */
              ((inClose[i - 4] >= inOpen[i - 4]) ? 1 : 0 - 1) == 1 &&     /* when 1st is white: */
               (Math.min(inOpen[i - 3], inClose[i - 3]) > Math.max(inOpen[i - 4], inClose[i - 4])) && /* 2nd gaps up */
               inHigh[i - 2] > inHigh[i - 3] &&
               inLow[i - 2] > inLow[i - 3] &&                             /* 3rd has higher high and low than 2nd */
               inHigh[i - 1] > inHigh[i - 2] &&
               inLow[i - 1] > inLow[i - 2] &&                             /* 4th has higher high and low than 3rd */
               inClose[i] < inOpen[i - 3] &&
               inClose[i] > inClose[i - 4]) )                             /* 5th closes inside the gap */
         {
            outInteger[outIdx++ * outStride] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 4] - inOpen[i - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 4] - inLow[i - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 4] - (((inClose[i - 4]) >= (inOpen[i - 4])) ? (inClose[i - 4]) : (inOpen[i - 4]))) + ((((inClose[i - 4]) >= (inOpen[i - 4])) ? (inOpen[i - 4]) : (inClose[i - 4])) - inLow[i - 4])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - 4] - inOpen[BodyLongTrailingIdx - 4])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - 4] - inLow[BodyLongTrailingIdx - 4]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - 4] - (((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inClose[BodyLongTrailingIdx - 4]) : (inOpen[BodyLongTrailingIdx - 4]))) + ((((inClose[BodyLongTrailingIdx - 4]) >= (inOpen[BodyLongTrailingIdx - 4])) ? (inOpen[BodyLongTrailingIdx - 4]) : (inClose[BodyLongTrailingIdx - 4])) - inLow[BodyLongTrailingIdx - 4])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      int cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 5;
      if( capLag_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyLongTrailingIdx = (cap_BodyLongTrailingIdx > 0)? cap_BodyLongTrailingIdx : 1;
      double[] capRing_BodyLongTrailingIdx_derived = new double[allocN_BodyLongTrailingIdx];
      for( int fillJ = historyLen - cap_BodyLongTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyLongTrailingIdx_derived[fillJ % cap_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyLong_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyLong_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag3_inOpen = inOpen[historyLen - 3];
      sp.lag4_inOpen = inOpen[historyLen - 4];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag3_inHigh = inHigh[historyLen - 3];
      sp.lag4_inHigh = inHigh[historyLen - 4];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag3_inLow = inLow[historyLen - 3];
      sp.lag4_inLow = inLow[historyLen - 4];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.lag3_inClose = inClose[historyLen - 3];
      sp.lag4_inClose = inClose[historyLen - 4];
      sp.ringPos_BodyLongTrailingIdx = historyLen % cap_BodyLongTrailingIdx;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ringLag_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_derived = capRing_BodyLongTrailingIdx_derived;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cdlbreakawayOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CdlbreakawayStream cdlbreakawayOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlbreakawayStream sp = new CdlbreakawayStream(this);
      RetCode retCode = cdlbreakawayOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLBREAKAWAY openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLBREAKAWAY openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLBREAKAWAY openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cdlbreakawayOpen (composition seam). */
   CdlbreakawayStream cdlbreakawayOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlbreakawayStream sp = new CdlbreakawayStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdlbreakawayOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLBREAKAWAY open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLBREAKAWAY open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLBREAKAWAY open: " + retCode, retCode);
   }
   /**
    * Open a live CDLBREAKAWAY stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLBREAKAWAY} at that bar.
    * <p>The history must hold at least {@code CDLBREAKAWAY_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CdlbreakawayStream cdlbreakawayOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLBREAKAWAY open", "inOpen", inOpen);
      requireHistory("CDLBREAKAWAY open", inOpen.length);
      requireArgument("CDLBREAKAWAY open", "inHigh", inHigh);
      requireArgument("CDLBREAKAWAY open", "inLow", inLow);
      requireArgument("CDLBREAKAWAY open", "inClose", inClose);
      requireHistoryLength("CDLBREAKAWAY open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLBREAKAWAY open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLBREAKAWAY open", "inClose", inClose.length, inOpen.length);
      return cdlbreakawayOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlbreakawayOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CDLBREAKAWAY} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CdlbreakawayStream#outRange()}.
    */
   public CdlbreakawayStream cdlbreakawayOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLBREAKAWAY openAndFill", "inOpen", inOpen);
      requireHistory("CDLBREAKAWAY openAndFill", inOpen.length);
      requireArgument("CDLBREAKAWAY openAndFill", "inHigh", inHigh);
      requireArgument("CDLBREAKAWAY openAndFill", "inLow", inLow);
      requireArgument("CDLBREAKAWAY openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLBREAKAWAY openAndFill", inOpen.length, CDLBREAKAWAY_Lookback());
      requireHistoryLength("CDLBREAKAWAY openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLBREAKAWAY openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLBREAKAWAY openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLBREAKAWAY openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLBREAKAWAY openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdlbreakawayOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
