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
 *  071126 MF,CC  Inline the fixed-26/12 MACD lockstep pass (was a
 *                delegation to macd(...,0,0,...)); bit-exact, streamable.
 */

   public int macdFixLookback( int optInSignalPeriod )
   {
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      /* The lookback is driven by the signal line output.
       *
       * (must also account for the initial data consume
       *  by the fix 26 period EMA).
       */
      return emaLookback(26) + emaLookback(optInSignalPeriod) ;

   }
   public RetCode macdFix( int startIdx,
                           int endIdx,
                           double inReal[],
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
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int optInFastPeriod = 0;
      int optInSlowPeriod = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      optInFastPeriod = 12;
      optInSlowPeriod = 26;
      /* MACDFIX is the fixed 26/12 MACD: the fast/slow periods and their
       * smoothing factors are hardcoded (the general MACD selects these
       * exact values when its fast/slow period arguments are 0). Only the
       * signal period is caller-provided.
       *    Fix 12 -> fastK = 0.15
       *    Fix 26 -> slowK = 0.075
       */
      fastK = 0.15;
      slowK = 0.075;
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(26);
      /* fixed slow period */
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
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
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
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
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
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         }
         macdValue = prevFast - prevSlow;
         prevSignal = macdValue;
      }
      /* Advance everything in lockstep through the unstable period
       * of the signal EMA, up to the first output bar.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
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
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
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
   public RetCode macdFixUnguarded( int startIdx,
                                    int endIdx,
                                    double inReal[],
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
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int optInFastPeriod = 0;
      int optInSlowPeriod = 0;
      optInFastPeriod = 12;
      optInSlowPeriod = 26;
      fastK = 0.15;
      slowK = 0.075;
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(26);
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
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         }
         macdValue = prevFast - prevSlow;
         prevSignal = 0.0;
         prevSignal += macdValue;
         i = optInSignalPeriod - 1;
         while( i-- > 0 ) {
            tempReal = inReal[today++];
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
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
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         }
         macdValue = prevFast - prevSlow;
         prevSignal = macdValue;
      }
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
      }
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode macdFix( int startIdx,
                           int endIdx,
                           float inReal[],
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
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int optInFastPeriod = 0;
      int optInSlowPeriod = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      optInFastPeriod = 12;
      optInSlowPeriod = 26;
      fastK = 0.15;
      slowK = 0.075;
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(26);
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
            tempReal += (double)inReal[today++];
         }
         prevFast = 0.0;
         i = optInFastPeriod;
         while( i-- > 0 ) {
            prevFast += (double)inReal[today];
            tempReal += (double)inReal[today++];
         }
         prevSlow = tempReal / optInSlowPeriod;
         prevFast = prevFast / optInFastPeriod;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = (double)inReal[today++];
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         }
         macdValue = prevFast - prevSlow;
         prevSignal = 0.0;
         prevSignal += macdValue;
         i = optInSignalPeriod - 1;
         while( i-- > 0 ) {
            tempReal = (double)inReal[today++];
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
            macdValue = prevFast - prevSlow;
            prevSignal += macdValue;
         }
         prevSignal = prevSignal / optInSignalPeriod;
      } else {
         prevFast = (double)inReal[0];
         prevSlow = (double)inReal[0];
         today = 1;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = (double)inReal[today++];
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         }
         macdValue = prevFast - prevSlow;
         prevSignal = macdValue;
      }
      while( today <= startIdx ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
      }
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode macdFixUnguarded( int startIdx,
                                    int endIdx,
                                    float inReal[],
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
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int optInFastPeriod = 0;
      int optInSlowPeriod = 0;
      optInFastPeriod = 12;
      optInSlowPeriod = 26;
      fastK = 0.15;
      slowK = 0.075;
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(26);
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
            tempReal += (double)inReal[today++];
         }
         prevFast = 0.0;
         i = optInFastPeriod;
         while( i-- > 0 ) {
            prevFast += (double)inReal[today];
            tempReal += (double)inReal[today++];
         }
         prevSlow = tempReal / optInSlowPeriod;
         prevFast = prevFast / optInFastPeriod;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = (double)inReal[today++];
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         }
         macdValue = prevFast - prevSlow;
         prevSignal = 0.0;
         prevSignal += macdValue;
         i = optInSignalPeriod - 1;
         while( i-- > 0 ) {
            tempReal = (double)inReal[today++];
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
            macdValue = prevFast - prevSlow;
            prevSignal += macdValue;
         }
         prevSignal = prevSignal / optInSignalPeriod;
      } else {
         prevFast = (double)inReal[0];
         prevSlow = (double)inReal[0];
         today = 1;
         while( today <= startIdx - lookbackSignal ) {
            tempReal = (double)inReal[today++];
            prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
            prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         }
         macdValue = prevFast - prevSlow;
         prevSignal = macdValue;
      }
      while( today <= startIdx ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
      }
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
