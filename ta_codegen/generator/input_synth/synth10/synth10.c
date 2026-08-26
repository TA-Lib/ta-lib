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
 *  082526 MF,CC  Creation (synthetic gate: two nullable outputs).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth10.md — one copy, so there is one thing to keep true.
 */

int synth10_lookback(void)
{
   /* Each bar is read on its own; no history is consumed. */
   return 0;
}

TA_RetCode synth10(int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx, int *outNBElement,
   double outFirstOptional[],
   double outRequired[],
   double outSecondOptional[])
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

      /* The two declinable stores. Neither advances outIdx. Halving and
       * quartering a double are exact, so these carry no rounding of their
       * own for a gate to attribute to the wrong thing.
       */
      outFirstOptional[outIdx] = bar * 0.5;
      outSecondOptional[outIdx] = bar * 0.25;

      /* The required store owns the advance. */
      outRequired[outIdx++] = bar;
      i++;
   } while( i <= endIdx );

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
