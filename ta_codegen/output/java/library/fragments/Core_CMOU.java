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
 *  071626 MF,CC  Initial version.
 */

   public int cmouLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* CMOU needs optInTimePeriod price changes -> optInTimePeriod+1 prices ->
       * the first output is at index optInTimePeriod.
       *
       * Unlike the shipped CMO, there is NO unstable period and NO Metastock
       * "extra initial bar" adjustment: CMOU is a plain moving-window sum, so its
       * lookback is exactly the period.
       */
      return optInTimePeriod ;

   }
   public RetCode cmou( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* CMOU -- unsmoothed Chande Momentum Oscillator (as in TradingView ta.cmo,
       * QuantConnect, pandas-ta default). Over the trailing optInTimePeriod changes
       * d = inReal[i]-inReal[i-1]: Su = sum of up-moves (d>0), Sd = sum of
       * |down-moves| (d<0); CMOU = 100*(Su-Sd)/(Su+Sd), 0 for a flat window. A plain
       * moving-window sum (drop oldest change, add newest), NOT TA_CMO's Wilder
       * smoothing -- hence no unstable period.
       *
       * In-place safe (outReal == inReal): the trailing read inReal[trailingIdx]
       * precedes this iteration's write (trailingIdx >= outIdx), and the oldest
       * change's older endpoint comes from the `trailingValue` cache, not a re-read.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cmouLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* Accumulate the up/down sums over the first window: the optInTimePeriod
       * changes ending at startIdx (prices inReal[startIdx-optInTimePeriod ..
       * startIdx]). `trailingValue` caches the oldest price so the window's oldest
       * change can later be dropped by reading only the newer of its two prices.
       * `trailingIdx` points AT that newer price (one past the cached one).
       */
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = inReal[today];
      trailingValue = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      for( i = 0; i < optInTimePeriod; i += 1 ) {
         today += 1;
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
      }
      /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
       * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
       * with TA_IS_ZERO (as TA_CMO does for its own gain+loss) and emit 0.0.
       *
       * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
       * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
       * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
       * which both scale before dividing. The two orders differ by <=1 ULP.
       */
      outIdx = 0;
      sum = upSum + downSum;
      if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
         outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
         outReal[outIdx++] = 0.0;
      }
      /* Slide the window forward one bar at a time. */
      today += 1;
      while( today <= endIdx ) {
         /* Drop the oldest change: inReal[trailingIdx] - inReal[trailingIdx-1].
          * inReal[trailingIdx-1] comes from the cache (already overwritten when
          * outReal == inReal); inReal[trailingIdx] is read here, before this
          * iteration writes outReal[outIdx], so it is still the original price.
          */
         tempReal = inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         /* Add the newest change: inReal[today] - inReal[today-1]. */
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         sum = upSum + downSum;
         if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode cmouUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cmouLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = inReal[today];
      trailingValue = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      for( i = 0; i < optInTimePeriod; i += 1 ) {
         today += 1;
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
      }
      outIdx = 0;
      sum = upSum + downSum;
      if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
         outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
         outReal[outIdx++] = 0.0;
      }
      today += 1;
      while( today <= endIdx ) {
         tempReal = inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         sum = upSum + downSum;
         if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode cmou( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cmouLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = (double)inReal[today];
      trailingValue = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      for( i = 0; i < optInTimePeriod; i += 1 ) {
         today += 1;
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
      }
      outIdx = 0;
      sum = upSum + downSum;
      if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
         outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
         outReal[outIdx++] = 0.0;
      }
      today += 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         sum = upSum + downSum;
         if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode cmouUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cmouLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = (double)inReal[today];
      trailingValue = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      for( i = 0; i < optInTimePeriod; i += 1 ) {
         today += 1;
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
      }
      outIdx = 0;
      sum = upSum + downSum;
      if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
         outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
         outReal[outIdx++] = 0.0;
      }
      today += 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         sum = upSum + downSum;
         if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
