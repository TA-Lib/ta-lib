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
 *  082726 MF,CC  Creation (synthetic gate: a cross-indicator call feeding
 *                mixed real and integer outputs).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth14.md — one copy, so there is one thing to keep true.
 */

int synth14_lookback(int optInTimePeriod)
{
   /* The single leg below calls sma() at this period, so its lookback is the
    * lookback synth14 itself must honor. */
   return sma_lookback( optInTimePeriod );
}

TA_RetCode synth14(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outAvg[],
   int    outSide[],
   double outTwice[])
{
   double *scratch;
   TA_RetCode retCode;
   int legBegIdx, legNbElement;
   int bufferIsAllocated;
   int lookbackTotal;
   int i;

   lookbackTotal = sma_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* The cross-call needs a destination that is not one of the outputs: two of
    * them are double and one is int, so no single output can hold the sub-call's
    * real-valued series for all three to read afterwards.
    */
   scratch = malloc( (endIdx-startIdx+1) * sizeof(double) );
   if( !scratch )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   bufferIsAllocated = 1;
   legNbElement = 0;

   retCode = sma( startIdx, endIdx, inReal, optInTimePeriod,
                  &legBegIdx, &legNbElement, scratch );
   if( retCode != TA_SUCCESS )
   {
      if( bufferIsAllocated ) { free( scratch ); }
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* One sub-call result, read three ways. Halving and doubling are exact, and
    * the side comes from comparisons, so every value here is reproducible
    * bit-for-bit without depending on a rounding mode.
    */
   for( i = 0; i < legNbElement; i++ )
   {
      outAvg[i] = scratch[i] * 0.5;
      outSide[i] = scratch[i] > 0.0 ? 1 : ( scratch[i] < 0.0 ? -1 : 0 );
      outTwice[i] = scratch[i] * 2.0;
   }

   if( bufferIsAllocated )
   {
      free( scratch );
   }

   *outBegIdx = startIdx;
   *outNBElement = legNbElement;

   return TA_SUCCESS;
}
