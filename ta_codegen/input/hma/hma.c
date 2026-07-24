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
 *
 */

int hma_lookback(int optInTimePeriod)
{
   int sqrtPeriod;

   sqrtPeriod = (int)sqrt((double)optInTimePeriod);

   return wma_lookback(optInTimePeriod) + wma_lookback(sqrtPeriod);
}

TA_RetCode hma(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int lookbackTotal, lookbackSqrt;
   int halfPeriod, sqrtPeriod, ringSize;
   int wmaStartIdx, today, outIdx, i, w;
   double dividerFull, dividerHalf, dividerSqrt;
   int trailingIdxFull, trailingIdxHalf;
   double periodSubFull, periodSumFull, trailingFull;
   double periodSubHalf, periodSumHalf, trailingHalf;
   double periodSubSqrt, periodSumSqrt, trailingSqrt;
   double tempReal, fullOut, halfOut, diffReal;

   /* The de-lagged series needs only its last sqrt(n) values, so the whole
    * computation runs in one pass over a single window into the input:
    * three interleaved WMA rolling sums plus this small ring. The ring has
    * sqrt(n)-1 slots: on the stack while that fits the 50-slot prolog
    * (optInTimePeriod <= 2703), TA_Malloc from optInTimePeriod = 2704 up.
    */
   CIRCBUF_PROLOG(dRing,double,50);

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
   sqrtPeriod = (int)sqrt((double)optInTimePeriod);

   lookbackSqrt  = wma_lookback(sqrtPeriod);
   lookbackTotal = wma_lookback(optInTimePeriod) + lookbackSqrt;

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* The two price WMAs are anchored where the first de-lagged value is
    * needed: lookbackSqrt bars before the first requested output.
    * wmaStartIdx >= optInTimePeriod-1 is implied by the clamp above.
    */
   wmaStartIdx = startIdx - lookbackSqrt;

   dividerFull = (double)optInTimePeriod*(optInTimePeriod+1)/2.0;

   /* Prime the full-period WMA over the optInTimePeriod-1 bars before
    * wmaStartIdx, exactly as TA_WMA does (weights 1..period-1).
    */
   periodSubFull = 0.0;
   periodSumFull = 0.0;
   trailingIdxFull = wmaStartIdx - (optInTimePeriod-1);
   i = trailingIdxFull;
   w = 1;
   while( i < wmaStartIdx )
   {
      tempReal = inReal[i++];
      periodSubFull += tempReal;
      periodSumFull += tempReal*w;
      w++;
   }
   trailingFull = 0.0;

   outIdx = 0;

   /* sqrtPeriod == 1 exactly when optInTimePeriod is 2 or 3; stated on the
    * param so the stream analyzer sees a param-pure dual-mode split. */
   if( (optInTimePeriod == 2) || (optInTimePeriod == 3) )
   {
      /* Degenerate regime, optInTimePeriod 2 or 3 only: halfPeriod and
       * sqrtPeriod are both 1, and a period-1 WMA is the identity (TA_WMA's
       * own short-circuit). The whole formula collapses to
       *    HMA[t] = 2*price[t] - WMA(price, n)[t]
       * with no de-lag ring at all. In-place note: the output store lands on
       * the SAME slot the trailing read just consumed (zero margin), so the
       * read stays ordered before the store.
       */
      for( today = startIdx; today <= endIdx; today++ )
      {
         tempReal = inReal[today];
         periodSubFull += tempReal;
         periodSubFull -= trailingFull;
         periodSumFull += tempReal*optInTimePeriod;
         trailingFull = inReal[trailingIdxFull++];
         fullOut = periodSumFull/dividerFull;
         periodSumFull -= periodSubFull;
         outReal[outIdx++] = 2.0*tempReal - fullOut;
      }
   }
   else
   {
      /* General regime: optInTimePeriod >= 4, so halfPeriod >= 2 and
       * sqrtPeriod >= 2 -- no period-1 special cases below this point.
       */
      dividerHalf = (double)halfPeriod*(halfPeriod+1)/2.0;
      dividerSqrt = (double)sqrtPeriod*(sqrtPeriod+1)/2.0;

      /* Prime the half-period WMA the same way. */
      periodSubHalf = 0.0;
      periodSumHalf = 0.0;
      trailingIdxHalf = wmaStartIdx - (halfPeriod-1);
      i = trailingIdxHalf;
      w = 1;
      while( i < wmaStartIdx )
      {
         tempReal = inReal[i++];
         periodSubHalf += tempReal;
         periodSumHalf += tempReal*w;
         w++;
      }
      trailingHalf = 0.0;

      /* The de-lagged value computed at bar t is consumed as the outer WMA's
       * trailing value sqrtPeriod-1 bars later, so a single-cursor ring of
       * sqrtPeriod-1 slots is enough: read the expiring value, overwrite the
       * slot with the current one, advance.
       */
      ringSize = sqrtPeriod - 1;
      CIRCBUF_INIT(dRing,double,ringSize);

      /* Warm-up: the sqrtPeriod-1 de-lagged values before the first output
       * prime the outer WMA (weights 1..sqrtPeriod-1) and fill the ring.
       */
      periodSubSqrt = 0.0;
      periodSumSqrt = 0.0;
      trailingSqrt = 0.0;
      w = 1;
      for( today = wmaStartIdx; today < startIdx; today++ )
      {
         tempReal = inReal[today];
         periodSubFull += tempReal;
         periodSubFull -= trailingFull;
         periodSumFull += tempReal*optInTimePeriod;
         trailingFull = inReal[trailingIdxFull++];
         fullOut = periodSumFull/dividerFull;
         periodSumFull -= periodSubFull;

         periodSubHalf += tempReal;
         periodSubHalf -= trailingHalf;
         periodSumHalf += tempReal*halfPeriod;
         trailingHalf = inReal[trailingIdxHalf++];
         halfOut = periodSumHalf/dividerHalf;
         periodSumHalf -= periodSubHalf;

         diffReal = 2.0*halfOut - fullOut;
         periodSubSqrt += diffReal;
         periodSumSqrt += diffReal*w;
         w++;
         dRing[dRing_Idx] = diffReal;
         CIRCBUF_NEXT(dRing);
      }

      /* Steady state: one pass, three rolling WMAs. Writes trail every read by
       * at least sqrtPeriod-1 slots (the lookback clamp), so outReal == inReal
       * stays safe.
       */
      for( today = startIdx; today <= endIdx; today++ )
      {
         tempReal = inReal[today];
         periodSubFull += tempReal;
         periodSubFull -= trailingFull;
         periodSumFull += tempReal*optInTimePeriod;
         trailingFull = inReal[trailingIdxFull++];
         fullOut = periodSumFull/dividerFull;
         periodSumFull -= periodSubFull;

         periodSubHalf += tempReal;
         periodSubHalf -= trailingHalf;
         periodSumHalf += tempReal*halfPeriod;
         trailingHalf = inReal[trailingIdxHalf++];
         halfOut = periodSumHalf/dividerHalf;
         periodSumHalf -= periodSubHalf;

         diffReal = 2.0*halfOut - fullOut;
         periodSubSqrt += diffReal;
         periodSubSqrt -= trailingSqrt;
         periodSumSqrt += diffReal*sqrtPeriod;
         trailingSqrt = dRing[dRing_Idx];
         dRing[dRing_Idx] = diffReal;
         CIRCBUF_NEXT(dRing);
         outReal[outIdx++] = periodSumSqrt/dividerSqrt;
         periodSumSqrt -= periodSubSqrt;
      }

      CIRCBUF_DESTROY(dRing);
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
