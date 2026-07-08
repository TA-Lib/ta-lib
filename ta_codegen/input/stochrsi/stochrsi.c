/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  PP       Peter Pudaite
 *  AA       Andrew Atkinson
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  101103 PP   Initial creation of code.
 *  112603 MF   Add independent control to the RSI period.
 *  020605 AA   Fix #1117656. NULL pointer assignement.
 */

int stochrsi_lookback(int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, TA_MAType optInFastD_MAType)
{
   int retValue;

   retValue = rsi_lookback( optInTimePeriod ) + stochf_lookback( optInFastK_Period, optInFastD_Period, optInFastD_MAType );

   return retValue;
}

TA_RetCode stochrsi(int startIdx, int endIdx, const double inReal[], int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, TA_MAType optInFastD_MAType, int *outBegIdx, int *outNBElement, double outFastK[], double outFastD[])
{
   double *tempRSIBuffer;

   TA_RetCode retCode;
   int lookbackTotal, lookbackSTOCHF, tempArraySize;
   int outBegIdx1;
   int outBegIdx2;
   int outNbElement1;

   /* Stochastic RSI
    *
    * Reference: "Stochastic RSI and Dynamic Momentum Index"
    *            by Tushar Chande and Stanley Kroll
    *            Stock&Commodities V.11:5 (189-199)
    *
    * The TA-Lib version offer flexibility beyond what is explain
    * in the Stock&Commodities article.
    *
    * To calculate the "Unsmoothed stochastic RSI" with symetry like
    * explain in the article, keep the optInTimePeriod and optInFastK_Period
    * equal. Example:
    *
    *    unsmoothed stoch RSI 14 : optInTimePeriod   = 14
    *                              optInFastK_Period = 14
    *                              optInFastD_Period = 'x'
    *
    * The outFastK is the unsmoothed RSI discuss in the article.
    *
    * You can set the optInFastD_Period to smooth the RSI. The smooth
    * version will be found in outFastD. The outFastK will still contain
    * the unsmoothed stoch RSI. If you do not care about the smoothing of
    * the StochRSI, just leave optInFastD_Period to 1 and ignore outFastD.
    */

   *outBegIdx = 0;
   *outNBElement = 0;

   /* Adjust startIdx to account for the lookback period. */
   lookbackSTOCHF = stochf_lookback( optInFastK_Period, optInFastD_Period, optInFastD_MAType );
   lookbackTotal = rsi_lookback( optInTimePeriod ) + lookbackSTOCHF;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx = startIdx;

   tempArraySize = (endIdx - startIdx) + 1 + lookbackSTOCHF;

   double *tempRSIBuffer = malloc((tempArraySize) * sizeof(double));

   retCode = rsi(startIdx-lookbackSTOCHF,
      endIdx,
      inReal,
      optInTimePeriod,
      &outBegIdx1,
      &outNbElement1,
      tempRSIBuffer);

   if( retCode != TA_SUCCESS || outNbElement1 == 0 )
   {
      free(tempRSIBuffer);
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = stochf(0,
      tempArraySize-1,
      tempRSIBuffer,
      tempRSIBuffer,
      tempRSIBuffer,
      optInFastK_Period,
      optInFastD_Period,
      optInFastD_MAType,
      &outBegIdx2,
      outNBElement,
      outFastK,
      outFastD);

   free(tempRSIBuffer);

   if( retCode != TA_SUCCESS || ((int)*outNBElement) == 0 )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   return TA_SUCCESS;
}
