/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  JP       John Price <jp_talib@gcfl.net>
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  070203 JP   Initial.
 */

   public int linearRegLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode linearReg( int startIdx,
                             int endIdx,
                             double inReal[],
                             int optInTimePeriod,
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      double b = 0;
      int i = 0;
      double tempValue1 = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Linear Regression is a concept also known as the
       * "least squares method" or "best fit." Linear
       * Regression attempts to fit a straight line between
       * several data points in such a way that distance
       * between each data point and the line is minimized.
       *
       * For each point, a straight line over the specified
       * previous bar period is determined in terms
       * of y = b + m*x:
       *
       * TA_LINEARREG          : Returns b+m*(period-1)
       * TA_LINEARREG_SLOPE    : Returns 'm'
       * TA_LINEARREG_ANGLE    : Returns 'm' in degree.
       * TA_LINEARREG_INTERCEPT: Returns 'b'
       * TA_TSF                : Returns b+m*(period)
       */
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = linearRegLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      /* Index into the output. */
      today = startIdx;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      while( today <= endIdx ) {
         SumXY = 0;
         SumY = 0;
         for( i = optInTimePeriod; i-- != 0;  ) {
            tempValue1 = inReal[today - i];
            SumY += tempValue1;
            SumXY += (double)i * tempValue1;
         }
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         b = (SumY - m * SumX) / (double)optInTimePeriod;
         outReal[outIdx++] = b + m * (double)(optInTimePeriod - 1);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode linearRegUnguarded( int startIdx,
                                      int endIdx,
                                      double inReal[],
                                      int optInTimePeriod,
                                      MInteger outBegIdx,
                                      MInteger outNBElement,
                                      double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      double b = 0;
      int i = 0;
      double tempValue1 = 0;
      lookbackTotal = linearRegLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      while( today <= endIdx ) {
         SumXY = 0;
         SumY = 0;
         for( i = optInTimePeriod; i-- != 0;  ) {
            tempValue1 = inReal[today - i];
            SumY += tempValue1;
            SumXY += (double)i * tempValue1;
         }
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         b = (SumY - m * SumX) / (double)optInTimePeriod;
         outReal[outIdx++] = b + m * (double)(optInTimePeriod - 1);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode linearReg( int startIdx,
                             int endIdx,
                             float inReal[],
                             int optInTimePeriod,
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      double b = 0;
      int i = 0;
      double tempValue1 = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = linearRegLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      while( today <= endIdx ) {
         SumXY = 0;
         SumY = 0;
         for( i = optInTimePeriod; i-- != 0;  ) {
            tempValue1 = (double)inReal[today - i];
            SumY += tempValue1;
            SumXY += (double)i * tempValue1;
         }
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         b = (SumY - m * SumX) / (double)optInTimePeriod;
         outReal[outIdx++] = b + m * (double)(optInTimePeriod - 1);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode linearRegUnguarded( int startIdx,
                                      int endIdx,
                                      float inReal[],
                                      int optInTimePeriod,
                                      MInteger outBegIdx,
                                      MInteger outNBElement,
                                      double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      double b = 0;
      int i = 0;
      double tempValue1 = 0;
      lookbackTotal = linearRegLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      while( today <= endIdx ) {
         SumXY = 0;
         SumY = 0;
         for( i = optInTimePeriod; i-- != 0;  ) {
            tempValue1 = (double)inReal[today - i];
            SumY += tempValue1;
            SumXY += (double)i * tempValue1;
         }
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         b = (SumY - m * SumX) / (double)optInTimePeriod;
         outReal[outIdx++] = b + m * (double)(optInTimePeriod - 1);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
