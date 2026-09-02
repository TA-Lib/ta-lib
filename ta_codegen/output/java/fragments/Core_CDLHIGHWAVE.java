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
 *  072404 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLHIGHWAVE} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLHIGHWAVE_Lookback( )
   {
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      return Math.max(BodyShort_avgPeriod, ShadowVeryLong_avgPeriod) ;

   }
   RetCode CDLHIGHWAVE_Impl( int startIdx,
                             int endIdx,
                             double inOpen[],
                             double inHigh[],
                             double inLow[],
                             double inClose[],
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLHIGHWAVE_Lookback();
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
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - short real body
       * - very long upper and lower shadow
       * The meaning of "short" and "very long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when white or negative (-1 to -100) when black;
       * it does not mean bullish or bearish
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[ShadowTrailingIdx] - (((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inClose[ShadowTrailingIdx]) : (inOpen[ShadowTrailingIdx]))) + ((((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inOpen[ShadowTrailingIdx]) : (inClose[ShadowTrailingIdx])) - inLow[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLHIGHWAVE_Impl( int startIdx,
                             int endIdx,
                             float inOpen[],
                             float inHigh[],
                             float inLow[],
                             float inClose[],
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLHIGHWAVE_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i] - (double)inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? ((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? (((double)inHigh[BodyTrailingIdx] - ((((double)inClose[BodyTrailingIdx]) >= ((double)inOpen[BodyTrailingIdx])) ? ((double)inClose[BodyTrailingIdx]) : ((double)inOpen[BodyTrailingIdx]))) + (((((double)inClose[BodyTrailingIdx]) >= ((double)inOpen[BodyTrailingIdx])) ? ((double)inOpen[BodyTrailingIdx]) : ((double)inClose[BodyTrailingIdx])) - (double)inLow[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs((double)inClose[ShadowTrailingIdx] - (double)inOpen[ShadowTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? ((double)inHigh[ShadowTrailingIdx] - (double)inLow[ShadowTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? (((double)inHigh[ShadowTrailingIdx] - ((((double)inClose[ShadowTrailingIdx]) >= ((double)inOpen[ShadowTrailingIdx])) ? ((double)inClose[ShadowTrailingIdx]) : ((double)inOpen[ShadowTrailingIdx]))) + (((((double)inClose[ShadowTrailingIdx]) >= ((double)inOpen[ShadowTrailingIdx])) ? ((double)inOpen[ShadowTrailingIdx]) : ((double)inClose[ShadowTrailingIdx])) - (double)inLow[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Single-candle pattern: a short real body with both a very long upper and a
    * very long lower shadow. Signals market indecision; the output sign reports
    * only candle color, not a bullish/bearish direction. A hit marks indecision
    * (long-legged candle); not directional - sign encodes only the candle's
    * color.
    * <p><b>Formula</b>
    * <pre>{@code
    * One candle at index i. Hit when all hold: (1) short real body: real body < the BodyShort average; (2) very long upper shadow: upper shadow > the ShadowVeryLong average; (3) very long lower shadow: lower shadow > the ShadowVeryLong average. No color, gap, or trend condition.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Bulkowski's testing found the High-Wave candle acts as a reversal only 51% of the time — statistically indistinguishable from random — which he notes actually agrees with the pattern's theoretical meaning of pure indecision. ([thepatternsite.com](https://thepatternsite.com/HighWave.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHIGHWAVE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger On a hit, +100 when the candle is white (close &gt;=
    *        open) or -100 when black (close &lt; open); 0 otherwise. Sign denotes
    *        color, NOT bull/bear. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
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
    * @see Core#CDLLONGLEGGEDDOJI
    * @see Core#CDLSPINNINGTOP
    * @see Core#CDLRICKSHAWMAN
    * @see Core#CDLDOJI
    */
   public OutRange CDLHIGHWAVE( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                int outInteger[] )
   {
      requireIndexRange("CDLHIGHWAVE", startIdx, endIdx);
      int guardStart = clampedStart("CDLHIGHWAVE", startIdx, CDLHIGHWAVE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLHIGHWAVE", "inOpen", inOpen, guardInLen);
      requireLength("CDLHIGHWAVE", "inHigh", inHigh, guardInLen);
      requireLength("CDLHIGHWAVE", "inLow", inLow, guardInLen);
      requireLength("CDLHIGHWAVE", "inClose", inClose, guardInLen);
      requireLength("CDLHIGHWAVE", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIGHWAVE_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHIGHWAVE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Single-candle pattern: a short real body with both a very long upper and a
    * very long lower shadow. Signals market indecision; the output sign reports
    * only candle color, not a bullish/bearish direction. A hit marks indecision
    * (long-legged candle); not directional - sign encodes only the candle's
    * color.
    * <p><b>Formula</b>
    * <pre>{@code
    * One candle at index i. Hit when all hold: (1) short real body: real body < the BodyShort average; (2) very long upper shadow: upper shadow > the ShadowVeryLong average; (3) very long lower shadow: lower shadow > the ShadowVeryLong average. No color, gap, or trend condition.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Bulkowski's testing found the High-Wave candle acts as a reversal only 51% of the time — statistically indistinguishable from random — which he notes actually agrees with the pattern's theoretical meaning of pure indecision. ([thepatternsite.com](https://thepatternsite.com/HighWave.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHIGHWAVE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger On a hit, +100 when the candle is white (close &gt;=
    *        open) or -100 when black (close &lt; open); 0 otherwise. Sign denotes
    *        color, NOT bull/bear. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
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
    * @see Core#CDLLONGLEGGEDDOJI
    * @see Core#CDLSPINNINGTOP
    * @see Core#CDLRICKSHAWMAN
    * @see Core#CDLDOJI
    */
   public OutRange CDLHIGHWAVE( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                int outInteger[] )
   {
      requireIndexRange("CDLHIGHWAVE", startIdx, endIdx);
      int guardStart = clampedStart("CDLHIGHWAVE", startIdx, CDLHIGHWAVE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLHIGHWAVE", "inOpen", inOpen, guardInLen);
      requireLength("CDLHIGHWAVE", "inHigh", inHigh, guardInLen);
      requireLength("CDLHIGHWAVE", "inLow", inLow, guardInLen);
      requireLength("CDLHIGHWAVE", "inClose", inClose, guardInLen);
      requireLength("CDLHIGHWAVE", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIGHWAVE_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHIGHWAVE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLHIGHWAVE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLHIGHWAVE} over the same series.
    * Open with {@link Core#cdlhighwaveOpen}; there is no close — the handle is
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
   public static final class CdlhighwaveStream {
      Core core;
      double BodyPeriodTotal;
      double ShadowPeriodTotal;
      int ringPos_BodyTrailingIdx;
      int ringCap_BodyTrailingIdx;
      double[] ring_BodyTrailingIdx_derived;
      int ringPos_ShadowTrailingIdx;
      int ringCap_ShadowTrailingIdx;
      double[] ring_ShadowTrailingIdx_derived;
      int cs_BodyShort_rangeType;
      int cs_BodyShort_avgPeriod;
      double cs_BodyShort_factor;
      int cs_ShadowVeryLong_rangeType;
      int cs_ShadowVeryLong_avgPeriod;
      double cs_ShadowVeryLong_factor;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      CdlhighwaveStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLHIGHWAVE} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CdlhighwaveStream( CdlhighwaveStream other ) {
         this.core = other.core;
         this.BodyPeriodTotal = other.BodyPeriodTotal;
         this.ShadowPeriodTotal = other.ShadowPeriodTotal;
         this.ringPos_BodyTrailingIdx = other.ringPos_BodyTrailingIdx;
         this.ringCap_BodyTrailingIdx = other.ringCap_BodyTrailingIdx;
         this.ring_BodyTrailingIdx_derived = other.ring_BodyTrailingIdx_derived.clone();
         this.ringPos_ShadowTrailingIdx = other.ringPos_ShadowTrailingIdx;
         this.ringCap_ShadowTrailingIdx = other.ringCap_ShadowTrailingIdx;
         this.ring_ShadowTrailingIdx_derived = other.ring_ShadowTrailingIdx_derived.clone();
         this.cs_BodyShort_rangeType = other.cs_BodyShort_rangeType;
         this.cs_BodyShort_avgPeriod = other.cs_BodyShort_avgPeriod;
         this.cs_BodyShort_factor = other.cs_BodyShort_factor;
         this.cs_ShadowVeryLong_rangeType = other.cs_ShadowVeryLong_rangeType;
         this.cs_ShadowVeryLong_avgPeriod = other.cs_ShadowVeryLong_avgPeriod;
         this.cs_ShadowVeryLong_factor = other.cs_ShadowVeryLong_factor;
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
            throw new TaLibArgumentException("CDLHIGHWAVE update: BadParam", RetCode.BadParam);
         }
         core.cdlhighwaveStepImpl(this, inOpen, inHigh, inLow, inClose);
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
         requireArgument("CDLHIGHWAVE updateAndFill", "inOpen", inOpen);
         requireArgument("CDLHIGHWAVE updateAndFill", "inHigh", inHigh);
         requireArgument("CDLHIGHWAVE updateAndFill", "inLow", inLow);
         requireArgument("CDLHIGHWAVE updateAndFill", "inClose", inClose);
         requireArgument("CDLHIGHWAVE updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLHIGHWAVE updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CDLHIGHWAVE updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cdlhighwaveStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
            throw new TaLibArgumentException("CDLHIGHWAVE peek: BadParam", RetCode.BadParam);
         CdlhighwaveStream sp = this;
         int cur_outInteger = sp.cur_outInteger;
         int BodyShort_rangeType = sp.cs_BodyShort_rangeType;
         int BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
         double BodyShort_factor = sp.cs_BodyShort_factor;
         int ShadowVeryLong_rangeType = sp.cs_ShadowVeryLong_rangeType;
         int ShadowVeryLong_avgPeriod = sp.cs_ShadowVeryLong_avgPeriod;
         double ShadowVeryLong_factor = sp.cs_ShadowVeryLong_factor;
         if( Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (sp.ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (sp.ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
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
      public CdlhighwaveStream clone() {
         return new CdlhighwaveStream(this);
      }
   }
   void cdlhighwaveStepImpl( CdlhighwaveStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyShort_rangeType = sp.cs_BodyShort_rangeType;
      int BodyShort_avgPeriod = sp.cs_BodyShort_avgPeriod;
      double BodyShort_factor = sp.cs_BodyShort_factor;
      int ShadowVeryLong_rangeType = sp.cs_ShadowVeryLong_rangeType;
      int ShadowVeryLong_avgPeriod = sp.cs_ShadowVeryLong_avgPeriod;
      double ShadowVeryLong_factor = sp.cs_ShadowVeryLong_factor;
      if( sp.ringCap_BodyTrailingIdx == 0 ) {
         sp.ring_BodyTrailingIdx_derived[0] = ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      }
      if( sp.ringCap_ShadowTrailingIdx == 0 ) {
         sp.ring_ShadowTrailingIdx_derived[0] = ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      }
      if( Math.abs(inClose - inOpen) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (sp.BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (sp.ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (sp.ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
         sp.cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_BodyTrailingIdx_derived[sp.ringPos_BodyTrailingIdx];
      sp.ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0))) - sp.ring_ShadowTrailingIdx_derived[sp.ringPos_ShadowTrailingIdx];
      sp.ring_BodyTrailingIdx_derived[sp.ringPos_BodyTrailingIdx] = ((BodyShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyShort_rangeType == 1) ? (inHigh - inLow) : ((BodyShort_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ringPos_BodyTrailingIdx = sp.ringPos_BodyTrailingIdx + 1;
      if( sp.ringPos_BodyTrailingIdx >= sp.ringCap_BodyTrailingIdx ) {
         sp.ringPos_BodyTrailingIdx = 0;
      }
      sp.ring_ShadowTrailingIdx_derived[sp.ringPos_ShadowTrailingIdx] = ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowVeryLong_rangeType == 1) ? (inHigh - inLow) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      sp.ringPos_ShadowTrailingIdx = sp.ringPos_ShadowTrailingIdx + 1;
      if( sp.ringPos_ShadowTrailingIdx >= sp.ringCap_ShadowTrailingIdx ) {
         sp.ringPos_ShadowTrailingIdx = 0;
      }
   }
   private RetCode cdlhighwaveOpenImpl( CdlhighwaveStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double BodyPeriodTotal = 0;
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
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
      int BodyShort_rangeType = this.candleSettings[CandleSettingType.BodyShort.ordinal()].rangeType.ordinal();
      int BodyShort_avgPeriod = this.candleSettings[CandleSettingType.BodyShort.ordinal()].avgPeriod;
      double BodyShort_factor = this.candleSettings[CandleSettingType.BodyShort.ordinal()].factor;
      int ShadowVeryLong_rangeType = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].rangeType.ordinal();
      int ShadowVeryLong_avgPeriod = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].avgPeriod;
      double ShadowVeryLong_factor = this.candleSettings[CandleSettingType.ShadowVeryLong.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLHIGHWAVE_Lookback();
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
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyShort_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowVeryLong_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - short real body
       * - very long upper and lower shadow
       * The meaning of "short" and "very long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when white or negative (-1 to -100) when black;
       * it does not mean bullish or bearish
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) < ((BodyShort_factor * (((BodyShort_avgPeriod != 0) ? (BodyPeriodTotal / BodyShort_avgPeriod) : ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyShort_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) > ((ShadowVeryLong_factor * (((ShadowVeryLong_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowVeryLong_avgPeriod) : ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((ShadowVeryLong_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++ * outStride] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyShort_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((BodyShort_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyShort_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyShort_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - (((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inClose[BodyTrailingIdx]) : (inOpen[BodyTrailingIdx]))) + ((((inClose[BodyTrailingIdx]) >= (inOpen[BodyTrailingIdx])) ? (inOpen[BodyTrailingIdx]) : (inClose[BodyTrailingIdx])) - inLow[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0))) - ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[ShadowTrailingIdx] - (((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inClose[ShadowTrailingIdx]) : (inOpen[ShadowTrailingIdx]))) + ((((inClose[ShadowTrailingIdx]) >= (inOpen[ShadowTrailingIdx])) ? (inOpen[ShadowTrailingIdx]) : (inClose[ShadowTrailingIdx])) - inLow[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_BodyTrailingIdx = i - BodyTrailingIdx;
      if( cap_BodyTrailingIdx < 0 || cap_BodyTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyTrailingIdx = (cap_BodyTrailingIdx > 0)? cap_BodyTrailingIdx : 1;
      double[] capRing_BodyTrailingIdx_derived = new double[allocN_BodyTrailingIdx];
      for( int fillJ = historyLen - cap_BodyTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_BodyTrailingIdx_derived[fillJ - (historyLen - cap_BodyTrailingIdx)] = ((BodyShort_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((BodyShort_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((BodyShort_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      int cap_ShadowTrailingIdx = i - ShadowTrailingIdx;
      if( cap_ShadowTrailingIdx < 0 || cap_ShadowTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_ShadowTrailingIdx = (cap_ShadowTrailingIdx > 0)? cap_ShadowTrailingIdx : 1;
      double[] capRing_ShadowTrailingIdx_derived = new double[allocN_ShadowTrailingIdx];
      for( int fillJ = historyLen - cap_ShadowTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_ShadowTrailingIdx_derived[fillJ - (historyLen - cap_ShadowTrailingIdx)] = ((ShadowVeryLong_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((ShadowVeryLong_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((ShadowVeryLong_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.BodyPeriodTotal = BodyPeriodTotal;
      sp.ShadowPeriodTotal = ShadowPeriodTotal;
      sp.ringPos_BodyTrailingIdx = 0;
      sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
      sp.ring_BodyTrailingIdx_derived = capRing_BodyTrailingIdx_derived;
      sp.ringPos_ShadowTrailingIdx = 0;
      sp.ringCap_ShadowTrailingIdx = cap_ShadowTrailingIdx;
      sp.ring_ShadowTrailingIdx_derived = capRing_ShadowTrailingIdx_derived;
      sp.cs_BodyShort_rangeType = BodyShort_rangeType;
      sp.cs_BodyShort_avgPeriod = BodyShort_avgPeriod;
      sp.cs_BodyShort_factor = BodyShort_factor;
      sp.cs_ShadowVeryLong_rangeType = ShadowVeryLong_rangeType;
      sp.cs_ShadowVeryLong_avgPeriod = ShadowVeryLong_avgPeriod;
      sp.cs_ShadowVeryLong_factor = ShadowVeryLong_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cdlhighwaveOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CdlhighwaveStream cdlhighwaveOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlhighwaveStream sp = new CdlhighwaveStream(this);
      RetCode retCode = cdlhighwaveOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLHIGHWAVE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLHIGHWAVE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLHIGHWAVE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cdlhighwaveOpen (composition seam). */
   CdlhighwaveStream cdlhighwaveOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlhighwaveStream sp = new CdlhighwaveStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdlhighwaveOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLHIGHWAVE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLHIGHWAVE open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLHIGHWAVE open: " + retCode, retCode);
   }
   /**
    * Open a live CDLHIGHWAVE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLHIGHWAVE} at that bar.
    * <p>The history must hold at least {@code CDLHIGHWAVE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CdlhighwaveStream cdlhighwaveOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLHIGHWAVE open", "inOpen", inOpen);
      requireHistory("CDLHIGHWAVE open", inOpen.length);
      requireArgument("CDLHIGHWAVE open", "inHigh", inHigh);
      requireArgument("CDLHIGHWAVE open", "inLow", inLow);
      requireArgument("CDLHIGHWAVE open", "inClose", inClose);
      requireHistoryLength("CDLHIGHWAVE open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLHIGHWAVE open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLHIGHWAVE open", "inClose", inClose.length, inOpen.length);
      return cdlhighwaveOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlhighwaveOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CDLHIGHWAVE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CdlhighwaveStream#outRange()}.
    */
   public CdlhighwaveStream cdlhighwaveOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLHIGHWAVE openAndFill", "inOpen", inOpen);
      requireHistory("CDLHIGHWAVE openAndFill", inOpen.length);
      requireArgument("CDLHIGHWAVE openAndFill", "inHigh", inHigh);
      requireArgument("CDLHIGHWAVE openAndFill", "inLow", inLow);
      requireArgument("CDLHIGHWAVE openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLHIGHWAVE openAndFill", inOpen.length, CDLHIGHWAVE_Lookback());
      requireHistoryLength("CDLHIGHWAVE openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLHIGHWAVE openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLHIGHWAVE openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLHIGHWAVE openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLHIGHWAVE openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdlhighwaveOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
