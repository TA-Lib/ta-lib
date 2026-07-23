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
   public RetCode hma( int startIdx,
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
      int dividerFull = 0;
      int dividerHalf = 0;
      int dividerSqrt = 0;
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
      dividerFull = optInTimePeriod * (optInTimePeriod + 1) >> 1;
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
         dividerHalf = halfPeriod * (halfPeriod + 1) >> 1;
         dividerSqrt = sqrtPeriod * (sqrtPeriod + 1) >> 1;
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
   public RetCode hmaUnguarded( int startIdx,
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
      int dividerFull = 0;
      int dividerHalf = 0;
      int dividerSqrt = 0;
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
      dividerFull = optInTimePeriod * (optInTimePeriod + 1) >> 1;
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
         dividerHalf = halfPeriod * (halfPeriod + 1) >> 1;
         dividerSqrt = sqrtPeriod * (sqrtPeriod + 1) >> 1;
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
   public RetCode hma( int startIdx,
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
      int dividerFull = 0;
      int dividerHalf = 0;
      int dividerSqrt = 0;
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
      dividerFull = optInTimePeriod * (optInTimePeriod + 1) >> 1;
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
         dividerHalf = halfPeriod * (halfPeriod + 1) >> 1;
         dividerSqrt = sqrtPeriod * (sqrtPeriod + 1) >> 1;
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
   public RetCode hmaUnguarded( int startIdx,
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
      int dividerFull = 0;
      int dividerHalf = 0;
      int dividerSqrt = 0;
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
      dividerFull = optInTimePeriod * (optInTimePeriod + 1) >> 1;
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
         dividerHalf = halfPeriod * (halfPeriod + 1) >> 1;
         dividerSqrt = sqrtPeriod * (sqrtPeriod + 1) >> 1;
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
