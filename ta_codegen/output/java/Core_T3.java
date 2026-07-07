/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MHL      Matthew Lindblom
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  032003 MHL    Implementation of T3
 *  040503 MF     Adapt for compatibility with published code
 *                for TradeStation and Metastock.
 *                See "Smoothing Techniques For More Accurate Signals"
 *                from Tim Tillson in Stock&Commodities V16:1 Page 33-37
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070226 MF,CC  Allow period of 1: output is an exact copy of the
 *                input, consistent with TA_MA (issues #48, #59). The
 *                natural math is only near-identity at period=1: the
 *                coefficients sum to 1 in real arithmetic but not in
 *                floating point (~1e-14 drift), so the copy is explicit.
 */

   public int t3Lookback( int optInTimePeriod, double optInVFactor )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInVFactor == -4e37 ) {
         optInVFactor = 7e-1;
      } else if( optInVFactor < 0e0 || optInVFactor > 1e0 ) {
         return -1;
      }
      return 6 * (optInTimePeriod - 1) + this.unstablePeriod[FuncUnstId.T3.ordinal()] ;

   }
   public RetCode t3( int startIdx,
                      int endIdx,
                      double inReal[],
                      int optInTimePeriod,
                      double optInVFactor,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      int outIdx = 0;
      int lookbackTotal = 0;
      int today = 0;
      int i = 0;
      double k = 0;
      double one_minus_k = 0;
      double e1 = 0;
      double e2 = 0;
      double e3 = 0;
      double e4 = 0;
      double e5 = 0;
      double e6 = 0;
      double c1 = 0;
      double c2 = 0;
      double c3 = 0;
      double c4 = 0;
      double tempReal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInVFactor == -4e37 ) {
         optInVFactor = 7e-1;
      } else if( optInVFactor < 0e0 || optInVFactor > 1e0 ) {
         return RetCode.BadParam;
      }
      /* For an explanation of this function, please read:
       *
       * Magazine articles written by Tim Tillson
       *
       * Essentially, a T3 of time serie 't' is:
       *   EMA1(x,Period) = EMA(x,Period)
       *   EMA2(x,Period) = EMA(EMA1(x,Period),Period)
       *   GD(x,Period,vFactor) = (EMA1(x,Period)*(1+vFactor)) - (EMA2(x,Period)*vFactor)
       *   T3 = GD (GD ( GD(t, Period, vFactor), Period, vFactor), Period, vFactor);
       *
       * T3 offers a moving average with less lags then the
       * traditional EMA.
       *
       * Do not confuse a T3 with EMA3. Both are called "Triple EMA"
       * in the litterature.
       */
      lookbackTotal = 6 * (optInTimePeriod - 1) + this.unstablePeriod[FuncUnstId.T3.ordinal()];
      if( startIdx <= lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outNBElement.value = 0;
         outBegIdx.value = 0;
         return RetCode.Success ;
      }
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). Explicit because the
       * coefficients below sum to 1 only in real arithmetic; going through
       * the math would leave ~1e-14 floating-point drift on every value.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      k = 2.0 / (optInTimePeriod + 1.0);
      one_minus_k = 1.0 - k;
      /* Initialize e1 */
      tempReal = inReal[today++];
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         tempReal += inReal[today++];
      }
      e1 = tempReal / optInTimePeriod;
      /* Initialize e2 */
      tempReal = e1;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      /* Initialize e3 */
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      /* Initialize e4 */
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      /* Initialize e5 */
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      /* Initialize e6 */
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      /* Skip the unstable period */
      while( today <= startIdx ) {
         /* Do the calculation but do not write the output */
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         e6 = k * e5 + one_minus_k * e6;
      }
      /* Calculate the constants */
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = 1.0 + 3.0 * optInVFactor - c1 + 3.0 * tempReal;
      /* Write the first output */
      outIdx = 0;
      outReal[outIdx++] = c1 * e6 + c2 * e5 + c3 * e4 + c4 * e3;
      /* Calculate and output the remaining of the range. */
      while( today <= endIdx ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         e6 = k * e5 + one_minus_k * e6;
         outReal[outIdx++] = c1 * e6 + c2 * e5 + c3 * e4 + c4 * e3;
      }
      /* Indicates to the caller the number of output
       * successfully calculated.
       */
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode t3Unguarded( int startIdx,
                               int endIdx,
                               double inReal[],
                               int optInTimePeriod,
                               double optInVFactor,
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               double outReal[] )
   {
      int outIdx = 0;
      int lookbackTotal = 0;
      int today = 0;
      int i = 0;
      double k = 0;
      double one_minus_k = 0;
      double e1 = 0;
      double e2 = 0;
      double e3 = 0;
      double e4 = 0;
      double e5 = 0;
      double e6 = 0;
      double c1 = 0;
      double c2 = 0;
      double c3 = 0;
      double c4 = 0;
      double tempReal = 0;
      lookbackTotal = 6 * (optInTimePeriod - 1) + this.unstablePeriod[FuncUnstId.T3.ordinal()];
      if( startIdx <= lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outNBElement.value = 0;
         outBegIdx.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      k = 2.0 / (optInTimePeriod + 1.0);
      one_minus_k = 1.0 - k;
      tempReal = inReal[today++];
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         tempReal += inReal[today++];
      }
      e1 = tempReal / optInTimePeriod;
      tempReal = e1;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         e6 = k * e5 + one_minus_k * e6;
      }
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = 1.0 + 3.0 * optInVFactor - c1 + 3.0 * tempReal;
      outIdx = 0;
      outReal[outIdx++] = c1 * e6 + c2 * e5 + c3 * e4 + c4 * e3;
      while( today <= endIdx ) {
         e1 = k * inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         e6 = k * e5 + one_minus_k * e6;
         outReal[outIdx++] = c1 * e6 + c2 * e5 + c3 * e4 + c4 * e3;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode t3( int startIdx,
                      int endIdx,
                      float inReal[],
                      int optInTimePeriod,
                      double optInVFactor,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      int outIdx = 0;
      int lookbackTotal = 0;
      int today = 0;
      int i = 0;
      double k = 0;
      double one_minus_k = 0;
      double e1 = 0;
      double e2 = 0;
      double e3 = 0;
      double e4 = 0;
      double e5 = 0;
      double e6 = 0;
      double c1 = 0;
      double c2 = 0;
      double c3 = 0;
      double c4 = 0;
      double tempReal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInVFactor == -4e37 ) {
         optInVFactor = 7e-1;
      } else if( optInVFactor < 0e0 || optInVFactor > 1e0 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = 6 * (optInTimePeriod - 1) + this.unstablePeriod[FuncUnstId.T3.ordinal()];
      if( startIdx <= lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outNBElement.value = 0;
         outBegIdx.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      k = 2.0 / (optInTimePeriod + 1.0);
      one_minus_k = 1.0 - k;
      tempReal = (double)inReal[today++];
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         tempReal += (double)inReal[today++];
      }
      e1 = tempReal / optInTimePeriod;
      tempReal = e1;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         e6 = k * e5 + one_minus_k * e6;
      }
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = 1.0 + 3.0 * optInVFactor - c1 + 3.0 * tempReal;
      outIdx = 0;
      outReal[outIdx++] = c1 * e6 + c2 * e5 + c3 * e4 + c4 * e3;
      while( today <= endIdx ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         e6 = k * e5 + one_minus_k * e6;
         outReal[outIdx++] = c1 * e6 + c2 * e5 + c3 * e4 + c4 * e3;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode t3Unguarded( int startIdx,
                               int endIdx,
                               float inReal[],
                               int optInTimePeriod,
                               double optInVFactor,
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               double outReal[] )
   {
      int outIdx = 0;
      int lookbackTotal = 0;
      int today = 0;
      int i = 0;
      double k = 0;
      double one_minus_k = 0;
      double e1 = 0;
      double e2 = 0;
      double e3 = 0;
      double e4 = 0;
      double e5 = 0;
      double e6 = 0;
      double c1 = 0;
      double c2 = 0;
      double c3 = 0;
      double c4 = 0;
      double tempReal = 0;
      lookbackTotal = 6 * (optInTimePeriod - 1) + this.unstablePeriod[FuncUnstId.T3.ordinal()];
      if( startIdx <= lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outNBElement.value = 0;
         outBegIdx.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      k = 2.0 / (optInTimePeriod + 1.0);
      one_minus_k = 1.0 - k;
      tempReal = (double)inReal[today++];
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         tempReal += (double)inReal[today++];
      }
      e1 = tempReal / optInTimePeriod;
      tempReal = e1;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         e6 = k * e5 + one_minus_k * e6;
      }
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = 1.0 + 3.0 * optInVFactor - c1 + 3.0 * tempReal;
      outIdx = 0;
      outReal[outIdx++] = c1 * e6 + c2 * e5 + c3 * e4 + c4 * e3;
      while( today <= endIdx ) {
         e1 = k * (double)inReal[today++] + one_minus_k * e1;
         e2 = k * e1 + one_minus_k * e2;
         e3 = k * e2 + one_minus_k * e3;
         e4 = k * e3 + one_minus_k * e4;
         e5 = k * e4 + one_minus_k * e5;
         e6 = k * e5 + one_minus_k * e6;
         outReal[outIdx++] = c1 * e6 + c2 * e5 + c3 * e4 + c4 * e3;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
