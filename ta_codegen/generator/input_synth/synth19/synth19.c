/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF,CC    Mario Fortier, Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  092526 MF,CC Creation (synthetic gate: memset, #444 G1)
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth19.md — one copy, so there is one thing to keep true.
 */

int synth19_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode synth19(int    startIdx,
   int    endIdx,
   const double inReal[],
   int    optInTimePeriod,
   int   *outBegIdx,
   int   *outNBElement,
   int    outInteger[])
{
   int *countBuf;
   double *sumBuf;
   int outIdx;
   int i, k, v, seedSumInt, seedCountInt, lookbackTotal;
   double seedSum, seedCountReal, tempReal;

   lookbackTotal = (optInTimePeriod-1);
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   countBuf = malloc(8 * sizeof(int));
   if( !countBuf )
      return TA_ALLOC_ERR;
   sumBuf = malloc(8 * sizeof(double));
   if( !sumBuf )
   {
      free( countBuf );
      return TA_ALLOC_ERR;
   }

   v = 0;
   for( k = 0; k < 8; k++ )
   {
      v += 1;
      tempReal = inReal[startIdx - lookbackTotal + (k % optInTimePeriod)];
      if( !(tempReal >= 0.0) || !(tempReal < 1000000.0) )
         tempReal = 0.0;
      countBuf[k] = (int)tempReal;
      countBuf[k] &= 7;
      countBuf[k] += v * 8;
      sumBuf[k] = tempReal + (double)v;
   }
   memset( countBuf, 0, 2 * sizeof(int) );
   memset( &countBuf[5], 0, 3 * sizeof(int) );
   memset( sumBuf, 0, 1 * sizeof(double) );
   memset( &sumBuf[5], 0, 2 * sizeof(double) );

   seedSum = 0.0;
   seedCountReal = 0.0;
   for( k = 0; k < 8; k++ )
   {
      seedSum += sumBuf[k];
      seedCountReal += (double)countBuf[k];
   }
   free( countBuf );
   free( sumBuf );
   seedSumInt = (int)seedSum;
   seedCountInt = (int)seedCountReal;

   outIdx = 0;
   for( i = startIdx; i <= endIdx; i++ )
   {
      tempReal = inReal[i];
      if( !(tempReal >= 0.0) || !(tempReal < 1000000.0) )
         tempReal = 0.0;
      v = (int)tempReal;
      v &= 7;
      outInteger[outIdx++] = seedCountInt * 1000000 + seedSumInt * 10 + v;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
