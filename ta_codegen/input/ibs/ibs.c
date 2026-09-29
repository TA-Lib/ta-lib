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
 *  092926 MF,CC  Creation (#468).
 */

int ibs_lookback(void)
{
   /* One bar in, one bar out: the value reads only its own bar, so there is
    * nothing to warm up. bop.c:19-22's lookback, and stochf_lookback(1,1,SMA)
    * agrees, since ma_lookback returns 0 for a period of 1 (ma.c:28-29).
    */
   return 0;
}

TA_RetCode ibs(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx, i;
   double range;

   /* IBS = (Close - Low)/(High - Low): where the close sits inside its own
    * bar, 0 at the low and 1 at the high.
    */

   outIdx = 0;

   for( i=startIdx; i <= endIdx; i++ )
   {
      /* The test is exact and spelled `<= 0.0`, which is bop.c:47-48's.
       *
       * Exact, with no band: a bar's range is the difference of two prices
       * within a factor of two of each other, so it is computed exactly
       * (Sterbenz) and is never a cancellation residue there is anything to
       * absorb. A fixed band is issue #253 again -- at corpus prices scaled by
       * 2^-50 a 1e-14 band sends 250 of 252 bars to the degenerate value.
       *
       * Spelled this way round rather than `range > 0.0 ? ratio : 0.5`: a NaN
       * input compares false either way, so only this spelling lets it fall
       * through to the division and come out NaN. The inverted spelling
       * answers a finite, neutral-looking 0.5 where the input was garbage.
       *
       * 0.5 is this indicator's own neutral point, per issue #112's rule --
       * the bar's midpoint, which is what ad.c:66 already assumes when it
       * skips a bar with no range. 0.0 would read as "closed at the low",
       * which is the primary's long trigger.
       */
      range = inHigh[i]-inLow[i];
      if( range <= 0.0 )
         outReal[outIdx++] = 0.5;
      else
         outReal[outIdx++] = (inClose[i]-inLow[i])/range;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
