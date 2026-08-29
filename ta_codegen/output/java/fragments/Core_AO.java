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
 *  081626 MF,CC  Initial version (#227).
 */

   /**
    * Number of leading input bars {@link Core#AO} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Number of bars in the short moving average (default
    *        5; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Number of bars in the long moving average (default
    *        34; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int AO_Lookback( int optInFastPeriod, int optInSlowPeriod )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 5;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 34;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      /* The longer of the two windows drives the lookback, and it is the lookback
       * of that window's SMA. There is no swap of an inverted pair, so the max is
       * taken over the periods exactly as the caller gave them.
       */
      return SMA_Lookback(Math.max(optInFastPeriod, optInSlowPeriod)) ;

   }
   RetCode AO_Impl( int startIdx,
                    int endIdx,
                    double inHigh[],
                    double inLow[],
                    int optInFastPeriod,
                    int optInSlowPeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double sumFast = 0;
      double sumSlow = 0;
      double medianPrice = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingFastIdx = 0;
      int trailingSlowIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 5;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 34;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Bill Williams' Awesome Oscillator (New Trading Dimensions, 1998): the
       * spread between a short and a long simple moving average of the median
       * price, drawn as a zero-centred histogram.
       *
       *    median_t = (high_t + low_t) / 2
       *    AO_t     = SMA(median, fast)_t - SMA(median, slow)_t
       *
       * Both legs are plain SMAs, so there is no seeding convention to get wrong
       * and no cross-library divergence of the kind an EMA brings.
       *
       * This is two copies of ta_codegen/input/sma/sma.c walking one derived
       * series, sharing a single pass: the same running-sum order, the same
       * snapshot of each total before the trailing bar leaves it, and the same
       * divide by the period. Keeping the arithmetic identical is what makes the
       * composed reference in test_composite.c -- TA_MEDPRICE, two TA_SMA calls
       * and a TA_SUB -- bit-exact rather than merely close, so any future drift
       * in either path is a hard failure instead of a tolerance argument.
       *
       * In particular each total is DIVIDED by its period; it is not multiplied
       * by a precomputed 1/period. Tulip's ao.c multiplies by per5/per34, which
       * costs it up to one ULP against TA_SMA. Dividing buys the memcmp
       * differential, which is the stronger of the two gates (#117, #118).
       *
       * An inverted pair (fast > slow) is NOT swapped, unlike MACD and APO. The
       * lookback is well defined either way and the result is simply -AO, so the
       * swap would buy nothing and would have to be duplicated in ao_lookback.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = (int)AO_Lookback(optInFastPeriod, optInSlowPeriod);
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
      sumFast = 0.0;
      sumSlow = 0.0;
      trailingFastIdx = startIdx - (int)(optInFastPeriod - 1);
      trailingSlowIdx = startIdx - (int)(optInSlowPeriod - 1);
      /* Add-up both initial periods, except for the last value.
       *
       * One pass over the longer warm-up window replaces two overlapping passes.
       * A bar inside the shorter window is added to that total as it is reached,
       * so each total still accumulates exactly the same bars in exactly the same
       * ascending order two separate loops would have given it -- which is what
       * keeps each leg bit-identical to the TA_SMA called with this same startIdx.
       */
      i = startIdx - lookbackTotal;
      while( i < startIdx ) {
         medianPrice = (inHigh[i] + inLow[i]) / 2.0;
         if( i >= trailingFastIdx ) {
            sumFast += medianPrice;
         }
         if( i >= trailingSlowIdx ) {
            sumSlow += medianPrice;
         }
         i = i + 1;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows outReal to be the same
       * buffer as either input.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         medianPrice = (inHigh[i] + inLow[i]) / 2.0;
         sumFast += medianPrice;
         sumSlow += medianPrice;
         i = i + 1;
         /* Snapshot the oscillator before either total drops its trailing bar,
          * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
          */
         tempReal = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
         /* Read both trailing bars before writing the output. When startIdx is
          * clamped to the lookback the longer window's trailing index equals
          * outIdx exactly, so a store hoisted above this would read back the
          * value it had just overwritten whenever the caller aliases outReal
          * over inHigh or inLow.
          */
         sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
         sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
         trailingFastIdx = trailingFastIdx + 1;
         trailingSlowIdx = trailingSlowIdx + 1;
         outReal[outIdx] = tempReal;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode AO_Impl( int startIdx,
                    int endIdx,
                    float inHigh[],
                    float inLow[],
                    int optInFastPeriod,
                    int optInSlowPeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double sumFast = 0;
      double sumSlow = 0;
      double medianPrice = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingFastIdx = 0;
      int trailingSlowIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 5;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 34;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = (int)AO_Lookback(optInFastPeriod, optInSlowPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumFast = 0.0;
      sumSlow = 0.0;
      trailingFastIdx = startIdx - (int)(optInFastPeriod - 1);
      trailingSlowIdx = startIdx - (int)(optInSlowPeriod - 1);
      i = startIdx - lookbackTotal;
      while( i < startIdx ) {
         medianPrice = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
         if( i >= trailingFastIdx ) {
            sumFast += medianPrice;
         }
         if( i >= trailingSlowIdx ) {
            sumSlow += medianPrice;
         }
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         medianPrice = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
         sumFast += medianPrice;
         sumSlow += medianPrice;
         i = i + 1;
         tempReal = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
         sumFast -= ((double)inHigh[trailingFastIdx] + (double)inLow[trailingFastIdx]) / 2.0;
         sumSlow -= ((double)inHigh[trailingSlowIdx] + (double)inLow[trailingSlowIdx]) / 2.0;
         trailingFastIdx = trailingFastIdx + 1;
         trailingSlowIdx = trailingSlowIdx + 1;
         outReal[outIdx] = tempReal;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Bill Williams' Awesome Oscillator (*New Trading Dimensions*, 1998): market
    * momentum read as the spread between a short and a long simple moving
    * average of the median price. It contrasts what the recent bars have done
    * against a longer stretch of the same market, using the bar midpoint rather
    * than the close so that intrabar range, not the settle, drives the reading.
    * Above zero the short window sits higher than the long one and momentum is
    * with the bulls; below zero it is with the bears. It is drawn as a
    * zero-centred histogram, and the readings that get traded are the zero-line
    * crossings, the twin-peaks divergence, and the run of consecutive same-side
    * bars — which is why the sign and the bar-to-bar change matter more than
    * the level. The oscillator is the first leg of Williams' Profitunity
    * system, alongside the Alligator and the Accelerator/Decelerator
    * ([{@code AC}](/functions/ac)).
    * <p><b>Formula</b>
    * <pre>{@code
    * median_t = ( high_t + low_t ) / 2
    * AO_t = SMA(median, fast)_t − SMA(median, slow)_t
    * An inverted pair is not swapped: passing a fast period longer than the slow one is well defined and simply yields −AO.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AO_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInFastPeriod Number of bars in the short moving average (default
    *        5; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Number of bars in the long moving average (default
    *        34; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Spread between the two moving averages, centred on zero.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#APO
    * @see Core#MACD
    * @see Core#MEDPRICE
    * @see Core#PPO
    * @see Core#ULTOSC
    */
   public OutRange AO( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       double outReal[] )
   {
      requireIndexRange("AO", startIdx, endIdx);
      int guardStart = clampedStart("AO", startIdx, AO_Lookback(optInFastPeriod, optInSlowPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AO", "inHigh", inHigh, guardInLen);
      requireLength("AO", "inLow", inLow, guardInLen);
      requireLength("AO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AO_Impl(startIdx, endIdx, inHigh, inLow, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Bill Williams' Awesome Oscillator (*New Trading Dimensions*, 1998): market
    * momentum read as the spread between a short and a long simple moving
    * average of the median price. It contrasts what the recent bars have done
    * against a longer stretch of the same market, using the bar midpoint rather
    * than the close so that intrabar range, not the settle, drives the reading.
    * Above zero the short window sits higher than the long one and momentum is
    * with the bulls; below zero it is with the bears. It is drawn as a
    * zero-centred histogram, and the readings that get traded are the zero-line
    * crossings, the twin-peaks divergence, and the run of consecutive same-side
    * bars — which is why the sign and the bar-to-bar change matter more than
    * the level. The oscillator is the first leg of Williams' Profitunity
    * system, alongside the Alligator and the Accelerator/Decelerator
    * ([{@code AC}](/functions/ac)).
    * <p><b>Formula</b>
    * <pre>{@code
    * median_t = ( high_t + low_t ) / 2
    * AO_t = SMA(median, fast)_t − SMA(median, slow)_t
    * An inverted pair is not swapped: passing a fast period longer than the slow one is well defined and simply yields −AO.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AO_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInFastPeriod Number of bars in the short moving average (default
    *        5; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Number of bars in the long moving average (default
    *        34; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Spread between the two moving averages, centred on zero.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#APO
    * @see Core#MACD
    * @see Core#MEDPRICE
    * @see Core#PPO
    * @see Core#ULTOSC
    */
   public OutRange AO( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       double outReal[] )
   {
      requireIndexRange("AO", startIdx, endIdx);
      int guardStart = clampedStart("AO", startIdx, AO_Lookback(optInFastPeriod, optInSlowPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AO", "inHigh", inHigh, guardInLen);
      requireLength("AO", "inLow", inLow, guardInLen);
      requireLength("AO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AO_Impl(startIdx, endIdx, inHigh, inLow, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live AO stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#AO} over the same series.
    * Open with {@link Core#aoOpen}; there is no close — the handle is
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
   public static final class AoStream {
      Core core;
      int optInFastPeriod;
      int optInSlowPeriod;
      double sumFast;
      double sumSlow;
      int ringPos_trailingFastIdx;
      int ringCap_trailingFastIdx;
      double[] ring_trailingFastIdx_derived;
      int ringPos_trailingSlowIdx;
      int ringCap_trailingSlowIdx;
      double[] ring_trailingSlowIdx_derived;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      AoStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#AO} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      AoStream( AoStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.sumFast = other.sumFast;
         this.sumSlow = other.sumSlow;
         this.ringPos_trailingFastIdx = other.ringPos_trailingFastIdx;
         this.ringCap_trailingFastIdx = other.ringCap_trailingFastIdx;
         this.ring_trailingFastIdx_derived = other.ring_trailingFastIdx_derived.clone();
         this.ringPos_trailingSlowIdx = other.ringPos_trailingSlowIdx;
         this.ringCap_trailingSlowIdx = other.ringCap_trailingSlowIdx;
         this.ring_trailingSlowIdx_derived = other.ring_trailingSlowIdx_derived.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( AoStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.sumFast = other.sumFast;
         this.sumSlow = other.sumSlow;
         this.ringPos_trailingFastIdx = other.ringPos_trailingFastIdx;
         this.ringCap_trailingFastIdx = other.ringCap_trailingFastIdx;
         if( this.ring_trailingFastIdx_derived != null && this.ring_trailingFastIdx_derived.length == other.ring_trailingFastIdx_derived.length ) {
            System.arraycopy( other.ring_trailingFastIdx_derived, 0, this.ring_trailingFastIdx_derived, 0, other.ring_trailingFastIdx_derived.length );
         } else {
            this.ring_trailingFastIdx_derived = other.ring_trailingFastIdx_derived.clone();
         }
         this.ringPos_trailingSlowIdx = other.ringPos_trailingSlowIdx;
         this.ringCap_trailingSlowIdx = other.ringCap_trailingSlowIdx;
         if( this.ring_trailingSlowIdx_derived != null && this.ring_trailingSlowIdx_derived.length == other.ring_trailingSlowIdx_derived.length ) {
            System.arraycopy( other.ring_trailingSlowIdx_derived, 0, this.ring_trailingSlowIdx_derived, 0, other.ring_trailingSlowIdx_derived.length );
         } else {
            this.ring_trailingSlowIdx_derived = other.ring_trailingSlowIdx_derived.clone();
         }
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<AoStream> PEEK_SCRATCH = new ThreadLocal<>();

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
      public double update( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("AO update: BadParam", RetCode.BadParam);
         core.aoStepImpl(this, inHigh, inLow);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inHigh[], double inLow[], double outReal[] ) {
         requireArgument("AO updateAndFill", "inHigh", inHigh);
         requireArgument("AO updateAndFill", "inLow", inLow);
         requireArgument("AO updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow )
            throw new TaLibArgumentException("AO updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) )
               throw new TaLibArgumentException("AO updateAndFill: BadParam", RetCode.BadParam);
            core.aoStepImpl(this, inHigh[i], inLow[i]);
            outReal[i] = this.cur_outReal;
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
      public double peek( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("AO peek: BadParam", RetCode.BadParam);
         AoStream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new AoStream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.aoStepImpl(scratch, inHigh, inLow);
         return scratch.cur_outReal;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public AoStream copy() {
         return new AoStream(this);
      }
   }
   void aoStepImpl( AoStream sp, double inHigh, double inLow )
   {
      double medianPrice = 0.0;
      double tempReal = 0.0;
      if( sp.ringCap_trailingFastIdx == 0 ) {
         sp.ring_trailingFastIdx_derived[0] = (inHigh + inLow) / 2.0;
      }
      if( sp.ringCap_trailingSlowIdx == 0 ) {
         sp.ring_trailingSlowIdx_derived[0] = (inHigh + inLow) / 2.0;
      }
      medianPrice = (inHigh + inLow) / 2.0;
      sp.sumFast += medianPrice;
      sp.sumSlow += medianPrice;
      /* Snapshot the oscillator before either total drops its trailing bar,
       * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
       */
      tempReal = sp.sumFast / (double)sp.optInFastPeriod - sp.sumSlow / (double)sp.optInSlowPeriod;
      /* Read both trailing bars before writing the output. When startIdx is
       * clamped to the lookback the longer window's trailing index equals
       * outIdx exactly, so a store hoisted above this would read back the
       * value it had just overwritten whenever the caller aliases outReal
       * over inHigh or inLow.
       */
      sp.sumFast -= sp.ring_trailingFastIdx_derived[sp.ringPos_trailingFastIdx];
      sp.sumSlow -= sp.ring_trailingSlowIdx_derived[sp.ringPos_trailingSlowIdx];
      sp.cur_outReal = tempReal;
      sp.ring_trailingFastIdx_derived[sp.ringPos_trailingFastIdx] = (inHigh + inLow) / 2.0;
      sp.ringPos_trailingFastIdx = sp.ringPos_trailingFastIdx + 1;
      if( sp.ringPos_trailingFastIdx >= sp.ringCap_trailingFastIdx ) {
         sp.ringPos_trailingFastIdx = 0;
      }
      sp.ring_trailingSlowIdx_derived[sp.ringPos_trailingSlowIdx] = (inHigh + inLow) / 2.0;
      sp.ringPos_trailingSlowIdx = sp.ringPos_trailingSlowIdx + 1;
      if( sp.ringPos_trailingSlowIdx >= sp.ringCap_trailingSlowIdx ) {
         sp.ringPos_trailingSlowIdx = 0;
      }
   }
   private RetCode aoOpenImpl( AoStream sp, double inHigh[], double inLow[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double sumFast = 0;
      double sumSlow = 0;
      double medianPrice = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingFastIdx = 0;
      int trailingSlowIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 5;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 34;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Bill Williams' Awesome Oscillator (New Trading Dimensions, 1998): the
       * spread between a short and a long simple moving average of the median
       * price, drawn as a zero-centred histogram.
       *
       *    median_t = (high_t + low_t) / 2
       *    AO_t     = SMA(median, fast)_t - SMA(median, slow)_t
       *
       * Both legs are plain SMAs, so there is no seeding convention to get wrong
       * and no cross-library divergence of the kind an EMA brings.
       *
       * This is two copies of ta_codegen/input/sma/sma.c walking one derived
       * series, sharing a single pass: the same running-sum order, the same
       * snapshot of each total before the trailing bar leaves it, and the same
       * divide by the period. Keeping the arithmetic identical is what makes the
       * composed reference in test_composite.c -- TA_MEDPRICE, two TA_SMA calls
       * and a TA_SUB -- bit-exact rather than merely close, so any future drift
       * in either path is a hard failure instead of a tolerance argument.
       *
       * In particular each total is DIVIDED by its period; it is not multiplied
       * by a precomputed 1/period. Tulip's ao.c multiplies by per5/per34, which
       * costs it up to one ULP against TA_SMA. Dividing buys the memcmp
       * differential, which is the stronger of the two gates (#117, #118).
       *
       * An inverted pair (fast > slow) is NOT swapped, unlike MACD and APO. The
       * lookback is well defined either way and the result is simply -AO, so the
       * swap would buy nothing and would have to be duplicated in ao_lookback.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = (int)AO_Lookback(optInFastPeriod, optInSlowPeriod);
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
      sumFast = 0.0;
      sumSlow = 0.0;
      trailingFastIdx = startIdx - (int)(optInFastPeriod - 1);
      trailingSlowIdx = startIdx - (int)(optInSlowPeriod - 1);
      /* Add-up both initial periods, except for the last value.
       *
       * One pass over the longer warm-up window replaces two overlapping passes.
       * A bar inside the shorter window is added to that total as it is reached,
       * so each total still accumulates exactly the same bars in exactly the same
       * ascending order two separate loops would have given it -- which is what
       * keeps each leg bit-identical to the TA_SMA called with this same startIdx.
       */
      i = startIdx - lookbackTotal;
      while( i < startIdx ) {
         medianPrice = (inHigh[i] + inLow[i]) / 2.0;
         if( i >= trailingFastIdx ) {
            sumFast += medianPrice;
         }
         if( i >= trailingSlowIdx ) {
            sumSlow += medianPrice;
         }
         i = i + 1;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows outReal to be the same
       * buffer as either input.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         medianPrice = (inHigh[i] + inLow[i]) / 2.0;
         sumFast += medianPrice;
         sumSlow += medianPrice;
         i = i + 1;
         /* Snapshot the oscillator before either total drops its trailing bar,
          * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
          */
         tempReal = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
         /* Read both trailing bars before writing the output. When startIdx is
          * clamped to the lookback the longer window's trailing index equals
          * outIdx exactly, so a store hoisted above this would read back the
          * value it had just overwritten whenever the caller aliases outReal
          * over inHigh or inLow.
          */
         sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
         sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
         trailingFastIdx = trailingFastIdx + 1;
         trailingSlowIdx = trailingSlowIdx + 1;
         outReal[outIdx * outStride] = tempReal;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingFastIdx = i - trailingFastIdx;
      if( cap_trailingFastIdx < 0 || cap_trailingFastIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingFastIdx = (cap_trailingFastIdx > 0)? cap_trailingFastIdx : 1;
      double[] capRing_trailingFastIdx_derived = new double[allocN_trailingFastIdx];
      for( int fillJ = historyLen - cap_trailingFastIdx; fillJ < historyLen; fillJ++ ) {
         capRing_trailingFastIdx_derived[fillJ - (historyLen - cap_trailingFastIdx)] = (inHigh[fillJ] + inLow[fillJ]) / 2.0;
      }
      int cap_trailingSlowIdx = i - trailingSlowIdx;
      if( cap_trailingSlowIdx < 0 || cap_trailingSlowIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingSlowIdx = (cap_trailingSlowIdx > 0)? cap_trailingSlowIdx : 1;
      double[] capRing_trailingSlowIdx_derived = new double[allocN_trailingSlowIdx];
      for( int fillJ = historyLen - cap_trailingSlowIdx; fillJ < historyLen; fillJ++ ) {
         capRing_trailingSlowIdx_derived[fillJ - (historyLen - cap_trailingSlowIdx)] = (inHigh[fillJ] + inLow[fillJ]) / 2.0;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.sumFast = sumFast;
      sp.sumSlow = sumSlow;
      sp.ringPos_trailingFastIdx = 0;
      sp.ringCap_trailingFastIdx = cap_trailingFastIdx;
      sp.ring_trailingFastIdx_derived = capRing_trailingFastIdx_derived;
      sp.ringPos_trailingSlowIdx = 0;
      sp.ringCap_trailingSlowIdx = cap_trailingSlowIdx;
      sp.ring_trailingSlowIdx_derived = capRing_trailingSlowIdx_derived;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* aoOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AoStream aoOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AoStream sp = new AoStream(this);
      RetCode retCode = aoOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AO openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AO openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("AO openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind aoOpen (composition seam). */
   AoStream aoOpenInternal( double inHigh[], double inLow[], int startIdx, int optInFastPeriod, int optInSlowPeriod )
   {
      AoStream sp = new AoStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = aoOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AO open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AO open: internal error", retCode);
      }
      throw new TaLibArgumentException("AO open: " + retCode, retCode);
   }
   /**
    * Open a live AO stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#AO} at that bar.
    * <p>The history must hold at least {@code AO_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public AoStream aoOpen( double inHigh[], double inLow[], int optInFastPeriod, int optInSlowPeriod )
   {
      requireArgument("AO open", "inHigh", inHigh);
      requireHistory("AO open", inHigh.length);
      requireArgument("AO open", "inLow", inLow);
      requireHistoryLength("AO open", "inLow", inLow.length, inHigh.length);
      return aoOpenInternal(inHigh, inLow, 0, optInFastPeriod, optInSlowPeriod);
   }
   /**
    * {@link Core#aoOpen} that also fills the output array(s) bit-identically
    * to {@link Core#AO} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AoStream#outRange()}.
    */
   public AoStream aoOpenAndFill( double inHigh[], double inLow[], int optInFastPeriod, int optInSlowPeriod, double outReal[] )
   {
      requireArgument("AO openAndFill", "inHigh", inHigh);
      requireHistory("AO openAndFill", inHigh.length);
      requireArgument("AO openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("AO openAndFill", inHigh.length, AO_Lookback(optInFastPeriod, optInSlowPeriod));
      requireHistoryLength("AO openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("AO openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw new TaLibArgumentException("AO openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return aoOpenAndFillInternal(inHigh, inLow, 0, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
   }
