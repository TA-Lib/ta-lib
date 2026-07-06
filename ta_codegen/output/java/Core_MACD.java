/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JPP      JP Pienaar (j.pienaar@mci.co.za)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  080403 JPP    Fix #767653 for logic when swapping periods.
 *  070526 MF,CC  Speed optimization: compute the two price EMA, the
 *                signal line and the histogram in a single lockstep
 *                pass (bit-exact, no temporary buffers).
 */

   public int macdLookback( int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      int tempInteger;
      /* The lookback is driven by the signal line output.
       *
       * (must also account for the initial data consume
       *  by the slow period).
       */
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      return emaLookback(optInSlowPeriod) + emaLookback(optInSignalPeriod) ;

   }
   public RetCode macd( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        int optInSignalPeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outMACD[],
                        double outMACDSignal[],
                        double outMACDHist[] )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevSignal = 0;
      double macdValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Catch special case for fix 26/12 MACD.
       * Use hardcoded k values matching the original algorithm.
       */
      if( optInSlowPeriod == 0 ) {
         /* Fix 26 */
         optInSlowPeriod = 26;
         slowK = 0.075;
      } else {
         slowK = 2.0 / (double)(optInSlowPeriod + 1);
      }
      if( optInFastPeriod == 0 ) {
         /* Fix 12 */
         optInFastPeriod = 12;
         fastK = 0.15;
      } else {
         fastK = 2.0 / (double)(optInFastPeriod + 1);
      }
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(optInSlowPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Everything is computed in a single lockstep pass: each bar
       * advances the fast and slow EMA (two independent recursions),
       * their difference is the MACD line, and each MACD-line value
       * is immediately fed into the signal EMA. No temporary buffers.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum of
       *    its first 'period' inputs, accumulated from 0.0 in input
       *    order, divided by the period. The fast and slow seed
       *    windows end on the same bar. The signal EMA is seeded the
       *    same way from the first 'signal period' MACD-line values.
       *  - Metastock compatibility: the fast and slow EMA are seeded
       *    from inReal[0], the signal EMA from the first MACD-line
       *    value.
       * Output alignment is identical for all compatibility modes;
       * only the seed values differ.
       *
       * In-place (an output == inReal) is supported: outputs at
       * [outIdx] are written only after inReal[startIdx+outIdx] was
       * read.
       */
      if( this.compatibility == Compatibility.Default ) {
         /* Seed each price EMA with a simple average of its first
          * 'period' price bars. The fast window is the tail of the
          * slow window: consume the leading slow-only bars first,
          * then accumulate both over the shared bars.
          */
         today = startIdx - lookbackTotal;
         tempReal = 0.0;
         i = optInSlowPeriod - optInFastPeriod;
         while( i-- > 0 ) {
            tempReal += inReal[today++];
         }
         prevFast = 0.0;
         i = optInFastPeriod;
         while( i-- > 0 ) {
            prevFast += inReal[today];
            tempReal += inReal[today++];
         }
         prevSlow = tempReal / optInSlowPeriod;
         prevFast = prevFast / optInFastPeriod;
         /* Advance both EMA through their unstable period, up to the
          * first MACD-line bar.
          */
         while( today <= startIdx - lookbackSignal ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         }
         macdValue = prevFast - prevSlow;
         /* Seed the signal EMA with a simple average of the first
          * 'signal period' MACD-line values, accumulated as they are
          * produced.
          */
         prevSignal = 0.0;
         prevSignal += macdValue;
         i = optInSignalPeriod - 1;
         while( i-- > 0 ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
            macdValue = prevFast - prevSlow;
            prevSignal += macdValue;
         }
         prevSignal = prevSignal / optInSignalPeriod;
      } else {
         /* Metastock/Tradestation: seed the fast and slow EMA with
          * inReal[0], advance them in lockstep up to the first
          * MACD-line bar, then seed the signal EMA with the first
          * MACD-line value.
          */
         prevFast = inReal[0];
         prevSlow = inReal[0];
         today = 1;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         }
         macdValue = prevFast - prevSlow;
         prevSignal = macdValue;
      }
      /* Advance everything in lockstep through the unstable period
       * of the signal EMA, up to the first output bar.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = (tempReal - prevFast) * fastK + prevFast;
         prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         macdValue = prevFast - prevSlow;
         prevSignal = (macdValue - prevSignal) * signalK + prevSignal;
      }
      /* Stable zone: keep advancing in lockstep and write the three
       * outputs.
       */
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = (tempReal - prevFast) * fastK + prevFast;
         prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         macdValue = prevFast - prevSlow;
         prevSignal = (macdValue - prevSignal) * signalK + prevSignal;
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode macdUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInFastPeriod,
                                 int optInSlowPeriod,
                                 int optInSignalPeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outMACD[],
                                 double outMACDSignal[],
                                 double outMACDHist[] )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevSignal = 0;
      double macdValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      if( optInSlowPeriod == 0 ) {
         optInSlowPeriod = 26;
         slowK = 0.075;
      } else {
         slowK = 2.0 / (double)(optInSlowPeriod + 1);
      }
      if( optInFastPeriod == 0 ) {
         optInFastPeriod = 12;
         fastK = 0.15;
      } else {
         fastK = 2.0 / (double)(optInFastPeriod + 1);
      }
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(optInSlowPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( this.compatibility == Compatibility.Default ) {
         today = startIdx - lookbackTotal;
         tempReal = 0.0;
         i = optInSlowPeriod - optInFastPeriod;
         while( i-- > 0 ) {
            tempReal += inReal[today++];
         }
         prevFast = 0.0;
         i = optInFastPeriod;
         while( i-- > 0 ) {
            prevFast += inReal[today];
            tempReal += inReal[today++];
         }
         prevSlow = tempReal / optInSlowPeriod;
         prevFast = prevFast / optInFastPeriod;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         }
         macdValue = prevFast - prevSlow;
         prevSignal = 0.0;
         prevSignal += macdValue;
         i = optInSignalPeriod - 1;
         while( i-- > 0 ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
            macdValue = prevFast - prevSlow;
            prevSignal += macdValue;
         }
         prevSignal = prevSignal / optInSignalPeriod;
      } else {
         prevFast = inReal[0];
         prevSlow = inReal[0];
         today = 1;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         }
         macdValue = prevFast - prevSlow;
         prevSignal = macdValue;
      }
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = (tempReal - prevFast) * fastK + prevFast;
         prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         macdValue = prevFast - prevSlow;
         prevSignal = (macdValue - prevSignal) * signalK + prevSignal;
      }
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = (tempReal - prevFast) * fastK + prevFast;
         prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         macdValue = prevFast - prevSlow;
         prevSignal = (macdValue - prevSignal) * signalK + prevSignal;
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode macd( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        int optInSignalPeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outMACD[],
                        double outMACDSignal[],
                        double outMACDHist[] )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevSignal = 0;
      double macdValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      if( optInSlowPeriod == 0 ) {
         optInSlowPeriod = 26;
         slowK = 0.075;
      } else {
         slowK = 2.0 / (double)(optInSlowPeriod + 1);
      }
      if( optInFastPeriod == 0 ) {
         optInFastPeriod = 12;
         fastK = 0.15;
      } else {
         fastK = 2.0 / (double)(optInFastPeriod + 1);
      }
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(optInSlowPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( this.compatibility == Compatibility.Default ) {
         today = startIdx - lookbackTotal;
         tempReal = 0.0;
         i = optInSlowPeriod - optInFastPeriod;
         while( i-- > 0 ) {
            tempReal += inReal[today++];
         }
         prevFast = 0.0;
         i = optInFastPeriod;
         while( i-- > 0 ) {
            prevFast += inReal[today];
            tempReal += inReal[today++];
         }
         prevSlow = tempReal / optInSlowPeriod;
         prevFast = prevFast / optInFastPeriod;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         }
         macdValue = prevFast - prevSlow;
         prevSignal = 0.0;
         prevSignal += macdValue;
         i = optInSignalPeriod - 1;
         while( i-- > 0 ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
            macdValue = prevFast - prevSlow;
            prevSignal += macdValue;
         }
         prevSignal = prevSignal / optInSignalPeriod;
      } else {
         prevFast = inReal[0];
         prevSlow = inReal[0];
         today = 1;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         }
         macdValue = prevFast - prevSlow;
         prevSignal = macdValue;
      }
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = (tempReal - prevFast) * fastK + prevFast;
         prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         macdValue = prevFast - prevSlow;
         prevSignal = (macdValue - prevSignal) * signalK + prevSignal;
      }
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = (tempReal - prevFast) * fastK + prevFast;
         prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         macdValue = prevFast - prevSlow;
         prevSignal = (macdValue - prevSignal) * signalK + prevSignal;
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode macdUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInFastPeriod,
                                 int optInSlowPeriod,
                                 int optInSignalPeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outMACD[],
                                 double outMACDSignal[],
                                 double outMACDHist[] )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevSignal = 0;
      double macdValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      if( optInSlowPeriod == 0 ) {
         optInSlowPeriod = 26;
         slowK = 0.075;
      } else {
         slowK = 2.0 / (double)(optInSlowPeriod + 1);
      }
      if( optInFastPeriod == 0 ) {
         optInFastPeriod = 12;
         fastK = 0.15;
      } else {
         fastK = 2.0 / (double)(optInFastPeriod + 1);
      }
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(optInSlowPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( this.compatibility == Compatibility.Default ) {
         today = startIdx - lookbackTotal;
         tempReal = 0.0;
         i = optInSlowPeriod - optInFastPeriod;
         while( i-- > 0 ) {
            tempReal += inReal[today++];
         }
         prevFast = 0.0;
         i = optInFastPeriod;
         while( i-- > 0 ) {
            prevFast += inReal[today];
            tempReal += inReal[today++];
         }
         prevSlow = tempReal / optInSlowPeriod;
         prevFast = prevFast / optInFastPeriod;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         }
         macdValue = prevFast - prevSlow;
         prevSignal = 0.0;
         prevSignal += macdValue;
         i = optInSignalPeriod - 1;
         while( i-- > 0 ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
            macdValue = prevFast - prevSlow;
            prevSignal += macdValue;
         }
         prevSignal = prevSignal / optInSignalPeriod;
      } else {
         prevFast = inReal[0];
         prevSlow = inReal[0];
         today = 1;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = inReal[today++];
            prevFast = (tempReal - prevFast) * fastK + prevFast;
            prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         }
         macdValue = prevFast - prevSlow;
         prevSignal = macdValue;
      }
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = (tempReal - prevFast) * fastK + prevFast;
         prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         macdValue = prevFast - prevSlow;
         prevSignal = (macdValue - prevSignal) * signalK + prevSignal;
      }
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = (tempReal - prevFast) * fastK + prevFast;
         prevSlow = (tempReal - prevSlow) * slowK + prevSlow;
         macdValue = prevFast - prevSlow;
         prevSignal = (macdValue - prevSignal) * signalK + prevSignal;
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
