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
   RetCode CDLHIKKAKEMOD_Impl( int startIdx,
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
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
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
             (inHigh[i] < inHigh[i - 1] &&
               inLow[i] < inLow[i - 1] &&     /* (bull) 4th: lower high and lower low */
               inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
              inHigh[i] > inHigh[i - 1] &&
               inLow[i] > inLow[i - 1] &&     /* (bear) 4th: higher high and higher low */
               inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = inHigh[i - 1];
            patternLow = inLow[i - 1];
            patternCount = 4;
         } else if( patternCount > 0 &&
             (patternResult > 0 &&         /* search for confirmation if modified hikkake was no more than 3 bars ago */
               inClose[i] > patternHigh || /* close higher than the high of 3rd */
              patternResult < 0 &&
               inClose[i] < patternLow) )  /* close lower than the low of 3rd */
         {
            patternCount = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
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
             (inHigh[i] < inHigh[i - 1] &&
               inLow[i] < inLow[i - 1] &&     /* (bull) 4th: lower high and lower low */
               inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
              inHigh[i] > inHigh[i - 1] &&
               inLow[i] > inLow[i - 1] &&     /* (bear) 4th: higher high and higher low */
               inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = inHigh[i - 1];
            patternLow = inLow[i - 1];
            patternCount = 4;
            outInteger[outIdx++] = patternResult;
         } else if( patternCount > 0 &&
             (patternResult > 0 &&         /* search for confirmation if modified hikkake was no more than 3 bars ago */
               inClose[i] > patternHigh || /* close higher than the high of 3rd */
              patternResult < 0 &&
               inClose[i] < patternLow) )  /* close lower than the low of 3rd */
         {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternCount = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
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
   RetCode CDLHIKKAKEMOD_Impl( int startIdx,
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
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
         i += 1;
      }
      patternCount = 0;
      patternResult = 0;
      patternHigh = 0.0;
      patternLow = 0.0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] && (double)inClose[i - 2] <= (double)inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1] && (double)inClose[i - 2] >= (double)inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = (double)inHigh[i - 1];
            patternLow = (double)inLow[i - 1];
            patternCount = 4;
         } else if( patternCount > 0 && (patternResult > 0 && (double)inClose[i] > patternHigh || patternResult < 0 && (double)inClose[i] < patternLow) ) {
            patternCount = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 2] - ((((double)inClose[NearTrailingIdx - 2]) >= ((double)inOpen[NearTrailingIdx - 2])) ? ((double)inClose[NearTrailingIdx - 2]) : ((double)inOpen[NearTrailingIdx - 2]))) + (((((double)inClose[NearTrailingIdx - 2]) >= ((double)inOpen[NearTrailingIdx - 2])) ? ((double)inOpen[NearTrailingIdx - 2]) : ((double)inClose[NearTrailingIdx - 2])) - (double)inLow[NearTrailingIdx - 2])) : 0.0)));
         NearTrailingIdx += 1;
         if( patternCount > 0 ) {
            patternCount -= 1;
         }
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (double)inHigh[i - 2] < (double)inHigh[i - 3] && (double)inLow[i - 2] > (double)inLow[i - 3] && (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] && (double)inClose[i - 2] <= (double)inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1] && (double)inClose[i - 2] >= (double)inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) {
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
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs((double)inClose[NearTrailingIdx - 2] - (double)inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? ((double)inHigh[NearTrailingIdx - 2] - (double)inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? (((double)inHigh[NearTrailingIdx - 2] - ((((double)inClose[NearTrailingIdx - 2]) >= ((double)inOpen[NearTrailingIdx - 2])) ? ((double)inClose[NearTrailingIdx - 2]) : ((double)inOpen[NearTrailingIdx - 2]))) + (((((double)inClose[NearTrailingIdx - 2]) >= ((double)inOpen[NearTrailingIdx - 2])) ? ((double)inOpen[NearTrailingIdx - 2]) : ((double)inClose[NearTrailingIdx - 2])) - (double)inLow[NearTrailingIdx - 2])) : 0.0)));
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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      requireIndexRange("CDLHIKKAKEMOD", startIdx, endIdx);
      int guardStart = clampedStart("CDLHIKKAKEMOD", startIdx, CDLHIKKAKEMOD_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLHIKKAKEMOD", "inOpen", inOpen, guardInLen);
      requireLength("CDLHIKKAKEMOD", "inHigh", inHigh, guardInLen);
      requireLength("CDLHIKKAKEMOD", "inLow", inLow, guardInLen);
      requireLength("CDLHIKKAKEMOD", "inClose", inClose, guardInLen);
      requireLength("CDLHIKKAKEMOD", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIKKAKEMOD_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      requireIndexRange("CDLHIKKAKEMOD", startIdx, endIdx);
      int guardStart = clampedStart("CDLHIKKAKEMOD", startIdx, CDLHIKKAKEMOD_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLHIKKAKEMOD", "inOpen", inOpen, guardInLen);
      requireLength("CDLHIKKAKEMOD", "inHigh", inHigh, guardInLen);
      requireLength("CDLHIKKAKEMOD", "inLow", inLow, guardInLen);
      requireLength("CDLHIKKAKEMOD", "inClose", inClose, guardInLen);
      requireLength("CDLHIKKAKEMOD", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIKKAKEMOD_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHIKKAKEMOD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLHIKKAKEMOD stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLHIKKAKEMOD} over the same series.
    * Open with {@link Core#cdlhikkakemodOpen}; there is no close — the handle is
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
   public static final class CdlhikkakemodStream {
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
      double[] ring_NearTrailingIdx_derived;
      int cs_Near_rangeType;
      int cs_Near_avgPeriod;
      double cs_Near_factor;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      CdlhikkakemodStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLHIKKAKEMOD} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CdlhikkakemodStream( CdlhikkakemodStream other ) {
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
            throw new TaLibArgumentException("CDLHIKKAKEMOD update: BadParam", RetCode.BadParam);
         }
         core.cdlhikkakemodStepImpl(this, inOpen, inHigh, inLow, inClose);
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
         requireArgument("CDLHIKKAKEMOD updateAndFill", "inOpen", inOpen);
         requireArgument("CDLHIKKAKEMOD updateAndFill", "inHigh", inHigh);
         requireArgument("CDLHIKKAKEMOD updateAndFill", "inLow", inLow);
         requireArgument("CDLHIKKAKEMOD updateAndFill", "inClose", inClose);
         requireArgument("CDLHIKKAKEMOD updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLHIKKAKEMOD updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CDLHIKKAKEMOD updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cdlhikkakemodStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
            throw new TaLibArgumentException("CDLHIKKAKEMOD peek: BadParam", RetCode.BadParam);
         CdlhikkakemodStream sp = this;
         int cur_outInteger = 0;
         int patternCount = sp.patternCount;
         double patternHigh = sp.patternHigh;
         double patternLow = sp.patternLow;
         int patternResult = sp.patternResult;
         int Near_rangeType = sp.cs_Near_rangeType;
         int Near_avgPeriod = sp.cs_Near_avgPeriod;
         double Near_factor = sp.cs_Near_factor;
         if( sp.lag2_inHigh < sp.lag3_inHigh &&
             sp.lag2_inLow > sp.lag3_inLow &&   /* 2nd: lower high and higher low than 1st */
             sp.lag1_inHigh < sp.lag2_inHigh &&
             sp.lag1_inLow > sp.lag2_inLow &&   /* 3rd: lower high and higher low than 2nd */
             (inHigh < sp.lag1_inHigh &&
               inLow < sp.lag1_inLow &&         /* (bull) 4th: lower high and lower low */
               sp.lag2_inClose <= sp.lag2_inLow + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
              inHigh > sp.lag1_inHigh &&
               inLow > sp.lag1_inLow &&         /* (bear) 4th: higher high and higher low */
               sp.lag2_inClose >= sp.lag2_inHigh - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh < sp.lag1_inHigh) ? 1 : 0 - 1);
            patternHigh = sp.lag1_inHigh;
            patternLow = sp.lag1_inLow;
            patternCount = 4;
            cur_outInteger = patternResult;
         } else if( patternCount > 0 &&
             (patternResult > 0 &&      /* search for confirmation if modified hikkake was no more than 3 bars ago */
               inClose > patternHigh || /* close higher than the high of 3rd */
              patternResult < 0 &&
               inClose < patternLow) )  /* close lower than the low of 3rd */
         {
            cur_outInteger = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternCount = 0;
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
      public CdlhikkakemodStream clone() {
         return new CdlhikkakemodStream(this);
      }
   }
   void cdlhikkakemodStepImpl( CdlhikkakemodStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int Near_rangeType = sp.cs_Near_rangeType;
      int Near_avgPeriod = sp.cs_Near_avgPeriod;
      double Near_factor = sp.cs_Near_factor;
      sp.ring_NearTrailingIdx_derived[sp.ringPos_NearTrailingIdx] = ((Near_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Near_rangeType == 1) ? (inHigh - inLow) : ((Near_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( sp.lag2_inHigh < sp.lag3_inHigh &&
          sp.lag2_inLow > sp.lag3_inLow &&   /* 2nd: lower high and higher low than 1st */
          sp.lag1_inHigh < sp.lag2_inHigh &&
          sp.lag1_inLow > sp.lag2_inLow &&   /* 3rd: lower high and higher low than 2nd */
          (inHigh < sp.lag1_inHigh &&
            inLow < sp.lag1_inLow &&         /* (bull) 4th: lower high and lower low */
            sp.lag2_inClose <= sp.lag2_inLow + ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
           inHigh > sp.lag1_inHigh &&
            inLow > sp.lag1_inLow &&         /* (bear) 4th: higher high and higher low */
            sp.lag2_inClose >= sp.lag2_inHigh - ((Near_factor * (((Near_avgPeriod != 0) ? (sp.NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 2nd: close near the top */
      {
         sp.patternResult = 100 * ((inHigh < sp.lag1_inHigh) ? 1 : 0 - 1);
         sp.patternHigh = sp.lag1_inHigh;
         sp.patternLow = sp.lag1_inLow;
         sp.patternCount = 4;
         sp.cur_outInteger = sp.patternResult;
      } else if( sp.patternCount > 0 &&
          (sp.patternResult > 0 &&      /* search for confirmation if modified hikkake was no more than 3 bars ago */
            inClose > sp.patternHigh || /* close higher than the high of 3rd */
           sp.patternResult < 0 &&
            inClose < sp.patternLow) )  /* close lower than the low of 3rd */
      {
         sp.cur_outInteger = sp.patternResult + 100 * ((sp.patternResult > 0) ? 1 : 0 - 1);
         sp.patternCount = 0;
      } else {
         sp.cur_outInteger = 0;
      }
      sp.NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Near_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Near_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0))) - sp.ring_NearTrailingIdx_derived[(sp.ringPos_NearTrailingIdx + sp.ringCap_NearTrailingIdx - sp.ringLag_NearTrailingIdx - 2) % sp.ringCap_NearTrailingIdx];
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
      sp.ringPos_NearTrailingIdx = sp.ringPos_NearTrailingIdx + 1;
      if( sp.ringPos_NearTrailingIdx >= sp.ringCap_NearTrailingIdx ) {
         sp.ringPos_NearTrailingIdx = 0;
      }
   }
   private RetCode cdlhikkakemodOpenImpl( CdlhikkakemodStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
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
         return RetCode.InsufficientHistory ;
      }
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      NearPeriodTotal = 0;
      NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
      i = NearTrailingIdx;
      while( i < startIdx - 3 ) {
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
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
             (inHigh[i] < inHigh[i - 1] &&
               inLow[i] < inLow[i - 1] &&     /* (bull) 4th: lower high and lower low */
               inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
              inHigh[i] > inHigh[i - 1] &&
               inLow[i] > inLow[i - 1] &&     /* (bear) 4th: higher high and higher low */
               inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = inHigh[i - 1];
            patternLow = inLow[i - 1];
            patternCount = 4;
         } else if( patternCount > 0 &&
             (patternResult > 0 &&         /* search for confirmation if modified hikkake was no more than 3 bars ago */
               inClose[i] > patternHigh || /* close higher than the high of 3rd */
              patternResult < 0 &&
               inClose[i] < patternLow) )  /* close lower than the low of 3rd */
         {
            patternCount = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
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
             (inHigh[i] < inHigh[i - 1] &&
               inLow[i] < inLow[i - 1] &&     /* (bull) 4th: lower high and lower low */
               inClose[i - 2] <= inLow[i - 2] + ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0)))) || /* (bull) 2nd: close near the low */
              inHigh[i] > inHigh[i - 1] &&
               inLow[i] > inLow[i - 1] &&     /* (bear) 4th: higher high and higher low */
               inClose[i - 2] >= inHigh[i - 2] - ((Near_factor * (((Near_avgPeriod != 0) ? (NearPeriodTotal / Near_avgPeriod) : ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Near_rangeType == 2) ? 2.0 : 1.0))))) ) /* (bull) 2nd: close near the top */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternHigh = inHigh[i - 1];
            patternLow = inLow[i - 1];
            patternCount = 4;
            outInteger[outIdx++ * outStride] = patternResult;
         } else if( patternCount > 0 &&
             (patternResult > 0 &&         /* search for confirmation if modified hikkake was no more than 3 bars ago */
               inClose[i] > patternHigh || /* close higher than the high of 3rd */
              patternResult < 0 &&
               inClose[i] < patternLow) )  /* close lower than the low of 3rd */
         {
            outInteger[outIdx++ * outStride] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternCount = 0;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         NearPeriodTotal += ((Near_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Near_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Near_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Near_rangeType == 0) ? (Math.abs(inClose[NearTrailingIdx - 2] - inOpen[NearTrailingIdx - 2])) : ((Near_rangeType == 1) ? (inHigh[NearTrailingIdx - 2] - inLow[NearTrailingIdx - 2]) : ((Near_rangeType == 2) ? ((inHigh[NearTrailingIdx - 2] - (((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inClose[NearTrailingIdx - 2]) : (inOpen[NearTrailingIdx - 2]))) + ((((inClose[NearTrailingIdx - 2]) >= (inOpen[NearTrailingIdx - 2])) ? (inOpen[NearTrailingIdx - 2]) : (inClose[NearTrailingIdx - 2])) - inLow[NearTrailingIdx - 2])) : 0.0)));
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
      double[] capRing_NearTrailingIdx_derived = new double[allocN_NearTrailingIdx];
      for( int fillJ = historyLen - cap_NearTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_NearTrailingIdx_derived[fillJ % cap_NearTrailingIdx] = ((Near_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Near_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Near_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
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
      sp.ring_NearTrailingIdx_derived = capRing_NearTrailingIdx_derived;
      sp.cs_Near_rangeType = Near_rangeType;
      sp.cs_Near_avgPeriod = Near_avgPeriod;
      sp.cs_Near_factor = Near_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cdlhikkakemodOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CdlhikkakemodStream cdlhikkakemodOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlhikkakemodStream sp = new CdlhikkakemodStream(this);
      RetCode retCode = cdlhikkakemodOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLHIKKAKEMOD openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLHIKKAKEMOD openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLHIKKAKEMOD openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cdlhikkakemodOpen (composition seam). */
   CdlhikkakemodStream cdlhikkakemodOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlhikkakemodStream sp = new CdlhikkakemodStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdlhikkakemodOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLHIKKAKEMOD open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLHIKKAKEMOD open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLHIKKAKEMOD open: " + retCode, retCode);
   }
   /**
    * Open a live CDLHIKKAKEMOD stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLHIKKAKEMOD} at that bar.
    * <p>The history must hold at least {@code CDLHIKKAKEMOD_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CdlhikkakemodStream cdlhikkakemodOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLHIKKAKEMOD open", "inOpen", inOpen);
      requireHistory("CDLHIKKAKEMOD open", inOpen.length);
      requireArgument("CDLHIKKAKEMOD open", "inHigh", inHigh);
      requireArgument("CDLHIKKAKEMOD open", "inLow", inLow);
      requireArgument("CDLHIKKAKEMOD open", "inClose", inClose);
      requireHistoryLength("CDLHIKKAKEMOD open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLHIKKAKEMOD open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLHIKKAKEMOD open", "inClose", inClose.length, inOpen.length);
      return cdlhikkakemodOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlhikkakemodOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CDLHIKKAKEMOD} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CdlhikkakemodStream#outRange()}.
    */
   public CdlhikkakemodStream cdlhikkakemodOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLHIKKAKEMOD openAndFill", "inOpen", inOpen);
      requireHistory("CDLHIKKAKEMOD openAndFill", inOpen.length);
      requireArgument("CDLHIKKAKEMOD openAndFill", "inHigh", inHigh);
      requireArgument("CDLHIKKAKEMOD openAndFill", "inLow", inLow);
      requireArgument("CDLHIKKAKEMOD openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLHIKKAKEMOD openAndFill", inOpen.length, CDLHIKKAKEMOD_Lookback());
      requireHistoryLength("CDLHIKKAKEMOD openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLHIKKAKEMOD openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLHIKKAKEMOD openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLHIKKAKEMOD openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLHIKKAKEMOD openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdlhikkakemodOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
