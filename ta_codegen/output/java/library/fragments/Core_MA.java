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

   public int movingAverageLookback( int optInTimePeriod, MAType optInMAType )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int retValue;
      if( optInTimePeriod <= 1 ) {
         return 0 ;
      }
      switch( optInMAType )
      {
      case Sma:
         retValue = smaLookback(optInTimePeriod);
         break;
      case Ema:
         retValue = emaLookback(optInTimePeriod);
         break;
      case Wma:
         retValue = wmaLookback(optInTimePeriod);
         break;
      case Dema:
         retValue = demaLookback(optInTimePeriod);
         break;
      case Tema:
         retValue = temaLookback(optInTimePeriod);
         break;
      case Trima:
         retValue = trimaLookback(optInTimePeriod);
         break;
      case Kama:
         retValue = kamaLookback(optInTimePeriod);
         break;
      case Mama:
         retValue = mamaLookback(0.5, 0.05);
         break;
      case T3:
         retValue = t3Lookback(optInTimePeriod, 0.7);
         break;
      default:
         retValue = 0;
         break;
      }
      return retValue ;

   }
   public RetCode movingAverage( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 MAType optInMAType,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double[] dummyBuffer;
      RetCode retCode;
      int nbElement = 0;
      int outIdx = 0;
      int todayIdx = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         nbElement = endIdx - startIdx + 1;
         outNBElement.value = nbElement;
         for( todayIdx = startIdx, outIdx = 0; outIdx < nbElement; outIdx += 1, todayIdx += 1 ) {
            outReal[outIdx] = inReal[todayIdx];
         }
         outBegIdx.value = startIdx;
         return RetCode.Success ;
      }
      /* Simply forward the job to the corresponding TA function. */
      switch( optInMAType )
      {
      case Sma:
         retCode = smaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         /* The optInTimePeriod is ignored and the FAMA output of the MAMA
          * is ignored.
          */
         dummyBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
         retCode = mamaUnguarded(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, dummyBuffer);
         break;
      case T3:
         retCode = t3Unguarded(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   public RetCode movingAverageUnguarded( int startIdx,
                                          int endIdx,
                                          double inReal[],
                                          int optInTimePeriod,
                                          MAType optInMAType,
                                          MInteger outBegIdx,
                                          MInteger outNBElement,
                                          double outReal[] )
   {
      double[] dummyBuffer;
      RetCode retCode;
      int nbElement = 0;
      int outIdx = 0;
      int todayIdx = 0;
      if( optInTimePeriod == 1 ) {
         nbElement = endIdx - startIdx + 1;
         outNBElement.value = nbElement;
         for( todayIdx = startIdx, outIdx = 0; outIdx < nbElement; outIdx += 1, todayIdx += 1 ) {
            outReal[outIdx] = inReal[todayIdx];
         }
         outBegIdx.value = startIdx;
         return RetCode.Success ;
      }
      switch( optInMAType )
      {
      case Sma:
         retCode = smaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         dummyBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
         retCode = mamaUnguarded(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, dummyBuffer);
         break;
      case T3:
         retCode = t3Unguarded(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   public RetCode movingAverage( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 MAType optInMAType,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double[] dummyBuffer;
      RetCode retCode;
      int nbElement = 0;
      int outIdx = 0;
      int todayIdx = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         nbElement = endIdx - startIdx + 1;
         outNBElement.value = nbElement;
         for( todayIdx = startIdx, outIdx = 0; outIdx < nbElement; outIdx += 1, todayIdx += 1 ) {
            outReal[outIdx] = (double)inReal[todayIdx];
         }
         outBegIdx.value = startIdx;
         return RetCode.Success ;
      }
      switch( optInMAType )
      {
      case Sma:
         retCode = smaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         dummyBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
         retCode = mamaUnguarded(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, dummyBuffer);
         break;
      case T3:
         retCode = t3Unguarded(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   public RetCode movingAverageUnguarded( int startIdx,
                                          int endIdx,
                                          float inReal[],
                                          int optInTimePeriod,
                                          MAType optInMAType,
                                          MInteger outBegIdx,
                                          MInteger outNBElement,
                                          double outReal[] )
   {
      double[] dummyBuffer;
      RetCode retCode;
      int nbElement = 0;
      int outIdx = 0;
      int todayIdx = 0;
      if( optInTimePeriod == 1 ) {
         nbElement = endIdx - startIdx + 1;
         outNBElement.value = nbElement;
         for( todayIdx = startIdx, outIdx = 0; outIdx < nbElement; outIdx += 1, todayIdx += 1 ) {
            outReal[outIdx] = (double)inReal[todayIdx];
         }
         outBegIdx.value = startIdx;
         return RetCode.Success ;
      }
      switch( optInMAType )
      {
      case Sma:
         retCode = smaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         dummyBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
         retCode = mamaUnguarded(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, dummyBuffer);
         break;
      case T3:
         retCode = t3Unguarded(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
