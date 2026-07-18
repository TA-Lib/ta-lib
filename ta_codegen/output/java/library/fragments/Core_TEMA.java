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
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070226 MF,CC  Allow period of 1: output is an exact copy of the
 *                input, consistent with TA_MA (issues #48, #59). The
 *                natural math (3*e1 - 3*e2 + e3 with e1=e2=e3=x) is
 *                exact on x86 but not under FMA contraction (ARM64
 *                clang leaves ~1e-14 residue), so the copy is explicit.
 *  070526 MF,CC  Speed optimization: compute the three EMA in a single
 *                lockstep pass (bit-exact, no temporary buffers).
 */

   public int temaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int retValue;
      /* Get lookack for one EMA. */
      retValue = emaLookback(optInTimePeriod);
      return retValue * 3 ;

   }
   public RetCode tema( int startIdx,
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
      /* For an explanation of this function, please read:
       *
       * Stocks & Commodities V. 12:1 (11-19):
       *   Smoothing Data With Faster Moving Averages
       * Stocks & Commodities V. 12:2 (72-80):
       *   Smoothing Data With Less Lag
       *
       * Both magazine articles written by Patrick G. Mulloy
       *
       * Essentially, a TEMA of time serie 't' is:
       *   EMA1 = EMA(t,period)
       *   EMA2 = EMA(EMA(t,period),period)
       *   EMA3 = EMA(EMA(EMA(t,period),period))
       *   TEMA = 3*EMA1 - 3*EMA2 + EMA3
       *
       * TEMA offers a moving average with less lags then the
       * traditional EMA.
       *
       * Do not confuse a TEMA with EMA3. Both are called "Triple EMA"
       * in the litterature.
       *
       * DEMA is very similar (and from the same author).
       */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). Explicit because the
       * 3*e1 - 3*e2 + e3 composition cancels exactly only without FMA
       * contraction; ARM64 fused multiply-add leaves ~1e-14 residue.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         while( startIdx <= endIdx ) {
            outReal[outIdx++] = inReal[startIdx++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      /* The three EMA are computed in a single lockstep pass: each new
       * EMA1 value is immediately fed into EMA2, and each new EMA2 value
       * into EMA3. No temporary buffers are needed.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum
       *    of its first 'period' inputs, accumulated from 0.0 in
       *    input order (0.0+x is not x for x=-0.0), divided by
       *    the period.
       *  - Metastock compatibility: EMA1 is seeded from inReal[0],
       *    EMA2 from the first EMA1 value, EMA3 from the first EMA2
       *    value.
       *  - The combine keeps the (3.0*EMA1)-(3.0*EMA2) grouping,
       *    added to EMA3 on the left.
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
         while( today <= startIdx - lookbackEMA * 2 ) {
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
         while( today <= startIdx - lookbackEMA * 2 ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance EMA1 and EMA2 in lockstep through the unstable
       * period of EMA2, up to the bar where EMA3 seeding begins.
       */
      while( today <= startIdx - lookbackEMA ) {
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
       * period of EMA3, up to the first output bar.
       */
      while( today <= startIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
      }
      /* Stable zone: keep advancing the three EMA in lockstep and
       * write the TEMA into the output.
       */
      outReal[0] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
         outReal[outIdx++] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      }
      /* Succeed. Indicate where the output starts relative to
       * the caller input.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode temaUnguarded( int startIdx,
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
      lookbackTotal = lookbackEMA * 3;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         while( startIdx <= endIdx ) {
            outReal[outIdx++] = inReal[startIdx++];
         }
         outNBElement.value = outIdx;
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
         while( today <= startIdx - lookbackEMA * 2 ) {
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
         while( today <= startIdx - lookbackEMA * 2 ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - lookbackEMA ) {
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
      while( today <= startIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
      }
      outReal[0] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
         outReal[outIdx++] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode tema( int startIdx,
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
      lookbackTotal = lookbackEMA * 3;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         while( startIdx <= endIdx ) {
            outReal[outIdx++] = (double)inReal[startIdx++];
         }
         outNBElement.value = outIdx;
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
         while( today <= startIdx - lookbackEMA * 2 ) {
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
         while( today <= startIdx - lookbackEMA * 2 ) {
            prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - lookbackEMA ) {
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
      while( today <= startIdx ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
      }
      outReal[0] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
         outReal[outIdx++] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode temaUnguarded( int startIdx,
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
      lookbackTotal = lookbackEMA * 3;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         while( startIdx <= endIdx ) {
            outReal[outIdx++] = (double)inReal[startIdx++];
         }
         outNBElement.value = outIdx;
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
         while( today <= startIdx - lookbackEMA * 2 ) {
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
         while( today <= startIdx - lookbackEMA * 2 ) {
            prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - lookbackEMA ) {
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
      while( today <= startIdx ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
      }
      outReal[0] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma((double)inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
         outReal[outIdx++] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live TEMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#tema} over the same series.
    * Open with {@link Core#temaOpen}; there is no close — the handle is
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
   public static final class TemaStream {
      final Core core;
      int optInTimePeriod;
      double prevEMA1;
      double prevEMA2;
      double prevEMA3;
      double optInK_1;
      double cur_outReal;

      TemaStream( Core core ) { this.core = core; }

      TemaStream( TemaStream other ) {
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
         core.temaStreamStep(this, inReal);
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
         TemaStream scratch = new TemaStream(this);
         core.temaStreamStep(scratch, inReal);
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
      public TemaStream copy() {
         return new TemaStream(this);
      }
   }
   void temaStreamStep( TemaStream sp, double inReal )
   {
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      sp.prevEMA1 = Math.fma(inReal - sp.prevEMA1, sp.optInK_1, sp.prevEMA1);
      sp.prevEMA2 = Math.fma(sp.prevEMA1 - sp.prevEMA2, sp.optInK_1, sp.prevEMA2);
      sp.prevEMA3 = Math.fma(sp.prevEMA2 - sp.prevEMA3, sp.optInK_1, sp.prevEMA3);
      sp.cur_outReal = sp.prevEMA3 + (3.0 * sp.prevEMA1 - 3.0 * sp.prevEMA2);
   }
   private RetCode temaOpenBody( TemaStream sp, double inReal[], int startIdx, int optInTimePeriod )
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
      if( optInTimePeriod == 1 ) {
         if( historyLen < temaLookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevEMA1 = 0.0;
         sp.prevEMA2 = 0.0;
         sp.prevEMA3 = 0.0;
         sp.optInK_1 = 0.0;
         sp.cur_outReal = inReal[historyLen - 1];
         return RetCode.Success;
      }
      /* For an explanation of this function, please read:
       *
       * Stocks & Commodities V. 12:1 (11-19):
       *   Smoothing Data With Faster Moving Averages
       * Stocks & Commodities V. 12:2 (72-80):
       *   Smoothing Data With Less Lag
       *
       * Both magazine articles written by Patrick G. Mulloy
       *
       * Essentially, a TEMA of time serie 't' is:
       *   EMA1 = EMA(t,period)
       *   EMA2 = EMA(EMA(t,period),period)
       *   EMA3 = EMA(EMA(EMA(t,period),period))
       *   TEMA = 3*EMA1 - 3*EMA2 + EMA3
       *
       * TEMA offers a moving average with less lags then the
       * traditional EMA.
       *
       * Do not confuse a TEMA with EMA3. Both are called "Triple EMA"
       * in the litterature.
       *
       * DEMA is very similar (and from the same author).
       */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). Explicit because the
       * 3*e1 - 3*e2 + e3 composition cancels exactly only without FMA
       * contraction; ARM64 fused multiply-add leaves ~1e-14 residue.
       */
      /* The three EMA are computed in a single lockstep pass: each new
       * EMA1 value is immediately fed into EMA2, and each new EMA2 value
       * into EMA3. No temporary buffers are needed.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum
       *    of its first 'period' inputs, accumulated from 0.0 in
       *    input order (0.0+x is not x for x=-0.0), divided by
       *    the period.
       *  - Metastock compatibility: EMA1 is seeded from inReal[0],
       *    EMA2 from the first EMA1 value, EMA3 from the first EMA2
       *    value.
       *  - The combine keeps the (3.0*EMA1)-(3.0*EMA2) grouping,
       *    added to EMA3 on the left.
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
         while( today <= startIdx - lookbackEMA * 2 ) {
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
         while( today <= startIdx - lookbackEMA * 2 ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance EMA1 and EMA2 in lockstep through the unstable
       * period of EMA2, up to the bar where EMA3 seeding begins.
       */
      while( today <= startIdx - lookbackEMA ) {
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
       * period of EMA3, up to the first output bar.
       */
      while( today <= startIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
      }
      /* Stable zone: keep advancing the three EMA in lockstep and
       * write the TEMA into the output.
       */
      lastValue_outReal = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
         lastValue_outReal = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
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
   private RetCode temaOpenAndFillBody( TemaStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      if( optInTimePeriod == 1 ) {
         if( historyLen < temaLookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevEMA1 = 0.0;
         sp.prevEMA2 = 0.0;
         sp.prevEMA3 = 0.0;
         sp.optInK_1 = 0.0;
         int fillLb = temaLookback(optInTimePeriod);
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
            outReal[fillIdx] = inReal[fillLb + fillIdx];
         }
         sp.cur_outReal = outReal[outNBElement.value - 1];
         return RetCode.Success;
      }
      /* For an explanation of this function, please read:
       *
       * Stocks & Commodities V. 12:1 (11-19):
       *   Smoothing Data With Faster Moving Averages
       * Stocks & Commodities V. 12:2 (72-80):
       *   Smoothing Data With Less Lag
       *
       * Both magazine articles written by Patrick G. Mulloy
       *
       * Essentially, a TEMA of time serie 't' is:
       *   EMA1 = EMA(t,period)
       *   EMA2 = EMA(EMA(t,period),period)
       *   EMA3 = EMA(EMA(EMA(t,period),period))
       *   TEMA = 3*EMA1 - 3*EMA2 + EMA3
       *
       * TEMA offers a moving average with less lags then the
       * traditional EMA.
       *
       * Do not confuse a TEMA with EMA3. Both are called "Triple EMA"
       * in the litterature.
       *
       * DEMA is very similar (and from the same author).
       */
      /* Will change only on success. */
      outNBElement.value = 0;
      outBegIdx.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackEMA = emaLookback(optInTimePeriod);
      lookbackTotal = lookbackEMA * 3;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). Explicit because the
       * 3*e1 - 3*e2 + e3 composition cancels exactly only without FMA
       * contraction; ARM64 fused multiply-add leaves ~1e-14 residue.
       */
      /* The three EMA are computed in a single lockstep pass: each new
       * EMA1 value is immediately fed into EMA2, and each new EMA2 value
       * into EMA3. No temporary buffers are needed.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum
       *    of its first 'period' inputs, accumulated from 0.0 in
       *    input order (0.0+x is not x for x=-0.0), divided by
       *    the period.
       *  - Metastock compatibility: EMA1 is seeded from inReal[0],
       *    EMA2 from the first EMA1 value, EMA3 from the first EMA2
       *    value.
       *  - The combine keeps the (3.0*EMA1)-(3.0*EMA2) grouping,
       *    added to EMA3 on the left.
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
         while( today <= startIdx - lookbackEMA * 2 ) {
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
         while( today <= startIdx - lookbackEMA * 2 ) {
            prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance EMA1 and EMA2 in lockstep through the unstable
       * period of EMA2, up to the bar where EMA3 seeding begins.
       */
      while( today <= startIdx - lookbackEMA ) {
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
       * period of EMA3, up to the first output bar.
       */
      while( today <= startIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
      }
      /* Stable zone: keep advancing the three EMA in lockstep and
       * write the TEMA into the output.
       */
      outReal[0] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = Math.fma(inReal[today++] - prevEMA1, optInK_1, prevEMA1);
         prevEMA2 = Math.fma(prevEMA1 - prevEMA2, optInK_1, prevEMA2);
         prevEMA3 = Math.fma(prevEMA2 - prevEMA3, optInK_1, prevEMA3);
         outReal[outIdx++] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
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
   /* Internal startIdx-anchored open behind temaOpen (composition seam). */
   TemaStream temaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      TemaStream sp = new TemaStream(this);
      RetCode retCode = temaOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_TEMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_TEMA open: internal error");
      }
      throw new IllegalArgumentException("TA_TEMA open: " + retCode);
   }
   /**
    * Open a live TEMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#tema} at that bar.
    * <p>The history must hold at least {@code temaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public TemaStream temaOpen( double inReal[], int optInTimePeriod )
   {
      return temaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#temaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#tema} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public TemaStream temaOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      TemaStream sp = new TemaStream(this);
      RetCode retCode = temaOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_TEMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_TEMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_TEMA openAndFill: " + retCode);
   }
