/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  CSB      Christopher Barnhouse
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  100204 AC      Creation
 *  051005 CSB,AC  Fix #1199526 for out-of-bound write in output.
 */

   /**
    * Number of leading input bars {@link Core#CDLTRISTAR} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLTRISTAR_Lookback( )
   {
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      return BodyDoji_avgPeriod + 2 ;

   }
   RetCode CDLTRISTAR_Internal( int startIdx,
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
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLTRISTAR_Lookback();
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
      BodyTrailingIdx = startIdx - 2 - BodyDoji_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - 3 consecutive doji days
       * - the second doji is a star
       * The meaning of "doji" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish
       */
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 2] - inOpen[i - 2]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: doji */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
             Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            /* 3rd: doji */
            outInteger[outIdx] = 0;
            if( (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && /* 2nd gaps up */
                Math.max(inOpen[i], inClose[i]) < Math.max(inOpen[i - 1], inClose[i - 1]) ) /* 3rd is not higher than 2nd */
            {
               outInteger[outIdx] = 0 - 100;
            }
            if( (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && /* 2nd gaps down */
                Math.min(inOpen[i], inClose[i]) > Math.min(inOpen[i - 1], inClose[i - 1]) ) /* 3rd is not lower than 2nd */
            {
               outInteger[outIdx] = 100;
            }
            outIdx += 1;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLTRISTAR_Internal( int startIdx,
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
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int lookbackTotal = 0;
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLTRISTAR_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - 2 - BodyDoji_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i - 1] - (double)inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && Math.abs((double)inClose[i] - (double)inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx] = 0;
            if( (Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) && Math.max((double)inOpen[i], (double)inClose[i]) < Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) ) {
               outInteger[outIdx] = 0 - 100;
            }
            if( (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2])) && Math.min((double)inOpen[i], (double)inClose[i]) > Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) ) {
               outInteger[outIdx] = 100;
            }
            outIdx += 1;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[i - 2] - (double)inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[i - 2] - (double)inLow[i - 2]) - Math.abs((double)inClose[i - 2] - (double)inOpen[i - 2])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : ((BodyDoji_rangeType == 1) ? ((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) : ((BodyDoji_rangeType == 2) ? (((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) - Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A three-candle pattern of three consecutive doji where the middle doji is
    * a star (its body gaps away from the first). Bullish or bearish reversal
    * signal. +100 = bullish reversal (middle doji gapped down), -100 = bearish
    * reversal (middle doji gapped up).
    * <p><b>Notes</b>
    * <ul>
    * <li>This reversal pattern does not verify the prior trend it classically assumes.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLTRISTAR_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 (bullish, star gapped down), -100 (bearish, star
    *        gapped up), or 0 when no pattern. Both signs are emitted. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLDOJI
    * @see Core#CDLDOJISTAR
    * @see Core#CDLMORNINGDOJISTAR
    * @see Core#CDLEVENINGDOJISTAR
    */
   public OutRange CDLTRISTAR( int startIdx,
                               int endIdx,
                               double inOpen[],
                               double inHigh[],
                               double inLow[],
                               double inClose[],
                               int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLTRISTAR_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLTRISTAR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A three-candle pattern of three consecutive doji where the middle doji is
    * a star (its body gaps away from the first). Bullish or bearish reversal
    * signal. +100 = bullish reversal (middle doji gapped down), -100 = bearish
    * reversal (middle doji gapped up).
    * <p><b>Notes</b>
    * <ul>
    * <li>This reversal pattern does not verify the prior trend it classically assumes.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLTRISTAR_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 (bullish, star gapped down), -100 (bearish, star
    *        gapped up), or 0 when no pattern. Both signs are emitted. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLDOJI
    * @see Core#CDLDOJISTAR
    * @see Core#CDLMORNINGDOJISTAR
    * @see Core#CDLEVENINGDOJISTAR
    */
   public OutRange CDLTRISTAR( int startIdx,
                               int endIdx,
                               float inOpen[],
                               float inHigh[],
                               float inLow[],
                               float inClose[],
                               int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLTRISTAR_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLTRISTAR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLTRISTAR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLTRISTAR} over the same series.
    * Open with {@link Core#CDLTRISTAR_Open}; there is no close — the handle is
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
   public static final class CDLTRISTAR_Stream {
      final Core core;
      double BodyPeriodTotal;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      double lag1_inClose;
      double lag2_inClose;
      int ringPos_BodyTrailingIdx;
      int ringCap_BodyTrailingIdx;
      double[] ring_BodyTrailingIdx_inOpen;
      double[] ring_BodyTrailingIdx_inHigh;
      double[] ring_BodyTrailingIdx_inLow;
      double[] ring_BodyTrailingIdx_inClose;
      int cs_BodyDoji_rangeType;
      int cs_BodyDoji_avgPeriod;
      double cs_BodyDoji_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLTRISTAR_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLTRISTAR_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLTRISTAR_Stream( CDLTRISTAR_Stream other ) {
         this.core = other.core;
         this.BodyPeriodTotal = other.BodyPeriodTotal;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.ringPos_BodyTrailingIdx = other.ringPos_BodyTrailingIdx;
         this.ringCap_BodyTrailingIdx = other.ringCap_BodyTrailingIdx;
         this.ring_BodyTrailingIdx_inOpen = other.ring_BodyTrailingIdx_inOpen.clone();
         this.ring_BodyTrailingIdx_inHigh = other.ring_BodyTrailingIdx_inHigh.clone();
         this.ring_BodyTrailingIdx_inLow = other.ring_BodyTrailingIdx_inLow.clone();
         this.ring_BodyTrailingIdx_inClose = other.ring_BodyTrailingIdx_inClose.clone();
         this.cs_BodyDoji_rangeType = other.cs_BodyDoji_rangeType;
         this.cs_BodyDoji_avgPeriod = other.cs_BodyDoji_avgPeriod;
         this.cs_BodyDoji_factor = other.cs_BodyDoji_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.CDLTRISTAR_StreamStep(this, inOpen, inHigh, inLow, inClose);
         return this.cur_outInteger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         CDLTRISTAR_Stream scratch = new CDLTRISTAR_Stream(this);
         core.CDLTRISTAR_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLTRISTAR_Stream copy() {
         return new CDLTRISTAR_Stream(this);
      }
   }
   void CDLTRISTAR_StreamStep( CDLTRISTAR_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyDoji_rangeType = sp.cs_BodyDoji_rangeType;
      int BodyDoji_avgPeriod = sp.cs_BodyDoji_avgPeriod;
      double BodyDoji_factor = sp.cs_BodyDoji_factor;
      if( sp.ringCap_BodyTrailingIdx == 0 ) {
         sp.ring_BodyTrailingIdx_inOpen[0] = inOpen;
         sp.ring_BodyTrailingIdx_inHigh[0] = inHigh;
         sp.ring_BodyTrailingIdx_inLow[0] = inLow;
         sp.ring_BodyTrailingIdx_inClose[0] = inClose;
      }
      if( Math.abs(sp.lag2_inClose - sp.lag2_inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (sp.BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyDoji_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyDoji_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: doji */
          Math.abs(sp.lag1_inClose - sp.lag1_inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (sp.BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyDoji_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyDoji_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
          Math.abs(inClose - inOpen) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (sp.BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyDoji_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyDoji_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) )
      {
         /* 3rd: doji */
         sp.cur_outInteger = 0;
         if( (Math.min(sp.lag1_inOpen, sp.lag1_inClose) > Math.max(sp.lag2_inOpen, sp.lag2_inClose)) && /* 2nd gaps up */
             Math.max(inOpen, inClose) < Math.max(sp.lag1_inOpen, sp.lag1_inClose) ) /* 3rd is not higher than 2nd */
         {
            sp.cur_outInteger = 0 - 100;
         }
         if( (Math.max(sp.lag1_inOpen, sp.lag1_inClose) < Math.min(sp.lag2_inOpen, sp.lag2_inClose)) && /* 2nd gaps down */
             Math.min(inOpen, inClose) > Math.min(sp.lag1_inOpen, sp.lag1_inClose) ) /* 3rd is not lower than 2nd */
         {
            sp.cur_outInteger = 100;
         }
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : ((BodyDoji_rangeType == 1) ? (sp.lag2_inHigh - sp.lag2_inLow) : ((BodyDoji_rangeType == 2) ? ((sp.lag2_inHigh - sp.lag2_inLow) - Math.abs(sp.lag2_inClose - sp.lag2_inOpen)) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx]) - Math.abs(sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx])) : 0.0)));
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
      sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx] = inOpen;
      sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] = inHigh;
      sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx] = inLow;
      sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] = inClose;
      sp.ringPos_BodyTrailingIdx = sp.ringPos_BodyTrailingIdx + 1;
      if( sp.ringPos_BodyTrailingIdx >= sp.ringCap_BodyTrailingIdx ) {
         sp.ringPos_BodyTrailingIdx = 0;
      }
   }
   private RetCode CDLTRISTAR_OpenBody( CDLTRISTAR_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      double BodyPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int lastValue_outInteger = 0;
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
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLTRISTAR_Lookback();
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
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - 2 - BodyDoji_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - 3 consecutive doji days
       * - the second doji is a star
       * The meaning of "doji" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish
       */
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 2] - inOpen[i - 2]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: doji */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
             Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            /* 3rd: doji */
            lastValue_outInteger = 0;
            if( (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && /* 2nd gaps up */
                Math.max(inOpen[i], inClose[i]) < Math.max(inOpen[i - 1], inClose[i - 1]) ) /* 3rd is not higher than 2nd */
            {
               lastValue_outInteger = 0 - 100;
            }
            if( (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && /* 2nd gaps down */
                Math.min(inOpen[i], inClose[i]) > Math.min(inOpen[i - 1], inClose[i - 1]) ) /* 3rd is not lower than 2nd */
            {
               lastValue_outInteger = 100;
            }
            outIdx += 1;
         } else {
            lastValue_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
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
      double[] capRing_BodyTrailingIdx_inOpen = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inOpen, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inHigh = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inHigh, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inLow = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inLow, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inClose = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inClose, 0, cap_BodyTrailingIdx);
      sp.BodyPeriodTotal = BodyPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_BodyTrailingIdx = 0;
      sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
      sp.ring_BodyTrailingIdx_inOpen = capRing_BodyTrailingIdx_inOpen;
      sp.ring_BodyTrailingIdx_inHigh = capRing_BodyTrailingIdx_inHigh;
      sp.ring_BodyTrailingIdx_inLow = capRing_BodyTrailingIdx_inLow;
      sp.ring_BodyTrailingIdx_inClose = capRing_BodyTrailingIdx_inClose;
      sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
      sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
      sp.cs_BodyDoji_factor = BodyDoji_factor;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode CDLTRISTAR_OpenAndFillBody( CDLTRISTAR_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      double BodyPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      int BodyDoji_rangeType = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].rangeType.ordinal();
      int BodyDoji_avgPeriod = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].avgPeriod;
      double BodyDoji_factor = this.candleSettings[CandleSettingType.BodyDoji.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLTRISTAR_Lookback();
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
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - 2 - BodyDoji_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx - 2 ) {
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyDoji_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - 3 consecutive doji days
       * - the second doji is a star
       * The meaning of "doji" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish
       */
      i = startIdx;
      outIdx = 0;
      do {
         if( Math.abs(inClose[i - 2] - inOpen[i - 2]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 1st: doji */
             Math.abs(inClose[i - 1] - inOpen[i - 1]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) && /* 2nd: doji */
             Math.abs(inClose[i] - inOpen[i]) <= ((BodyDoji_factor * (((BodyDoji_avgPeriod != 0) ? (BodyPeriodTotal / BodyDoji_avgPeriod) : ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0)))) / ((BodyDoji_rangeType == 2) ? 2.0 : 1.0)))) )
         {
            /* 3rd: doji */
            outInteger[outIdx] = 0;
            if( (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) && /* 2nd gaps up */
                Math.max(inOpen[i], inClose[i]) < Math.max(inOpen[i - 1], inClose[i - 1]) ) /* 3rd is not higher than 2nd */
            {
               outInteger[outIdx] = 0 - 100;
            }
            if( (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2])) && /* 2nd gaps down */
                Math.min(inOpen[i], inClose[i]) > Math.min(inOpen[i - 1], inClose[i - 1]) ) /* 3rd is not lower than 2nd */
            {
               outInteger[outIdx] = 100;
            }
            outIdx += 1;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[i - 2] - inOpen[i - 2])) : ((BodyDoji_rangeType == 1) ? (inHigh[i - 2] - inLow[i - 2]) : ((BodyDoji_rangeType == 2) ? ((inHigh[i - 2] - inLow[i - 2]) - Math.abs(inClose[i - 2] - inOpen[i - 2])) : 0.0))) - ((BodyDoji_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyDoji_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyDoji_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
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
      double[] capRing_BodyTrailingIdx_inOpen = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inOpen, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inOpen, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inHigh = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inHigh, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inHigh, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inLow = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inLow, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inLow, 0, cap_BodyTrailingIdx);
      double[] capRing_BodyTrailingIdx_inClose = new double[allocN_BodyTrailingIdx];
      System.arraycopy(inClose, historyLen - cap_BodyTrailingIdx, capRing_BodyTrailingIdx_inClose, 0, cap_BodyTrailingIdx);
      sp.BodyPeriodTotal = BodyPeriodTotal;
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.ringPos_BodyTrailingIdx = 0;
      sp.ringCap_BodyTrailingIdx = cap_BodyTrailingIdx;
      sp.ring_BodyTrailingIdx_inOpen = capRing_BodyTrailingIdx_inOpen;
      sp.ring_BodyTrailingIdx_inHigh = capRing_BodyTrailingIdx_inHigh;
      sp.ring_BodyTrailingIdx_inLow = capRing_BodyTrailingIdx_inLow;
      sp.ring_BodyTrailingIdx_inClose = capRing_BodyTrailingIdx_inClose;
      sp.cs_BodyDoji_rangeType = BodyDoji_rangeType;
      sp.cs_BodyDoji_avgPeriod = BodyDoji_avgPeriod;
      sp.cs_BodyDoji_factor = BodyDoji_factor;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind CDLTRISTAR_Open (composition seam). */
   CDLTRISTAR_Stream CDLTRISTAR_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLTRISTAR_Stream sp = new CDLTRISTAR_Stream(this);
      RetCode retCode = CDLTRISTAR_OpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLTRISTAR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLTRISTAR open: internal error");
      }
      throw new IllegalArgumentException("CDLTRISTAR open: " + retCode);
   }
   /**
    * Open a live CDLTRISTAR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLTRISTAR} at that bar.
    * <p>The history must hold at least {@code CDLTRISTAR_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLTRISTAR_Stream CDLTRISTAR_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLTRISTAR_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLTRISTAR_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLTRISTAR} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLTRISTAR_Stream#fillRange()}.
    */
   public CDLTRISTAR_Stream CDLTRISTAR_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLTRISTAR_Stream sp = new CDLTRISTAR_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLTRISTAR_OpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLTRISTAR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLTRISTAR openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLTRISTAR openAndFill: " + retCode);
   }
