/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   /**
    * Number of leading input bars {@link Core#WMA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the weighting window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int WMA_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode WMA_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      double divider = 0;
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
      lookbackTotal = optInTimePeriod - 1;
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
      /* To make the rest more efficient, handle exception
       * case where the user is asking for a period of '1'.
       * In that case outputs equals inputs for the requested
       * range.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outNBElement.value = endIdx - startIdx + 1;
         /* Element loop, not a block copy: the C single-precision variant reads a
          * float array, so a double-sized byte copy would reinterpret and
          * over-read it (#137). Forward order keeps the in-place case correct (#94).
          */
         inIdx = startIdx;
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            outReal[i] = inReal[inIdx++];
         }
         return RetCode.Success ;
      }
      /* Weighted denominator 1+2+...+n = n(n+1)/2. Computed in double: the
       * int product n*(n+1) overflows int32 at n>=46341 (#142).
       */
      divider = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
      /* The algo used here use a very basic property of
       * multiplication/addition: (x*2) = x+x
       *
       * As an example, a 3 period weighted can be
       * interpreted in two way:
       *  (x1*1)+(x2*2)+(x3*3)
       *      OR
       *  x1+x2+x2+x3+x3+x3 (this is the periodSum)
       *
       * When you move forward in the time serie
       * you can quickly adjust the periodSum for the
       * period by substracting:
       *   x1+x2+x3 (This is the periodSub)
       * Making the new periodSum equals to:
       *   x2+x3+x3
       *
       * You can then add the new price bar
       * which is x4+x4+x4 giving:
       *   x2+x3+x3+x4+x4+x4
       *
       * At this point one iteration is completed and you can
       * see that we are back to the step 1 of this example.
       *
       * Why making it so un-intuitive? The number of memory
       * access and floating point operations are kept to a
       * minimum with this algo.
       */
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      /* Evaluate the initial periodSum/periodSub and trailingValue. */
      periodSub = (double)0.0;
      periodSum = periodSub;
      inIdx = trailingIdx;
      i = 1;
      while( inIdx < startIdx ) {
         tempReal = inReal[inIdx++];
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
      }
      trailingValue = 0.0;
      /* Tight loop for the requested range. */
      while( inIdx <= endIdx ) {
         /* Add the current price bar to the sum
          * who are carried through the iterations.
          */
         tempReal = inReal[inIdx++];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         /* Save the trailing value for being substract at
          * the next iteration.
          * (must be saved here just in case outReal and
          *  inReal are the same buffer).
          */
         trailingValue = inReal[trailingIdx++];
         /* Calculate the WMA for this price bar. */
         outReal[outIdx++] = periodSum / divider;
         /* Prepare the periodSum for the next iteration. */
         periodSum -= periodSub;
      }
      /* Set output limits. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode WMA_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      double divider = 0;
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
      lookbackTotal = optInTimePeriod - 1;
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
         outNBElement.value = endIdx - startIdx + 1;
         inIdx = startIdx;
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            outReal[i] = (double)inReal[inIdx++];
         }
         return RetCode.Success ;
      }
      divider = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      periodSub = (double)0.0;
      periodSum = periodSub;
      inIdx = trailingIdx;
      i = 1;
      while( inIdx < startIdx ) {
         tempReal = (double)inReal[inIdx++];
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
      }
      trailingValue = 0.0;
      while( inIdx <= endIdx ) {
         tempReal = (double)inReal[inIdx++];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         trailingValue = (double)inReal[trailingIdx++];
         outReal[outIdx++] = periodSum / divider;
         periodSum -= periodSub;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Linearly weighted moving average: each of the last N prices is weighted by
    * its position, oldest getting weight 1 and newest weight N. Smooths price
    * while emphasizing recent bars.
    * <p><b>Formula</b>
    * <pre>{@code
    * WMA = ( sum_{k=1..N} k * P_k ) / (N(N+1)/2), where P_N is the most recent bar
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#WMA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/data series.
    * @param optInTimePeriod Number of bars in the weighting window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Weighted moving average series. Must hold at least
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
    * @see Core#SMA
    * @see Core#EMA
    * @see Core#MA
    * @see Core#DEMA
    * @see Core#TEMA
    */
   public OutRange WMA( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("WMA", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, WMA_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WMA", "inReal", inReal, guardInLen);
      requireLength("WMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("WMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Linearly weighted moving average: each of the last N prices is weighted by
    * its position, oldest getting weight 1 and newest weight N. Smooths price
    * while emphasizing recent bars.
    * <p><b>Formula</b>
    * <pre>{@code
    * WMA = ( sum_{k=1..N} k * P_k ) / (N(N+1)/2), where P_N is the most recent bar
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#WMA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/data series.
    * @param optInTimePeriod Number of bars in the weighting window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Weighted moving average series. Must hold at least
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
    * @see Core#SMA
    * @see Core#EMA
    * @see Core#MA
    * @see Core#DEMA
    * @see Core#TEMA
    */
   public OutRange WMA( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("WMA", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, WMA_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WMA", "inReal", inReal, guardInLen);
      requireLength("WMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("WMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live WMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#WMA} over the same series.
    * Open with {@link Core#WMA_Open}; there is no close — the handle is
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
   public static final class WMA_Stream {
      Core core;
      int optInTimePeriod;
      double periodSum;
      double periodSub;
      double trailingValue;
      double divider;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      WMA_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#WMA} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      WMA_Stream( WMA_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.periodSum = other.periodSum;
         this.periodSub = other.periodSub;
         this.trailingValue = other.trailingValue;
         this.divider = other.divider;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( WMA_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.periodSum = other.periodSum;
         this.periodSub = other.periodSub;
         this.trailingValue = other.trailingValue;
         this.divider = other.divider;
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
            throw new TaLibArgumentException("WMA update: BadParam", RetCode.BadParam);
         core.WMA_StepImpl(this, inReal);
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
            throw new TaLibArgumentException("WMA updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) )
               throw new TaLibArgumentException("WMA updateAndFill: BadParam", RetCode.BadParam);
            core.WMA_StepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("WMA peek: BadParam", RetCode.BadParam);
         WMA_Stream scratch = new WMA_Stream(this);
         core.WMA_StepImpl(scratch, inReal);
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
      public WMA_Stream copy() {
         return new WMA_Stream(this);
      }
   }
   void WMA_StepImpl( WMA_Stream sp, double inReal )
   {
      double tempReal = 0.0;
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      /* Add the current price bar to the sum
       * who are carried through the iterations.
       */
      tempReal = inReal;
      sp.periodSub += tempReal;
      sp.periodSub -= sp.trailingValue;
      sp.periodSum += tempReal * sp.optInTimePeriod;
      /* Save the trailing value for being substract at
       * the next iteration.
       * (must be saved here just in case outReal and
       *  inReal are the same buffer).
       */
      sp.trailingValue = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      /* Calculate the WMA for this price bar. */
      sp.cur_outReal = sp.periodSum / sp.divider;
      /* Prepare the periodSum for the next iteration. */
      sp.periodSum -= sp.periodSub;
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode WMA_OpenImpl( WMA_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      double divider = 0;
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
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( optInTimePeriod == 1 ) {
         int fillLb = WMA_Lookback(optInTimePeriod);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.periodSum = 0.0;
         sp.periodSub = 0.0;
         sp.trailingValue = 0.0;
         sp.divider = 0.0;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = 0;
         sp.ring_trailingIdx_inReal = new double[1];
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
      lookbackTotal = optInTimePeriod - 1;
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
      /* Weighted denominator 1+2+...+n = n(n+1)/2. Computed in double: the
       * int product n*(n+1) overflows int32 at n>=46341 (#142).
       */
      divider = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
      /* The algo used here use a very basic property of
       * multiplication/addition: (x*2) = x+x
       *
       * As an example, a 3 period weighted can be
       * interpreted in two way:
       *  (x1*1)+(x2*2)+(x3*3)
       *      OR
       *  x1+x2+x2+x3+x3+x3 (this is the periodSum)
       *
       * When you move forward in the time serie
       * you can quickly adjust the periodSum for the
       * period by substracting:
       *   x1+x2+x3 (This is the periodSub)
       * Making the new periodSum equals to:
       *   x2+x3+x3
       *
       * You can then add the new price bar
       * which is x4+x4+x4 giving:
       *   x2+x3+x3+x4+x4+x4
       *
       * At this point one iteration is completed and you can
       * see that we are back to the step 1 of this example.
       *
       * Why making it so un-intuitive? The number of memory
       * access and floating point operations are kept to a
       * minimum with this algo.
       */
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      /* Evaluate the initial periodSum/periodSub and trailingValue. */
      periodSub = (double)0.0;
      periodSum = periodSub;
      inIdx = trailingIdx;
      i = 1;
      while( inIdx < startIdx ) {
         tempReal = inReal[inIdx++];
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
      }
      trailingValue = 0.0;
      /* Tight loop for the requested range. */
      while( inIdx <= endIdx ) {
         /* Add the current price bar to the sum
          * who are carried through the iterations.
          */
         tempReal = inReal[inIdx++];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         /* Save the trailing value for being substract at
          * the next iteration.
          * (must be saved here just in case outReal and
          *  inReal are the same buffer).
          */
         trailingValue = inReal[trailingIdx++];
         /* Calculate the WMA for this price bar. */
         outReal[outIdx++ * outStride] = periodSum / divider;
         /* Prepare the periodSum for the next iteration. */
         periodSum -= periodSub;
      }
      /* Set output limits. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = inIdx - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.periodSum = periodSum;
      sp.periodSub = periodSub;
      sp.trailingValue = trailingValue;
      sp.divider = divider;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* WMA_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   WMA_Stream WMA_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      WMA_Stream sp = new WMA_Stream(this);
      RetCode retCode = WMA_OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WMA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("WMA openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind WMA_Open (composition seam). */
   WMA_Stream WMA_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      WMA_Stream sp = new WMA_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = WMA_OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WMA open: internal error", retCode);
      }
      throw new TaLibArgumentException("WMA open: " + retCode, retCode);
   }
   /**
    * Open a live WMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#WMA} at that bar.
    * <p>The history must hold at least {@code WMA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public WMA_Stream WMA_Open( double inReal[], int optInTimePeriod )
   {
      return WMA_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#WMA_Open} that also fills the output array(s) bit-identically
    * to {@link Core#WMA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link WMA_Stream#outRange()}.
    */
   public WMA_Stream WMA_OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("WMA openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return WMA_OpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
