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
 *  092526 MF,CC  Creation (synthetic gate: a read made only in a branch, at an index that
 *                divides by a parameter which can be zero).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth21.md — one copy, so there is one thing to keep true.
 */

int synth21_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode synth21(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double seed;
   int i, outIdx, lookbackTotal, d;

   lookbackTotal = optInTimePeriod-1;
   d = optInTimePeriod-2;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   seed = 0.0;
   i = startIdx-lookbackTotal;
   while( i < startIdx )
   {
      seed += ( d != 0 ) ? inReal[i+lookbackTotal/d-1] : 0.0;
      i++;
   }

   outIdx = 0;
   i = startIdx;
   while( i <= endIdx )
   {
      outReal[outIdx++] = inReal[i] + seed;
      i++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
