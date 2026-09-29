/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092926 KL,CC  Creation (#468).
 *  092926 MF,CC  Store the quotient, then overwrite it, so the loop vectorizes.
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
      /* An exact test: the range of one bar carries the quote unit, so any
       * band would flip every bar of a small enough instrument to 0.5. Spelled
       * <= so a NaN input keeps the quotient's NaN.
       *
       * Store, then overwrite: gcc keeps a guarded or selected quotient
       * scalar under -ftrapping-math.
       */
      range = inHigh[i]-inLow[i];
      outReal[outIdx] = (inClose[i]-inLow[i])/range;
      if( range <= 0.0 )
         outReal[outIdx] = 0.5;
      outIdx++;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
