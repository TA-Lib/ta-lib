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
 *  072026 MF,CC  First version.
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
   RetCode VWMA_Internal( int startIdx,
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
   RetCode VWMA_Internal( int startIdx,
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
    * Equivalently, and bit-identically so in TA-Lib, SMA(P * V, N) / SMA(V, N) — the composition TradingView documents for `ta.vwma`. There is no seeding and no recursion, hence no unstable period.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Volume is expected to be non-negative. Individual zero-volume bars are fine: a bar that did not trade simply carries no weight, and the average stays well defined as long as some bar in the window has volume. Only a window in which *every* volume is zero has no weights at all; the weighted mean is then undefined and that element is NaN, as it is in every other implementation. Series carrying no volume on any bar, such as cash-index feeds, are outside what a volume-weighted average can describe — use SMA or WMA there.</li>
    * <li>A period of 1 reduces to {@code (P * V) / V}. That is the price arithmetically, but not a guaranteed IEEE round trip, so unlike SMA of period 1 it must not be relied upon as an exact copy of the input.</li>
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VWMA_Internal(startIdx, endIdx, inReal, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
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
    * Equivalently, and bit-identically so in TA-Lib, SMA(P * V, N) / SMA(V, N) — the composition TradingView documents for `ta.vwma`. There is no seeding and no recursion, hence no unstable period.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Volume is expected to be non-negative. Individual zero-volume bars are fine: a bar that did not trade simply carries no weight, and the average stays well defined as long as some bar in the window has volume. Only a window in which *every* volume is zero has no weights at all; the weighted mean is then undefined and that element is NaN, as it is in every other implementation. Series carrying no volume on any bar, such as cash-index feeds, are outside what a volume-weighted average can describe — use SMA or WMA there.</li>
    * <li>A period of 1 reduces to {@code (P * V) / V}. That is the price arithmetically, but not a guaranteed IEEE round trip, so unlike SMA of period 1 it must not be relied upon as an exact copy of the input.</li>
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VWMA_Internal(startIdx, endIdx, inReal, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
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
      final Core core;
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

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal, double inVolume ) {
         core.VWMA_StreamStep(this, inReal, inVolume);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal, double inVolume ) {
         VWMA_Stream scratch = new VWMA_Stream(this);
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
   private RetCode VWMA_OpenBody( VWMA_Stream sp, double inReal[], double inVolume[], int startIdx, int optInTimePeriod )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
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
         return RetCode.OutOfRangeEndIndex ;
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
         lastValue_outReal = tempPV / (double)optInTimePeriod / (tempV / (double)optInTimePeriod);
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
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode VWMA_OpenAndFillBody( VWMA_Stream sp, double inReal[], double inVolume[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      int startIdx = 0;
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
      if( (Object)outReal == (Object)inReal || (Object)outReal == (Object)inVolume ) {
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
         return RetCode.OutOfRangeEndIndex ;
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
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind VWMA_Open (composition seam). */
   VWMA_Stream VWMA_OpenInternal( double inReal[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      VWMA_Stream sp = new VWMA_Stream(this);
      RetCode retCode = VWMA_OpenBody(sp, inReal, inVolume, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("VWMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("VWMA open: internal error");
      }
      throw new IllegalArgumentException("VWMA open: " + retCode);
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
      RetCode retCode = VWMA_OpenAndFillBody(sp, inReal, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("VWMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("VWMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("VWMA openAndFill: " + retCode);
   }
