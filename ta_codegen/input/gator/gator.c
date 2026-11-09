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
 *  110726 MF,CC  First version (issue #583).
 */

int gator_display_shift(int optInJawPeriod, int optInJawShift,
   int optInTeethPeriod, int optInTeethShift,
   int optInLipsPeriod, int optInLipsShift,
   int outputIdx)
{
   if( outputIdx == 0 )
      return optInJawShift < optInTeethShift ? optInJawShift : optInTeethShift;
   return optInTeethShift < optInLipsShift ? optInTeethShift : optInLipsShift;
}

int gator_lookback(int optInJawPeriod, int optInJawShift,
   int optInTeethPeriod, int optInTeethShift,
   int optInLipsPeriod, int optInLipsShift)
{
   int backJaw, backTeethUpper, backTeethLower, backLips;
   int lookback, legLookback;

   /* Each histogram pairs its two lines as a chart draws them: the line drawn
    * further ahead is read back by the difference of the two shifts. Each term
    * is that leg's rma_lookback plus its read-back, so every leg inherits
    * TA_FUNC_UNST_RMA.
    */
   backJaw = 0;
   backTeethUpper = 0;
   if( optInJawShift > optInTeethShift )
      backJaw = optInJawShift - optInTeethShift;
   else
      backTeethUpper = optInTeethShift - optInJawShift;

   backTeethLower = 0;
   backLips = 0;
   if( optInTeethShift > optInLipsShift )
      backTeethLower = optInTeethShift - optInLipsShift;
   else
      backLips = optInLipsShift - optInTeethShift;

   lookback = rma_lookback( optInJawPeriod ) + backJaw;

   legLookback = rma_lookback( optInTeethPeriod ) + backTeethUpper;
   if( legLookback > lookback )
      lookback = legLookback;

   legLookback = rma_lookback( optInTeethPeriod ) + backTeethLower;
   if( legLookback > lookback )
      lookback = legLookback;

   legLookback = rma_lookback( optInLipsPeriod ) + backLips;
   if( legLookback > lookback )
      lookback = legLookback;

   return lookback;
}

TA_RetCode gator(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInJawPeriod,
   int optInJawShift,
   int optInTeethPeriod,
   int optInTeethShift,
   int optInLipsPeriod,
   int optInLipsShift,
   int *outBegIdx, int *outNBElement,
   double outUpper[],
   double outLower[])
{
   TA_RetCode retCode;
   int i, nbElement, maxBack;
   int lookbackTotal;
   int mpStartIdx, localStart, localEnd;
   int backJaw, backTeethUpper, backTeethLower, backLips;
   int tempBegIdx, tempNbElement;
   double *tempMP;
   double *tempA;
   double *tempB;

   lookbackTotal = gator_lookback( optInJawPeriod, optInJawShift,
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

   backJaw = 0;
   backTeethUpper = 0;
   if( optInJawShift > optInTeethShift )
      backJaw = optInJawShift - optInTeethShift;
   else
      backTeethUpper = optInTeethShift - optInJawShift;

   backTeethLower = 0;
   backLips = 0;
   if( optInTeethShift > optInLipsShift )
      backTeethLower = optInTeethShift - optInLipsShift;
   else
      backLips = optInLipsShift - optInTeethShift;

   maxBack = backJaw + backTeethUpper;
   if( backTeethLower + backLips > maxBack )
      maxBack = backTeethLower + backLips;

   mpStartIdx = startIdx - lookbackTotal;
   localStart = lookbackTotal;
   localEnd = endIdx - mpStartIdx;
   nbElement = endIdx - startIdx + 1;

   tempMP = malloc((endIdx-mpStartIdx+1) * sizeof(double));
   if( !tempMP )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempA = malloc((nbElement+maxBack) * sizeof(double));
   if( !tempA )
   {
      free( tempMP );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempB = malloc((nbElement+maxBack) * sizeof(double));
   if( !tempB )
   {
      free( tempMP );
      free( tempA );
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
      free( tempA );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Each leg is its own rma call starting at its read-back bar and seeded at
    * its own lookback before that, so neither histogram moves when the line it
    * does not read changes; the teeth is therefore computed once per
    * histogram. A leg read back by `back` bars starts that many bars early, so
    * element i of its buffer is the bar the output's element i pairs it with.
    */
   retCode = rma( localStart-backJaw, localEnd, tempMP,
      optInJawPeriod,
      &tempBegIdx, &tempNbElement, tempA );

   if( retCode != TA_SUCCESS )
   {
      free( tempMP );
      free( tempA );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = rma( localStart-backTeethUpper, localEnd, tempMP,
      optInTeethPeriod,
      &tempBegIdx, &tempNbElement, tempB );

   if( retCode != TA_SUCCESS )
   {
      free( tempMP );
      free( tempA );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   for( i=0; i < nbElement; i++ )
      outUpper[i] = fabs( tempA[i] - tempB[i] );

   retCode = rma( localStart-backTeethLower, localEnd, tempMP,
      optInTeethPeriod,
      &tempBegIdx, &tempNbElement, tempA );

   if( retCode != TA_SUCCESS )
   {
      free( tempMP );
      free( tempA );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = rma( localStart-backLips, localEnd, tempMP,
      optInLipsPeriod,
      &tempBegIdx, &tempNbElement, tempB );

   if( retCode != TA_SUCCESS )
   {
      free( tempMP );
      free( tempA );
      free( tempB );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* A zero spread must come out +0.0. Keep the select: gcc folds the literal
    * 0.0 - fabs(x) into -fabs(x), which is -0.0 there.
    */
   for( i=0; i < nbElement; i++ )
      outLower[i] = tempA[i] < tempB[i] ? tempA[i] - tempB[i] : tempB[i] - tempA[i];

   free( tempMP );
   free( tempA );
   free( tempB );

   *outBegIdx = startIdx;
   *outNBElement = nbElement;

   return TA_SUCCESS;
}
