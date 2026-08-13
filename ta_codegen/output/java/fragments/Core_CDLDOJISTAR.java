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
 *  100204 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLDOJISTAR} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLDOJISTAR_Lookback( )
   {
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      return Math.max(BodyDoji_avgPeriod, BodyLong_avgPeriod) + 1 ;

   }
   RetCode CDLDOJISTAR_Internal( int startIdx,
                                 int endIdx,
                                 double inOpen[],
                                 double inHigh[],
                                 double inLow[],
                                 double inClose[],
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
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
      lookbackTotal = CDLDOJISTAR_Lookback();
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
      BodyDojiPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long real body
       * - second candle: star (open gapping up in an uptrend or down in a downtrend) with a doji
       * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * it's defined bullish when the long candle is white and the star gaps up, bearish when the long candle
       * is black and the star gaps down; the user should consider that a doji star is bullish when it appears
       * in an uptrend and it's bearish when it appears in a downtrend, so to determine the bullishness or
       * bearishness of the pattern the trend must be analyzed
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
             Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
             (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i], inClose[i]) > Math.max(inOpen[i - 1], inClose[i - 1])) || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i], inClose[i]) < Math.min(inOpen[i - 1], inClose[i - 1]))) ) /* that gaps up if 1st is white or down if 1st is black */
         {
            outInteger[outIdx++] = (0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1)) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLDOJISTAR_Internal( int startIdx,
                                 int endIdx,
                                 float inOpen[],
                                 float inHigh[],
                                 float inLow[],
                                 float inClose[],
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 int outInteger[] )
   {
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLDOJISTAR_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyLongPeriodTotal = 0;
      BodyDojiPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i], (double)inClose[i]) > Math.max((double)inOpen[i - 1], (double)inClose[i - 1])) || (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (Math.max((double)inOpen[i], (double)inClose[i]) < Math.min((double)inOpen[i - 1], (double)inClose[i - 1]))) ) {
            outInteger[outIdx++] = (0 - (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1)) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i - 1] - (double)inLow[i - 1]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i - 1] - (double)inLow[i - 1]) - Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyLongTrailingIdx] - (double)inLow[BodyLongTrailingIdx]) - Math.abs((double)inClose[BodyLongTrailingIdx] - (double)inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyDojiTrailingIdx] - (double)inLow[BodyDojiTrailingIdx]) - Math.abs((double)inClose[BodyDojiTrailingIdx] - (double)inOpen[BodyDojiTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A two-candle reversal pattern: a long real body followed by a doji whose
    * real body gaps away from it (up after a white body, down after a black
    * body). Signals a potential reversal of the prevailing trend. A hit flags a
    * likely trend reversal; true direction depends on the prevailing trend
    * (bullish in a downtrend, bearish in an uptrend), which the code does not
    * itself verify.
    * <p><b>Formula</b>
    * <pre>{@code
    * Two candles. Candle 1: long real body (realbody > BodyLong average). Candle 2: doji (realbody <= BodyDoji average). Gap: either candle 1 white (color==1) AND candle 2 real body gaps up above it (the real bodies gap up), or candle 1 black (color==-1) AND candle 2 real body gaps down below it (the real bodies gap down).
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend the reversal signal classically assumes.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLDOJISTAR_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger Emits +100 or -100 on a hit, 0 otherwise. Value is
    *        -candlecolor(candle1)*100: -100 when candle 1 is white (gap up), +100 when
    *        candle 1 is black (gap down) Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLMORNINGDOJISTAR
    * @see Core#CDLEVENINGDOJISTAR
    * @see Core#CDLDOJI
    * @see Core#CDLMORNINGSTAR
    * @see Core#CDLEVENINGSTAR
    */
   public OutRange CDLDOJISTAR( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLDOJISTAR_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLDOJISTAR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A two-candle reversal pattern: a long real body followed by a doji whose
    * real body gaps away from it (up after a white body, down after a black
    * body). Signals a potential reversal of the prevailing trend. A hit flags a
    * likely trend reversal; true direction depends on the prevailing trend
    * (bullish in a downtrend, bearish in an uptrend), which the code does not
    * itself verify.
    * <p><b>Formula</b>
    * <pre>{@code
    * Two candles. Candle 1: long real body (realbody > BodyLong average). Candle 2: doji (realbody <= BodyDoji average). Gap: either candle 1 white (color==1) AND candle 2 real body gaps up above it (the real bodies gap up), or candle 1 black (color==-1) AND candle 2 real body gaps down below it (the real bodies gap down).
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend the reversal signal classically assumes.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLDOJISTAR_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger Emits +100 or -100 on a hit, 0 otherwise. Value is
    *        -candlecolor(candle1)*100: -100 when candle 1 is white (gap up), +100 when
    *        candle 1 is black (gap down) Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLMORNINGDOJISTAR
    * @see Core#CDLEVENINGDOJISTAR
    * @see Core#CDLDOJI
    * @see Core#CDLMORNINGSTAR
    * @see Core#CDLEVENINGSTAR
    */
   public OutRange CDLDOJISTAR( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLDOJISTAR_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLDOJISTAR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLDOJISTAR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLDOJISTAR} over the same series.
    * Open with {@link Core#CDLDOJISTAR_Open}; there is no close — the handle is
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
   public static final class CDLDOJISTAR_Stream {
      Core core;
      double BodyDojiPeriodTotal;
      double BodyLongPeriodTotal;
      double lag1_inOpen;
      double lag1_inHigh;
      double lag1_inLow;
      double lag1_inClose;
      int ringPos_BodyDojiTrailingIdx;
      int ringCap_BodyDojiTrailingIdx;
      double[] ring_BodyDojiTrailingIdx_inOpen;
      double[] ring_BodyDojiTrailingIdx_inHigh;
      double[] ring_BodyDojiTrailingIdx_inLow;
      double[] ring_BodyDojiTrailingIdx_inClose;
      int ringPos_BodyLongTrailingIdx;
      int ringCap_BodyLongTrailingIdx;
      double[] ring_BodyLongTrailingIdx_inOpen;
      double[] ring_BodyLongTrailingIdx_inHigh;
      double[] ring_BodyLongTrailingIdx_inLow;
      double[] ring_BodyLongTrailingIdx_inClose;
      int cs_BodyDoji_rangeType;
      int cs_BodyDoji_avgPeriod;
      double cs_BodyDoji_factor;
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLDOJISTAR_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLDOJISTAR_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLDOJISTAR_Stream( CDLDOJISTAR_Stream other ) {
         this.core = other.core;
         this.BodyDojiPeriodTotal = other.BodyDojiPeriodTotal;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyDojiTrailingIdx = other.ringPos_BodyDojiTrailingIdx;
         this.ringCap_BodyDojiTrailingIdx = other.ringCap_BodyDojiTrailingIdx;
         this.ring_BodyDojiTrailingIdx_inOpen = other.ring_BodyDojiTrailingIdx_inOpen.clone();
         this.ring_BodyDojiTrailingIdx_inHigh = other.ring_BodyDojiTrailingIdx_inHigh.clone();
         this.ring_BodyDojiTrailingIdx_inLow = other.ring_BodyDojiTrailingIdx_inLow.clone();
         this.ring_BodyDojiTrailingIdx_inClose = other.ring_BodyDojiTrailingIdx_inClose.clone();
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         this.ring_BodyLongTrailingIdx_inOpen = other.ring_BodyLongTrailingIdx_inOpen.clone();
         this.ring_BodyLongTrailingIdx_inHigh = other.ring_BodyLongTrailingIdx_inHigh.clone();
         this.ring_BodyLongTrailingIdx_inLow = other.ring_BodyLongTrailingIdx_inLow.clone();
         this.ring_BodyLongTrailingIdx_inClose = other.ring_BodyLongTrailingIdx_inClose.clone();
         this.cs_BodyDoji_rangeType = other.cs_BodyDoji_rangeType;
         this.cs_BodyDoji_avgPeriod = other.cs_BodyDoji_avgPeriod;
         this.cs_BodyDoji_factor = other.cs_BodyDoji_factor;
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDLDOJISTAR_Stream other ) {
         this.core = other.core;
         this.BodyDojiPeriodTotal = other.BodyDojiPeriodTotal;
         this.BodyLongPeriodTotal = other.BodyLongPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_BodyDojiTrailingIdx = other.ringPos_BodyDojiTrailingIdx;
         this.ringCap_BodyDojiTrailingIdx = other.ringCap_BodyDojiTrailingIdx;
         if( this.ring_BodyDojiTrailingIdx_inOpen != null && this.ring_BodyDojiTrailingIdx_inOpen.length == other.ring_BodyDojiTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inOpen, 0, this.ring_BodyDojiTrailingIdx_inOpen, 0, other.ring_BodyDojiTrailingIdx_inOpen.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inOpen = other.ring_BodyDojiTrailingIdx_inOpen.clone();
         }
         if( this.ring_BodyDojiTrailingIdx_inHigh != null && this.ring_BodyDojiTrailingIdx_inHigh.length == other.ring_BodyDojiTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inHigh, 0, this.ring_BodyDojiTrailingIdx_inHigh, 0, other.ring_BodyDojiTrailingIdx_inHigh.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inHigh = other.ring_BodyDojiTrailingIdx_inHigh.clone();
         }
         if( this.ring_BodyDojiTrailingIdx_inLow != null && this.ring_BodyDojiTrailingIdx_inLow.length == other.ring_BodyDojiTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inLow, 0, this.ring_BodyDojiTrailingIdx_inLow, 0, other.ring_BodyDojiTrailingIdx_inLow.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inLow = other.ring_BodyDojiTrailingIdx_inLow.clone();
         }
         if( this.ring_BodyDojiTrailingIdx_inClose != null && this.ring_BodyDojiTrailingIdx_inClose.length == other.ring_BodyDojiTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_BodyDojiTrailingIdx_inClose, 0, this.ring_BodyDojiTrailingIdx_inClose, 0, other.ring_BodyDojiTrailingIdx_inClose.length );
         } else {
            this.ring_BodyDojiTrailingIdx_inClose = other.ring_BodyDojiTrailingIdx_inClose.clone();
         }
         this.ringPos_BodyLongTrailingIdx = other.ringPos_BodyLongTrailingIdx;
         this.ringCap_BodyLongTrailingIdx = other.ringCap_BodyLongTrailingIdx;
         if( this.ring_BodyLongTrailingIdx_inOpen != null && this.ring_BodyLongTrailingIdx_inOpen.length == other.ring_BodyLongTrailingIdx_inOpen.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_inOpen, 0, this.ring_BodyLongTrailingIdx_inOpen, 0, other.ring_BodyLongTrailingIdx_inOpen.length );
         } else {
            this.ring_BodyLongTrailingIdx_inOpen = other.ring_BodyLongTrailingIdx_inOpen.clone();
         }
         if( this.ring_BodyLongTrailingIdx_inHigh != null && this.ring_BodyLongTrailingIdx_inHigh.length == other.ring_BodyLongTrailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_inHigh, 0, this.ring_BodyLongTrailingIdx_inHigh, 0, other.ring_BodyLongTrailingIdx_inHigh.length );
         } else {
            this.ring_BodyLongTrailingIdx_inHigh = other.ring_BodyLongTrailingIdx_inHigh.clone();
         }
         if( this.ring_BodyLongTrailingIdx_inLow != null && this.ring_BodyLongTrailingIdx_inLow.length == other.ring_BodyLongTrailingIdx_inLow.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_inLow, 0, this.ring_BodyLongTrailingIdx_inLow, 0, other.ring_BodyLongTrailingIdx_inLow.length );
         } else {
            this.ring_BodyLongTrailingIdx_inLow = other.ring_BodyLongTrailingIdx_inLow.clone();
         }
         if( this.ring_BodyLongTrailingIdx_inClose != null && this.ring_BodyLongTrailingIdx_inClose.length == other.ring_BodyLongTrailingIdx_inClose.length ) {
            System.arraycopy( other.ring_BodyLongTrailingIdx_inClose, 0, this.ring_BodyLongTrailingIdx_inClose, 0, other.ring_BodyLongTrailingIdx_inClose.length );
         } else {
            this.ring_BodyLongTrailingIdx_inClose = other.ring_BodyLongTrailingIdx_inClose.clone();
         }
         this.cs_BodyDoji_rangeType = other.cs_BodyDoji_rangeType;
         this.cs_BodyDoji_avgPeriod = other.cs_BodyDoji_avgPeriod;
         this.cs_BodyDoji_factor = other.cs_BodyDoji_factor;
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<CDLDOJISTAR_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.CDLDOJISTAR_StreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CDLDOJISTAR_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new CDLDOJISTAR_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.CDLDOJISTAR_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLDOJISTAR_Stream copy() {
         return new CDLDOJISTAR_Stream(this);
      }
   }
   void CDLDOJISTAR_StreamStep( CDLDOJISTAR_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
      int BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
      double BodyDoji_factor = sp.cs_BodyDoji_factor;
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      if( sp.ringCap_BodyDojiTrailingIdx == 0 ) {
         sp.ring_BodyDojiTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyDojiTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyDojiTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyDojiTrailingIdx_inClose[0] = inClose;
      }
      if( sp.ringCap_BodyLongTrailingIdx == 0 ) {
         sp.ring_BodyLongTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyLongTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyLongTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyLongTrailingIdx_inClose[0] = inClose;
      }
      if( Math.abs(sp.lag1_inClose - sp.lag1_inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
          Math.abs(inClose - inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (sp.BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType == 1) ? (inHigh - inLow) : ((BodyDoji_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
          (((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && (Math.min(inOpen, inClose) > Math.max(sp.lag1_inOpen, sp.lag1_inClose)) || ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen, inClose) < Math.min(sp.lag1_inOpen, sp.lag1_inClose))) ) /* that gaps up if 1st is white or down if 1st is black */
      {
         sp.cur_outInteger = (0 - ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1)) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : ((BodyLong_rangeType == 1) ? (sp.lag1_inHigh - sp.lag1_inLow) : ((BodyLong_rangeType == 2) ? ((sp.lag1_inHigh - sp.lag1_inLow) - Math.abs(sp.lag1_inClose - sp.lag1_inOpen)) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx]) - Math.abs(sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] - sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx])) : 0.0)));
      sp.BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyDoji_rangeType == 1) ? (inHigh - inLow) : ((BodyDoji_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx]) - Math.abs(sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] - sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx])) : 0.0)));
      sp.lag1_inOpen = inOpen;
      sp.lag1_inHigh = inHigh;
      sp.lag1_inLow = inLow;
      sp.lag1_inClose = inClose;
      sp.ring_BodyDojiTrailingIdx_inOpen[sp.ringPos_BodyDojiTrailingIdx] = inOpen;
      sp.ring_BodyDojiTrailingIdx_inHigh[sp.ringPos_BodyDojiTrailingIdx] = inHigh;
      sp.ring_BodyDojiTrailingIdx_inLow[sp.ringPos_BodyDojiTrailingIdx] = inLow;
      sp.ring_BodyDojiTrailingIdx_inClose[sp.ringPos_BodyDojiTrailingIdx] = inClose;
      sp.ringPos_BodyDojiTrailingIdx = sp.ringPos_BodyDojiTrailingIdx + 1;
      if( sp.ringPos_BodyDojiTrailingIdx >= sp.ringCap_BodyDojiTrailingIdx ) {
         sp.ringPos_BodyDojiTrailingIdx = 0;
      }
      sp.ring_BodyLongTrailingIdx_inOpen[sp.ringPos_BodyLongTrailingIdx] = inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh[sp.ringPos_BodyLongTrailingIdx] = inHigh;
      sp.ring_BodyLongTrailingIdx_inLow[sp.ringPos_BodyLongTrailingIdx] = inLow;
      sp.ring_BodyLongTrailingIdx_inClose[sp.ringPos_BodyLongTrailingIdx] = inClose;
      sp.ringPos_BodyLongTrailingIdx = sp.ringPos_BodyLongTrailingIdx + 1;
      if( sp.ringPos_BodyLongTrailingIdx >= sp.ringCap_BodyLongTrailingIdx ) {
         sp.ringPos_BodyLongTrailingIdx = 0;
      }
   }
   private RetCode CDLDOJISTAR_OpenCore( CDLDOJISTAR_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double BodyDojiPeriodTotal = 0;
      double BodyLongPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyDojiTrailingIdx = 0;
      int BodyLongTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLDOJISTAR_Lookback();
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
      BodyLongPeriodTotal = 0;
      BodyDojiPeriodTotal = 0;
      BodyLongTrailingIdx = startIdx - 1 - BodyLong_avgPeriod;
      BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;
      i = BodyLongTrailingIdx;
      while( i < startIdx - 1 ) {
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = BodyDojiTrailingIdx;
      while( i < startIdx ) {
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first candle: long real body
       * - second candle: star (open gapping up in an uptrend or down in a downtrend) with a doji
       * The meaning of "doji" and "long" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * it's defined bullish when the long candle is white and the star gaps up, bearish when the long candle
       * is black and the star gaps down; the user should consider that a doji star is bullish when it appears
       * in an uptrend and it's bearish when it appears in a downtrend, so to determine the bullishness or
       * bearishness of the pattern the trend must be analyzed
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 1] - inOpen[i - 1]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyLongPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: long real body */
             Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyDojiPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
             (((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i], inClose[i]) > Math.max(inOpen[i - 1], inClose[i - 1])) || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i], inClose[i]) < Math.min(inOpen[i - 1], inClose[i - 1]))) ) /* that gaps up if 1st is white or down if 1st is black */
         {
            outInteger[outIdx++ * outStride] = (0 - ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1)) * 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyLongPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i - 1] - inOpen[i - 1])) : ((BodyLong_rangeType == 1) ? (inHigh[i - 1] - inLow[i - 1]) : ((BodyLong_rangeType == 2) ? ((inHigh[i - 1] - inLow[i - 1]) - Math.abs(inClose[i - 1] - inOpen[i - 1])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyLongTrailingIdx] - inLow[BodyLongTrailingIdx]) - Math.abs(inClose[BodyLongTrailingIdx] - inOpen[BodyLongTrailingIdx])) : 0.0)));
         BodyDojiPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyDojiTrailingIdx] - inLow[BodyDojiTrailingIdx]) - Math.abs(inClose[BodyDojiTrailingIdx] - inOpen[BodyDojiTrailingIdx])) : 0.0)));
         i += 1;
         BodyLongTrailingIdx += 1;
         BodyDojiTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_BodyDojiTrailingIdx = i - BodyDojiTrailingIdx;
      if( cap_BodyDojiTrailingIdx < 0 || cap_BodyDojiTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyDojiTrailingIdx = (cap_BodyDojiTrailingIdx > 0)? cap_BodyDojiTrailingIdx : 1;
      double[] capRing_BodyDojiTrailingIdx_inOpen = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inOpen, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inHigh = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inHigh, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inLow = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inLow, 0, cap_BodyDojiTrailingIdx);
      double[] capRing_BodyDojiTrailingIdx_inClose = new double[allocN_BodyDojiTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyDojiTrailingIdx, capRing_BodyDojiTrailingIdx_inClose, 0, cap_BodyDojiTrailingIdx);
      int cap_BodyLongTrailingIdx = i - BodyLongTrailingIdx;
      if( cap_BodyLongTrailingIdx < 0 || cap_BodyLongTrailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_BodyLongTrailingIdx = (cap_BodyLongTrailingIdx > 0)? cap_BodyLongTrailingIdx : 1;
      double[] capRing_BodyLongTrailingIdx_inOpen = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inOpen, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inHigh = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inHigh, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inLow = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inLow, 0, cap_BodyLongTrailingIdx);
      double[] capRing_BodyLongTrailingIdx_inClose = new double[allocN_BodyLongTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyLongTrailingIdx, capRing_BodyLongTrailingIdx_inClose, 0, cap_BodyLongTrailingIdx);
      sp.BodyDojiPeriodTotal = BodyDojiPeriodTotal;
      sp.BodyLongPeriodTotal = BodyLongPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_BodyDojiTrailingIdx = 0;
      sp.ringCap_BodyDojiTrailingIdx = cap_BodyDojiTrailingIdx;
      sp.ring_BodyDojiTrailingIdx_inOpen = capRing_BodyDojiTrailingIdx_inOpen;
      sp.ring_BodyDojiTrailingIdx_inHigh = capRing_BodyDojiTrailingIdx_inHigh;
      sp.ring_BodyDojiTrailingIdx_inLow = capRing_BodyDojiTrailingIdx_inLow;
      sp.ring_BodyDojiTrailingIdx_inClose = capRing_BodyDojiTrailingIdx_inClose;
      sp.ringPos_BodyLongTrailingIdx = 0;
      sp.ringCap_BodyLongTrailingIdx = cap_BodyLongTrailingIdx;
      sp.ring_BodyLongTrailingIdx_inOpen = capRing_BodyLongTrailingIdx_inOpen;
      sp.ring_BodyLongTrailingIdx_inHigh = capRing_BodyLongTrailingIdx_inHigh;
      sp.ring_BodyLongTrailingIdx_inLow = capRing_BodyLongTrailingIdx_inLow;
      sp.ring_BodyLongTrailingIdx_inClose = capRing_BodyLongTrailingIdx_inClose;
      sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
      sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
      sp.cs_BodyDoji_factor = BodyDoji_factor;
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CDLDOJISTAR_OpenBody( CDLDOJISTAR_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDLDOJISTAR_OpenCore( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDLDOJISTAR_OpenAndFillBody( CDLDOJISTAR_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDLDOJISTAR_OpenCore( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDLDOJISTAR_OpenAndFillInternalBody( CDLDOJISTAR_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDLDOJISTAR_OpenCore(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDLDOJISTAR_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLDOJISTAR_Stream CDLDOJISTAR_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLDOJISTAR_Stream sp = new CDLDOJISTAR_Stream(this);
      RetCode retCode = CDLDOJISTAR_OpenAndFillInternalBody(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLDOJISTAR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLDOJISTAR openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLDOJISTAR openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind CDLDOJISTAR_Open (composition seam). */
   CDLDOJISTAR_Stream CDLDOJISTAR_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLDOJISTAR_Stream sp = new CDLDOJISTAR_Stream(this);
      RetCode retCode = CDLDOJISTAR_OpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLDOJISTAR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLDOJISTAR open: internal error");
      }
      throw new IllegalArgumentException("CDLDOJISTAR open: " + retCode);
   }
   /**
    * Open a live CDLDOJISTAR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLDOJISTAR} at that bar.
    * <p>The history must hold at least {@code CDLDOJISTAR_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLDOJISTAR_Stream CDLDOJISTAR_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLDOJISTAR_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLDOJISTAR_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLDOJISTAR} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLDOJISTAR_Stream#fillRange()}.
    */
   public CDLDOJISTAR_Stream CDLDOJISTAR_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLDOJISTAR_Stream sp = new CDLDOJISTAR_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLDOJISTAR_OpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLDOJISTAR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLDOJISTAR openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLDOJISTAR openAndFill: " + retCode);
   }
