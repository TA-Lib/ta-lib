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
 *  092426 MF,CC  Creation (synthetic gate: integer buffer stored in a step loop, #439).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth15.md — one copy, so there is one thing to keep true.
 */

int synth15_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode synth15(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double newValue;
   int lookbackTotal, outIdx, i, j, inIdx, cut;

   /* The previous lookbackTotal inputs, truncated to int, oldest first. */
   CIRCBUF_PROLOG(hist,int,30);

   if( optInTimePeriod == 1 )
   {
      *outBegIdx    = startIdx;
      *outNBElement = endIdx-startIdx+1;
      inIdx = startIdx;
      for( i = 0; i < (int)*outNBElement; i++ )
         outReal[i] = inReal[inIdx++];
      return TA_SUCCESS;
   }

   lookbackTotal = optInTimePeriod-1;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   CIRCBUF_INIT( hist, int, lookbackTotal );

   j = 0;
   i = startIdx-lookbackTotal;
   while( i < startIdx )
   {
      newValue = inReal[i];
      if( newValue > -1e6 && newValue < 1e6 )
         hist[j] = (int)newValue;
      else
         hist[j] = 0;
      j++;
      i++;
   }

   /* Every buffer store sits below the output store, so the whole answer is
    * derived read-only above it.
    */
   outIdx = 0;
   do
   {
      newValue = inReal[i];
      outReal[outIdx++] = newValue - (double)hist[0];

      j = 0;
      while( j < lookbackTotal-1 )
      {
         hist[j] = hist[j+1];
         j++;
      }
      if( newValue > -1e6 && newValue < 1e6 )
         cut = (int)newValue;
      else
         cut = 0;
      hist[lookbackTotal-1] = cut;
      i++;
   } while( i <= endIdx );

   CIRCBUF_DESTROY(hist);

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
