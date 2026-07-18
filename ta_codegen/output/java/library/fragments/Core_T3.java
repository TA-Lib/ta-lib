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
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      /* Initialize e3 */
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      /* Initialize e4 */
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      /* Initialize e5 */
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      /* Initialize e6 */
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      /* Skip the unstable period */
      while( today <= startIdx ) {
         /* Do the calculation but do not write the output */
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
      }
      /* Calculate the constants */
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = Math.fma(3.0, tempReal, Math.fma(3.0, optInVFactor, 1.0) - c1);
      /* Write the first output */
      outIdx = 0;
      outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      /* Calculate and output the remaining of the range. */
      while( today <= endIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
         outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
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
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
      }
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = Math.fma(3.0, tempReal, Math.fma(3.0, optInVFactor, 1.0) - c1);
      outIdx = 0;
      outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      while( today <= endIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
         outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
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
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
      }
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = Math.fma(3.0, tempReal, Math.fma(3.0, optInVFactor, 1.0) - c1);
      outIdx = 0;
      outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      while( today <= endIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
         outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
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
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
      }
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = Math.fma(3.0, tempReal, Math.fma(3.0, optInVFactor, 1.0) - c1);
      outIdx = 0;
      outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      while( today <= endIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
         outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live T3 stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#t3} over the same series.
    * Open with {@link Core#t3Open}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class T3Stream {
      final Core core;
      int optInTimePeriod;
      double optInVFactor;
      double k;
      double one_minus_k;
      double e1;
      double e2;
      double e3;
      double e4;
      double e5;
      double e6;
      double c1;
      double c2;
      double c3;
      double c4;
      double cur_outReal;

      T3Stream( Core core ) { this.core = core; }

      T3Stream( T3Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInVFactor = other.optInVFactor;
         this.k = other.k;
         this.one_minus_k = other.one_minus_k;
         this.e1 = other.e1;
         this.e2 = other.e2;
         this.e3 = other.e3;
         this.e4 = other.e4;
         this.e5 = other.e5;
         this.e6 = other.e6;
         this.c1 = other.c1;
         this.c2 = other.c2;
         this.c3 = other.c3;
         this.c4 = other.c4;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.t3StreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         T3Stream scratch = new T3Stream(this);
         core.t3StreamStep(scratch, inReal);
         return scratch.cur_outReal;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public T3Stream copy() {
         return new T3Stream(this);
      }
   }
   void t3StreamStep( T3Stream sp, double inReal )
   {
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      sp.e1 = Math.fma(sp.one_minus_k, sp.e1, sp.k * inReal);
      sp.e2 = Math.fma(sp.one_minus_k, sp.e2, sp.k * sp.e1);
      sp.e3 = Math.fma(sp.one_minus_k, sp.e3, sp.k * sp.e2);
      sp.e4 = Math.fma(sp.one_minus_k, sp.e4, sp.k * sp.e3);
      sp.e5 = Math.fma(sp.one_minus_k, sp.e5, sp.k * sp.e4);
      sp.e6 = Math.fma(sp.one_minus_k, sp.e6, sp.k * sp.e5);
      sp.cur_outReal = Math.fma(sp.c4, sp.e3, Math.fma(sp.c3, sp.e4, Math.fma(sp.c1, sp.e6, sp.c2 * sp.e5)));
   }
   private RetCode t3OpenBody( T3Stream sp, double inReal[], int startIdx, int optInTimePeriod, double optInVFactor )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
      if( optInTimePeriod == 1 ) {
         if( historyLen < t3Lookback(optInTimePeriod, optInVFactor) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInVFactor = optInVFactor;
         sp.k = 0.0;
         sp.one_minus_k = 0.0;
         sp.e1 = 0.0;
         sp.e2 = 0.0;
         sp.e3 = 0.0;
         sp.e4 = 0.0;
         sp.e5 = 0.0;
         sp.e6 = 0.0;
         sp.c1 = 0.0;
         sp.c2 = 0.0;
         sp.c3 = 0.0;
         sp.c4 = 0.0;
         sp.cur_outReal = inReal[historyLen - 1];
         return RetCode.Success;
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
         return RetCode.OutOfRangeEndIndex ;
      }
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). Explicit because the
       * coefficients below sum to 1 only in real arithmetic; going through
       * the math would leave ~1e-14 floating-point drift on every value.
       */
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
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      /* Initialize e3 */
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      /* Initialize e4 */
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      /* Initialize e5 */
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      /* Initialize e6 */
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      /* Skip the unstable period */
      while( today <= startIdx ) {
         /* Do the calculation but do not write the output */
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
      }
      /* Calculate the constants */
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = Math.fma(3.0, tempReal, Math.fma(3.0, optInVFactor, 1.0) - c1);
      /* Write the first output */
      outIdx = 0;
      lastValue_outReal = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      /* Calculate and output the remaining of the range. */
      while( today <= endIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
         lastValue_outReal = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      }
      /* Indicates to the caller the number of output
       * successfully calculated.
       */
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInVFactor = optInVFactor;
      sp.k = k;
      sp.one_minus_k = one_minus_k;
      sp.e1 = e1;
      sp.e2 = e2;
      sp.e3 = e3;
      sp.e4 = e4;
      sp.e5 = e5;
      sp.e6 = e6;
      sp.c1 = c1;
      sp.c2 = c2;
      sp.c3 = c3;
      sp.c4 = c4;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode t3OpenAndFillBody( T3Stream sp, double inReal[], int optInTimePeriod, double optInVFactor, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         if( historyLen < t3Lookback(optInTimePeriod, optInVFactor) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInVFactor = optInVFactor;
         sp.k = 0.0;
         sp.one_minus_k = 0.0;
         sp.e1 = 0.0;
         sp.e2 = 0.0;
         sp.e3 = 0.0;
         sp.e4 = 0.0;
         sp.e5 = 0.0;
         sp.e6 = 0.0;
         sp.c1 = 0.0;
         sp.c2 = 0.0;
         sp.c3 = 0.0;
         sp.c4 = 0.0;
         int fillLb = t3Lookback(optInTimePeriod, optInVFactor);
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
            outReal[fillIdx] = inReal[fillLb + fillIdx];
         }
         sp.cur_outReal = outReal[outNBElement.value - 1];
         return RetCode.Success;
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
         return RetCode.OutOfRangeEndIndex ;
      }
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). Explicit because the
       * coefficients below sum to 1 only in real arithmetic; going through
       * the math would leave ~1e-14 floating-point drift on every value.
       */
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
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      /* Initialize e3 */
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      /* Initialize e4 */
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      /* Initialize e5 */
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      /* Initialize e6 */
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      /* Skip the unstable period */
      while( today <= startIdx ) {
         /* Do the calculation but do not write the output */
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
      }
      /* Calculate the constants */
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = Math.fma(3.0, tempReal, Math.fma(3.0, optInVFactor, 1.0) - c1);
      /* Write the first output */
      outIdx = 0;
      outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      /* Calculate and output the remaining of the range. */
      while( today <= endIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
         outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      }
      /* Indicates to the caller the number of output
       * successfully calculated.
       */
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInVFactor = optInVFactor;
      sp.k = k;
      sp.one_minus_k = one_minus_k;
      sp.e1 = e1;
      sp.e2 = e2;
      sp.e3 = e3;
      sp.e4 = e4;
      sp.e5 = e5;
      sp.e6 = e6;
      sp.c1 = c1;
      sp.c2 = c2;
      sp.c3 = c3;
      sp.c4 = c4;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind t3Open (composition seam). */
   T3Stream t3OpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInVFactor )
   {
      T3Stream sp = new T3Stream(this);
      RetCode retCode = t3OpenBody(sp, inReal, startIdx, optInTimePeriod, optInVFactor);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_T3 open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_T3 open: internal error");
      }
      throw new IllegalArgumentException("TA_T3 open: " + retCode);
   }
   /**
    * Open a live T3 stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#t3} at that bar.
    * <p>The history must hold at least {@code t3Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public T3Stream t3Open( double inReal[], int optInTimePeriod, double optInVFactor )
   {
      return t3OpenInternal(inReal, 0, optInTimePeriod, optInVFactor);
   }
   /**
    * {@link Core#t3Open} that also fills the output array(s) bit-identically
    * to {@link Core#t3} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public T3Stream t3OpenAndFill( double inReal[], int optInTimePeriod, double optInVFactor, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      T3Stream sp = new T3Stream(this);
      RetCode retCode = t3OpenAndFillBody(sp, inReal, optInTimePeriod, optInVFactor, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_T3 openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_T3 openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_T3 openAndFill: " + retCode);
   }
