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
 *  090426 MF,CC  First version (issue #364).
 */

int pvt_lookback(void)
{
   /* This function have no lookback needed. */
   return 0;
}

TA_RetCode pvt(int startIdx, int endIdx,
   const double inClose[],
   const double inVolume[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i;
   int outIdx;
   double prevPVT, prevClose, tempClose;

   prevPVT = 0.0;
   prevClose = inClose[startIdx];
   outIdx = 0;

   for( i=startIdx; i <= endIdx; i++ )
   {
      tempClose = inClose[i];

      /* Exact test, never an epsilon band: a band carries the quote unit and
       * would zero the indicator for every instrument priced under it (#253).
       * A zero previous close contributes nothing rather than Inf/NaN (#112).
       */
      if( prevClose != 0.0 )
         prevPVT += ((tempClose-prevClose)/prevClose) * inVolume[i];

      outReal[outIdx++] = prevPVT;
      prevClose = tempClose;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
