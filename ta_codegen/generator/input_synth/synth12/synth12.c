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
 *  082726 MF,CC  Creation (synthetic gate: real and integer outputs in one
 *                function).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth12.md — one copy, so there is one thing to keep true.
 */

int synth12_lookback(void)
{
   /* Each bar is read on its own; no history is consumed. */
   return 0;
}

TA_RetCode synth12(int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx, int *outNBElement,
   double outHalf[],
   int    outSign[],
   double outQuarter[])
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

      /* Halving and quartering a double are exact, and the sign comes from
       * comparisons rather than a conversion, so no store here carries any
       * rounding of its own for a gate to attribute to the wrong thing.
       */
      outHalf[outIdx] = bar * 0.5;
      outSign[outIdx] = bar > 0.0 ? 1 : ( bar < 0.0 ? -1 : 0 );

      /* The trailing real store owns the cursor. */
      outQuarter[outIdx++] = bar * 0.25;
      i++;
   } while( i <= endIdx );

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
