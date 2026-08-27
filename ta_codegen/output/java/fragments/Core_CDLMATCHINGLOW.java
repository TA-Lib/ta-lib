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
 *  032605 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLMATCHINGLOW} consumes before
    * it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLMATCHINGLOW_Lookback( )
   {
      int Equal_rangeType = this.candleSettings[CandleSettingType.Equal.ordinal()].rangeType.ordinal();
      int Equal_avgPeriod = this.candleSettings[CandleSettingType.Equal.ordinal()].avgPeriod;
      double Equal_factor = this.candleSettings[CandleSettingType.Equal.ordinal()].factor;
      return Equal_avgPeriod + 1 ;

   }
   RetCode CDLMATCHINGLOW_Impl( int startIdx,
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
      lookbackTotal = CDLMATCHINGLOW_Lookback();
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
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: black candle
       * - second candle: black candle with the close equal to the previous close
       * The meaning of "equal" is specified with TA_SetCandleSettings
       * outInteger is always positive (1 to 100): matching low is always bullish;
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* first black */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* second black */
             inClose[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st and 2nd same close */
             inClose[i] >= inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLMATCHINGLOW_Impl( int startIdx,
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
      lookbackTotal = CDLMATCHINGLOW_Lookback();
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
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (double)inClose[i] <= (double)inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && (double)inClose[i] >= (double)inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[i - 1] - ((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inClose[i - 1]) : ((double)inOpen[i - 1]))) + (((((double)inClose[i - 1]) >= ((double)inOpen[i - 1])) ? ((double)inOpen[i - 1]) : ((double)inClose[i - 1])) - (double)inLow[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs((double)inClose[EqualTrailingIdx - 1] - (double)inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? ((double)inHigh[EqualTrailingIdx - 1] - (double)inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? (((double)inHigh[EqualTrailingIdx - 1] - ((((double)inClose[EqualTrailingIdx - 1]) >= ((double)inOpen[EqualTrailingIdx - 1])) ? ((double)inClose[EqualTrailingIdx - 1]) : ((double)inOpen[EqualTrailingIdx - 1]))) + (((((double)inClose[EqualTrailingIdx - 1]) >= ((double)inOpen[EqualTrailingIdx - 1])) ? ((double)inOpen[EqualTrailingIdx - 1]) : ((double)inClose[EqualTrailingIdx - 1])) - (double)inLow[EqualTrailingIdx - 1])) : 0.0)));
         i += 1;
         EqualTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A two-candle pattern of two consecutive black (bearish) candles with equal
    * closes (within a tolerance). Treated as a bullish reversal signal. A hit
    * signals a potential bullish reversal (shared support close after two down
    * candles).
    * <p><b>Formula</b>
    * <pre>{@code
    * Two candles i-1, i. Candle i-1: black (close<open). Candle i: black (close<open). Equal closes: close[i-1]-E <= close[i] <= close[i-1]+E, where E = the Equal average. No shadow, body-size, or gap conditions are checked.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The bullish-reversal reading assumes a prior downtrend, which is not verified.</li>
    * <li>Although classically read as a bullish reversal (and TA-Lib only emits +100), Bulkowski's testing found it actually acts as a bearish continuation 61% of the time — even so, it still ranks a strong 8th of 103 patterns for overall performance. ([thepatternsite.com](https://thepatternsite.com/MatchingLow.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLMATCHINGLOW_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the pattern is present, 0 otherwise. Only +100
    *        is ever emitted (matching low is always bullish); never -100. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#CDLHOMINGPIGEON
    */
   public OutRange CDLMATCHINGLOW( int startIdx,
                                   int endIdx,
                                   double inOpen[],
                                   double inHigh[],
                                   double inLow[],
                                   double inClose[],
                                   int outInteger[] )
   {
      requireIndexRange("CDLMATCHINGLOW", startIdx, endIdx);
      int guardStart = clampedStart("CDLMATCHINGLOW", startIdx, CDLMATCHINGLOW_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLMATCHINGLOW", "inOpen", inOpen, guardInLen);
      requireLength("CDLMATCHINGLOW", "inHigh", inHigh, guardInLen);
      requireLength("CDLMATCHINGLOW", "inLow", inLow, guardInLen);
      requireLength("CDLMATCHINGLOW", "inClose", inClose, guardInLen);
      requireLength("CDLMATCHINGLOW", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLMATCHINGLOW_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLMATCHINGLOW", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A two-candle pattern of two consecutive black (bearish) candles with equal
    * closes (within a tolerance). Treated as a bullish reversal signal. A hit
    * signals a potential bullish reversal (shared support close after two down
    * candles).
    * <p><b>Formula</b>
    * <pre>{@code
    * Two candles i-1, i. Candle i-1: black (close<open). Candle i: black (close<open). Equal closes: close[i-1]-E <= close[i] <= close[i-1]+E, where E = the Equal average. No shadow, body-size, or gap conditions are checked.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The bullish-reversal reading assumes a prior downtrend, which is not verified.</li>
    * <li>Although classically read as a bullish reversal (and TA-Lib only emits +100), Bulkowski's testing found it actually acts as a bearish continuation 61% of the time — even so, it still ranks a strong 8th of 103 patterns for overall performance. ([thepatternsite.com](https://thepatternsite.com/MatchingLow.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLMATCHINGLOW_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the pattern is present, 0 otherwise. Only +100
    *        is ever emitted (matching low is always bullish); never -100. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#CDLHOMINGPIGEON
    */
   public OutRange CDLMATCHINGLOW( int startIdx,
                                   int endIdx,
                                   float inOpen[],
                                   float inHigh[],
                                   float inLow[],
                                   float inClose[],
                                   int outInteger[] )
   {
      requireIndexRange("CDLMATCHINGLOW", startIdx, endIdx);
      int guardStart = clampedStart("CDLMATCHINGLOW", startIdx, CDLMATCHINGLOW_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLMATCHINGLOW", "inOpen", inOpen, guardInLen);
      requireLength("CDLMATCHINGLOW", "inHigh", inHigh, guardInLen);
      requireLength("CDLMATCHINGLOW", "inLow", inLow, guardInLen);
      requireLength("CDLMATCHINGLOW", "inClose", inClose, guardInLen);
      requireLength("CDLMATCHINGLOW", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLMATCHINGLOW_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLMATCHINGLOW", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLMATCHINGLOW stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLMATCHINGLOW} over the same series.
    * Open with {@link Core#CDLMATCHINGLOW_Open}; there is no close — the handle is
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
   public static final class CDLMATCHINGLOW_Stream {
      Core core;
      double EqualPeriodTotal;
      double lag1_inOpen;
      double lag1_inHigh;
      double lag1_inLow;
      double lag1_inClose;
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

      CDLMATCHINGLOW_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLMATCHINGLOW} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CDLMATCHINGLOW_Stream( CDLMATCHINGLOW_Stream other ) {
         this.core = other.core;
         this.EqualPeriodTotal = other.EqualPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
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

      void copyFrom( CDLMATCHINGLOW_Stream other ) {
         this.core = other.core;
         this.EqualPeriodTotal = other.EqualPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_EqualTrailingIdx = other.ringPos_EqualTrailingIdx;
         this.ringCap_EqualTrailingIdx = other.ringCap_EqualTrailingIdx;
         this.ringLag_EqualTrailingIdx = other.ringLag_EqualTrailingIdx;
         if( this.ring_EqualTrailingIdx_derived != null && this.ring_EqualTrailingIdx_derived.length == other.ring_EqualTrailingIdx_derived.length ) {
            System.arraycopy( other.ring_EqualTrailingIdx_derived, 0, this.ring_EqualTrailingIdx_derived, 0, other.ring_EqualTrailingIdx_derived.length );
         } else {
            this.ring_EqualTrailingIdx_derived = other.ring_EqualTrailingIdx_derived.clone();
         }
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
       * written, so the handle is left exactly as it was —
       * the stream stays usable, so skip the bar or re-open on a clean
       * history. This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDLMATCHINGLOW update: BadParam", RetCode.BadParam);
         core.CDLMATCHINGLOW_StepImpl(this, inOpen, inHigh, inLow, inClose);
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
         requireArgument("CDLMATCHINGLOW updateAndFill", "inOpen", inOpen);
         requireArgument("CDLMATCHINGLOW updateAndFill", "inHigh", inHigh);
         requireArgument("CDLMATCHINGLOW updateAndFill", "inLow", inLow);
         requireArgument("CDLMATCHINGLOW updateAndFill", "inClose", inClose);
         requireArgument("CDLMATCHINGLOW updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLMATCHINGLOW updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("CDLMATCHINGLOW updateAndFill: BadParam", RetCode.BadParam);
            core.CDLMATCHINGLOW_StepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
            outInteger[i] = this.cur_outInteger;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDLMATCHINGLOW peek: BadParam", RetCode.BadParam);
         CDLMATCHINGLOW_Stream scratch = new CDLMATCHINGLOW_Stream(this);
         core.CDLMATCHINGLOW_StepImpl(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLMATCHINGLOW_Stream copy() {
         return new CDLMATCHINGLOW_Stream(this);
      }
   }
   void CDLMATCHINGLOW_StepImpl( CDLMATCHINGLOW_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int Equal_rangeType = sp.cs_Equal_rangeType;
      int Equal_avgPeriod = sp.cs_Equal_avgPeriod;
      double Equal_factor = sp.cs_Equal_factor;
      sp.ring_EqualTrailingIdx_derived[sp.ringPos_EqualTrailingIdx] = ((Equal_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((Equal_rangeType == 1) ? (inHigh - inLow) : ((Equal_rangeType == 2) ? ((inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen))) + ((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)) : 0.0)));
      if( ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* first black */
          ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 &&                 /* second black */
          inClose <= sp.lag1_inClose + ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st and 2nd same close */
          inClose >= sp.lag1_inClose - ((Equal_factor * (((Equal_avgPeriod != 0) ? (sp.EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
      {
         sp.cur_outInteger = 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((Equal_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((Equal_rangeType == 2) ? ((sp.lag1_inHigh - (((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inClose) : (sp.lag1_inOpen))) + ((((sp.lag1_inClose) >= (sp.lag1_inOpen)) ? (sp.lag1_inOpen) : (sp.lag1_inClose)) - sp.lag1_inLow)) : 0.0))) - sp.ring_EqualTrailingIdx_derived[(sp.ringPos_EqualTrailingIdx + sp.ringCap_EqualTrailingIdx - sp.ringLag_EqualTrailingIdx - 1) % sp.ringCap_EqualTrailingIdx];
      sp.lag1_inOpen = inOpen;
      sp.lag1_inHigh = inHigh;
      sp.lag1_inLow = inLow;
      sp.lag1_inClose = inClose;
      sp.ringPos_EqualTrailingIdx = sp.ringPos_EqualTrailingIdx + 1;
      if( sp.ringPos_EqualTrailingIdx >= sp.ringCap_EqualTrailingIdx ) {
         sp.ringPos_EqualTrailingIdx = 0;
      }
   }
   private RetCode CDLMATCHINGLOW_OpenImpl( CDLMATCHINGLOW_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
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
      lookbackTotal = CDLMATCHINGLOW_Lookback();
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
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: black candle
       * - second candle: black candle with the close equal to the previous close
       * The meaning of "equal" is specified with TA_SetCandleSettings
       * outInteger is always positive (1 to 100): matching low is always bullish;
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* first black */
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&         /* second black */
             inClose[i] <= inClose[i - 1] + ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st and 2nd same close */
             inClose[i] >= inClose[i - 1] - ((Equal_factor * (((Equal_avgPeriod != 0) ? (EqualPeriodTotal / Equal_avgPeriod) : ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0)))) / ((Equal_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            outInteger[outIdx++ * outStride] = 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         EqualPeriodTotal += ((Equal_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((Equal_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((Equal_rangeType == 2) ? ((inHigh[i - 1] - (((inClose[i - 1]) >= (inOpen[i - 1])) ? (inClose[i - 1]) : (inOpen[i - 1]))) + ((((inClose[i - 1]) >= (inOpen[i - 1])) ? (inOpen[i - 1]) : (inClose[i - 1])) - inLow[i - 1])) : 0.0))) - ((Equal_rangeType == 0) ? (Math.abs(inClose[EqualTrailingIdx - 1] - inOpen[EqualTrailingIdx - 1])) : ((Equal_rangeType == 1) ? (inHigh[EqualTrailingIdx - 1] - inLow[EqualTrailingIdx - 1]) : ((Equal_rangeType == 2) ? ((inHigh[EqualTrailingIdx - 1] - (((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inClose[EqualTrailingIdx - 1]) : (inOpen[EqualTrailingIdx - 1]))) + ((((inClose[EqualTrailingIdx - 1]) >= (inOpen[EqualTrailingIdx - 1])) ? (inOpen[EqualTrailingIdx - 1]) : (inClose[EqualTrailingIdx - 1])) - inLow[EqualTrailingIdx - 1])) : 0.0)));
         i += 1;
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
      sp.EqualPeriodTotal = EqualPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
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
   /* CDLMATCHINGLOW_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLMATCHINGLOW_Stream CDLMATCHINGLOW_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLMATCHINGLOW_Stream sp = new CDLMATCHINGLOW_Stream(this);
      RetCode retCode = CDLMATCHINGLOW_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLMATCHINGLOW openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLMATCHINGLOW openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLMATCHINGLOW openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind CDLMATCHINGLOW_Open (composition seam). */
   CDLMATCHINGLOW_Stream CDLMATCHINGLOW_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLMATCHINGLOW_Stream sp = new CDLMATCHINGLOW_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = CDLMATCHINGLOW_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLMATCHINGLOW open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLMATCHINGLOW open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLMATCHINGLOW open: " + retCode, retCode);
   }
   /**
    * Open a live CDLMATCHINGLOW stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLMATCHINGLOW} at that bar.
    * <p>The history must hold at least {@code CDLMATCHINGLOW_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CDLMATCHINGLOW_Stream CDLMATCHINGLOW_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLMATCHINGLOW open", "inOpen", inOpen);
      requireHistory("CDLMATCHINGLOW open", inOpen.length);
      requireArgument("CDLMATCHINGLOW open", "inHigh", inHigh);
      requireArgument("CDLMATCHINGLOW open", "inLow", inLow);
      requireArgument("CDLMATCHINGLOW open", "inClose", inClose);
      requireHistoryLength("CDLMATCHINGLOW open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLMATCHINGLOW open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLMATCHINGLOW open", "inClose", inClose.length, inOpen.length);
      return CDLMATCHINGLOW_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLMATCHINGLOW_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLMATCHINGLOW} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CDLMATCHINGLOW_Stream#outRange()}.
    */
   public CDLMATCHINGLOW_Stream CDLMATCHINGLOW_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLMATCHINGLOW openAndFill", "inOpen", inOpen);
      requireHistory("CDLMATCHINGLOW openAndFill", inOpen.length);
      requireArgument("CDLMATCHINGLOW openAndFill", "inHigh", inHigh);
      requireArgument("CDLMATCHINGLOW openAndFill", "inLow", inLow);
      requireArgument("CDLMATCHINGLOW openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLMATCHINGLOW openAndFill", inOpen.length, CDLMATCHINGLOW_Lookback());
      requireHistoryLength("CDLMATCHINGLOW openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLMATCHINGLOW openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLMATCHINGLOW openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLMATCHINGLOW openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLMATCHINGLOW openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return CDLMATCHINGLOW_OpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
