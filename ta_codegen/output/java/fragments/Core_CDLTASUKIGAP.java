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
 *  011605 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLTASUKIGAP} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLTASUKIGAP_Lookback( )
   {
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      return Near_avgPeriod + 2 ;

   }
   RetCode CDLTASUKIGAP_Impl( int startIdx,
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
      int i = 0;
      int outIdx = 0;
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
      lookbackTotal = CDLTASUKIGAP_Lookback();
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
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - upside (downside) gap
       * - first candle after the window: white (black) candlestick
       * - second candle: black (white) candlestick that opens within the previous real body and closes under (above)
       *   the previous real body inside the gap
       * - the size of two real bodies should be near the same
       * The meaning of "near" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that tasuki gap is significant when it appears in a trend, while this function does
       * not consider it
       */
      outIdx = 0;
      do {
         if( (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && inOpen[i] < inClose[i - 1] && inOpen[i] > inOpen[i - 1] && inClose[i] < inOpen[i - 1] && inClose[i] > Math.max(inClose[i - 2], inOpen[i - 2]) && Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inOpen[i] < inOpen[i - 1] && inOpen[i] > inClose[i - 1] && inClose[i] > inOpen[i - 1] && inClose[i] < Math.min(inClose[i - 2], inOpen[i - 2]) && Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            /* upside gap */
            /* 1st: white */
            /* 2nd: black */
            /* that opens within the white rb */
            /* and closes under the white rb */
            /* inside the gap */
            /* size of 2 rb near the same */
            /* downside gap */
            /* 1st: black */
            /* 2nd: white */
            /* that opens within the black rb */
            /* and closes above the black rb */
            /* inside the gap */
            /* size of 2 rb near the same */
            outInteger[outIdx++] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLTASUKIGAP_Impl( int startIdx,
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
      int i = 0;
      int outIdx = 0;
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
      lookbackTotal = CDLTASUKIGAP_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (double)inOpen[i] < (double)inClose[i - 1] && (double)inOpen[i] > (double)inOpen[i - 1] && (double)inClose[i] < (double)inOpen[i - 1] && (double)inClose[i] > Math.max((double)inClose[i - 2], (double)inOpen[i - 2]) && Math.abs(Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - Math.abs((double)inClose[i] - (double)inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2])) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (double)inOpen[i] < (double)inOpen[i - 1] && (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] > (double)inOpen[i - 1] && (double)inClose[i] < Math.min((double)inClose[i - 2], (double)inOpen[i - 2]) && Math.abs(Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) - Math.abs((double)inClose[i] - (double)inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 1] - (double)inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 1] - (double)inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 1] - ((((double)inClose[NearTrailingIdx - 1]) >= ((double)inOpen[NearTrailingIdx - 1])) ? ((double)inClose[NearTrailingIdx - 1]) : ((double)inOpen[NearTrailingIdx - 1]))) + (((((double)inClose[NearTrailingIdx - 1]) >= ((double)inOpen[NearTrailingIdx - 1])) ? ((double)inOpen[NearTrailingIdx - 1]) : ((double)inClose[NearTrailingIdx - 1])) - (double)inLow[NearTrailingIdx - 1])) : 0.0)));
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A three-candle pattern: a real-body-gapping candle followed by an
    * opposite-color candle that opens inside its body and closes back into the
    * gap without filling it. An upside gap is a bullish continuation signal; a
    * downside gap is a bearish continuation signal.
    * <p><b>Notes</b>
    * <ul>
    * <li>This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.</li>
    * <li>Bulkowski's testing found the downside Tasuki Gap actually acts as a bullish REVERSAL 54% of the time — opposite its textbook bearish-continuation label — while the upside variant does continue as labeled, but only 57% of the time ("near random"). ([thepatternsite.com](https://thepatternsite.com/DownsideTasukiGap.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLTASUKIGAP_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 on a bullish (upside-gap) tasuki gap, -100 on a
    *        bearish (downside-gap) tasuki gap, 0 otherwise. Sign equals the color of
    *        the gap candle i-1 (candlecolor(i-1)*100) Must hold at least
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
    * @see Core#CDLXSIDEGAP3METHODS
    */
   public OutRange CDLTASUKIGAP( int startIdx,
                                 int endIdx,
                                 double inOpen[],
                                 double inHigh[],
                                 double inLow[],
                                 double inClose[],
                                 int outInteger[] )
   {
      requireIndexRange("CDLTASUKIGAP", startIdx, endIdx);
      int guardStart = clampedStart("CDLTASUKIGAP", startIdx, CDLTASUKIGAP_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLTASUKIGAP", "inOpen", inOpen, guardInLen);
      requireLength("CDLTASUKIGAP", "inHigh", inHigh, guardInLen);
      requireLength("CDLTASUKIGAP", "inLow", inLow, guardInLen);
      requireLength("CDLTASUKIGAP", "inClose", inClose, guardInLen);
      requireLength("CDLTASUKIGAP", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLTASUKIGAP_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLTASUKIGAP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A three-candle pattern: a real-body-gapping candle followed by an
    * opposite-color candle that opens inside its body and closes back into the
    * gap without filling it. An upside gap is a bullish continuation signal; a
    * downside gap is a bearish continuation signal.
    * <p><b>Notes</b>
    * <ul>
    * <li>This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.</li>
    * <li>Bulkowski's testing found the downside Tasuki Gap actually acts as a bullish REVERSAL 54% of the time — opposite its textbook bearish-continuation label — while the upside variant does continue as labeled, but only 57% of the time ("near random"). ([thepatternsite.com](https://thepatternsite.com/DownsideTasukiGap.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLTASUKIGAP_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 on a bullish (upside-gap) tasuki gap, -100 on a
    *        bearish (downside-gap) tasuki gap, 0 otherwise. Sign equals the color of
    *        the gap candle i-1 (candlecolor(i-1)*100) Must hold at least
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
    * @see Core#CDLXSIDEGAP3METHODS
    */
   public OutRange CDLTASUKIGAP( int startIdx,
                                 int endIdx,
                                 float inOpen[],
                                 float inHigh[],
                                 float inLow[],
                                 float inClose[],
                                 int outInteger[] )
   {
      requireIndexRange("CDLTASUKIGAP", startIdx, endIdx);
      int guardStart = clampedStart("CDLTASUKIGAP", startIdx, CDLTASUKIGAP_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLTASUKIGAP", "inOpen", inOpen, guardInLen);
      requireLength("CDLTASUKIGAP", "inHigh", inHigh, guardInLen);
      requireLength("CDLTASUKIGAP", "inLow", inLow, guardInLen);
      requireLength("CDLTASUKIGAP", "inClose", inClose, guardInLen);
      requireLength("CDLTASUKIGAP", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLTASUKIGAP_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLTASUKIGAP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLTASUKIGAP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLTASUKIGAP} over the same series.
    * Open with {@link Core#cdltasukigapOpen}; there is no close — the handle is
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
   public static final class CdltasukigapStream {
      Core core;
      double NearPeriodTotal;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inHigh;
      double lag1_inLow;
      double lag1_inClose;
      double lag2_inClose;
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

      CdltasukigapStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLTASUKIGAP} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CdltasukigapStream( CdltasukigapStream other ) {
         this.core = other.core;
         this.NearPeriodTotal = other.NearPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
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
            throw new TaLibArgumentException("CDLTASUKIGAP update: BadParam", RetCode.BadParam);
         }
         core.cdltasukigapStepImpl(this, inOpen, inHigh, inLow, inClose);
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
         requireArgument("CDLTASUKIGAP updateAndFill", "inOpen", inOpen);
         requireArgument("CDLTASUKIGAP updateAndFill", "inHigh", inHigh);
         requireArgument("CDLTASUKIGAP updateAndFill", "inLow", inLow);
         requireArgument("CDLTASUKIGAP updateAndFill", "inClose", inClose);
         requireArgument("CDLTASUKIGAP updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLTASUKIGAP updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CDLTASUKIGAP updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cdltasukigapStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
            throw new TaLibArgumentException("CDLTASUKIGAP peek: BadParam", RetCode.BadParam);
         CdltasukigapStream sp = this;
         int cur_outInteger = sp.cur_outInteger;
         int Near_rangeType = sp.cs_Near_rangeType;
         int Near_avgPeriod = sp.cs_Near_avgPeriod;
         double Near_factor = sp.cs_Near_factor;
         if( (Math.min(sp.lag1_inOpen, sp.lag1_inClose) > Math.max(sp.lag2_inOpen, sp.lag2_inClose)) && ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 && inOpen < sp.lag1_inClose && inOpen > sp.lag1_inOpen && inClose < sp.lag1_inOpen && inClose > Math.max(sp.lag2_inClose, sp.lag2_inOpen) && Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (Math.max(sp.lag1_inOpen, sp.lag1_inClose) < Math.min(sp.lag2_inOpen, sp.lag2_inClose)) && ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && ((inClose >= inOpen) ? 1 : 0 - 1) == 1 && inOpen < sp.lag1_inOpen && inOpen > sp.lag1_inClose && inClose > sp.lag1_inOpen && inClose < Math.min(sp.lag2_inClose, sp.lag2_inOpen) && Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            /* upside gap */
            /* 1st: white */
            /* 2nd: black */
            /* that opens within the white rb */
            /* and closes under the white rb */
            /* inside the gap */
            /* size of 2 rb near the same */
            /* downside gap */
            /* 1st: black */
            /* 2nd: white */
            /* that opens within the black rb */
            /* and closes above the black rb */
            /* inside the gap */
            /* size of 2 rb near the same */
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
      public CdltasukigapStream clone() {
         return new CdltasukigapStream(this);
      }
   }
   void cdltasukigapStepImpl( CdltasukigapStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int Near_rangeType = sp.cs_Near_rangeType;
      int Near_avgPeriod = sp.cs_Near_avgPeriod;
      double Near_factor = sp.cs_Near_factor;
      sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType == 1) ? (inHigh - inLow) : ((Near_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( (Math.min(sp.lag1_inOpen, sp.lag1_inClose) > Math.max(sp.lag2_inOpen, sp.lag2_inClose)) && ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 && inOpen < sp.lag1_inClose && inOpen > sp.lag1_inOpen && inClose < sp.lag1_inOpen && inClose > Math.max(sp.lag2_inClose, sp.lag2_inOpen) && Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (Math.max(sp.lag1_inOpen, sp.lag1_inClose) < Math.min(sp.lag2_inOpen, sp.lag2_inClose)) && ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && ((inClose >= inOpen) ? 1 : 0 - 1) == 1 && inOpen < sp.lag1_inOpen && inOpen > sp.lag1_inClose && inClose > sp.lag1_inOpen && inClose < Math.min(sp.lag2_inClose, sp.lag2_inOpen) && Math.abs(Math.abs(sp.lag1_inClose - sp.lag1_inOpen) - Math.abs(inClose - inOpen)) < ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
         /* upside gap */
         /* 1st: white */
         /* 2nd: black */
         /* that opens within the white rb */
         /* and closes under the white rb */
         /* inside the gap */
         /* size of 2 rb near the same */
         /* downside gap */
         /* 1st: black */
         /* 2nd: white */
         /* that opens within the black rb */
         /* and closes above the black rb */
         /* inside the gap */
         /* size of 2 rb near the same */
         sp.cur_outInteger = ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Near_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Near_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_NearTrailingIdx_derived[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 1) % sp.ringCap_NearTrailingIdx];
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag1_inHigh = inHigh;
      sp.lag1_inLow = inLow;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
      sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
      if( sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx ) {
         sp.ringPos_NearTrailingIdx = 0;
      }
   }
   private RetCode cdltasukigapOpenImpl( CdltasukigapStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double NearPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
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
      lookbackTotal = CDLTASUKIGAP_Lookback();
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
      NearTrailingIdx = startIdx - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - upside (downside) gap
       * - first candle after the window: white (black) candlestick
       * - second candle: black (white) candlestick that opens within the previous real body and closes under (above)
       *   the previous real body inside the gap
       * - the size of two real bodies should be near the same
       * The meaning of "near" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that tasuki gap is significant when it appears in a trend, while this function does
       * not consider it
       */
      outIdx = 0;
      do {
         if( (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && inOpen[i] < inClose[i - 1] && inOpen[i] > inOpen[i - 1] && inClose[i] < inOpen[i - 1] && inClose[i] > Math.max(inClose[i - 2], inOpen[i - 2]) && Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && inOpen[i] < inOpen[i - 1] && inOpen[i] > inClose[i - 1] && inClose[i] > inOpen[i - 1] && inClose[i] < Math.min(inClose[i - 2], inOpen[i - 2]) && Math.abs(Math.abs(inClose[i - 1] - inOpen[i - 1]) - Math.abs(inClose[i] - inOpen[i])) < ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) ) {
            /* upside gap */
            /* 1st: white */
            /* 2nd: black */
            /* that opens within the white rb */
            /* and closes under the white rb */
            /* inside the gap */
            /* size of 2 rb near the same */
            /* downside gap */
            /* 1st: black */
            /* 2nd: white */
            /* that opens within the black rb */
            /* and closes above the black rb */
            /* inside the gap */
            /* size of 2 rb near the same */
            outInteger[outIdx++ * outStride] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Near_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Near_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 1] - inOpen[NearTrailingIdx - 1])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 1] - inLow[NearTrailingIdx - 1]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 1] - (((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inClose[NearTrailingIdx - 1]) : (inOpen[NearTrailingIdx - 1]))) + ((((inClose[NearTrailingIdx - 1]) >= (inOpen[NearTrailingIdx - 1])) ? (inOpen[NearTrailingIdx - 1]) : (inClose[NearTrailingIdx - 1])) - inLow[NearTrailingIdx - 1])) : 0.0)));
         i += 1;
         NearTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
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
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
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
   /* cdltasukigapOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CdltasukigapStream cdltasukigapOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdltasukigapStream sp = new CdltasukigapStream(this);
      RetCode retCode = cdltasukigapOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLTASUKIGAP openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLTASUKIGAP openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLTASUKIGAP openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cdltasukigapOpen (composition seam). */
   CdltasukigapStream cdltasukigapOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdltasukigapStream sp = new CdltasukigapStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdltasukigapOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLTASUKIGAP open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLTASUKIGAP open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLTASUKIGAP open: " + retCode, retCode);
   }
   /**
    * Open a live CDLTASUKIGAP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLTASUKIGAP} at that bar.
    * <p>The history must hold at least {@code CDLTASUKIGAP_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CdltasukigapStream cdltasukigapOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLTASUKIGAP open", "inOpen", inOpen);
      requireHistory("CDLTASUKIGAP open", inOpen.length);
      requireArgument("CDLTASUKIGAP open", "inHigh", inHigh);
      requireArgument("CDLTASUKIGAP open", "inLow", inLow);
      requireArgument("CDLTASUKIGAP open", "inClose", inClose);
      requireHistoryLength("CDLTASUKIGAP open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLTASUKIGAP open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLTASUKIGAP open", "inClose", inClose.length, inOpen.length);
      return cdltasukigapOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdltasukigapOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CDLTASUKIGAP} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CdltasukigapStream#outRange()}.
    */
   public CdltasukigapStream cdltasukigapOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLTASUKIGAP openAndFill", "inOpen", inOpen);
      requireHistory("CDLTASUKIGAP openAndFill", inOpen.length);
      requireArgument("CDLTASUKIGAP openAndFill", "inHigh", inHigh);
      requireArgument("CDLTASUKIGAP openAndFill", "inLow", inLow);
      requireArgument("CDLTASUKIGAP openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLTASUKIGAP openAndFill", inOpen.length, CDLTASUKIGAP_Lookback());
      requireHistoryLength("CDLTASUKIGAP openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLTASUKIGAP openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLTASUKIGAP openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLTASUKIGAP openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLTASUKIGAP openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdltasukigapOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
