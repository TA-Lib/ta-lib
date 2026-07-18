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

   public int wmaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode wma( int startIdx,
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
      int divider = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      int lookbackTotal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
         System.arraycopy(inReal, startIdx, outReal, 0, (int)outNBElement.value * 1);
         return RetCode.Success ;
      }
      /* Calculate the divider (always an integer value).
       * By induction: 1+2+3+4+'n' = n(n+1)/2
       * '>>1' is usually faster than '/2' for unsigned.
       */
      divider = optInTimePeriod * (optInTimePeriod + 1) >> 1;
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
   public RetCode wmaUnguarded( int startIdx,
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
      int divider = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      int lookbackTotal = 0;
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
         System.arraycopy(inReal, startIdx, outReal, 0, (int)outNBElement.value * 1);
         return RetCode.Success ;
      }
      divider = optInTimePeriod * (optInTimePeriod + 1) >> 1;
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
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
      while( inIdx <= endIdx ) {
         tempReal = inReal[inIdx++];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         trailingValue = inReal[trailingIdx++];
         outReal[outIdx++] = periodSum / divider;
         periodSum -= periodSub;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode wma( int startIdx,
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
      int divider = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      int lookbackTotal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
         System.arraycopy(inReal, startIdx, outReal, 0, (int)outNBElement.value * 1);
         return RetCode.Success ;
      }
      divider = optInTimePeriod * (optInTimePeriod + 1) >> 1;
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
   public RetCode wmaUnguarded( int startIdx,
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
      int divider = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      int lookbackTotal = 0;
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
         System.arraycopy(inReal, startIdx, outReal, 0, (int)outNBElement.value * 1);
         return RetCode.Success ;
      }
      divider = optInTimePeriod * (optInTimePeriod + 1) >> 1;
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
/**** Streaming API *****/

   /**
    * A live WMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#wma} over the same series.
    * Open with {@link Core#wmaOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class WmaStream {
      final Core core;
      int optInTimePeriod;
      int divider;
      double periodSum;
      double periodSub;
      double trailingValue;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;

      WmaStream( Core core ) { this.core = core; }

      WmaStream( WmaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.divider = other.divider;
         this.periodSum = other.periodSum;
         this.periodSub = other.periodSub;
         this.trailingValue = other.trailingValue;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.wmaStreamStep(this, inReal);
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
         WmaStream scratch = new WmaStream(this);
         core.wmaStreamStep(scratch, inReal);
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
      public WmaStream copy() {
         return new WmaStream(this);
      }
   }
   void wmaStreamStep( WmaStream sp, double inReal )
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
   private RetCode wmaOpenBody( WmaStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      int divider = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         if( historyLen < wmaLookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.divider = 0;
         sp.periodSum = 0.0;
         sp.periodSub = 0.0;
         sp.trailingValue = 0.0;
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
      /* Calculate the divider (always an integer value).
       * By induction: 1+2+3+4+'n' = n(n+1)/2
       * '>>1' is usually faster than '/2' for unsigned.
       */
      divider = optInTimePeriod * (optInTimePeriod + 1) >> 1;
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
      sp.divider = divider;
      sp.periodSum = periodSum;
      sp.periodSub = periodSub;
      sp.trailingValue = trailingValue;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode wmaOpenAndFillBody( WmaStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      int divider = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      int lookbackTotal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
         if( historyLen < wmaLookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.divider = 0;
         sp.periodSum = 0.0;
         sp.periodSub = 0.0;
         sp.trailingValue = 0.0;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = 0;
         sp.ring_trailingIdx_inReal = new double[1];
         int fillLb = wmaLookback(optInTimePeriod);
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
      /* Calculate the divider (always an integer value).
       * By induction: 1+2+3+4+'n' = n(n+1)/2
       * '>>1' is usually faster than '/2' for unsigned.
       */
      divider = optInTimePeriod * (optInTimePeriod + 1) >> 1;
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
      sp.divider = divider;
      sp.periodSum = periodSum;
      sp.periodSub = periodSub;
      sp.trailingValue = trailingValue;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind wmaOpen (composition seam). */
   WmaStream wmaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      WmaStream sp = new WmaStream(this);
      RetCode retCode = wmaOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_WMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_WMA open: internal error");
      }
      throw new IllegalArgumentException("TA_WMA open: " + retCode);
   }
   /**
    * Open a live WMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#wma} at that bar.
    * <p>The history must hold at least {@code wmaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public WmaStream wmaOpen( double inReal[], int optInTimePeriod )
   {
      return wmaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#wmaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#wma} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public WmaStream wmaOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      WmaStream sp = new WmaStream(this);
      RetCode retCode = wmaOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_WMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_WMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_WMA openAndFill: " + retCode);
   }
