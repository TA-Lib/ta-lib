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
 *  092526 MF,CC  Creation (synthetic gate: a loop whose Rust windows never fit, so it
 *                runs as written).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth22.md — one copy, so there is one thing to keep true.
 */

int synth22_lookback(void)
{
   return 1;
}

TA_RetCode synth22(int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double x;
   int i;
   int outIdx, havePrev;

   if( startIdx < 1 )
      startIdx = 1;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   outIdx = 0;
   havePrev = 0;
   for( i = startIdx; i <= endIdx; i++ )
   {
      x = inReal[i] - inReal[i-1];
      x += ( havePrev != 0 ) ? outReal[outIdx-1] * 0.5 : 0.0;
      outReal[outIdx++] = x;
      havePrev = 1;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
