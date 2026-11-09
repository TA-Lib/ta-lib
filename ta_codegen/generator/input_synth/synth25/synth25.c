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
 *  110826 MF,CC  Creation (synthetic gate: an intermediate read across
 *                series at three shifts, #583).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth25.md: one copy, so there is one thing to keep true.
 */

int synth25_lookback(int optInTimePeriod, int optInShiftA, int optInShiftB, int optInShiftC)
{
   int back;

   back = optInShiftA;
   if( optInShiftB > back )
      back = optInShiftB;
   if( optInShiftC > back )
      back = optInShiftC;

   return mom_lookback( 1 ) + midpoint_lookback( optInTimePeriod ) + back;
}

TA_RetCode synth25(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int optInShiftA,
   int optInShiftB,
   int optInShiftC,
   int *outBegIdx, int *outNBElement,
   int outAB[],
   int outAC[])
{
   double *tempM;
   double *tempA;
   double *tempB;
   double *tempC;
   TA_RetCode retCode;
   int lookbackTotal, nbElement;
   int anchor, lastLookback, localEnd;
   int begM, nbM, begA, nbA, begB, nbB, begC, nbC;
   int i;

   lookbackTotal = synth25_lookback( optInTimePeriod, optInShiftA, optInShiftB, optInShiftC );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   nbElement = endIdx - startIdx + 1;
   anchor = startIdx - lookbackTotal + 1;
   lastLookback = lookbackTotal - 1;
   localEnd = endIdx - anchor;

   tempM = malloc( (nbElement+lookbackTotal-1) * sizeof(double) );
   if( !tempM )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempA = malloc( (nbElement+optInShiftA) * sizeof(double) );
   if( !tempA )
   {
      free( tempM );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempB = malloc( nbElement * sizeof(double) );
   if( !tempB )
   {
      free( tempM );
      free( tempA );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempC = malloc( nbElement * sizeof(double) );
   if( !tempC )
   {
      free( tempM );
      free( tempA );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   retCode = mom( anchor, endIdx, inReal, 1,
      &begM, &nbM, tempM );
   if( retCode != TA_SUCCESS )
   {
      free( tempM );
      free( tempA );
      free( tempB );
      free( tempC );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = midpoint( lastLookback-optInShiftA, localEnd, tempM, optInTimePeriod,
      &begA, &nbA, tempA );
   if( retCode != TA_SUCCESS )
   {
      free( tempM );
      free( tempA );
      free( tempB );
      free( tempC );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = midpoint( lastLookback-optInShiftB, localEnd-optInShiftB, tempM, optInTimePeriod,
      &begB, &nbB, tempB );
   if( retCode != TA_SUCCESS )
   {
      free( tempM );
      free( tempA );
      free( tempB );
      free( tempC );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = midpoint( lastLookback-optInShiftC, localEnd-optInShiftC, tempM, optInTimePeriod,
      &begC, &nbC, tempC );
   if( retCode != TA_SUCCESS )
   {
      free( tempM );
      free( tempA );
      free( tempB );
      free( tempC );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   for( i=0; i < nbElement; i++ )
   {
      outAB[i] = tempA[i] > tempB[i] ? 1 : ( tempA[i] < tempB[i] ? -1 : 0 );
      outAC[i] = tempA[i] > tempC[i] ? 1 : ( tempA[i] < tempC[i] ? -1 : 0 );
   }

   free( tempM );
   free( tempA );
   free( tempB );
   free( tempC );

   *outBegIdx = startIdx;
   *outNBElement = nbElement;

   return TA_SUCCESS;
}
