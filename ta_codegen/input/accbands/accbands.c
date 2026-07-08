/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  RM       Robert Meier
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120307 RM     Initial Version
 *  120907 MF     Handling of a few limit cases
 */

int accbands_lookback(int optInTimePeriod)
{
   return sma_lookback( optInTimePeriod );
}

TA_RetCode accbands(int startIdx, int endIdx, const double inHigh[], const double inLow[], const double inClose[], int optInTimePeriod, int *outBegIdx, int *outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[])
{
   TA_RetCode retCode;
   double *tempBuffer1;
   double *tempBuffer2;
   int outBegIdxDummy;
   int outNbElementDummy;
   int i, j, outputSize, bufferSize, lookbackTotal;
   double tempReal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = sma_lookback( optInTimePeriod );

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Buffer will contains also the lookback required for SMA
    * to satisfy the caller requested startIdx/endIdx.
    */
   outputSize = endIdx-startIdx+1;
   bufferSize = outputSize+lookbackTotal;
   double *tempBuffer1 = malloc((bufferSize) * sizeof(double));
   if( !tempBuffer1 )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   double *tempBuffer2 = malloc((bufferSize) * sizeof(double));
   if( !tempBuffer2 )
   {
      free(tempBuffer1);
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   /* Calculate the upper/lower band at the same time (no SMA yet).
    * Must start calculation back enough to cover the lookback
    * required later for the SMA.
    */
   for(j=0, i=startIdx-lookbackTotal; i<=endIdx; i++, j++)
   {
      tempReal = inHigh[i]+inLow[i];
      if( !TA_IS_ZERO(tempReal) )
      {
         tempReal = 4*(inHigh[i]-inLow[i])/tempReal;
         tempBuffer1[j] = inHigh[i]*(1+tempReal);
         tempBuffer2[j] = inLow[i]*(1-tempReal);
      }
      else
      {
         tempBuffer1[j] = inHigh[i];
         tempBuffer2[j] = inLow[i];
      }
   }

   /* Calculate the middle band, which is a moving average of the close. */
   retCode = sma( startIdx, endIdx, inClose,
      optInTimePeriod,
      &outBegIdxDummy, &outNbElementDummy, outRealMiddleBand );

   if( (retCode != TA_SUCCESS ) || ((int)outNbElementDummy != outputSize) )
   {
      free(tempBuffer1);
      free(tempBuffer2);
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Now let's take the SMA for the upper band. */
   retCode = sma( 0, bufferSize-1, tempBuffer1,
      optInTimePeriod,
      &outBegIdxDummy, &outNbElementDummy,
      outRealUpperBand );

   if( (retCode != TA_SUCCESS ) || ((int)outNbElementDummy != outputSize) )
   {
      free(tempBuffer1);
      free(tempBuffer2);
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Now let's take the SMA for the lower band. */
   retCode = sma( 0, bufferSize-1, tempBuffer2,
      optInTimePeriod,
      &outBegIdxDummy, &outNbElementDummy,
      outRealLowerBand );

   free(tempBuffer1);
   free(tempBuffer2);

   if( (retCode != TA_SUCCESS ) || ((int)outNbElementDummy != outputSize) )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outputSize;

   return TA_SUCCESS;
}
