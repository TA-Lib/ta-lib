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
   RetCode WMA_Internal( int startIdx,
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
   RetCode WMA_Internal( int startIdx,
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
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
      final Core core;
      int optInTimePeriod;
      double periodSum;
      double periodSub;
      double trailingValue;
      double divider;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      WMA_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#WMA_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

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
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.WMA_StreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         WMA_Stream scratch = new WMA_Stream(this);
         core.WMA_StreamStep(scratch, inReal);
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
   void WMA_StreamStep( WMA_Stream sp, double inReal )
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
   private RetCode WMA_OpenBody( WMA_Stream sp, double inReal[], int startIdx, int optInTimePeriod )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
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
      if( optInTimePeriod == 1 ) {
         if( historyLen < WMA_Lookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.periodSum = 0.0;
         sp.periodSub = 0.0;
         sp.trailingValue = 0.0;
         sp.divider = 0.0;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = 0;
         sp.ring_trailingIdx_inReal = new double[1];
         sp.cur_outReal = inReal[historyLen - 1];
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
         return RetCode.OutOfRangeEndIndex ;
      }
      /* To make the rest more efficient, handle exception
       * case where the user is asking for a period of '1'.
       * In that case outputs equals inputs for the requested
       * range.
       */
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
         lastValue_outReal = periodSum / divider;
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
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode WMA_OpenAndFillBody( WMA_Stream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      int startIdx = 0;
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
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         if( historyLen < WMA_Lookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.periodSum = 0.0;
         sp.periodSub = 0.0;
         sp.trailingValue = 0.0;
         sp.divider = 0.0;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = 0;
         sp.ring_trailingIdx_inReal = new double[1];
         int fillLb = WMA_Lookback(optInTimePeriod);
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
            outReal[fillIdx] = inReal[fillLb + fillIdx];
         }
         sp.cur_outReal = outReal[outNBElement.value - 1];
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
         return RetCode.OutOfRangeEndIndex ;
      }
      /* To make the rest more efficient, handle exception
       * case where the user is asking for a period of '1'.
       * In that case outputs equals inputs for the requested
       * range.
       */
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
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind WMA_Open (composition seam). */
   WMA_Stream WMA_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      WMA_Stream sp = new WMA_Stream(this);
      RetCode retCode = WMA_OpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("WMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("WMA open: internal error");
      }
      throw new IllegalArgumentException("WMA open: " + retCode);
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
    * {@link WMA_Stream#fillRange()}.
    */
   public WMA_Stream WMA_OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      WMA_Stream sp = new WMA_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WMA_OpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("WMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("WMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("WMA openAndFill: " + retCode);
   }
