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
 *  072226 MF,CC Add HMA (issue #139).
 *  072426 MF,CC TA_MAType_DISABLED: period-independent identity copy (issue #93).
 *  090426 MF,CC Add ZLEMA (issue #347).
 *  090426 MF,CC Add RMA (issue #348).
 */

int ma_lookback(int optInTimePeriod, TA_MAType optInMAType)
{
   int retValue;

   if( optInTimePeriod <= 1 || optInMAType == TA_MAType_DISABLED )
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

      case TA_MAType_HMA:
         retValue = hma_lookback( optInTimePeriod );
         break;

      case TA_MAType_ZLEMA:
         retValue = zlema_lookback( optInTimePeriod );
         break;

      case TA_MAType_RMA:
         retValue = rma_lookback( optInTimePeriod );
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

   /* Nothing to produce: the range is shorter than the lookback. Answer here
    * rather than forwarding.
    *
    * The VALUE is the same either way: ma_lookback returns exactly the lookback
    * the arm's callee computes for itself, from the same arguments the arm
    * passes it, and every callee clamps startIdx to that lookback and yields
    * 0,0 without reading. What changes is whose frame answers, and that is
    * visible to one caller only - the zero-length no-I/O probe. It hands the
    * body empty arrays on this range to check that nothing is read; forwarding
    * makes the callee's PUBLIC input bound (endIdx + 1 elements, no
    * sub-lookback escape) answer TA_BAD_PARAM before any array is reached, so
    * the probe cannot tell "read nothing" from "never ran". ma was the last
    * core withheld from that sweep for exactly this reason; apo, bbands, ppo,
    * pvo and stddev already carried this guard. No legitimate caller is
    * affected: ma's own public tier already requires endIdx + 1 input elements,
    * and on this range the callee's OUTPUT bound is 0, so a caller sizing by
    * the published formula was never rejected by it.
    *
    * It cannot mask a TA_BAD_PARAM. An optInMAType outside the enum is refused
    * by the generated entry point above this guard. An in-range member with no
    * arm here would reach ma_lookback's own default and get 0, and 0 > endIdx
    * is false for every endIdx the entry point admits, so control still reaches
    * the switch and still answers TA_BAD_PARAM. The identity path below is out
    * of the guard's reach for the same reason - its lookback is 0.
    */
   if( ma_lookback( optInTimePeriod, optInMAType ) > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* No-smoothing identity: period 1 (every MA type) or the explicit
    * TA_MAType_DISABLED (any period, issue #93). One copy path, lookback 0. */
   if( optInTimePeriod == 1 || optInMAType == TA_MAType_DISABLED )
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

      case TA_MAType_HMA:
         retCode = hma( startIdx, endIdx, inReal, optInTimePeriod,
            outBegIdx, outNBElement, outReal );
         break;

      case TA_MAType_ZLEMA:
         retCode = zlema( startIdx, endIdx, inReal, optInTimePeriod,
            outBegIdx, outNBElement, outReal );
         break;

      case TA_MAType_RMA:
         retCode = rma( startIdx, endIdx, inReal, optInTimePeriod,
            outBegIdx, outNBElement, outReal );
         break;

      default:
         retCode = TA_BAD_PARAM;
         break;
   }

   return retCode;
}
