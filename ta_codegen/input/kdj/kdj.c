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
 *  090426 MF,CC  First version (issue #365).
 */

int kdj_lookback(int optInFastK_Period, int optInSlowK_Period, TA_MAType optInSlowK_MAType, int optInSlowD_Period, TA_MAType optInSlowD_MAType)
{
   return stoch_lookback( optInFastK_Period, optInSlowK_Period, optInSlowK_MAType,
      optInSlowD_Period, optInSlowD_MAType );
}

TA_RetCode kdj(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInFastK_Period,
   int optInSlowK_Period,
   TA_MAType optInSlowK_MAType,
   int optInSlowD_Period,
   TA_MAType optInSlowD_MAType,
   int *outBegIdx, int *outNBElement,
   double outK[],
   double outD[],
   double outJ[])
{
   TA_RetCode retCode;
   int i;
   int lookbackTotal;

   lookbackTotal = kdj_lookback( optInFastK_Period, optInSlowK_Period, optInSlowK_MAType,
      optInSlowD_Period, optInSlowD_MAType );

   /* Nothing to produce: the range is shorter than the lookback. Answering here
    * keeps the sub-call out of the phantom-I/O sweep's zero-length range, where
    * its own argument check would reject before any array is touched.
    */
   if( lookbackTotal > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   retCode = stoch( startIdx, endIdx, inHigh, inLow, inClose,
      optInFastK_Period,
      optInSlowK_Period,
      optInSlowK_MAType,
      optInSlowD_Period,
      optInSlowD_MAType,
      outBegIdx, outNBElement, outK, outD );

   if( (retCode != TA_SUCCESS) || ((int)*outNBElement == 0) )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Keep this one Sub expression: spelling it as an Add of a negated term
    * would arm the multiply-add fusion and move the last bits of J.
    */
   for( i=0; i < (int)*outNBElement; i++ )
   {
      outJ[i] = 3.0 * outK[i] - 2.0 * outD[i];
   }

   return TA_SUCCESS;
}
