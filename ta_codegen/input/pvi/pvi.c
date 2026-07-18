/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  MF,CC    Mario Fortier, Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  071726 MF,CC Implement Positive Volume Index (#126).
 *
 */

int pvi_lookback(void)
{
   /* This function have no lookback needed. */
   return 0;
}

TA_RetCode pvi(int startIdx, int endIdx,
   const double inClose[],
   const double inVolume[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i;
   int outIdx;
   double prevPVI, prevClose, prevVolume, tempClose, tempVolume;

   /* The index is a running cumulative value seeded at 1000, updated only on
    * bars whose volume increased versus the prior bar (Positive Volume).
    */
   prevPVI    = 1000.0;
   prevClose  = inClose[startIdx];
   prevVolume = inVolume[startIdx];
   outIdx = 0;

   for( i=startIdx; i <= endIdx; i++ )
   {
      tempClose  = inClose[i];
      tempVolume = inVolume[i];

      /* prevClose != 0 guards the percentage-change division: a zero previous
       * close is a degenerate input that would otherwise emit NaN/Inf; carry
       * the index forward unchanged instead. Never triggers on real prices. */
      if( (tempVolume > prevVolume) && (prevClose != 0.0) )
         prevPVI += ((tempClose-prevClose)/prevClose) * prevPVI;

      outReal[outIdx++] = prevPVI;
      prevClose  = tempClose;
      prevVolume = tempVolume;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
