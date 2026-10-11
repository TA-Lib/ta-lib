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
 *  100726 MF,CC  Initial version (#490).
 *
 */

int ichimoku_display_shift(int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod, int outputIdx)
{
   /* rL9: a shift of s means a chart draws the value computed at bar i at
    * bar i + s. The two spans are the leading ones -- Goichi Hosoda draws
    * them kijun bars into the future, which is what makes the cloud sit
    * ahead of price -- so their shift is POSITIVE, where dpo.c's and
    * fractal.c's are negative. The conversion and base lines are drawn on
    * the bar that computed them.
    *
    * The shift reads the optional parameters and the output index only,
    * never a setting (rL9).
    */
   if( outputIdx == 2 || outputIdx == 3 )
      return optInKijunPeriod;
   return 0;
}

int ichimoku_lookback(int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod)
{
   int longest;

   /* Each line is a midpoint over its own window, so each needs its own
    * window filled; the first bar that has all of them is the longest one's.
    * The MAX is not simplified to the Senkou B period: nothing orders the
    * three, and a kijun longer than senkouB dominates.
    *
    * Span A is the mean of the other two lines, so it adds nothing: it is
    * ready on the bar they both are.
    */
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
   double *tempB;

   /* PROTOTYPE (#490 Q7): each line IS a midpoint over its own window, which is
    * exactly what midprice computes, so the three scans are three midprice calls
    * and Span A is the mean of two of them. MEASURED bit-identical to the fused
    * loop over four parameter triples on the suite's corpus, every line, before
    * this was written.
    *
    * The point is the stream tier: three windows of different periods cannot be
    * one extrema automaton (the census refuses with "expected exactly one
    * window-start variable"), but a composed body is a different tier.
    *
    * The three results go to temporaries and are copied at the end: every read of
    * high and low has to happen before the first write to a caller buffer, or an
    * output aliased onto an input is read after it has been overwritten.
    */
   lookbackTotal = ichimoku_lookback( optInTenkanPeriod, optInKijunPeriod,
      optInSenkouBPeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   n = endIdx - startIdx + 1;

   tempT = malloc( n * sizeof(double) );
   if( !tempT )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempK = malloc( n * sizeof(double) );
   if( !tempK )
   {
      free( tempT );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempB = malloc( n * sizeof(double) );
   if( !tempB )
   {
      free( tempT );
      free( tempK );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   retCode = midprice( startIdx, endIdx, inHigh, inLow, optInTenkanPeriod,
      &tempBegIdx, &tempNbElement, tempT );
   if( retCode != TA_SUCCESS )
   {
      free( tempT );
      free( tempK );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = midprice( startIdx, endIdx, inHigh, inLow, optInKijunPeriod,
      &tempBegIdx, &tempNbElement, tempK );
   if( retCode != TA_SUCCESS )
   {
      free( tempT );
      free( tempK );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = midprice( startIdx, endIdx, inHigh, inLow, optInSenkouBPeriod,
      &tempBegIdx, &tempNbElement, tempB );
   if( retCode != TA_SUCCESS )
   {
      free( tempT );
      free( tempK );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Span A is the mean of the two lines, which medprice is over any two series. */
   retCode = medprice( 0, n-1, tempT, tempK,
      &tempBegIdx, &tempNbElement, outSenkouSpanA );
   if( retCode != TA_SUCCESS )
   {
      free( tempT );
      free( tempK );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   for( i = 0; i < n; i++ )
   {
      outTenkanSen[i] = tempT[i];
      outKijunSen[i] = tempK[i];
      outSenkouSpanB[i] = tempB[i];
   }

   free( tempT );
   free( tempK );
   free( tempB );

   *outBegIdx = startIdx;
   *outNBElement = n;

   return TA_SUCCESS;
}
