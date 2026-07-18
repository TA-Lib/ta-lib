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

   public int trixLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int emaLookback;
      emaLookback = emaLookback(optInTimePeriod);
      return emaLookback * 3 + rocRLookback(1) ;

   }
   public RetCode trix( int startIdx,
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
      /* TRIX = 1-day percent rate-of-change of a triple EMA. */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3 + rocRLookback(1);
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
      if( this.compatibility == Compatibility.Default ) {
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
      } else {
         /* Metastock/Tradestation: seed EMA1 from the first price
          * bar, EMA2 from the first EMA1 value.
          */
         prevEMA1 = inReal[0];
         today = 1;
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance EMA1 and EMA2 in lockstep through the unstable
       * period of EMA2, up to the bar where EMA3 seeding begins.
       */
      while( today <= startIdx - (lookbackEMA + 1) ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      if( this.compatibility == Compatibility.Default ) {
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
      } else {
         /* Metastock/Tradestation: seed EMA3 from the first EMA2
          * value.
          */
         prevEMA3 = prevEMA2;
      }
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
   public RetCode trixUnguarded( int startIdx,
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
      outNBElement.value = 0;
      outBegIdx.value = 0;
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3 + rocRLookback(1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      if( this.compatibility == Compatibility.Default ) {
         today = startIdx - lookbackTotal;
         i = optInTimePeriod;
         tempReal = 0.0;
         while( i-- > 0 ) {
            tempReal += inReal[today++];
         }
         prevEMA1 = tempReal / optInTimePeriod;
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         tempReal = 0.0;
         tempReal += prevEMA1;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
            tempReal += prevEMA1;
         }
         prevEMA2 = tempReal / optInTimePeriod;
      } else {
         prevEMA1 = inReal[0];
         today = 1;
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - (lookbackEMA + 1) ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      if( this.compatibility == Compatibility.Default ) {
         tempReal = 0.0;
         tempReal += prevEMA2;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
            prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
            tempReal += prevEMA2;
         }
         prevEMA3 = tempReal / optInTimePeriod;
      } else {
         prevEMA3 = prevEMA2;
      }
      while( today <= startIdx - 1 ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
      }
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
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode trix( int startIdx,
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
      outNBElement.value = 0;
      outBegIdx.value = 0;
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3 + rocRLookback(1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      if( this.compatibility == Compatibility.Default ) {
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
      } else {
         prevEMA1 = (double)inReal[0];
         today = 1;
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - (lookbackEMA + 1) ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      if( this.compatibility == Compatibility.Default ) {
         tempReal = 0.0;
         tempReal += prevEMA2;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
            prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
            tempReal += prevEMA2;
         }
         prevEMA3 = tempReal / optInTimePeriod;
      } else {
         prevEMA3 = prevEMA2;
      }
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
   public RetCode trixUnguarded( int startIdx,
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
      outNBElement.value = 0;
      outBegIdx.value = 0;
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3 + rocRLookback(1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      if( this.compatibility == Compatibility.Default ) {
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
      } else {
         prevEMA1 = (double)inReal[0];
         today = 1;
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - (lookbackEMA + 1) ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      if( this.compatibility == Compatibility.Default ) {
         tempReal = 0.0;
         tempReal += prevEMA2;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
            prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
            tempReal += prevEMA2;
         }
         prevEMA3 = tempReal / optInTimePeriod;
      } else {
         prevEMA3 = prevEMA2;
      }
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
/**** Streaming API *****/

   /**
    * A live TRIX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#trix} over the same series.
    * Open with {@link Core#trixOpen}; there is no close — the handle is
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
   public static final class TrixStream {
      final Core core;
      int optInTimePeriod;
      double prevEMA1;
      double prevEMA2;
      double prevEMA3;
      double optInK_1;
      double cur_outReal;

      TrixStream( Core core ) { this.core = core; }

      TrixStream( TrixStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevEMA1 = other.prevEMA1;
         this.prevEMA2 = other.prevEMA2;
         this.prevEMA3 = other.prevEMA3;
         this.optInK_1 = other.optInK_1;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.trixStreamStep(this, inReal);
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
         TrixStream scratch = new TrixStream(this);
         core.trixStreamStep(scratch, inReal);
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
      public TrixStream copy() {
         return new TrixStream(this);
      }
   }
   void trixStreamStep( TrixStream sp, double inReal )
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
   private RetCode trixOpenBody( TrixStream sp, double inReal[], int startIdx, int optInTimePeriod )
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
      /* TRIX = 1-day percent rate-of-change of a triple EMA. */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3 + rocRLookback(1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
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
      if( this.compatibility == Compatibility.Default ) {
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
      } else {
         /* Metastock/Tradestation: seed EMA1 from the first price
          * bar, EMA2 from the first EMA1 value.
          */
         prevEMA1 = inReal[0];
         today = 1;
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance EMA1 and EMA2 in lockstep through the unstable
       * period of EMA2, up to the bar where EMA3 seeding begins.
       */
      while( today <= startIdx - (lookbackEMA + 1) ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      if( this.compatibility == Compatibility.Default ) {
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
      } else {
         /* Metastock/Tradestation: seed EMA3 from the first EMA2
          * value.
          */
         prevEMA3 = prevEMA2;
      }
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
            lastValue_outReal = (prevEMA3 / tempReal - 1.0) * 100.0;
         } else {
            lastValue_outReal = 0.0;
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
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode trixOpenAndFillBody( TrixStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      /* TRIX = 1-day percent rate-of-change of a triple EMA. */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3 + rocRLookback(1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
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
      if( this.compatibility == Compatibility.Default ) {
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
      } else {
         /* Metastock/Tradestation: seed EMA1 from the first price
          * bar, EMA2 from the first EMA1 value.
          */
         prevEMA1 = inReal[0];
         today = 1;
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance EMA1 and EMA2 in lockstep through the unstable
       * period of EMA2, up to the bar where EMA3 seeding begins.
       */
      while( today <= startIdx - (lookbackEMA + 1) ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      if( this.compatibility == Compatibility.Default ) {
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
      } else {
         /* Metastock/Tradestation: seed EMA3 from the first EMA2
          * value.
          */
         prevEMA3 = prevEMA2;
      }
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
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.prevEMA1 = prevEMA1;
      sp.prevEMA2 = prevEMA2;
      sp.prevEMA3 = prevEMA3;
      sp.optInK_1 = optInK_1;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind trixOpen (composition seam). */
   TrixStream trixOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      TrixStream sp = new TrixStream(this);
      RetCode retCode = trixOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_TRIX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_TRIX open: internal error");
      }
      throw new IllegalArgumentException("TA_TRIX open: " + retCode);
   }
   /**
    * Open a live TRIX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#trix} at that bar.
    * <p>The history must hold at least {@code trixLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public TrixStream trixOpen( double inReal[], int optInTimePeriod )
   {
      return trixOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#trixOpen} that also fills the output array(s) bit-identically
    * to {@link Core#trix} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public TrixStream trixOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      TrixStream sp = new TrixStream(this);
      RetCode retCode = trixOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_TRIX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_TRIX openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_TRIX openAndFill: " + retCode);
   }
