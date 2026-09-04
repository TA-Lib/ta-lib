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
 *  090426 MF,CC  First version (issue #348).
 */

int rma_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1 + TA_GetUnstablePeriod(TA_FUNC_UNST_RMA);
}

TA_RetCode rma(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx, today, lookbackTotal;
   int nbRMA;

   double prevRMA, periodTotal, wAlpha, wBeta;

   *outBegIdx = 0;
   *outNBElement = 0;

   /* Adjust startIdx to account for the lookback period. */
   lookbackTotal = rma_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
    * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
    * measures closer to the exact recursion than the 1/period-first spelling
    * at nearly every period. The order is a gated contract, not a preference:
    * TA_RMA(TA_TRANGE(h,l,c),n) must equal TA_ATR(n) bit for bit.
    * The pair is exactly (1, 0) at period 1 -- hence no period-1 arm.
    */
   wBeta  = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
   wAlpha = 1.0 - wBeta;

   /* The arithmetic order is the rest of that contract: the seed accumulates
    * from 0.0 in input order, and the smoothing step stays ONE statement --
    * splitting it unfuses the multiply-add and puts a second latency on the
    * recurrence's dependency chain.
    *
    * In-place (outReal being inReal) is supported: the output index never
    * passes the bar index of any remaining read.
    */
   today = startIdx - lookbackTotal;

   /* Seed with a simple average of the first 'period' values. */
   periodTotal = 0.0;
   i = optInTimePeriod;
   while( i-- > 0 )
   {
      periodTotal += inReal[today];
      today++;
   }
   prevRMA = periodTotal / optInTimePeriod;

   /* Skip the unstable period. */
   i = TA_GetUnstablePeriod(TA_FUNC_UNST_RMA);
   while( i != 0 )
   {
      prevRMA = wAlpha * inReal[today] + wBeta * prevRMA;
      today++;
      i--;
   }

   /* Now start to write the final RMA in the caller
    * provided outReal.
    */
   outIdx = 1;
   outReal[0] = prevRMA;

   /* Now do the number of requested RMA. */
   nbRMA = (endIdx - startIdx)+1;

   while( --nbRMA != 0 )
   {
      prevRMA = wAlpha * inReal[today] + wBeta * prevRMA;
      outReal[outIdx++] = prevRMA;
      today++;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
