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
 *  110826 MF,CC  Creation (synthetic gate: one series read at a lag in
 *                both arms of a mode switch, #583).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth27.md: one copy, so there is one thing to keep true.
 */

int synth27_lookback(int optInTimePeriod, int optInLag, int optInMode)
{
   (void)optInMode;
   return midpoint_lookback( optInTimePeriod ) + optInLag + 1;
}

TA_RetCode synth27(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int optInLag,
   int optInMode,
   int *outBegIdx, int *outNBElement,
   int outUp[])
{
   double *tempA;
   double *tempB;
   TA_RetCode retCode;
   int lookbackTotal, nbElement;
   int begA, nbA, begB, nbB;
   int i;

   lookbackTotal = synth27_lookback( optInTimePeriod, optInLag, optInMode );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   nbElement = endIdx - startIdx + 1;

   tempA = malloc( (nbElement+optInLag+1) * sizeof(double) );
   if( !tempA )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempB = malloc( nbElement * sizeof(double) );
   if( !tempB )
   {
      free( tempA );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   retCode = midpoint( startIdx-optInLag-1, endIdx, inReal, optInTimePeriod,
      &begA, &nbA, tempA );
   if( retCode != TA_SUCCESS )
   {
      free( tempA );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = midpoint( startIdx, endIdx, inReal, optInTimePeriod,
      &begB, &nbB, tempB );
   if( retCode != TA_SUCCESS )
   {
      free( tempA );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   if( optInMode == 0 )
   {
      for( i=0; i < nbElement; i++ )
         outUp[i] = tempA[i+1] > tempB[i] ? 1 : 0;
   }
   else
   {
      for( i=0; i < nbElement; i++ )
         outUp[i] = tempA[i+optInLag+1] > tempA[i] ? 1 : ( tempA[i+1] > tempB[i] ? 2 : 0 );
   }

   free( tempA );
   free( tempB );

   *outBegIdx = startIdx;
   *outNBElement = nbElement;

   return TA_SUCCESS;
}
