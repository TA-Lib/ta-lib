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
 *  110826 MF,CC  Creation (synthetic gate: copied and partially rewritten
 *                intermediates read at a lag, #583).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth28.md: one copy, so there is one thing to keep true.
 */

int synth28_lookback(int optInTimePeriod, int optInLag)
{
   return midpoint_lookback( optInTimePeriod ) + optInLag;
}

TA_RetCode synth28(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int optInLag,
   int *outBegIdx, int *outNBElement,
   double outMaxNow[],
   double outMaxLag[],
   int outSide[])
{
   double *tempA;
   double *tempD;
   TA_RetCode retCode;
   int lookbackTotal, nbElement;
   int begA, nbA, begB, nbB;
   int i;

   lookbackTotal = synth28_lookback( optInTimePeriod, optInLag );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   nbElement = endIdx - startIdx + 1;

   tempA = malloc( (nbElement+optInLag) * sizeof(double) );
   if( !tempA )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempD = malloc( (nbElement+optInLag) * sizeof(double) );
   if( !tempD )
   {
      free( tempA );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   /* Every input is consumed here, before any output is written: an output
    * may be aliased onto inReal.
    */
   retCode = midpoint( startIdx-optInLag, endIdx, inReal, optInTimePeriod,
      &begA, &nbA, tempA );
   if( retCode != TA_SUCCESS )
   {
      free( tempA );
      free( tempD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   for( i=0; i < nbA; i++ )
      tempD[i] = tempA[i] * 0.5;

   memmove( outMaxNow, &tempA[optInLag], nbElement * sizeof(double) );
   memmove( outMaxLag, &tempA[0], nbElement * sizeof(double) );

   retCode = midpoint( 0, nbA-1, tempA, 2,
      &begB, &nbB, tempA );
   if( retCode != TA_SUCCESS )
   {
      free( tempA );
      free( tempD );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   for( i=0; i < nbElement; i++ )
   {
      if( outMaxLag[i] > outMaxNow[i] )
         tempA[i] = outMaxLag[i];
   }

   for( i=0; i < nbElement; i++ )
      outSide[i] = tempD[i+optInLag] > tempA[i] ? 1 : ( tempD[i] > tempA[i] ? 2 : 0 );

   free( tempA );
   free( tempD );

   *outBegIdx = startIdx;
   *outNBElement = nbElement;

   return TA_SUCCESS;
}
