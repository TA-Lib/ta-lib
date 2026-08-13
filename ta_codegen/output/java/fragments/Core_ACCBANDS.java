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
   RetCode ACCBANDS_Internal( int startIdx,
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
         tempReal = inHigh[i] + inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
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
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
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
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
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
   RetCode ACCBANDS_Internal( int startIdx,
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
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
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
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
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
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ACCBANDS_Internal(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ACCBANDS_Internal(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode != RetCode.Success ) {
         throw failure("ACCBANDS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ACCBANDS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ACCBANDS} over the same series.
    * Open with {@link Core#ACCBANDS_Open}; there is no close — the handle is
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
   public static final class ACCBANDS_Stream {
      Core core;
      int optInTimePeriod;
      double periodTotalUpper;
      double periodTotalMiddle;
      double periodTotalLower;
      double tempUpper;
      double tempMiddle;
      double tempLower;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inHigh;
      double[] ring_trailingIdx_inLow;
      double[] ring_trailingIdx_inClose;
      double cur_outRealUpperBand;
      double cur_outRealMiddleBand;
      double cur_outRealLowerBand;
      Value cachedValue;
      OutRange fillRange = OutRange.EMPTY;

      ACCBANDS_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#ACCBANDS_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      ACCBANDS_Stream( ACCBANDS_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.periodTotalUpper = other.periodTotalUpper;
         this.periodTotalMiddle = other.periodTotalMiddle;
         this.periodTotalLower = other.periodTotalLower;
         this.tempUpper = other.tempUpper;
         this.tempMiddle = other.tempMiddle;
         this.tempLower = other.tempLower;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inHigh = other.ring_trailingIdx_inHigh.clone();
         this.ring_trailingIdx_inLow = other.ring_trailingIdx_inLow.clone();
         this.ring_trailingIdx_inClose = other.ring_trailingIdx_inClose.clone();
         this.cur_outRealUpperBand = other.cur_outRealUpperBand;
         this.cur_outRealMiddleBand = other.cur_outRealMiddleBand;
         this.cur_outRealLowerBand = other.cur_outRealLowerBand;
         this.cachedValue = other.cachedValue;
         this.fillRange = other.fillRange;
      }

      void copyFrom( ACCBANDS_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.periodTotalUpper = other.periodTotalUpper;
         this.periodTotalMiddle = other.periodTotalMiddle;
         this.periodTotalLower = other.periodTotalLower;
         this.tempUpper = other.tempUpper;
         this.tempMiddle = other.tempMiddle;
         this.tempLower = other.tempLower;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         if( this.ring_trailingIdx_inHigh != null && this.ring_trailingIdx_inHigh.length == other.ring_trailingIdx_inHigh.length ) {
            System.arraycopy( other.ring_trailingIdx_inHigh, 0, this.ring_trailingIdx_inHigh, 0, other.ring_trailingIdx_inHigh.length );
         } else {
            this.ring_trailingIdx_inHigh = other.ring_trailingIdx_inHigh.clone();
         }
         if( this.ring_trailingIdx_inLow != null && this.ring_trailingIdx_inLow.length == other.ring_trailingIdx_inLow.length ) {
            System.arraycopy( other.ring_trailingIdx_inLow, 0, this.ring_trailingIdx_inLow, 0, other.ring_trailingIdx_inLow.length );
         } else {
            this.ring_trailingIdx_inLow = other.ring_trailingIdx_inLow.clone();
         }
         if( this.ring_trailingIdx_inClose != null && this.ring_trailingIdx_inClose.length == other.ring_trailingIdx_inClose.length ) {
            System.arraycopy( other.ring_trailingIdx_inClose, 0, this.ring_trailingIdx_inClose, 0, other.ring_trailingIdx_inClose.length );
         } else {
            this.ring_trailingIdx_inClose = other.ring_trailingIdx_inClose.clone();
         }
         this.cur_outRealUpperBand = other.cur_outRealUpperBand;
         this.cur_outRealMiddleBand = other.cur_outRealMiddleBand;
         this.cur_outRealLowerBand = other.cur_outRealLowerBand;
         this.cachedValue = other.cachedValue;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<ACCBANDS_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * One output set, in batch output order. Immutable.
       *
       * <p>{@code equals} compares every component bitwise, so {@code NaN}
       * equals {@code NaN} and {@code 0.0} does not equal {@code -0.0}.
       * {@code hashCode} is consistent with it but its exact value is
       * unspecified — do not persist it or compare it across JVM versions.
       *
       * @param realUpperBand SMA of the range-scaled high band.
       * @param realMiddleBand SMA of the close.
       * @param realLowerBand SMA of the range-scaled low band.
       */
      public record Value(double realUpperBand, double realMiddleBand, double realLowerBand) { }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inHigh, double inLow, double inClose ) {
         core.ACCBANDS_StreamStep(this, inHigh, inLow, inClose);
         this.cachedValue = new Value(this.cur_outRealUpperBand, this.cur_outRealMiddleBand, this.cur_outRealLowerBand);
         return this.cachedValue;
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
      public Value peek( double inHigh, double inLow, double inClose ) {
         ACCBANDS_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new ACCBANDS_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.ACCBANDS_StreamStep(scratch, inHigh, inLow, inClose);
         return new Value(scratch.cur_outRealUpperBand, scratch.cur_outRealMiddleBand, scratch.cur_outRealLowerBand);
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public Value value() {
         return this.cachedValue;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public ACCBANDS_Stream copy() {
         return new ACCBANDS_Stream(this);
      }
   }
   void ACCBANDS_StreamStep( ACCBANDS_Stream sp, double inHigh, double inLow, double inClose )
   {
      double tempReal = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inHigh[0] = inHigh;
         sp.ring_trailingIdx_inLow[0] = inLow;
         sp.ring_trailingIdx_inClose[0] = inClose;
      }
      /* Add the incoming bar to each running sum. */
      tempReal = inHigh + inLow;
      if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
         tempReal = 4 * (inHigh - inLow) / tempReal;
         sp.periodTotalUpper += inHigh * (1 + tempReal);
         sp.periodTotalLower += inLow * (1 - tempReal);
      } else {
         sp.periodTotalUpper += inHigh;
         sp.periodTotalLower += inLow;
      }
      sp.periodTotalMiddle += inClose;
      /* Record the current window sums. */
      sp.tempUpper = sp.periodTotalUpper;
      sp.tempMiddle = sp.periodTotalMiddle;
      sp.tempLower = sp.periodTotalLower;
      /* Remove the trailing bar from each running sum. */
      tempReal = sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] + sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
      if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
         tempReal = 4 * (sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] - sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx]) / tempReal;
         sp.periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] * (1 + tempReal);
         sp.periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] * (1 - tempReal);
      } else {
         sp.periodTotalUpper -= sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx];
         sp.periodTotalLower -= sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx];
      }
      sp.periodTotalMiddle -= sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx];
      /* Write the three bands. */
      sp.cur_outRealUpperBand = sp.tempUpper / (double)sp.optInTimePeriod;
      sp.cur_outRealMiddleBand = sp.tempMiddle / (double)sp.optInTimePeriod;
      sp.cur_outRealLowerBand = sp.tempLower / (double)sp.optInTimePeriod;
      sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] = inHigh;
      sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] = inLow;
      sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx] = inClose;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode ACCBANDS_OpenCore( ACCBANDS_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[], int outStride )
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
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
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
         return RetCode.OutOfRangeEndIndex ;
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
         tempReal = inHigh[i] + inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
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
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
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
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
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
      sp.tempUpper = tempUpper;
      sp.tempMiddle = tempMiddle;
      sp.tempLower = tempLower;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inHigh = capRing_trailingIdx_inHigh;
      sp.ring_trailingIdx_inLow = capRing_trailingIdx_inLow;
      sp.ring_trailingIdx_inClose = capRing_trailingIdx_inClose;
      sp.cur_outRealUpperBand = outRealUpperBand[(outNBElement.value - 1) * outStride];
      sp.cur_outRealMiddleBand = outRealMiddleBand[(outNBElement.value - 1) * outStride];
      sp.cur_outRealLowerBand = outRealLowerBand[(outNBElement.value - 1) * outStride];
      sp.cachedValue = new ACCBANDS_Stream.Value(sp.cur_outRealUpperBand, sp.cur_outRealMiddleBand, sp.cur_outRealLowerBand);
      return RetCode.Success;
   }
   private RetCode ACCBANDS_OpenBody( ACCBANDS_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outRealUpperBand = new double[1];
      double[] sink_outRealMiddleBand = new double[1];
      double[] sink_outRealLowerBand = new double[1];
      return ACCBANDS_OpenCore( sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0 );
   }
   private RetCode ACCBANDS_OpenAndFillBody( ACCBANDS_Stream sp, double inHigh[], double inLow[], double inClose[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      if( (Object)outRealUpperBand == (Object)inHigh || (Object)outRealUpperBand == (Object)inLow || (Object)outRealUpperBand == (Object)inClose || (Object)outRealMiddleBand == (Object)inHigh || (Object)outRealMiddleBand == (Object)inLow || (Object)outRealMiddleBand == (Object)inClose || (Object)outRealLowerBand == (Object)inHigh || (Object)outRealLowerBand == (Object)inLow || (Object)outRealLowerBand == (Object)inClose || (Object)outRealUpperBand == (Object)outRealMiddleBand || (Object)outRealUpperBand == (Object)outRealLowerBand || (Object)outRealMiddleBand == (Object)outRealLowerBand ) {
         return RetCode.BadParam;
      }
      return ACCBANDS_OpenCore( sp, inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1 );
   }
   private RetCode ACCBANDS_OpenAndFillInternalBody( ACCBANDS_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      return ACCBANDS_OpenCore(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
   }
   /* ACCBANDS_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   ACCBANDS_Stream ACCBANDS_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      ACCBANDS_Stream sp = new ACCBANDS_Stream(this);
      RetCode retCode = ACCBANDS_OpenAndFillInternalBody(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("ACCBANDS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("ACCBANDS openAndFill: internal error");
      }
      throw new IllegalArgumentException("ACCBANDS openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind ACCBANDS_Open (composition seam). */
   ACCBANDS_Stream ACCBANDS_OpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      ACCBANDS_Stream sp = new ACCBANDS_Stream(this);
      RetCode retCode = ACCBANDS_OpenBody(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("ACCBANDS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("ACCBANDS open: internal error");
      }
      throw new IllegalArgumentException("ACCBANDS open: " + retCode);
   }
   /**
    * Open a live ACCBANDS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ACCBANDS} at that bar.
    * <p>The history must hold at least {@code ACCBANDS_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public ACCBANDS_Stream ACCBANDS_Open( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      return ACCBANDS_OpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#ACCBANDS_Open} that also fills the output array(s) bit-identically
    * to {@link Core#ACCBANDS} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link ACCBANDS_Stream#fillRange()}.
    */
   public ACCBANDS_Stream ACCBANDS_OpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      ACCBANDS_Stream sp = new ACCBANDS_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ACCBANDS_OpenAndFillBody(sp, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("ACCBANDS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("ACCBANDS openAndFill: internal error");
      }
      throw new IllegalArgumentException("ACCBANDS openAndFill: " + retCode);
   }
