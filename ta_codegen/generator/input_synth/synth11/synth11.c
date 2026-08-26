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
 *  082526 MF,CC  Creation (synthetic gate: a third integer output, and a
 *                cross-typed output pair).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth11.md — one copy, so there is one thing to keep true.
 */

int synth11_lookback(void)
{
   /* Each bar is read on its own; no history is consumed. */
   return 0;
}

TA_RetCode synth11(int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx, int *outNBElement,
   int outAbove[],
   int outBelow[],
   int outLarge[])
{
   double bar;
   int i, outIdx;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   outIdx = 0;
   i = startIdx;

   do
   {
      bar = inReal[i];

      /* Comparisons, not conversions: exact and language-neutral. The third
       * store owns the cursor.
       */
      outAbove[outIdx] = bar > 0.0 ? 1 : 0;
      outBelow[outIdx] = bar < 0.0 ? 1 : 0;
      outLarge[outIdx++] = bar > 1000.0 ? 1 : 0;
      i++;
   } while( i <= endIdx );

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
