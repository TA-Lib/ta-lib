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
 *  082726 MF,CC  Creation (synthetic gate: a cross-indicator call, five
 *                guard shapes).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth13.md — one copy, so there is one thing to keep true.
 */

int synth13_lookback(int optInTimePeriod)
{
   /* Every leg below calls sma() at the same period, so its lookback is the
    * lookback synth13 itself must honor. */
   return sma_lookback( optInTimePeriod );
}

TA_RetCode synth13(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double *scratch;
   TA_RetCode retCode;
   TA_RetCode savedRetCode;
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

   /* Four legs, four guard shapes, one destination. Each leg recomputes the
    * same sma() into `scratch`, and outReal is written ONCE at the end --
    * after every read of inReal. Whole-buffer in place is legal in the batch
    * tier (rule N4), so a leg that wrote outReal before a later leg re-read
    * inReal would corrupt its own input.
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
   savedRetCode = TA_SUCCESS;

   /* Leg A: a bare guard. Folded away whole in Rust/Java/C#, kept in C. */
   retCode = sma( startIdx, endIdx, inReal, optInTimePeriod,
                  &legBegIdx, &legNbElement, scratch );
   if( retCode != TA_SUCCESS )
   {
      if( bufferIsAllocated ) { free( scratch ); }
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Leg B: the error half ORed with a live count test, writing through the
    * caller's own out-parameters -- as bbands.c and stoch.c both do. The
    * error half goes; the count test survives alone. */
   retCode = sma( startIdx, endIdx, inReal, optInTimePeriod,
                  outBegIdx, outNBElement, scratch );
   if( (retCode != TA_SUCCESS ) || ((int)*outNBElement == 0) )
   {
      if( bufferIsAllocated ) { free( scratch ); }
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Leg C: a statement between the call and its guard that READS the code
    * variable. The scan must stop there, so the guard survives everywhere. */
   retCode = sma( startIdx, endIdx, inReal, optInTimePeriod,
                  &legBegIdx, &legNbElement, scratch );
   savedRetCode = retCode;
   if( retCode != TA_SUCCESS )
   {
      if( bufferIsAllocated ) { free( scratch ); }
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Leg D: an equality test. Only `!=` is classified, so this is refused and
    * survives untouched everywhere. Its body is bookkeeping only, so no
    * output depends on whether it runs. */
   retCode = sma( startIdx, endIdx, inReal, optInTimePeriod,
                  &legBegIdx, &legNbElement, scratch );
   if( retCode == TA_SUCCESS )
   {
      savedRetCode = retCode;
   }

   /* Every read of inReal is done; only now is the caller's buffer written. */
   for( i = 0; i < legNbElement; i++ )
      outReal[i] = 4.0 * scratch[i];

   /* Standalone flag-guarded deallocation, unrelated to any cross-call guard
    * above: gone in Rust/Java/C#, kept in C. */
   if( bufferIsAllocated )
   {
      free( scratch );
   }

   *outBegIdx = startIdx;
   if( savedRetCode == TA_SUCCESS )
      *outNBElement = legNbElement;
   else
      *outNBElement = 0;

   return TA_SUCCESS;
}
