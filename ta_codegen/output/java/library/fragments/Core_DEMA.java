/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010102 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070526 MF,CC  Speed optimization: compute both EMA in a single
 *                lockstep pass (bit-exact, no temporary buffers).
 */

   public int demaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* Get lookback for one EMA.
       * Multiply by two (because double smoothing).
       */
      return emaLookback(optInTimePeriod) * 2 ;

   }
   public RetCode dema( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double prevEMA1 = 0;
      double prevEMA2 = 0;
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
      /* For an explanation of this function, please read
       *
       * Stocks & Commodities V. 12:1 (11-19):
       *   Smoothing Data With Faster Moving Averages
       * Stocks & Commodities V. 12:2 (72-80):
       *   Smoothing Data With Less Lag
       *
       * Both magazine articles written by Patrick G. Mulloy
       *
       * Essentially, a DEMA of time serie 't' is:
       *   EMA2 = EMA(EMA(t,period),period)
       *   DEMA = 2*EMA(t,period)- EMA2
       *
       * DEMA offers a moving average with less lags then the
       * traditional EMA.
       *
       * Do not confuse a DEMA with the EMA2. Both are called
       * "Double EMA" in the litterature, but EMA2 is a simple
       * EMA of an EMA, while DEMA is a compostie of a single
       * EMA with EMA2.
       *
       * TEMA is very similar (and from the same author).
       */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 2;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* Both EMA are computed in a single lockstep pass: each new
       * EMA1 value is immediately fed into EMA2. No temporary
       * buffers are needed.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum
       *    of its first 'period' inputs, accumulated from 0.0 in
       *    input order (0.0+x is not x for x=-0.0), divided by
       *    the period.
       *  - Metastock compatibility: EMA1 is seeded from inReal[0],
       *    EMA2 from the first EMA1 value.
       * Output alignment is identical for all compatibility modes;
       * only the seed values differ.
       *
       * In-place (inReal == outReal) is supported: outReal[outIdx]
       * is written only after inReal[startIdx+outIdx] was read.
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
         while( today <= startIdx - lookbackEMA ) {
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
         /* Metastock/Tradestation: seed each EMA with its first
          * input value: EMA1 from inReal[0], EMA2 from the first
          * EMA1 value.
          */
         prevEMA1 = inReal[0];
         today = 1;
         while( today <= startIdx - lookbackEMA ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance both EMA in lockstep through the unstable period
       * of EMA2, up to the first output bar.
       */
      while( today <= startIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      /* Stable zone: keep advancing both EMA in lockstep and
       * write the DEMA into the output.
       */
      outReal[0] = 2.0 * prevEMA1 - prevEMA2;
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         outReal[outIdx++] = 2.0 * prevEMA1 - prevEMA2;
      }
      /* Succeed. Indicate where the output starts relative to
       * the caller input.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode demaUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double prevEMA1 = 0;
      double prevEMA2 = 0;
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
      lookbackTotal = lookbackEMA * 2;
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
         while( today <= startIdx - lookbackEMA ) {
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
         while( today <= startIdx - lookbackEMA ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      outReal[0] = 2.0 * prevEMA1 - prevEMA2;
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         outReal[outIdx++] = 2.0 * prevEMA1 - prevEMA2;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode dema( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double prevEMA1 = 0;
      double prevEMA2 = 0;
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
      lookbackTotal = lookbackEMA * 2;
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
         while( today <= startIdx - lookbackEMA ) {
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
         while( today <= startIdx - lookbackEMA ) {
            prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      outReal[0] = 2.0 * prevEMA1 - prevEMA2;
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         outReal[outIdx++] = 2.0 * prevEMA1 - prevEMA2;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode demaUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double prevEMA1 = 0;
      double prevEMA2 = 0;
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
      lookbackTotal = lookbackEMA * 2;
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
         while( today <= startIdx - lookbackEMA ) {
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
         while( today <= startIdx - lookbackEMA ) {
            prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      outReal[0] = 2.0 * prevEMA1 - prevEMA2;
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         outReal[outIdx++] = 2.0 * prevEMA1 - prevEMA2;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live DEMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#dema} over the same series.
    * Open with {@link Core#demaOpen}; there is no close — the handle is
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
   public static final class DemaStream {
      final Core core;
      int optInTimePeriod;
      double prevEMA1;
      double prevEMA2;
      double optInK_1;
      double cur_outReal;

      DemaStream( Core core ) { this.core = core; }

      DemaStream( DemaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevEMA1 = other.prevEMA1;
         this.prevEMA2 = other.prevEMA2;
         this.optInK_1 = other.optInK_1;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.demaStreamStep(this, inReal);
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
         DemaStream scratch = new DemaStream(this);
         core.demaStreamStep(scratch, inReal);
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
      public DemaStream copy() {
         return new DemaStream(this);
      }
   }
   void demaStreamStep( DemaStream sp, double inReal )
   {
      sp.prevEMA1 = Math.fma(inReal - sp.prevEMA1, sp.optInK_1, sp.prevEMA1);
      sp.prevEMA2 = Math.fma(sp.prevEMA1 - sp.prevEMA2, sp.optInK_1, sp.prevEMA2);
      sp.cur_outReal = 2.0 * sp.prevEMA1 - sp.prevEMA2;
   }
   private RetCode demaOpenBody( DemaStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      double prevEMA1 = 0;
      double prevEMA2 = 0;
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
      /* For an explanation of this function, please read
       *
       * Stocks & Commodities V. 12:1 (11-19):
       *   Smoothing Data With Faster Moving Averages
       * Stocks & Commodities V. 12:2 (72-80):
       *   Smoothing Data With Less Lag
       *
       * Both magazine articles written by Patrick G. Mulloy
       *
       * Essentially, a DEMA of time serie 't' is:
       *   EMA2 = EMA(EMA(t,period),period)
       *   DEMA = 2*EMA(t,period)- EMA2
       *
       * DEMA offers a moving average with less lags then the
       * traditional EMA.
       *
       * Do not confuse a DEMA with the EMA2. Both are called
       * "Double EMA" in the litterature, but EMA2 is a simple
       * EMA of an EMA, while DEMA is a compostie of a single
       * EMA with EMA2.
       *
       * TEMA is very similar (and from the same author).
       */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 2;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Both EMA are computed in a single lockstep pass: each new
       * EMA1 value is immediately fed into EMA2. No temporary
       * buffers are needed.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum
       *    of its first 'period' inputs, accumulated from 0.0 in
       *    input order (0.0+x is not x for x=-0.0), divided by
       *    the period.
       *  - Metastock compatibility: EMA1 is seeded from inReal[0],
       *    EMA2 from the first EMA1 value.
       * Output alignment is identical for all compatibility modes;
       * only the seed values differ.
       *
       * In-place (inReal == outReal) is supported: outReal[outIdx]
       * is written only after inReal[startIdx+outIdx] was read.
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
         while( today <= startIdx - lookbackEMA ) {
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
         /* Metastock/Tradestation: seed each EMA with its first
          * input value: EMA1 from inReal[0], EMA2 from the first
          * EMA1 value.
          */
         prevEMA1 = inReal[0];
         today = 1;
         while( today <= startIdx - lookbackEMA ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance both EMA in lockstep through the unstable period
       * of EMA2, up to the first output bar.
       */
      while( today <= startIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      /* Stable zone: keep advancing both EMA in lockstep and
       * write the DEMA into the output.
       */
      lastValue_outReal = 2.0 * prevEMA1 - prevEMA2;
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         lastValue_outReal = 2.0 * prevEMA1 - prevEMA2;
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
      sp.optInK_1 = optInK_1;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode demaOpenAndFillBody( DemaStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double prevEMA1 = 0;
      double prevEMA2 = 0;
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
      /* For an explanation of this function, please read
       *
       * Stocks & Commodities V. 12:1 (11-19):
       *   Smoothing Data With Faster Moving Averages
       * Stocks & Commodities V. 12:2 (72-80):
       *   Smoothing Data With Less Lag
       *
       * Both magazine articles written by Patrick G. Mulloy
       *
       * Essentially, a DEMA of time serie 't' is:
       *   EMA2 = EMA(EMA(t,period),period)
       *   DEMA = 2*EMA(t,period)- EMA2
       *
       * DEMA offers a moving average with less lags then the
       * traditional EMA.
       *
       * Do not confuse a DEMA with the EMA2. Both are called
       * "Double EMA" in the litterature, but EMA2 is a simple
       * EMA of an EMA, while DEMA is a compostie of a single
       * EMA with EMA2.
       *
       * TEMA is very similar (and from the same author).
       */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 2;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Both EMA are computed in a single lockstep pass: each new
       * EMA1 value is immediately fed into EMA2. No temporary
       * buffers are needed.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum
       *    of its first 'period' inputs, accumulated from 0.0 in
       *    input order (0.0+x is not x for x=-0.0), divided by
       *    the period.
       *  - Metastock compatibility: EMA1 is seeded from inReal[0],
       *    EMA2 from the first EMA1 value.
       * Output alignment is identical for all compatibility modes;
       * only the seed values differ.
       *
       * In-place (inReal == outReal) is supported: outReal[outIdx]
       * is written only after inReal[startIdx+outIdx] was read.
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
         while( today <= startIdx - lookbackEMA ) {
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
         /* Metastock/Tradestation: seed each EMA with its first
          * input value: EMA1 from inReal[0], EMA2 from the first
          * EMA1 value.
          */
         prevEMA1 = inReal[0];
         today = 1;
         while( today <= startIdx - lookbackEMA ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance both EMA in lockstep through the unstable period
       * of EMA2, up to the first output bar.
       */
      while( today <= startIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
      }
      /* Stable zone: keep advancing both EMA in lockstep and
       * write the DEMA into the output.
       */
      outReal[0] = 2.0 * prevEMA1 - prevEMA2;
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         outReal[outIdx++] = 2.0 * prevEMA1 - prevEMA2;
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
      sp.optInK_1 = optInK_1;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind demaOpen (composition seam). */
   DemaStream demaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      DemaStream sp = new DemaStream(this);
      RetCode retCode = demaOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_DEMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_DEMA open: internal error");
      }
      throw new IllegalArgumentException("TA_DEMA open: " + retCode);
   }
   /**
    * Open a live DEMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#dema} at that bar.
    * <p>The history must hold at least {@code demaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public DemaStream demaOpen( double inReal[], int optInTimePeriod )
   {
      return demaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#demaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#dema} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public DemaStream demaOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      DemaStream sp = new DemaStream(this);
      RetCode retCode = demaOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_DEMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_DEMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_DEMA openAndFill: " + retCode);
   }
