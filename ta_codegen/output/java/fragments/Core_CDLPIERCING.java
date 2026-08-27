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
 *  120904 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLPIERCING} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLPIERCING_Lookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      return BodyLong_avgPeriod + 1 ;

   }
   RetCode CDLPIERCING_Impl( int startIdx,
                             int endIdx,
                             double inOpen[],
                             double inHigh[],
                             double inLow[],
                             double inClose[],
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             int outInteger[] )
   {
      double[] BodyLongPeriodTotal = new double[2];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
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
      lookbackTotal = CDLPIERCING_Lookback();
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
      BodyLongPeriodTotal[1] = 0;
      BodyLongPeriodTotal[0] = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         BodyLongPeriodTotal[0] = BodyLongPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black candle
       * - second candle: long white candle with open below previous day low and close at least at 50% of previous day
       * real body
       * The meaning of "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100): piercing pattern is always bullish
       * the user should consider that a piercing pattern is significant when it appears in a downtrend, while
       * this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 1st: black */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&             /* 2nd: white */
             Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long */
             inOpen[i] < inLow[i - 1] &&                                 /* open below prior low */
             inClose[i] < inOpen[i - 1] &&                               /* close within prior body */
             inClose[i] > Math.fma(Math.abs(inClose[i - 1] - inOpen[i - 1]), 0.5, inClose[i - 1]) ) /* above midpoint */
         {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            BodyLongPeriodTotal[totIdx] = BodyLongPeriodTotal[totIdx] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - totIdx] - inOpen[BodyLongTrailingIdx - totIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - totIdx] - inLow[BodyLongTrailingIdx - totIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - totIdx] - (((inClose[BodyLongTrailingIdx - totIdx]) >= (inOpen[BodyLongTrailingIdx - totIdx])) ? (inClose[BodyLongTrailingIdx - totIdx]) : (inOpen[BodyLongTrailingIdx - totIdx]))) + ((((inClose[BodyLongTrailingIdx - totIdx]) >= (inOpen[BodyLongTrailingIdx - totIdx])) ? (inOpen[BodyLongTrailingIdx - totIdx]) : (inClose[BodyLongTrailingIdx - totIdx])) - inLow[BodyLongTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLPIERCING_Impl( int startIdx,
                             int endIdx,
                             float inOpen[],
                             float inHigh[],
                             float inLow[],
                             float inClose[],
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             int outInteger[] )
   {
      double[] BodyLongPeriodTotal = new double[2];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
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
      lookbackTotal = CDLPIERCING_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal[1] = 0;
      BodyLongPeriodTotal[0] = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         BodyLongPeriodTotal[0] = BodyLongPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && Math.abs((double)inClose[i] - (double)inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - ((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inClose[i]) : ((double)inOpen[i]))) + (((((double)inClose[i]) >= ((double)inOpen[i])) ? ((double)inOpen[i]) : ((double)inClose[i])) - (double)inLow[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (double)inOpen[i] < (double)inLow[i - 1] && (double)inClose[i] < (double)inOpen[i - 1] && (double)inClose[i] > Math.fma(Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]), 0.5, (double)inClose[i - 1]) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            BodyLongPeriodTotal[totIdx] = BodyLongPeriodTotal[totIdx] + (((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - totIdx] - (double)inOpen[i - totIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - totIdx] - (double)inLow[i - totIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - totIdx] - ((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inClose[i - totIdx]) : ((double)inOpen[i - totIdx]))) + (((((double)inClose[i - totIdx]) >= ((double)inOpen[i - totIdx])) ? ((double)inOpen[i - totIdx]) : ((double)inClose[i - totIdx])) - (double)inLow[i - totIdx])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx - totIdx] - (double)inOpen[BodyLongTrailingIdx - totIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx - totIdx] - (double)inLow[BodyLongTrailingIdx - totIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx - totIdx] - ((((double)inClose[BodyLongTrailingIdx - totIdx]) >= ((double)inOpen[BodyLongTrailingIdx - totIdx])) ? ((double)inClose[BodyLongTrailingIdx - totIdx]) : ((double)inOpen[BodyLongTrailingIdx - totIdx]))) + (((((double)inClose[BodyLongTrailingIdx - totIdx]) >= ((double)inOpen[BodyLongTrailingIdx - totIdx])) ? ((double)inOpen[BodyLongTrailingIdx - totIdx]) : ((double)inClose[BodyLongTrailingIdx - totIdx])) - (double)inLow[BodyLongTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Two-candle pattern: a long black candle followed by a long white candle
    * that opens below the prior low and closes back above the midpoint of the
    * prior black body. Bullish reversal signal. A hit (+100) is a bullish
    * reversal signal.
    * <p><b>Notes</b>
    * <ul>
    * <li>A prior downtrend is not verified.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLPIERCING_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the piercing pattern is detected; 0 otherwise.
    *        Always bullish, never emits -100. Must hold at least
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
    * @see Core#CDLDARKCLOUDCOVER
    * @see Core#CDLENGULFING
    * @see Core#CDLMORNINGSTAR
    */
   public OutRange CDLPIERCING( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                int outInteger[] )
   {
      requireIndexRange("CDLPIERCING", startIdx, endIdx);
      int guardStart = clampedStart("CDLPIERCING", startIdx, CDLPIERCING_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLPIERCING", "inOpen", inOpen, guardInLen);
      requireLength("CDLPIERCING", "inHigh", inHigh, guardInLen);
      requireLength("CDLPIERCING", "inLow", inLow, guardInLen);
      requireLength("CDLPIERCING", "inClose", inClose, guardInLen);
      requireLength("CDLPIERCING", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLPIERCING_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLPIERCING", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Two-candle pattern: a long black candle followed by a long white candle
    * that opens below the prior low and closes back above the midpoint of the
    * prior black body. Bullish reversal signal. A hit (+100) is a bullish
    * reversal signal.
    * <p><b>Notes</b>
    * <ul>
    * <li>A prior downtrend is not verified.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLPIERCING_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the piercing pattern is detected; 0 otherwise.
    *        Always bullish, never emits -100. Must hold at least
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
    * @see Core#CDLDARKCLOUDCOVER
    * @see Core#CDLENGULFING
    * @see Core#CDLMORNINGSTAR
    */
   public OutRange CDLPIERCING( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                int outInteger[] )
   {
      requireIndexRange("CDLPIERCING", startIdx, endIdx);
      int guardStart = clampedStart("CDLPIERCING", startIdx, CDLPIERCING_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLPIERCING", "inOpen", inOpen, guardInLen);
      requireLength("CDLPIERCING", "inHigh", inHigh, guardInLen);
      requireLength("CDLPIERCING", "inLow", inLow, guardInLen);
      requireLength("CDLPIERCING", "inClose", inClose, guardInLen);
      requireLength("CDLPIERCING", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLPIERCING_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLPIERCING", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLPIERCING stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLPIERCING} over the same series.
    * Open with {@link Core#CDLPIERCING_Open}; there is no close — the handle is
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
   public static final class CDLPIERCING_Stream {
      Core core;
      double[] BodyLongPeriodTotal;
      double lag1_inOpen;
      double lag1_inHigh;
      double lag1_inLow;
      double lag1_inClose;
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

      CDLPIERCING_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLPIERCING} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CDLPIERCING_Stream( CDLPIERCING_Stream other ) {
         this.core = other.core;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal.clone();
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
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

      void copyFrom( CDLPIERCING_Stream other ) {
         this.core = other.core;
         if( this.BodyLongPeriodTotal != null && this.BodyLongPeriodTotal.length == other.BodyLongPeriodTotal.length ) {
            System.arraycopy( other.BodyLongPeriodTotal, 0, this.BodyLongPeriodTotal, 0, other.BodyLongPeriodTotal.length );
         } else {
            this.BodyLongPeriodTotal = other.BodyLongPeriodTotal.clone();
         }
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ringLag_BodyLongTrailingIdx = other.ringLag_BodyLongTrailingIdx;
         if( this.ring_BodyLongTrailingIdx_derived != null && this.ring_BodyLongTrailingIdx_derived.length == other.ring_BodyLongTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_derived, 0, this.ring_BodyLongTrailingIdx_derived, 0, other.ring_BodyLongTrailingIdx_derived.length );
         } else {
            this.ring_BodyLongTrailingIdx_derived = other.ring_BodyLongTrailingIdx_derived.clone();
         }
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDLPIERCING_Stream> PEEK_SCRATCH = new ThreadLocal<>();

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
            throw new TaLibArgumentException("CDLPIERCING update: BadParam", RetCode.BadParam);
         core.CDLPIERCING_StepImpl(this, inOpen, inHigh, inLow, inClose);
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
         requireArgument("CDLPIERCING updateAndFill", "inOpen", inOpen);
         requireArgument("CDLPIERCING updateAndFill", "inHigh", inHigh);
         requireArgument("CDLPIERCING updateAndFill", "inLow", inLow);
         requireArgument("CDLPIERCING updateAndFill", "inClose", inClose);
         requireArgument("CDLPIERCING updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLPIERCING updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("CDLPIERCING updateAndFill: BadParam", RetCode.BadParam);
            core.CDLPIERCING_StepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
            throw new TaLibArgumentException("CDLPIERCING peek: BadParam", RetCode.BadParam);
         CDLPIERCING_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLPIERCING_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLPIERCING_StepImpl(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLPIERCING_Stream copy() {
         return new CDLPIERCING_Stream(this);
      }
   }
   void CDLPIERCING_StepImpl( CDLPIERCING_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int totIdx = 0;
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      sp.ring_BodyLongTrailingIdx_derived[sp.ringPos_BodyLongTrailingIdx] = ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* 1st: black */
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 1 &&                     /* 2nd: white */
          Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long */
          inOpen < sp.lag1_inLow &&                                     /* open below prior low */
          inClose < sp.lag1_inOpen &&                                   /* close within prior body */
          inClose > Math.fma(Math.abs(sp.lag1_inClose - sp.lag1_inOpen), 0.5, sp.lag1_inClose) ) /* above midpoint */
      {
         sp.cur_outInteger = 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
         sp.BodyLongPeriodTotal[totIdx] = sp.BodyLongPeriodTotal[totIdx] + (sp.ring_BodyLongTrailingIdx_derived[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - totIdx >= sp.ringCap_BodyLongTrailingIdx) ? sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - totIdx - sp.ringCap_BodyLongTrailingIdx : sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - totIdx] - sp.ring_BodyLongTrailingIdx_derived[(sp.ringPos_BodyLongTrailingIdx + sp.ringCap_BodyLongTrailingIdx - sp.ringLag_BodyLongTrailingIdx - totIdx) % sp.ringCap_BodyLongTrailingIdx]);
      }
      sp.lag1_inOpen = inOpen;
      sp.lag1_inHigh = inHigh;
      sp.lag1_inLow = inLow;
      sp.lag1_inClose = inClose;
      sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
      if( sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx ) {
         sp.ringPos_BodyLongTrailingIdx = 0;
      }
   }
   private RetCode CDLPIERCING_OpenImpl( CDLPIERCING_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double[] BodyLongPeriodTotal = new double[2];
      int i = 0;
      int outIdx = 0;
      int totIdx = 0;
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
      lookbackTotal = CDLPIERCING_Lookback();
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
      BodyLongPeriodTotal[1] = 0;
      BodyLongPeriodTotal[0] = 0;
      BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx ) {
         BodyLongPeriodTotal[1] = BodyLongPeriodTotal[1] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         BodyLongPeriodTotal[0] = BodyLongPeriodTotal[0] + ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long black candle
       * - second candle: long white candle with open below previous day low and close at least at 50% of previous day
       * real body
       * The meaning of "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100): piercing pattern is always bullish
       * the user should consider that a piercing pattern is significant when it appears in a downtrend, while
       * this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* 1st: black */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[1] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&             /* 2nd: white */
             Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal[0] / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i]))) + ((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* long */
             inOpen[i] < inLow[i - 1] &&                                 /* open below prior low */
             inClose[i] < inOpen[i - 1] &&                               /* close within prior body */
             inClose[i] > Math.fma(Math.abs(inClose[i - 1] - inOpen[i - 1]), 0.5, inClose[i - 1]) ) /* above midpoint */
         {
            outInteger[outIdx++ * outStride] = 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         for( totIdx = 1; totIdx >= 0; totIdx -= 1 ) {
            BodyLongPeriodTotal[totIdx] = BodyLongPeriodTotal[totIdx] + (((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - totIdx] - inOpen[i - totIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[i - totIdx] - inLow[i - totIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - totIdx] - (((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inClose[i - totIdx]) : (inOpen[i - totIdx]))) + ((((inClose[i - totIdx]) >= (inOpen[i - totIdx])) ? (inOpen[i - totIdx]) : (inClose[i - totIdx])) - inLow[i - totIdx])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx - totIdx] - inOpen[BodyLongTrailingIdx - totIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx - totIdx] - inLow[BodyLongTrailingIdx - totIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx - totIdx] - (((inClose[BodyLongTrailingIdx - totIdx]) >= (inOpen[BodyLongTrailingIdx - totIdx])) ? (inClose[BodyLongTrailingIdx - totIdx]) : (inOpen[BodyLongTrailingIdx - totIdx]))) + ((((inClose[BodyLongTrailingIdx - totIdx]) >= (inOpen[BodyLongTrailingIdx - totIdx])) ? (inOpen[BodyLongTrailingIdx - totIdx]) : (inClose[BodyLongTrailingIdx - totIdx])) - inLow[BodyLongTrailingIdx - totIdx])) : 0.0))));
         }
         i += 1;
         BodyLongTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      int cap_BodyLongTrailingIdx = capLag_BodyLongTrailingIdx + 2;
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
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
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
   /* CDLPIERCING_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLPIERCING_Stream CDLPIERCING_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLPIERCING_Stream sp = new CDLPIERCING_Stream(this);
      RetCode retCode = CDLPIERCING_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLPIERCING openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLPIERCING openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLPIERCING openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind CDLPIERCING_Open (composition seam). */
   CDLPIERCING_Stream CDLPIERCING_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLPIERCING_Stream sp = new CDLPIERCING_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = CDLPIERCING_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLPIERCING open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLPIERCING open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLPIERCING open: " + retCode, retCode);
   }
   /**
    * Open a live CDLPIERCING stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLPIERCING} at that bar.
    * <p>The history must hold at least {@code CDLPIERCING_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CDLPIERCING_Stream CDLPIERCING_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLPIERCING open", "inOpen", inOpen);
      requireHistory("CDLPIERCING open", inOpen.length);
      requireArgument("CDLPIERCING open", "inHigh", inHigh);
      requireArgument("CDLPIERCING open", "inLow", inLow);
      requireArgument("CDLPIERCING open", "inClose", inClose);
      requireHistoryLength("CDLPIERCING open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLPIERCING open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLPIERCING open", "inClose", inClose.length, inOpen.length);
      return CDLPIERCING_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLPIERCING_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLPIERCING} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CDLPIERCING_Stream#outRange()}.
    */
   public CDLPIERCING_Stream CDLPIERCING_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLPIERCING openAndFill", "inOpen", inOpen);
      requireHistory("CDLPIERCING openAndFill", inOpen.length);
      requireArgument("CDLPIERCING openAndFill", "inHigh", inHigh);
      requireArgument("CDLPIERCING openAndFill", "inLow", inLow);
      requireArgument("CDLPIERCING openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLPIERCING openAndFill", inOpen.length, CDLPIERCING_Lookback());
      requireHistoryLength("CDLPIERCING openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLPIERCING openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLPIERCING openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLPIERCING openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLPIERCING openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return CDLPIERCING_OpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
