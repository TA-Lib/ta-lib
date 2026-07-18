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
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   public int htSineLookback( )
   {
      /* 31 input are skip
       * +32 output are skip to account for misc lookback
       * ---
       *  63 Total Lookback
       *
       * 31 is for being compatible with Tradestation.
       * See mama_lookback for an explanation of the "32".
       */
      return 63 + this.unstablePeriod[FuncUnstId.HtSine.ordinal()] ;

   }
   public RetCode htSine( int startIdx,
                          int endIdx,
                          double inReal[],
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outSine[],
                          double outLeadSine[] )
   {
      int outIdx = 0;
      int i = 0;
      int lookbackTotal = 0;
      int today = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double adjustedPrevPeriod = 0;
      double period = 0;
      int trailingWMAIdx = 0;
      double periodWMASum = 0;
      double periodWMASub = 0;
      double trailingWMAValue = 0;
      double smoothedValue = 0;
      double a = 0;
      double b = 0;
      double hilbertTempReal = 0;
      int hilbertIdx = 0;
      double[] detrender_Odd = new double[3];
      double[] detrender_Even = new double[3];
      double detrender = 0;
      double prev_detrender_Odd = 0;
      double prev_detrender_Even = 0;
      double prev_detrender_input_Odd = 0;
      double prev_detrender_input_Even = 0;
      double[] Q1_Odd = new double[3];
      double[] Q1_Even = new double[3];
      double Q1 = 0;
      double prev_Q1_Odd = 0;
      double prev_Q1_Even = 0;
      double prev_Q1_input_Odd = 0;
      double prev_Q1_input_Even = 0;
      double[] jI_Odd = new double[3];
      double[] jI_Even = new double[3];
      double jI = 0;
      double prev_jI_Odd = 0;
      double prev_jI_Even = 0;
      double prev_jI_input_Odd = 0;
      double prev_jI_input_Even = 0;
      double[] jQ_Odd = new double[3];
      double[] jQ_Even = new double[3];
      double jQ = 0;
      double prev_jQ_Odd = 0;
      double prev_jQ_Even = 0;
      double prev_jQ_input_Odd = 0;
      double prev_jQ_input_Even = 0;
      double Q2 = 0;
      double I2 = 0;
      double prevQ2 = 0;
      double prevI2 = 0;
      double Re = 0;
      double Im = 0;
      double I1ForOddPrev2 = 0;
      double I1ForOddPrev3 = 0;
      double I1ForEvenPrev2 = 0;
      double I1ForEvenPrev3 = 0;
      double rad2Deg = 0;
      double deg2Rad = 0;
      double constDeg2RadBy360 = 0;
      double todayValue = 0;
      double smoothPeriod = 0;
      double[] smoothPrice;
      int smoothPrice_Idx = 0;
      int maxIdx_smoothPrice = (50)-1;
      int idx = 0;
      int DCPeriodInt = 0;
      double DCPhase = 0;
      double DCPeriod = 0;
      double imagPart = 0;
      double realPart = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( outSine == outLeadSine ) {
         return RetCode.BadParam ;
      }
      a = 0.0962;
      b = 0.5769;
      /* Variable used for the price smoother (a weighted moving average). */
      /* Variables used for the Hilbert Transormation */
      /* Varaible used to keep track of the previous
       * smooth price. In the case of this algorithm,
       * we will never need more than 50 values.
       */
      smoothPrice = new double[maxIdx_smoothPrice+1];
      /* Variable used to calculate the dominant cycle phase */
      /* circular buffer already declared */
      /* The following could be replaced by constant eventually. */
      tempReal = Math.atan(1);
      rad2Deg = 45.0 / tempReal;
      deg2Rad = 1.0 / rad2Deg;
      constDeg2RadBy360 = tempReal * 8.0;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = 63 + this.unstablePeriod[FuncUnstId.HtSine.ordinal()];
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
      outBegIdx.value = startIdx;
      /* Initialize the price smoother, which is simply a weighted
       * moving average of the price.
       * To understand this algorithm, I strongly suggest to understand
       * first how TA_WMA is done.
       */
      trailingWMAIdx = startIdx - lookbackTotal;
      today = trailingWMAIdx;
      /* Initialization is same as WMA, except loop is unrolled
       * for speed optimization.
       */
      tempReal = inReal[today++];
      periodWMASub = tempReal;
      periodWMASum = tempReal;
      tempReal = inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 2.0;
      tempReal = inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 3.0;
      trailingWMAValue = 0.0;
      /* Subsequent WMA value are evaluated by using
       * the DO_PRICE_WMA macro.
       */
      i = 34;
      do {
         tempReal = inReal[today++];
         periodWMASub += tempReal;
         periodWMASub -= trailingWMAValue;
         periodWMASum += tempReal * 4.0;
         trailingWMAValue = inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
      } while( --i != 0 );
      /* Initialize the circular buffers used by the hilbert
       * transform logic.
       * A buffer is used for odd day and another for even days.
       * This minimize the number of memory access and floating point
       * operations needed (note also that by using static circular buffer,
       * no large dynamic memory allocation is needed for storing
       * intermediate calculation!).
       */
      hilbertIdx = 0;
      detrender_Odd[0] = 0.0;
      detrender_Odd[1] = 0.0;
      detrender_Odd[2] = 0.0;
      detrender_Even[0] = 0.0;
      detrender_Even[1] = 0.0;
      detrender_Even[2] = 0.0;
      detrender = 0.0;
      prev_detrender_Odd = 0.0;
      prev_detrender_Even = 0.0;
      prev_detrender_input_Odd = 0.0;
      prev_detrender_input_Even = 0.0;
      Q1_Odd[0] = 0.0;
      Q1_Odd[1] = 0.0;
      Q1_Odd[2] = 0.0;
      Q1_Even[0] = 0.0;
      Q1_Even[1] = 0.0;
      Q1_Even[2] = 0.0;
      Q1 = 0.0;
      prev_Q1_Odd = 0.0;
      prev_Q1_Even = 0.0;
      prev_Q1_input_Odd = 0.0;
      prev_Q1_input_Even = 0.0;
      jI_Odd[0] = 0.0;
      jI_Odd[1] = 0.0;
      jI_Odd[2] = 0.0;
      jI_Even[0] = 0.0;
      jI_Even[1] = 0.0;
      jI_Even[2] = 0.0;
      jI = 0.0;
      prev_jI_Odd = 0.0;
      prev_jI_Even = 0.0;
      prev_jI_input_Odd = 0.0;
      prev_jI_input_Even = 0.0;
      jQ_Odd[0] = 0.0;
      jQ_Odd[1] = 0.0;
      jQ_Odd[2] = 0.0;
      jQ_Even[0] = 0.0;
      jQ_Even[1] = 0.0;
      jQ_Even[2] = 0.0;
      jQ = 0.0;
      prev_jQ_Odd = 0.0;
      prev_jQ_Even = 0.0;
      prev_jQ_input_Odd = 0.0;
      prev_jQ_input_Even = 0.0;
      period = 0.0;
      outIdx = 0;
      prevQ2 = 0.0;
      prevI2 = prevQ2;
      Im = 0.0;
      Re = Im;
      I1ForEvenPrev3 = 0.0;
      I1ForOddPrev3 = I1ForEvenPrev3;
      I1ForEvenPrev2 = 0.0;
      I1ForOddPrev2 = I1ForEvenPrev2;
      smoothPeriod = 0.0;
      for( i = 0; i < 50; i += 1 ) {
         smoothPrice[i] = 0.0;
      }
      /* The code is speed optimized and is most likely very
       * hard to follow if you do not already know well the
       * original algorithm.
       * To understadn better, it is strongly suggested to look
       * first at the Excel implementation in "test_MAMA.xls" included
       * in this package.
       */
      DCPhase = 0.0;
      while( today <= endIdx ) {
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
         todayValue = inReal[today];
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
         /* Remember the smoothedValue into the smoothPrice
          * circular buffer.
          */
         smoothPrice[smoothPrice_Idx] = smoothedValue;
         if( today % 2 == 0 ) {
            /* Do the Hilbert Transforms for even price bar */
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Even[hilbertIdx];
            detrender_Even[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Even;
            prev_detrender_Even = b * prev_detrender_input_Even;
            detrender += prev_detrender_Even;
            prev_detrender_input_Even = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Even[hilbertIdx];
            Q1_Even[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Even;
            prev_Q1_Even = b * prev_Q1_input_Even;
            Q1 += prev_Q1_Even;
            prev_Q1_input_Even = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForEvenPrev3;
            jI = 0 - jI_Even[hilbertIdx];
            jI_Even[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Even;
            prev_jI_Even = b * prev_jI_input_Even;
            jI += prev_jI_Even;
            prev_jI_input_Even = I1ForEvenPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Even[hilbertIdx];
            jQ_Even[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Even;
            prev_jQ_Even = b * prev_jQ_input_Even;
            jQ += prev_jQ_Even;
            prev_jQ_input_Even = Q1;
            jQ *= adjustedPrevPeriod;
            if( ++hilbertIdx == 3 ) {
               hilbertIdx = 0;
            }
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForEvenPrev3 - jQ, 0.8 * prevI2);
            /* The variable I1 is the detrender delayed for
             * 3 price bars.
             *
             * Save the current detrender value for being
             * used by the "odd" logic later.
             */
            I1ForOddPrev3 = I1ForOddPrev2;
            I1ForOddPrev2 = detrender;
         } else {
            /* Do the Hilbert Transforms for odd price bar */
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Odd[hilbertIdx];
            detrender_Odd[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Odd;
            prev_detrender_Odd = b * prev_detrender_input_Odd;
            detrender += prev_detrender_Odd;
            prev_detrender_input_Odd = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Odd[hilbertIdx];
            Q1_Odd[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Odd;
            prev_Q1_Odd = b * prev_Q1_input_Odd;
            Q1 += prev_Q1_Odd;
            prev_Q1_input_Odd = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForOddPrev3;
            jI = 0 - jI_Odd[hilbertIdx];
            jI_Odd[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Odd;
            prev_jI_Odd = b * prev_jI_input_Odd;
            jI += prev_jI_Odd;
            prev_jI_input_Odd = I1ForOddPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Odd[hilbertIdx];
            jQ_Odd[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Odd;
            prev_jQ_Odd = b * prev_jQ_input_Odd;
            jQ += prev_jQ_Odd;
            prev_jQ_input_Odd = Q1;
            jQ *= adjustedPrevPeriod;
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
            /* The varaiable I1 is the detrender delayed for
             * 3 price bars.
             *
             * Save the current detrender value for being
             * used by the "even" logic later.
             */
            I1ForEvenPrev3 = I1ForEvenPrev2;
            I1ForEvenPrev2 = detrender;
         }
         /* Adjust the period for next price bar */
         Re = Math.fma(0.8, Re, 0.2 * (Math.fma(I2, prevI2, Q2 * prevQ2)));
         Im = Math.fma(0.8, Im, 0.2 * (I2 * prevQ2 - Q2 * prevI2));
         prevQ2 = Q2;
         prevI2 = I2;
         tempReal = period;
         if( Im != 0.0 && Re != 0.0 ) {
            period = 360.0 / (Math.atan(Im / Re) * rad2Deg);
         }
         tempReal2 = 1.5 * tempReal;
         if( period > tempReal2 ) {
            period = tempReal2;
         }
         tempReal2 = 0.67 * tempReal;
         if( period < tempReal2 ) {
            period = tempReal2;
         }
         if( period < 6 ) {
            period = 6;
         } else if( period > 50 ) {
            period = 50;
         }
         period = Math.fma(0.2, period, 0.8 * tempReal);
         smoothPeriod = Math.fma(0.67, smoothPeriod, 0.33 * period);
         /* Compute Dominant Cycle Phase */
         DCPeriod = smoothPeriod + 0.5;
         DCPeriodInt = (int)DCPeriod;
         realPart = 0.0;
         imagPart = 0.0;
         /* idx is used to iterate for up to 50 of the last
          * value of smoothPrice.
          */
         idx = smoothPrice_Idx;
         for( i = 0; i < DCPeriodInt; i += 1 ) {
            tempReal = (double)i * constDeg2RadBy360 / (double)DCPeriodInt;
            tempReal2 = smoothPrice[idx];
            realPart += Math.sin(tempReal) * tempReal2;
            imagPart += Math.cos(tempReal) * tempReal2;
            if( idx == 0 ) {
               idx = 50 - 1;
            } else {
               idx -= 1;
            }
         }
         tempReal = Math.abs(imagPart);
         if( tempReal > 0.0 ) {
            DCPhase = Math.atan(realPart / imagPart) * rad2Deg;
         } else if( tempReal <= 0.01 ) {
            if( realPart < 0.0 ) {
               DCPhase -= 90.0;
            } else if( realPart > 0.0 ) {
               DCPhase += 90.0;
            }
         }
         DCPhase += 90.0;
         /* Compensate for one bar lag of the weighted moving average */
         DCPhase += 360.0 / smoothPeriod;
         if( imagPart < 0.0 ) {
            DCPhase += 180.0;
         }
         if( DCPhase > 315.0 ) {
            DCPhase -= 360.0;
         }
         if( today >= startIdx ) {
            outSine[outIdx] = Math.sin(DCPhase * deg2Rad);
            outLeadSine[outIdx++] = Math.sin((DCPhase + 45) * deg2Rad);
         }
         /* Ooof... let's do the next price bar now! */
         smoothPrice_Idx++;
         if( smoothPrice_Idx > maxIdx_smoothPrice ) { smoothPrice_Idx = 0; }
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode htSineUnguarded( int startIdx,
                                   int endIdx,
                                   double inReal[],
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outSine[],
                                   double outLeadSine[] )
   {
      int outIdx = 0;
      int i = 0;
      int lookbackTotal = 0;
      int today = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double adjustedPrevPeriod = 0;
      double period = 0;
      int trailingWMAIdx = 0;
      double periodWMASum = 0;
      double periodWMASub = 0;
      double trailingWMAValue = 0;
      double smoothedValue = 0;
      double a = 0;
      double b = 0;
      double hilbertTempReal = 0;
      int hilbertIdx = 0;
      double[] detrender_Odd = new double[3];
      double[] detrender_Even = new double[3];
      double detrender = 0;
      double prev_detrender_Odd = 0;
      double prev_detrender_Even = 0;
      double prev_detrender_input_Odd = 0;
      double prev_detrender_input_Even = 0;
      double[] Q1_Odd = new double[3];
      double[] Q1_Even = new double[3];
      double Q1 = 0;
      double prev_Q1_Odd = 0;
      double prev_Q1_Even = 0;
      double prev_Q1_input_Odd = 0;
      double prev_Q1_input_Even = 0;
      double[] jI_Odd = new double[3];
      double[] jI_Even = new double[3];
      double jI = 0;
      double prev_jI_Odd = 0;
      double prev_jI_Even = 0;
      double prev_jI_input_Odd = 0;
      double prev_jI_input_Even = 0;
      double[] jQ_Odd = new double[3];
      double[] jQ_Even = new double[3];
      double jQ = 0;
      double prev_jQ_Odd = 0;
      double prev_jQ_Even = 0;
      double prev_jQ_input_Odd = 0;
      double prev_jQ_input_Even = 0;
      double Q2 = 0;
      double I2 = 0;
      double prevQ2 = 0;
      double prevI2 = 0;
      double Re = 0;
      double Im = 0;
      double I1ForOddPrev2 = 0;
      double I1ForOddPrev3 = 0;
      double I1ForEvenPrev2 = 0;
      double I1ForEvenPrev3 = 0;
      double rad2Deg = 0;
      double deg2Rad = 0;
      double constDeg2RadBy360 = 0;
      double todayValue = 0;
      double smoothPeriod = 0;
      double[] smoothPrice;
      int smoothPrice_Idx = 0;
      int maxIdx_smoothPrice = (50)-1;
      int idx = 0;
      int DCPeriodInt = 0;
      double DCPhase = 0;
      double DCPeriod = 0;
      double imagPart = 0;
      double realPart = 0;
      a = 0.0962;
      b = 0.5769;
      smoothPrice = new double[maxIdx_smoothPrice+1];
      tempReal = Math.atan(1);
      rad2Deg = 45.0 / tempReal;
      deg2Rad = 1.0 / rad2Deg;
      constDeg2RadBy360 = tempReal * 8.0;
      lookbackTotal = 63 + this.unstablePeriod[FuncUnstId.HtSine.ordinal()];
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingWMAIdx = startIdx - lookbackTotal;
      today = trailingWMAIdx;
      tempReal = inReal[today++];
      periodWMASub = tempReal;
      periodWMASum = tempReal;
      tempReal = inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 2.0;
      tempReal = inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 3.0;
      trailingWMAValue = 0.0;
      i = 34;
      do {
         tempReal = inReal[today++];
         periodWMASub += tempReal;
         periodWMASub -= trailingWMAValue;
         periodWMASum += tempReal * 4.0;
         trailingWMAValue = inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
      } while( --i != 0 );
      hilbertIdx = 0;
      detrender_Odd[0] = 0.0;
      detrender_Odd[1] = 0.0;
      detrender_Odd[2] = 0.0;
      detrender_Even[0] = 0.0;
      detrender_Even[1] = 0.0;
      detrender_Even[2] = 0.0;
      detrender = 0.0;
      prev_detrender_Odd = 0.0;
      prev_detrender_Even = 0.0;
      prev_detrender_input_Odd = 0.0;
      prev_detrender_input_Even = 0.0;
      Q1_Odd[0] = 0.0;
      Q1_Odd[1] = 0.0;
      Q1_Odd[2] = 0.0;
      Q1_Even[0] = 0.0;
      Q1_Even[1] = 0.0;
      Q1_Even[2] = 0.0;
      Q1 = 0.0;
      prev_Q1_Odd = 0.0;
      prev_Q1_Even = 0.0;
      prev_Q1_input_Odd = 0.0;
      prev_Q1_input_Even = 0.0;
      jI_Odd[0] = 0.0;
      jI_Odd[1] = 0.0;
      jI_Odd[2] = 0.0;
      jI_Even[0] = 0.0;
      jI_Even[1] = 0.0;
      jI_Even[2] = 0.0;
      jI = 0.0;
      prev_jI_Odd = 0.0;
      prev_jI_Even = 0.0;
      prev_jI_input_Odd = 0.0;
      prev_jI_input_Even = 0.0;
      jQ_Odd[0] = 0.0;
      jQ_Odd[1] = 0.0;
      jQ_Odd[2] = 0.0;
      jQ_Even[0] = 0.0;
      jQ_Even[1] = 0.0;
      jQ_Even[2] = 0.0;
      jQ = 0.0;
      prev_jQ_Odd = 0.0;
      prev_jQ_Even = 0.0;
      prev_jQ_input_Odd = 0.0;
      prev_jQ_input_Even = 0.0;
      period = 0.0;
      outIdx = 0;
      prevQ2 = 0.0;
      prevI2 = prevQ2;
      Im = 0.0;
      Re = Im;
      I1ForEvenPrev3 = 0.0;
      I1ForOddPrev3 = I1ForEvenPrev3;
      I1ForEvenPrev2 = 0.0;
      I1ForOddPrev2 = I1ForEvenPrev2;
      smoothPeriod = 0.0;
      for( i = 0; i < 50; i += 1 ) {
         smoothPrice[i] = 0.0;
      }
      DCPhase = 0.0;
      while( today <= endIdx ) {
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
         todayValue = inReal[today];
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
         smoothPrice[smoothPrice_Idx] = smoothedValue;
         if( today % 2 == 0 ) {
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Even[hilbertIdx];
            detrender_Even[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Even;
            prev_detrender_Even = b * prev_detrender_input_Even;
            detrender += prev_detrender_Even;
            prev_detrender_input_Even = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Even[hilbertIdx];
            Q1_Even[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Even;
            prev_Q1_Even = b * prev_Q1_input_Even;
            Q1 += prev_Q1_Even;
            prev_Q1_input_Even = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForEvenPrev3;
            jI = 0 - jI_Even[hilbertIdx];
            jI_Even[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Even;
            prev_jI_Even = b * prev_jI_input_Even;
            jI += prev_jI_Even;
            prev_jI_input_Even = I1ForEvenPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Even[hilbertIdx];
            jQ_Even[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Even;
            prev_jQ_Even = b * prev_jQ_input_Even;
            jQ += prev_jQ_Even;
            prev_jQ_input_Even = Q1;
            jQ *= adjustedPrevPeriod;
            if( ++hilbertIdx == 3 ) {
               hilbertIdx = 0;
            }
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForEvenPrev3 - jQ, 0.8 * prevI2);
            I1ForOddPrev3 = I1ForOddPrev2;
            I1ForOddPrev2 = detrender;
         } else {
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Odd[hilbertIdx];
            detrender_Odd[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Odd;
            prev_detrender_Odd = b * prev_detrender_input_Odd;
            detrender += prev_detrender_Odd;
            prev_detrender_input_Odd = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Odd[hilbertIdx];
            Q1_Odd[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Odd;
            prev_Q1_Odd = b * prev_Q1_input_Odd;
            Q1 += prev_Q1_Odd;
            prev_Q1_input_Odd = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForOddPrev3;
            jI = 0 - jI_Odd[hilbertIdx];
            jI_Odd[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Odd;
            prev_jI_Odd = b * prev_jI_input_Odd;
            jI += prev_jI_Odd;
            prev_jI_input_Odd = I1ForOddPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Odd[hilbertIdx];
            jQ_Odd[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Odd;
            prev_jQ_Odd = b * prev_jQ_input_Odd;
            jQ += prev_jQ_Odd;
            prev_jQ_input_Odd = Q1;
            jQ *= adjustedPrevPeriod;
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
            I1ForEvenPrev3 = I1ForEvenPrev2;
            I1ForEvenPrev2 = detrender;
         }
         Re = Math.fma(0.8, Re, 0.2 * (Math.fma(I2, prevI2, Q2 * prevQ2)));
         Im = Math.fma(0.8, Im, 0.2 * (I2 * prevQ2 - Q2 * prevI2));
         prevQ2 = Q2;
         prevI2 = I2;
         tempReal = period;
         if( Im != 0.0 && Re != 0.0 ) {
            period = 360.0 / (Math.atan(Im / Re) * rad2Deg);
         }
         tempReal2 = 1.5 * tempReal;
         if( period > tempReal2 ) {
            period = tempReal2;
         }
         tempReal2 = 0.67 * tempReal;
         if( period < tempReal2 ) {
            period = tempReal2;
         }
         if( period < 6 ) {
            period = 6;
         } else if( period > 50 ) {
            period = 50;
         }
         period = Math.fma(0.2, period, 0.8 * tempReal);
         smoothPeriod = Math.fma(0.67, smoothPeriod, 0.33 * period);
         DCPeriod = smoothPeriod + 0.5;
         DCPeriodInt = (int)DCPeriod;
         realPart = 0.0;
         imagPart = 0.0;
         idx = smoothPrice_Idx;
         for( i = 0; i < DCPeriodInt; i += 1 ) {
            tempReal = (double)i * constDeg2RadBy360 / (double)DCPeriodInt;
            tempReal2 = smoothPrice[idx];
            realPart += Math.sin(tempReal) * tempReal2;
            imagPart += Math.cos(tempReal) * tempReal2;
            if( idx == 0 ) {
               idx = 50 - 1;
            } else {
               idx -= 1;
            }
         }
         tempReal = Math.abs(imagPart);
         if( tempReal > 0.0 ) {
            DCPhase = Math.atan(realPart / imagPart) * rad2Deg;
         } else if( tempReal <= 0.01 ) {
            if( realPart < 0.0 ) {
               DCPhase -= 90.0;
            } else if( realPart > 0.0 ) {
               DCPhase += 90.0;
            }
         }
         DCPhase += 90.0;
         DCPhase += 360.0 / smoothPeriod;
         if( imagPart < 0.0 ) {
            DCPhase += 180.0;
         }
         if( DCPhase > 315.0 ) {
            DCPhase -= 360.0;
         }
         if( today >= startIdx ) {
            outSine[outIdx] = Math.sin(DCPhase * deg2Rad);
            outLeadSine[outIdx++] = Math.sin((DCPhase + 45) * deg2Rad);
         }
         smoothPrice_Idx++;
         if( smoothPrice_Idx > maxIdx_smoothPrice ) { smoothPrice_Idx = 0; }
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode htSine( int startIdx,
                          int endIdx,
                          float inReal[],
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outSine[],
                          double outLeadSine[] )
   {
      int outIdx = 0;
      int i = 0;
      int lookbackTotal = 0;
      int today = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double adjustedPrevPeriod = 0;
      double period = 0;
      int trailingWMAIdx = 0;
      double periodWMASum = 0;
      double periodWMASub = 0;
      double trailingWMAValue = 0;
      double smoothedValue = 0;
      double a = 0;
      double b = 0;
      double hilbertTempReal = 0;
      int hilbertIdx = 0;
      double[] detrender_Odd = new double[3];
      double[] detrender_Even = new double[3];
      double detrender = 0;
      double prev_detrender_Odd = 0;
      double prev_detrender_Even = 0;
      double prev_detrender_input_Odd = 0;
      double prev_detrender_input_Even = 0;
      double[] Q1_Odd = new double[3];
      double[] Q1_Even = new double[3];
      double Q1 = 0;
      double prev_Q1_Odd = 0;
      double prev_Q1_Even = 0;
      double prev_Q1_input_Odd = 0;
      double prev_Q1_input_Even = 0;
      double[] jI_Odd = new double[3];
      double[] jI_Even = new double[3];
      double jI = 0;
      double prev_jI_Odd = 0;
      double prev_jI_Even = 0;
      double prev_jI_input_Odd = 0;
      double prev_jI_input_Even = 0;
      double[] jQ_Odd = new double[3];
      double[] jQ_Even = new double[3];
      double jQ = 0;
      double prev_jQ_Odd = 0;
      double prev_jQ_Even = 0;
      double prev_jQ_input_Odd = 0;
      double prev_jQ_input_Even = 0;
      double Q2 = 0;
      double I2 = 0;
      double prevQ2 = 0;
      double prevI2 = 0;
      double Re = 0;
      double Im = 0;
      double I1ForOddPrev2 = 0;
      double I1ForOddPrev3 = 0;
      double I1ForEvenPrev2 = 0;
      double I1ForEvenPrev3 = 0;
      double rad2Deg = 0;
      double deg2Rad = 0;
      double constDeg2RadBy360 = 0;
      double todayValue = 0;
      double smoothPeriod = 0;
      double[] smoothPrice;
      int smoothPrice_Idx = 0;
      int maxIdx_smoothPrice = (50)-1;
      int idx = 0;
      int DCPeriodInt = 0;
      double DCPhase = 0;
      double DCPeriod = 0;
      double imagPart = 0;
      double realPart = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( outSine == outLeadSine ) {
         return RetCode.BadParam ;
      }
      a = 0.0962;
      b = 0.5769;
      smoothPrice = new double[maxIdx_smoothPrice+1];
      tempReal = Math.atan(1);
      rad2Deg = 45.0 / tempReal;
      deg2Rad = 1.0 / rad2Deg;
      constDeg2RadBy360 = tempReal * 8.0;
      lookbackTotal = 63 + this.unstablePeriod[FuncUnstId.HtSine.ordinal()];
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingWMAIdx = startIdx - lookbackTotal;
      today = trailingWMAIdx;
      tempReal = (double)inReal[today++];
      periodWMASub = tempReal;
      periodWMASum = tempReal;
      tempReal = (double)inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 2.0;
      tempReal = (double)inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 3.0;
      trailingWMAValue = 0.0;
      i = 34;
      do {
         tempReal = (double)inReal[today++];
         periodWMASub += tempReal;
         periodWMASub -= trailingWMAValue;
         periodWMASum += tempReal * 4.0;
         trailingWMAValue = (double)inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
      } while( --i != 0 );
      hilbertIdx = 0;
      detrender_Odd[0] = 0.0;
      detrender_Odd[1] = 0.0;
      detrender_Odd[2] = 0.0;
      detrender_Even[0] = 0.0;
      detrender_Even[1] = 0.0;
      detrender_Even[2] = 0.0;
      detrender = 0.0;
      prev_detrender_Odd = 0.0;
      prev_detrender_Even = 0.0;
      prev_detrender_input_Odd = 0.0;
      prev_detrender_input_Even = 0.0;
      Q1_Odd[0] = 0.0;
      Q1_Odd[1] = 0.0;
      Q1_Odd[2] = 0.0;
      Q1_Even[0] = 0.0;
      Q1_Even[1] = 0.0;
      Q1_Even[2] = 0.0;
      Q1 = 0.0;
      prev_Q1_Odd = 0.0;
      prev_Q1_Even = 0.0;
      prev_Q1_input_Odd = 0.0;
      prev_Q1_input_Even = 0.0;
      jI_Odd[0] = 0.0;
      jI_Odd[1] = 0.0;
      jI_Odd[2] = 0.0;
      jI_Even[0] = 0.0;
      jI_Even[1] = 0.0;
      jI_Even[2] = 0.0;
      jI = 0.0;
      prev_jI_Odd = 0.0;
      prev_jI_Even = 0.0;
      prev_jI_input_Odd = 0.0;
      prev_jI_input_Even = 0.0;
      jQ_Odd[0] = 0.0;
      jQ_Odd[1] = 0.0;
      jQ_Odd[2] = 0.0;
      jQ_Even[0] = 0.0;
      jQ_Even[1] = 0.0;
      jQ_Even[2] = 0.0;
      jQ = 0.0;
      prev_jQ_Odd = 0.0;
      prev_jQ_Even = 0.0;
      prev_jQ_input_Odd = 0.0;
      prev_jQ_input_Even = 0.0;
      period = 0.0;
      outIdx = 0;
      prevQ2 = 0.0;
      prevI2 = prevQ2;
      Im = 0.0;
      Re = Im;
      I1ForEvenPrev3 = 0.0;
      I1ForOddPrev3 = I1ForEvenPrev3;
      I1ForEvenPrev2 = 0.0;
      I1ForOddPrev2 = I1ForEvenPrev2;
      smoothPeriod = 0.0;
      for( i = 0; i < 50; i += 1 ) {
         smoothPrice[i] = 0.0;
      }
      DCPhase = 0.0;
      while( today <= endIdx ) {
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
         todayValue = (double)inReal[today];
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = (double)inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
         smoothPrice[smoothPrice_Idx] = smoothedValue;
         if( today % 2 == 0 ) {
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Even[hilbertIdx];
            detrender_Even[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Even;
            prev_detrender_Even = b * prev_detrender_input_Even;
            detrender += prev_detrender_Even;
            prev_detrender_input_Even = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Even[hilbertIdx];
            Q1_Even[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Even;
            prev_Q1_Even = b * prev_Q1_input_Even;
            Q1 += prev_Q1_Even;
            prev_Q1_input_Even = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForEvenPrev3;
            jI = 0 - jI_Even[hilbertIdx];
            jI_Even[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Even;
            prev_jI_Even = b * prev_jI_input_Even;
            jI += prev_jI_Even;
            prev_jI_input_Even = I1ForEvenPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Even[hilbertIdx];
            jQ_Even[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Even;
            prev_jQ_Even = b * prev_jQ_input_Even;
            jQ += prev_jQ_Even;
            prev_jQ_input_Even = Q1;
            jQ *= adjustedPrevPeriod;
            if( ++hilbertIdx == 3 ) {
               hilbertIdx = 0;
            }
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForEvenPrev3 - jQ, 0.8 * prevI2);
            I1ForOddPrev3 = I1ForOddPrev2;
            I1ForOddPrev2 = detrender;
         } else {
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Odd[hilbertIdx];
            detrender_Odd[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Odd;
            prev_detrender_Odd = b * prev_detrender_input_Odd;
            detrender += prev_detrender_Odd;
            prev_detrender_input_Odd = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Odd[hilbertIdx];
            Q1_Odd[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Odd;
            prev_Q1_Odd = b * prev_Q1_input_Odd;
            Q1 += prev_Q1_Odd;
            prev_Q1_input_Odd = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForOddPrev3;
            jI = 0 - jI_Odd[hilbertIdx];
            jI_Odd[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Odd;
            prev_jI_Odd = b * prev_jI_input_Odd;
            jI += prev_jI_Odd;
            prev_jI_input_Odd = I1ForOddPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Odd[hilbertIdx];
            jQ_Odd[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Odd;
            prev_jQ_Odd = b * prev_jQ_input_Odd;
            jQ += prev_jQ_Odd;
            prev_jQ_input_Odd = Q1;
            jQ *= adjustedPrevPeriod;
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
            I1ForEvenPrev3 = I1ForEvenPrev2;
            I1ForEvenPrev2 = detrender;
         }
         Re = Math.fma(0.8, Re, 0.2 * (Math.fma(I2, prevI2, Q2 * prevQ2)));
         Im = Math.fma(0.8, Im, 0.2 * (I2 * prevQ2 - Q2 * prevI2));
         prevQ2 = Q2;
         prevI2 = I2;
         tempReal = period;
         if( Im != 0.0 && Re != 0.0 ) {
            period = 360.0 / (Math.atan(Im / Re) * rad2Deg);
         }
         tempReal2 = 1.5 * tempReal;
         if( period > tempReal2 ) {
            period = tempReal2;
         }
         tempReal2 = 0.67 * tempReal;
         if( period < tempReal2 ) {
            period = tempReal2;
         }
         if( period < 6 ) {
            period = 6;
         } else if( period > 50 ) {
            period = 50;
         }
         period = Math.fma(0.2, period, 0.8 * tempReal);
         smoothPeriod = Math.fma(0.67, smoothPeriod, 0.33 * period);
         DCPeriod = smoothPeriod + 0.5;
         DCPeriodInt = (int)DCPeriod;
         realPart = 0.0;
         imagPart = 0.0;
         idx = smoothPrice_Idx;
         for( i = 0; i < DCPeriodInt; i += 1 ) {
            tempReal = (double)i * constDeg2RadBy360 / (double)DCPeriodInt;
            tempReal2 = smoothPrice[idx];
            realPart += Math.sin(tempReal) * tempReal2;
            imagPart += Math.cos(tempReal) * tempReal2;
            if( idx == 0 ) {
               idx = 50 - 1;
            } else {
               idx -= 1;
            }
         }
         tempReal = Math.abs(imagPart);
         if( tempReal > 0.0 ) {
            DCPhase = Math.atan(realPart / imagPart) * rad2Deg;
         } else if( tempReal <= 0.01 ) {
            if( realPart < 0.0 ) {
               DCPhase -= 90.0;
            } else if( realPart > 0.0 ) {
               DCPhase += 90.0;
            }
         }
         DCPhase += 90.0;
         DCPhase += 360.0 / smoothPeriod;
         if( imagPart < 0.0 ) {
            DCPhase += 180.0;
         }
         if( DCPhase > 315.0 ) {
            DCPhase -= 360.0;
         }
         if( today >= startIdx ) {
            outSine[outIdx] = Math.sin(DCPhase * deg2Rad);
            outLeadSine[outIdx++] = Math.sin((DCPhase + 45) * deg2Rad);
         }
         smoothPrice_Idx++;
         if( smoothPrice_Idx > maxIdx_smoothPrice ) { smoothPrice_Idx = 0; }
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode htSineUnguarded( int startIdx,
                                   int endIdx,
                                   float inReal[],
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outSine[],
                                   double outLeadSine[] )
   {
      int outIdx = 0;
      int i = 0;
      int lookbackTotal = 0;
      int today = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double adjustedPrevPeriod = 0;
      double period = 0;
      int trailingWMAIdx = 0;
      double periodWMASum = 0;
      double periodWMASub = 0;
      double trailingWMAValue = 0;
      double smoothedValue = 0;
      double a = 0;
      double b = 0;
      double hilbertTempReal = 0;
      int hilbertIdx = 0;
      double[] detrender_Odd = new double[3];
      double[] detrender_Even = new double[3];
      double detrender = 0;
      double prev_detrender_Odd = 0;
      double prev_detrender_Even = 0;
      double prev_detrender_input_Odd = 0;
      double prev_detrender_input_Even = 0;
      double[] Q1_Odd = new double[3];
      double[] Q1_Even = new double[3];
      double Q1 = 0;
      double prev_Q1_Odd = 0;
      double prev_Q1_Even = 0;
      double prev_Q1_input_Odd = 0;
      double prev_Q1_input_Even = 0;
      double[] jI_Odd = new double[3];
      double[] jI_Even = new double[3];
      double jI = 0;
      double prev_jI_Odd = 0;
      double prev_jI_Even = 0;
      double prev_jI_input_Odd = 0;
      double prev_jI_input_Even = 0;
      double[] jQ_Odd = new double[3];
      double[] jQ_Even = new double[3];
      double jQ = 0;
      double prev_jQ_Odd = 0;
      double prev_jQ_Even = 0;
      double prev_jQ_input_Odd = 0;
      double prev_jQ_input_Even = 0;
      double Q2 = 0;
      double I2 = 0;
      double prevQ2 = 0;
      double prevI2 = 0;
      double Re = 0;
      double Im = 0;
      double I1ForOddPrev2 = 0;
      double I1ForOddPrev3 = 0;
      double I1ForEvenPrev2 = 0;
      double I1ForEvenPrev3 = 0;
      double rad2Deg = 0;
      double deg2Rad = 0;
      double constDeg2RadBy360 = 0;
      double todayValue = 0;
      double smoothPeriod = 0;
      double[] smoothPrice;
      int smoothPrice_Idx = 0;
      int maxIdx_smoothPrice = (50)-1;
      int idx = 0;
      int DCPeriodInt = 0;
      double DCPhase = 0;
      double DCPeriod = 0;
      double imagPart = 0;
      double realPart = 0;
      a = 0.0962;
      b = 0.5769;
      smoothPrice = new double[maxIdx_smoothPrice+1];
      tempReal = Math.atan(1);
      rad2Deg = 45.0 / tempReal;
      deg2Rad = 1.0 / rad2Deg;
      constDeg2RadBy360 = tempReal * 8.0;
      lookbackTotal = 63 + this.unstablePeriod[FuncUnstId.HtSine.ordinal()];
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingWMAIdx = startIdx - lookbackTotal;
      today = trailingWMAIdx;
      tempReal = (double)inReal[today++];
      periodWMASub = tempReal;
      periodWMASum = tempReal;
      tempReal = (double)inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 2.0;
      tempReal = (double)inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 3.0;
      trailingWMAValue = 0.0;
      i = 34;
      do {
         tempReal = (double)inReal[today++];
         periodWMASub += tempReal;
         periodWMASub -= trailingWMAValue;
         periodWMASum += tempReal * 4.0;
         trailingWMAValue = (double)inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
      } while( --i != 0 );
      hilbertIdx = 0;
      detrender_Odd[0] = 0.0;
      detrender_Odd[1] = 0.0;
      detrender_Odd[2] = 0.0;
      detrender_Even[0] = 0.0;
      detrender_Even[1] = 0.0;
      detrender_Even[2] = 0.0;
      detrender = 0.0;
      prev_detrender_Odd = 0.0;
      prev_detrender_Even = 0.0;
      prev_detrender_input_Odd = 0.0;
      prev_detrender_input_Even = 0.0;
      Q1_Odd[0] = 0.0;
      Q1_Odd[1] = 0.0;
      Q1_Odd[2] = 0.0;
      Q1_Even[0] = 0.0;
      Q1_Even[1] = 0.0;
      Q1_Even[2] = 0.0;
      Q1 = 0.0;
      prev_Q1_Odd = 0.0;
      prev_Q1_Even = 0.0;
      prev_Q1_input_Odd = 0.0;
      prev_Q1_input_Even = 0.0;
      jI_Odd[0] = 0.0;
      jI_Odd[1] = 0.0;
      jI_Odd[2] = 0.0;
      jI_Even[0] = 0.0;
      jI_Even[1] = 0.0;
      jI_Even[2] = 0.0;
      jI = 0.0;
      prev_jI_Odd = 0.0;
      prev_jI_Even = 0.0;
      prev_jI_input_Odd = 0.0;
      prev_jI_input_Even = 0.0;
      jQ_Odd[0] = 0.0;
      jQ_Odd[1] = 0.0;
      jQ_Odd[2] = 0.0;
      jQ_Even[0] = 0.0;
      jQ_Even[1] = 0.0;
      jQ_Even[2] = 0.0;
      jQ = 0.0;
      prev_jQ_Odd = 0.0;
      prev_jQ_Even = 0.0;
      prev_jQ_input_Odd = 0.0;
      prev_jQ_input_Even = 0.0;
      period = 0.0;
      outIdx = 0;
      prevQ2 = 0.0;
      prevI2 = prevQ2;
      Im = 0.0;
      Re = Im;
      I1ForEvenPrev3 = 0.0;
      I1ForOddPrev3 = I1ForEvenPrev3;
      I1ForEvenPrev2 = 0.0;
      I1ForOddPrev2 = I1ForEvenPrev2;
      smoothPeriod = 0.0;
      for( i = 0; i < 50; i += 1 ) {
         smoothPrice[i] = 0.0;
      }
      DCPhase = 0.0;
      while( today <= endIdx ) {
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
         todayValue = (double)inReal[today];
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = (double)inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
         smoothPrice[smoothPrice_Idx] = smoothedValue;
         if( today % 2 == 0 ) {
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Even[hilbertIdx];
            detrender_Even[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Even;
            prev_detrender_Even = b * prev_detrender_input_Even;
            detrender += prev_detrender_Even;
            prev_detrender_input_Even = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Even[hilbertIdx];
            Q1_Even[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Even;
            prev_Q1_Even = b * prev_Q1_input_Even;
            Q1 += prev_Q1_Even;
            prev_Q1_input_Even = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForEvenPrev3;
            jI = 0 - jI_Even[hilbertIdx];
            jI_Even[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Even;
            prev_jI_Even = b * prev_jI_input_Even;
            jI += prev_jI_Even;
            prev_jI_input_Even = I1ForEvenPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Even[hilbertIdx];
            jQ_Even[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Even;
            prev_jQ_Even = b * prev_jQ_input_Even;
            jQ += prev_jQ_Even;
            prev_jQ_input_Even = Q1;
            jQ *= adjustedPrevPeriod;
            if( ++hilbertIdx == 3 ) {
               hilbertIdx = 0;
            }
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForEvenPrev3 - jQ, 0.8 * prevI2);
            I1ForOddPrev3 = I1ForOddPrev2;
            I1ForOddPrev2 = detrender;
         } else {
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Odd[hilbertIdx];
            detrender_Odd[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Odd;
            prev_detrender_Odd = b * prev_detrender_input_Odd;
            detrender += prev_detrender_Odd;
            prev_detrender_input_Odd = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Odd[hilbertIdx];
            Q1_Odd[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Odd;
            prev_Q1_Odd = b * prev_Q1_input_Odd;
            Q1 += prev_Q1_Odd;
            prev_Q1_input_Odd = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForOddPrev3;
            jI = 0 - jI_Odd[hilbertIdx];
            jI_Odd[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Odd;
            prev_jI_Odd = b * prev_jI_input_Odd;
            jI += prev_jI_Odd;
            prev_jI_input_Odd = I1ForOddPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Odd[hilbertIdx];
            jQ_Odd[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Odd;
            prev_jQ_Odd = b * prev_jQ_input_Odd;
            jQ += prev_jQ_Odd;
            prev_jQ_input_Odd = Q1;
            jQ *= adjustedPrevPeriod;
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
            I1ForEvenPrev3 = I1ForEvenPrev2;
            I1ForEvenPrev2 = detrender;
         }
         Re = Math.fma(0.8, Re, 0.2 * (Math.fma(I2, prevI2, Q2 * prevQ2)));
         Im = Math.fma(0.8, Im, 0.2 * (I2 * prevQ2 - Q2 * prevI2));
         prevQ2 = Q2;
         prevI2 = I2;
         tempReal = period;
         if( Im != 0.0 && Re != 0.0 ) {
            period = 360.0 / (Math.atan(Im / Re) * rad2Deg);
         }
         tempReal2 = 1.5 * tempReal;
         if( period > tempReal2 ) {
            period = tempReal2;
         }
         tempReal2 = 0.67 * tempReal;
         if( period < tempReal2 ) {
            period = tempReal2;
         }
         if( period < 6 ) {
            period = 6;
         } else if( period > 50 ) {
            period = 50;
         }
         period = Math.fma(0.2, period, 0.8 * tempReal);
         smoothPeriod = Math.fma(0.67, smoothPeriod, 0.33 * period);
         DCPeriod = smoothPeriod + 0.5;
         DCPeriodInt = (int)DCPeriod;
         realPart = 0.0;
         imagPart = 0.0;
         idx = smoothPrice_Idx;
         for( i = 0; i < DCPeriodInt; i += 1 ) {
            tempReal = (double)i * constDeg2RadBy360 / (double)DCPeriodInt;
            tempReal2 = smoothPrice[idx];
            realPart += Math.sin(tempReal) * tempReal2;
            imagPart += Math.cos(tempReal) * tempReal2;
            if( idx == 0 ) {
               idx = 50 - 1;
            } else {
               idx -= 1;
            }
         }
         tempReal = Math.abs(imagPart);
         if( tempReal > 0.0 ) {
            DCPhase = Math.atan(realPart / imagPart) * rad2Deg;
         } else if( tempReal <= 0.01 ) {
            if( realPart < 0.0 ) {
               DCPhase -= 90.0;
            } else if( realPart > 0.0 ) {
               DCPhase += 90.0;
            }
         }
         DCPhase += 90.0;
         DCPhase += 360.0 / smoothPeriod;
         if( imagPart < 0.0 ) {
            DCPhase += 180.0;
         }
         if( DCPhase > 315.0 ) {
            DCPhase -= 360.0;
         }
         if( today >= startIdx ) {
            outSine[outIdx] = Math.sin(DCPhase * deg2Rad);
            outLeadSine[outIdx++] = Math.sin((DCPhase + 45) * deg2Rad);
         }
         smoothPrice_Idx++;
         if( smoothPrice_Idx > maxIdx_smoothPrice ) { smoothPrice_Idx = 0; }
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live HT_SINE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#htSine} over the same series.
    * Open with {@link Core#htSineOpen}; there is no close — the handle is
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
   public static final class HtSineStream {
      final Core core;
      int i;
      double tempReal;
      double tempReal2;
      double period;
      double periodWMASum;
      double periodWMASub;
      double trailingWMAValue;
      double smoothedValue;
      double a;
      double b;
      double hilbertTempReal;
      int hilbertIdx;
      double[] detrender_Odd;
      double[] detrender_Even;
      double detrender;
      double prev_detrender_Odd;
      double prev_detrender_Even;
      double prev_detrender_input_Odd;
      double prev_detrender_input_Even;
      double[] Q1_Odd;
      double[] Q1_Even;
      double Q1;
      double prev_Q1_Odd;
      double prev_Q1_Even;
      double prev_Q1_input_Odd;
      double prev_Q1_input_Even;
      double[] jI_Odd;
      double[] jI_Even;
      double jI;
      double prev_jI_Odd;
      double prev_jI_Even;
      double prev_jI_input_Odd;
      double prev_jI_input_Even;
      double[] jQ_Odd;
      double[] jQ_Even;
      double jQ;
      double prev_jQ_Odd;
      double prev_jQ_Even;
      double prev_jQ_input_Odd;
      double prev_jQ_input_Even;
      double Q2;
      double I2;
      double prevQ2;
      double prevI2;
      double Re;
      double Im;
      double I1ForOddPrev2;
      double I1ForOddPrev3;
      double I1ForEvenPrev2;
      double I1ForEvenPrev3;
      double rad2Deg;
      double deg2Rad;
      double constDeg2RadBy360;
      double smoothPeriod;
      int idx;
      int DCPeriodInt;
      double DCPhase;
      double DCPeriod;
      double imagPart;
      double realPart;
      int smoothPrice_Idx;
      int maxIdx_smoothPrice;
      int streamParity;
      int ringPos_trailingWMAIdx;
      int ringCap_trailingWMAIdx;
      double[] ring_trailingWMAIdx_inReal;
      int cbSize_smoothPrice;
      double[] cb_smoothPrice;
      double cur_outSine;
      double cur_outLeadSine;
      Value cachedValue;

      HtSineStream( Core core ) { this.core = core; }

      HtSineStream( HtSineStream other ) {
         this.core = other.core;
         this.i = other.i;
         this.tempReal = other.tempReal;
         this.tempReal2 = other.tempReal2;
         this.period = other.period;
         this.periodWMASum = other.periodWMASum;
         this.periodWMASub = other.periodWMASub;
         this.trailingWMAValue = other.trailingWMAValue;
         this.smoothedValue = other.smoothedValue;
         this.a = other.a;
         this.b = other.b;
         this.hilbertTempReal = other.hilbertTempReal;
         this.hilbertIdx = other.hilbertIdx;
         this.detrender_Odd = other.detrender_Odd.clone();
         this.detrender_Even = other.detrender_Even.clone();
         this.detrender = other.detrender;
         this.prev_detrender_Odd = other.prev_detrender_Odd;
         this.prev_detrender_Even = other.prev_detrender_Even;
         this.prev_detrender_input_Odd = other.prev_detrender_input_Odd;
         this.prev_detrender_input_Even = other.prev_detrender_input_Even;
         this.Q1_Odd = other.Q1_Odd.clone();
         this.Q1_Even = other.Q1_Even.clone();
         this.Q1 = other.Q1;
         this.prev_Q1_Odd = other.prev_Q1_Odd;
         this.prev_Q1_Even = other.prev_Q1_Even;
         this.prev_Q1_input_Odd = other.prev_Q1_input_Odd;
         this.prev_Q1_input_Even = other.prev_Q1_input_Even;
         this.jI_Odd = other.jI_Odd.clone();
         this.jI_Even = other.jI_Even.clone();
         this.jI = other.jI;
         this.prev_jI_Odd = other.prev_jI_Odd;
         this.prev_jI_Even = other.prev_jI_Even;
         this.prev_jI_input_Odd = other.prev_jI_input_Odd;
         this.prev_jI_input_Even = other.prev_jI_input_Even;
         this.jQ_Odd = other.jQ_Odd.clone();
         this.jQ_Even = other.jQ_Even.clone();
         this.jQ = other.jQ;
         this.prev_jQ_Odd = other.prev_jQ_Odd;
         this.prev_jQ_Even = other.prev_jQ_Even;
         this.prev_jQ_input_Odd = other.prev_jQ_input_Odd;
         this.prev_jQ_input_Even = other.prev_jQ_input_Even;
         this.Q2 = other.Q2;
         this.I2 = other.I2;
         this.prevQ2 = other.prevQ2;
         this.prevI2 = other.prevI2;
         this.Re = other.Re;
         this.Im = other.Im;
         this.I1ForOddPrev2 = other.I1ForOddPrev2;
         this.I1ForOddPrev3 = other.I1ForOddPrev3;
         this.I1ForEvenPrev2 = other.I1ForEvenPrev2;
         this.I1ForEvenPrev3 = other.I1ForEvenPrev3;
         this.rad2Deg = other.rad2Deg;
         this.deg2Rad = other.deg2Rad;
         this.constDeg2RadBy360 = other.constDeg2RadBy360;
         this.smoothPeriod = other.smoothPeriod;
         this.idx = other.idx;
         this.DCPeriodInt = other.DCPeriodInt;
         this.DCPhase = other.DCPhase;
         this.DCPeriod = other.DCPeriod;
         this.imagPart = other.imagPart;
         this.realPart = other.realPart;
         this.smoothPrice_Idx = other.smoothPrice_Idx;
         this.maxIdx_smoothPrice = other.maxIdx_smoothPrice;
         this.streamParity = other.streamParity;
         this.ringPos_trailingWMAIdx = other.ringPos_trailingWMAIdx;
         this.ringCap_trailingWMAIdx = other.ringCap_trailingWMAIdx;
         this.ring_trailingWMAIdx_inReal = other.ring_trailingWMAIdx_inReal.clone();
         this.cbSize_smoothPrice = other.cbSize_smoothPrice;
         this.cb_smoothPrice = other.cb_smoothPrice.clone();
         this.cur_outSine = other.cur_outSine;
         this.cur_outLeadSine = other.cur_outLeadSine;
         this.cachedValue = other.cachedValue;
      }

      /** One output set, in batch output order. Immutable. */
      public static final class Value {
         public final double sine;
         public final double leadSine;
         Value( double sine, double leadSine ) {
            this.sine = sine;
            this.leadSine = leadSine;
         }
         @Override public String toString() {
            return "Value[" + "sine=" + sine + ", " + "leadSine=" + leadSine + "]";
         }
         @Override public boolean equals( Object o ) {
            if( !(o instanceof Value) ) return false;
            Value v = (Value) o;
            return Double.doubleToLongBits(this.sine) == Double.doubleToLongBits(v.sine) && Double.doubleToLongBits(this.leadSine) == Double.doubleToLongBits(v.leadSine);
         }
         @Override public int hashCode() {
            int h = 17;
            h = 31 * h + Double.hashCode(sine);
            h = 31 * h + Double.hashCode(leadSine);
            return h;
         }
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inReal ) {
         core.htSineStreamStep(this, inReal);
         this.cachedValue = new Value(this.cur_outSine, this.cur_outLeadSine);
         return this.cachedValue;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public Value peek( double inReal ) {
         HtSineStream scratch = new HtSineStream(this);
         core.htSineStreamStep(scratch, inReal);
         return new Value(scratch.cur_outSine, scratch.cur_outLeadSine);
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public Value value() {
         return this.cachedValue;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public HtSineStream copy() {
         return new HtSineStream(this);
      }
   }
   void htSineStreamStep( HtSineStream sp, double inReal )
   {
      double adjustedPrevPeriod = 0.0;
      double todayValue = 0.0;
      if( sp.ringCap_trailingWMAIdx == 0 ) {
         sp.ring_trailingWMAIdx_inReal[0] = inReal;
      }
      adjustedPrevPeriod = Math.fma(0.075, sp.period, 0.54);
      todayValue = inReal;
      sp.periodWMASub += todayValue;
      sp.periodWMASub -= sp.trailingWMAValue;
      sp.periodWMASum += todayValue * 4.0;
      sp.trailingWMAValue = sp.ring_trailingWMAIdx_inReal[sp.ringPos_trailingWMAIdx];
      sp.smoothedValue = sp.periodWMASum * 0.1;
      sp.periodWMASum -= sp.periodWMASub;
      /* Remember the smoothedValue into the smoothPrice
       * circular buffer.
       */
      sp.cb_smoothPrice[sp.smoothPrice_Idx] = sp.smoothedValue;
      if( sp.streamParity == 0 ) {
         /* Do the Hilbert Transforms for even price bar */
         sp.hilbertTempReal = sp.a * sp.smoothedValue;
         sp.detrender = 0 - sp.detrender_Even[sp.hilbertIdx];
         sp.detrender_Even[sp.hilbertIdx] = sp.hilbertTempReal;
         sp.detrender += sp.hilbertTempReal;
         sp.detrender -= sp.prev_detrender_Even;
         sp.prev_detrender_Even = sp.b * sp.prev_detrender_input_Even;
         sp.detrender += sp.prev_detrender_Even;
         sp.prev_detrender_input_Even = sp.smoothedValue;
         sp.detrender *= adjustedPrevPeriod;
         sp.hilbertTempReal = sp.a * sp.detrender;
         sp.Q1 = 0 - sp.Q1_Even[sp.hilbertIdx];
         sp.Q1_Even[sp.hilbertIdx] = sp.hilbertTempReal;
         sp.Q1 += sp.hilbertTempReal;
         sp.Q1 -= sp.prev_Q1_Even;
         sp.prev_Q1_Even = sp.b * sp.prev_Q1_input_Even;
         sp.Q1 += sp.prev_Q1_Even;
         sp.prev_Q1_input_Even = sp.detrender;
         sp.Q1 *= adjustedPrevPeriod;
         sp.hilbertTempReal = sp.a * sp.I1ForEvenPrev3;
         sp.jI = 0 - sp.jI_Even[sp.hilbertIdx];
         sp.jI_Even[sp.hilbertIdx] = sp.hilbertTempReal;
         sp.jI += sp.hilbertTempReal;
         sp.jI -= sp.prev_jI_Even;
         sp.prev_jI_Even = sp.b * sp.prev_jI_input_Even;
         sp.jI += sp.prev_jI_Even;
         sp.prev_jI_input_Even = sp.I1ForEvenPrev3;
         sp.jI *= adjustedPrevPeriod;
         sp.hilbertTempReal = sp.a * sp.Q1;
         sp.jQ = 0 - sp.jQ_Even[sp.hilbertIdx];
         sp.jQ_Even[sp.hilbertIdx] = sp.hilbertTempReal;
         sp.jQ += sp.hilbertTempReal;
         sp.jQ -= sp.prev_jQ_Even;
         sp.prev_jQ_Even = sp.b * sp.prev_jQ_input_Even;
         sp.jQ += sp.prev_jQ_Even;
         sp.prev_jQ_input_Even = sp.Q1;
         sp.jQ *= adjustedPrevPeriod;
         if( ++sp.hilbertIdx == 3 ) {
            sp.hilbertIdx = 0;
         }
         sp.Q2 = Math.fma(0.2, sp.Q1 + sp.jI, 0.8 * sp.prevQ2);
         sp.I2 = Math.fma(0.2, sp.I1ForEvenPrev3 - sp.jQ, 0.8 * sp.prevI2);
         /* The variable I1 is the detrender delayed for
          * 3 price bars.
          *
          * Save the current detrender value for being
          * used by the "odd" logic later.
          */
         sp.I1ForOddPrev3 = sp.I1ForOddPrev2;
         sp.I1ForOddPrev2 = sp.detrender;
      } else {
         /* Do the Hilbert Transforms for odd price bar */
         sp.hilbertTempReal = sp.a * sp.smoothedValue;
         sp.detrender = 0 - sp.detrender_Odd[sp.hilbertIdx];
         sp.detrender_Odd[sp.hilbertIdx] = sp.hilbertTempReal;
         sp.detrender += sp.hilbertTempReal;
         sp.detrender -= sp.prev_detrender_Odd;
         sp.prev_detrender_Odd = sp.b * sp.prev_detrender_input_Odd;
         sp.detrender += sp.prev_detrender_Odd;
         sp.prev_detrender_input_Odd = sp.smoothedValue;
         sp.detrender *= adjustedPrevPeriod;
         sp.hilbertTempReal = sp.a * sp.detrender;
         sp.Q1 = 0 - sp.Q1_Odd[sp.hilbertIdx];
         sp.Q1_Odd[sp.hilbertIdx] = sp.hilbertTempReal;
         sp.Q1 += sp.hilbertTempReal;
         sp.Q1 -= sp.prev_Q1_Odd;
         sp.prev_Q1_Odd = sp.b * sp.prev_Q1_input_Odd;
         sp.Q1 += sp.prev_Q1_Odd;
         sp.prev_Q1_input_Odd = sp.detrender;
         sp.Q1 *= adjustedPrevPeriod;
         sp.hilbertTempReal = sp.a * sp.I1ForOddPrev3;
         sp.jI = 0 - sp.jI_Odd[sp.hilbertIdx];
         sp.jI_Odd[sp.hilbertIdx] = sp.hilbertTempReal;
         sp.jI += sp.hilbertTempReal;
         sp.jI -= sp.prev_jI_Odd;
         sp.prev_jI_Odd = sp.b * sp.prev_jI_input_Odd;
         sp.jI += sp.prev_jI_Odd;
         sp.prev_jI_input_Odd = sp.I1ForOddPrev3;
         sp.jI *= adjustedPrevPeriod;
         sp.hilbertTempReal = sp.a * sp.Q1;
         sp.jQ = 0 - sp.jQ_Odd[sp.hilbertIdx];
         sp.jQ_Odd[sp.hilbertIdx] = sp.hilbertTempReal;
         sp.jQ += sp.hilbertTempReal;
         sp.jQ -= sp.prev_jQ_Odd;
         sp.prev_jQ_Odd = sp.b * sp.prev_jQ_input_Odd;
         sp.jQ += sp.prev_jQ_Odd;
         sp.prev_jQ_input_Odd = sp.Q1;
         sp.jQ *= adjustedPrevPeriod;
         sp.Q2 = Math.fma(0.2, sp.Q1 + sp.jI, 0.8 * sp.prevQ2);
         sp.I2 = Math.fma(0.2, sp.I1ForOddPrev3 - sp.jQ, 0.8 * sp.prevI2);
         /* The varaiable I1 is the detrender delayed for
          * 3 price bars.
          *
          * Save the current detrender value for being
          * used by the "even" logic later.
          */
         sp.I1ForEvenPrev3 = sp.I1ForEvenPrev2;
         sp.I1ForEvenPrev2 = sp.detrender;
      }
      /* Adjust the period for next price bar */
      sp.Re = Math.fma(0.8, sp.Re, 0.2 * (Math.fma(sp.I2, sp.prevI2, sp.Q2 * sp.prevQ2)));
      sp.Im = Math.fma(0.8, sp.Im, 0.2 * (sp.I2 * sp.prevQ2 - sp.Q2 * sp.prevI2));
      sp.prevQ2 = sp.Q2;
      sp.prevI2 = sp.I2;
      sp.tempReal = sp.period;
      if( sp.Im != 0.0 && sp.Re != 0.0 ) {
         sp.period = 360.0 / (Math.atan(sp.Im / sp.Re) * sp.rad2Deg);
      }
      sp.tempReal2 = 1.5 * sp.tempReal;
      if( sp.period > sp.tempReal2 ) {
         sp.period = sp.tempReal2;
      }
      sp.tempReal2 = 0.67 * sp.tempReal;
      if( sp.period < sp.tempReal2 ) {
         sp.period = sp.tempReal2;
      }
      if( sp.period < 6 ) {
         sp.period = 6;
      } else if( sp.period > 50 ) {
         sp.period = 50;
      }
      sp.period = Math.fma(0.2, sp.period, 0.8 * sp.tempReal);
      sp.smoothPeriod = Math.fma(0.67, sp.smoothPeriod, 0.33 * sp.period);
      /* Compute Dominant Cycle Phase */
      sp.DCPeriod = sp.smoothPeriod + 0.5;
      sp.DCPeriodInt = (int)sp.DCPeriod;
      sp.realPart = 0.0;
      sp.imagPart = 0.0;
      /* idx is used to iterate for up to 50 of the last
       * value of smoothPrice.
       */
      sp.idx = sp.smoothPrice_Idx;
      for( sp.i = 0; sp.i < sp.DCPeriodInt; sp.i += 1 ) {
         sp.tempReal = (double)sp.i * sp.constDeg2RadBy360 / (double)sp.DCPeriodInt;
         sp.tempReal2 = sp.cb_smoothPrice[sp.idx];
         sp.realPart += Math.sin(sp.tempReal) * sp.tempReal2;
         sp.imagPart += Math.cos(sp.tempReal) * sp.tempReal2;
         if( sp.idx == 0 ) {
            sp.idx = 50 - 1;
         } else {
            sp.idx -= 1;
         }
      }
      sp.tempReal = Math.abs(sp.imagPart);
      if( sp.tempReal > 0.0 ) {
         sp.DCPhase = Math.atan(sp.realPart / sp.imagPart) * sp.rad2Deg;
      } else if( sp.tempReal <= 0.01 ) {
         if( sp.realPart < 0.0 ) {
            sp.DCPhase -= 90.0;
         } else if( sp.realPart > 0.0 ) {
            sp.DCPhase += 90.0;
         }
      }
      sp.DCPhase += 90.0;
      /* Compensate for one bar lag of the weighted moving average */
      sp.DCPhase += 360.0 / sp.smoothPeriod;
      if( sp.imagPart < 0.0 ) {
         sp.DCPhase += 180.0;
      }
      if( sp.DCPhase > 315.0 ) {
         sp.DCPhase -= 360.0;
      }
      sp.cur_outSine = Math.sin(sp.DCPhase * sp.deg2Rad);
      sp.cur_outLeadSine = Math.sin((sp.DCPhase + 45) * sp.deg2Rad);
      /* Ooof... let's do the next price bar now! */
      sp.smoothPrice_Idx = sp.smoothPrice_Idx + 1;
      if( sp.smoothPrice_Idx > sp.maxIdx_smoothPrice ) {
         sp.smoothPrice_Idx = 0;
      }
      sp.ring_trailingWMAIdx_inReal[sp.ringPos_trailingWMAIdx] = inReal;
      sp.ringPos_trailingWMAIdx = sp.ringPos_trailingWMAIdx + 1;
      if( sp.ringPos_trailingWMAIdx >= sp.ringCap_trailingWMAIdx ) {
         sp.ringPos_trailingWMAIdx = 0;
      }
      sp.streamParity = 1 - sp.streamParity;
   }
   private RetCode htSineOpenBody( HtSineStream sp, double inReal[], int startIdx )
   {
      int outIdx = 0;
      int i = 0;
      int lookbackTotal = 0;
      int today = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double adjustedPrevPeriod = 0;
      double period = 0;
      int trailingWMAIdx = 0;
      double periodWMASum = 0;
      double periodWMASub = 0;
      double trailingWMAValue = 0;
      double smoothedValue = 0;
      double a = 0;
      double b = 0;
      double hilbertTempReal = 0;
      int hilbertIdx = 0;
      double[] detrender_Odd = new double[3];
      double[] detrender_Even = new double[3];
      double detrender = 0;
      double prev_detrender_Odd = 0;
      double prev_detrender_Even = 0;
      double prev_detrender_input_Odd = 0;
      double prev_detrender_input_Even = 0;
      double[] Q1_Odd = new double[3];
      double[] Q1_Even = new double[3];
      double Q1 = 0;
      double prev_Q1_Odd = 0;
      double prev_Q1_Even = 0;
      double prev_Q1_input_Odd = 0;
      double prev_Q1_input_Even = 0;
      double[] jI_Odd = new double[3];
      double[] jI_Even = new double[3];
      double jI = 0;
      double prev_jI_Odd = 0;
      double prev_jI_Even = 0;
      double prev_jI_input_Odd = 0;
      double prev_jI_input_Even = 0;
      double[] jQ_Odd = new double[3];
      double[] jQ_Even = new double[3];
      double jQ = 0;
      double prev_jQ_Odd = 0;
      double prev_jQ_Even = 0;
      double prev_jQ_input_Odd = 0;
      double prev_jQ_input_Even = 0;
      double Q2 = 0;
      double I2 = 0;
      double prevQ2 = 0;
      double prevI2 = 0;
      double Re = 0;
      double Im = 0;
      double I1ForOddPrev2 = 0;
      double I1ForOddPrev3 = 0;
      double I1ForEvenPrev2 = 0;
      double I1ForEvenPrev3 = 0;
      double rad2Deg = 0;
      double deg2Rad = 0;
      double constDeg2RadBy360 = 0;
      double todayValue = 0;
      double smoothPeriod = 0;
      double[] smoothPrice;
      int smoothPrice_Idx = 0;
      int maxIdx_smoothPrice = (50)-1;
      int idx = 0;
      int DCPeriodInt = 0;
      double DCPhase = 0;
      double DCPeriod = 0;
      double imagPart = 0;
      double realPart = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outSine = 0.0;
      double lastValue_outLeadSine = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      a = 0.0962;
      b = 0.5769;
      /* Variable used for the price smoother (a weighted moving average). */
      /* Variables used for the Hilbert Transormation */
      /* Varaible used to keep track of the previous
       * smooth price. In the case of this algorithm,
       * we will never need more than 50 values.
       */
      smoothPrice = new double[maxIdx_smoothPrice+1];
      /* Variable used to calculate the dominant cycle phase */
      /* circular buffer already declared */
      /* The following could be replaced by constant eventually. */
      tempReal = Math.atan(1);
      rad2Deg = 45.0 / tempReal;
      deg2Rad = 1.0 / rad2Deg;
      constDeg2RadBy360 = tempReal * 8.0;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = 63 + this.unstablePeriod[FuncUnstId.HtSine.ordinal()];
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
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      /* Initialize the price smoother, which is simply a weighted
       * moving average of the price.
       * To understand this algorithm, I strongly suggest to understand
       * first how TA_WMA is done.
       */
      trailingWMAIdx = startIdx - lookbackTotal;
      today = trailingWMAIdx;
      /* Initialization is same as WMA, except loop is unrolled
       * for speed optimization.
       */
      tempReal = inReal[today++];
      periodWMASub = tempReal;
      periodWMASum = tempReal;
      tempReal = inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 2.0;
      tempReal = inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 3.0;
      trailingWMAValue = 0.0;
      /* Subsequent WMA value are evaluated by using
       * the DO_PRICE_WMA macro.
       */
      i = 34;
      do {
         tempReal = inReal[today++];
         periodWMASub += tempReal;
         periodWMASub -= trailingWMAValue;
         periodWMASum += tempReal * 4.0;
         trailingWMAValue = inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
      } while( --i != 0 );
      /* Initialize the circular buffers used by the hilbert
       * transform logic.
       * A buffer is used for odd day and another for even days.
       * This minimize the number of memory access and floating point
       * operations needed (note also that by using static circular buffer,
       * no large dynamic memory allocation is needed for storing
       * intermediate calculation!).
       */
      hilbertIdx = 0;
      detrender_Odd[0] = 0.0;
      detrender_Odd[1] = 0.0;
      detrender_Odd[2] = 0.0;
      detrender_Even[0] = 0.0;
      detrender_Even[1] = 0.0;
      detrender_Even[2] = 0.0;
      detrender = 0.0;
      prev_detrender_Odd = 0.0;
      prev_detrender_Even = 0.0;
      prev_detrender_input_Odd = 0.0;
      prev_detrender_input_Even = 0.0;
      Q1_Odd[0] = 0.0;
      Q1_Odd[1] = 0.0;
      Q1_Odd[2] = 0.0;
      Q1_Even[0] = 0.0;
      Q1_Even[1] = 0.0;
      Q1_Even[2] = 0.0;
      Q1 = 0.0;
      prev_Q1_Odd = 0.0;
      prev_Q1_Even = 0.0;
      prev_Q1_input_Odd = 0.0;
      prev_Q1_input_Even = 0.0;
      jI_Odd[0] = 0.0;
      jI_Odd[1] = 0.0;
      jI_Odd[2] = 0.0;
      jI_Even[0] = 0.0;
      jI_Even[1] = 0.0;
      jI_Even[2] = 0.0;
      jI = 0.0;
      prev_jI_Odd = 0.0;
      prev_jI_Even = 0.0;
      prev_jI_input_Odd = 0.0;
      prev_jI_input_Even = 0.0;
      jQ_Odd[0] = 0.0;
      jQ_Odd[1] = 0.0;
      jQ_Odd[2] = 0.0;
      jQ_Even[0] = 0.0;
      jQ_Even[1] = 0.0;
      jQ_Even[2] = 0.0;
      jQ = 0.0;
      prev_jQ_Odd = 0.0;
      prev_jQ_Even = 0.0;
      prev_jQ_input_Odd = 0.0;
      prev_jQ_input_Even = 0.0;
      period = 0.0;
      outIdx = 0;
      prevQ2 = 0.0;
      prevI2 = prevQ2;
      Im = 0.0;
      Re = Im;
      I1ForEvenPrev3 = 0.0;
      I1ForOddPrev3 = I1ForEvenPrev3;
      I1ForEvenPrev2 = 0.0;
      I1ForOddPrev2 = I1ForEvenPrev2;
      smoothPeriod = 0.0;
      for( i = 0; i < 50; i += 1 ) {
         smoothPrice[i] = 0.0;
      }
      /* The code is speed optimized and is most likely very
       * hard to follow if you do not already know well the
       * original algorithm.
       * To understadn better, it is strongly suggested to look
       * first at the Excel implementation in "test_MAMA.xls" included
       * in this package.
       */
      DCPhase = 0.0;
      while( today <= endIdx ) {
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
         todayValue = inReal[today];
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
         /* Remember the smoothedValue into the smoothPrice
          * circular buffer.
          */
         smoothPrice[smoothPrice_Idx] = smoothedValue;
         if( today % 2 == 0 ) {
            /* Do the Hilbert Transforms for even price bar */
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Even[hilbertIdx];
            detrender_Even[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Even;
            prev_detrender_Even = b * prev_detrender_input_Even;
            detrender += prev_detrender_Even;
            prev_detrender_input_Even = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Even[hilbertIdx];
            Q1_Even[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Even;
            prev_Q1_Even = b * prev_Q1_input_Even;
            Q1 += prev_Q1_Even;
            prev_Q1_input_Even = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForEvenPrev3;
            jI = 0 - jI_Even[hilbertIdx];
            jI_Even[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Even;
            prev_jI_Even = b * prev_jI_input_Even;
            jI += prev_jI_Even;
            prev_jI_input_Even = I1ForEvenPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Even[hilbertIdx];
            jQ_Even[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Even;
            prev_jQ_Even = b * prev_jQ_input_Even;
            jQ += prev_jQ_Even;
            prev_jQ_input_Even = Q1;
            jQ *= adjustedPrevPeriod;
            if( ++hilbertIdx == 3 ) {
               hilbertIdx = 0;
            }
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForEvenPrev3 - jQ, 0.8 * prevI2);
            /* The variable I1 is the detrender delayed for
             * 3 price bars.
             *
             * Save the current detrender value for being
             * used by the "odd" logic later.
             */
            I1ForOddPrev3 = I1ForOddPrev2;
            I1ForOddPrev2 = detrender;
         } else {
            /* Do the Hilbert Transforms for odd price bar */
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Odd[hilbertIdx];
            detrender_Odd[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Odd;
            prev_detrender_Odd = b * prev_detrender_input_Odd;
            detrender += prev_detrender_Odd;
            prev_detrender_input_Odd = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Odd[hilbertIdx];
            Q1_Odd[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Odd;
            prev_Q1_Odd = b * prev_Q1_input_Odd;
            Q1 += prev_Q1_Odd;
            prev_Q1_input_Odd = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForOddPrev3;
            jI = 0 - jI_Odd[hilbertIdx];
            jI_Odd[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Odd;
            prev_jI_Odd = b * prev_jI_input_Odd;
            jI += prev_jI_Odd;
            prev_jI_input_Odd = I1ForOddPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Odd[hilbertIdx];
            jQ_Odd[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Odd;
            prev_jQ_Odd = b * prev_jQ_input_Odd;
            jQ += prev_jQ_Odd;
            prev_jQ_input_Odd = Q1;
            jQ *= adjustedPrevPeriod;
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
            /* The varaiable I1 is the detrender delayed for
             * 3 price bars.
             *
             * Save the current detrender value for being
             * used by the "even" logic later.
             */
            I1ForEvenPrev3 = I1ForEvenPrev2;
            I1ForEvenPrev2 = detrender;
         }
         /* Adjust the period for next price bar */
         Re = Math.fma(0.8, Re, 0.2 * (Math.fma(I2, prevI2, Q2 * prevQ2)));
         Im = Math.fma(0.8, Im, 0.2 * (I2 * prevQ2 - Q2 * prevI2));
         prevQ2 = Q2;
         prevI2 = I2;
         tempReal = period;
         if( Im != 0.0 && Re != 0.0 ) {
            period = 360.0 / (Math.atan(Im / Re) * rad2Deg);
         }
         tempReal2 = 1.5 * tempReal;
         if( period > tempReal2 ) {
            period = tempReal2;
         }
         tempReal2 = 0.67 * tempReal;
         if( period < tempReal2 ) {
            period = tempReal2;
         }
         if( period < 6 ) {
            period = 6;
         } else if( period > 50 ) {
            period = 50;
         }
         period = Math.fma(0.2, period, 0.8 * tempReal);
         smoothPeriod = Math.fma(0.67, smoothPeriod, 0.33 * period);
         /* Compute Dominant Cycle Phase */
         DCPeriod = smoothPeriod + 0.5;
         DCPeriodInt = (int)DCPeriod;
         realPart = 0.0;
         imagPart = 0.0;
         /* idx is used to iterate for up to 50 of the last
          * value of smoothPrice.
          */
         idx = smoothPrice_Idx;
         for( i = 0; i < DCPeriodInt; i += 1 ) {
            tempReal = (double)i * constDeg2RadBy360 / (double)DCPeriodInt;
            tempReal2 = smoothPrice[idx];
            realPart += Math.sin(tempReal) * tempReal2;
            imagPart += Math.cos(tempReal) * tempReal2;
            if( idx == 0 ) {
               idx = 50 - 1;
            } else {
               idx -= 1;
            }
         }
         tempReal = Math.abs(imagPart);
         if( tempReal > 0.0 ) {
            DCPhase = Math.atan(realPart / imagPart) * rad2Deg;
         } else if( tempReal <= 0.01 ) {
            if( realPart < 0.0 ) {
               DCPhase -= 90.0;
            } else if( realPart > 0.0 ) {
               DCPhase += 90.0;
            }
         }
         DCPhase += 90.0;
         /* Compensate for one bar lag of the weighted moving average */
         DCPhase += 360.0 / smoothPeriod;
         if( imagPart < 0.0 ) {
            DCPhase += 180.0;
         }
         if( DCPhase > 315.0 ) {
            DCPhase -= 360.0;
         }
         if( today >= startIdx ) {
            lastValue_outSine = Math.sin(DCPhase * deg2Rad);
            lastValue_outLeadSine = Math.sin((DCPhase + 45) * deg2Rad);
         }
         /* Ooof... let's do the next price bar now! */
         smoothPrice_Idx++;
         if( smoothPrice_Idx > maxIdx_smoothPrice ) { smoothPrice_Idx = 0; }
         today += 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingWMAIdx = today - trailingWMAIdx;
      if( cap_trailingWMAIdx < 0 || cap_trailingWMAIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingWMAIdx = (cap_trailingWMAIdx > 0)? cap_trailingWMAIdx : 1;
      double[] capRing_trailingWMAIdx_inReal = new double[allocN_trailingWMAIdx];
      System.arraycopy(inReal, historyLen - cap_trailingWMAIdx, capRing_trailingWMAIdx_inReal, 0, cap_trailingWMAIdx);
      int capCb_smoothPrice = maxIdx_smoothPrice + 1;
      if( capCb_smoothPrice > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.i = i;
      sp.tempReal = tempReal;
      sp.tempReal2 = tempReal2;
      sp.period = period;
      sp.periodWMASum = periodWMASum;
      sp.periodWMASub = periodWMASub;
      sp.trailingWMAValue = trailingWMAValue;
      sp.smoothedValue = smoothedValue;
      sp.a = a;
      sp.b = b;
      sp.hilbertTempReal = hilbertTempReal;
      sp.hilbertIdx = hilbertIdx;
      sp.detrender_Odd = detrender_Odd;
      sp.detrender_Even = detrender_Even;
      sp.detrender = detrender;
      sp.prev_detrender_Odd = prev_detrender_Odd;
      sp.prev_detrender_Even = prev_detrender_Even;
      sp.prev_detrender_input_Odd = prev_detrender_input_Odd;
      sp.prev_detrender_input_Even = prev_detrender_input_Even;
      sp.Q1_Odd = Q1_Odd;
      sp.Q1_Even = Q1_Even;
      sp.Q1 = Q1;
      sp.prev_Q1_Odd = prev_Q1_Odd;
      sp.prev_Q1_Even = prev_Q1_Even;
      sp.prev_Q1_input_Odd = prev_Q1_input_Odd;
      sp.prev_Q1_input_Even = prev_Q1_input_Even;
      sp.jI_Odd = jI_Odd;
      sp.jI_Even = jI_Even;
      sp.jI = jI;
      sp.prev_jI_Odd = prev_jI_Odd;
      sp.prev_jI_Even = prev_jI_Even;
      sp.prev_jI_input_Odd = prev_jI_input_Odd;
      sp.prev_jI_input_Even = prev_jI_input_Even;
      sp.jQ_Odd = jQ_Odd;
      sp.jQ_Even = jQ_Even;
      sp.jQ = jQ;
      sp.prev_jQ_Odd = prev_jQ_Odd;
      sp.prev_jQ_Even = prev_jQ_Even;
      sp.prev_jQ_input_Odd = prev_jQ_input_Odd;
      sp.prev_jQ_input_Even = prev_jQ_input_Even;
      sp.Q2 = Q2;
      sp.I2 = I2;
      sp.prevQ2 = prevQ2;
      sp.prevI2 = prevI2;
      sp.Re = Re;
      sp.Im = Im;
      sp.I1ForOddPrev2 = I1ForOddPrev2;
      sp.I1ForOddPrev3 = I1ForOddPrev3;
      sp.I1ForEvenPrev2 = I1ForEvenPrev2;
      sp.I1ForEvenPrev3 = I1ForEvenPrev3;
      sp.rad2Deg = rad2Deg;
      sp.deg2Rad = deg2Rad;
      sp.constDeg2RadBy360 = constDeg2RadBy360;
      sp.smoothPeriod = smoothPeriod;
      sp.idx = idx;
      sp.DCPeriodInt = DCPeriodInt;
      sp.DCPhase = DCPhase;
      sp.DCPeriod = DCPeriod;
      sp.imagPart = imagPart;
      sp.realPart = realPart;
      sp.smoothPrice_Idx = smoothPrice_Idx;
      sp.maxIdx_smoothPrice = maxIdx_smoothPrice;
      sp.streamParity = historyLen % 2;
      sp.ringPos_trailingWMAIdx = 0;
      sp.ringCap_trailingWMAIdx = cap_trailingWMAIdx;
      sp.ring_trailingWMAIdx_inReal = capRing_trailingWMAIdx_inReal;
      sp.cbSize_smoothPrice = capCb_smoothPrice;
      sp.cb_smoothPrice = smoothPrice;
      sp.cur_outSine = lastValue_outSine;
      sp.cur_outLeadSine = lastValue_outLeadSine;
      sp.cachedValue = new HtSineStream.Value(sp.cur_outSine, sp.cur_outLeadSine);
      return RetCode.Success;
   }
   private RetCode htSineOpenAndFillBody( HtSineStream sp, double inReal[], MInteger outBegIdx, MInteger outNBElement, double outSine[], double outLeadSine[] )
   {
      int outIdx = 0;
      int i = 0;
      int lookbackTotal = 0;
      int today = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double adjustedPrevPeriod = 0;
      double period = 0;
      int trailingWMAIdx = 0;
      double periodWMASum = 0;
      double periodWMASub = 0;
      double trailingWMAValue = 0;
      double smoothedValue = 0;
      double a = 0;
      double b = 0;
      double hilbertTempReal = 0;
      int hilbertIdx = 0;
      double[] detrender_Odd = new double[3];
      double[] detrender_Even = new double[3];
      double detrender = 0;
      double prev_detrender_Odd = 0;
      double prev_detrender_Even = 0;
      double prev_detrender_input_Odd = 0;
      double prev_detrender_input_Even = 0;
      double[] Q1_Odd = new double[3];
      double[] Q1_Even = new double[3];
      double Q1 = 0;
      double prev_Q1_Odd = 0;
      double prev_Q1_Even = 0;
      double prev_Q1_input_Odd = 0;
      double prev_Q1_input_Even = 0;
      double[] jI_Odd = new double[3];
      double[] jI_Even = new double[3];
      double jI = 0;
      double prev_jI_Odd = 0;
      double prev_jI_Even = 0;
      double prev_jI_input_Odd = 0;
      double prev_jI_input_Even = 0;
      double[] jQ_Odd = new double[3];
      double[] jQ_Even = new double[3];
      double jQ = 0;
      double prev_jQ_Odd = 0;
      double prev_jQ_Even = 0;
      double prev_jQ_input_Odd = 0;
      double prev_jQ_input_Even = 0;
      double Q2 = 0;
      double I2 = 0;
      double prevQ2 = 0;
      double prevI2 = 0;
      double Re = 0;
      double Im = 0;
      double I1ForOddPrev2 = 0;
      double I1ForOddPrev3 = 0;
      double I1ForEvenPrev2 = 0;
      double I1ForEvenPrev3 = 0;
      double rad2Deg = 0;
      double deg2Rad = 0;
      double constDeg2RadBy360 = 0;
      double todayValue = 0;
      double smoothPeriod = 0;
      double[] smoothPrice;
      int smoothPrice_Idx = 0;
      int maxIdx_smoothPrice = (50)-1;
      int idx = 0;
      int DCPeriodInt = 0;
      double DCPhase = 0;
      double DCPeriod = 0;
      double imagPart = 0;
      double realPart = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( (Object)outSine == (Object)inReal || (Object)outLeadSine == (Object)inReal || (Object)outSine == (Object)outLeadSine ) {
         return RetCode.BadParam;
      }
      a = 0.0962;
      b = 0.5769;
      /* Variable used for the price smoother (a weighted moving average). */
      /* Variables used for the Hilbert Transormation */
      /* Varaible used to keep track of the previous
       * smooth price. In the case of this algorithm,
       * we will never need more than 50 values.
       */
      smoothPrice = new double[maxIdx_smoothPrice+1];
      /* Variable used to calculate the dominant cycle phase */
      /* circular buffer already declared */
      /* The following could be replaced by constant eventually. */
      tempReal = Math.atan(1);
      rad2Deg = 45.0 / tempReal;
      deg2Rad = 1.0 / rad2Deg;
      constDeg2RadBy360 = tempReal * 8.0;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = 63 + this.unstablePeriod[FuncUnstId.HtSine.ordinal()];
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
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      /* Initialize the price smoother, which is simply a weighted
       * moving average of the price.
       * To understand this algorithm, I strongly suggest to understand
       * first how TA_WMA is done.
       */
      trailingWMAIdx = startIdx - lookbackTotal;
      today = trailingWMAIdx;
      /* Initialization is same as WMA, except loop is unrolled
       * for speed optimization.
       */
      tempReal = inReal[today++];
      periodWMASub = tempReal;
      periodWMASum = tempReal;
      tempReal = inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 2.0;
      tempReal = inReal[today++];
      periodWMASub += tempReal;
      periodWMASum += tempReal * 3.0;
      trailingWMAValue = 0.0;
      /* Subsequent WMA value are evaluated by using
       * the DO_PRICE_WMA macro.
       */
      i = 34;
      do {
         tempReal = inReal[today++];
         periodWMASub += tempReal;
         periodWMASub -= trailingWMAValue;
         periodWMASum += tempReal * 4.0;
         trailingWMAValue = inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
      } while( --i != 0 );
      /* Initialize the circular buffers used by the hilbert
       * transform logic.
       * A buffer is used for odd day and another for even days.
       * This minimize the number of memory access and floating point
       * operations needed (note also that by using static circular buffer,
       * no large dynamic memory allocation is needed for storing
       * intermediate calculation!).
       */
      hilbertIdx = 0;
      detrender_Odd[0] = 0.0;
      detrender_Odd[1] = 0.0;
      detrender_Odd[2] = 0.0;
      detrender_Even[0] = 0.0;
      detrender_Even[1] = 0.0;
      detrender_Even[2] = 0.0;
      detrender = 0.0;
      prev_detrender_Odd = 0.0;
      prev_detrender_Even = 0.0;
      prev_detrender_input_Odd = 0.0;
      prev_detrender_input_Even = 0.0;
      Q1_Odd[0] = 0.0;
      Q1_Odd[1] = 0.0;
      Q1_Odd[2] = 0.0;
      Q1_Even[0] = 0.0;
      Q1_Even[1] = 0.0;
      Q1_Even[2] = 0.0;
      Q1 = 0.0;
      prev_Q1_Odd = 0.0;
      prev_Q1_Even = 0.0;
      prev_Q1_input_Odd = 0.0;
      prev_Q1_input_Even = 0.0;
      jI_Odd[0] = 0.0;
      jI_Odd[1] = 0.0;
      jI_Odd[2] = 0.0;
      jI_Even[0] = 0.0;
      jI_Even[1] = 0.0;
      jI_Even[2] = 0.0;
      jI = 0.0;
      prev_jI_Odd = 0.0;
      prev_jI_Even = 0.0;
      prev_jI_input_Odd = 0.0;
      prev_jI_input_Even = 0.0;
      jQ_Odd[0] = 0.0;
      jQ_Odd[1] = 0.0;
      jQ_Odd[2] = 0.0;
      jQ_Even[0] = 0.0;
      jQ_Even[1] = 0.0;
      jQ_Even[2] = 0.0;
      jQ = 0.0;
      prev_jQ_Odd = 0.0;
      prev_jQ_Even = 0.0;
      prev_jQ_input_Odd = 0.0;
      prev_jQ_input_Even = 0.0;
      period = 0.0;
      outIdx = 0;
      prevQ2 = 0.0;
      prevI2 = prevQ2;
      Im = 0.0;
      Re = Im;
      I1ForEvenPrev3 = 0.0;
      I1ForOddPrev3 = I1ForEvenPrev3;
      I1ForEvenPrev2 = 0.0;
      I1ForOddPrev2 = I1ForEvenPrev2;
      smoothPeriod = 0.0;
      for( i = 0; i < 50; i += 1 ) {
         smoothPrice[i] = 0.0;
      }
      /* The code is speed optimized and is most likely very
       * hard to follow if you do not already know well the
       * original algorithm.
       * To understadn better, it is strongly suggested to look
       * first at the Excel implementation in "test_MAMA.xls" included
       * in this package.
       */
      DCPhase = 0.0;
      while( today <= endIdx ) {
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
         todayValue = inReal[today];
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
         /* Remember the smoothedValue into the smoothPrice
          * circular buffer.
          */
         smoothPrice[smoothPrice_Idx] = smoothedValue;
         if( today % 2 == 0 ) {
            /* Do the Hilbert Transforms for even price bar */
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Even[hilbertIdx];
            detrender_Even[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Even;
            prev_detrender_Even = b * prev_detrender_input_Even;
            detrender += prev_detrender_Even;
            prev_detrender_input_Even = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Even[hilbertIdx];
            Q1_Even[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Even;
            prev_Q1_Even = b * prev_Q1_input_Even;
            Q1 += prev_Q1_Even;
            prev_Q1_input_Even = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForEvenPrev3;
            jI = 0 - jI_Even[hilbertIdx];
            jI_Even[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Even;
            prev_jI_Even = b * prev_jI_input_Even;
            jI += prev_jI_Even;
            prev_jI_input_Even = I1ForEvenPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Even[hilbertIdx];
            jQ_Even[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Even;
            prev_jQ_Even = b * prev_jQ_input_Even;
            jQ += prev_jQ_Even;
            prev_jQ_input_Even = Q1;
            jQ *= adjustedPrevPeriod;
            if( ++hilbertIdx == 3 ) {
               hilbertIdx = 0;
            }
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForEvenPrev3 - jQ, 0.8 * prevI2);
            /* The variable I1 is the detrender delayed for
             * 3 price bars.
             *
             * Save the current detrender value for being
             * used by the "odd" logic later.
             */
            I1ForOddPrev3 = I1ForOddPrev2;
            I1ForOddPrev2 = detrender;
         } else {
            /* Do the Hilbert Transforms for odd price bar */
            hilbertTempReal = a * smoothedValue;
            detrender = 0 - detrender_Odd[hilbertIdx];
            detrender_Odd[hilbertIdx] = hilbertTempReal;
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Odd;
            prev_detrender_Odd = b * prev_detrender_input_Odd;
            detrender += prev_detrender_Odd;
            prev_detrender_input_Odd = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = a * detrender;
            Q1 = 0 - Q1_Odd[hilbertIdx];
            Q1_Odd[hilbertIdx] = hilbertTempReal;
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Odd;
            prev_Q1_Odd = b * prev_Q1_input_Odd;
            Q1 += prev_Q1_Odd;
            prev_Q1_input_Odd = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = a * I1ForOddPrev3;
            jI = 0 - jI_Odd[hilbertIdx];
            jI_Odd[hilbertIdx] = hilbertTempReal;
            jI += hilbertTempReal;
            jI -= prev_jI_Odd;
            prev_jI_Odd = b * prev_jI_input_Odd;
            jI += prev_jI_Odd;
            prev_jI_input_Odd = I1ForOddPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = a * Q1;
            jQ = 0 - jQ_Odd[hilbertIdx];
            jQ_Odd[hilbertIdx] = hilbertTempReal;
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Odd;
            prev_jQ_Odd = b * prev_jQ_input_Odd;
            jQ += prev_jQ_Odd;
            prev_jQ_input_Odd = Q1;
            jQ *= adjustedPrevPeriod;
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
            /* The varaiable I1 is the detrender delayed for
             * 3 price bars.
             *
             * Save the current detrender value for being
             * used by the "even" logic later.
             */
            I1ForEvenPrev3 = I1ForEvenPrev2;
            I1ForEvenPrev2 = detrender;
         }
         /* Adjust the period for next price bar */
         Re = Math.fma(0.8, Re, 0.2 * (Math.fma(I2, prevI2, Q2 * prevQ2)));
         Im = Math.fma(0.8, Im, 0.2 * (I2 * prevQ2 - Q2 * prevI2));
         prevQ2 = Q2;
         prevI2 = I2;
         tempReal = period;
         if( Im != 0.0 && Re != 0.0 ) {
            period = 360.0 / (Math.atan(Im / Re) * rad2Deg);
         }
         tempReal2 = 1.5 * tempReal;
         if( period > tempReal2 ) {
            period = tempReal2;
         }
         tempReal2 = 0.67 * tempReal;
         if( period < tempReal2 ) {
            period = tempReal2;
         }
         if( period < 6 ) {
            period = 6;
         } else if( period > 50 ) {
            period = 50;
         }
         period = Math.fma(0.2, period, 0.8 * tempReal);
         smoothPeriod = Math.fma(0.67, smoothPeriod, 0.33 * period);
         /* Compute Dominant Cycle Phase */
         DCPeriod = smoothPeriod + 0.5;
         DCPeriodInt = (int)DCPeriod;
         realPart = 0.0;
         imagPart = 0.0;
         /* idx is used to iterate for up to 50 of the last
          * value of smoothPrice.
          */
         idx = smoothPrice_Idx;
         for( i = 0; i < DCPeriodInt; i += 1 ) {
            tempReal = (double)i * constDeg2RadBy360 / (double)DCPeriodInt;
            tempReal2 = smoothPrice[idx];
            realPart += Math.sin(tempReal) * tempReal2;
            imagPart += Math.cos(tempReal) * tempReal2;
            if( idx == 0 ) {
               idx = 50 - 1;
            } else {
               idx -= 1;
            }
         }
         tempReal = Math.abs(imagPart);
         if( tempReal > 0.0 ) {
            DCPhase = Math.atan(realPart / imagPart) * rad2Deg;
         } else if( tempReal <= 0.01 ) {
            if( realPart < 0.0 ) {
               DCPhase -= 90.0;
            } else if( realPart > 0.0 ) {
               DCPhase += 90.0;
            }
         }
         DCPhase += 90.0;
         /* Compensate for one bar lag of the weighted moving average */
         DCPhase += 360.0 / smoothPeriod;
         if( imagPart < 0.0 ) {
            DCPhase += 180.0;
         }
         if( DCPhase > 315.0 ) {
            DCPhase -= 360.0;
         }
         if( today >= startIdx ) {
            outSine[outIdx] = Math.sin(DCPhase * deg2Rad);
            outLeadSine[outIdx++] = Math.sin((DCPhase + 45) * deg2Rad);
         }
         /* Ooof... let's do the next price bar now! */
         smoothPrice_Idx++;
         if( smoothPrice_Idx > maxIdx_smoothPrice ) { smoothPrice_Idx = 0; }
         today += 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingWMAIdx = today - trailingWMAIdx;
      if( cap_trailingWMAIdx < 0 || cap_trailingWMAIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingWMAIdx = (cap_trailingWMAIdx > 0)? cap_trailingWMAIdx : 1;
      double[] capRing_trailingWMAIdx_inReal = new double[allocN_trailingWMAIdx];
      System.arraycopy(inReal, historyLen - cap_trailingWMAIdx, capRing_trailingWMAIdx_inReal, 0, cap_trailingWMAIdx);
      int capCb_smoothPrice = maxIdx_smoothPrice + 1;
      if( capCb_smoothPrice > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.i = i;
      sp.tempReal = tempReal;
      sp.tempReal2 = tempReal2;
      sp.period = period;
      sp.periodWMASum = periodWMASum;
      sp.periodWMASub = periodWMASub;
      sp.trailingWMAValue = trailingWMAValue;
      sp.smoothedValue = smoothedValue;
      sp.a = a;
      sp.b = b;
      sp.hilbertTempReal = hilbertTempReal;
      sp.hilbertIdx = hilbertIdx;
      sp.detrender_Odd = detrender_Odd;
      sp.detrender_Even = detrender_Even;
      sp.detrender = detrender;
      sp.prev_detrender_Odd = prev_detrender_Odd;
      sp.prev_detrender_Even = prev_detrender_Even;
      sp.prev_detrender_input_Odd = prev_detrender_input_Odd;
      sp.prev_detrender_input_Even = prev_detrender_input_Even;
      sp.Q1_Odd = Q1_Odd;
      sp.Q1_Even = Q1_Even;
      sp.Q1 = Q1;
      sp.prev_Q1_Odd = prev_Q1_Odd;
      sp.prev_Q1_Even = prev_Q1_Even;
      sp.prev_Q1_input_Odd = prev_Q1_input_Odd;
      sp.prev_Q1_input_Even = prev_Q1_input_Even;
      sp.jI_Odd = jI_Odd;
      sp.jI_Even = jI_Even;
      sp.jI = jI;
      sp.prev_jI_Odd = prev_jI_Odd;
      sp.prev_jI_Even = prev_jI_Even;
      sp.prev_jI_input_Odd = prev_jI_input_Odd;
      sp.prev_jI_input_Even = prev_jI_input_Even;
      sp.jQ_Odd = jQ_Odd;
      sp.jQ_Even = jQ_Even;
      sp.jQ = jQ;
      sp.prev_jQ_Odd = prev_jQ_Odd;
      sp.prev_jQ_Even = prev_jQ_Even;
      sp.prev_jQ_input_Odd = prev_jQ_input_Odd;
      sp.prev_jQ_input_Even = prev_jQ_input_Even;
      sp.Q2 = Q2;
      sp.I2 = I2;
      sp.prevQ2 = prevQ2;
      sp.prevI2 = prevI2;
      sp.Re = Re;
      sp.Im = Im;
      sp.I1ForOddPrev2 = I1ForOddPrev2;
      sp.I1ForOddPrev3 = I1ForOddPrev3;
      sp.I1ForEvenPrev2 = I1ForEvenPrev2;
      sp.I1ForEvenPrev3 = I1ForEvenPrev3;
      sp.rad2Deg = rad2Deg;
      sp.deg2Rad = deg2Rad;
      sp.constDeg2RadBy360 = constDeg2RadBy360;
      sp.smoothPeriod = smoothPeriod;
      sp.idx = idx;
      sp.DCPeriodInt = DCPeriodInt;
      sp.DCPhase = DCPhase;
      sp.DCPeriod = DCPeriod;
      sp.imagPart = imagPart;
      sp.realPart = realPart;
      sp.smoothPrice_Idx = smoothPrice_Idx;
      sp.maxIdx_smoothPrice = maxIdx_smoothPrice;
      sp.streamParity = historyLen % 2;
      sp.ringPos_trailingWMAIdx = 0;
      sp.ringCap_trailingWMAIdx = cap_trailingWMAIdx;
      sp.ring_trailingWMAIdx_inReal = capRing_trailingWMAIdx_inReal;
      sp.cbSize_smoothPrice = capCb_smoothPrice;
      sp.cb_smoothPrice = smoothPrice;
      sp.cur_outSine = outSine[outNBElement.value - 1];
      sp.cur_outLeadSine = outLeadSine[outNBElement.value - 1];
      sp.cachedValue = new HtSineStream.Value(sp.cur_outSine, sp.cur_outLeadSine);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind htSineOpen (composition seam). */
   HtSineStream htSineOpenInternal( double inReal[], int startIdx )
   {
      HtSineStream sp = new HtSineStream(this);
      RetCode retCode = htSineOpenBody(sp, inReal, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_HT_SINE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_HT_SINE open: internal error");
      }
      throw new IllegalArgumentException("TA_HT_SINE open: " + retCode);
   }
   /**
    * Open a live HT_SINE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#htSine} at that bar.
    * <p>The history must hold at least {@code htSineLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public HtSineStream htSineOpen( double inReal[] )
   {
      return htSineOpenInternal(inReal, 0);
   }
   /**
    * {@link Core#htSineOpen} that also fills the output array(s) bit-identically
    * to {@link Core#htSine} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public HtSineStream htSineOpenAndFill( double inReal[], MInteger outBegIdx, MInteger outNBElement, double outSine[], double outLeadSine[] )
   {
      HtSineStream sp = new HtSineStream(this);
      RetCode retCode = htSineOpenAndFillBody(sp, inReal, outBegIdx, outNBElement, outSine, outLeadSine);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_HT_SINE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_HT_SINE openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_HT_SINE openAndFill: " + retCode);
   }
