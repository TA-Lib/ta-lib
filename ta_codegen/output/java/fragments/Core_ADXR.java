/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  082303 MF     Fix #792298. Remove rounding. Bug reported by AM.
 *  071126 MF,CC  Rewrite the ADX combine as a single cursor: outReal[k] =
 *                (adx[k+(period-1)] + adx[k])/2 (current ADX + ADX lagged by
 *                period-1). Bit-identical to the two-cursor form, and the
 *                streamable-source form (a sub-output lag ring).
 */

   /**
    * Number of leading input bars {@link Core#ADXR} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Smoothing period, also the bar gap between the two
    *        averaged ADX values (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ADXR_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInTimePeriod > 1 ) {
         return optInTimePeriod + ADX_Lookback(optInTimePeriod) - 1 ;
      } else {
         return 3 ;
      }

   }
   RetCode ADXR_Impl( int startIdx,
                      int endIdx,
                      double inHigh[],
                      double inLow[],
                      double inClose[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double[] adx;
      int adxrLookback = 0;
      int outIdx = 0;
      int nbElement = 0;
      RetCode retCode;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Original implementation from Wilder's book was doing some integer
       * rounding in its calculations.
       *
       * This was understandable in the context that at the time the book
       * was written, most user were doing the calculation by hand.
       *
       * For a computer, rounding is unnecessary (and even problematic when inputs
       * are close to 1).
       *
       * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
       * you can comment out the following #undef/#define and rebuild the library.
       */
      /* Move up the start index if there is not
       * enough initial data.
       * Always one price bar gets consumed.
       */
      adxrLookback = ADXR_Lookback(optInTimePeriod);
      if( startIdx < adxrLookback ) {
         startIdx = adxrLookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      adx = new double[(int)((endIdx - startIdx + optInTimePeriod) * 1)];
      /* Compute ADX over a range that starts (period-1) bars earlier, so each
       * ADXR bar can pair the current ADX with the ADX from (period-1) bars ago.
       */
      OutRange _xr0 = ADX(startIdx - (optInTimePeriod - 1), endIdx, inHigh, inLow, inClose, optInTimePeriod, adx);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* ADXR[k] = (ADX[k] + ADX[k-(period-1)]) / 2. Walking a single cursor over
       * the ADXR output, the current ADX is adx[k+(period-1)] and the lagged one
       * is adx[k]; the ADX range holds (period-1) more elements than the output.
       */
      nbElement = outNBElement.value - (optInTimePeriod - 1);
      for( outIdx = 0; outIdx < nbElement; outIdx += 1 ) {
         outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      return RetCode.Success ;
   }
   RetCode ADXR_Impl( int startIdx,
                      int endIdx,
                      float inHigh[],
                      float inLow[],
                      float inClose[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double[] adx;
      int adxrLookback = 0;
      int outIdx = 0;
      int nbElement = 0;
      RetCode retCode;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      adxrLookback = ADXR_Lookback(optInTimePeriod);
      if( startIdx < adxrLookback ) {
         startIdx = adxrLookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      adx = new double[(int)((endIdx - startIdx + optInTimePeriod) * 1)];
      OutRange _xr0 = ADX(startIdx - (optInTimePeriod - 1), endIdx, inHigh, inLow, inClose, optInTimePeriod, adx);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      nbElement = outNBElement.value - (optInTimePeriod - 1);
      for( outIdx = 0; outIdx < nbElement; outIdx += 1 ) {
         outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      return RetCode.Success ;
   }
   /**
    * Smoothed variant of ADX: the average of the current ADX value and the ADX
    * value from (period-1) bars earlier. Further damps ADX to gauge trend
    * strength. Higher values mean a stronger trend; smoother and more lagging
    * than ADX.
    * <p><b>Formula</b>
    * <pre>{@code
    * ADXR[i] = (ADX[i] + ADX[i-(period-1)]) / 2
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Wilder's original integer rounding is not applied (unreliable when values are near 1).</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ADXR_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing period, also the bar gap between the two
    *        averaged ADX values (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal ADXR line (averaged ADX) Must hold at least
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
    * @see Core#ADX
    * @see Core#DX
    * @see Core#PLUS_DI
    * @see Core#MINUS_DI
    */
   public OutRange ADXR( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         double inClose[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("ADXR", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, ADXR_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ADXR", "inHigh", inHigh, guardInLen);
      requireLength("ADXR", "inLow", inLow, guardInLen);
      requireLength("ADXR", "inClose", inClose, guardInLen);
      requireLength("ADXR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ADXR_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ADXR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Smoothed variant of ADX: the average of the current ADX value and the ADX
    * value from (period-1) bars earlier. Further damps ADX to gauge trend
    * strength. Higher values mean a stronger trend; smoother and more lagging
    * than ADX.
    * <p><b>Formula</b>
    * <pre>{@code
    * ADXR[i] = (ADX[i] + ADX[i-(period-1)]) / 2
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Wilder's original integer rounding is not applied (unreliable when values are near 1).</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ADXR_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing period, also the bar gap between the two
    *        averaged ADX values (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal ADXR line (averaged ADX) Must hold at least
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
    * @see Core#ADX
    * @see Core#DX
    * @see Core#PLUS_DI
    * @see Core#MINUS_DI
    */
   public OutRange ADXR( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         float inClose[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("ADXR", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, ADXR_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ADXR", "inHigh", inHigh, guardInLen);
      requireLength("ADXR", "inLow", inLow, guardInLen);
      requireLength("ADXR", "inClose", inClose, guardInLen);
      requireLength("ADXR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ADXR_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ADXR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ADXR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ADXR} over the same series.
    * Open with {@link Core#ADXR_Open}; there is no close — the handle is
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
   public static final class ADXR_Stream {
      Core core;
      int optInTimePeriod;
      double cur_outReal;
      int lagRingPos_adx;
      int lagRingCap_adx;
      double[] lagRing_adx;
      ADX_Stream sub0;
      int outRangeBegIdx;
      int outRangeCount;

      ADXR_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ADXR} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      ADXR_Stream( ADXR_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.cur_outReal = other.cur_outReal;
         this.lagRingPos_adx = other.lagRingPos_adx;
         this.lagRingCap_adx = other.lagRingCap_adx;
         this.lagRing_adx = other.lagRing_adx.clone();
         this.sub0 = new ADX_Stream(other.sub0);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( ADXR_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.cur_outReal = other.cur_outReal;
         this.lagRingPos_adx = other.lagRingPos_adx;
         this.lagRingCap_adx = other.lagRingCap_adx;
         if( this.lagRing_adx != null && this.lagRing_adx.length == other.lagRing_adx.length ) {
            System.arraycopy( other.lagRing_adx, 0, this.lagRing_adx, 0, other.lagRing_adx.length );
         } else {
            this.lagRing_adx = other.lagRing_adx.clone();
         }
         if( this.sub0 == null ) {
            this.sub0 = new ADX_Stream(other.sub0);
         } else {
            this.sub0.copyFrom(other.sub0);
         }
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
      public double update( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("ADXR update: BadParam", RetCode.BadParam);
         core.ADXR_StepImpl(this, inHigh, inLow, inClose);
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
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] ) {
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("ADXR updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("ADXR updateAndFill: BadParam", RetCode.BadParam);
            core.ADXR_StepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outReal[i] = this.cur_outReal;
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
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("ADXR peek: BadParam", RetCode.BadParam);
         ADXR_Stream scratch = new ADXR_Stream(this);
         core.ADXR_StepImpl(scratch, inHigh, inLow, inClose);
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
      public ADXR_Stream copy() {
         return new ADXR_Stream(this);
      }
   }
   void ADXR_StepImpl( ADXR_Stream sp, double inHigh, double inLow, double inClose )
   {
      double cur_adx = 0.0;
      double cur_outReal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_adx = sp.sub0.update(inHigh, inLow, inClose);
      /* Combine map (batch tail, per bar). */
      cur_outReal = ((cur_adx + sp.lagRing_adx[sp.lagRingPos_adx]) / 2.0);
      sp.lagRing_adx[sp.lagRingPos_adx] = cur_adx;
      sp.lagRingPos_adx = (sp.lagRingPos_adx + 1) % sp.lagRingCap_adx;
      sp.cur_outReal = cur_outReal;
   }
   private RetCode ADXR_OpenImpl( ADXR_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double[] adx;
      int adxrLookback = 0;
      int outIdx = 0;
      int nbElement = 0;
      RetCode retCode;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( historyLen < ADXR_Lookback(optInTimePeriod) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outReal = outStride == 1 ? outReal : new double[historyLen];
      /* Original implementation from Wilder's book was doing some integer
       * rounding in its calculations.
       *
       * This was understandable in the context that at the time the book
       * was written, most user were doing the calculation by hand.
       *
       * For a computer, rounding is unnecessary (and even problematic when inputs
       * are close to 1).
       *
       * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
       * you can comment out the following #undef/#define and rebuild the library.
       */
      /* Move up the start index if there is not
       * enough initial data.
       * Always one price bar gets consumed.
       */
      adxrLookback = ADXR_Lookback(optInTimePeriod);
      if( startIdx < adxrLookback ) {
         startIdx = adxrLookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      adx = new double[(int)((endIdx - startIdx + optInTimePeriod) * 1)];
      /* Compute ADX over a range that starts (period-1) bars earlier, so each
       * ADXR bar can pair the current ADX with the ADX from (period-1) bars ago.
       */
      /* Sub-stream 0: adx over `inHigh, inLow, inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      ADX_Stream sub0 = ADX_OpenAndFillInternal(inHigh, inLow, inClose, startIdx - (optInTimePeriod - 1), optInTimePeriod, outBegIdx, outNBElement, adx);
      retCode = RetCode.Success;
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* ADXR[k] = (ADX[k] + ADX[k-(period-1)]) / 2. Walking a single cursor over
       * the ADXR output, the current ADX is adx[k+(period-1)] and the lagged one
       * is adx[k]; the ADX range holds (period-1) more elements than the output.
       */
      nbElement = outNBElement.value - (optInTimePeriod - 1);
      for( outIdx = 0; outIdx < nbElement; outIdx += 1 ) {
         sc_outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.InsufficientHistory;
      }
      int lagCap_adx = (int)(optInTimePeriod - 1);
      double[] lagRing_adx = new double[lagCap_adx];
      for( int lagI = 0; lagI < lagCap_adx; lagI++ ) {
         lagRing_adx[lagI] = adx[outNBElement.value + lagI];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.sub0 = sub0;
      sp.lagRingPos_adx = 0;
      sp.lagRingCap_adx = lagCap_adx;
      sp.lagRing_adx = lagRing_adx;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* ADXR_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   ADXR_Stream ADXR_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      ADXR_Stream sp = new ADXR_Stream(this);
      RetCode retCode = ADXR_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ADXR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ADXR openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ADXR openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind ADXR_Open (composition seam). */
   ADXR_Stream ADXR_OpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      ADXR_Stream sp = new ADXR_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = ADXR_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ADXR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ADXR open: internal error", retCode);
      }
      throw new TaLibArgumentException("ADXR open: " + retCode, retCode);
   }
   /**
    * Open a live ADXR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ADXR} at that bar.
    * <p>The history must hold at least {@code ADXR_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public ADXR_Stream ADXR_Open( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      return ADXR_OpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#ADXR_Open} that also fills the output array(s) bit-identically
    * to {@link Core#ADXR} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link ADXR_Stream#outRange()}.
    */
   public ADXR_Stream ADXR_OpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outReal[] )
   {
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("ADXR openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ADXR_OpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
