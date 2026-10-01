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
 *  100126 MF,CC  Creation (synthetic gate: a display shift that differs per output, #489).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth24.md: one copy, so there is one thing to keep true.
 */

int synth24_lookback(int optInTimePeriod)
{
   return 0;
}

int synth24_display_shift(int optInTimePeriod, int outputIdx)
{
   int shift;

   shift = 0;
   if( outputIdx == 1 )
      shift = optInTimePeriod;
   else if( outputIdx == 2 )
      shift = -( optInTimePeriod / outputIdx + 1 );

   return shift;
}

TA_RetCode synth24(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outOwnBar[],
   double outAhead[],
   double outBehind[])
{
   double tempReal;
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
   {
      /* One read, before any store: an output may alias inReal. */
      tempReal = inReal[i];
      outOwnBar[outIdx] = tempReal;
      outAhead[outIdx] = tempReal * 2.0;
      outBehind[outIdx] = tempReal * 4.0;
      outIdx++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
