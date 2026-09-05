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
 *  090426 MF,CC  Initial version (#363).
 */

int dpo_lookback(int optInTimePeriod)
{
   /* The max is load-bearing, not cosmetic. The displaced read reaches back
    * optInTimePeriod/2+1 bars, which exceeds the moving average's own
    * optInTimePeriod-1 at a period of 2, and a bare optInTimePeriod-1 would
    * then read inReal[-1].
    */
   return max( optInTimePeriod - 1, optInTimePeriod / 2 + 1 );
}

TA_RetCode dpo(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double periodTotal, tempReal, dispVal;
   int i, outIdx, trailingIdx, dispIdx, lookbackTotal;

   /* The running sum is accumulated, snapshot and divided in exactly the order
    * ta_codegen/input/sma/sma.c uses, so this fused loop is bit-for-bit equal
    * to a TA_SMA anchored at the same startIdx. That equality is what makes
    * the composite differential in test_dpo.c a memcmp instead of a tolerance
    * argument, and it is lost by dividing once into a reciprocal or by
    * reordering the add/snapshot/subtract.
    */

   lookbackTotal = dpo_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   periodTotal = 0.0;
   trailingIdx = startIdx - (optInTimePeriod - 1);
   dispIdx = startIdx - (optInTimePeriod / 2 + 1);

   i = trailingIdx;
   while( i < startIdx )
   {
      periodTotal += inReal[i];
      i = i + 1;
   }

   outIdx = 0;
   while( i <= endIdx )
   {
      periodTotal += inReal[i];
      i = i + 1;
      tempReal = periodTotal;
      periodTotal -= inReal[trailingIdx];
      trailingIdx = trailingIdx + 1;

      /* Both reads precede the store. Either cursor can EQUAL outIdx -- the
       * displaced one whenever startIdx equals the displacement, the trailing
       * one whenever startIdx sits at the lookback -- so a store hoisted above
       * them would read back what it had just overwritten when the caller
       * aliases outReal over inReal.
       */
      dispVal = inReal[dispIdx];
      dispIdx = dispIdx + 1;

      outReal[outIdx] = dispVal - tempReal / (double)optInTimePeriod;
      outIdx = outIdx + 1;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
