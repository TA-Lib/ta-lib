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
 *  092526 MF,CC  Creation (synthetic gate: a countdown nested in a countdown inside the main
 *                loop).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth20.md — one copy, so there is one thing to keep true.
 */

int synth20_lookback(void)
{
   return 1;
}

TA_RetCode synth20(int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double acc;
   int i, outIdx, a, b;

   if( startIdx < 1 )
      startIdx = 1;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   outIdx = 0;
   i = startIdx;
   while( i <= endIdx )
   {
      acc = 0.0;
      for( a = 1; a >= 0; a-- )
         for( b = 1; b >= 0; b-- )
            acc += inReal[i-a];
      outReal[outIdx++] = acc;
      i++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
