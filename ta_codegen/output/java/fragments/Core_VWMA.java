/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  072026 MF,CC  First version (#131).
 *  080926 MF,CC  Allow period of 1. Just copy input into output.
 */

   /**
    * Number of leading input bars {@link Core#VWMA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the weighting window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int VWMA_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode VWMA_Impl( int startIdx,
                      int endIdx,
                      double inReal[],
                      double inVolume[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double sumPV = 0;
      double sumV = 0;
      double tempPV = 0;
      double tempV = 0;
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
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = (int)(optInTimePeriod - 1);
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
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). Explicit because
       * (P*V)/V round-trips only ~97% of the time in IEEE double, and
       * because a lone zero volume must give the price, not NaN.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         i = startIdx;
         while( i <= (int)endIdx ) {
            outReal[outIdx++] = inReal[i++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      /* Add-up the initial period, except for the last value.
       *
       * The price*volume product is kept in its own statement so no compiler may
       * contract it into an FMA: that would make this function disagree with the
       * Rust/Java backends under the cross-language bitwise gate, and with the
       * two-TA_SMA composite reference.
       */
      sumPV = 0.0;
      sumV = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = inReal[i] * inVolume[i];
            sumPV += tempReal;
            sumV += inVolume[i];
            i = i + 1;
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = inReal[i] * inVolume[i];
         sumPV += tempReal;
         sumV += inVolume[i];
         i = i + 1;
         /* Snapshot both sums before removing the trailing bar, mirroring the
          * add-new / snapshot / subtract-old order of TA_SMA. That order is what
          * makes this bit-identical to SMA(inReal*inVolume)/SMA(inVolume).
          */
         tempPV = sumPV;
         tempV = sumV;
         /* Read the trailing values before writing the output, since the caller
          * may pass the same buffer for an input and the output.
          */
         tempReal = inReal[trailingIdx] * inVolume[trailingIdx];
         sumPV -= tempReal;
         sumV -= inVolume[trailingIdx];
         outReal[outIdx] = tempPV / (double)optInTimePeriod / (tempV / (double)optInTimePeriod);
         trailingIdx = trailingIdx + 1;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode VWMA_Impl( int startIdx,
                      int endIdx,
                      float inReal[],
                      float inVolume[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double sumPV = 0;
      double sumV = 0;
      double tempPV = 0;
      double tempV = 0;
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
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = (int)(optInTimePeriod - 1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         i = startIdx;
         while( i <= (int)endIdx ) {
            outReal[outIdx++] = (double)inReal[i++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      sumPV = 0.0;
      sumV = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = (double)inReal[i] * (double)inVolume[i];
            sumPV += tempReal;
            sumV += (double)inVolume[i];
            i = i + 1;
         }
      }
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = (double)inReal[i] * (double)inVolume[i];
         sumPV += tempReal;
         sumV += (double)inVolume[i];
         i = i + 1;
         tempPV = sumPV;
         tempV = sumV;
         tempReal = (double)inReal[trailingIdx] * (double)inVolume[trailingIdx];
         sumPV -= tempReal;
         sumV -= (double)inVolume[trailingIdx];
         outReal[outIdx] = tempPV / (double)optInTimePeriod / (tempV / (double)optInTimePeriod);
         trailingIdx = trailingIdx + 1;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Volume Weighted Moving Average: the mean price over a trailing window of
    * {@code optInTimePeriod} bars, each bar weighted by its own volume. Heavily
    * traded bars pull the average toward their price; quiet bars barely move
    * it. Read like any moving average — price above is strength, below is
    * weakness. Against a plain [{@code SMA}](/functions/sma) of the same window
    * it leads on high-volume moves and lags on low-volume drift, so the gap
    * between the two lines measures how volume-confirmed a move is. It has no
    * attributable inventor — charting-package folklore — and every published
    * definition agrees, so there is no competing variant.
    * <p><b>Formula</b>
    * <pre>{@code
    * VWMA = ( sum_{k=t-N+1..t} P[k] * V[k] ) / ( sum_{k=t-N+1..t} V[k] ), N = optInTimePeriod
    * Equivalently, and bit-identically so in TA-Lib for N of 2 or more, SMA(P * V, N) / SMA(V, N) — the composition TradingView documents for `ta.vwma`. There is no seeding and no recursion, hence no unstable period.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input, whatever the volume.</li>
    * <li>Volume is expected to be non-negative. Individual zero-volume bars are fine: a bar that did not trade simply carries no weight, and the average stays well defined as long as some bar in the window has volume. At a period of 2 or more, a window in which *every* volume is zero has no weights at all; the weighted mean is then undefined and that element is NaN, as it is in every other implementation. Series carrying no volume on any bar, such as cash-index feeds, are outside what a volume-weighted average can describe — use SMA or WMA there.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#VWMA_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series, close by convention.
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod Number of bars in the weighting window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Volume weighted moving average of the input. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#SMA
    * @see Core#WMA
    * @see Core#MA
    * @see Core#OBV
    */
   public OutRange VWMA( int startIdx,
                         int endIdx,
                         double inReal[],
                         double inVolume[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("VWMA", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, VWMA_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VWMA", "inReal", inReal, guardInLen);
      requireLength("VWMA", "inVolume", inVolume, guardInLen);
      requireLength("VWMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VWMA_Impl(startIdx, endIdx, inReal, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("VWMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Volume Weighted Moving Average: the mean price over a trailing window of
    * {@code optInTimePeriod} bars, each bar weighted by its own volume. Heavily
    * traded bars pull the average toward their price; quiet bars barely move
    * it. Read like any moving average — price above is strength, below is
    * weakness. Against a plain [{@code SMA}](/functions/sma) of the same window
    * it leads on high-volume moves and lags on low-volume drift, so the gap
    * between the two lines measures how volume-confirmed a move is. It has no
    * attributable inventor — charting-package folklore — and every published
    * definition agrees, so there is no competing variant.
    * <p><b>Formula</b>
    * <pre>{@code
    * VWMA = ( sum_{k=t-N+1..t} P[k] * V[k] ) / ( sum_{k=t-N+1..t} V[k] ), N = optInTimePeriod
    * Equivalently, and bit-identically so in TA-Lib for N of 2 or more, SMA(P * V, N) / SMA(V, N) — the composition TradingView documents for `ta.vwma`. There is no seeding and no recursion, hence no unstable period.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input, whatever the volume.</li>
    * <li>Volume is expected to be non-negative. Individual zero-volume bars are fine: a bar that did not trade simply carries no weight, and the average stays well defined as long as some bar in the window has volume. At a period of 2 or more, a window in which *every* volume is zero has no weights at all; the weighted mean is then undefined and that element is NaN, as it is in every other implementation. Series carrying no volume on any bar, such as cash-index feeds, are outside what a volume-weighted average can describe — use SMA or WMA there.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#VWMA_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series, close by convention.
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod Number of bars in the weighting window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Volume weighted moving average of the input. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#SMA
    * @see Core#WMA
    * @see Core#MA
    * @see Core#OBV
    */
   public OutRange VWMA( int startIdx,
                         int endIdx,
                         float inReal[],
                         float inVolume[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("VWMA", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, VWMA_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VWMA", "inReal", inReal, guardInLen);
      requireLength("VWMA", "inVolume", inVolume, guardInLen);
      requireLength("VWMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VWMA_Impl(startIdx, endIdx, inReal, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("VWMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live VWMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#VWMA} over the same series.
    * Open with {@link Core#VWMA_Open}; there is no close — the handle is
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
   public static final class VWMA_Stream {
      Core core;
      int optInTimePeriod;
      double sumPV;
      double sumV;
      double tempPV;
      double tempV;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double[] ring_trailingIdx_inVolume;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      VWMA_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#VWMA_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      VWMA_Stream( VWMA_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.sumPV = other.sumPV;
         this.sumV = other.sumV;
         this.tempPV = other.tempPV;
         this.tempV = other.tempV;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.ring_trailingIdx_inVolume = other.ring_trailingIdx_inVolume.clone();
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( VWMA_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.sumPV = other.sumPV;
         this.sumV = other.sumV;
         this.tempPV = other.tempPV;
         this.tempV = other.tempV;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         if( this.ring_trailingIdx_inReal != null && this.ring_trailingIdx_inReal.length == other.ring_trailingIdx_inReal.length ) {
            System.arraycopy( other.ring_trailingIdx_inReal, 0, this.ring_trailingIdx_inReal, 0, other.ring_trailingIdx_inReal.length );
         } else {
            this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         }
         if( this.ring_trailingIdx_inVolume != null && this.ring_trailingIdx_inVolume.length == other.ring_trailingIdx_inVolume.length ) {
            System.arraycopy( other.ring_trailingIdx_inVolume, 0, this.ring_trailingIdx_inVolume, 0, other.ring_trailingIdx_inVolume.length );
         } else {
            this.ring_trailingIdx_inVolume = other.ring_trailingIdx_inVolume.clone();
         }
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<VWMA_Stream> PEEK_SCRATCH = new ThreadLocal<>();

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
      public double update( double inReal, double inVolume ) {
         if( !Double.isFinite(inReal) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("VWMA update: BadParam", RetCode.BadParam);
         core.VWMA_StreamStep(this, inReal, inVolume);
         return this.cur_outReal;
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
      public double peek( double inReal, double inVolume ) {
         if( !Double.isFinite(inReal) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("VWMA peek: BadParam", RetCode.BadParam);
         VWMA_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new VWMA_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.VWMA_StreamStep(scratch, inReal, inVolume);
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
      public VWMA_Stream copy() {
         return new VWMA_Stream(this);
      }
   }
   void VWMA_StreamStep( VWMA_Stream sp, double inReal, double inVolume )
   {
      double tempReal = 0.0;
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
         sp.ring_trailingIdx_inVolume[0] = inVolume;
      }
      tempReal = inReal * inVolume;
      sp.sumPV += tempReal;
      sp.sumV += inVolume;
      /* Snapshot both sums before removing the trailing bar, mirroring the
       * add-new / snapshot / subtract-old order of TA_SMA. That order is what
       * makes this bit-identical to SMA(inReal*inVolume)/SMA(inVolume).
       */
      sp.tempPV = sp.sumPV;
      sp.tempV = sp.sumV;
      /* Read the trailing values before writing the output, since the caller
       * may pass the same buffer for an input and the output.
       */
      tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] * sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx];
      sp.sumPV -= tempReal;
      sp.sumV -= sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx];
      sp.cur_outReal = sp.tempPV / (double)sp.optInTimePeriod / (sp.tempV / (double)sp.optInTimePeriod);
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx] = inVolume;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode VWMA_OpenPass( VWMA_Stream sp, double inReal[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double sumPV = 0;
      double sumV = 0;
      double tempPV = 0;
      double tempV = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inVolume.length != inReal.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         if( historyLen < VWMA_Lookback(optInTimePeriod) + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.sumPV = 0.0;
         sp.sumV = 0.0;
         sp.tempPV = 0.0;
         sp.tempV = 0.0;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = 0;
         sp.ring_trailingIdx_inReal = new double[1];
         sp.ring_trailingIdx_inVolume = new double[1];
         int fillLb = VWMA_Lookback(optInTimePeriod);
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         if( outStride == 0 ) {
            outReal[0] = inReal[historyLen - 1];
         } else {
            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
               outReal[fillIdx] = inReal[fillLb + fillIdx];
            }
         }
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = (int)(optInTimePeriod - 1);
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
      /* Add-up the initial period, except for the last value.
       *
       * The price*volume product is kept in its own statement so no compiler may
       * contract it into an FMA: that would make this function disagree with the
       * Rust/Java backends under the cross-language bitwise gate, and with the
       * two-TA_SMA composite reference.
       */
      sumPV = 0.0;
      sumV = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = inReal[i] * inVolume[i];
            sumPV += tempReal;
            sumV += inVolume[i];
            i = i + 1;
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = inReal[i] * inVolume[i];
         sumPV += tempReal;
         sumV += inVolume[i];
         i = i + 1;
         /* Snapshot both sums before removing the trailing bar, mirroring the
          * add-new / snapshot / subtract-old order of TA_SMA. That order is what
          * makes this bit-identical to SMA(inReal*inVolume)/SMA(inVolume).
          */
         tempPV = sumPV;
         tempV = sumV;
         /* Read the trailing values before writing the output, since the caller
          * may pass the same buffer for an input and the output.
          */
         tempReal = inReal[trailingIdx] * inVolume[trailingIdx];
         sumPV -= tempReal;
         sumV -= inVolume[trailingIdx];
         outReal[outIdx * outStride] = tempPV / (double)optInTimePeriod / (tempV / (double)optInTimePeriod);
         trailingIdx = trailingIdx + 1;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = i - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      double[] capRing_trailingIdx_inVolume = new double[allocN_trailingIdx];
      System.arraycopy(inVolume, historyLen - cap_trailingIdx, capRing_trailingIdx_inVolume, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.sumPV = sumPV;
      sp.sumV = sumV;
      sp.tempPV = tempPV;
      sp.tempV = tempV;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.ring_trailingIdx_inVolume = capRing_trailingIdx_inVolume;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode VWMA_OpenImpl( VWMA_Stream sp, double inReal[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return VWMA_OpenPass( sp, inReal, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode VWMA_OpenAndFillImpl( VWMA_Stream sp, double inReal[], double inVolume[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal || (Object)outReal == (Object)inVolume ) {
         return RetCode.BadParam;
      }
      return VWMA_OpenPass( sp, inReal, inVolume, 0, optInTimePeriod, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode VWMA_OpenAndFillInternalImpl( VWMA_Stream sp, double inReal[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return VWMA_OpenPass(sp, inReal, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
   }
   /* VWMA_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   VWMA_Stream VWMA_OpenAndFillInternal( double inReal[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      VWMA_Stream sp = new VWMA_Stream(this);
      RetCode retCode = VWMA_OpenAndFillInternalImpl(sp, inReal, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VWMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VWMA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("VWMA openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind VWMA_Open (composition seam). */
   VWMA_Stream VWMA_OpenInternal( double inReal[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      VWMA_Stream sp = new VWMA_Stream(this);
      RetCode retCode = VWMA_OpenImpl(sp, inReal, inVolume, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VWMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VWMA open: internal error", retCode);
      }
      throw new TaLibArgumentException("VWMA open: " + retCode, retCode);
   }
   /**
    * Open a live VWMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#VWMA} at that bar.
    * <p>The history must hold at least {@code VWMA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public VWMA_Stream VWMA_Open( double inReal[], double inVolume[], int optInTimePeriod )
   {
      return VWMA_OpenInternal(inReal, inVolume, 0, optInTimePeriod);
   }
   /**
    * {@link Core#VWMA_Open} that also fills the output array(s) bit-identically
    * to {@link Core#VWMA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link VWMA_Stream#fillRange()}.
    */
   public VWMA_Stream VWMA_OpenAndFill( double inReal[], double inVolume[], int optInTimePeriod, double outReal[] )
   {
      VWMA_Stream sp = new VWMA_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VWMA_OpenAndFillImpl(sp, inReal, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VWMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VWMA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("VWMA openAndFill: " + retCode, retCode);
   }
