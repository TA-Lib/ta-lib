/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  JP       John Price <jp_talib@gcfl.net>
 *  MF       Mario Fortier
 *  AM       Adrian Michel <http://amichel.com>
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  070203 JP      Initial.
 *  072106 MF,AM   Fix #1526632. Add missing atan().
 *  071326 MF,CC   O(period) per-bar rescan -> O(1) sliding-sum recurrence
 *                 (numerics-changing). See issue #103.
 */

   public int linearRegAngleLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode linearRegAngle( int startIdx,
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
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
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
      lookbackTotal = linearRegAngleLookback(optInTimePeriod);
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
      trailingIdx = startIdx - lookbackTotal;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      /* Prime the two data-dependent window sums for the first output with a
       * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
       * SumY = sum of the window, SumXY = sum of i*value (i the reversed
       * 0..period-1 position).
       */
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      outReal[outIdx++] = Math.atan(m) * (180.0 / 3.141592653589793);
      today += 1;
      /* Slide the window one bar at a time, keeping both sums in O(1): advancing
       * the window raises every retained value's weight by 1 (adds SumY) and drops
       * the departing value at full weight (subtracts period*trailingValue). Same
       * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
       * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
       */
      while( today <= endIdx ) {
         trailingValue = inReal[trailingIdx++];
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + inReal[today];
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         outReal[outIdx++] = Math.atan(m) * (180.0 / 3.141592653589793);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode linearRegAngleUnguarded( int startIdx,
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
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
      lookbackTotal = linearRegAngleLookback(optInTimePeriod);
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
      trailingIdx = startIdx - lookbackTotal;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      outReal[outIdx++] = Math.atan(m) * (180.0 / 3.141592653589793);
      today += 1;
      while( today <= endIdx ) {
         trailingValue = inReal[trailingIdx++];
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + inReal[today];
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         outReal[outIdx++] = Math.atan(m) * (180.0 / 3.141592653589793);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode linearRegAngle( int startIdx,
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
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
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
      lookbackTotal = linearRegAngleLookback(optInTimePeriod);
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
      trailingIdx = startIdx - lookbackTotal;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = (double)inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      outReal[outIdx++] = Math.atan(m) * (180.0 / 3.141592653589793);
      today += 1;
      while( today <= endIdx ) {
         trailingValue = (double)inReal[trailingIdx++];
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + (double)inReal[today];
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         outReal[outIdx++] = Math.atan(m) * (180.0 / 3.141592653589793);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode linearRegAngleUnguarded( int startIdx,
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
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      double m = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
      lookbackTotal = linearRegAngleLookback(optInTimePeriod);
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
      trailingIdx = startIdx - lookbackTotal;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = (double)inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      outReal[outIdx++] = Math.atan(m) * (180.0 / 3.141592653589793);
      today += 1;
      while( today <= endIdx ) {
         trailingValue = (double)inReal[trailingIdx++];
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + (double)inReal[today];
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         outReal[outIdx++] = Math.atan(m) * (180.0 / 3.141592653589793);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
