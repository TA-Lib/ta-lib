/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  062704 MF     Fix limit case to avoid divid by zero (or by
 *                a value close to zero induce by the imprecision
 *                of floating points).
 *  070226 MF,CC  Allow period of 1: output is a copy of the input,
 *                consistent with TA_MA (issues #48, #59). The natural
 *                KAMA math at period=1 would be a fixed-alpha EMA
 *                (efficiency ratio is always 1), which would disagree
 *                with TA_MA's period-1 copy, so identity is explicit.
 */

   public int kamaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInTimePeriod == 1 ) {
         return this.unstablePeriod[FuncUnstId.Kama.ordinal()] ;
      }
      return optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()] ;

   }
   public RetCode kama( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double trailingValue = 0;
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
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      /* Default return values */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). The unstable period
       * still delays the first output for API consistency.
       */
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.Kama.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.Success ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()];
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
      /* Initialize the variables by going through
       * the lookback period.
       */
      sumROC1 = 0.0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         tempReal -= inReal[today];
         sumROC1 += Math.abs(tempReal);
      }
      /* At this point sumROC1 represent the
       * summation of the 1-day price difference
       * over the (optInTimePeriod-1)
       */
      /* Calculate the first KAMA */
      /* The yesterday price is used here as the previous KAMA. */
      prevKAMA = inReal[today - 1];
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      /* Save the trailing value. Do this because inReal
       * and outReal can be pointers to the same buffer.
       */
      trailingValue = tempReal2;
      /* Calculate the efficiency ratio */
      if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      /* Calculate the smoothing constant */
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      /* Calculate the KAMA like an EMA, using the
       * smoothing constant as the adaptive factor.
       */
      prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      /* 'today' keep track of where the processing is within the
       * input.
       */
      /* Skip the unstable period. Do the whole processing
       * needed for KAMA, but do not write it in the output.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      /* Write the first value. */
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      /* Do the KAMA calculation for the requested range. */
      while( today <= endIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode kamaUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double trailingValue = 0;
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.Kama.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.Success ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()];
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumROC1 = 0.0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         tempReal -= inReal[today];
         sumROC1 += Math.abs(tempReal);
      }
      prevKAMA = inReal[today - 1];
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      trailingValue = tempReal2;
      if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      while( today <= startIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      while( today <= endIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode kama( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double trailingValue = 0;
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
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.Kama.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.Success ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()];
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumROC1 = 0.0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = (double)inReal[today++];
         tempReal -= (double)inReal[today];
         sumROC1 += Math.abs(tempReal);
      }
      prevKAMA = (double)inReal[today - 1];
      tempReal = (double)inReal[today];
      tempReal2 = (double)inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      trailingValue = tempReal2;
      if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
      while( today <= startIdx ) {
         tempReal = (double)inReal[today];
         tempReal2 = (double)inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - (double)inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today];
         tempReal2 = (double)inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - (double)inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode kamaUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double trailingValue = 0;
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.Kama.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.Success ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()];
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumROC1 = 0.0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = (double)inReal[today++];
         tempReal -= (double)inReal[today];
         sumROC1 += Math.abs(tempReal);
      }
      prevKAMA = (double)inReal[today - 1];
      tempReal = (double)inReal[today];
      tempReal2 = (double)inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      trailingValue = tempReal2;
      if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
      while( today <= startIdx ) {
         tempReal = (double)inReal[today];
         tempReal2 = (double)inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - (double)inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today];
         tempReal2 = (double)inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - (double)inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live KAMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#kama} over the same series.
    * Open with {@link Core#kamaOpen}; there is no close — the handle is
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
   public static final class KamaStream {
      final Core core;
      int optInTimePeriod;
      double constMax;
      double constDiff;
      double sumROC1;
      double prevKAMA;
      double trailingValue;
      double lag1_inReal;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;

      KamaStream( Core core ) { this.core = core; }

      KamaStream( KamaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.constMax = other.constMax;
         this.constDiff = other.constDiff;
         this.sumROC1 = other.sumROC1;
         this.prevKAMA = other.prevKAMA;
         this.trailingValue = other.trailingValue;
         this.lag1_inReal = other.lag1_inReal;
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
         core.kamaStreamStep(this, inReal);
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
         KamaStream scratch = new KamaStream(this);
         core.kamaStreamStep(scratch, inReal);
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
      public KamaStream copy() {
         return new KamaStream(this);
      }
   }
   void kamaStreamStep( KamaStream sp, double inReal )
   {
      double tempReal = 0.0;
      double tempReal2 = 0.0;
      double periodROC = 0.0;
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      tempReal = inReal;
      tempReal2 = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      periodROC = tempReal - tempReal2;
      /* Adjust sumROC1:
       *  - Remove trailing ROC1
       *  - Add new ROC1
       */
      sp.sumROC1 -= Math.abs(sp.trailingValue - tempReal2);
      sp.sumROC1 += Math.abs(tempReal - sp.lag1_inReal);
      /* Save the trailing value. Do this because inReal
       * and outReal can be pointers to the same buffer.
       */
      sp.trailingValue = tempReal2;
      /* Calculate the efficiency ratio */
      if( sp.sumROC1 <= periodROC || ((-0.00000000000001 < sp.sumROC1) && (sp.sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sp.sumROC1);
      }
      /* Calculate the smoothing constant */
      tempReal = Math.fma(tempReal, sp.constDiff, sp.constMax);
      tempReal *= tempReal;
      /* Calculate the KAMA like an EMA, using the
       * smoothing constant as the adaptive factor.
       */
      sp.prevKAMA = Math.fma(inReal - sp.prevKAMA, tempReal, sp.prevKAMA);
      sp.cur_outReal = sp.prevKAMA;
      sp.lag1_inReal = inReal;
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode kamaOpenBody( KamaStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double trailingValue = 0;
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
         if( historyLen < kamaLookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.constMax = 0.0;
         sp.constDiff = 0.0;
         sp.sumROC1 = 0.0;
         sp.prevKAMA = 0.0;
         sp.trailingValue = 0.0;
         sp.lag1_inReal = 0.0;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = 0;
         sp.ring_trailingIdx_inReal = new double[1];
         sp.cur_outReal = inReal[historyLen - 1];
         return RetCode.Success;
      }
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      /* Default return values */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). The unstable period
       * still delays the first output for API consistency.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()];
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
      /* Initialize the variables by going through
       * the lookback period.
       */
      sumROC1 = 0.0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         tempReal -= inReal[today];
         sumROC1 += Math.abs(tempReal);
      }
      /* At this point sumROC1 represent the
       * summation of the 1-day price difference
       * over the (optInTimePeriod-1)
       */
      /* Calculate the first KAMA */
      /* The yesterday price is used here as the previous KAMA. */
      prevKAMA = inReal[today - 1];
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      /* Save the trailing value. Do this because inReal
       * and outReal can be pointers to the same buffer.
       */
      trailingValue = tempReal2;
      /* Calculate the efficiency ratio */
      if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      /* Calculate the smoothing constant */
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      /* Calculate the KAMA like an EMA, using the
       * smoothing constant as the adaptive factor.
       */
      prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      /* 'today' keep track of where the processing is within the
       * input.
       */
      /* Skip the unstable period. Do the whole processing
       * needed for KAMA, but do not write it in the output.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      /* Write the first value. */
      lastValue_outReal = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      /* Do the KAMA calculation for the requested range. */
      while( today <= endIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
         lastValue_outReal = prevKAMA;
      }
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
      sp.constMax = constMax;
      sp.constDiff = constDiff;
      sp.sumROC1 = sumROC1;
      sp.prevKAMA = prevKAMA;
      sp.trailingValue = trailingValue;
      sp.lag1_inReal = inReal[historyLen - 1];
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode kamaOpenAndFillBody( KamaStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double trailingValue = 0;
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
         if( historyLen < kamaLookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.constMax = 0.0;
         sp.constDiff = 0.0;
         sp.sumROC1 = 0.0;
         sp.prevKAMA = 0.0;
         sp.trailingValue = 0.0;
         sp.lag1_inReal = 0.0;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = 0;
         sp.ring_trailingIdx_inReal = new double[1];
         int fillLb = kamaLookback(optInTimePeriod);
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
            outReal[fillIdx] = inReal[fillLb + fillIdx];
         }
         sp.cur_outReal = outReal[outNBElement.value - 1];
         return RetCode.Success;
      }
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      /* Default return values */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). The unstable period
       * still delays the first output for API consistency.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()];
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
      /* Initialize the variables by going through
       * the lookback period.
       */
      sumROC1 = 0.0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         tempReal -= inReal[today];
         sumROC1 += Math.abs(tempReal);
      }
      /* At this point sumROC1 represent the
       * summation of the 1-day price difference
       * over the (optInTimePeriod-1)
       */
      /* Calculate the first KAMA */
      /* The yesterday price is used here as the previous KAMA. */
      prevKAMA = inReal[today - 1];
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      /* Save the trailing value. Do this because inReal
       * and outReal can be pointers to the same buffer.
       */
      trailingValue = tempReal2;
      /* Calculate the efficiency ratio */
      if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      /* Calculate the smoothing constant */
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      /* Calculate the KAMA like an EMA, using the
       * smoothing constant as the adaptive factor.
       */
      prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      /* 'today' keep track of where the processing is within the
       * input.
       */
      /* Skip the unstable period. Do the whole processing
       * needed for KAMA, but do not write it in the output.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      /* Write the first value. */
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      /* Do the KAMA calculation for the requested range. */
      while( today <= endIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
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
      sp.constMax = constMax;
      sp.constDiff = constDiff;
      sp.sumROC1 = sumROC1;
      sp.prevKAMA = prevKAMA;
      sp.trailingValue = trailingValue;
      sp.lag1_inReal = inReal[historyLen - 1];
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind kamaOpen (composition seam). */
   KamaStream kamaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      KamaStream sp = new KamaStream(this);
      RetCode retCode = kamaOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_KAMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_KAMA open: internal error");
      }
      throw new IllegalArgumentException("TA_KAMA open: " + retCode);
   }
   /**
    * Open a live KAMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#kama} at that bar.
    * <p>The history must hold at least {@code kamaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public KamaStream kamaOpen( double inReal[], int optInTimePeriod )
   {
      return kamaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#kamaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#kama} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public KamaStream kamaOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      KamaStream sp = new KamaStream(this);
      RetCode retCode = kamaOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_KAMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_KAMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_KAMA openAndFill: " + retCode);
   }
