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
 *  092826 MF,CC  Creation (synthetic gate: the negation of a zero keeps its sign, #463).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth23.md — one copy, so there is one thing to keep true.
 */

int synth23_lookback(void)
{
   return 0;
}

TA_RetCode synth23(int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i;
   int outIdx;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   outIdx = 0;
   for( i = startIdx; i <= endIdx; i++ )
      outReal[outIdx++] = -( inReal[i] * 0.0 );

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
