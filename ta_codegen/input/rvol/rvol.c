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
 *  090426 MF,CC  Initial version (#370).
 *  092526 MF,CC  #446 exact zero total on a dead volume window.
 *  092626 MF,CC  #446 branch-free zero count.
 *  102326 MF,CC  #554 rare reset, zero run as a mask, output first.
 */

int rvol_lookback(int optInTimePeriod)
{
   return optInTimePeriod;
}

TA_RetCode rvol(int startIdx, int endIdx,
   const double inVolume[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double periodTotal;
   double baseline;
   double todayVolume;
   double trailingVolume;
   size_t i;
   size_t outIdx;
   size_t trailingIdx;
   size_t lookbackTotal;
   int nullRun;
   int liveIn;

   /* One bar more than a moving average of the same period: today is excluded
    * from its own baseline.
    */
   lookbackTotal = (size_t)optInTimePeriod;

   if( startIdx < lookbackTotal ) {
      startIdx = lookbackTotal;
   }

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   periodTotal = 0.0;
   trailingIdx = startIdx - lookbackTotal;

   /* Consecutive zero-volume bars, ending at the last one added. Once they
    * fill a window the total is exactly zero, where add-then-subtract would
    * leave the rounding residue of the volumes that departed, of either sign.
    * The run advances through a mask: as a select or a product gcc compiles
    * it to a branch that mispredicts on scattered zero volumes. The test is
    * fabs(v) <= 0.0 rather than == 0.0: the same result, NaN included, from
    * one flag instead of two.
    */
   nullRun = 0;
   i = trailingIdx;
   while( i < startIdx ) {
      periodTotal += (double)(inVolume[i]);
      liveIn = fabs(inVolume[i]) <= 0.0 ? 0 : 1;
      nullRun = (nullRun + 1) & (liveIn - 1);
      i = i + 1;
   }

   outIdx = 0;
   while( i <= endIdx )
   {
      /* Drop the trailing bar BEFORE adding today's. Up to the first dead
       * window, that order makes each baseline bit-identical to the moving
       * average of the same period at the previous bar; the reverse order
       * differs only in the last ulp, so no tolerance can tell the two apart.
       */
      baseline = periodTotal / (double)optInTimePeriod;
      trailingVolume = (double)(inVolume[trailingIdx]);
      trailingIdx = trailingIdx + 1;
      todayVolume = (double)(inVolume[i]);
      i = i + 1;

      /* After both reads (input and output may be one buffer) and before the
       * total moves, or the divide keeps the old total live across the reset.
       */
      outReal[outIdx] = todayVolume / baseline;
      outIdx = outIdx + 1;

      periodTotal -= trailingVolume;
      periodTotal += todayVolume;
      liveIn = fabs(todayVolume) <= 0.0 ? 0 : 1;
      nullRun = (nullRun + 1) & (liveIn - 1);
      if( TA_UNLIKELY(nullRun >= optInTimePeriod) )
         periodTotal = 0.0;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
