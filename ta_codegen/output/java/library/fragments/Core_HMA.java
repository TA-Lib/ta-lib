/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  072226 MF,CC  First version (issue #139).
 *  072326 MF,CC  Fused single-pass rewrite: rolling sums + sqrt(n)-sized
 *                CIRCBUF, no whole-range temporaries (issue #139).
 */

   /**
    * Number of leading input bars {@link Core#hma} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the full-period WMA; the half and
    *        square-root periods derive from it (default 20; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int hmaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int sqrtPeriod;
      sqrtPeriod = (int)Math.sqrt((double)optInTimePeriod);
      return wmaLookback(optInTimePeriod) + wmaLookback(sqrtPeriod) ;

   }
   RetCode hmaInternal( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int lookbackTotal = 0;
      int lookbackSqrt = 0;
      int halfPeriod = 0;
      int sqrtPeriod = 0;
      int ringSize = 0;
      int wmaStartIdx = 0;
      int today = 0;
      int outIdx = 0;
      int i = 0;
      int w = 0;
      double dividerFull = 0;
      double dividerHalf = 0;
      double dividerSqrt = 0;
      int trailingIdxFull = 0;
      int trailingIdxHalf = 0;
      double periodSubFull = 0;
      double periodSumFull = 0;
      double trailingFull = 0;
      double periodSubHalf = 0;
      double periodSumHalf = 0;
      double trailingHalf = 0;
      double periodSubSqrt = 0;
      double periodSumSqrt = 0;
      double trailingSqrt = 0;
      double tempReal = 0;
      double fullOut = 0;
      double halfOut = 0;
      double diffReal = 0;
      double[] dRing;
      int dRing_Idx = 0;
      int maxIdx_dRing = (50)-1;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* The de-lagged series needs only its last sqrt(n) values, so the whole
       * computation runs in one pass over a single window into the input:
       * three interleaved WMA rolling sums plus this small ring. The ring has
       * sqrt(n)-1 slots: on the stack while that fits the 50-slot prolog
       * (optInTimePeriod <= 2703), TA_Malloc from optInTimePeriod = 2704 up.
       */
      /* Hull Moving Average (Alan Hull, 2005):
       *
       *    HMA(n) = WMA( 2*WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )
       *
       * Both derived periods use the author's Integer() truncation; some other
       * published sources round to nearest instead, which is a visibly different
       * line. See hma.md and issue #139.
       *
       * Each of the three WMAs keeps TA_WMA's exact accumulation order
       * (periodSub/periodSum, lagged trailing subtract), so this fused pass is
       * BIT-IDENTICAL to composing three TA_WMA calls -- the composite
       * differential in test_composite.c holds it to that, memcmp-exact.
       */
      halfPeriod = optInTimePeriod / 2;
      sqrtPeriod = (int)Math.sqrt((double)optInTimePeriod);
      lookbackSqrt = wmaLookback(sqrtPeriod);
      lookbackTotal = wmaLookback(optInTimePeriod) + lookbackSqrt;
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
      /* The two price WMAs are anchored where the first de-lagged value is
       * needed: lookbackSqrt bars before the first requested output.
       * wmaStartIdx >= optInTimePeriod-1 is implied by the clamp above.
       */
      wmaStartIdx = startIdx - lookbackSqrt;
      dividerFull = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
      /* Prime the full-period WMA over the optInTimePeriod-1 bars before
       * wmaStartIdx, exactly as TA_WMA does (weights 1..period-1).
       */
      periodSubFull = 0.0;
      periodSumFull = 0.0;
      trailingIdxFull = wmaStartIdx - (optInTimePeriod - 1);
      i = trailingIdxFull;
      w = 1;
      while( i < wmaStartIdx ) {
         tempReal = inReal[i++];
         periodSubFull += tempReal;
         periodSumFull += tempReal * w;
         w += 1;
      }
      trailingFull = 0.0;
      outIdx = 0;
      /* sqrtPeriod == 1 exactly when optInTimePeriod is 2 or 3; stated on the
       * param so the stream analyzer sees a param-pure dual-mode split.
       */
      if( optInTimePeriod == 2 || optInTimePeriod == 3 ) {
         /* Degenerate regime, optInTimePeriod 2 or 3 only: halfPeriod and
          * sqrtPeriod are both 1, and a period-1 WMA is the identity (TA_WMA's
          * own short-circuit). The whole formula collapses to
          *    HMA[t] = 2*price[t] - WMA(price, n)[t]
          * with no de-lag ring at all. In-place note: the output store lands on
          * the SAME slot the trailing read just consumed (zero margin), so the
          * read stays ordered before the store.
          */
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            outReal[outIdx++] = 2.0 * tempReal - fullOut;
         }
      } else {
         /* General regime: optInTimePeriod >= 4, so halfPeriod >= 2 and
          * sqrtPeriod >= 2 -- no period-1 special cases below this point.
          */
         dividerHalf = (double)halfPeriod * (halfPeriod + 1) / 2.0;
         dividerSqrt = (double)sqrtPeriod * (sqrtPeriod + 1) / 2.0;
         /* Prime the half-period WMA the same way. */
         periodSubHalf = 0.0;
         periodSumHalf = 0.0;
         trailingIdxHalf = wmaStartIdx - (halfPeriod - 1);
         i = trailingIdxHalf;
         w = 1;
         while( i < wmaStartIdx ) {
            tempReal = inReal[i++];
            periodSubHalf += tempReal;
            periodSumHalf += tempReal * w;
            w += 1;
         }
         trailingHalf = 0.0;
         /* The de-lagged value computed at bar t is consumed as the outer WMA's
          * trailing value sqrtPeriod-1 bars later, so a single-cursor ring of
          * sqrtPeriod-1 slots is enough: read the expiring value, overwrite the
          * slot with the current one, advance.
          */
         ringSize = sqrtPeriod - 1;
         if( ringSize < 1 ) return RetCode.AllocErr;
         dRing = new double[ringSize];
         maxIdx_dRing = (ringSize)-1;
         dRing_Idx = 0;
         /* Warm-up: the sqrtPeriod-1 de-lagged values before the first output
          * prime the outer WMA (weights 1..sqrtPeriod-1) and fill the ring.
          */
         periodSubSqrt = 0.0;
         periodSumSqrt = 0.0;
         trailingSqrt = 0.0;
         w = 1;
         for( today = wmaStartIdx; today < startIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSumSqrt += diffReal * w;
            w += 1;
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
         }
         /* Steady state: one pass, three rolling WMAs. Writes trail every read by
          * at least sqrtPeriod-1 slots (the lookback clamp), so outReal == inReal
          * stays safe.
          */
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSubSqrt -= trailingSqrt;
            periodSumSqrt += diffReal * sqrtPeriod;
            trailingSqrt = dRing[dRing_Idx];
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
            outReal[outIdx++] = periodSumSqrt / dividerSqrt;
            periodSumSqrt -= periodSubSqrt;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode hmaUnguardedInternal( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int lookbackTotal = 0;
      int lookbackSqrt = 0;
      int halfPeriod = 0;
      int sqrtPeriod = 0;
      int ringSize = 0;
      int wmaStartIdx = 0;
      int today = 0;
      int outIdx = 0;
      int i = 0;
      int w = 0;
      double dividerFull = 0;
      double dividerHalf = 0;
      double dividerSqrt = 0;
      int trailingIdxFull = 0;
      int trailingIdxHalf = 0;
      double periodSubFull = 0;
      double periodSumFull = 0;
      double trailingFull = 0;
      double periodSubHalf = 0;
      double periodSumHalf = 0;
      double trailingHalf = 0;
      double periodSubSqrt = 0;
      double periodSumSqrt = 0;
      double trailingSqrt = 0;
      double tempReal = 0;
      double fullOut = 0;
      double halfOut = 0;
      double diffReal = 0;
      double[] dRing;
      int dRing_Idx = 0;
      int maxIdx_dRing = (50)-1;
      halfPeriod = optInTimePeriod / 2;
      sqrtPeriod = (int)Math.sqrt((double)optInTimePeriod);
      lookbackSqrt = wmaLookback(sqrtPeriod);
      lookbackTotal = wmaLookback(optInTimePeriod) + lookbackSqrt;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      wmaStartIdx = startIdx - lookbackSqrt;
      dividerFull = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
      periodSubFull = 0.0;
      periodSumFull = 0.0;
      trailingIdxFull = wmaStartIdx - (optInTimePeriod - 1);
      i = trailingIdxFull;
      w = 1;
      while( i < wmaStartIdx ) {
         tempReal = inReal[i++];
         periodSubFull += tempReal;
         periodSumFull += tempReal * w;
         w += 1;
      }
      trailingFull = 0.0;
      outIdx = 0;
      if( optInTimePeriod == 2 || optInTimePeriod == 3 ) {
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            outReal[outIdx++] = 2.0 * tempReal - fullOut;
         }
      } else {
         dividerHalf = (double)halfPeriod * (halfPeriod + 1) / 2.0;
         dividerSqrt = (double)sqrtPeriod * (sqrtPeriod + 1) / 2.0;
         periodSubHalf = 0.0;
         periodSumHalf = 0.0;
         trailingIdxHalf = wmaStartIdx - (halfPeriod - 1);
         i = trailingIdxHalf;
         w = 1;
         while( i < wmaStartIdx ) {
            tempReal = inReal[i++];
            periodSubHalf += tempReal;
            periodSumHalf += tempReal * w;
            w += 1;
         }
         trailingHalf = 0.0;
         ringSize = sqrtPeriod - 1;
         if( ringSize < 1 ) return RetCode.AllocErr;
         dRing = new double[ringSize];
         maxIdx_dRing = (ringSize)-1;
         dRing_Idx = 0;
         periodSubSqrt = 0.0;
         periodSumSqrt = 0.0;
         trailingSqrt = 0.0;
         w = 1;
         for( today = wmaStartIdx; today < startIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSumSqrt += diffReal * w;
            w += 1;
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
         }
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSubSqrt -= trailingSqrt;
            periodSumSqrt += diffReal * sqrtPeriod;
            trailingSqrt = dRing[dRing_Idx];
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
            outReal[outIdx++] = periodSumSqrt / dividerSqrt;
            periodSumSqrt -= periodSubSqrt;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode hmaInternal( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int lookbackTotal = 0;
      int lookbackSqrt = 0;
      int halfPeriod = 0;
      int sqrtPeriod = 0;
      int ringSize = 0;
      int wmaStartIdx = 0;
      int today = 0;
      int outIdx = 0;
      int i = 0;
      int w = 0;
      double dividerFull = 0;
      double dividerHalf = 0;
      double dividerSqrt = 0;
      int trailingIdxFull = 0;
      int trailingIdxHalf = 0;
      double periodSubFull = 0;
      double periodSumFull = 0;
      double trailingFull = 0;
      double periodSubHalf = 0;
      double periodSumHalf = 0;
      double trailingHalf = 0;
      double periodSubSqrt = 0;
      double periodSumSqrt = 0;
      double trailingSqrt = 0;
      double tempReal = 0;
      double fullOut = 0;
      double halfOut = 0;
      double diffReal = 0;
      double[] dRing;
      int dRing_Idx = 0;
      int maxIdx_dRing = (50)-1;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      halfPeriod = optInTimePeriod / 2;
      sqrtPeriod = (int)Math.sqrt((double)optInTimePeriod);
      lookbackSqrt = wmaLookback(sqrtPeriod);
      lookbackTotal = wmaLookback(optInTimePeriod) + lookbackSqrt;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      wmaStartIdx = startIdx - lookbackSqrt;
      dividerFull = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
      periodSubFull = 0.0;
      periodSumFull = 0.0;
      trailingIdxFull = wmaStartIdx - (optInTimePeriod - 1);
      i = trailingIdxFull;
      w = 1;
      while( i < wmaStartIdx ) {
         tempReal = (double)inReal[i++];
         periodSubFull += tempReal;
         periodSumFull += tempReal * w;
         w += 1;
      }
      trailingFull = 0.0;
      outIdx = 0;
      if( optInTimePeriod == 2 || optInTimePeriod == 3 ) {
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = (double)inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = (double)inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            outReal[outIdx++] = 2.0 * tempReal - fullOut;
         }
      } else {
         dividerHalf = (double)halfPeriod * (halfPeriod + 1) / 2.0;
         dividerSqrt = (double)sqrtPeriod * (sqrtPeriod + 1) / 2.0;
         periodSubHalf = 0.0;
         periodSumHalf = 0.0;
         trailingIdxHalf = wmaStartIdx - (halfPeriod - 1);
         i = trailingIdxHalf;
         w = 1;
         while( i < wmaStartIdx ) {
            tempReal = (double)inReal[i++];
            periodSubHalf += tempReal;
            periodSumHalf += tempReal * w;
            w += 1;
         }
         trailingHalf = 0.0;
         ringSize = sqrtPeriod - 1;
         if( ringSize < 1 ) return RetCode.AllocErr;
         dRing = new double[ringSize];
         maxIdx_dRing = (ringSize)-1;
         dRing_Idx = 0;
         periodSubSqrt = 0.0;
         periodSumSqrt = 0.0;
         trailingSqrt = 0.0;
         w = 1;
         for( today = wmaStartIdx; today < startIdx; today += 1 ) {
            tempReal = (double)inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = (double)inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = (double)inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSumSqrt += diffReal * w;
            w += 1;
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
         }
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = (double)inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = (double)inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = (double)inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSubSqrt -= trailingSqrt;
            periodSumSqrt += diffReal * sqrtPeriod;
            trailingSqrt = dRing[dRing_Idx];
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
            outReal[outIdx++] = periodSumSqrt / dividerSqrt;
            periodSumSqrt -= periodSubSqrt;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode hmaUnguardedInternal( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int lookbackTotal = 0;
      int lookbackSqrt = 0;
      int halfPeriod = 0;
      int sqrtPeriod = 0;
      int ringSize = 0;
      int wmaStartIdx = 0;
      int today = 0;
      int outIdx = 0;
      int i = 0;
      int w = 0;
      double dividerFull = 0;
      double dividerHalf = 0;
      double dividerSqrt = 0;
      int trailingIdxFull = 0;
      int trailingIdxHalf = 0;
      double periodSubFull = 0;
      double periodSumFull = 0;
      double trailingFull = 0;
      double periodSubHalf = 0;
      double periodSumHalf = 0;
      double trailingHalf = 0;
      double periodSubSqrt = 0;
      double periodSumSqrt = 0;
      double trailingSqrt = 0;
      double tempReal = 0;
      double fullOut = 0;
      double halfOut = 0;
      double diffReal = 0;
      double[] dRing;
      int dRing_Idx = 0;
      int maxIdx_dRing = (50)-1;
      halfPeriod = optInTimePeriod / 2;
      sqrtPeriod = (int)Math.sqrt((double)optInTimePeriod);
      lookbackSqrt = wmaLookback(sqrtPeriod);
      lookbackTotal = wmaLookback(optInTimePeriod) + lookbackSqrt;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      wmaStartIdx = startIdx - lookbackSqrt;
      dividerFull = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
      periodSubFull = 0.0;
      periodSumFull = 0.0;
      trailingIdxFull = wmaStartIdx - (optInTimePeriod - 1);
      i = trailingIdxFull;
      w = 1;
      while( i < wmaStartIdx ) {
         tempReal = (double)inReal[i++];
         periodSubFull += tempReal;
         periodSumFull += tempReal * w;
         w += 1;
      }
      trailingFull = 0.0;
      outIdx = 0;
      if( optInTimePeriod == 2 || optInTimePeriod == 3 ) {
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = (double)inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = (double)inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            outReal[outIdx++] = 2.0 * tempReal - fullOut;
         }
      } else {
         dividerHalf = (double)halfPeriod * (halfPeriod + 1) / 2.0;
         dividerSqrt = (double)sqrtPeriod * (sqrtPeriod + 1) / 2.0;
         periodSubHalf = 0.0;
         periodSumHalf = 0.0;
         trailingIdxHalf = wmaStartIdx - (halfPeriod - 1);
         i = trailingIdxHalf;
         w = 1;
         while( i < wmaStartIdx ) {
            tempReal = (double)inReal[i++];
            periodSubHalf += tempReal;
            periodSumHalf += tempReal * w;
            w += 1;
         }
         trailingHalf = 0.0;
         ringSize = sqrtPeriod - 1;
         if( ringSize < 1 ) return RetCode.AllocErr;
         dRing = new double[ringSize];
         maxIdx_dRing = (ringSize)-1;
         dRing_Idx = 0;
         periodSubSqrt = 0.0;
         periodSumSqrt = 0.0;
         trailingSqrt = 0.0;
         w = 1;
         for( today = wmaStartIdx; today < startIdx; today += 1 ) {
            tempReal = (double)inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = (double)inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = (double)inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSumSqrt += diffReal * w;
            w += 1;
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
         }
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = (double)inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = (double)inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = (double)inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSubSqrt -= trailingSqrt;
            periodSumSqrt += diffReal * sqrtPeriod;
            trailingSqrt = dRing[dRing_Idx];
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
            outReal[outIdx++] = periodSumSqrt / dividerSqrt;
            periodSumSqrt -= periodSubSqrt;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Hull Moving Average, published by Alan Hull in 2005: a moving average
    * built to track price with far less lag than an
    * [{@code SMA}](/functions/sma), [{@code WMA}](/functions/wma) or
    * [{@code EMA}](/functions/ema) of the same length while staying smooth. It
    * first removes lag by doubling a half-period [{@code WMA}](/functions/wma)
    * and subtracting the full-period one — extrapolating the average toward
    * current price — then smooths that de-lagged series with a final WMA over
    * the square root of the period. HMA is also selectable as a moving-average
    * type ({@code TA_MAType_HMA}) wherever an {@code optInMAType} parameter is
    * accepted ([{@code MA}](/functions/ma),
    * [{@code BBANDS}](/functions/bbands), [{@code STOCH}](/functions/stoch),
    * [{@code MACDEXT}](/functions/macdext), ...).
    * <p><b>Formula</b>
    * <pre>{@code
    * HMA(n) = WMA( 2 * WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )
    * All three averages are the standard linearly-weighted moving average (TA-Lib's WMA). Every output is a closed-form weighted sum of the input window: there is no seeding, no recursion, hence no unstable period.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The two derived periods {@code n/2} and {@code sqrt(n)} are **truncated** to integers, exactly as in Alan Hull's own statement of the formula ({@code Integer()}); Tulip Indicators and pandas-ta do the same. Some other published descriptions round to nearest instead, which changes both the values and, for the square root, the lookback — a visibly different line, not a tolerance-level difference. TA-Lib follows the author.</li>
    * <li>The default period of 20 is Alan Hull's own default. It is also a period on which the truncate and round-to-nearest conventions coincide (20/2 is exact; sqrt(20) = 4.47 truncates and rounds to 4), so at the default TA-Lib matches charting platforms regardless of their rounding convention.</li>
    * <li>The period range starts at 2: a period of 1 would make the half-period WMA degenerate ({@code Integer(1/2) = 0}).</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#hmaLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series, close by convention.
    * @param optInTimePeriod Number of bars in the full-period WMA; the half and
    *        square-root periods derive from it (default 20; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Hull moving average of the input. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#wma
    * @see Core#movingAverage
    * @see Core#sma
    * @see Core#ema
    */
   public OutRange hma( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = hmaInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("HMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Hull Moving Average, published by Alan Hull in 2005: a moving average
    * built to track price with far less lag than an
    * [{@code SMA}](/functions/sma), [{@code WMA}](/functions/wma) or
    * [{@code EMA}](/functions/ema) of the same length while staying smooth. It
    * first removes lag by doubling a half-period [{@code WMA}](/functions/wma)
    * and subtracting the full-period one — extrapolating the average toward
    * current price — then smooths that de-lagged series with a final WMA over
    * the square root of the period. HMA is also selectable as a moving-average
    * type ({@code TA_MAType_HMA}) wherever an {@code optInMAType} parameter is
    * accepted ([{@code MA}](/functions/ma),
    * [{@code BBANDS}](/functions/bbands), [{@code STOCH}](/functions/stoch),
    * [{@code MACDEXT}](/functions/macdext), ...). — <b>unchecked</b> variant of
    * {@link Core#hma}.
    * <p>Validates nothing and never throws. The caller guarantees: non-negative
    * {@code startIdx}, {@code endIdx >= startIdx}, non-null arrays, output
    * arrays distinct from each other, and every optional parameter already
    * resolved and within its documented range — a sentinel such as
    * {@code Integer.MIN_VALUE} is <b>not</b> substituted here.
    * <p>Breaking any of those yields an empty {@link OutRange} or undefined
    * output rather than a diagnostic. (C and Rust return a status code from
    * this tier, so their callers can detect it; this one has nowhere to report
    * it.) Use the guarded method unless the arguments are already known good.
    *
    * @return The range written, exactly as the guarded method reports it.
    */
   public OutRange hmaUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      hmaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Hull Moving Average, published by Alan Hull in 2005: a moving average
    * built to track price with far less lag than an
    * [{@code SMA}](/functions/sma), [{@code WMA}](/functions/wma) or
    * [{@code EMA}](/functions/ema) of the same length while staying smooth. It
    * first removes lag by doubling a half-period [{@code WMA}](/functions/wma)
    * and subtracting the full-period one — extrapolating the average toward
    * current price — then smooths that de-lagged series with a final WMA over
    * the square root of the period. HMA is also selectable as a moving-average
    * type ({@code TA_MAType_HMA}) wherever an {@code optInMAType} parameter is
    * accepted ([{@code MA}](/functions/ma),
    * [{@code BBANDS}](/functions/bbands), [{@code STOCH}](/functions/stoch),
    * [{@code MACDEXT}](/functions/macdext), ...).
    * <p><b>Formula</b>
    * <pre>{@code
    * HMA(n) = WMA( 2 * WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )
    * All three averages are the standard linearly-weighted moving average (TA-Lib's WMA). Every output is a closed-form weighted sum of the input window: there is no seeding, no recursion, hence no unstable period.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The two derived periods {@code n/2} and {@code sqrt(n)} are **truncated** to integers, exactly as in Alan Hull's own statement of the formula ({@code Integer()}); Tulip Indicators and pandas-ta do the same. Some other published descriptions round to nearest instead, which changes both the values and, for the square root, the lookback — a visibly different line, not a tolerance-level difference. TA-Lib follows the author.</li>
    * <li>The default period of 20 is Alan Hull's own default. It is also a period on which the truncate and round-to-nearest conventions coincide (20/2 is exact; sqrt(20) = 4.47 truncates and rounds to 4), so at the default TA-Lib matches charting platforms regardless of their rounding convention.</li>
    * <li>The period range starts at 2: a period of 1 would make the half-period WMA degenerate ({@code Integer(1/2) = 0}).</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#hmaLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series, close by convention.
    * @param optInTimePeriod Number of bars in the full-period WMA; the half and
    *        square-root periods derive from it (default 20; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Hull moving average of the input. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#wma
    * @see Core#movingAverage
    * @see Core#sma
    * @see Core#ema
    */
   public OutRange hma( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = hmaInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("HMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Hull Moving Average, published by Alan Hull in 2005: a moving average
    * built to track price with far less lag than an
    * [{@code SMA}](/functions/sma), [{@code WMA}](/functions/wma) or
    * [{@code EMA}](/functions/ema) of the same length while staying smooth. It
    * first removes lag by doubling a half-period [{@code WMA}](/functions/wma)
    * and subtracting the full-period one — extrapolating the average toward
    * current price — then smooths that de-lagged series with a final WMA over
    * the square root of the period. HMA is also selectable as a moving-average
    * type ({@code TA_MAType_HMA}) wherever an {@code optInMAType} parameter is
    * accepted ([{@code MA}](/functions/ma),
    * [{@code BBANDS}](/functions/bbands), [{@code STOCH}](/functions/stoch),
    * [{@code MACDEXT}](/functions/macdext), ...). — <b>unchecked</b> variant of
    * {@link Core#hma}.
    * <p>Validates nothing and never throws. The caller guarantees: non-negative
    * {@code startIdx}, {@code endIdx >= startIdx}, non-null arrays, output
    * arrays distinct from each other, and every optional parameter already
    * resolved and within its documented range — a sentinel such as
    * {@code Integer.MIN_VALUE} is <b>not</b> substituted here.
    * <p>Breaking any of those yields an empty {@link OutRange} or undefined
    * output rather than a diagnostic. (C and Rust return a status code from
    * this tier, so their callers can detect it; this one has nowhere to report
    * it.) Use the guarded method unless the arguments are already known good.
    * <p>This is the {@code float[]} overload; see the guarded method.
    *
    * @return The range written, exactly as the guarded method reports it.
    */
   public OutRange hmaUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      hmaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live HMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#hma} over the same series.
    * Open with {@link Core#hmaOpen}; there is no close — the handle is
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
   public static final class HmaStream {
      final Core core;
      int optInTimePeriod;
      double dividerFull;
      double periodSubFull;
      double periodSumFull;
      double trailingFull;
      double fullOut;
      int halfPeriod;
      int sqrtPeriod;
      double dividerHalf;
      double dividerSqrt;
      double periodSubHalf;
      double periodSumHalf;
      double trailingHalf;
      double periodSubSqrt;
      double periodSumSqrt;
      double trailingSqrt;
      double halfOut;
      double diffReal;
      int dRing_Idx;
      int maxIdx_dRing;
      int ringPos_trailingIdxFull;
      int ringCap_trailingIdxFull;
      double[] ring_trailingIdxFull_inReal;
      double cur_outReal;
      int ringPos_trailingIdxHalf;
      int ringCap_trailingIdxHalf;
      double[] ring_trailingIdxHalf_inReal;
      int cbSize_dRing;
      double[] cb_dRing;
      OutRange fillRange;

      HmaStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#hmaOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      HmaStream( HmaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.dividerFull = other.dividerFull;
         this.periodSubFull = other.periodSubFull;
         this.periodSumFull = other.periodSumFull;
         this.trailingFull = other.trailingFull;
         this.fullOut = other.fullOut;
         this.halfPeriod = other.halfPeriod;
         this.sqrtPeriod = other.sqrtPeriod;
         this.dividerHalf = other.dividerHalf;
         this.dividerSqrt = other.dividerSqrt;
         this.periodSubHalf = other.periodSubHalf;
         this.periodSumHalf = other.periodSumHalf;
         this.trailingHalf = other.trailingHalf;
         this.periodSubSqrt = other.periodSubSqrt;
         this.periodSumSqrt = other.periodSumSqrt;
         this.trailingSqrt = other.trailingSqrt;
         this.halfOut = other.halfOut;
         this.diffReal = other.diffReal;
         this.dRing_Idx = other.dRing_Idx;
         this.maxIdx_dRing = other.maxIdx_dRing;
         this.ringPos_trailingIdxFull = other.ringPos_trailingIdxFull;
         this.ringCap_trailingIdxFull = other.ringCap_trailingIdxFull;
         this.ring_trailingIdxFull_inReal = other.ring_trailingIdxFull_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.ringPos_trailingIdxHalf = other.ringPos_trailingIdxHalf;
         this.ringCap_trailingIdxHalf = other.ringCap_trailingIdxHalf;
         this.ring_trailingIdxHalf_inReal = other.ring_trailingIdxHalf_inReal.clone();
         this.cbSize_dRing = other.cbSize_dRing;
         this.cb_dRing = other.cb_dRing.clone();
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.hmaStreamStep(this, inReal);
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
         HmaStream scratch = new HmaStream(this);
         core.hmaStreamStep(scratch, inReal);
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
      public HmaStream copy() {
         return new HmaStream(this);
      }
   }
   void hmaStreamStep( HmaStream sp, double inReal )
   {
      if( sp.optInTimePeriod == 2 || sp.optInTimePeriod == 3 ) {
         double tempReal = 0.0;
         if( sp.ringCap_trailingIdxFull == 0 ) {
            sp.ring_trailingIdxFull_inReal[0] = inReal;
         }
         tempReal = inReal;
         sp.periodSubFull += tempReal;
         sp.periodSubFull -= sp.trailingFull;
         sp.periodSumFull += tempReal * sp.optInTimePeriod;
         sp.trailingFull = sp.ring_trailingIdxFull_inReal[sp.ringPos_trailingIdxFull];
         sp.fullOut = sp.periodSumFull / sp.dividerFull;
         sp.periodSumFull -= sp.periodSubFull;
         sp.cur_outReal = 2.0 * tempReal - sp.fullOut;
         sp.ring_trailingIdxFull_inReal[sp.ringPos_trailingIdxFull] = inReal;
         sp.ringPos_trailingIdxFull = sp.ringPos_trailingIdxFull + 1;
         if( sp.ringPos_trailingIdxFull >= sp.ringCap_trailingIdxFull ) {
            sp.ringPos_trailingIdxFull = 0;
         }
      } else {
         double tempReal = 0.0;
         if( sp.ringCap_trailingIdxFull == 0 ) {
            sp.ring_trailingIdxFull_inReal[0] = inReal;
         }
         if( sp.ringCap_trailingIdxHalf == 0 ) {
            sp.ring_trailingIdxHalf_inReal[0] = inReal;
         }
         tempReal = inReal;
         sp.periodSubFull += tempReal;
         sp.periodSubFull -= sp.trailingFull;
         sp.periodSumFull += tempReal * sp.optInTimePeriod;
         sp.trailingFull = sp.ring_trailingIdxFull_inReal[sp.ringPos_trailingIdxFull];
         sp.fullOut = sp.periodSumFull / sp.dividerFull;
         sp.periodSumFull -= sp.periodSubFull;
         sp.periodSubHalf += tempReal;
         sp.periodSubHalf -= sp.trailingHalf;
         sp.periodSumHalf += tempReal * sp.halfPeriod;
         sp.trailingHalf = sp.ring_trailingIdxHalf_inReal[sp.ringPos_trailingIdxHalf];
         sp.halfOut = sp.periodSumHalf / sp.dividerHalf;
         sp.periodSumHalf -= sp.periodSubHalf;
         sp.diffReal = 2.0 * sp.halfOut - sp.fullOut;
         sp.periodSubSqrt += sp.diffReal;
         sp.periodSubSqrt -= sp.trailingSqrt;
         sp.periodSumSqrt += sp.diffReal * sp.sqrtPeriod;
         sp.trailingSqrt = sp.cb_dRing[sp.dRing_Idx];
         sp.cb_dRing[sp.dRing_Idx] = sp.diffReal;
         sp.dRing_Idx = sp.dRing_Idx + 1;
         if( sp.dRing_Idx > sp.maxIdx_dRing ) {
            sp.dRing_Idx = 0;
         }
         sp.cur_outReal = sp.periodSumSqrt / sp.dividerSqrt;
         sp.periodSumSqrt -= sp.periodSubSqrt;
         sp.ring_trailingIdxFull_inReal[sp.ringPos_trailingIdxFull] = inReal;
         sp.ringPos_trailingIdxFull = sp.ringPos_trailingIdxFull + 1;
         if( sp.ringPos_trailingIdxFull >= sp.ringCap_trailingIdxFull ) {
            sp.ringPos_trailingIdxFull = 0;
         }
         sp.ring_trailingIdxHalf_inReal[sp.ringPos_trailingIdxHalf] = inReal;
         sp.ringPos_trailingIdxHalf = sp.ringPos_trailingIdxHalf + 1;
         if( sp.ringPos_trailingIdxHalf >= sp.ringCap_trailingIdxHalf ) {
            sp.ringPos_trailingIdxHalf = 0;
         }
      }
   }
   private RetCode hmaOpenBody( HmaStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 2 || optInTimePeriod == 3 ) {
         int lookbackTotal = 0;
         int lookbackSqrt = 0;
         int halfPeriod = 0;
         int sqrtPeriod = 0;
         int ringSize = 0;
         int wmaStartIdx = 0;
         int today = 0;
         int outIdx = 0;
         int i = 0;
         int w = 0;
         double dividerFull = 0;
         double dividerHalf = 0;
         double dividerSqrt = 0;
         int trailingIdxFull = 0;
         int trailingIdxHalf = 0;
         double periodSubFull = 0;
         double periodSumFull = 0;
         double trailingFull = 0;
         double periodSubHalf = 0;
         double periodSumHalf = 0;
         double trailingHalf = 0;
         double periodSubSqrt = 0;
         double periodSumSqrt = 0;
         double trailingSqrt = 0;
         double tempReal = 0;
         double fullOut = 0;
         double halfOut = 0;
         double diffReal = 0;
         double[] dRing;
         int dRing_Idx = 0;
         int maxIdx_dRing = (50)-1;
         /* The de-lagged series needs only its last sqrt(n) values, so the whole
          * computation runs in one pass over a single window into the input:
          * three interleaved WMA rolling sums plus this small ring. The ring has
          * sqrt(n)-1 slots: on the stack while that fits the 50-slot prolog
          * (optInTimePeriod <= 2703), TA_Malloc from optInTimePeriod = 2704 up.
          */
         /* Hull Moving Average (Alan Hull, 2005):
          *
          *    HMA(n) = WMA( 2*WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )
          *
          * Both derived periods use the author's Integer() truncation; some other
          * published sources round to nearest instead, which is a visibly different
          * line. See hma.md and issue #139.
          *
          * Each of the three WMAs keeps TA_WMA's exact accumulation order
          * (periodSub/periodSum, lagged trailing subtract), so this fused pass is
          * BIT-IDENTICAL to composing three TA_WMA calls -- the composite
          * differential in test_composite.c holds it to that, memcmp-exact.
          */
         halfPeriod = optInTimePeriod / 2;
         sqrtPeriod = (int)Math.sqrt((double)optInTimePeriod);
         lookbackSqrt = wmaLookback(sqrtPeriod);
         lookbackTotal = wmaLookback(optInTimePeriod) + lookbackSqrt;
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
         /* The two price WMAs are anchored where the first de-lagged value is
          * needed: lookbackSqrt bars before the first requested output.
          * wmaStartIdx >= optInTimePeriod-1 is implied by the clamp above.
          */
         wmaStartIdx = startIdx - lookbackSqrt;
         dividerFull = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
         /* Prime the full-period WMA over the optInTimePeriod-1 bars before
          * wmaStartIdx, exactly as TA_WMA does (weights 1..period-1).
          */
         periodSubFull = 0.0;
         periodSumFull = 0.0;
         trailingIdxFull = wmaStartIdx - (optInTimePeriod - 1);
         i = trailingIdxFull;
         w = 1;
         while( i < wmaStartIdx ) {
            tempReal = inReal[i++];
            periodSubFull += tempReal;
            periodSumFull += tempReal * w;
            w += 1;
         }
         trailingFull = 0.0;
         outIdx = 0;
         /* sqrtPeriod == 1 exactly when optInTimePeriod is 2 or 3; stated on the
          * param so the stream analyzer sees a param-pure dual-mode split.
          */
         /* Degenerate regime, optInTimePeriod 2 or 3 only: halfPeriod and
          * sqrtPeriod are both 1, and a period-1 WMA is the identity (TA_WMA's
          * own short-circuit). The whole formula collapses to
          *    HMA[t] = 2*price[t] - WMA(price, n)[t]
          * with no de-lag ring at all. In-place note: the output store lands on
          * the SAME slot the trailing read just consumed (zero margin), so the
          * read stays ordered before the store.
          */
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            lastValue_outReal = 2.0 * tempReal - fullOut;
         }
         outBegIdx.value = startIdx;
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         int cap_trailingIdxFull = today - trailingIdxFull;
         if( cap_trailingIdxFull < 0 || cap_trailingIdxFull > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdxFull = (cap_trailingIdxFull > 0)? cap_trailingIdxFull : 1;
         double[] capRing_trailingIdxFull_inReal = new double[allocN_trailingIdxFull];
         System.arraycopy(inReal, historyLen - cap_trailingIdxFull, capRing_trailingIdxFull_inReal, 0, cap_trailingIdxFull);
         sp.optInTimePeriod = optInTimePeriod;
         sp.dividerFull = dividerFull;
         sp.periodSubFull = periodSubFull;
         sp.periodSumFull = periodSumFull;
         sp.trailingFull = trailingFull;
         sp.fullOut = fullOut;
         sp.halfPeriod = halfPeriod;
         sp.sqrtPeriod = sqrtPeriod;
         sp.dividerHalf = dividerHalf;
         sp.dividerSqrt = dividerSqrt;
         sp.periodSubHalf = periodSubHalf;
         sp.periodSumHalf = periodSumHalf;
         sp.trailingHalf = trailingHalf;
         sp.periodSubSqrt = periodSubSqrt;
         sp.periodSumSqrt = periodSumSqrt;
         sp.trailingSqrt = trailingSqrt;
         sp.halfOut = halfOut;
         sp.diffReal = diffReal;
         sp.dRing_Idx = dRing_Idx;
         sp.maxIdx_dRing = maxIdx_dRing;
         sp.ringPos_trailingIdxFull = 0;
         sp.ringCap_trailingIdxFull = cap_trailingIdxFull;
         sp.ring_trailingIdxFull_inReal = capRing_trailingIdxFull_inReal;
         sp.ring_trailingIdxHalf_inReal = new double[1];
         sp.cb_dRing = new double[1];
         sp.cur_outReal = lastValue_outReal;
         return RetCode.Success;
      } else {
         int lookbackTotal = 0;
         int lookbackSqrt = 0;
         int halfPeriod = 0;
         int sqrtPeriod = 0;
         int ringSize = 0;
         int wmaStartIdx = 0;
         int today = 0;
         int outIdx = 0;
         int i = 0;
         int w = 0;
         double dividerFull = 0;
         double dividerHalf = 0;
         double dividerSqrt = 0;
         int trailingIdxFull = 0;
         int trailingIdxHalf = 0;
         double periodSubFull = 0;
         double periodSumFull = 0;
         double trailingFull = 0;
         double periodSubHalf = 0;
         double periodSumHalf = 0;
         double trailingHalf = 0;
         double periodSubSqrt = 0;
         double periodSumSqrt = 0;
         double trailingSqrt = 0;
         double tempReal = 0;
         double fullOut = 0;
         double halfOut = 0;
         double diffReal = 0;
         double[] dRing;
         int dRing_Idx = 0;
         int maxIdx_dRing = (50)-1;
         /* The de-lagged series needs only its last sqrt(n) values, so the whole
          * computation runs in one pass over a single window into the input:
          * three interleaved WMA rolling sums plus this small ring. The ring has
          * sqrt(n)-1 slots: on the stack while that fits the 50-slot prolog
          * (optInTimePeriod <= 2703), TA_Malloc from optInTimePeriod = 2704 up.
          */
         /* Hull Moving Average (Alan Hull, 2005):
          *
          *    HMA(n) = WMA( 2*WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )
          *
          * Both derived periods use the author's Integer() truncation; some other
          * published sources round to nearest instead, which is a visibly different
          * line. See hma.md and issue #139.
          *
          * Each of the three WMAs keeps TA_WMA's exact accumulation order
          * (periodSub/periodSum, lagged trailing subtract), so this fused pass is
          * BIT-IDENTICAL to composing three TA_WMA calls -- the composite
          * differential in test_composite.c holds it to that, memcmp-exact.
          */
         halfPeriod = optInTimePeriod / 2;
         sqrtPeriod = (int)Math.sqrt((double)optInTimePeriod);
         lookbackSqrt = wmaLookback(sqrtPeriod);
         lookbackTotal = wmaLookback(optInTimePeriod) + lookbackSqrt;
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
         /* The two price WMAs are anchored where the first de-lagged value is
          * needed: lookbackSqrt bars before the first requested output.
          * wmaStartIdx >= optInTimePeriod-1 is implied by the clamp above.
          */
         wmaStartIdx = startIdx - lookbackSqrt;
         dividerFull = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
         /* Prime the full-period WMA over the optInTimePeriod-1 bars before
          * wmaStartIdx, exactly as TA_WMA does (weights 1..period-1).
          */
         periodSubFull = 0.0;
         periodSumFull = 0.0;
         trailingIdxFull = wmaStartIdx - (optInTimePeriod - 1);
         i = trailingIdxFull;
         w = 1;
         while( i < wmaStartIdx ) {
            tempReal = inReal[i++];
            periodSubFull += tempReal;
            periodSumFull += tempReal * w;
            w += 1;
         }
         trailingFull = 0.0;
         outIdx = 0;
         /* sqrtPeriod == 1 exactly when optInTimePeriod is 2 or 3; stated on the
          * param so the stream analyzer sees a param-pure dual-mode split.
          */
         /* General regime: optInTimePeriod >= 4, so halfPeriod >= 2 and
          * sqrtPeriod >= 2 -- no period-1 special cases below this point.
          */
         dividerHalf = (double)halfPeriod * (halfPeriod + 1) / 2.0;
         dividerSqrt = (double)sqrtPeriod * (sqrtPeriod + 1) / 2.0;
         /* Prime the half-period WMA the same way. */
         periodSubHalf = 0.0;
         periodSumHalf = 0.0;
         trailingIdxHalf = wmaStartIdx - (halfPeriod - 1);
         i = trailingIdxHalf;
         w = 1;
         while( i < wmaStartIdx ) {
            tempReal = inReal[i++];
            periodSubHalf += tempReal;
            periodSumHalf += tempReal * w;
            w += 1;
         }
         trailingHalf = 0.0;
         /* The de-lagged value computed at bar t is consumed as the outer WMA's
          * trailing value sqrtPeriod-1 bars later, so a single-cursor ring of
          * sqrtPeriod-1 slots is enough: read the expiring value, overwrite the
          * slot with the current one, advance.
          */
         ringSize = sqrtPeriod - 1;
         if( ringSize < 1 ) return RetCode.AllocErr;
         dRing = new double[ringSize];
         maxIdx_dRing = (ringSize)-1;
         dRing_Idx = 0;
         /* Warm-up: the sqrtPeriod-1 de-lagged values before the first output
          * prime the outer WMA (weights 1..sqrtPeriod-1) and fill the ring.
          */
         periodSubSqrt = 0.0;
         periodSumSqrt = 0.0;
         trailingSqrt = 0.0;
         w = 1;
         for( today = wmaStartIdx; today < startIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSumSqrt += diffReal * w;
            w += 1;
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
         }
         /* Steady state: one pass, three rolling WMAs. Writes trail every read by
          * at least sqrtPeriod-1 slots (the lookback clamp), so outReal == inReal
          * stays safe.
          */
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSubSqrt -= trailingSqrt;
            periodSumSqrt += diffReal * sqrtPeriod;
            trailingSqrt = dRing[dRing_Idx];
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
            lastValue_outReal = periodSumSqrt / dividerSqrt;
            periodSumSqrt -= periodSubSqrt;
         }
         outBegIdx.value = startIdx;
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         int cap_trailingIdxFull = today - trailingIdxFull;
         if( cap_trailingIdxFull < 0 || cap_trailingIdxFull > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdxFull = (cap_trailingIdxFull > 0)? cap_trailingIdxFull : 1;
         double[] capRing_trailingIdxFull_inReal = new double[allocN_trailingIdxFull];
         System.arraycopy(inReal, historyLen - cap_trailingIdxFull, capRing_trailingIdxFull_inReal, 0, cap_trailingIdxFull);
         int cap_trailingIdxHalf = today - trailingIdxHalf;
         if( cap_trailingIdxHalf < 0 || cap_trailingIdxHalf > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdxHalf = (cap_trailingIdxHalf > 0)? cap_trailingIdxHalf : 1;
         double[] capRing_trailingIdxHalf_inReal = new double[allocN_trailingIdxHalf];
         System.arraycopy(inReal, historyLen - cap_trailingIdxHalf, capRing_trailingIdxHalf_inReal, 0, cap_trailingIdxHalf);
         int capCb_dRing = maxIdx_dRing + 1;
         if( capCb_dRing > historyLen + 1 ) {
            return RetCode.InternalError;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.dividerFull = dividerFull;
         sp.periodSubFull = periodSubFull;
         sp.periodSumFull = periodSumFull;
         sp.trailingFull = trailingFull;
         sp.fullOut = fullOut;
         sp.halfPeriod = halfPeriod;
         sp.sqrtPeriod = sqrtPeriod;
         sp.dividerHalf = dividerHalf;
         sp.dividerSqrt = dividerSqrt;
         sp.periodSubHalf = periodSubHalf;
         sp.periodSumHalf = periodSumHalf;
         sp.trailingHalf = trailingHalf;
         sp.periodSubSqrt = periodSubSqrt;
         sp.periodSumSqrt = periodSumSqrt;
         sp.trailingSqrt = trailingSqrt;
         sp.halfOut = halfOut;
         sp.diffReal = diffReal;
         sp.dRing_Idx = dRing_Idx;
         sp.maxIdx_dRing = maxIdx_dRing;
         sp.ringPos_trailingIdxFull = 0;
         sp.ringCap_trailingIdxFull = cap_trailingIdxFull;
         sp.ring_trailingIdxFull_inReal = capRing_trailingIdxFull_inReal;
         sp.ringPos_trailingIdxHalf = 0;
         sp.ringCap_trailingIdxHalf = cap_trailingIdxHalf;
         sp.ring_trailingIdxHalf_inReal = capRing_trailingIdxHalf_inReal;
         sp.cbSize_dRing = capCb_dRing;
         sp.cb_dRing = dRing;
         sp.cur_outReal = lastValue_outReal;
         return RetCode.Success;
      }
   }
   private RetCode hmaOpenAndFillBody( HmaStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 2 || optInTimePeriod == 3 ) {
         int lookbackTotal = 0;
         int lookbackSqrt = 0;
         int halfPeriod = 0;
         int sqrtPeriod = 0;
         int ringSize = 0;
         int wmaStartIdx = 0;
         int today = 0;
         int outIdx = 0;
         int i = 0;
         int w = 0;
         double dividerFull = 0;
         double dividerHalf = 0;
         double dividerSqrt = 0;
         int trailingIdxFull = 0;
         int trailingIdxHalf = 0;
         double periodSubFull = 0;
         double periodSumFull = 0;
         double trailingFull = 0;
         double periodSubHalf = 0;
         double periodSumHalf = 0;
         double trailingHalf = 0;
         double periodSubSqrt = 0;
         double periodSumSqrt = 0;
         double trailingSqrt = 0;
         double tempReal = 0;
         double fullOut = 0;
         double halfOut = 0;
         double diffReal = 0;
         double[] dRing;
         int dRing_Idx = 0;
         int maxIdx_dRing = (50)-1;
         /* The de-lagged series needs only its last sqrt(n) values, so the whole
          * computation runs in one pass over a single window into the input:
          * three interleaved WMA rolling sums plus this small ring. The ring has
          * sqrt(n)-1 slots: on the stack while that fits the 50-slot prolog
          * (optInTimePeriod <= 2703), TA_Malloc from optInTimePeriod = 2704 up.
          */
         /* Hull Moving Average (Alan Hull, 2005):
          *
          *    HMA(n) = WMA( 2*WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )
          *
          * Both derived periods use the author's Integer() truncation; some other
          * published sources round to nearest instead, which is a visibly different
          * line. See hma.md and issue #139.
          *
          * Each of the three WMAs keeps TA_WMA's exact accumulation order
          * (periodSub/periodSum, lagged trailing subtract), so this fused pass is
          * BIT-IDENTICAL to composing three TA_WMA calls -- the composite
          * differential in test_composite.c holds it to that, memcmp-exact.
          */
         halfPeriod = optInTimePeriod / 2;
         sqrtPeriod = (int)Math.sqrt((double)optInTimePeriod);
         lookbackSqrt = wmaLookback(sqrtPeriod);
         lookbackTotal = wmaLookback(optInTimePeriod) + lookbackSqrt;
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
         /* The two price WMAs are anchored where the first de-lagged value is
          * needed: lookbackSqrt bars before the first requested output.
          * wmaStartIdx >= optInTimePeriod-1 is implied by the clamp above.
          */
         wmaStartIdx = startIdx - lookbackSqrt;
         dividerFull = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
         /* Prime the full-period WMA over the optInTimePeriod-1 bars before
          * wmaStartIdx, exactly as TA_WMA does (weights 1..period-1).
          */
         periodSubFull = 0.0;
         periodSumFull = 0.0;
         trailingIdxFull = wmaStartIdx - (optInTimePeriod - 1);
         i = trailingIdxFull;
         w = 1;
         while( i < wmaStartIdx ) {
            tempReal = inReal[i++];
            periodSubFull += tempReal;
            periodSumFull += tempReal * w;
            w += 1;
         }
         trailingFull = 0.0;
         outIdx = 0;
         /* sqrtPeriod == 1 exactly when optInTimePeriod is 2 or 3; stated on the
          * param so the stream analyzer sees a param-pure dual-mode split.
          */
         /* Degenerate regime, optInTimePeriod 2 or 3 only: halfPeriod and
          * sqrtPeriod are both 1, and a period-1 WMA is the identity (TA_WMA's
          * own short-circuit). The whole formula collapses to
          *    HMA[t] = 2*price[t] - WMA(price, n)[t]
          * with no de-lag ring at all. In-place note: the output store lands on
          * the SAME slot the trailing read just consumed (zero margin), so the
          * read stays ordered before the store.
          */
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            outReal[outIdx++] = 2.0 * tempReal - fullOut;
         }
         outBegIdx.value = startIdx;
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         int cap_trailingIdxFull = today - trailingIdxFull;
         if( cap_trailingIdxFull < 0 || cap_trailingIdxFull > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdxFull = (cap_trailingIdxFull > 0)? cap_trailingIdxFull : 1;
         double[] capRing_trailingIdxFull_inReal = new double[allocN_trailingIdxFull];
         System.arraycopy(inReal, historyLen - cap_trailingIdxFull, capRing_trailingIdxFull_inReal, 0, cap_trailingIdxFull);
         sp.optInTimePeriod = optInTimePeriod;
         sp.dividerFull = dividerFull;
         sp.periodSubFull = periodSubFull;
         sp.periodSumFull = periodSumFull;
         sp.trailingFull = trailingFull;
         sp.fullOut = fullOut;
         sp.halfPeriod = halfPeriod;
         sp.sqrtPeriod = sqrtPeriod;
         sp.dividerHalf = dividerHalf;
         sp.dividerSqrt = dividerSqrt;
         sp.periodSubHalf = periodSubHalf;
         sp.periodSumHalf = periodSumHalf;
         sp.trailingHalf = trailingHalf;
         sp.periodSubSqrt = periodSubSqrt;
         sp.periodSumSqrt = periodSumSqrt;
         sp.trailingSqrt = trailingSqrt;
         sp.halfOut = halfOut;
         sp.diffReal = diffReal;
         sp.dRing_Idx = dRing_Idx;
         sp.maxIdx_dRing = maxIdx_dRing;
         sp.ringPos_trailingIdxFull = 0;
         sp.ringCap_trailingIdxFull = cap_trailingIdxFull;
         sp.ring_trailingIdxFull_inReal = capRing_trailingIdxFull_inReal;
         sp.ring_trailingIdxHalf_inReal = new double[1];
         sp.cb_dRing = new double[1];
         sp.cur_outReal = outReal[outNBElement.value - 1];
         return RetCode.Success;
      } else {
         int lookbackTotal = 0;
         int lookbackSqrt = 0;
         int halfPeriod = 0;
         int sqrtPeriod = 0;
         int ringSize = 0;
         int wmaStartIdx = 0;
         int today = 0;
         int outIdx = 0;
         int i = 0;
         int w = 0;
         double dividerFull = 0;
         double dividerHalf = 0;
         double dividerSqrt = 0;
         int trailingIdxFull = 0;
         int trailingIdxHalf = 0;
         double periodSubFull = 0;
         double periodSumFull = 0;
         double trailingFull = 0;
         double periodSubHalf = 0;
         double periodSumHalf = 0;
         double trailingHalf = 0;
         double periodSubSqrt = 0;
         double periodSumSqrt = 0;
         double trailingSqrt = 0;
         double tempReal = 0;
         double fullOut = 0;
         double halfOut = 0;
         double diffReal = 0;
         double[] dRing;
         int dRing_Idx = 0;
         int maxIdx_dRing = (50)-1;
         /* The de-lagged series needs only its last sqrt(n) values, so the whole
          * computation runs in one pass over a single window into the input:
          * three interleaved WMA rolling sums plus this small ring. The ring has
          * sqrt(n)-1 slots: on the stack while that fits the 50-slot prolog
          * (optInTimePeriod <= 2703), TA_Malloc from optInTimePeriod = 2704 up.
          */
         /* Hull Moving Average (Alan Hull, 2005):
          *
          *    HMA(n) = WMA( 2*WMA(price, Integer(n/2)) - WMA(price, n), Integer(SquareRoot(n)) )
          *
          * Both derived periods use the author's Integer() truncation; some other
          * published sources round to nearest instead, which is a visibly different
          * line. See hma.md and issue #139.
          *
          * Each of the three WMAs keeps TA_WMA's exact accumulation order
          * (periodSub/periodSum, lagged trailing subtract), so this fused pass is
          * BIT-IDENTICAL to composing three TA_WMA calls -- the composite
          * differential in test_composite.c holds it to that, memcmp-exact.
          */
         halfPeriod = optInTimePeriod / 2;
         sqrtPeriod = (int)Math.sqrt((double)optInTimePeriod);
         lookbackSqrt = wmaLookback(sqrtPeriod);
         lookbackTotal = wmaLookback(optInTimePeriod) + lookbackSqrt;
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
         /* The two price WMAs are anchored where the first de-lagged value is
          * needed: lookbackSqrt bars before the first requested output.
          * wmaStartIdx >= optInTimePeriod-1 is implied by the clamp above.
          */
         wmaStartIdx = startIdx - lookbackSqrt;
         dividerFull = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
         /* Prime the full-period WMA over the optInTimePeriod-1 bars before
          * wmaStartIdx, exactly as TA_WMA does (weights 1..period-1).
          */
         periodSubFull = 0.0;
         periodSumFull = 0.0;
         trailingIdxFull = wmaStartIdx - (optInTimePeriod - 1);
         i = trailingIdxFull;
         w = 1;
         while( i < wmaStartIdx ) {
            tempReal = inReal[i++];
            periodSubFull += tempReal;
            periodSumFull += tempReal * w;
            w += 1;
         }
         trailingFull = 0.0;
         outIdx = 0;
         /* sqrtPeriod == 1 exactly when optInTimePeriod is 2 or 3; stated on the
          * param so the stream analyzer sees a param-pure dual-mode split.
          */
         /* General regime: optInTimePeriod >= 4, so halfPeriod >= 2 and
          * sqrtPeriod >= 2 -- no period-1 special cases below this point.
          */
         dividerHalf = (double)halfPeriod * (halfPeriod + 1) / 2.0;
         dividerSqrt = (double)sqrtPeriod * (sqrtPeriod + 1) / 2.0;
         /* Prime the half-period WMA the same way. */
         periodSubHalf = 0.0;
         periodSumHalf = 0.0;
         trailingIdxHalf = wmaStartIdx - (halfPeriod - 1);
         i = trailingIdxHalf;
         w = 1;
         while( i < wmaStartIdx ) {
            tempReal = inReal[i++];
            periodSubHalf += tempReal;
            periodSumHalf += tempReal * w;
            w += 1;
         }
         trailingHalf = 0.0;
         /* The de-lagged value computed at bar t is consumed as the outer WMA's
          * trailing value sqrtPeriod-1 bars later, so a single-cursor ring of
          * sqrtPeriod-1 slots is enough: read the expiring value, overwrite the
          * slot with the current one, advance.
          */
         ringSize = sqrtPeriod - 1;
         if( ringSize < 1 ) return RetCode.AllocErr;
         dRing = new double[ringSize];
         maxIdx_dRing = (ringSize)-1;
         dRing_Idx = 0;
         /* Warm-up: the sqrtPeriod-1 de-lagged values before the first output
          * prime the outer WMA (weights 1..sqrtPeriod-1) and fill the ring.
          */
         periodSubSqrt = 0.0;
         periodSumSqrt = 0.0;
         trailingSqrt = 0.0;
         w = 1;
         for( today = wmaStartIdx; today < startIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSumSqrt += diffReal * w;
            w += 1;
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
         }
         /* Steady state: one pass, three rolling WMAs. Writes trail every read by
          * at least sqrtPeriod-1 slots (the lookback clamp), so outReal == inReal
          * stays safe.
          */
         for( today = startIdx; today <= endIdx; today += 1 ) {
            tempReal = inReal[today];
            periodSubFull += tempReal;
            periodSubFull -= trailingFull;
            periodSumFull += tempReal * optInTimePeriod;
            trailingFull = inReal[trailingIdxFull++];
            fullOut = periodSumFull / dividerFull;
            periodSumFull -= periodSubFull;
            periodSubHalf += tempReal;
            periodSubHalf -= trailingHalf;
            periodSumHalf += tempReal * halfPeriod;
            trailingHalf = inReal[trailingIdxHalf++];
            halfOut = periodSumHalf / dividerHalf;
            periodSumHalf -= periodSubHalf;
            diffReal = 2.0 * halfOut - fullOut;
            periodSubSqrt += diffReal;
            periodSubSqrt -= trailingSqrt;
            periodSumSqrt += diffReal * sqrtPeriod;
            trailingSqrt = dRing[dRing_Idx];
            dRing[dRing_Idx] = diffReal;
            dRing_Idx++;
            if( dRing_Idx > maxIdx_dRing ) { dRing_Idx = 0; }
            outReal[outIdx++] = periodSumSqrt / dividerSqrt;
            periodSumSqrt -= periodSubSqrt;
         }
         outBegIdx.value = startIdx;
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         int cap_trailingIdxFull = today - trailingIdxFull;
         if( cap_trailingIdxFull < 0 || cap_trailingIdxFull > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdxFull = (cap_trailingIdxFull > 0)? cap_trailingIdxFull : 1;
         double[] capRing_trailingIdxFull_inReal = new double[allocN_trailingIdxFull];
         System.arraycopy(inReal, historyLen - cap_trailingIdxFull, capRing_trailingIdxFull_inReal, 0, cap_trailingIdxFull);
         int cap_trailingIdxHalf = today - trailingIdxHalf;
         if( cap_trailingIdxHalf < 0 || cap_trailingIdxHalf > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdxHalf = (cap_trailingIdxHalf > 0)? cap_trailingIdxHalf : 1;
         double[] capRing_trailingIdxHalf_inReal = new double[allocN_trailingIdxHalf];
         System.arraycopy(inReal, historyLen - cap_trailingIdxHalf, capRing_trailingIdxHalf_inReal, 0, cap_trailingIdxHalf);
         int capCb_dRing = maxIdx_dRing + 1;
         if( capCb_dRing > historyLen + 1 ) {
            return RetCode.InternalError;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.dividerFull = dividerFull;
         sp.periodSubFull = periodSubFull;
         sp.periodSumFull = periodSumFull;
         sp.trailingFull = trailingFull;
         sp.fullOut = fullOut;
         sp.halfPeriod = halfPeriod;
         sp.sqrtPeriod = sqrtPeriod;
         sp.dividerHalf = dividerHalf;
         sp.dividerSqrt = dividerSqrt;
         sp.periodSubHalf = periodSubHalf;
         sp.periodSumHalf = periodSumHalf;
         sp.trailingHalf = trailingHalf;
         sp.periodSubSqrt = periodSubSqrt;
         sp.periodSumSqrt = periodSumSqrt;
         sp.trailingSqrt = trailingSqrt;
         sp.halfOut = halfOut;
         sp.diffReal = diffReal;
         sp.dRing_Idx = dRing_Idx;
         sp.maxIdx_dRing = maxIdx_dRing;
         sp.ringPos_trailingIdxFull = 0;
         sp.ringCap_trailingIdxFull = cap_trailingIdxFull;
         sp.ring_trailingIdxFull_inReal = capRing_trailingIdxFull_inReal;
         sp.ringPos_trailingIdxHalf = 0;
         sp.ringCap_trailingIdxHalf = cap_trailingIdxHalf;
         sp.ring_trailingIdxHalf_inReal = capRing_trailingIdxHalf_inReal;
         sp.cbSize_dRing = capCb_dRing;
         sp.cb_dRing = dRing;
         sp.cur_outReal = outReal[outNBElement.value - 1];
         return RetCode.Success;
      }
   }
   /* Internal startIdx-anchored open behind hmaOpen (composition seam). */
   HmaStream hmaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      HmaStream sp = new HmaStream(this);
      RetCode retCode = hmaOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_HMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_HMA open: internal error");
      }
      throw new IllegalArgumentException("TA_HMA open: " + retCode);
   }
   /**
    * Open a live HMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#hma} at that bar.
    * <p>The history must hold at least {@code hmaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public HmaStream hmaOpen( double inReal[], int optInTimePeriod )
   {
      return hmaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#hmaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#hma} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link HmaStream#fillRange()}.
    */
   public HmaStream hmaOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      HmaStream sp = new HmaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = hmaOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_HMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_HMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_HMA openAndFill: " + retCode);
   }
