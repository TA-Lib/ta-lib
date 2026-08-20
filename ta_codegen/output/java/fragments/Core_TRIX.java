/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AA       Andrew Atkinson
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  020605 AA     Fix #1117656. NULL pointer assignement.
 *  070526 MF,CC  Speed optimization: single lockstep pass (bit-exact
 *                for startIdx <= lookback). Fix #98: partial-range
 *                output was mislabeled by up to one EMA lookback.
 */

   /**
    * Number of leading input bars {@link Core#TRIX} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod EMA period used at each of the three smoothing
    *        passes (default 30; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int TRIX_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int emaLookback;
      emaLookback = EMA_Lookback(optInTimePeriod);
      return emaLookback * 3 + ROCR_Lookback(1) ;

   }
   RetCode TRIX_Impl( int startIdx,
                      int endIdx,
                      double inReal[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double prevEMA1 = 0;
      double prevEMA2 = 0;
      double prevEMA3 = 0;
      double tempReal = 0;
      double optInK_1 = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackEMA = 0;
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
      /* TRIX = 1-day percent rate-of-change of a triple EMA. */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = EMA_Lookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3 + ROCR_Lookback(1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* Single lockstep pass: EMA1 feeds EMA2 feeds EMA3, output is the
       * roc() of consecutive EMA3 values. Output element j is the TRIX
       * of bar startIdx+j (fix #98). The arithmetic order below is the
       * bit-exactness contract — do not reorder or fuse operations; the
       * seed sums accumulate from 0.0 in production order (0.0+x is not
       * x for x=-0.0). In-place safe: outReal[outIdx] is written after
       * inReal[startIdx+outIdx] was read.
       */
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* Seed EMA1 with a simple average of the first
       * 'period' price bars.
       */
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevEMA1 = tempReal / optInTimePeriod;
      /* Advance EMA1 alone through its unstable period, up to
       * the bar where EMA2 seeding begins.
       */
      while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
      }
      /* Seed EMA2 with a simple average of the first 'period'
       * EMA1 values, accumulated as EMA1 produces them.
       */
      tempReal = 0.0;
      tempReal += prevEMA1;
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         tempReal += prevEMA1;
      }
      prevEMA2 = tempReal / optInTimePeriod;
      /* Advance EMA1 and EMA2 in lockstep through the unstable
       * period of EMA2, up to the bar where EMA3 seeding begins.
       */
      while( today <= startIdx - (lookbackEMA + 1) ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      /* Seed EMA3 with a simple average of the first 'period'
       * EMA2 values, accumulated as EMA2 produces them.
       */
      tempReal = 0.0;
      tempReal += prevEMA2;
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         tempReal += prevEMA2;
      }
      prevEMA3 = tempReal / optInTimePeriod;
      /* Advance all three EMA in lockstep through the unstable
       * period of EMA3, up to the bar before the first output.
       */
      while( today <= startIdx - 1 ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
      }
      /* Stable zone: keep advancing the three EMA in lockstep and
       * write the 1-day rate-of-change of EMA3 into the output.
       */
      outIdx = 0;
      while( today <= endIdx ) {
         tempReal = prevEMA3;
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
         if( tempReal != 0.0 ) {
            outReal[outIdx++] = (prevEMA3 / tempReal - 1.0) * 100.0;
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      /* Succeed. Indicate where the output starts relative to
       * the caller input.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode TRIX_Impl( int startIdx,
                      int endIdx,
                      float inReal[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double prevEMA1 = 0;
      double prevEMA2 = 0;
      double prevEMA3 = 0;
      double tempReal = 0;
      double optInK_1 = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackEMA = 0;
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
      outNBElement.value = 0;
      outBegIdx.value = 0;
      lookbackEMA = EMA_Lookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3 + ROCR_Lookback(1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += (double)inReal[today++];
      }
      prevEMA1 = tempReal / optInTimePeriod;
      while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
      }
      tempReal = 0.0;
      tempReal += prevEMA1;
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         tempReal += prevEMA1;
      }
      prevEMA2 = tempReal / optInTimePeriod;
      while( today <= startIdx - (lookbackEMA + 1) ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      tempReal = 0.0;
      tempReal += prevEMA2;
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         tempReal += prevEMA2;
      }
      prevEMA3 = tempReal / optInTimePeriod;
      while( today <= startIdx - 1 ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
      }
      outIdx = 0;
      while( today <= endIdx ) {
         tempReal = prevEMA3;
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
         if( tempReal != 0.0 ) {
            outReal[outIdx++] = (prevEMA3 / tempReal - 1.0) * 100.0;
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * 1-day Rate-Of-Change of a triple-smoothed EMA of the input. Momentum
    * oscillator that filters out price moves shorter than the chosen period.
    * Oscillates around zero; sign, zero-crossings and slope signal momentum
    * direction.
    * <p><b>Formula</b>
    * <pre>{@code
    * E1 = EMA(inReal, n); E2 = EMA(E1, n); E3 = EMA(E2, n); TRIX = ROC_1(E3) = 100 * (E3_today/E3_yesterday - 1)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The final rate-of-change step yields 0 when the previous smoothed value is exactly zero, rather than being undefined.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#TRIX_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series to smooth.
    * @param optInTimePeriod EMA period used at each of the three smoothing
    *        passes (default 30; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal 1-day percent ROC of the triple EMA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#EMA
    * @see Core#ROC
    * @see Core#ROCR
    * @see Core#TEMA
    */
   public OutRange TRIX( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("TRIX", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, TRIX_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("TRIX", "inReal", inReal, guardInLen);
      requireLength("TRIX", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TRIX_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("TRIX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * 1-day Rate-Of-Change of a triple-smoothed EMA of the input. Momentum
    * oscillator that filters out price moves shorter than the chosen period.
    * Oscillates around zero; sign, zero-crossings and slope signal momentum
    * direction.
    * <p><b>Formula</b>
    * <pre>{@code
    * E1 = EMA(inReal, n); E2 = EMA(E1, n); E3 = EMA(E2, n); TRIX = ROC_1(E3) = 100 * (E3_today/E3_yesterday - 1)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The final rate-of-change step yields 0 when the previous smoothed value is exactly zero, rather than being undefined.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#TRIX_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series to smooth.
    * @param optInTimePeriod EMA period used at each of the three smoothing
    *        passes (default 30; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal 1-day percent ROC of the triple EMA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#EMA
    * @see Core#ROC
    * @see Core#ROCR
    * @see Core#TEMA
    */
   public OutRange TRIX( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("TRIX", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, TRIX_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("TRIX", "inReal", inReal, guardInLen);
      requireLength("TRIX", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TRIX_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("TRIX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live TRIX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#TRIX} over the same series.
    * Open with {@link Core#TRIX_Open}; there is no close — the handle is
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
   public static final class TRIX_Stream {
      Core core;
      int optInTimePeriod;
      double prevEMA1;
      double prevEMA2;
      double prevEMA3;
      double optInK_1;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      TRIX_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#TRIX_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      TRIX_Stream( TRIX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevEMA1 = other.prevEMA1;
         this.prevEMA2 = other.prevEMA2;
         this.prevEMA3 = other.prevEMA3;
         this.optInK_1 = other.optInK_1;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( TRIX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevEMA1 = other.prevEMA1;
         this.prevEMA2 = other.prevEMA2;
         this.prevEMA3 = other.prevEMA3;
         this.optInK_1 = other.optInK_1;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
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
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("TRIX update: BadParam", RetCode.BadParam);
         core.TRIX_StreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("TRIX peek: BadParam", RetCode.BadParam);
         TRIX_Stream scratch = new TRIX_Stream(this);
         core.TRIX_StreamStep(scratch, inReal);
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
      public TRIX_Stream copy() {
         return new TRIX_Stream(this);
      }
   }
   void TRIX_StreamStep( TRIX_Stream sp, double inReal )
   {
      double tempReal = 0.0;
      tempReal = sp.prevEMA3;
      sp.prevEMA1 = Math.fma(inReal - sp.prevEMA1, sp.optInK_1, sp.prevEMA1);
      sp.prevEMA2 = Math.fma(sp.prevEMA1 - sp.prevEMA2, sp.optInK_1, sp.prevEMA2);
      sp.prevEMA3 = Math.fma(sp.prevEMA2 - sp.prevEMA3, sp.optInK_1, sp.prevEMA3);
      if( tempReal != 0.0 ) {
         sp.cur_outReal = (sp.prevEMA3 / tempReal - 1.0) * 100.0;
      } else {
         sp.cur_outReal = 0.0;
      }
   }
   private RetCode TRIX_OpenPass( TRIX_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double prevEMA1 = 0;
      double prevEMA2 = 0;
      double prevEMA3 = 0;
      double tempReal = 0;
      double optInK_1 = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackEMA = 0;
      int lookbackTotal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
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
      /* TRIX = 1-day percent rate-of-change of a triple EMA. */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = EMA_Lookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3 + ROCR_Lookback(1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      /* Single lockstep pass: EMA1 feeds EMA2 feeds EMA3, output is the
       * roc() of consecutive EMA3 values. Output element j is the TRIX
       * of bar startIdx+j (fix #98). The arithmetic order below is the
       * bit-exactness contract — do not reorder or fuse operations; the
       * seed sums accumulate from 0.0 in production order (0.0+x is not
       * x for x=-0.0). In-place safe: outReal[outIdx] is written after
       * inReal[startIdx+outIdx] was read.
       */
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* Seed EMA1 with a simple average of the first
       * 'period' price bars.
       */
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevEMA1 = tempReal / optInTimePeriod;
      /* Advance EMA1 alone through its unstable period, up to
       * the bar where EMA2 seeding begins.
       */
      while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
      }
      /* Seed EMA2 with a simple average of the first 'period'
       * EMA1 values, accumulated as EMA1 produces them.
       */
      tempReal = 0.0;
      tempReal += prevEMA1;
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         tempReal += prevEMA1;
      }
      prevEMA2 = tempReal / optInTimePeriod;
      /* Advance EMA1 and EMA2 in lockstep through the unstable
       * period of EMA2, up to the bar where EMA3 seeding begins.
       */
      while( today <= startIdx - (lookbackEMA + 1) ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      /* Seed EMA3 with a simple average of the first 'period'
       * EMA2 values, accumulated as EMA2 produces them.
       */
      tempReal = 0.0;
      tempReal += prevEMA2;
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         tempReal += prevEMA2;
      }
      prevEMA3 = tempReal / optInTimePeriod;
      /* Advance all three EMA in lockstep through the unstable
       * period of EMA3, up to the bar before the first output.
       */
      while( today <= startIdx - 1 ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
      }
      /* Stable zone: keep advancing the three EMA in lockstep and
       * write the 1-day rate-of-change of EMA3 into the output.
       */
      outIdx = 0;
      while( today <= endIdx ) {
         tempReal = prevEMA3;
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
         if( tempReal != 0.0 ) {
            outReal[outIdx++ * outStride] = (prevEMA3 / tempReal - 1.0) * 100.0;
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
      }
      /* Succeed. Indicate where the output starts relative to
       * the caller input.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.prevEMA1 = prevEMA1;
      sp.prevEMA2 = prevEMA2;
      sp.prevEMA3 = prevEMA3;
      sp.optInK_1 = optInK_1;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode TRIX_OpenImpl( TRIX_Stream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return TRIX_OpenPass( sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode TRIX_OpenAndFillImpl( TRIX_Stream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      return TRIX_OpenPass( sp, inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode TRIX_OpenAndFillInternalImpl( TRIX_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return TRIX_OpenPass(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
   }
   /* TRIX_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   TRIX_Stream TRIX_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      TRIX_Stream sp = new TRIX_Stream(this);
      RetCode retCode = TRIX_OpenAndFillInternalImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("TRIX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("TRIX openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("TRIX openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind TRIX_Open (composition seam). */
   TRIX_Stream TRIX_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      TRIX_Stream sp = new TRIX_Stream(this);
      RetCode retCode = TRIX_OpenImpl(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("TRIX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("TRIX open: internal error", retCode);
      }
      throw new TaLibArgumentException("TRIX open: " + retCode, retCode);
   }
   /**
    * Open a live TRIX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#TRIX} at that bar.
    * <p>The history must hold at least {@code TRIX_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public TRIX_Stream TRIX_Open( double inReal[], int optInTimePeriod )
   {
      return TRIX_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#TRIX_Open} that also fills the output array(s) bit-identically
    * to {@link Core#TRIX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link TRIX_Stream#fillRange()}.
    */
   public TRIX_Stream TRIX_OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      TRIX_Stream sp = new TRIX_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TRIX_OpenAndFillImpl(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("TRIX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("TRIX openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("TRIX openAndFill: " + retCode, retCode);
   }
