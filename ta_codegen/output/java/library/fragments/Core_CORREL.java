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
 *  120802 MF   Template creation.
 *  101003 MF   Initial Coding
 *  062804 MF   Resolve div by zero bug on limit case.
 */

   public int correlLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode correl( int startIdx,
                          int endIdx,
                          double inReal0[],
                          double inReal1[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
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
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      /* Calculate the initial values. */
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( today = trailingIdx; today <= startIdx; today += 1 ) {
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      /* Write the first output.
       * Save first the trailing values since the input
       * and output might be the same array,
       */
      trailingX = inReal0[trailingIdx];
      trailingY = inReal1[trailingIdx++];
      tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
      if( !(tempReal < 0.00000000000001) ) {
         outReal[0] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
      } else {
         outReal[0] = 0.0;
      }
      /* Tight loop to do subsequent values. */
      outIdx = 1;
      while( today <= endIdx ) {
         /* Remove trailing values */
         sumX -= trailingX;
         sumX2 -= trailingX * trailingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= trailingY * trailingY;
         /* Add new values */
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today++];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         /* Output new coefficient.
          * Save first the trailing values since the input
          * and output might be the same array,
          */
         trailingX = inReal0[trailingIdx];
         trailingY = inReal1[trailingIdx++];
         tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
         if( !(tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode correlUnguarded( int startIdx,
                                   int endIdx,
                                   double inReal0[],
                                   double inReal1[],
                                   int optInTimePeriod,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( today = trailingIdx; today <= startIdx; today += 1 ) {
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      trailingX = inReal0[trailingIdx];
      trailingY = inReal1[trailingIdx++];
      tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
      if( !(tempReal < 0.00000000000001) ) {
         outReal[0] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
      } else {
         outReal[0] = 0.0;
      }
      outIdx = 1;
      while( today <= endIdx ) {
         sumX -= trailingX;
         sumX2 -= trailingX * trailingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= trailingY * trailingY;
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today++];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         trailingX = inReal0[trailingIdx];
         trailingY = inReal1[trailingIdx++];
         tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
         if( !(tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode correl( int startIdx,
                          int endIdx,
                          float inReal0[],
                          float inReal1[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
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
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( today = trailingIdx; today <= startIdx; today += 1 ) {
         x = (double)inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = (double)inReal1[today];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      trailingX = (double)inReal0[trailingIdx];
      trailingY = (double)inReal1[trailingIdx++];
      tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
      if( !(tempReal < 0.00000000000001) ) {
         outReal[0] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
      } else {
         outReal[0] = 0.0;
      }
      outIdx = 1;
      while( today <= endIdx ) {
         sumX -= trailingX;
         sumX2 -= trailingX * trailingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= trailingY * trailingY;
         x = (double)inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = (double)inReal1[today++];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         trailingX = (double)inReal0[trailingIdx];
         trailingY = (double)inReal1[trailingIdx++];
         tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
         if( !(tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode correlUnguarded( int startIdx,
                                   int endIdx,
                                   float inReal0[],
                                   float inReal1[],
                                   int optInTimePeriod,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( today = trailingIdx; today <= startIdx; today += 1 ) {
         x = (double)inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = (double)inReal1[today];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      trailingX = (double)inReal0[trailingIdx];
      trailingY = (double)inReal1[trailingIdx++];
      tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
      if( !(tempReal < 0.00000000000001) ) {
         outReal[0] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
      } else {
         outReal[0] = 0.0;
      }
      outIdx = 1;
      while( today <= endIdx ) {
         sumX -= trailingX;
         sumX2 -= trailingX * trailingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= trailingY * trailingY;
         x = (double)inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = (double)inReal1[today++];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         trailingX = (double)inReal0[trailingIdx];
         trailingY = (double)inReal1[trailingIdx++];
         tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
         if( !(tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
