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
 *  100126 MF,CC  First version (#491).
 *  101126 MF,CC  Each lookback is asked once per call (#522).
 */

int kstext_lookback(int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInMA1Period, int optInMA2Period, int optInMA3Period, int optInMA4Period, int optInSignalPeriod, TA_MAType optInROCMAType, TA_MAType optInSignalMAType)
{
   int legMax, leg;

   legMax = optInROC1Period + ma_lookback( optInMA1Period, optInROCMAType );
   leg = optInROC2Period + ma_lookback( optInMA2Period, optInROCMAType );
   if( leg > legMax ) legMax = leg;
   leg = optInROC3Period + ma_lookback( optInMA3Period, optInROCMAType );
   if( leg > legMax ) legMax = leg;
   leg = optInROC4Period + ma_lookback( optInMA4Period, optInROCMAType );
   if( leg > legMax ) legMax = leg;
   return legMax + ma_lookback( optInSignalPeriod, optInSignalMAType );
}

TA_RetCode kstext(int startIdx, int endIdx,
   const double inReal[],
   int optInROC1Period,
   int optInROC2Period,
   int optInROC3Period,
   int optInROC4Period,
   int optInMA1Period,
   int optInMA2Period,
   int optInMA3Period,
   int optInMA4Period,
   int optInSignalPeriod,
   TA_MAType optInROCMAType,
   TA_MAType optInSignalMAType,
   int *outBegIdx,
   int *outNBElement,
   double outKST[],
   double outKSTSignal[])
{
   double *kstBuffer;
   double *tempBuffer;
   TA_RetCode retCode;
   int lookbackTotal, lookbackSignal, lookbackMA, sigStart;
   int lookbackMA1, lookbackMA2, lookbackMA3, lookbackMA4;
   int tempInteger, tempBegIdx, rocNb, kstNb, legNb, sigNb;
   int i;

   /* With every type SMA this is bit-exact with kst(): each leg's average
    * starts on the first bar the signal consumes, and the line accumulates
    * its legs left to right. Changing either breaks the equality.
    */

   *outBegIdx = 0;
   *outNBElement = 0;

   /* lookbackTotal must stay kstext_lookback() of the same parameters. */
   lookbackSignal = ma_lookback( optInSignalPeriod, optInSignalMAType );
   lookbackMA1 = ma_lookback( optInMA1Period, optInROCMAType );
   lookbackMA2 = ma_lookback( optInMA2Period, optInROCMAType );
   lookbackMA3 = ma_lookback( optInMA3Period, optInROCMAType );
   lookbackMA4 = ma_lookback( optInMA4Period, optInROCMAType );

   lookbackTotal = optInROC1Period + lookbackMA1;
   tempInteger = optInROC2Period + lookbackMA2;
   if( tempInteger > lookbackTotal ) lookbackTotal = tempInteger;
   tempInteger = optInROC3Period + lookbackMA3;
   if( tempInteger > lookbackTotal ) lookbackTotal = tempInteger;
   tempInteger = optInROC4Period + lookbackMA4;
   if( tempInteger > lookbackTotal ) lookbackTotal = tempInteger;
   lookbackTotal += lookbackSignal;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   sigStart = startIdx - lookbackSignal;

   /* Both buffers are sized by the longest leg average: a rate of change
    * starts its average's lookback before sigStart.
    */
   lookbackMA = lookbackMA1;
   if( lookbackMA2 > lookbackMA ) lookbackMA = lookbackMA2;
   if( lookbackMA3 > lookbackMA ) lookbackMA = lookbackMA3;
   if( lookbackMA4 > lookbackMA ) lookbackMA = lookbackMA4;

   tempInteger = (endIdx-sigStart)+1+lookbackMA;
   kstBuffer = malloc((tempInteger) * sizeof(double));
   if( !kstBuffer )
      return TA_ALLOC_ERR;

   tempBuffer = malloc((tempInteger) * sizeof(double));
   if( !tempBuffer )
   {
      free( kstBuffer );
      return TA_ALLOC_ERR;
   }

   retCode = roc( sigStart-lookbackMA1, endIdx, inReal, optInROC1Period,
      &tempBegIdx, &rocNb, kstBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( kstBuffer );
      free( tempBuffer );
      return retCode;
   }

   retCode = ma( 0, rocNb-1, kstBuffer, optInMA1Period, optInROCMAType,
      &tempBegIdx, &kstNb, kstBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( kstBuffer );
      free( tempBuffer );
      return retCode;
   }

   retCode = roc( sigStart-lookbackMA2, endIdx, inReal, optInROC2Period,
      &tempBegIdx, &rocNb, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( kstBuffer );
      free( tempBuffer );
      return retCode;
   }

   retCode = ma( 0, rocNb-1, tempBuffer, optInMA2Period, optInROCMAType,
      &tempBegIdx, &legNb, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( kstBuffer );
      free( tempBuffer );
      return retCode;
   }

   for( i=0; i < kstNb; i++ )
      kstBuffer[i] = kstBuffer[i] + 2.0*tempBuffer[i];

   retCode = roc( sigStart-lookbackMA3, endIdx, inReal, optInROC3Period,
      &tempBegIdx, &rocNb, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( kstBuffer );
      free( tempBuffer );
      return retCode;
   }

   retCode = ma( 0, rocNb-1, tempBuffer, optInMA3Period, optInROCMAType,
      &tempBegIdx, &legNb, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( kstBuffer );
      free( tempBuffer );
      return retCode;
   }

   for( i=0; i < kstNb; i++ )
      kstBuffer[i] = kstBuffer[i] + 3.0*tempBuffer[i];

   retCode = roc( sigStart-lookbackMA4, endIdx, inReal, optInROC4Period,
      &tempBegIdx, &rocNb, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( kstBuffer );
      free( tempBuffer );
      return retCode;
   }

   retCode = ma( 0, rocNb-1, tempBuffer, optInMA4Period, optInROCMAType,
      &tempBegIdx, &legNb, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( kstBuffer );
      free( tempBuffer );
      return retCode;
   }

   for( i=0; i < kstNb; i++ )
      kstBuffer[i] = kstBuffer[i] + 4.0*tempBuffer[i];

   /* Every read of inReal is done: an output may alias it. */
   memmove(outKST, &kstBuffer[lookbackSignal], ((endIdx-startIdx)+1) * sizeof(double));

   retCode = ma( 0, kstNb-1, kstBuffer, optInSignalPeriod, optInSignalMAType,
      &tempBegIdx, &sigNb, outKSTSignal );

   free( kstBuffer );
   free( tempBuffer );

   if( retCode != TA_SUCCESS )
      return retCode;

   *outBegIdx    = startIdx;
   *outNBElement = sigNb;

   return TA_SUCCESS;
}
