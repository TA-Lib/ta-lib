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
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         /* Seed EMA2 with a simple average of the first 'period'
          * EMA1 values, accumulated as EMA1 produces them.
          */
         tempReal = 0.0;
         tempReal += prevEMA1;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
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
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance EMA1 and EMA2 in lockstep through the unstable
       * period of EMA2, up to the bar where EMA3 seeding begins.
       */
      while( today <= startIdx - lookbackEMA ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
      }
      if( this.compatibility == Compatibility.Default ) {
         /* Seed EMA3 with a simple average of the first 'period'
          * EMA2 values, accumulated as EMA2 produces them.
          */
         tempReal = 0.0;
         tempReal += prevEMA2;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
            prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
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
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
      }
      /* Stable zone: keep advancing the three EMA in lockstep and
       * write the TEMA into the output.
       */
      outReal[0] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
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
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         tempReal = 0.0;
         tempReal += prevEMA1;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
            tempReal += prevEMA1;
         }
         prevEMA2 = tempReal / optInTimePeriod;
      } else {
         prevEMA1 = inReal[0];
         today = 1;
         while( today <= startIdx - lookbackEMA * 2 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - lookbackEMA ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
      }
      if( this.compatibility == Compatibility.Default ) {
         tempReal = 0.0;
         tempReal += prevEMA2;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
            prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
            tempReal += prevEMA2;
         }
         prevEMA3 = tempReal / optInTimePeriod;
      } else {
         prevEMA3 = prevEMA2;
      }
      while( today <= startIdx ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
      }
      outReal[0] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
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
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         tempReal = 0.0;
         tempReal += prevEMA1;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
            tempReal += prevEMA1;
         }
         prevEMA2 = tempReal / optInTimePeriod;
      } else {
         prevEMA1 = inReal[0];
         today = 1;
         while( today <= startIdx - lookbackEMA * 2 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - lookbackEMA ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
      }
      if( this.compatibility == Compatibility.Default ) {
         tempReal = 0.0;
         tempReal += prevEMA2;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
            prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
            tempReal += prevEMA2;
         }
         prevEMA3 = tempReal / optInTimePeriod;
      } else {
         prevEMA3 = prevEMA2;
      }
      while( today <= startIdx ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
      }
      outReal[0] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
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
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         tempReal = 0.0;
         tempReal += prevEMA1;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
            tempReal += prevEMA1;
         }
         prevEMA2 = tempReal / optInTimePeriod;
      } else {
         prevEMA1 = inReal[0];
         today = 1;
         while( today <= startIdx - lookbackEMA * 2 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - lookbackEMA ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
      }
      if( this.compatibility == Compatibility.Default ) {
         tempReal = 0.0;
         tempReal += prevEMA2;
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
            prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
            tempReal += prevEMA2;
         }
         prevEMA3 = tempReal / optInTimePeriod;
      } else {
         prevEMA3 = prevEMA2;
      }
      while( today <= startIdx ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
      }
      outReal[0] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      outIdx = 1;
      while( today <= endIdx ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
         outReal[outIdx++] = prevEMA3 + (3.0 * prevEMA1 - 3.0 * prevEMA2);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
