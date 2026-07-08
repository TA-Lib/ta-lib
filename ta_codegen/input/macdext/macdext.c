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
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070526 MF,CC  Speed optimization: delegate to the single-pass MACD
 *                when all three MA types are EMA (bit-exact).
 *
 */

int macdext_lookback(int optInFastPeriod, TA_MAType optInFastMAType, int optInSlowPeriod, TA_MAType optInSlowMAType, int optInSignalPeriod, TA_MAType optInSignalMAType)
{
   int tempInteger;
   int lookbackLargest;

   /* Find the MA with the largest lookback */
   lookbackLargest = ma_lookback( optInFastPeriod, optInFastMAType );
   tempInteger = ma_lookback( optInSlowPeriod, optInSlowMAType );
   if( tempInteger > lookbackLargest )
      lookbackLargest = tempInteger;

   /* Add to the largest MA lookback the signal line lookback */
   return lookbackLargest + ma_lookback( optInSignalPeriod, optInSignalMAType );
}

TA_RetCode macdext(int startIdx, int endIdx, const double inReal[], int optInFastPeriod, TA_MAType optInFastMAType, int optInSlowPeriod, TA_MAType optInSlowMAType, int optInSignalPeriod, TA_MAType optInSignalMAType, int *outBegIdx, int *outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[])
{
   double *slowMABuffer;
   double *fastMABuffer;
   TA_RetCode retCode;
   int tempInteger;
   int outBegIdx1;
   int outNbElement1;
   int outBegIdx2;
   int outNbElement2;
   int lookbackTotal, lookbackSignal, lookbackLargest;
   int i;
   ENUM_DECLARATION(MAType) tempMAType;

   /* An all-EMA MACDEXT computes exactly what MACD computes. Delegate
    * to its single-pass implementation. Period 1 stays on the generic
    * path: ma() copies the input for it instead of running an EMA
    * recursion.
    */
   if( ( optInFastMAType   == TA_MAType_EMA ) &&
      ( optInSlowMAType   == TA_MAType_EMA ) &&
      ( optInSignalMAType == TA_MAType_EMA ) &&
      ( optInFastPeriod   >= 2 ) &&
      ( optInSlowPeriod   >= 2 ) &&
      ( optInSignalPeriod >= 2 ) )
   {
      return macd( startIdx, endIdx, inReal,
         optInFastPeriod, optInSlowPeriod, optInSignalPeriod,
         outBegIdx, outNBElement,
         outMACD, outMACDSignal, outMACDHist );
   }

   /* Make sure slow is really slower than
    * the fast period! if not, swap...
    */
   if( optInSlowPeriod < optInFastPeriod )
   {
      /* swap period */
      tempInteger     = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
      /* swap type */
      tempMAType      = optInSlowMAType;
      optInSlowMAType = optInFastMAType;
      optInFastMAType = tempMAType;
   }

   /* Find the MA with the largest lookback */
   lookbackLargest = ma_lookback( optInFastPeriod, optInFastMAType );
   tempInteger = ma_lookback( optInSlowPeriod, optInSlowMAType );
   if( tempInteger > lookbackLargest )
      lookbackLargest = tempInteger;

   /* Add the lookback needed for the signal line */
   lookbackSignal = ma_lookback( optInSignalPeriod, optInSignalMAType );
   lookbackTotal  = lookbackSignal+lookbackLargest;

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

   /* Allocate intermediate buffer for fast/slow MA. */
   tempInteger = (endIdx-startIdx)+1+lookbackSignal;
   fastMABuffer = malloc((tempInteger) * sizeof(double));
   if( !fastMABuffer )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   slowMABuffer = malloc((tempInteger) * sizeof(double));
   if( !slowMABuffer )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      free(fastMABuffer);
      return TA_ALLOC_ERR;
   }

   /* Calculate the slow MA.
    *
    * Move back the startIdx to get enough data
    * for the signal period. That way, once the
    * signal calculation is done, all the output
    * will start at the requested 'startIdx'.
    */
   tempInteger = startIdx-lookbackSignal;
   retCode = ma( tempInteger, endIdx,
      inReal, optInSlowPeriod, optInSlowMAType,
      &outBegIdx1, &outNbElement1,
      slowMABuffer );

   if( retCode != TA_SUCCESS )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      free(fastMABuffer);
      free(slowMABuffer);
      return retCode;
   }

   /* Calculate the fast MA. */
   retCode = ma( tempInteger, endIdx,
      inReal, optInFastPeriod, optInFastMAType,
      &outBegIdx2, &outNbElement2,
      fastMABuffer );

   if( retCode != TA_SUCCESS )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      free(fastMABuffer);
      free(slowMABuffer);
      return retCode;
   }

   /* Parano tests. Will be removed eventually. */
   if( (outBegIdx1 != tempInteger) ||
      (outBegIdx2 != tempInteger) ||
      (outNbElement1 != outNbElement2) ||
      (outNbElement1 != (endIdx-startIdx)+1+lookbackSignal) )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      free(fastMABuffer);
      free(slowMABuffer);
      return TA_BAD_PARAM;
   }

   /* Calculate (fast MA) - (slow MA). */
   for( i=0; i < outNbElement1; i++ )
      fastMABuffer[i] = fastMABuffer[i] - slowMABuffer[i];

   /* Copy the result into the output for the caller. */
   /* memmove, not memcpy: fastMABuffer aliases outMACD when the caller buffer is
    * reused as scratch, so source and destination overlap (issue #94). */
   memmove(outMACD, &fastMABuffer[lookbackSignal], ((endIdx-startIdx)+1) * sizeof(double));

   /* Calculate the signal/trigger line. */
   retCode = ma( 0, outNbElement1-1,
      fastMABuffer, optInSignalPeriod, optInSignalMAType,
      &outBegIdx2, &outNbElement2, outMACDSignal );

   free(fastMABuffer);
   free(slowMABuffer);

   if( retCode != TA_SUCCESS )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Calculate the histogram. */
   for( i=0; i < outNbElement2; i++ )
      outMACDHist[i] = outMACD[i]-outMACDSignal[i];

   /* All done! Indicate the output limits and return success. */
   *outBegIdx     = startIdx;
   *outNBElement  = outNbElement2;

   return TA_SUCCESS;
}
