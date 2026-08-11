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
 *  071726 MF,CC Implement Negative Volume Index (#126).
 *
 */

int nvi_lookback(void)
{
   /* This function have no lookback needed. */
   return 0;
}

TA_RetCode nvi(int startIdx, int endIdx,
   const double inClose[],
   const double inVolume[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i;
   int outIdx;
   double prevNVI, prevClose, prevVolume, tempClose, tempVolume, tempNVI;

   /* The index is a running cumulative value seeded at 1000, updated only on
    * bars whose volume decreased versus the prior bar (Negative Volume).
    */
   prevNVI    = 1000.0;
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
      if( (tempVolume < prevVolume) && (prevClose != 0.0) )
      {
         /* The index is a running product, so it has no upper bound: enough
          * compounding gains push it past the largest double. Keep the last
          * representable value instead of writing +/-Inf, which no caller can
          * chart and which poisons every arithmetic downstream of it. Real
          * price series never come close.
          *
          * Written as a compound assignment on the copy, exactly as the update
          * was before the guard: spelling it `a + r*a` would match the FMA
          * fusion detector and silently re-round every bar, not just the
          * overflowing one.
          */
         tempNVI = prevNVI;
         tempNVI += ((tempClose-prevClose)/prevClose) * tempNVI;
         if( IS_FINITE(tempNVI) )
            prevNVI = tempNVI;
      }

      outReal[outIdx++] = prevNVI;
      prevClose  = tempClose;
      prevVolume = tempVolume;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
