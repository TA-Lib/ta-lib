/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  022203 MF   Add MAMA
 *  040503 MF   Add T3
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  111603 MF   Allow period of 1. Just copy input into output.
 *  060907 MF   Use TA_SMA/TA_EMA instead of internal implementation.
 */

int ma_lookback(int optInTimePeriod, TA_MAType optInMAType)
{
   int retValue;

   if( optInTimePeriod <= 1 )
      return 0;

   switch( optInMAType )
   {
      case TA_MAType_SMA:
         retValue = sma_lookback( optInTimePeriod );
         break;

      case TA_MAType_EMA:
         retValue = ema_lookback( optInTimePeriod );
         break;

      case TA_MAType_WMA:
         retValue = wma_lookback( optInTimePeriod );
         break;

      case TA_MAType_DEMA:
         retValue = dema_lookback( optInTimePeriod );
         break;

      case TA_MAType_TEMA:
         retValue = tema_lookback( optInTimePeriod );
         break;

      case TA_MAType_TRIMA:
         retValue = trima_lookback( optInTimePeriod );
         break;

      case TA_MAType_KAMA:
         retValue = kama_lookback( optInTimePeriod );
         break;

      case TA_MAType_MAMA:
         retValue = mama_lookback( 0.5, 0.05 );
         break;

      case TA_MAType_T3:
         retValue = t3_lookback( optInTimePeriod, 0.7 );
         break;

      default:
         retValue = 0;
   }

   return retValue;
}

TA_RetCode ma(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   TA_RetCode retCode;

   int nbElement;
   int outIdx, todayIdx;

   if( optInTimePeriod == 1 )
   {
      nbElement = endIdx-startIdx+1;
      *outNBElement = nbElement;
      for( todayIdx=startIdx, outIdx=0; outIdx < nbElement; outIdx++, todayIdx++ )
         outReal[outIdx] = inReal[todayIdx];
      *outBegIdx    = startIdx;
      return TA_SUCCESS;
   }
   /* Simply forward the job to the corresponding TA function. */
   switch( optInMAType )
   {
      case TA_MAType_SMA:
         retCode = sma( startIdx, endIdx, inReal, optInTimePeriod,
            outBegIdx, outNBElement, outReal );
         break;

      case TA_MAType_EMA:
         retCode = ema( startIdx, endIdx, inReal, optInTimePeriod,
            outBegIdx, outNBElement, outReal );
         break;

      case TA_MAType_WMA:
         retCode = wma( startIdx, endIdx, inReal, optInTimePeriod,
            outBegIdx, outNBElement, outReal );
         break;

      case TA_MAType_DEMA:
         retCode = dema( startIdx, endIdx, inReal, optInTimePeriod,
            outBegIdx, outNBElement, outReal );
         break;

      case TA_MAType_TEMA:
         retCode = tema( startIdx, endIdx, inReal, optInTimePeriod,
            outBegIdx, outNBElement, outReal );
         break;

      case TA_MAType_TRIMA:
         retCode = trima( startIdx, endIdx, inReal, optInTimePeriod,
            outBegIdx, outNBElement, outReal );
         break;

      case TA_MAType_KAMA:
         retCode = kama( startIdx, endIdx, inReal, optInTimePeriod,
            outBegIdx, outNBElement, outReal );
         break;

      case TA_MAType_MAMA:
         /* The optInTimePeriod is ignored. FAMA is a nullable output
          * (issue #125): pass NULL to compute only the MAMA line into outReal.
          */
         retCode = mama( startIdx, endIdx, inReal, 0.5, 0.05,
            outBegIdx, outNBElement,
            outReal, NULL );
         break;

      case TA_MAType_T3:
         retCode = t3( startIdx, endIdx, inReal,
            optInTimePeriod, 0.7,
            outBegIdx, outNBElement, outReal );
         break;

      default:
         retCode = TA_BAD_PARAM;
         break;
   }

   return retCode;
}
