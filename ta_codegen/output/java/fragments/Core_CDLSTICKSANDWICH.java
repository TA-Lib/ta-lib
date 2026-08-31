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
 *  032005 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLSTICKSANDWICH} consumes before
    * it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLSTICKSANDWICH_Lookback( )
   {
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      return Equal_avgPeriod + 2 ;

   }
   RetCode CDLSTICKSANDWICH_Impl( int startIdx,
                                  int endIdx,
                                  double inOpen[],
                                  double inHigh[],
                                  double inLow[],
                                  double inClose[],
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  int outInteger[] )
   {
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLSTICKSANDWICH_Lookback();
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
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: black candle
       * - second candle: white candle that trades only above the prior close (low > prior close)
       * - third candle: black candle with the close equal to the first candle's close
       * The meaning of "equal" is specified with TA_SetCandleSettings
       * outInteger is always positive (1 to 100): stick sandwich is always bullish;
       * the user should consider that stick sandwich is significant when coming in a downtrend,
       * while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* first black */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 &&     /* second white */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* third black */
             inLow[i - 1] > inClose[i - 2] &&                            /* 2nd low > prior close */
             inClose[i] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st and 3rd same close */
             inClose[i] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 2] - inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 2] - inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 2] - (((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inClose[EqualTrailingIdx - 2]) : (inOpen[EqualTrailingIdx - 2]))) + ((((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inOpen[EqualTrailingIdx - 2]) : (inClose[EqualTrailingIdx - 2])) - inLow[EqualTrailingIdx - 2])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLSTICKSANDWICH_Impl( int startIdx,
                                  int endIdx,
                                  float inOpen[],
                                  float inHigh[],
                                  float inLow[],
                                  float inClose[],
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  int outInteger[] )
   {
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
      int lookbackTotal = 0;
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLSTICKSANDWICH_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (double)inLow[i - 1] > (double)inClose[i - 2] && (double)inClose[i] <= (double)inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inClose[i] >= (double)inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 2] - ((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inClose[i - 2]) : ((double)inOpen[i - 2]))) + (((((double)inClose[i - 2]) >= ((double)inOpen[i - 2])) ? ((double)inOpen[i - 2]) : ((double)inClose[i - 2])) - (double)inLow[i - 2])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs((double)inClose[EqualTrailingIdx - 2] - (double)inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType == 1) ? ((double)inHigh[EqualTrailingIdx - 2] - (double)inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType == 2) ? (((double)inHigh[EqualTrailingIdx - 2] - ((((double)inClose[EqualTrailingIdx - 2]) >= ((double)inOpen[EqualTrailingIdx - 2])) ? ((double)inClose[EqualTrailingIdx - 2]) : ((double)inOpen[EqualTrailingIdx - 2]))) + (((((double)inClose[EqualTrailingIdx - 2]) >= ((double)inOpen[EqualTrailingIdx - 2])) ? ((double)inOpen[EqualTrailingIdx - 2]) : ((double)inClose[EqualTrailingIdx - 2])) - (double)inLow[EqualTrailingIdx - 2])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A three-candle bullish reversal pattern: two black candles (1st and 3rd)
    * sandwiching a white candle, where the 3rd black candle closes at the same
    * level as the 1st (the "bread"). A hit signals a bullish reversal (code
    * comment notes it is significant in a downtrend, which the function does
    * not verify).
    * <p><b>Notes</b>
    * <ul>
    * <li>Although classically a bullish reversal (and TA-Lib only emits +100), Bulkowski's testing found it actually acts as a bearish continuation 62% of the time — despite that, it still ranks a respectable 14th of 103 patterns for overall performance. ([thepatternsite.com](https://thepatternsite.com/StickSandwich.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLSTICKSANDWICH_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the pattern is present, 0 otherwise. Never
    *        -100 — Stick Sandwich is always bullish. Must hold at least
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
    * @see Core#CDLMATCHINGLOW
    * @see Core#CDLHOMINGPIGEON
    */
   public OutRange CDLSTICKSANDWICH( int startIdx,
                                     int endIdx,
                                     double inOpen[],
                                     double inHigh[],
                                     double inLow[],
                                     double inClose[],
                                     int outInteger[] )
   {
      requireIndexRange("CDLSTICKSANDWICH", startIdx, endIdx);
      int guardStart = clampedStart("CDLSTICKSANDWICH", startIdx, CDLSTICKSANDWICH_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLSTICKSANDWICH", "inOpen", inOpen, guardInLen);
      requireLength("CDLSTICKSANDWICH", "inHigh", inHigh, guardInLen);
      requireLength("CDLSTICKSANDWICH", "inLow", inLow, guardInLen);
      requireLength("CDLSTICKSANDWICH", "inClose", inClose, guardInLen);
      requireLength("CDLSTICKSANDWICH", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLSTICKSANDWICH_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLSTICKSANDWICH", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A three-candle bullish reversal pattern: two black candles (1st and 3rd)
    * sandwiching a white candle, where the 3rd black candle closes at the same
    * level as the 1st (the "bread"). A hit signals a bullish reversal (code
    * comment notes it is significant in a downtrend, which the function does
    * not verify).
    * <p><b>Notes</b>
    * <ul>
    * <li>Although classically a bullish reversal (and TA-Lib only emits +100), Bulkowski's testing found it actually acts as a bearish continuation 62% of the time — despite that, it still ranks a respectable 14th of 103 patterns for overall performance. ([thepatternsite.com](https://thepatternsite.com/StickSandwich.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLSTICKSANDWICH_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the pattern is present, 0 otherwise. Never
    *        -100 — Stick Sandwich is always bullish. Must hold at least
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
    * @see Core#CDLMATCHINGLOW
    * @see Core#CDLHOMINGPIGEON
    */
   public OutRange CDLSTICKSANDWICH( int startIdx,
                                     int endIdx,
                                     float inOpen[],
                                     float inHigh[],
                                     float inLow[],
                                     float inClose[],
                                     int outInteger[] )
   {
      requireIndexRange("CDLSTICKSANDWICH", startIdx, endIdx);
      int guardStart = clampedStart("CDLSTICKSANDWICH", startIdx, CDLSTICKSANDWICH_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLSTICKSANDWICH", "inOpen", inOpen, guardInLen);
      requireLength("CDLSTICKSANDWICH", "inHigh", inHigh, guardInLen);
      requireLength("CDLSTICKSANDWICH", "inLow", inLow, guardInLen);
      requireLength("CDLSTICKSANDWICH", "inClose", inClose, guardInLen);
      requireLength("CDLSTICKSANDWICH", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLSTICKSANDWICH_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLSTICKSANDWICH", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLSTICKSANDWICH stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLSTICKSANDWICH} over the same series.
    * Open with {@link Core#cdlsticksandwichOpen}; there is no close — the handle is
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
   public static final class CdlsticksandwichStream {
      Core core;
      double EqualPeriodTotal;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag1_inClose;
      double lag2_inClose;
      int ringPos_EqualTrailingIdx;
      int ringCap_EqualTrailingIdx;
      int ringLag_EqualTrailingIdx;
      double[] ring_EqualTrailingIdx_derived;
      int cs_Equal_rangeType;
      int cs_Equal_avgPeriod;
      double cs_Equal_factor;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      CdlsticksandwichStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLSTICKSANDWICH} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CdlsticksandwichStream( CdlsticksandwichStream other ) {
         this.core = other.core;
         this.EqualPeriodTotal = other.EqualPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.ringPos_EqualTrailingIdx = other.ringPos_EqualTrailingIdx;
         this.ringCap_EqualTrailingIdx = other.ringCap_EqualTrailingIdx;
         this.ringLag_EqualTrailingIdx = other.ringLag_EqualTrailingIdx;
         this.ring_EqualTrailingIdx_derived = other.ring_EqualTrailingIdx_derived.clone();
         this.cs_Equal_rangeType = other.cs_Equal_rangeType;
         this.cs_Equal_avgPeriod = other.cs_Equal_avgPeriod;
         this.cs_Equal_factor = other.cs_Equal_factor;
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
            throw new TaLibArgumentException("CDLSTICKSANDWICH update: BadParam", RetCode.BadParam);
         }
         core.cdlsticksandwichStepImpl(this, inOpen, inHigh, inLow, inClose);
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
         requireArgument("CDLSTICKSANDWICH updateAndFill", "inOpen", inOpen);
         requireArgument("CDLSTICKSANDWICH updateAndFill", "inHigh", inHigh);
         requireArgument("CDLSTICKSANDWICH updateAndFill", "inLow", inLow);
         requireArgument("CDLSTICKSANDWICH updateAndFill", "inClose", inClose);
         requireArgument("CDLSTICKSANDWICH updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLSTICKSANDWICH updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CDLSTICKSANDWICH updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cdlsticksandwichStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
            throw new TaLibArgumentException("CDLSTICKSANDWICH peek: BadParam", RetCode.BadParam);
         CdlsticksandwichStream sp = this;
         double EqualPeriodTotal = sp.EqualPeriodTotal;
         int cur_outInteger = sp.cur_outInteger;
         double lag1_inClose = sp.lag1_inClose;
         double lag1_inHigh = sp.lag1_inHigh;
         double lag1_inLow = sp.lag1_inLow;
         double lag1_inOpen = sp.lag1_inOpen;
         double lag2_inClose = sp.lag2_inClose;
         double lag2_inHigh = sp.lag2_inHigh;
         double lag2_inLow = sp.lag2_inLow;
         double lag2_inOpen = sp.lag2_inOpen;
         int ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int Equal_rangeType = sp.cs_Equal_rangeType;
         int Equal_avgPeriod = sp.cs_Equal_avgPeriod;
         double Equal_factor = sp.cs_Equal_factor;
         pkSlot0 = ringPos_EqualTrailingIdx;
         pkVal0 = ((Equal_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType == 1) ? (inHigh - inLow) : ((Equal_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
         if( ((lag2_inClose >= lag2_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* first black */
             ((lag1_inClose >= lag1_inOpen) ? 1 : 0 - 1) == 1 &&     /* second white */
             ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 &&           /* third black */
             lag1_inLow > lag2_inClose &&                            /* 2nd low > prior close */
             inClose <= lag2_inClose + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(lag2_inClose - lag2_inOpen)) : ((Equal_rangeType == 1) ? (lag2_inHigh - lag2_inLow) : ((Equal_rangeType == 2) ? ((lag2_inHigh - (((lag2_inClose) >= (lag2_inOpen)) ? (lag2_inClose) : (lag2_inOpen))) + ((((lag2_inClose) >= (lag2_inOpen)) ? (lag2_inOpen) : (lag2_inClose)) - lag2_inLow)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st and 3rd same close */
             inClose >= lag2_inClose - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(lag2_inClose - lag2_inOpen)) : ((Equal_rangeType == 1) ? (lag2_inHigh - lag2_inLow) : ((Equal_rangeType == 2) ? ((lag2_inHigh - (((lag2_inClose) >= (lag2_inOpen)) ? (lag2_inClose) : (lag2_inOpen))) + ((((lag2_inClose) >= (lag2_inOpen)) ? (lag2_inOpen) : (lag2_inClose)) - lag2_inLow)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            cur_outInteger = 100;
         } else {
            cur_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(lag2_inClose - lag2_inOpen)) : ((Equal_rangeType == 1) ? (lag2_inHigh - lag2_inLow) : ((Equal_rangeType == 2) ? ((lag2_inHigh - (((lag2_inClose) >= (lag2_inOpen)) ? (lag2_inClose) : (lag2_inOpen))) + ((((lag2_inClose) >= (lag2_inOpen)) ? (lag2_inOpen) : (lag2_inClose)) - lag2_inLow)) : 0.0))) - (((ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 2) % sp.ringCap_EqualTrailingIdx != pkSlot0) ? sp.ring_EqualTrailingIdx_derived[(ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 2) % sp.ringCap_EqualTrailingIdx] : pkVal0);
         lag2_inOpen = lag1_inOpen;
         lag1_inOpen = inOpen;
         lag2_inHigh = lag1_inHigh;
         lag1_inHigh = inHigh;
         lag2_inLow = lag1_inLow;
         lag1_inLow = inLow;
         lag2_inClose = lag1_inClose;
         lag1_inClose = inClose;
         ringPos_EqualTrailingIdx = ringPos_EqualTrailingIdx + 1;
         if( ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx ) {
            ringPos_EqualTrailingIdx = 0;
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
      public CdlsticksandwichStream clone() {
         return new CdlsticksandwichStream(this);
      }
   }
   void cdlsticksandwichStepImpl( CdlsticksandwichStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int Equal_rangeType = sp.cs_Equal_rangeType;
      int Equal_avgPeriod = sp.cs_Equal_avgPeriod;
      double Equal_factor = sp.cs_Equal_factor;
      sp.ring_EqualTrailingIdx_derived[sp.ringPos_EqualTrailingIdx] = ((Equal_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType == 1) ? (inHigh - inLow) : ((Equal_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* first black */
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 &&     /* second white */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 &&                 /* third black */
          sp.lag1_inLow > sp.lag2_inClose &&                            /* 2nd low > prior close */
          inClose <= sp.lag2_inClose + ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st and 3rd same close */
          inClose >= sp.lag2_inClose - ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
      {
         sp.cur_outInteger = 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((Equal_rangeType == 2) ? ((sp.lag2_inHigh - (((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inClose) : (sp.lag2_inOpen))) + ((((sp.lag2_inClose) >= (sp.lag2_inOpen)) ? (sp.lag2_inOpen) : (sp.lag2_inClose)) - sp.lag2_inLow)) : 0.0))) - sp.ring_EqualTrailingIdx_derived[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 2) % sp.ringCap_EqualTrailingIdx];
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
      sp.ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx + 1;
      if( sp.ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx ) {
         sp.ringPos_EqualTrailingIdx = 0;
      }
   }
   private RetCode cdlsticksandwichOpenImpl( CdlsticksandwichStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double EqualPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int EqualTrailingIdx = 0;
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
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLSTICKSANDWICH_Lookback();
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
      EqualPeriodTotal = 0;
      EqualTrailingIdx = startIdx - Equal_avgPeriod;
      i = EqualTrailingIdx;
      while( i < startIdx ) {
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: black candle
       * - second candle: white candle that trades only above the prior close (low > prior close)
       * - third candle: black candle with the close equal to the first candle's close
       * The meaning of "equal" is specified with TA_SetCandleSettings
       * outInteger is always positive (1 to 100): stick sandwich is always bullish;
       * the user should consider that stick sandwich is significant when coming in a downtrend,
       * while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && /* first black */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 &&     /* second white */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* third black */
             inLow[i - 1] > inClose[i - 2] &&                            /* 2nd low > prior close */
             inClose[i] <= inClose[i - 2] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st and 3rd same close */
             inClose[i] >= inClose[i - 2] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++ * outStride] = 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((Equal_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((Equal_rangeType == 2) ? ((inHigh[i - 2] - (((inClose[i - 2]) >= (inOpen[i - 2])) ? (inClose[i - 2]) : (inOpen[i - 2]))) + ((((inClose[i - 2]) >= (inOpen[i - 2])) ? (inOpen[i - 2]) : (inClose[i - 2])) - inLow[i - 2])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 2] - inOpen[EqualTrailingIdx - 2])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 2] - inLow[EqualTrailingIdx - 2]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 2] - (((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inClose[EqualTrailingIdx - 2]) : (inOpen[EqualTrailingIdx - 2]))) + ((((inClose[EqualTrailingIdx - 2]) >= (inOpen[EqualTrailingIdx - 2])) ? (inOpen[EqualTrailingIdx - 2]) : (inClose[EqualTrailingIdx - 2])) - inLow[EqualTrailingIdx - 2])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capLag_EqualTrailingIdx = i - EqualTrailingIdx;
      int cap_EqualTrailingIdx = capLag_EqualTrailingIdx + 3;
      if( capLag_EqualTrailingIdx < 0 || cap_EqualTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_EqualTrailingIdx = (cap_EqualTrailingIdx > 0)? cap_EqualTrailingIdx : 1;
      double[] capRing_EqualTrailingIdx_derived = new double[allocN_EqualTrailingIdx];
      for( int fillJ = historyLen - cap_EqualTrailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_EqualTrailingIdx_derived[fillJ % cap_EqualTrailingIdx] = ((Equal_rangeType == 0) ? (Math.abs(inClose[fillJ] - inOpen[fillJ])) : ((Equal_rangeType == 1) ? (inHigh[fillJ] - inLow[fillJ]) : ((Equal_rangeType == 2) ? ((inHigh[fillJ] - (((inClose[fillJ]) >= (inOpen[fillJ])) ? (inClose[fillJ]) : (inOpen[fillJ]))) + ((((inClose[fillJ]) >= (inOpen[fillJ])) ? (inOpen[fillJ]) : (inClose[fillJ])) - inLow[fillJ])) : 0.0)));
      }
      sp.EqualPeriodTotal = EqualPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_EqualTrailingIdx = historyLen % cap_EqualTrailingIdx;
      sp.ringCap_EqualTrailingIdx = cap_EqualTrailingIdx;
      sp.ringLag_EqualTrailingIdx = capLag_EqualTrailingIdx;
      sp.ring_EqualTrailingIdx_derived = capRing_EqualTrailingIdx_derived;
      sp.cs_Equal_rangeType = Equal_rangeType;
      sp.cs_Equal_avgPeriod = Equal_avgPeriod;
      sp.cs_Equal_factor = Equal_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cdlsticksandwichOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CdlsticksandwichStream cdlsticksandwichOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlsticksandwichStream sp = new CdlsticksandwichStream(this);
      RetCode retCode = cdlsticksandwichOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLSTICKSANDWICH openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLSTICKSANDWICH openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLSTICKSANDWICH openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cdlsticksandwichOpen (composition seam). */
   CdlsticksandwichStream cdlsticksandwichOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlsticksandwichStream sp = new CdlsticksandwichStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdlsticksandwichOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLSTICKSANDWICH open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLSTICKSANDWICH open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLSTICKSANDWICH open: " + retCode, retCode);
   }
   /**
    * Open a live CDLSTICKSANDWICH stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLSTICKSANDWICH} at that bar.
    * <p>The history must hold at least {@code CDLSTICKSANDWICH_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CdlsticksandwichStream cdlsticksandwichOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLSTICKSANDWICH open", "inOpen", inOpen);
      requireHistory("CDLSTICKSANDWICH open", inOpen.length);
      requireArgument("CDLSTICKSANDWICH open", "inHigh", inHigh);
      requireArgument("CDLSTICKSANDWICH open", "inLow", inLow);
      requireArgument("CDLSTICKSANDWICH open", "inClose", inClose);
      requireHistoryLength("CDLSTICKSANDWICH open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLSTICKSANDWICH open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLSTICKSANDWICH open", "inClose", inClose.length, inOpen.length);
      return cdlsticksandwichOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlsticksandwichOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CDLSTICKSANDWICH} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CdlsticksandwichStream#outRange()}.
    */
   public CdlsticksandwichStream cdlsticksandwichOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLSTICKSANDWICH openAndFill", "inOpen", inOpen);
      requireHistory("CDLSTICKSANDWICH openAndFill", inOpen.length);
      requireArgument("CDLSTICKSANDWICH openAndFill", "inHigh", inHigh);
      requireArgument("CDLSTICKSANDWICH openAndFill", "inLow", inLow);
      requireArgument("CDLSTICKSANDWICH openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLSTICKSANDWICH openAndFill", inOpen.length, CDLSTICKSANDWICH_Lookback());
      requireHistoryLength("CDLSTICKSANDWICH openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLSTICKSANDWICH openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLSTICKSANDWICH openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLSTICKSANDWICH openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLSTICKSANDWICH openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdlsticksandwichOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
