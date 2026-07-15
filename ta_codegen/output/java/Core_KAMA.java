/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  062704 MF     Fix limit case to avoid divid by zero (or by
 *                a value close to zero induce by the imprecision
 *                of floating points).
 *  070226 MF,CC  Allow period of 1: output is a copy of the input,
 *                consistent with TA_MA (issues #48, #59). The natural
 *                KAMA math at period=1 would be a fixed-alpha EMA
 *                (efficiency ratio is always 1), which would disagree
 *                with TA_MA's period-1 copy, so identity is explicit.
 */

   public int kamaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInTimePeriod == 1 ) {
         return this.unstablePeriod[FuncUnstId.Kama.ordinal()] ;
      }
      return optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()] ;

   }
   public RetCode kama( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double trailingValue = 0;
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
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      /* Default return values */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). The unstable period
       * still delays the first output for API consistency.
       */
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.Kama.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.Success ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()];
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Initialize the variables by going through
       * the lookback period.
       */
      sumROC1 = 0.0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         tempReal -= inReal[today];
         sumROC1 += Math.abs(tempReal);
      }
      /* At this point sumROC1 represent the
       * summation of the 1-day price difference
       * over the (optInTimePeriod-1)
       */
      /* Calculate the first KAMA */
      /* The yesterday price is used here as the previous KAMA. */
      prevKAMA = inReal[today - 1];
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      /* Save the trailing value. Do this because inReal
       * and outReal can be pointers to the same buffer.
       */
      trailingValue = tempReal2;
      /* Calculate the efficiency ratio */
      if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      /* Calculate the smoothing constant */
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      /* Calculate the KAMA like an EMA, using the
       * smoothing constant as the adaptive factor.
       */
      prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      /* 'today' keep track of where the processing is within the
       * input.
       */
      /* Skip the unstable period. Do the whole processing
       * needed for KAMA, but do not write it in the output.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      /* Write the first value. */
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      /* Do the KAMA calculation for the requested range. */
      while( today <= endIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         /* Adjust sumROC1:
          *  - Remove trailing ROC1
          *  - Add new ROC1
          */
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         /* Save the trailing value. Do this because inReal
          * and outReal can be pointers to the same buffer.
          */
         trailingValue = tempReal2;
         /* Calculate the efficiency ratio */
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         /* Calculate the smoothing constant */
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         /* Calculate the KAMA like an EMA, using the
          * smoothing constant as the adaptive factor.
          */
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode kamaUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double trailingValue = 0;
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.Kama.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.Success ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()];
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumROC1 = 0.0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         tempReal -= inReal[today];
         sumROC1 += Math.abs(tempReal);
      }
      prevKAMA = inReal[today - 1];
      tempReal = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      trailingValue = tempReal2;
      if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      while( today <= startIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      while( today <= endIdx ) {
         tempReal = inReal[today];
         tempReal2 = inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma(inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode kama( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double trailingValue = 0;
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
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.Kama.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.Success ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()];
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumROC1 = 0.0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = (double)inReal[today++];
         tempReal -= (double)inReal[today];
         sumROC1 += Math.abs(tempReal);
      }
      prevKAMA = (double)inReal[today - 1];
      tempReal = (double)inReal[today];
      tempReal2 = (double)inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      trailingValue = tempReal2;
      if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
      while( today <= startIdx ) {
         tempReal = (double)inReal[today];
         tempReal2 = (double)inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - (double)inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today];
         tempReal2 = (double)inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - (double)inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode kamaUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double constMax = 0;
      double constDiff = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double sumROC1 = 0;
      double periodROC = 0;
      double prevKAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double trailingValue = 0;
      constMax = 2.0 / (30.0 + 1.0);
      constDiff = 2.0 / (2.0 + 1.0) - constMax;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      if( optInTimePeriod == 1 ) {
         lookbackTotal = this.unstablePeriod[FuncUnstId.Kama.ordinal()];
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         if( startIdx > endIdx ) {
            return RetCode.Success ;
         }
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.Kama.ordinal()];
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumROC1 = 0.0;
      today = startIdx - lookbackTotal;
      trailingIdx = today;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempReal = (double)inReal[today++];
         tempReal -= (double)inReal[today];
         sumROC1 += Math.abs(tempReal);
      }
      prevKAMA = (double)inReal[today - 1];
      tempReal = (double)inReal[today];
      tempReal2 = (double)inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;
      trailingValue = tempReal2;
      if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
         tempReal = 1.0;
      } else {
         tempReal = Math.abs(periodROC / sumROC1);
      }
      tempReal = Math.fma(tempReal, constDiff, constMax);
      tempReal *= tempReal;
      prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
      while( today <= startIdx ) {
         tempReal = (double)inReal[today];
         tempReal2 = (double)inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - (double)inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
      }
      outReal[0] = prevKAMA;
      outIdx = 1;
      outBegIdx.value = today - 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today];
         tempReal2 = (double)inReal[trailingIdx++];
         periodROC = tempReal - tempReal2;
         sumROC1 -= Math.abs(trailingValue - tempReal2);
         sumROC1 += Math.abs(tempReal - (double)inReal[today - 1]);
         trailingValue = tempReal2;
         if( sumROC1 <= periodROC || ((-0.00000000000001 < sumROC1) && (sumROC1 < 0.00000000000001)) ) {
            tempReal = 1.0;
         } else {
            tempReal = Math.abs(periodROC / sumROC1);
         }
         tempReal = Math.fma(tempReal, constDiff, constMax);
         tempReal *= tempReal;
         prevKAMA = Math.fma((double)inReal[today++] - prevKAMA, tempReal, prevKAMA);
         outReal[outIdx++] = prevKAMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
