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
 *  112126 MF,CC  First version (issue #610).
 */

int maenv_lookback(int optInTimePeriod, double optInPercentUp, double optInPercentDn, TA_MAType optInMAType)
{
   (void)optInPercentUp;
   (void)optInPercentDn;

   return ma_lookback( optInTimePeriod, optInMAType );
}

TA_RetCode maenv(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInPercentUp,
   double optInPercentDn,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outRealUpperBand[],
   double outRealMiddleBand[],
   double outRealLowerBand[])
{
   TA_RetCode retCode;
   int i;
   double upFactor, dnFactor, middle;

   if( ma_lookback( optInTimePeriod, optInMAType ) > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   retCode = ma( startIdx, endIdx, inReal,
      optInTimePeriod, optInMAType,
      outBegIdx, outNBElement, outRealMiddleBand );

   if( retCode != TA_SUCCESS )
   {
      *outNBElement = 0;
      return retCode;
   }

   upFactor = 1.0 + optInPercentUp / 100.0;
   dnFactor = 1.0 - optInPercentDn / 100.0;

   for( i=0; i < (int)*outNBElement; i++ )
   {
      middle = outRealMiddleBand[i];
      outRealUpperBand[i] = middle * upFactor;
      outRealLowerBand[i] = middle * dnFactor;
   }

   return TA_SUCCESS;
}
