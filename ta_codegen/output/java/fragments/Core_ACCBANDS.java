/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  RM       Robert Meier
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120307 RM     Initial Version
 *  120907 MF     Handling of a few limit cases
 *  071226 MF,CC  Fused single-loop rewrite: maintain the three band running
 *                sums (close for the middle band; the pointwise High/Low map for
 *                the upper/lower bands) over one shared trailing window, instead
 *                of two scratch buffers + three sma() calls. Enables streaming
 *                and is bit-identical to the prior three-SMA form (verified vs
 *                v0.6.4).
 *  082326 MF,CC  Fix #253. Scale the High+Low cancellation test to its own
 *                operands instead of the fixed TA_IS_ZERO band, which widened
 *                the bands of any instrument quoted small enough to fall
 *                under it.
 */

   /**
    * Number of leading input bars {@link Core#ACCBANDS} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod SMA smoothing period for all three bands (default
    *        20; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ACCBANDS_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return SMA_Lookback(optInTimePeriod) ;

   }
   RetCode ACCBANDS_Impl( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          double inClose[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outRealUpperBand[],
                          double outRealMiddleBand[],
                          double outRealLowerBand[] )
   {
      double periodTotalUpper = 0;
      double periodTotalMiddle = 0;
      double periodTotalLower = 0;
      double tempUpper = 0;
      double tempMiddle = 0;
      double tempLower = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = SMA_Lookback(optInTimePeriod);
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
      /* Each band is a simple moving average maintained as a running sum over a
       * shared trailing window (all three share optInTimePeriod, so one trailing
       * index walks all three windows in lockstep):
       *    middle = SMA( close )
       *    upper  = SMA( high * (1 + 4*(high-low)/(high+low)) )
       *    lower  = SMA( low  * (1 - 4*(high-low)/(high+low)) )
       * When high+low is zero the upper/lower map degenerates to high/low.
       * Fusing the three moving averages into one loop is bit-identical to the
       * former "two scratch buffers + three sma() calls": each accumulator's
       * add/record/subtract order is unchanged, and the High/Low map is a pure
       * function recomputed from the raw trailing bar.
       */
      periodTotalUpper = 0.0;
      periodTotalMiddle = 0.0;
      periodTotalLower = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      /* Warm up the running sums with the initial period,
       * except for the last value.
       */
      i = trailingIdx;
      while( i < startIdx ) {
         /* The band factor 4*(H-L)/(H+L) is a ratio of two prices, so it is
          * scale-free -- but H+L is a sum that CANCELS when the two prices have
          * opposite signs, and the factor then blows up on what is left of the
          * operands' last bits. Test the sum against ITS OWN operands, not against
          * a fixed band: an absolute threshold answers "cancelled" for every bar
          * of an instrument quoted small enough to fall under it, and widened
          * every band it touched (issue #253). Same test on all three sites, so
          * the bar that enters a running sum is the one that later leaves it.
          */
         tempReal = inHigh[i] + inLow[i];
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i]))) ) {
            tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
            periodTotalUpper += inHigh[i] * (1 + tempReal);
            periodTotalLower += inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += inHigh[i];
            periodTotalLower += inLow[i];
         }
         periodTotalMiddle += inClose[i];
         i = i + 1;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and output to be the
       * same buffer: every trailing bar is read before any output is written.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         /* Add the incoming bar to each running sum. */
         tempReal = inHigh[i] + inLow[i];
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i]))) ) {
            tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
            periodTotalUpper += inHigh[i] * (1 + tempReal);
            periodTotalLower += inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += inHigh[i];
            periodTotalLower += inLow[i];
         }
         periodTotalMiddle += inClose[i];
         i = i + 1;
         /* Record the current window sums. */
         tempUpper = periodTotalUpper;
         tempMiddle = periodTotalMiddle;
         tempLower = periodTotalLower;
         /* Remove the trailing bar from each running sum. */
         tempReal = inHigh[trailingIdx] + inLow[trailingIdx];
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[trailingIdx]) + Math.abs(inLow[trailingIdx]))) ) {
            tempReal = 4 * (inHigh[trailingIdx] - inLow[trailingIdx]) / tempReal;
            periodTotalUpper -= inHigh[trailingIdx] * (1 + tempReal);
            periodTotalLower -= inLow[trailingIdx] * (1 - tempReal);
         } else {
            periodTotalUpper -= inHigh[trailingIdx];
            periodTotalLower -= inLow[trailingIdx];
         }
         periodTotalMiddle -= inClose[trailingIdx];
         trailingIdx = trailingIdx + 1;
         /* Write the three bands. */
         outRealUpperBand[outIdx] = tempUpper / (double)optInTimePeriod;
         outRealMiddleBand[outIdx] = tempMiddle / (double)optInTimePeriod;
         outRealLowerBand[outIdx] = tempLower / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode ACCBANDS_Impl( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          float inClose[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outRealUpperBand[],
                          double outRealMiddleBand[],
                          double outRealLowerBand[] )
   {
      double periodTotalUpper = 0;
      double periodTotalMiddle = 0;
      double periodTotalLower = 0;
      double tempUpper = 0;
      double tempMiddle = 0;
      double tempLower = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      lookbackTotal = SMA_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotalUpper = 0.0;
      periodTotalMiddle = 0.0;
      periodTotalLower = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      while( i < startIdx ) {
         tempReal = (double)inHigh[i] + (double)inLow[i];
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs((double)inHigh[i]) + Math.abs((double)inLow[i]))) ) {
            tempReal = 4 * ((double)inHigh[i] - (double)inLow[i]) / tempReal;
            periodTotalUpper += (double)inHigh[i] * (1 + tempReal);
            periodTotalLower += (double)inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += (double)inHigh[i];
            periodTotalLower += (double)inLow[i];
         }
         periodTotalMiddle += (double)inClose[i];
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = (double)inHigh[i] + (double)inLow[i];
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs((double)inHigh[i]) + Math.abs((double)inLow[i]))) ) {
            tempReal = 4 * ((double)inHigh[i] - (double)inLow[i]) / tempReal;
            periodTotalUpper += (double)inHigh[i] * (1 + tempReal);
            periodTotalLower += (double)inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += (double)inHigh[i];
            periodTotalLower += (double)inLow[i];
         }
         periodTotalMiddle += (double)inClose[i];
         i = i + 1;
         tempUpper = periodTotalUpper;
         tempMiddle = periodTotalMiddle;
         tempLower = periodTotalLower;
         tempReal = (double)inHigh[trailingIdx] + (double)inLow[trailingIdx];
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs((double)inHigh[trailingIdx]) + Math.abs((double)inLow[trailingIdx]))) ) {
            tempReal = 4 * ((double)inHigh[trailingIdx] - (double)inLow[trailingIdx]) / tempReal;
            periodTotalUpper -= (double)inHigh[trailingIdx] * (1 + tempReal);
            periodTotalLower -= (double)inLow[trailingIdx] * (1 - tempReal);
         } else {
            periodTotalUpper -= (double)inHigh[trailingIdx];
            periodTotalLower -= (double)inLow[trailingIdx];
         }
         periodTotalMiddle -= (double)inClose[trailingIdx];
         trailingIdx = trailingIdx + 1;
         outRealUpperBand[outIdx] = tempUpper / (double)optInTimePeriod;
         outRealMiddleBand[outIdx] = tempMiddle / (double)optInTimePeriod;
         outRealLowerBand[outIdx] = tempLower / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Acceleration Bands: three overlap lines around price. The middle band is
    * an SMA of the close; the upper/lower bands are SMAs of the high/low scaled
    * by an intraday-range factor.
    * <p><b>Formula</b>
    * <pre>{@code
    * factor = 4*(H-L)/(H+L)
    * upperRaw = H*(1+factor), lowerRaw = L*(1-factor)
    * Upper = SMA(upperRaw, N), Middle = SMA(Close, N), Lower = SMA(lowerRaw, N)
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ACCBANDS_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod SMA smoothing period for all three bands (default
    *        20; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outRealUpperBand SMA of the range-scaled high band. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @param outRealMiddleBand SMA of the close. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outRealLowerBand SMA of the range-scaled low band. Must hold at
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
    * @see Core#SMA
    * @see Core#BBANDS
    */
   public OutRange ACCBANDS( int startIdx,
                             int endIdx,
                             double inHigh[],
                             double inLow[],
                             double inClose[],
                             int optInTimePeriod,
                             double outRealUpperBand[],
                             double outRealMiddleBand[],
                             double outRealLowerBand[] )
   {
      requireIndexRange("ACCBANDS", startIdx, endIdx);
      int guardStart = clampedStart("ACCBANDS", startIdx, ACCBANDS_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ACCBANDS", "inHigh", inHigh, guardInLen);
      requireLength("ACCBANDS", "inLow", inLow, guardInLen);
      requireLength("ACCBANDS", "inClose", inClose, guardInLen);
      requireLength("ACCBANDS", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("ACCBANDS", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("ACCBANDS", "outRealLowerBand", outRealLowerBand, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ACCBANDS_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode != RetCode.Success ) {
         throw failure("ACCBANDS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Acceleration Bands: three overlap lines around price. The middle band is
    * an SMA of the close; the upper/lower bands are SMAs of the high/low scaled
    * by an intraday-range factor.
    * <p><b>Formula</b>
    * <pre>{@code
    * factor = 4*(H-L)/(H+L)
    * upperRaw = H*(1+factor), lowerRaw = L*(1-factor)
    * Upper = SMA(upperRaw, N), Middle = SMA(Close, N), Lower = SMA(lowerRaw, N)
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ACCBANDS_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod SMA smoothing period for all three bands (default
    *        20; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outRealUpperBand SMA of the range-scaled high band. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @param outRealMiddleBand SMA of the close. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outRealLowerBand SMA of the range-scaled low band. Must hold at
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
    * @see Core#SMA
    * @see Core#BBANDS
    */
   public OutRange ACCBANDS( int startIdx,
                             int endIdx,
                             float inHigh[],
                             float inLow[],
                             float inClose[],
                             int optInTimePeriod,
                             double outRealUpperBand[],
                             double outRealMiddleBand[],
                             double outRealLowerBand[] )
   {
      requireIndexRange("ACCBANDS", startIdx, endIdx);
      int guardStart = clampedStart("ACCBANDS", startIdx, ACCBANDS_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ACCBANDS", "inHigh", inHigh, guardInLen);
      requireLength("ACCBANDS", "inLow", inLow, guardInLen);
      requireLength("ACCBANDS", "inClose", inClose, guardInLen);
      requireLength("ACCBANDS", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("ACCBANDS", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("ACCBANDS", "outRealLowerBand", outRealLowerBand, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ACCBANDS_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode != RetCode.Success ) {
         throw failure("ACCBANDS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ACCBANDS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ACCBANDS} over the same series.
    * Open with {@link Core#accbandsOpen}; there is no close — the handle is
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
   public static final class AccbandsStream {
      Core core;
      int optInTimePeriod;
      double periodTotalUpper;
      double periodTotalMiddle;
      double periodTotalLower;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inHigh;
      double[] ring_trailingIdx_inLow;
      double[] ring_trailingIdx_inClose;
      double cur_outRealUpperBand;
      double cur_outRealMiddleBand;
      double cur_outRealLowerBand;
      int outRangeBegIdx;
      int outRangeCount;

      AccbandsStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ACCBANDS} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      AccbandsStream( AccbandsStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.periodTotalUpper = other.periodTotalUpper;
         this.periodTotalMiddle = other.periodTotalMiddle;
         this.periodTotalLower = other.periodTotalLower;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inHigh = other.ring_trailingIdx_inHigh.clone();
         this.ring_trailingIdx_inLow = other.ring_trailingIdx_inLow.clone();
         this.ring_trailingIdx_inClose = other.ring_trailingIdx_inClose.clone();
         this.cur_outRealUpperBand = other.cur_outRealUpperBand;
         this.cur_outRealMiddleBand = other.cur_outRealMiddleBand;
         this.cur_outRealLowerBand = other.cur_outRealLowerBand;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(AccbandsOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inHigh, double inLow, double inClose, AccbandsOut out ) {
         requireArgument("ACCBANDS update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("ACCBANDS update: BadParam", RetCode.BadParam);
         }
         core.accbandsStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.realUpperBand = this.cur_outRealUpperBand;
         out.realMiddleBand = this.cur_outRealMiddleBand;
         out.realLowerBand = this.cur_outRealLowerBand;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] ) {
         requireArgument("ACCBANDS updateAndFill", "inHigh", inHigh);
         requireArgument("ACCBANDS updateAndFill", "inLow", inLow);
         requireArgument("ACCBANDS updateAndFill", "inClose", inClose);
         requireArgument("ACCBANDS updateAndFill", "outRealUpperBand", outRealUpperBand);
         requireArgument("ACCBANDS updateAndFill", "outRealMiddleBand", outRealMiddleBand);
         requireArgument("ACCBANDS updateAndFill", "outRealLowerBand", outRealLowerBand);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outRealUpperBand.length < barCount || outRealMiddleBand.length < barCount || outRealLowerBand.length < barCount || (Object)outRealUpperBand == (Object)inHigh || (Object)outRealUpperBand == (Object)inLow || (Object)outRealUpperBand == (Object)inClose || (Object)outRealMiddleBand == (Object)inHigh || (Object)outRealMiddleBand == (Object)inLow || (Object)outRealMiddleBand == (Object)inClose || (Object)outRealLowerBand == (Object)inHigh || (Object)outRealLowerBand == (Object)inLow || (Object)outRealLowerBand == (Object)inClose || (Object)outRealUpperBand == (Object)outRealMiddleBand || (Object)outRealUpperBand == (Object)outRealLowerBand || (Object)outRealMiddleBand == (Object)outRealLowerBand )
            throw new TaLibArgumentException("ACCBANDS updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("ACCBANDS updateAndFill: BadParam", RetCode.BadParam);
            }
            core.accbandsStepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outRealUpperBand[i] = this.cur_outRealUpperBand;
            outRealMiddleBand[i] = this.cur_outRealMiddleBand;
            outRealLowerBand[i] = this.cur_outRealLowerBand;
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
      public void peek( double inHigh, double inLow, double inClose, AccbandsOut out ) {
         requireArgument("ACCBANDS peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("ACCBANDS peek: BadParam", RetCode.BadParam);
         AccbandsStream sp = this;
         double tempUpper = 0.0;
         double tempMiddle = 0.0;
         double tempLower = 0.0;
         double tempReal = 0.0;
         double cur_outRealLowerBand = sp.cur_outRealLowerBand;
         double cur_outRealMiddleBand = sp.cur_outRealMiddleBand;
         double cur_outRealUpperBand = sp.cur_outRealUpperBand;
         double periodTotalLower = sp.periodTotalLower;
         double periodTotalMiddle = sp.periodTotalMiddle;
         double periodTotalUpper = sp.periodTotalUpper;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         int pkSlot2 = -1;
         double pkVal2 = 0.0;
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inHigh;
            pkSlot1 = 0;
            pkVal1 = inLow;
            pkSlot2 = 0;
            pkVal2 = inClose;
         }
         /* Add the incoming bar to each running sum. */
         tempReal = inHigh + inLow;
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh) + Math.abs(inLow))) ) {
            tempReal = 4 * (inHigh - inLow) / tempReal;
            periodTotalUpper += inHigh * (1 + tempReal);
            periodTotalLower += inLow * (1 - tempReal);
         } else {
            periodTotalUpper += inHigh;
            periodTotalLower += inLow;
         }
         periodTotalMiddle += inClose;
         /* Record the current window sums. */
         tempUpper = periodTotalUpper;
         tempMiddle = periodTotalMiddle;
         tempLower = periodTotalLower;
         /* Remove the trailing bar from each running sum. */
         tempReal = ((sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] : pkVal0) + ((sp.ringPos_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] : pkVal1);
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs((sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] : pkVal0) + Math.abs((sp.ringPos_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] : pkVal1))) ) {
            tempReal = 4 * (((sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] : pkVal0) - ((sp.ringPos_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] : pkVal1)) / tempReal;
            periodTotalUpper -= ((sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] : pkVal0) * (1 + tempReal);
            periodTotalLower -= ((sp.ringPos_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] : pkVal1) * (1 - tempReal);
         } else {
            periodTotalUpper -= (sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] : pkVal0;
            periodTotalLower -= (sp.ringPos_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] : pkVal1;
         }
         periodTotalMiddle -= (sp.ringPos_trailingIdx != pkSlot2) ? sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx] : pkVal2;
         /* Write the three bands. */
         cur_outRealUpperBand = tempUpper / (double)sp.optInTimePeriod;
         cur_outRealMiddleBand = tempMiddle / (double)sp.optInTimePeriod;
         cur_outRealLowerBand = tempLower / (double)sp.optInTimePeriod;
         out.realUpperBand = cur_outRealUpperBand;
         out.realMiddleBand = cur_outRealMiddleBand;
         out.realLowerBand = cur_outRealLowerBand;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( AccbandsOut out ) {
         requireArgument("ACCBANDS value", "out", out);
         out.realUpperBand = this.cur_outRealUpperBand;
         out.realMiddleBand = this.cur_outRealMiddleBand;
         out.realLowerBand = this.cur_outRealLowerBand;
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
      public AccbandsStream clone() {
         return new AccbandsStream(this);
      }
   }

   /**
    * The outputs of one ACCBANDS bar, written by the stream into an object the
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
   public static final class AccbandsOut {
      /** SMA of the range-scaled high band. */
      public double realUpperBand;
      /** SMA of the close. */
      public double realMiddleBand;
      /** SMA of the range-scaled low band. */
      public double realLowerBand;
   }
   void accbandsStepImpl( AccbandsStream sp, double inHigh, double inLow, double inClose )
   {
      double tempUpper = 0.0;
      double tempMiddle = 0.0;
      double tempLower = 0.0;
      double tempReal = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inHigh[0] = inHigh;
         sp.ring_trailingIdx_inLow[0] = inLow;
         sp.ring_trailingIdx_inClose[0] = inClose;
      }
      /* Add the incoming bar to each running sum. */
      tempReal = inHigh + inLow;
      if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh) + Math.abs(inLow))) ) {
         tempReal = 4 * (inHigh - inLow) / tempReal;
         sp.periodTotalUpper += inHigh * (1 + tempReal);
         sp.periodTotalLower += inLow * (1 - tempReal);
      } else {
         sp.periodTotalUpper += inHigh;
         sp.periodTotalLower += inLow;
      }
      sp.periodTotalMiddle += inClose;
      /* Record the current window sums. */
      tempUpper = sp.periodTotalUpper;
      tempMiddle = sp.periodTotalMiddle;
      tempLower = sp.periodTotalLower;
      /* Remove the trailing bar from each running sum. */
      tempReal = sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] + sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
      if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx]) + Math.abs(sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx]))) ) {
         tempReal = 4 * (sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] - sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx]) / tempReal;
         sp.periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] * (1 + tempReal);
         sp.periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] * (1 - tempReal);
      } else {
         sp.periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx];
         sp.periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
      }
      sp.periodTotalMiddle -= sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx];
      /* Write the three bands. */
      sp.cur_outRealUpperBand = tempUpper / (double)sp.optInTimePeriod;
      sp.cur_outRealMiddleBand = tempMiddle / (double)sp.optInTimePeriod;
      sp.cur_outRealLowerBand = tempLower / (double)sp.optInTimePeriod;
      sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] = inHigh;
      sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] = inLow;
      sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx] = inClose;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode accbandsOpenImpl( AccbandsStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[], int outStride )
   {
      double periodTotalUpper = 0;
      double periodTotalMiddle = 0;
      double periodTotalLower = 0;
      double tempUpper = 0;
      double tempMiddle = 0;
      double tempLower = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = SMA_Lookback(optInTimePeriod);
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
      /* Each band is a simple moving average maintained as a running sum over a
       * shared trailing window (all three share optInTimePeriod, so one trailing
       * index walks all three windows in lockstep):
       *    middle = SMA( close )
       *    upper  = SMA( high * (1 + 4*(high-low)/(high+low)) )
       *    lower  = SMA( low  * (1 - 4*(high-low)/(high+low)) )
       * When high+low is zero the upper/lower map degenerates to high/low.
       * Fusing the three moving averages into one loop is bit-identical to the
       * former "two scratch buffers + three sma() calls": each accumulator's
       * add/record/subtract order is unchanged, and the High/Low map is a pure
       * function recomputed from the raw trailing bar.
       */
      periodTotalUpper = 0.0;
      periodTotalMiddle = 0.0;
      periodTotalLower = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      /* Warm up the running sums with the initial period,
       * except for the last value.
       */
      i = trailingIdx;
      while( i < startIdx ) {
         /* The band factor 4*(H-L)/(H+L) is a ratio of two prices, so it is
          * scale-free -- but H+L is a sum that CANCELS when the two prices have
          * opposite signs, and the factor then blows up on what is left of the
          * operands' last bits. Test the sum against ITS OWN operands, not against
          * a fixed band: an absolute threshold answers "cancelled" for every bar
          * of an instrument quoted small enough to fall under it, and widened
          * every band it touched (issue #253). Same test on all three sites, so
          * the bar that enters a running sum is the one that later leaves it.
          */
         tempReal = inHigh[i] + inLow[i];
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i]))) ) {
            tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
            periodTotalUpper += inHigh[i] * (1 + tempReal);
            periodTotalLower += inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += inHigh[i];
            periodTotalLower += inLow[i];
         }
         periodTotalMiddle += inClose[i];
         i = i + 1;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and output to be the
       * same buffer: every trailing bar is read before any output is written.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         /* Add the incoming bar to each running sum. */
         tempReal = inHigh[i] + inLow[i];
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[i]) + Math.abs(inLow[i]))) ) {
            tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
            periodTotalUpper += inHigh[i] * (1 + tempReal);
            periodTotalLower += inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += inHigh[i];
            periodTotalLower += inLow[i];
         }
         periodTotalMiddle += inClose[i];
         i = i + 1;
         /* Record the current window sums. */
         tempUpper = periodTotalUpper;
         tempMiddle = periodTotalMiddle;
         tempLower = periodTotalLower;
         /* Remove the trailing bar from each running sum. */
         tempReal = inHigh[trailingIdx] + inLow[trailingIdx];
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(inHigh[trailingIdx]) + Math.abs(inLow[trailingIdx]))) ) {
            tempReal = 4 * (inHigh[trailingIdx] - inLow[trailingIdx]) / tempReal;
            periodTotalUpper -= inHigh[trailingIdx] * (1 + tempReal);
            periodTotalLower -= inLow[trailingIdx] * (1 - tempReal);
         } else {
            periodTotalUpper -= inHigh[trailingIdx];
            periodTotalLower -= inLow[trailingIdx];
         }
         periodTotalMiddle -= inClose[trailingIdx];
         trailingIdx = trailingIdx + 1;
         /* Write the three bands. */
         outRealUpperBand[outIdx * outStride] = tempUpper / (double)optInTimePeriod;
         outRealMiddleBand[outIdx * outStride] = tempMiddle / (double)optInTimePeriod;
         outRealLowerBand[outIdx * outStride] = tempLower / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = i - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inHigh = new double[allocN_trailingIdx];
      System.arraycopy(inHigh, historyLen - cap_trailingIdx, capRing_trailingIdx_inHigh, 0, cap_trailingIdx);
      double[] capRing_trailingIdx_inLow = new double[allocN_trailingIdx];
      System.arraycopy(inLow, historyLen - cap_trailingIdx, capRing_trailingIdx_inLow, 0, cap_trailingIdx);
      double[] capRing_trailingIdx_inClose = new double[allocN_trailingIdx];
      System.arraycopy(inClose, historyLen - cap_trailingIdx, capRing_trailingIdx_inClose, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.periodTotalUpper = periodTotalUpper;
      sp.periodTotalMiddle = periodTotalMiddle;
      sp.periodTotalLower = periodTotalLower;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inHigh = capRing_trailingIdx_inHigh;
      sp.ring_trailingIdx_inLow = capRing_trailingIdx_inLow;
      sp.ring_trailingIdx_inClose = capRing_trailingIdx_inClose;
      sp.cur_outRealUpperBand = outRealUpperBand[(outNBElement.value - 1) * outStride];
      sp.cur_outRealMiddleBand = outRealMiddleBand[(outNBElement.value - 1) * outStride];
      sp.cur_outRealLowerBand = outRealLowerBand[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* accbandsOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AccbandsStream accbandsOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      AccbandsStream sp = new AccbandsStream(this);
      RetCode retCode = accbandsOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ACCBANDS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ACCBANDS openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ACCBANDS openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind accbandsOpen (composition seam). */
   AccbandsStream accbandsOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      AccbandsStream sp = new AccbandsStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outRealUpperBand = new double[1];
      double[] sink_outRealMiddleBand = new double[1];
      double[] sink_outRealLowerBand = new double[1];
      RetCode retCode = accbandsOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ACCBANDS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ACCBANDS open: internal error", retCode);
      }
      throw new TaLibArgumentException("ACCBANDS open: " + retCode, retCode);
   }
   /**
    * Open a live ACCBANDS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ACCBANDS} at that bar.
    * <p>The history must hold at least {@code ACCBANDS_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public AccbandsStream accbandsOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      requireArgument("ACCBANDS open", "inHigh", inHigh);
      requireHistory("ACCBANDS open", inHigh.length);
      requireArgument("ACCBANDS open", "inLow", inLow);
      requireArgument("ACCBANDS open", "inClose", inClose);
      requireHistoryLength("ACCBANDS open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ACCBANDS open", "inClose", inClose.length, inHigh.length);
      return accbandsOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#accbandsOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ACCBANDS} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AccbandsStream#outRange()}.
    */
   public AccbandsStream accbandsOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      requireArgument("ACCBANDS openAndFill", "inHigh", inHigh);
      requireHistory("ACCBANDS openAndFill", inHigh.length);
      requireArgument("ACCBANDS openAndFill", "inLow", inLow);
      requireArgument("ACCBANDS openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("ACCBANDS openAndFill", inHigh.length, ACCBANDS_Lookback(optInTimePeriod));
      requireHistoryLength("ACCBANDS openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ACCBANDS openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("ACCBANDS openAndFill", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("ACCBANDS openAndFill", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("ACCBANDS openAndFill", "outRealLowerBand", outRealLowerBand, guardOutLen);
      if( (Object)outRealUpperBand == (Object)inHigh || (Object)outRealUpperBand == (Object)inLow || (Object)outRealUpperBand == (Object)inClose || (Object)outRealMiddleBand == (Object)inHigh || (Object)outRealMiddleBand == (Object)inLow || (Object)outRealMiddleBand == (Object)inClose || (Object)outRealLowerBand == (Object)inHigh || (Object)outRealLowerBand == (Object)inLow || (Object)outRealLowerBand == (Object)inClose || (Object)outRealUpperBand == (Object)outRealMiddleBand || (Object)outRealUpperBand == (Object)outRealLowerBand || (Object)outRealMiddleBand == (Object)outRealLowerBand ) {
         throw new TaLibArgumentException("ACCBANDS openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return accbandsOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
   }
