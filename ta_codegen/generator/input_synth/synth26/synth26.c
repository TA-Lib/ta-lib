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
 *  110826 MF,CC  Creation (synthetic gate: a produced series read at
 *                several depths, #583).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth26.md: one copy, so there is one thing to keep true.
 */

int synth26_lookback(int optInTimePeriod)
{
   return midpoint_lookback( optInTimePeriod ) + 3;
}

TA_RetCode synth26(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   int outSum[],
   int outRise[])
{
   double *tempX;
   double *tempM;
   double *work;
   double s;
   TA_RetCode retCode;
   int lookbackTotal, nbElement;
   int anchor, today, outIdx;
   int begM, nbM;
   int i;

   lookbackTotal = synth26_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   nbElement = endIdx - startIdx + 1;

   tempX = malloc( (nbElement+lookbackTotal) * sizeof(double) );
   if( !tempX )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempM = malloc( nbElement * sizeof(double) );
   if( !tempM )
   {
      free( tempX );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   anchor = startIdx - lookbackTotal;
   outIdx = 0;
   today = anchor;
   while( today <= endIdx )
   {
      tempX[outIdx++] = inReal[today] * 0.5;
      today++;
   }

   retCode = midpoint( lookbackTotal, outIdx-1, tempX, optInTimePeriod,
      &begM, &nbM, tempM );
   if( retCode != TA_SUCCESS )
   {
      free( tempX );
      free( tempM );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   work = tempM;

   for( i=0; i < nbM; i++ )
   {
      s = (tempX[i+lookbackTotal] + tempX[i+lookbackTotal-1])
        + (tempX[i+lookbackTotal-2] + tempX[i+lookbackTotal-3]);
      outSum[i] = s > work[i] ? 1 : ( s < work[i] ? -1 : 0 );
      outRise[i] = inReal[startIdx+i-1] < inReal[startIdx+i] ? 1 : 0;
   }

   free( tempX );
   free( tempM );

   *outBegIdx = startIdx;
   *outNBElement = nbElement;

   return TA_SUCCESS;
}
