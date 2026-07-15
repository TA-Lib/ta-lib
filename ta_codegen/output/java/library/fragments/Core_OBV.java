/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  110206 AC   Change volume and open interest to double
 */

   public int obvLookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   public RetCode obv( int startIdx,
                       int endIdx,
                       double inReal[],
                       double inVolume[],
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevReal = 0;
      double tempReal = 0;
      double prevOBV = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      prevOBV = inVolume[startIdx];
      prevReal = inReal[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = inReal[i];
         if( tempReal > prevReal ) {
            prevOBV += inVolume[i];
         } else if( tempReal < prevReal ) {
            prevOBV -= inVolume[i];
         }
         outReal[outIdx++] = prevOBV;
         prevReal = tempReal;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode obvUnguarded( int startIdx,
                                int endIdx,
                                double inReal[],
                                double inVolume[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevReal = 0;
      double tempReal = 0;
      double prevOBV = 0;
      prevOBV = inVolume[startIdx];
      prevReal = inReal[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = inReal[i];
         if( tempReal > prevReal ) {
            prevOBV += inVolume[i];
         } else if( tempReal < prevReal ) {
            prevOBV -= inVolume[i];
         }
         outReal[outIdx++] = prevOBV;
         prevReal = tempReal;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode obv( int startIdx,
                       int endIdx,
                       float inReal[],
                       float inVolume[],
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevReal = 0;
      double tempReal = 0;
      double prevOBV = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      prevOBV = (double)inVolume[startIdx];
      prevReal = (double)inReal[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = (double)inReal[i];
         if( tempReal > prevReal ) {
            prevOBV += (double)inVolume[i];
         } else if( tempReal < prevReal ) {
            prevOBV -= (double)inVolume[i];
         }
         outReal[outIdx++] = prevOBV;
         prevReal = tempReal;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode obvUnguarded( int startIdx,
                                int endIdx,
                                float inReal[],
                                float inVolume[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevReal = 0;
      double tempReal = 0;
      double prevOBV = 0;
      prevOBV = (double)inVolume[startIdx];
      prevReal = (double)inReal[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = (double)inReal[i];
         if( tempReal > prevReal ) {
            prevOBV += (double)inVolume[i];
         } else if( tempReal < prevReal ) {
            prevOBV -= (double)inVolume[i];
         }
         outReal[outIdx++] = prevOBV;
         prevReal = tempReal;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
