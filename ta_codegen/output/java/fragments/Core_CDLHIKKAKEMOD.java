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
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  122605 AC   Creation
 *  071226 MF,CC Streaming-friendly rewrite: carry the confirmation state
 *               (countdown + cached 3rd-candle high/low) instead of the absolute
 *               bar index, so the per-bar logic reads no cursor. Bit-identical
 *               batch results (verified vs v0.6.4).
 */

   /**
    * Number of leading input bars {@link Core#CDLHIKKAKEMOD} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLHIKKAKEMOD_Lookback( )
   {
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      return Math.max(1, Near_avgPeriod) + 5 ;

   }
   RetCode CDLHIKKAKEMOD_Internal( int startIdx,
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
      int patternResult = 0;
      int patternCount = 0;
      double patternHigh = 0;
      double patternLow = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Confirmation window countdown (replaces the absolute patternIdx guard)
       * and a cache of the 3rd candle's high/low (replaces inHigh/inLow
       * [patternIdx-1]) so nothing in the per-bar logic references the cursor.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLHIKKAKEMOD_Lookback();
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
      NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx - 3 ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      patternCount = 0;
      patternResult = 0;
      patternHigh = 0.0;
      patternLow = 0.0;
      i = startIdx - 3;
      while( i < startIdx ) {
         /* copy here the pattern recognition code below */
         if( inHigh[i - 2] < inHigh[i - 3] &&
             inLow[i - 2] > inLow[i - 3] &&   /* 2nd: lower high and higher low than 1st */
             inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 3rd: lower high and higher low than 2nd */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] && inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1] && inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 4th: lower high and lower low (bull) 2nd: close near the low (bear) 4th: higher high and higher low (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = inHigh[i - 1];
            patternLow = inLow[i - 1];
            patternCount = 4;
         } else if( patternCount > 0 &&
             (patternResult > 0 && inClose[i] > patternHigh || patternResult < 0 && inClose[i] < patternLow) ) /* search for confirmation if modified hikkake was no more than 3 bars ago close higher than the high of 3rd close lower than the low of 3rd */
         {
            patternCount = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) - Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         if( patternCount > 0 ) {
            patternCount -= 1;
         }
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle
       * - second candle: candle with range less than first candle and close near the bottom (near the top)
       * - third candle: lower high and higher low than 2nd
       * - fourth candle: lower high and lower low (higher high and higher low) than 3rd
       * outInteger[hikkake bar] is positive (1 to 100) or negative (-1 to -100) meaning bullish or bearish hikkake
       * Confirmation could come in the next 3 days with:
       * - a day that closes higher than the high (lower than the low) of the 3rd candle
       * outInteger[confirmationbar] is equal to 100 + the bullish hikkake result or -100 - the bearish hikkake result
       * Note: if confirmation and a new hikkake come at the same bar, only the new hikkake is reported (the new hikkake
       * overwrites the confirmation of the old hikkake);
       * the user should consider that modified hikkake is a reversal pattern, while hikkake could be both a reversal
       * or a continuation pattern, so bullish (bearish) modified hikkake is significant when appearing in a downtrend
       * (uptrend)
       */
      outIdx = 0;
      do {
         if( inHigh[i - 2] < inHigh[i - 3] &&
             inLow[i - 2] > inLow[i - 3] &&   /* 2nd: lower high and higher low than 1st */
             inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 3rd: lower high and higher low than 2nd */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] && inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1] && inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 4th: lower high and lower low (bull) 2nd: close near the low (bear) 4th: higher high and higher low (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = inHigh[i - 1];
            patternLow = inLow[i - 1];
            patternCount = 4;
            outInteger[outIdx++] = patternResult;
         } else if( patternCount > 0 &&
             (patternResult > 0 && inClose[i] > patternHigh || patternResult < 0 && inClose[i] < patternLow) ) /* search for confirmation if modified hikkake was no more than 3 bars ago close higher than the high of 3rd close lower than the low of 3rd */
         {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternCount = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) - Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         if( patternCount > 0 ) {
            patternCount -= 1;
         }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLHIKKAKEMOD_Internal( int startIdx,
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
      int patternResult = 0;
      int patternCount = 0;
      double patternHigh = 0;
      double patternLow = 0;
      int Near_rangeType = this.candleSettings[CandleSettingType.Near.ordinal()].rangeType.ordinal();
      int Near_avgPeriod = this.candleSettings[CandleSettingType.Near.ordinal()].avgPeriod;
      double Near_factor = this.candleSettings[CandleSettingType.Near.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLHIKKAKEMOD_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx - 3 ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      patternCount = 0;
      patternResult = 0;
      patternHigh = 0.0;
      patternLow = 0.0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] && (double)inClose[i - 2] <= (double)inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1] && (double)inClose[i - 2] >= (double)inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = (double)inHigh[i - 1];
            patternLow = (double)inLow[i - 1];
            patternCount = 4;
         } else if( patternCount > 0 && (patternResult > 0 && (double)inClose[i] > patternHigh || patternResult < 0 && (double)inClose[i] < patternLow) ) {
            patternCount = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) - Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         if( patternCount > 0 ) {
            patternCount -= 1;
         }
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] && (double)inClose[i - 2] <= (double)inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1] && (double)inClose[i - 2] >= (double)inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = (double)inHigh[i - 1];
            patternLow = (double)inLow[i - 1];
            patternCount = 4;
            outInteger[outIdx++] = patternResult;
         } else if( patternCount > 0 && (patternResult > 0 && (double)inClose[i] > patternHigh || patternResult < 0 && (double)inClose[i] < patternLow) ) {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternCount = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) - Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         if( patternCount > 0 ) {
            patternCount -= 1;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A four-candle pattern: two successively narrower inside bars, then a
    * breakout bar, with the second candle closing near one extreme of its
    * range. Bullish or bearish reversal signal. Bullish (+) or bearish (-)
    * reversal; per the code's note it is significant in a downtrend (bull) or
    * uptrend (bear), context the code does not verify.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that this reversal pattern assumes.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHIKKAKEMOD_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 bullish hikkake bar, -100 bearish; +200 confirmed
    *        bullish, -200 confirmed bearish (confirmation adds another +/-100); 0
    *        otherwise. Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLHIKKAKE
    */
   public OutRange CDLHIKKAKEMOD( int startIdx,
                                  int endIdx,
                                  double inOpen[],
                                  double inHigh[],
                                  double inLow[],
                                  double inClose[],
                                  int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIKKAKEMOD_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHIKKAKEMOD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A four-candle pattern: two successively narrower inside bars, then a
    * breakout bar, with the second candle closing near one extreme of its
    * range. Bullish or bearish reversal signal. Bullish (+) or bearish (-)
    * reversal; per the code's note it is significant in a downtrend (bull) or
    * uptrend (bear), context the code does not verify.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that this reversal pattern assumes.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHIKKAKEMOD_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 bullish hikkake bar, -100 bearish; +200 confirmed
    *        bullish, -200 confirmed bearish (confirmation adds another +/-100); 0
    *        otherwise. Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLHIKKAKE
    */
   public OutRange CDLHIKKAKEMOD( int startIdx,
                                  int endIdx,
                                  float inOpen[],
                                  float inHigh[],
                                  float inLow[],
                                  float inClose[],
                                  int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIKKAKEMOD_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHIKKAKEMOD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLHIKKAKEMOD stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLHIKKAKEMOD} over the same series.
    * Open with {@link Core#CDLHIKKAKEMOD_Open}; there is no close — the handle is
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
   public static final class CDLHIKKAKEMOD_Stream {
      Core core;
      double NearPeriodTotal;
      int patternResult;
      int patternCount;
      double patternHigh;
      double patternLow;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag3_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag3_inLow;
      double lag1_inClose;
      double lag2_inClose;
      int ringPos_NearTrailingIdx;
      int ringCap_NearTrailingIdx;
      int ringLag_NearTrailingIdx;
      double[] ring_NearTrailingIdx_inOpen;
      double[] ring_NearTrailingIdx_inHigh;
      double[] ring_NearTrailingIdx_inLow;
      double[] ring_NearTrailingIdx_inClose;
      int cs_Near_rangeType;
      int cs_Near_avgPeriod;
      double cs_Near_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLHIKKAKEMOD_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLHIKKAKEMOD_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLHIKKAKEMOD_Stream( CDLHIKKAKEMOD_Stream other ) {
         this.core = other.core;
         this.NearPeriodTotal = other.NearPeriodTotal;
         this.patternResult = other.patternResult;
         this.patternCount = other.patternCount;
         this.patternHigh = other.patternHigh;
         this.patternLow = other.patternLow;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag3_inHigh = other.lag3_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag3_inLow = other.lag3_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.ringPos_NearTrailingIdx = other.ringPos_NearTrailingIdx;
         this.ringCap_NearTrailingIdx = other.ringCap_NearTrailingIdx;
         this.ringLag_NearTrailingIdx = other.ringLag_NearTrailingIdx;
         this.ring_NearTrailingIdx_inOpen = other.ring_NearTrailingIdx_inOpen.clone();
         this.ring_NearTrailingIdx_inHigh = other.ring_NearTrailingIdx_inHigh.clone();
         this.ring_NearTrailingIdx_inLow = other.ring_NearTrailingIdx_inLow.clone();
         this.ring_NearTrailingIdx_inClose = other.ring_NearTrailingIdx_inClose.clone();
         this.cs_Near_rangeType = other.cs_Near_rangeType;
         this.cs_Near_avgPeriod = other.cs_Near_avgPeriod;
         this.cs_Near_factor = other.cs_Near_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDLHIKKAKEMOD_Stream other ) {
         this.core = other.core;
         this.NearPeriodTotal = other.NearPeriodTotal;
         this.patternResult = other.patternResult;
         this.patternCount = other.patternCount;
         this.patternHigh = other.patternHigh;
         this.patternLow = other.patternLow;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag3_inHigh = other.lag3_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag3_inLow = other.lag3_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
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
         this.cs_Near_rangeType = other.cs_Near_rangeType;
         this.cs_Near_avgPeriod = other.cs_Near_avgPeriod;
         this.cs_Near_factor = other.cs_Near_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDLHIKKAKEMOD_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.CDLHIKKAKEMOD_StreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CDLHIKKAKEMOD_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLHIKKAKEMOD_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLHIKKAKEMOD_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLHIKKAKEMOD_Stream copy() {
         return new CDLHIKKAKEMOD_Stream(this);
      }
   }
   void CDLHIKKAKEMOD_StreamStep( CDLHIKKAKEMOD_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int Near_rangeType = sp.cs_Near_rangeType;
      int Near_avgPeriod = sp.cs_Near_avgPeriod;
      double Near_factor = sp.cs_Near_factor;
      sp.ring_NearTrailingIdx_inOpen[sp.ringPos_NearTrailingIdx] = inOpen;
      sp.ring_NearTrailingIdx_inHigh[sp.ringPos_NearTrailingIdx] = inHigh;
      sp.ring_NearTrailingIdx_inLow[sp.ringPos_NearTrailingIdx] = inLow;
      sp.ring_NearTrailingIdx_inClose[sp.ringPos_NearTrailingIdx] = inClose;
      if( sp.lag2_inHigh < sp.lag3_inHigh &&
          sp.lag2_inLow > sp.lag3_inLow &&   /* 2nd: lower high and higher low than 1st */
          sp.lag1_inHigh < sp.lag2_inHigh &&
          sp.lag1_inLow > sp.lag2_inLow &&   /* 3rd: lower high and higher low than 2nd */
          (inHigh < sp.lag1_inHigh && inLow < sp.lag1_inLow && sp.lag2_inClose <= sp.lag2_inLow + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || inHigh > sp.lag1_inHigh && inLow > sp.lag1_inLow && sp.lag2_inClose >= sp.lag2_inHigh - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 4th: lower high and lower low (bull) 2nd: close near the low (bear) 4th: higher high and higher low (bull) 2nd: close near the top */
      {
         sp.patternResult = 100 * ((inHigh < sp.lag1_inHigh) ? 1 : 0 - 1);
         sp.patternHigh = sp.lag1_inHigh;
         sp.patternLow = sp.lag1_inLow;
         sp.patternCount = 4;
         sp.cur_outInteger = sp.patternResult;
      } else if( sp.patternCount > 0 &&
          (sp.patternResult > 0 && inClose > sp.patternHigh || sp.patternResult < 0 && inClose < sp.patternLow) ) /* search for confirmation if modified hikkake was no more than 3 bars ago close higher than the high of 3rd close lower than the low of 3rd */
      {
         sp.cur_outInteger = sp.patternResult + 100 * ((sp.patternResult > 0) ? 1 : 0 - 1);
         sp.patternCount = 0;
      } else {
         sp.cur_outInteger = 0;
      }
      sp.NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(sp.ring_NearTrailingIdx_inClose[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inOpen[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx])) : ((Near_rangeType == 1) ? (sp.ring_NearTrailingIdx_inHigh[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inLow[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx]) : ((Near_rangeType == 2) ? ((sp.ring_NearTrailingIdx_inHigh[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inLow[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx]) - Math.abs(sp.ring_NearTrailingIdx_inClose[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx] - sp.ring_NearTrailingIdx_inOpen[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx])) : 0.0)));
      if( sp.patternCount > 0 ) {
         sp.patternCount -= 1;
      }
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag3_inHigh = sp.lag2_inHigh;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag3_inLow = sp.lag2_inLow;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
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
   }
   private RetCode CDLHIKKAKEMOD_OpenCore( CDLHIKKAKEMOD_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double NearPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int NearTrailingIdx = 0;
      int lookbackTotal = 0;
      int patternResult = 0;
      int patternCount = 0;
      double patternHigh = 0;
      double patternLow = 0;
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
      /* Confirmation window countdown (replaces the absolute patternIdx guard)
       * and a cache of the 3rd candle's high/low (replaces inHigh/inLow
       * [patternIdx-1]) so nothing in the per-bar logic references the cursor.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLHIKKAKEMOD_Lookback();
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
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx - 3 ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)));
         i += 1;
      }
      patternCount = 0;
      patternResult = 0;
      patternHigh = 0.0;
      patternLow = 0.0;
      i = startIdx - 3;
      while( i < startIdx ) {
         /* copy here the pattern recognition code below */
         if( inHigh[i - 2] < inHigh[i - 3] &&
             inLow[i - 2] > inLow[i - 3] &&   /* 2nd: lower high and higher low than 1st */
             inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 3rd: lower high and higher low than 2nd */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] && inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1] && inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 4th: lower high and lower low (bull) 2nd: close near the low (bear) 4th: higher high and higher low (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = inHigh[i - 1];
            patternLow = inLow[i - 1];
            patternCount = 4;
         } else if( patternCount > 0 &&
             (patternResult > 0 && inClose[i] > patternHigh || patternResult < 0 && inClose[i] < patternLow) ) /* search for confirmation if modified hikkake was no more than 3 bars ago close higher than the high of 3rd close lower than the low of 3rd */
         {
            patternCount = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) - Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         if( patternCount > 0 ) {
            patternCount -= 1;
         }
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle
       * - second candle: candle with range less than first candle and close near the bottom (near the top)
       * - third candle: lower high and higher low than 2nd
       * - fourth candle: lower high and lower low (higher high and higher low) than 3rd
       * outInteger[hikkake bar] is positive (1 to 100) or negative (-1 to -100) meaning bullish or bearish hikkake
       * Confirmation could come in the next 3 days with:
       * - a day that closes higher than the high (lower than the low) of the 3rd candle
       * outInteger[confirmationbar] is equal to 100 + the bullish hikkake result or -100 - the bearish hikkake result
       * Note: if confirmation and a new hikkake come at the same bar, only the new hikkake is reported (the new hikkake
       * overwrites the confirmation of the old hikkake);
       * the user should consider that modified hikkake is a reversal pattern, while hikkake could be both a reversal
       * or a continuation pattern, so bullish (bearish) modified hikkake is significant when appearing in a downtrend
       * (uptrend)
       */
      outIdx = 0;
      do {
         if( inHigh[i - 2] < inHigh[i - 3] &&
             inLow[i - 2] > inLow[i - 3] &&   /* 2nd: lower high and higher low than 1st */
             inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 3rd: lower high and higher low than 2nd */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] && inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1] && inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 4th: lower high and lower low (bull) 2nd: close near the low (bear) 4th: higher high and higher low (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = inHigh[i - 1];
            patternLow = inLow[i - 1];
            patternCount = 4;
            outInteger[outIdx++ * outStride] = patternResult;
         } else if( patternCount > 0 &&
             (patternResult > 0 && inClose[i] > patternHigh || patternResult < 0 && inClose[i] < patternLow) ) /* search for confirmation if modified hikkake was no more than 3 bars ago close higher than the high of 3rd close lower than the low of 3rd */
         {
            outInteger[outIdx++ * outStride] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternCount = 0;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) - Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         if( patternCount > 0 ) {
            patternCount -= 1;
         }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
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
      sp.NearPeriodTotal = NearPeriodTotal;
      sp.patternResult = patternResult;
      sp.patternCount = patternCount;
      sp.patternHigh = patternHigh;
      sp.patternLow = patternLow;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag3_inHigh = inHigh[historyLen - 3];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag3_inLow = inLow[historyLen - 3];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_NearTrailingIdx = historyLen % cap_NearTrailingIdx;
      sp.ringCap_NearTrailingIdx = cap_NearTrailingIdx;
      sp.ringLag_NearTrailingIdx = capLag_NearTrailingIdx;
      sp.ring_NearTrailingIdx_inOpen = capRing_NearTrailingIdx_inOpen;
      sp.ring_NearTrailingIdx_inHigh = capRing_NearTrailingIdx_inHigh;
      sp.ring_NearTrailingIdx_inLow = capRing_NearTrailingIdx_inLow;
      sp.ring_NearTrailingIdx_inClose = capRing_NearTrailingIdx_inClose;
      sp.cs_Near_rangeType = Near_rangeType;
      sp.cs_Near_avgPeriod = Near_avgPeriod;
      sp.cs_Near_factor = Near_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CDLHIKKAKEMOD_OpenBody( CDLHIKKAKEMOD_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDLHIKKAKEMOD_OpenCore( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDLHIKKAKEMOD_OpenAndFillBody( CDLHIKKAKEMOD_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDLHIKKAKEMOD_OpenCore( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDLHIKKAKEMOD_OpenAndFillInternalBody( CDLHIKKAKEMOD_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDLHIKKAKEMOD_OpenCore(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDLHIKKAKEMOD_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLHIKKAKEMOD_Stream CDLHIKKAKEMOD_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLHIKKAKEMOD_Stream sp = new CDLHIKKAKEMOD_Stream(this);
      RetCode retCode = CDLHIKKAKEMOD_OpenAndFillInternalBody(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLHIKKAKEMOD openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLHIKKAKEMOD openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLHIKKAKEMOD openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind CDLHIKKAKEMOD_Open (composition seam). */
   CDLHIKKAKEMOD_Stream CDLHIKKAKEMOD_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLHIKKAKEMOD_Stream sp = new CDLHIKKAKEMOD_Stream(this);
      RetCode retCode = CDLHIKKAKEMOD_OpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLHIKKAKEMOD open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLHIKKAKEMOD open: internal error");
      }
      throw new IllegalArgumentException("CDLHIKKAKEMOD open: " + retCode);
   }
   /**
    * Open a live CDLHIKKAKEMOD stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLHIKKAKEMOD} at that bar.
    * <p>The history must hold at least {@code CDLHIKKAKEMOD_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLHIKKAKEMOD_Stream CDLHIKKAKEMOD_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLHIKKAKEMOD_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLHIKKAKEMOD_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLHIKKAKEMOD} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLHIKKAKEMOD_Stream#fillRange()}.
    */
   public CDLHIKKAKEMOD_Stream CDLHIKKAKEMOD_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLHIKKAKEMOD_Stream sp = new CDLHIKKAKEMOD_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIKKAKEMOD_OpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLHIKKAKEMOD openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLHIKKAKEMOD openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLHIKKAKEMOD openAndFill: " + retCode);
   }
