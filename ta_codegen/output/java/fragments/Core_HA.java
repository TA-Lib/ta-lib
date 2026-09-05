/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090526 MF,CC  First version (issue #373).
 */

   /**
    * Number of leading input bars {@link Core#HA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int HA_Lookback( )
   {
      return this.unstablePeriod[FuncUnstId.HA.ordinal()] ;

   }
   RetCode HA_Impl( int startIdx,
                    int endIdx,
                    double inOpen[],
                    double inHigh[],
                    double inLow[],
                    double inClose[],
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outHAOpen[],
                    double outHAHigh[],
                    double outHALow[],
                    double outHAClose[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double haOpen = 0;
      double haClose = 0;
      double haHigh = 0;
      double haLow = 0;
      double tempOpen = 0;
      double tempHigh = 0;
      double tempLow = 0;
      double tempClose = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( outHAOpen == outHAHigh || outHAOpen == outHALow || outHAOpen == outHAClose || outHAHigh == outHALow || outHAHigh == outHAClose || outHALow == outHAClose ) {
         return RetCode.BadParam ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = HA_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* The summation order ((O+H)+L)+C is the bit-exactness contract with every
       * external implementation of this indicator. TA_AVGPRICE is documented as
       * the same quantity but sums ((H+L)+C)+O, which is a different double on
       * some bars; the two are deliberately not unified.
       */
      today = startIdx - lookbackTotal;
      haOpen = (inOpen[today] + inClose[today]) / 2.0;
      haClose = (inOpen[today] + inHigh[today] + inLow[today] + inClose[today]) / 4.0;
      today += 1;
      /* Skip the unstable period. */
      i = lookbackTotal;
      while( i != 0 ) {
         /* haOpen consumes the PREVIOUS candle on both sides of the midpoint, so
          * it must advance before haClose is overwritten. Swapping these two
          * still yields a plausible smoothed series.
          */
         haOpen = (haOpen + haClose) / 2.0;
         haClose = (inOpen[today] + inHigh[today] + inLow[today] + inClose[today]) / 4.0;
         today += 1;
         i -= 1;
      }
      tempHigh = inHigh[startIdx];
      tempLow = inLow[startIdx];
      /* The three-way extremum is spelled with plain comparisons, never the
       * max/min builtins: C's macros return their SECOND operand on a tie where
       * Rust, Java and .NET return the negative zero, so a bar of signed zeros
       * emits different bytes per backend and the cross-language gate compares
       * bytes. Measured on (O,H,L,C) = (-0.0, +0.0, -0.0, -0.0).
       */
      haHigh = tempHigh;
      if( haOpen > haHigh ) {
         haHigh = haOpen;
      }
      if( haClose > haHigh ) {
         haHigh = haClose;
      }
      haLow = tempLow;
      if( haOpen < haLow ) {
         haLow = haOpen;
      }
      if( haClose < haLow ) {
         haLow = haClose;
      }
      outHAOpen[0] = haOpen;
      outHAHigh[0] = haHigh;
      outHALow[0] = haLow;
      outHAClose[0] = haClose;
      outIdx = 1;
      while( today <= endIdx ) {
         /* Every price of the bar is read into a local before any output is
          * written, which is what lets an output alias any input: at startIdx 0
          * the store lands on the very slot the high and low were just read from.
          */
         tempOpen = inOpen[today];
         tempHigh = inHigh[today];
         tempLow = inLow[today];
         tempClose = inClose[today];
         haOpen = (haOpen + haClose) / 2.0;
         haClose = (tempOpen + tempHigh + tempLow + tempClose) / 4.0;
         haHigh = tempHigh;
         if( haOpen > haHigh ) {
            haHigh = haOpen;
         }
         if( haClose > haHigh ) {
            haHigh = haClose;
         }
         haLow = tempLow;
         if( haOpen < haLow ) {
            haLow = haOpen;
         }
         if( haClose < haLow ) {
            haLow = haClose;
         }
         outHAOpen[outIdx] = haOpen;
         outHAHigh[outIdx] = haHigh;
         outHALow[outIdx] = haLow;
         outHAClose[outIdx] = haClose;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode HA_Impl( int startIdx,
                    int endIdx,
                    float inOpen[],
                    float inHigh[],
                    float inLow[],
                    float inClose[],
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outHAOpen[],
                    double outHAHigh[],
                    double outHALow[],
                    double outHAClose[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double haOpen = 0;
      double haClose = 0;
      double haHigh = 0;
      double haLow = 0;
      double tempOpen = 0;
      double tempHigh = 0;
      double tempLow = 0;
      double tempClose = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( outHAOpen == outHAHigh || outHAOpen == outHALow || outHAOpen == outHAClose || outHAHigh == outHALow || outHAHigh == outHAClose || outHALow == outHAClose ) {
         return RetCode.BadParam ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = HA_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      haOpen = ((double)inOpen[today] + (double)inClose[today]) / 2.0;
      haClose = ((double)inOpen[today] + (double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 4.0;
      today += 1;
      i = lookbackTotal;
      while( i != 0 ) {
         haOpen = (haOpen + haClose) / 2.0;
         haClose = ((double)inOpen[today] + (double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 4.0;
         today += 1;
         i -= 1;
      }
      tempHigh = (double)inHigh[startIdx];
      tempLow = (double)inLow[startIdx];
      haHigh = tempHigh;
      if( haOpen > haHigh ) {
         haHigh = haOpen;
      }
      if( haClose > haHigh ) {
         haHigh = haClose;
      }
      haLow = tempLow;
      if( haOpen < haLow ) {
         haLow = haOpen;
      }
      if( haClose < haLow ) {
         haLow = haClose;
      }
      outHAOpen[0] = haOpen;
      outHAHigh[0] = haHigh;
      outHALow[0] = haLow;
      outHAClose[0] = haClose;
      outIdx = 1;
      while( today <= endIdx ) {
         tempOpen = (double)inOpen[today];
         tempHigh = (double)inHigh[today];
         tempLow = (double)inLow[today];
         tempClose = (double)inClose[today];
         haOpen = (haOpen + haClose) / 2.0;
         haClose = (tempOpen + tempHigh + tempLow + tempClose) / 4.0;
         haHigh = tempHigh;
         if( haOpen > haHigh ) {
            haHigh = haOpen;
         }
         if( haClose > haHigh ) {
            haHigh = haClose;
         }
         haLow = tempLow;
         if( haOpen < haLow ) {
            haLow = haOpen;
         }
         if( haClose < haLow ) {
            haLow = haClose;
         }
         outHAOpen[outIdx] = haOpen;
         outHAHigh[outIdx] = haHigh;
         outHALow[outIdx] = haLow;
         outHAClose[outIdx] = haClose;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Heikin-Ashi candles: an OHLC-to-OHLC transform that replaces each bar with
    * a smoothed synthetic candle. The synthetic close is the bar's four-price
    * average, the synthetic open is the midpoint of the previous synthetic
    * candle's own open and close, and the synthetic high and low extend the raw
    * extremes far enough to contain that pair. Read the result as a trend
    * filter rather than as price: consecutive candles of one colour run longer
    * than on the raw chart, small counter-trend bars are absorbed, and a candle
    * with no shadow on the trend side is the usual continuation cue. The cost
    * is that the synthetic open and close are not tradeable prices and gaps
    * disappear entirely, so orders must still be placed against the raw series.
    * {@code HA} is recursive: every candle carries the previous one, so the
    * first candle of a request is seeded from its own bar and its influence
    * halves on each bar that follows.
    * <p><b>Formula</b>
    * <pre>{@code
    * HA_close[i] = ( O[i] + H[i] + L[i] + C[i] ) / 4
    * HA_open[0]  = ( O[0] + C[0] ) / 2
    * HA_open[i]  = ( HA_open[i-1] + HA_close[i-1] ) / 2
    * HA_high[i]  = max( H[i], HA_open[i], HA_close[i] )
    * HA_low[i]   = min( L[i], HA_open[i], HA_close[i] )
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The first candle has no predecessor, so its open is seeded with the midpoint of the raw open and close. Other conventions exist — ta4j emits the raw bar unchanged as its first candle — and they differ only while the seed still carries weight.</li>
    * <li>Both divisors are exact powers of two, so implementations that scale by {@code 0.5} and {@code 0.25} produce the same doubles as those that divide by 2 and 4.</li>
    * <li>The unstable period discards that many candles of warm-up before the first output, trading history for a smaller residual difference between two requests that start at different bars.</li>
    * <li>Averaging four prices of one bar is also what [{@code AVGPRICE}](/functions/avgprice) computes, but it sums them in a different order, so the two can differ in the last bits.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#HA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outHAOpen Heikin-Ashi open. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outHAHigh Heikin-Ashi high. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outHALow Heikin-Ashi low. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outHAClose Heikin-Ashi close. Must hold at least
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
    * @see Core#AVGPRICE
    * @see Core#MEDPRICE
    * @see Core#TYPPRICE
    * @see Core#WCLPRICE
    */
   public OutRange HA( int startIdx,
                       int endIdx,
                       double inOpen[],
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       double outHAOpen[],
                       double outHAHigh[],
                       double outHALow[],
                       double outHAClose[] )
   {
      requireIndexRange("HA", startIdx, endIdx);
      int guardStart = clampedStart("HA", startIdx, HA_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("HA", "inOpen", inOpen, guardInLen);
      requireLength("HA", "inHigh", inHigh, guardInLen);
      requireLength("HA", "inLow", inLow, guardInLen);
      requireLength("HA", "inClose", inClose, guardInLen);
      requireLength("HA", "outHAOpen", outHAOpen, guardOutLen);
      requireLength("HA", "outHAHigh", outHAHigh, guardOutLen);
      requireLength("HA", "outHALow", outHALow, guardOutLen);
      requireLength("HA", "outHAClose", outHAClose, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = HA_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outHAOpen, outHAHigh, outHALow, outHAClose);
      if( retCode != RetCode.Success ) {
         throw failure("HA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Heikin-Ashi candles: an OHLC-to-OHLC transform that replaces each bar with
    * a smoothed synthetic candle. The synthetic close is the bar's four-price
    * average, the synthetic open is the midpoint of the previous synthetic
    * candle's own open and close, and the synthetic high and low extend the raw
    * extremes far enough to contain that pair. Read the result as a trend
    * filter rather than as price: consecutive candles of one colour run longer
    * than on the raw chart, small counter-trend bars are absorbed, and a candle
    * with no shadow on the trend side is the usual continuation cue. The cost
    * is that the synthetic open and close are not tradeable prices and gaps
    * disappear entirely, so orders must still be placed against the raw series.
    * {@code HA} is recursive: every candle carries the previous one, so the
    * first candle of a request is seeded from its own bar and its influence
    * halves on each bar that follows.
    * <p><b>Formula</b>
    * <pre>{@code
    * HA_close[i] = ( O[i] + H[i] + L[i] + C[i] ) / 4
    * HA_open[0]  = ( O[0] + C[0] ) / 2
    * HA_open[i]  = ( HA_open[i-1] + HA_close[i-1] ) / 2
    * HA_high[i]  = max( H[i], HA_open[i], HA_close[i] )
    * HA_low[i]   = min( L[i], HA_open[i], HA_close[i] )
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The first candle has no predecessor, so its open is seeded with the midpoint of the raw open and close. Other conventions exist — ta4j emits the raw bar unchanged as its first candle — and they differ only while the seed still carries weight.</li>
    * <li>Both divisors are exact powers of two, so implementations that scale by {@code 0.5} and {@code 0.25} produce the same doubles as those that divide by 2 and 4.</li>
    * <li>The unstable period discards that many candles of warm-up before the first output, trading history for a smaller residual difference between two requests that start at different bars.</li>
    * <li>Averaging four prices of one bar is also what [{@code AVGPRICE}](/functions/avgprice) computes, but it sums them in a different order, so the two can differ in the last bits.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#HA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outHAOpen Heikin-Ashi open. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outHAHigh Heikin-Ashi high. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outHALow Heikin-Ashi low. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outHAClose Heikin-Ashi close. Must hold at least
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
    * @see Core#AVGPRICE
    * @see Core#MEDPRICE
    * @see Core#TYPPRICE
    * @see Core#WCLPRICE
    */
   public OutRange HA( int startIdx,
                       int endIdx,
                       float inOpen[],
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       double outHAOpen[],
                       double outHAHigh[],
                       double outHALow[],
                       double outHAClose[] )
   {
      requireIndexRange("HA", startIdx, endIdx);
      int guardStart = clampedStart("HA", startIdx, HA_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("HA", "inOpen", inOpen, guardInLen);
      requireLength("HA", "inHigh", inHigh, guardInLen);
      requireLength("HA", "inLow", inLow, guardInLen);
      requireLength("HA", "inClose", inClose, guardInLen);
      requireLength("HA", "outHAOpen", outHAOpen, guardOutLen);
      requireLength("HA", "outHAHigh", outHAHigh, guardOutLen);
      requireLength("HA", "outHALow", outHALow, guardOutLen);
      requireLength("HA", "outHAClose", outHAClose, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = HA_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outHAOpen, outHAHigh, outHALow, outHAClose);
      if( retCode != RetCode.Success ) {
         throw failure("HA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live HA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#HA} over the same series.
    * Open with {@link Core#haOpen}; there is no close — the handle is
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
   public static final class HaStream {
      Core core;
      double haOpen;
      double haClose;
      double cur_outHAOpen;
      double cur_outHAHigh;
      double cur_outHALow;
      double cur_outHAClose;
      int outRangeBegIdx;
      int outRangeCount;

      HaStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#HA} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      HaStream( HaStream other ) {
         this.core = other.core;
         this.haOpen = other.haOpen;
         this.haClose = other.haClose;
         this.cur_outHAOpen = other.cur_outHAOpen;
         this.cur_outHAHigh = other.cur_outHAHigh;
         this.cur_outHALow = other.cur_outHALow;
         this.cur_outHAClose = other.cur_outHAClose;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(HaOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inOpen, double inHigh, double inLow, double inClose, HaOut out ) {
         requireArgument("HA update", "out", out);
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("HA update: BadParam", RetCode.BadParam);
         }
         core.haStepImpl(this, inOpen, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.haOpen = this.cur_outHAOpen;
         out.haHigh = this.cur_outHAHigh;
         out.haLow = this.cur_outHALow;
         out.haClose = this.cur_outHAClose;
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
      public void updateAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], double outHAOpen[], double outHAHigh[], double outHALow[], double outHAClose[] ) {
         requireArgument("HA updateAndFill", "inOpen", inOpen);
         requireArgument("HA updateAndFill", "inHigh", inHigh);
         requireArgument("HA updateAndFill", "inLow", inLow);
         requireArgument("HA updateAndFill", "inClose", inClose);
         requireArgument("HA updateAndFill", "outHAOpen", outHAOpen);
         requireArgument("HA updateAndFill", "outHAHigh", outHAHigh);
         requireArgument("HA updateAndFill", "outHALow", outHALow);
         requireArgument("HA updateAndFill", "outHAClose", outHAClose);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outHAOpen.length < barCount || outHAHigh.length < barCount || outHALow.length < barCount || outHAClose.length < barCount || (Object)outHAOpen == (Object)inOpen || (Object)outHAOpen == (Object)inHigh || (Object)outHAOpen == (Object)inLow || (Object)outHAOpen == (Object)inClose || (Object)outHAHigh == (Object)inOpen || (Object)outHAHigh == (Object)inHigh || (Object)outHAHigh == (Object)inLow || (Object)outHAHigh == (Object)inClose || (Object)outHALow == (Object)inOpen || (Object)outHALow == (Object)inHigh || (Object)outHALow == (Object)inLow || (Object)outHALow == (Object)inClose || (Object)outHAClose == (Object)inOpen || (Object)outHAClose == (Object)inHigh || (Object)outHAClose == (Object)inLow || (Object)outHAClose == (Object)inClose || (Object)outHAOpen == (Object)outHAHigh || (Object)outHAOpen == (Object)outHALow || (Object)outHAOpen == (Object)outHAClose || (Object)outHAHigh == (Object)outHALow || (Object)outHAHigh == (Object)outHAClose || (Object)outHALow == (Object)outHAClose )
            throw new TaLibArgumentException("HA updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("HA updateAndFill: BadParam", RetCode.BadParam);
            }
            core.haStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
            outHAOpen[i] = this.cur_outHAOpen;
            outHAHigh[i] = this.cur_outHAHigh;
            outHALow[i] = this.cur_outHALow;
            outHAClose[i] = this.cur_outHAClose;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public void peek( double inOpen, double inHigh, double inLow, double inClose, HaOut out ) {
         requireArgument("HA peek", "out", out);
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("HA peek: BadParam", RetCode.BadParam);
         HaStream sp = this;
         double haHigh = 0.0;
         double haLow = 0.0;
         double tempOpen = 0.0;
         double tempHigh = 0.0;
         double tempLow = 0.0;
         double tempClose = 0.0;
         double cur_outHAClose = 0.0;
         double cur_outHAHigh = 0.0;
         double cur_outHALow = 0.0;
         double cur_outHAOpen = 0.0;
         double haClose = sp.haClose;
         double haOpen = sp.haOpen;
         /* Every price of the bar is read into a local before any output is
          * written, which is what lets an output alias any input: at startIdx 0
          * the store lands on the very slot the high and low were just read from.
          */
         tempOpen = inOpen;
         tempHigh = inHigh;
         tempLow = inLow;
         tempClose = inClose;
         haOpen = (haOpen + haClose) / 2.0;
         haClose = (tempOpen + tempHigh + tempLow + tempClose) / 4.0;
         haHigh = tempHigh;
         if( haOpen > haHigh ) {
            haHigh = haOpen;
         }
         if( haClose > haHigh ) {
            haHigh = haClose;
         }
         haLow = tempLow;
         if( haOpen < haLow ) {
            haLow = haOpen;
         }
         if( haClose < haLow ) {
            haLow = haClose;
         }
         cur_outHAOpen = haOpen;
         cur_outHAHigh = haHigh;
         cur_outHALow = haLow;
         cur_outHAClose = haClose;
         out.haOpen = cur_outHAOpen;
         out.haHigh = cur_outHAHigh;
         out.haLow = cur_outHALow;
         out.haClose = cur_outHAClose;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( HaOut out ) {
         requireArgument("HA value", "out", out);
         out.haOpen = this.cur_outHAOpen;
         out.haHigh = this.cur_outHAHigh;
         out.haLow = this.cur_outHALow;
         out.haClose = this.cur_outHAClose;
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
      public HaStream clone() {
         return new HaStream(this);
      }
   }

   /**
    * The outputs of one HA bar, written by the stream into an object the
    * CALLER owns. Allocate one and reuse it: {@code update}, {@code peek}
    * and {@code value} overwrite its fields, so the sink itself costs
    * nothing per bar.
    *
    * <p><b>Its contents are only valid until the next call that writes it.</b>
    * It is a mutable buffer, not a reading: a reference kept past that call,
    * or one put in a collection, sees the value change underneath it. Copy the
    * fields out if the reading has to outlive the call.
    *
    * <p>Deliberately no {@code equals} or {@code hashCode}: a mutable type
    * with value equality breaks the {@code HashMap}/{@code HashSet}
    * invariant the moment a reused instance becomes a key. Compare the fields.
    */
   public static final class HaOut {
      /** Heikin-Ashi open. */
      public double haOpen;
      /** Heikin-Ashi high. */
      public double haHigh;
      /** Heikin-Ashi low. */
      public double haLow;
      /** Heikin-Ashi close. */
      public double haClose;
   }
   void haStepImpl( HaStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      double haHigh = 0.0;
      double haLow = 0.0;
      double tempOpen = 0.0;
      double tempHigh = 0.0;
      double tempLow = 0.0;
      double tempClose = 0.0;
      /* Every price of the bar is read into a local before any output is
       * written, which is what lets an output alias any input: at startIdx 0
       * the store lands on the very slot the high and low were just read from.
       */
      tempOpen = inOpen;
      tempHigh = inHigh;
      tempLow = inLow;
      tempClose = inClose;
      sp.haOpen = (sp.haOpen + sp.haClose) / 2.0;
      sp.haClose = (tempOpen + tempHigh + tempLow + tempClose) / 4.0;
      haHigh = tempHigh;
      if( sp.haOpen > haHigh ) {
         haHigh = sp.haOpen;
      }
      if( sp.haClose > haHigh ) {
         haHigh = sp.haClose;
      }
      haLow = tempLow;
      if( sp.haOpen < haLow ) {
         haLow = sp.haOpen;
      }
      if( sp.haClose < haLow ) {
         haLow = sp.haClose;
      }
      sp.cur_outHAOpen = sp.haOpen;
      sp.cur_outHAHigh = haHigh;
      sp.cur_outHALow = haLow;
      sp.cur_outHAClose = sp.haClose;
   }
   private RetCode haOpenImpl( HaStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outHAOpen[], double outHAHigh[], double outHALow[], double outHAClose[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double haOpen = 0;
      double haClose = 0;
      double haHigh = 0;
      double haLow = 0;
      double tempOpen = 0;
      double tempHigh = 0;
      double tempLow = 0;
      double tempClose = 0;
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
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = HA_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      /* The summation order ((O+H)+L)+C is the bit-exactness contract with every
       * external implementation of this indicator. TA_AVGPRICE is documented as
       * the same quantity but sums ((H+L)+C)+O, which is a different double on
       * some bars; the two are deliberately not unified.
       */
      today = startIdx - lookbackTotal;
      haOpen = (inOpen[today] + inClose[today]) / 2.0;
      haClose = (inOpen[today] + inHigh[today] + inLow[today] + inClose[today]) / 4.0;
      today += 1;
      /* Skip the unstable period. */
      i = lookbackTotal;
      while( i != 0 ) {
         /* haOpen consumes the PREVIOUS candle on both sides of the midpoint, so
          * it must advance before haClose is overwritten. Swapping these two
          * still yields a plausible smoothed series.
          */
         haOpen = (haOpen + haClose) / 2.0;
         haClose = (inOpen[today] + inHigh[today] + inLow[today] + inClose[today]) / 4.0;
         today += 1;
         i -= 1;
      }
      tempHigh = inHigh[startIdx];
      tempLow = inLow[startIdx];
      /* The three-way extremum is spelled with plain comparisons, never the
       * max/min builtins: C's macros return their SECOND operand on a tie where
       * Rust, Java and .NET return the negative zero, so a bar of signed zeros
       * emits different bytes per backend and the cross-language gate compares
       * bytes. Measured on (O,H,L,C) = (-0.0, +0.0, -0.0, -0.0).
       */
      haHigh = tempHigh;
      if( haOpen > haHigh ) {
         haHigh = haOpen;
      }
      if( haClose > haHigh ) {
         haHigh = haClose;
      }
      haLow = tempLow;
      if( haOpen < haLow ) {
         haLow = haOpen;
      }
      if( haClose < haLow ) {
         haLow = haClose;
      }
      outHAOpen[0 * outStride] = haOpen;
      outHAHigh[0 * outStride] = haHigh;
      outHALow[0 * outStride] = haLow;
      outHAClose[0 * outStride] = haClose;
      outIdx = 1;
      while( today <= endIdx ) {
         /* Every price of the bar is read into a local before any output is
          * written, which is what lets an output alias any input: at startIdx 0
          * the store lands on the very slot the high and low were just read from.
          */
         tempOpen = inOpen[today];
         tempHigh = inHigh[today];
         tempLow = inLow[today];
         tempClose = inClose[today];
         haOpen = (haOpen + haClose) / 2.0;
         haClose = (tempOpen + tempHigh + tempLow + tempClose) / 4.0;
         haHigh = tempHigh;
         if( haOpen > haHigh ) {
            haHigh = haOpen;
         }
         if( haClose > haHigh ) {
            haHigh = haClose;
         }
         haLow = tempLow;
         if( haOpen < haLow ) {
            haLow = haOpen;
         }
         if( haClose < haLow ) {
            haLow = haClose;
         }
         outHAOpen[outIdx * outStride] = haOpen;
         outHAHigh[outIdx * outStride] = haHigh;
         outHALow[outIdx * outStride] = haLow;
         outHAClose[outIdx * outStride] = haClose;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.haOpen = haOpen;
      sp.haClose = haClose;
      sp.cur_outHAOpen = outHAOpen[(outNBElement.value - 1) * outStride];
      sp.cur_outHAHigh = outHAHigh[(outNBElement.value - 1) * outStride];
      sp.cur_outHALow = outHALow[(outNBElement.value - 1) * outStride];
      sp.cur_outHAClose = outHAClose[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* haOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   HaStream haOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outHAOpen[], double outHAHigh[], double outHALow[], double outHAClose[] )
   {
      HaStream sp = new HaStream(this);
      RetCode retCode = haOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outHAOpen, outHAHigh, outHALow, outHAClose, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("HA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("HA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("HA openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind haOpen (composition seam). */
   HaStream haOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      HaStream sp = new HaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outHAOpen = new double[1];
      double[] sink_outHAHigh = new double[1];
      double[] sink_outHALow = new double[1];
      double[] sink_outHAClose = new double[1];
      RetCode retCode = haOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outHAOpen, sink_outHAHigh, sink_outHALow, sink_outHAClose, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("HA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("HA open: internal error", retCode);
      }
      throw new TaLibArgumentException("HA open: " + retCode, retCode);
   }
   /**
    * Open a live HA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#HA} at that bar.
    * <p>The history must hold at least {@code HA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public HaStream haOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("HA open", "inOpen", inOpen);
      requireHistory("HA open", inOpen.length);
      requireArgument("HA open", "inHigh", inHigh);
      requireArgument("HA open", "inLow", inLow);
      requireArgument("HA open", "inClose", inClose);
      requireHistoryLength("HA open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("HA open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("HA open", "inClose", inClose.length, inOpen.length);
      return haOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#haOpen} that also fills the output array(s) bit-identically
    * to {@link Core#HA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link HaStream#outRange()}.
    */
   public HaStream haOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], double outHAOpen[], double outHAHigh[], double outHALow[], double outHAClose[] )
   {
      requireArgument("HA openAndFill", "inOpen", inOpen);
      requireHistory("HA openAndFill", inOpen.length);
      requireArgument("HA openAndFill", "inHigh", inHigh);
      requireArgument("HA openAndFill", "inLow", inLow);
      requireArgument("HA openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("HA openAndFill", inOpen.length, HA_Lookback());
      requireHistoryLength("HA openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("HA openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("HA openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("HA openAndFill", "outHAOpen", outHAOpen, guardOutLen);
      requireLength("HA openAndFill", "outHAHigh", outHAHigh, guardOutLen);
      requireLength("HA openAndFill", "outHALow", outHALow, guardOutLen);
      requireLength("HA openAndFill", "outHAClose", outHAClose, guardOutLen);
      if( (Object)outHAOpen == (Object)inOpen || (Object)outHAOpen == (Object)inHigh || (Object)outHAOpen == (Object)inLow || (Object)outHAOpen == (Object)inClose || (Object)outHAHigh == (Object)inOpen || (Object)outHAHigh == (Object)inHigh || (Object)outHAHigh == (Object)inLow || (Object)outHAHigh == (Object)inClose || (Object)outHALow == (Object)inOpen || (Object)outHALow == (Object)inHigh || (Object)outHALow == (Object)inLow || (Object)outHALow == (Object)inClose || (Object)outHAClose == (Object)inOpen || (Object)outHAClose == (Object)inHigh || (Object)outHAClose == (Object)inLow || (Object)outHAClose == (Object)inClose || (Object)outHAOpen == (Object)outHAHigh || (Object)outHAOpen == (Object)outHALow || (Object)outHAOpen == (Object)outHAClose || (Object)outHAHigh == (Object)outHALow || (Object)outHAHigh == (Object)outHAClose || (Object)outHALow == (Object)outHAClose ) {
         throw new TaLibArgumentException("HA openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return haOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outHAOpen, outHAHigh, outHALow, outHAClose);
   }
