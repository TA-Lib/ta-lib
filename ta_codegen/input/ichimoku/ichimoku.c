/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100726 KL,CC  Creation (#490).
 *  101126 MF,CC  Two scratch buffers; Senkou B is written in place (#490).
 *
 */

int ichimoku_display_shift(int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod, int outputIdx)
{
   /* Positive: the two spans are drawn ahead of the bar that computed them. */
   if( outputIdx == 2 || outputIdx == 3 )
      return optInKijunPeriod;
   return 0;
}

int ichimoku_lookback(int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod)
{
   int longest;

   /* Nothing orders the three periods: any of them can be the longest. */
   longest = optInTenkanPeriod;
   if( optInKijunPeriod > longest )
      longest = optInKijunPeriod;
   if( optInSenkouBPeriod > longest )
      longest = optInSenkouBPeriod;

   return longest - 1;
}

TA_RetCode ichimoku(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInTenkanPeriod,
   int optInKijunPeriod,
   int optInSenkouBPeriod,
   int *outBegIdx, int *outNBElement,
   double outTenkanSen[],
   double outKijunSen[],
   double outSenkouSpanA[],
   double outSenkouSpanB[])
{
   TA_RetCode retCode;
   int lookbackTotal, n, i;
   int tempBegIdx, tempNbElement;
   double *tempT;
   double *tempK;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = ichimoku_lookback( optInTenkanPeriod, optInKijunPeriod,
      optInSenkouBPeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   n = endIdx - startIdx + 1;

   tempT = malloc( n * sizeof(double) );
   if( !tempT )
      return TA_ALLOC_ERR;

   tempK = malloc( n * sizeof(double) );
   if( !tempK )
   {
      free( tempT );
      return TA_ALLOC_ERR;
   }

   /* An output may alias inHigh or inLow: no output is written before the
    * last midprice call, which is the last read of either input.
    */
   retCode = midprice( startIdx, endIdx, inHigh, inLow, optInTenkanPeriod,
      &tempBegIdx, &tempNbElement, tempT );
   if( retCode != TA_SUCCESS )
   {
      free( tempT );
      free( tempK );
      return retCode;
   }

   retCode = midprice( startIdx, endIdx, inHigh, inLow, optInKijunPeriod,
      &tempBegIdx, &tempNbElement, tempK );
   if( retCode != TA_SUCCESS )
   {
      free( tempT );
      free( tempK );
      return retCode;
   }

   retCode = midprice( startIdx, endIdx, inHigh, inLow, optInSenkouBPeriod,
      &tempBegIdx, &tempNbElement, outSenkouSpanB );
   if( retCode != TA_SUCCESS )
   {
      free( tempT );
      free( tempK );
      return retCode;
   }

   /* Span A halves the two rounded midpoints. (hh+ll+hh+ll)/4 is a different
    * double in the last bit.
    */
   retCode = medprice( 0, n-1, tempT, tempK,
      &tempBegIdx, &tempNbElement, outSenkouSpanA );
   if( retCode != TA_SUCCESS )
   {
      free( tempT );
      free( tempK );
      return retCode;
   }

   for( i = 0; i < n; i++ )
   {
      outTenkanSen[i] = tempT[i];
      outKijunSen[i] = tempK[i];
   }

   free( tempT );
   free( tempK );

   *outBegIdx = startIdx;
   *outNBElement = n;

   return TA_SUCCESS;
}
