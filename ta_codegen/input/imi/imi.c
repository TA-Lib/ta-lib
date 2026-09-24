/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AB       Anatoliy Belsky
 *  MF       Mario Fortier
 *  WZ       wony (github @wony-zheng)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  181012 AB    Initial Version
 *  070526 MF,CC  Fix #98: the unstable period grew the summation window
 *                to period+u bars; window is now always 'period'.
 *  070726 WZ,CC  (#14) IMI has no unstable period; drop the unstable-period
 *                term from the lookback so TA_SetUnstablePeriod is a no-op.
 *  071326 MF,CC  Fix #112: an all-flat window (every close==open) leaves
 *                upsum==downsum==0, so 100*(0/0) emitted NaN from a *successful*
 *                call. Guard the divide, returning IMI's neutral center 50.0.
 *  092426 MF,CC  #440 ratio once per bar, not per element; branch-free split.
 */

int imi_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1;
}

TA_RetCode imi(int startIdx, int endIdx,
   const double inOpen[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int lookback, outIdx = 0;

   lookback = imi_lookback( optInTimePeriod );

   if(startIdx < lookback)
      startIdx = lookback;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx ) {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx = startIdx;

   while (startIdx <= endIdx) {
      double upsum = .0, downsum = .0;
      int i;

      for (i = startIdx - (optInTimePeriod - 1); i <= startIdx; i++) {
         double diff = inClose[i] - inOpen[i];
         /* max(diff, 0) spelled with fabs: every backend compiles it branch-free.
          * A ternary or if/else retires fewer instructions but branches (Java
          * always) and mispredicts on random data. */
         double up = (diff + fabs(diff)) * 0.5;

         upsum += up;
         downsum += up - diff;
      }

      /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
       * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
       * oscillator, so no up/down bias returns its neutral center, 50.0. */
      outReal[outIdx] = (upsum + downsum) == 0.0 ? 50.0 : 100.0*(upsum/(upsum + downsum));

      startIdx++;
      outIdx++;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
