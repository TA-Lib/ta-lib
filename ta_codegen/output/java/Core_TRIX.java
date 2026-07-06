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
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         prevEMA2 = prevEMA1;
      }
      /* Advance EMA1 and EMA2 in lockstep through the unstable
       * period of EMA2, up to the bar where EMA3 seeding begins.
       */
      while( today <= startIdx - (lookbackEMA + 1) ) {
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
       * period of EMA3, up to the bar before the first output.
       */
      while( today <= startIdx - 1 ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
      }
      /* Stable zone: keep advancing the three EMA in lockstep and
       * write the 1-day rate-of-change of EMA3 into the output.
       */
      outIdx = 0;
      while( today <= endIdx ) {
         tempReal = prevEMA3;
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
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
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - (lookbackEMA + 1) ) {
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
      while( today <= startIdx - 1 ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         tempReal = prevEMA3;
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
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
            tempReal += inReal[today++];
         }
         prevEMA1 = tempReal / optInTimePeriod;
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
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
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - (lookbackEMA + 1) ) {
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
      while( today <= startIdx - 1 ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         tempReal = prevEMA3;
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
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
            tempReal += inReal[today++];
         }
         prevEMA1 = tempReal / optInTimePeriod;
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
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
         while( today <= startIdx - (lookbackEMA * 2 + 1) ) {
            prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         }
         prevEMA2 = prevEMA1;
      }
      while( today <= startIdx - (lookbackEMA + 1) ) {
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
      while( today <= startIdx - 1 ) {
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         tempReal = prevEMA3;
         prevEMA1 = (inReal[today++] - prevEMA1) * optInK_1 + prevEMA1;
         prevEMA2 = (prevEMA1 - prevEMA2) * optInK_1 + prevEMA2;
         prevEMA3 = (prevEMA2 - prevEMA3) * optInK_1 + prevEMA3;
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
