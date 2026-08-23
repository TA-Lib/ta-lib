/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  JP       John Price <jp_talib@gcfl.net>
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  070203 JP     Initial.
 *  071326 MF,CC  O(period) per-bar rescan -> O(1) sliding-sum recurrence
 *                (numerics-changing). See issue #103.
 *  072026 MF,CC  Read the departing value before the output write so in-place
 *                (outReal==inReal) calls stay correct. See issue #130.
 */

   /**
    * Number of leading input bars {@link Core#LINEARREG} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in each regression window (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int LINEARREG_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode LINEARREG_Impl( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInTimePeriod,
                           MInteger outBegIdx,
                           MInteger outNBElement,
                           double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      double b = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
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
      /* Linear Regression is a concept also known as the
       * "least squares method" or "best fit." Linear
       * Regression attempts to fit a straight line between
       * several data points in such a way that distance
       * between each data point and the line is minimized.
       *
       * For each point, a straight line over the specified
       * previous bar period is determined in terms
       * of y = b + m*x:
       *
       * TA_LINEARREG          : Returns b+m*(period-1)
       * TA_LINEARREG_SLOPE    : Returns 'm'
       * TA_LINEARREG_ANGLE    : Returns 'm' in degree.
       * TA_LINEARREG_INTERCEPT: Returns 'b'
       * TA_TSF                : Returns b+m*(period)
       */
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = LINEARREG_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      /* Index into the output. */
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = (double)optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = (double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      /* Prime the two data-dependent window sums for the first output with a
       * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
       * SumY = sum of the window, SumXY = sum of i*value (i the reversed
       * 0..period-1 position).
       */
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = (SumY - m * SumX) / (double)optInTimePeriod;
      trailingValue = inReal[trailingIdx++];
      outReal[outIdx++] = Math.fma(m, (double)(optInTimePeriod - 1), b);
      today += 1;
      /* Slide the window one bar at a time, keeping both sums in O(1): advancing
       * the window raises every retained value's weight by 1 (adds SumY) and drops
       * the departing value at full weight (subtracts period*trailingValue). Same
       * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
       * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
       * Each departing value is read before the output write of the same bar:
       * with outReal==inReal (in-place, #130) that write lands on the cell the
       * next iteration departs from.
       */
      while( today <= endIdx ) {
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + inReal[today];
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         b = (SumY - m * SumX) / (double)optInTimePeriod;
         trailingValue = inReal[trailingIdx++];
         outReal[outIdx++] = Math.fma(m, (double)(optInTimePeriod - 1), b);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode LINEARREG_Impl( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInTimePeriod,
                           MInteger outBegIdx,
                           MInteger outNBElement,
                           double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      double b = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
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
      lookbackTotal = LINEARREG_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = (double)optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = (double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = (double)inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = (SumY - m * SumX) / (double)optInTimePeriod;
      trailingValue = (double)inReal[trailingIdx++];
      outReal[outIdx++] = Math.fma(m, (double)(optInTimePeriod - 1), b);
      today += 1;
      while( today <= endIdx ) {
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + (double)inReal[today];
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         b = (SumY - m * SumX) / (double)optInTimePeriod;
         trailingValue = (double)inReal[trailingIdx++];
         outReal[outIdx++] = Math.fma(m, (double)(optInTimePeriod - 1), b);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Least-squares straight-line fit over the last optInTimePeriod bars,
    * reported as the fitted line value at the window endpoint (b +
    * m*(period-1)).
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#LINEARREG_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to fit.
    * @param optInTimePeriod Number of bars in each regression window (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Regression line value at the window endpoint. Must hold at
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
    * @see Core#LINEARREG_SLOPE
    * @see Core#LINEARREG_ANGLE
    * @see Core#LINEARREG_INTERCEPT
    * @see Core#TSF
    */
   public OutRange LINEARREG( int startIdx,
                              int endIdx,
                              double inReal[],
                              int optInTimePeriod,
                              double outReal[] )
   {
      requireIndexRange("LINEARREG", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, LINEARREG_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("LINEARREG", "inReal", inReal, guardInLen);
      requireLength("LINEARREG", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = LINEARREG_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("LINEARREG", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Least-squares straight-line fit over the last optInTimePeriod bars,
    * reported as the fitted line value at the window endpoint (b +
    * m*(period-1)).
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#LINEARREG_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to fit.
    * @param optInTimePeriod Number of bars in each regression window (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Regression line value at the window endpoint. Must hold at
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
    * @see Core#LINEARREG_SLOPE
    * @see Core#LINEARREG_ANGLE
    * @see Core#LINEARREG_INTERCEPT
    * @see Core#TSF
    */
   public OutRange LINEARREG( int startIdx,
                              int endIdx,
                              float inReal[],
                              int optInTimePeriod,
                              double outReal[] )
   {
      requireIndexRange("LINEARREG", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, LINEARREG_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("LINEARREG", "inReal", inReal, guardInLen);
      requireLength("LINEARREG", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = LINEARREG_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("LINEARREG", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live LINEARREG stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#LINEARREG} over the same series.
    * Open with {@link Core#LINEARREG_Open}; there is no close — the handle is
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
   public static final class LINEARREG_Stream {
      Core core;
      int optInTimePeriod;
      double SumX;
      double SumXY;
      double SumY;
      double Divisor;
      double trailingValue;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      LINEARREG_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#LINEARREG} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      LINEARREG_Stream( LINEARREG_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.SumX = other.SumX;
         this.SumXY = other.SumXY;
         this.SumY = other.SumY;
         this.Divisor = other.Divisor;
         this.trailingValue = other.trailingValue;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( LINEARREG_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.SumX = other.SumX;
         this.SumXY = other.SumXY;
         this.SumY = other.SumY;
         this.Divisor = other.Divisor;
         this.trailingValue = other.trailingValue;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         if( this.ring_trailingIdx_inReal != null && this.ring_trailingIdx_inReal.length == other.ring_trailingIdx_inReal.length ) {
            System.arraycopy( other.ring_trailingIdx_inReal, 0, this.ring_trailingIdx_inReal, 0, other.ring_trailingIdx_inReal.length );
         } else {
            this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         }
         this.cur_outReal = other.cur_outReal;
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
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("LINEARREG update: BadParam", RetCode.BadParam);
         core.LINEARREG_StepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("LINEARREG updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) )
               throw new TaLibArgumentException("LINEARREG updateAndFill: BadParam", RetCode.BadParam);
            core.LINEARREG_StepImpl(this, inReal[i]);
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
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("LINEARREG peek: BadParam", RetCode.BadParam);
         LINEARREG_Stream scratch = new LINEARREG_Stream(this);
         core.LINEARREG_StepImpl(scratch, inReal);
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
      public LINEARREG_Stream copy() {
         return new LINEARREG_Stream(this);
      }
   }
   void LINEARREG_StepImpl( LINEARREG_Stream sp, double inReal )
   {
      double m = 0.0;
      double b = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      sp.SumXY = sp.SumXY + sp.SumY - (double)sp.optInTimePeriod * sp.trailingValue;
      sp.SumY = sp.SumY - sp.trailingValue + inReal;
      m = (sp.optInTimePeriod * sp.SumXY - sp.SumX * sp.SumY) / sp.Divisor;
      b = (sp.SumY - m * sp.SumX) / (double)sp.optInTimePeriod;
      sp.trailingValue = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      sp.cur_outReal = Math.fma(m, (double)(sp.optInTimePeriod - 1), b);
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode LINEARREG_OpenImpl( LINEARREG_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      double b = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
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
      /* Linear Regression is a concept also known as the
       * "least squares method" or "best fit." Linear
       * Regression attempts to fit a straight line between
       * several data points in such a way that distance
       * between each data point and the line is minimized.
       *
       * For each point, a straight line over the specified
       * previous bar period is determined in terms
       * of y = b + m*x:
       *
       * TA_LINEARREG          : Returns b+m*(period-1)
       * TA_LINEARREG_SLOPE    : Returns 'm'
       * TA_LINEARREG_ANGLE    : Returns 'm' in degree.
       * TA_LINEARREG_INTERCEPT: Returns 'b'
       * TA_TSF                : Returns b+m*(period)
       */
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = LINEARREG_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      outIdx = 0;
      /* Index into the output. */
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = (double)optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = (double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      /* Prime the two data-dependent window sums for the first output with a
       * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
       * SumY = sum of the window, SumXY = sum of i*value (i the reversed
       * 0..period-1 position).
       */
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = (SumY - m * SumX) / (double)optInTimePeriod;
      trailingValue = inReal[trailingIdx++];
      outReal[outIdx++ * outStride] = Math.fma(m, (double)(optInTimePeriod - 1), b);
      today += 1;
      /* Slide the window one bar at a time, keeping both sums in O(1): advancing
       * the window raises every retained value's weight by 1 (adds SumY) and drops
       * the departing value at full weight (subtracts period*trailingValue). Same
       * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
       * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
       * Each departing value is read before the output write of the same bar:
       * with outReal==inReal (in-place, #130) that write lands on the cell the
       * next iteration departs from.
       */
      while( today <= endIdx ) {
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + inReal[today];
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         b = (SumY - m * SumX) / (double)optInTimePeriod;
         trailingValue = inReal[trailingIdx++];
         outReal[outIdx++ * outStride] = Math.fma(m, (double)(optInTimePeriod - 1), b);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.SumX = SumX;
      sp.SumXY = SumXY;
      sp.SumY = SumY;
      sp.Divisor = Divisor;
      sp.trailingValue = trailingValue;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* LINEARREG_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   LINEARREG_Stream LINEARREG_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      LINEARREG_Stream sp = new LINEARREG_Stream(this);
      RetCode retCode = LINEARREG_OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("LINEARREG openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("LINEARREG openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("LINEARREG openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind LINEARREG_Open (composition seam). */
   LINEARREG_Stream LINEARREG_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      LINEARREG_Stream sp = new LINEARREG_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = LINEARREG_OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("LINEARREG open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("LINEARREG open: internal error", retCode);
      }
      throw new TaLibArgumentException("LINEARREG open: " + retCode, retCode);
   }
   /**
    * Open a live LINEARREG stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#LINEARREG} at that bar.
    * <p>The history must hold at least {@code LINEARREG_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public LINEARREG_Stream LINEARREG_Open( double inReal[], int optInTimePeriod )
   {
      return LINEARREG_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#LINEARREG_Open} that also fills the output array(s) bit-identically
    * to {@link Core#LINEARREG} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link LINEARREG_Stream#outRange()}.
    */
   public LINEARREG_Stream LINEARREG_OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("LINEARREG openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return LINEARREG_OpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
