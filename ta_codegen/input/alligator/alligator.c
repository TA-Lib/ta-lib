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
 *  110726 MF,CC  First version (issue #584).
 */

int alligator_display_shift(int optInJawPeriod, int optInJawShift,
   int optInTeethPeriod, int optInTeethShift,
   int optInLipsPeriod, int optInLipsShift,
   int outputIdx)
{
   if( outputIdx == 0 )
      return optInJawShift;
   if( outputIdx == 1 )
      return optInTeethShift;
   return optInLipsShift;
}

int alligator_lookback(int optInJawPeriod, int optInJawShift,
   int optInTeethPeriod, int optInTeethShift,
   int optInLipsPeriod, int optInLipsShift)
{
   int jawLookback;
   int teethLookback;
   int lipsLookback;
   int lookback;

   (void)optInJawShift;
   (void)optInTeethShift;
   (void)optInLipsShift;

   /* Each term is exactly its leg's rma_lookback, never restated, so all three
    * legs inherit TA_FUNC_UNST_RMA. Nothing orders the periods: any leg can be
    * the longest.
    */
   jawLookback = rma_lookback( optInJawPeriod );
   teethLookback = rma_lookback( optInTeethPeriod );
   lipsLookback = rma_lookback( optInLipsPeriod );

   lookback = jawLookback > teethLookback ? jawLookback : teethLookback;
   return lookback > lipsLookback ? lookback : lipsLookback;
}

TA_RetCode alligator(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInJawPeriod,
   int optInJawShift,
   int optInTeethPeriod,
   int optInTeethShift,
   int optInLipsPeriod,
   int optInLipsShift,
   int *outBegIdx, int *outNBElement,
   double outJaw[],
   double outTeeth[],
   double outLips[])
{
   TA_RetCode retCode;
   int lookbackTotal;
   int mpStartIdx;
   int tempBegIdx, tempNbElement;
   double *tempMP;

   lookbackTotal = alligator_lookback( optInJawPeriod, optInJawShift,
      optInTeethPeriod, optInTeethShift,
      optInLipsPeriod, optInLipsShift );

   /* Return before touching anything, so that an input which stops short of
    * endIdx is never read past its end.
    */
   if( lookbackTotal > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   mpStartIdx = startIdx - lookbackTotal;

   tempMP = malloc((endIdx-mpStartIdx+1) * sizeof(double));
   if( !tempMP )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   /* Every price is consumed here, before any output is written: an output
    * may be aliased onto inHigh or inLow.
    */
   retCode = medprice( mpStartIdx, endIdx, inHigh, inLow,
      &tempBegIdx, &tempNbElement, tempMP );

   if( retCode != TA_SUCCESS )
   {
      free( tempMP );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Each leg is entered at startIdx (local index lookbackTotal) and seeds at
    * its OWN lookback before it, so each is exactly TA_RMA over the bars it
    * reads and no leg moves when another leg's period changes. A caller who
    * wants the short legs converged sets TA_FUNC_UNST_RMA.
    */
   retCode = rma( lookbackTotal, endIdx-mpStartIdx, tempMP,
      optInJawPeriod,
      &tempBegIdx, &tempNbElement, outJaw );

   if( (retCode != TA_SUCCESS) || (tempNbElement == 0) )
   {
      free( tempMP );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = rma( lookbackTotal, endIdx-mpStartIdx, tempMP,
      optInTeethPeriod,
      &tempBegIdx, &tempNbElement, outTeeth );

   if( (retCode != TA_SUCCESS) || (tempNbElement == 0) )
   {
      free( tempMP );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = rma( lookbackTotal, endIdx-mpStartIdx, tempMP,
      optInLipsPeriod,
      &tempBegIdx, &tempNbElement, outLips );

   if( (retCode != TA_SUCCESS) || (tempNbElement == 0) )
   {
      free( tempMP );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   free( tempMP );

   *outBegIdx = startIdx;
   *outNBElement = tempNbElement;

   return TA_SUCCESS;
}
