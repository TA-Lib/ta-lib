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

   /**
    * Number of leading input bars {@link Core#HT_DCPHASE} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int HT_DCPHASE_Lookback( )
   {
      /* 31 input are skip
       * +32 output are skip to account for misc lookback
       * ---
       *  63 Total Lookback
       *
       * 31 is for being compatible with Tradestation.
       * See mama_lookback for an explanation of the "32".
       */
      return 63 + this.unstablePeriod[FuncUnstId.HT_DCPHASE.ordinal()] ;

   }
   RetCode HT_DCPHASE_Impl( int startIdx,
                            int endIdx,
                            double inReal[],
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
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
      /* Constant */
      tempReal = Math.atan(1);
      rad2Deg = 45.0 / tempReal;
      constDeg2RadBy360 = tempReal * 8.0;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = 63 + this.unstablePeriod[FuncUnstId.HT_DCPHASE.ordinal()];
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
            outReal[outIdx++] = DCPhase;
         }
         /* Ooof... let's do the next price bar now! */
         smoothPrice_Idx++;
         if( smoothPrice_Idx > maxIdx_smoothPrice ) { smoothPrice_Idx = 0; }
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode HT_DCPHASE_Impl( int startIdx,
                            int endIdx,
                            float inReal[],
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      a = 0.0962;
      b = 0.5769;
      smoothPrice = new double[maxIdx_smoothPrice+1];
      tempReal = Math.atan(1);
      rad2Deg = 45.0 / tempReal;
      constDeg2RadBy360 = tempReal * 8.0;
      lookbackTotal = 63 + this.unstablePeriod[FuncUnstId.HT_DCPHASE.ordinal()];
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
            outReal[outIdx++] = DCPhase;
         }
         smoothPrice_Idx++;
         if( smoothPrice_Idx > maxIdx_smoothPrice ) { smoothPrice_Idx = 0; }
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Hilbert Transform Dominant Cycle Phase: the instantaneous phase (in
    * degrees) of the dominant market cycle, derived from a homodyne
    * discriminator on a Hilbert-transformed, smoothed price. One real output
    * per bar. Output is degrees, in the range −45 to 315 (a full 360° span).
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#HT_DCPHASE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Price series to analyze.
    * @param outReal Dominant cycle phase in degrees. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#HT_DCPERIOD
    * @see Core#HT_PHASOR
    * @see Core#HT_SINE
    * @see Core#HT_TRENDLINE
    * @see Core#HT_TRENDMODE
    * @see Core#MAMA
    * @see Core#WMA
    */
   public OutRange HT_DCPHASE( int startIdx,
                               int endIdx,
                               double inReal[],
                               double outReal[] )
   {
      requireIndexRange("HT_DCPHASE", startIdx, endIdx);
      int guardStart = clampedStart("HT_DCPHASE", startIdx, HT_DCPHASE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("HT_DCPHASE", "inReal", inReal, guardInLen);
      requireLength("HT_DCPHASE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = HT_DCPHASE_Impl(startIdx, endIdx, inReal, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("HT_DCPHASE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Hilbert Transform Dominant Cycle Phase: the instantaneous phase (in
    * degrees) of the dominant market cycle, derived from a homodyne
    * discriminator on a Hilbert-transformed, smoothed price. One real output
    * per bar. Output is degrees, in the range −45 to 315 (a full 360° span).
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#HT_DCPHASE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Price series to analyze.
    * @param outReal Dominant cycle phase in degrees. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#HT_DCPERIOD
    * @see Core#HT_PHASOR
    * @see Core#HT_SINE
    * @see Core#HT_TRENDLINE
    * @see Core#HT_TRENDMODE
    * @see Core#MAMA
    * @see Core#WMA
    */
   public OutRange HT_DCPHASE( int startIdx,
                               int endIdx,
                               float inReal[],
                               double outReal[] )
   {
      requireIndexRange("HT_DCPHASE", startIdx, endIdx);
      int guardStart = clampedStart("HT_DCPHASE", startIdx, HT_DCPHASE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("HT_DCPHASE", "inReal", inReal, guardInLen);
      requireLength("HT_DCPHASE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = HT_DCPHASE_Impl(startIdx, endIdx, inReal, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("HT_DCPHASE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live HT_DCPHASE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#HT_DCPHASE} over the same series.
    * Open with {@link Core#htDcphaseOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class HtDcphaseStream {
      Core core;
      double period;
      double periodWMASum;
      double periodWMASub;
      double trailingWMAValue;
      double a;
      double b;
      int hilbertIdx;
      double[] detrender_Odd;
      double[] detrender_Even;
      double prev_detrender_Odd;
      double prev_detrender_Even;
      double prev_detrender_input_Odd;
      double prev_detrender_input_Even;
      double[] Q1_Odd;
      double[] Q1_Even;
      double prev_Q1_Odd;
      double prev_Q1_Even;
      double prev_Q1_input_Odd;
      double prev_Q1_input_Even;
      double[] jI_Odd;
      double[] jI_Even;
      double prev_jI_Odd;
      double prev_jI_Even;
      double prev_jI_input_Odd;
      double prev_jI_input_Even;
      double[] jQ_Odd;
      double[] jQ_Even;
      double prev_jQ_Odd;
      double prev_jQ_Even;
      double prev_jQ_input_Odd;
      double prev_jQ_input_Even;
      double prevQ2;
      double prevI2;
      double Re;
      double Im;
      double I1ForOddPrev2;
      double I1ForOddPrev3;
      double I1ForEvenPrev2;
      double I1ForEvenPrev3;
      double rad2Deg;
      double constDeg2RadBy360;
      double smoothPeriod;
      double DCPhase;
      int smoothPrice_Idx;
      int maxIdx_smoothPrice;
      int streamParity;
      int ringPos_trailingWMAIdx;
      int ringCap_trailingWMAIdx;
      double[] ring_trailingWMAIdx_inReal;
      int cbSize_smoothPrice;
      double[] cb_smoothPrice;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      HtDcphaseStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#HT_DCPHASE} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      HtDcphaseStream( HtDcphaseStream other ) {
         this.core = other.core;
         this.period = other.period;
         this.periodWMASum = other.periodWMASum;
         this.periodWMASub = other.periodWMASub;
         this.trailingWMAValue = other.trailingWMAValue;
         this.a = other.a;
         this.b = other.b;
         this.hilbertIdx = other.hilbertIdx;
         this.detrender_Odd = other.detrender_Odd.clone();
         this.detrender_Even = other.detrender_Even.clone();
         this.prev_detrender_Odd = other.prev_detrender_Odd;
         this.prev_detrender_Even = other.prev_detrender_Even;
         this.prev_detrender_input_Odd = other.prev_detrender_input_Odd;
         this.prev_detrender_input_Even = other.prev_detrender_input_Even;
         this.Q1_Odd = other.Q1_Odd.clone();
         this.Q1_Even = other.Q1_Even.clone();
         this.prev_Q1_Odd = other.prev_Q1_Odd;
         this.prev_Q1_Even = other.prev_Q1_Even;
         this.prev_Q1_input_Odd = other.prev_Q1_input_Odd;
         this.prev_Q1_input_Even = other.prev_Q1_input_Even;
         this.jI_Odd = other.jI_Odd.clone();
         this.jI_Even = other.jI_Even.clone();
         this.prev_jI_Odd = other.prev_jI_Odd;
         this.prev_jI_Even = other.prev_jI_Even;
         this.prev_jI_input_Odd = other.prev_jI_input_Odd;
         this.prev_jI_input_Even = other.prev_jI_input_Even;
         this.jQ_Odd = other.jQ_Odd.clone();
         this.jQ_Even = other.jQ_Even.clone();
         this.prev_jQ_Odd = other.prev_jQ_Odd;
         this.prev_jQ_Even = other.prev_jQ_Even;
         this.prev_jQ_input_Odd = other.prev_jQ_input_Odd;
         this.prev_jQ_input_Even = other.prev_jQ_input_Even;
         this.prevQ2 = other.prevQ2;
         this.prevI2 = other.prevI2;
         this.Re = other.Re;
         this.Im = other.Im;
         this.I1ForOddPrev2 = other.I1ForOddPrev2;
         this.I1ForOddPrev3 = other.I1ForOddPrev3;
         this.I1ForEvenPrev2 = other.I1ForEvenPrev2;
         this.I1ForEvenPrev3 = other.I1ForEvenPrev3;
         this.rad2Deg = other.rad2Deg;
         this.constDeg2RadBy360 = other.constDeg2RadBy360;
         this.smoothPeriod = other.smoothPeriod;
         this.DCPhase = other.DCPhase;
         this.smoothPrice_Idx = other.smoothPrice_Idx;
         this.maxIdx_smoothPrice = other.maxIdx_smoothPrice;
         this.streamParity = other.streamParity;
         this.ringPos_trailingWMAIdx = other.ringPos_trailingWMAIdx;
         this.ringCap_trailingWMAIdx = other.ringCap_trailingWMAIdx;
         this.ring_trailingWMAIdx_inReal = other.ring_trailingWMAIdx_inReal.clone();
         this.cbSize_smoothPrice = other.cbSize_smoothPrice;
         this.cb_smoothPrice = other.cb_smoothPrice.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("HT_DCPHASE update: BadParam", RetCode.BadParam);
         }
         core.htDcphaseStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         requireArgument("HT_DCPHASE updateAndFill", "inReal", inReal);
         requireArgument("HT_DCPHASE updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("HT_DCPHASE updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("HT_DCPHASE updateAndFill: BadParam", RetCode.BadParam);
            }
            core.htDcphaseStepImpl(this, inReal[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("HT_DCPHASE peek: BadParam", RetCode.BadParam);
         HtDcphaseStream sp = this;
         int i = 0;
         double tempReal = 0.0;
         double tempReal2 = 0.0;
         double adjustedPrevPeriod = 0.0;
         double smoothedValue = 0.0;
         double hilbertTempReal = 0.0;
         double detrender = 0.0;
         double Q1 = 0.0;
         double jI = 0.0;
         double jQ = 0.0;
         double Q2 = 0.0;
         double I2 = 0.0;
         double todayValue = 0.0;
         int idx = 0;
         int DCPeriodInt = 0;
         double DCPeriod = 0.0;
         double imagPart = 0.0;
         double realPart = 0.0;
         double DCPhase = sp.DCPhase;
         double I1ForEvenPrev2 = sp.I1ForEvenPrev2;
         double I1ForEvenPrev3 = sp.I1ForEvenPrev3;
         double I1ForOddPrev2 = sp.I1ForOddPrev2;
         double I1ForOddPrev3 = sp.I1ForOddPrev3;
         double Im = sp.Im;
         double Re = sp.Re;
         double cur_outReal = sp.cur_outReal;
         int hilbertIdx = sp.hilbertIdx;
         double period = sp.period;
         double periodWMASub = sp.periodWMASub;
         double periodWMASum = sp.periodWMASum;
         double prevI2 = sp.prevI2;
         double prevQ2 = sp.prevQ2;
         double prev_Q1_Even = sp.prev_Q1_Even;
         double prev_Q1_Odd = sp.prev_Q1_Odd;
         double prev_Q1_input_Even = sp.prev_Q1_input_Even;
         double prev_Q1_input_Odd = sp.prev_Q1_input_Odd;
         double prev_detrender_Even = sp.prev_detrender_Even;
         double prev_detrender_Odd = sp.prev_detrender_Odd;
         double prev_detrender_input_Even = sp.prev_detrender_input_Even;
         double prev_detrender_input_Odd = sp.prev_detrender_input_Odd;
         double prev_jI_Even = sp.prev_jI_Even;
         double prev_jI_Odd = sp.prev_jI_Odd;
         double prev_jI_input_Even = sp.prev_jI_input_Even;
         double prev_jI_input_Odd = sp.prev_jI_input_Odd;
         double prev_jQ_Even = sp.prev_jQ_Even;
         double prev_jQ_Odd = sp.prev_jQ_Odd;
         double prev_jQ_input_Even = sp.prev_jQ_input_Even;
         double prev_jQ_input_Odd = sp.prev_jQ_input_Odd;
         double smoothPeriod = sp.smoothPeriod;
         double trailingWMAValue = sp.trailingWMAValue;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         if( sp.ringCap_trailingWMAIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
         todayValue = inReal;
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = (sp.ringPos_trailingWMAIdx != pkSlot0) ? sp.ring_trailingWMAIdx_inReal[sp.ringPos_trailingWMAIdx] : pkVal0;
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
         /* Remember the smoothedValue into the smoothPrice
          * circular buffer.
          */
         pkSlot1 = sp.smoothPrice_Idx;
         pkVal1 = smoothedValue;
         if( sp.streamParity == 0 ) {
            /* Do the Hilbert Transforms for even price bar */
            hilbertTempReal = sp.a * smoothedValue;
            detrender = 0 - sp.detrender_Even[hilbertIdx];
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Even;
            prev_detrender_Even = sp.b * prev_detrender_input_Even;
            detrender += prev_detrender_Even;
            prev_detrender_input_Even = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = sp.a * detrender;
            Q1 = 0 - sp.Q1_Even[hilbertIdx];
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Even;
            prev_Q1_Even = sp.b * prev_Q1_input_Even;
            Q1 += prev_Q1_Even;
            prev_Q1_input_Even = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = sp.a * I1ForEvenPrev3;
            jI = 0 - sp.jI_Even[hilbertIdx];
            jI += hilbertTempReal;
            jI -= prev_jI_Even;
            prev_jI_Even = sp.b * prev_jI_input_Even;
            jI += prev_jI_Even;
            prev_jI_input_Even = I1ForEvenPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = sp.a * Q1;
            jQ = 0 - sp.jQ_Even[hilbertIdx];
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Even;
            prev_jQ_Even = sp.b * prev_jQ_input_Even;
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
            hilbertTempReal = sp.a * smoothedValue;
            detrender = 0 - sp.detrender_Odd[hilbertIdx];
            detrender += hilbertTempReal;
            detrender -= prev_detrender_Odd;
            prev_detrender_Odd = sp.b * prev_detrender_input_Odd;
            detrender += prev_detrender_Odd;
            prev_detrender_input_Odd = smoothedValue;
            detrender *= adjustedPrevPeriod;
            hilbertTempReal = sp.a * detrender;
            Q1 = 0 - sp.Q1_Odd[hilbertIdx];
            Q1 += hilbertTempReal;
            Q1 -= prev_Q1_Odd;
            prev_Q1_Odd = sp.b * prev_Q1_input_Odd;
            Q1 += prev_Q1_Odd;
            prev_Q1_input_Odd = detrender;
            Q1 *= adjustedPrevPeriod;
            hilbertTempReal = sp.a * I1ForOddPrev3;
            jI = 0 - sp.jI_Odd[hilbertIdx];
            jI += hilbertTempReal;
            jI -= prev_jI_Odd;
            prev_jI_Odd = sp.b * prev_jI_input_Odd;
            jI += prev_jI_Odd;
            prev_jI_input_Odd = I1ForOddPrev3;
            jI *= adjustedPrevPeriod;
            hilbertTempReal = sp.a * Q1;
            jQ = 0 - sp.jQ_Odd[hilbertIdx];
            jQ += hilbertTempReal;
            jQ -= prev_jQ_Odd;
            prev_jQ_Odd = sp.b * prev_jQ_input_Odd;
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
            period = 360.0 / (Math.atan(Im / Re) * sp.rad2Deg);
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
         idx = sp.smoothPrice_Idx;
         for( i = 0; i < DCPeriodInt; i += 1 ) {
            tempReal = (double)i * sp.constDeg2RadBy360 / (double)DCPeriodInt;
            tempReal2 = (idx != pkSlot1) ? sp.cb_smoothPrice[idx] : pkVal1;
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
            DCPhase = Math.atan(realPart / imagPart) * sp.rad2Deg;
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
         cur_outReal = DCPhase;
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public HtDcphaseStream clone() {
         return new HtDcphaseStream(this);
      }
   }
   void htDcphaseStepImpl( HtDcphaseStream sp, double inReal )
   {
      int i = 0;
      double tempReal = 0.0;
      double tempReal2 = 0.0;
      double adjustedPrevPeriod = 0.0;
      double smoothedValue = 0.0;
      double hilbertTempReal = 0.0;
      double detrender = 0.0;
      double Q1 = 0.0;
      double jI = 0.0;
      double jQ = 0.0;
      double Q2 = 0.0;
      double I2 = 0.0;
      double todayValue = 0.0;
      int idx = 0;
      int DCPeriodInt = 0;
      double DCPeriod = 0.0;
      double imagPart = 0.0;
      double realPart = 0.0;
      if( sp.ringCap_trailingWMAIdx == 0 ) {
         sp.ring_trailingWMAIdx_inReal[0] = inReal;
      }
      adjustedPrevPeriod = Math.fma(0.075, sp.period, 0.54);
      todayValue = inReal;
      sp.periodWMASub += todayValue;
      sp.periodWMASub -= sp.trailingWMAValue;
      sp.periodWMASum += todayValue * 4.0;
      sp.trailingWMAValue = sp.ring_trailingWMAIdx_inReal[sp.ringPos_trailingWMAIdx];
      smoothedValue = sp.periodWMASum * 0.1;
      sp.periodWMASum -= sp.periodWMASub;
      /* Remember the smoothedValue into the smoothPrice
       * circular buffer.
       */
      sp.cb_smoothPrice[sp.smoothPrice_Idx] = smoothedValue;
      if( sp.streamParity == 0 ) {
         /* Do the Hilbert Transforms for even price bar */
         hilbertTempReal = sp.a * smoothedValue;
         detrender = 0 - sp.detrender_Even[sp.hilbertIdx];
         sp.detrender_Even[sp.hilbertIdx] = hilbertTempReal;
         detrender += hilbertTempReal;
         detrender -= sp.prev_detrender_Even;
         sp.prev_detrender_Even = sp.b * sp.prev_detrender_input_Even;
         detrender += sp.prev_detrender_Even;
         sp.prev_detrender_input_Even = smoothedValue;
         detrender *= adjustedPrevPeriod;
         hilbertTempReal = sp.a * detrender;
         Q1 = 0 - sp.Q1_Even[sp.hilbertIdx];
         sp.Q1_Even[sp.hilbertIdx] = hilbertTempReal;
         Q1 += hilbertTempReal;
         Q1 -= sp.prev_Q1_Even;
         sp.prev_Q1_Even = sp.b * sp.prev_Q1_input_Even;
         Q1 += sp.prev_Q1_Even;
         sp.prev_Q1_input_Even = detrender;
         Q1 *= adjustedPrevPeriod;
         hilbertTempReal = sp.a * sp.I1ForEvenPrev3;
         jI = 0 - sp.jI_Even[sp.hilbertIdx];
         sp.jI_Even[sp.hilbertIdx] = hilbertTempReal;
         jI += hilbertTempReal;
         jI -= sp.prev_jI_Even;
         sp.prev_jI_Even = sp.b * sp.prev_jI_input_Even;
         jI += sp.prev_jI_Even;
         sp.prev_jI_input_Even = sp.I1ForEvenPrev3;
         jI *= adjustedPrevPeriod;
         hilbertTempReal = sp.a * Q1;
         jQ = 0 - sp.jQ_Even[sp.hilbertIdx];
         sp.jQ_Even[sp.hilbertIdx] = hilbertTempReal;
         jQ += hilbertTempReal;
         jQ -= sp.prev_jQ_Even;
         sp.prev_jQ_Even = sp.b * sp.prev_jQ_input_Even;
         jQ += sp.prev_jQ_Even;
         sp.prev_jQ_input_Even = Q1;
         jQ *= adjustedPrevPeriod;
         if( ++sp.hilbertIdx == 3 ) {
            sp.hilbertIdx = 0;
         }
         Q2 = Math.fma(0.2, Q1 + jI, 0.8 * sp.prevQ2);
         I2 = Math.fma(0.2, sp.I1ForEvenPrev3 - jQ, 0.8 * sp.prevI2);
         /* The variable I1 is the detrender delayed for
          * 3 price bars.
          *
          * Save the current detrender value for being
          * used by the "odd" logic later.
          */
         sp.I1ForOddPrev3 = sp.I1ForOddPrev2;
         sp.I1ForOddPrev2 = detrender;
      } else {
         /* Do the Hilbert Transforms for odd price bar */
         hilbertTempReal = sp.a * smoothedValue;
         detrender = 0 - sp.detrender_Odd[sp.hilbertIdx];
         sp.detrender_Odd[sp.hilbertIdx] = hilbertTempReal;
         detrender += hilbertTempReal;
         detrender -= sp.prev_detrender_Odd;
         sp.prev_detrender_Odd = sp.b * sp.prev_detrender_input_Odd;
         detrender += sp.prev_detrender_Odd;
         sp.prev_detrender_input_Odd = smoothedValue;
         detrender *= adjustedPrevPeriod;
         hilbertTempReal = sp.a * detrender;
         Q1 = 0 - sp.Q1_Odd[sp.hilbertIdx];
         sp.Q1_Odd[sp.hilbertIdx] = hilbertTempReal;
         Q1 += hilbertTempReal;
         Q1 -= sp.prev_Q1_Odd;
         sp.prev_Q1_Odd = sp.b * sp.prev_Q1_input_Odd;
         Q1 += sp.prev_Q1_Odd;
         sp.prev_Q1_input_Odd = detrender;
         Q1 *= adjustedPrevPeriod;
         hilbertTempReal = sp.a * sp.I1ForOddPrev3;
         jI = 0 - sp.jI_Odd[sp.hilbertIdx];
         sp.jI_Odd[sp.hilbertIdx] = hilbertTempReal;
         jI += hilbertTempReal;
         jI -= sp.prev_jI_Odd;
         sp.prev_jI_Odd = sp.b * sp.prev_jI_input_Odd;
         jI += sp.prev_jI_Odd;
         sp.prev_jI_input_Odd = sp.I1ForOddPrev3;
         jI *= adjustedPrevPeriod;
         hilbertTempReal = sp.a * Q1;
         jQ = 0 - sp.jQ_Odd[sp.hilbertIdx];
         sp.jQ_Odd[sp.hilbertIdx] = hilbertTempReal;
         jQ += hilbertTempReal;
         jQ -= sp.prev_jQ_Odd;
         sp.prev_jQ_Odd = sp.b * sp.prev_jQ_input_Odd;
         jQ += sp.prev_jQ_Odd;
         sp.prev_jQ_input_Odd = Q1;
         jQ *= adjustedPrevPeriod;
         Q2 = Math.fma(0.2, Q1 + jI, 0.8 * sp.prevQ2);
         I2 = Math.fma(0.2, sp.I1ForOddPrev3 - jQ, 0.8 * sp.prevI2);
         /* The varaiable I1 is the detrender delayed for
          * 3 price bars.
          *
          * Save the current detrender value for being
          * used by the "even" logic later.
          */
         sp.I1ForEvenPrev3 = sp.I1ForEvenPrev2;
         sp.I1ForEvenPrev2 = detrender;
      }
      /* Adjust the period for next price bar */
      sp.Re = Math.fma(0.8, sp.Re, 0.2 * (Math.fma(I2, sp.prevI2, Q2 * sp.prevQ2)));
      sp.Im = Math.fma(0.8, sp.Im, 0.2 * (I2 * sp.prevQ2 - Q2 * sp.prevI2));
      sp.prevQ2 = Q2;
      sp.prevI2 = I2;
      tempReal = sp.period;
      if( sp.Im != 0.0 && sp.Re != 0.0 ) {
         sp.period = 360.0 / (Math.atan(sp.Im / sp.Re) * sp.rad2Deg);
      }
      tempReal2 = 1.5 * tempReal;
      if( sp.period > tempReal2 ) {
         sp.period = tempReal2;
      }
      tempReal2 = 0.67 * tempReal;
      if( sp.period < tempReal2 ) {
         sp.period = tempReal2;
      }
      if( sp.period < 6 ) {
         sp.period = 6;
      } else if( sp.period > 50 ) {
         sp.period = 50;
      }
      sp.period = Math.fma(0.2, sp.period, 0.8 * tempReal);
      sp.smoothPeriod = Math.fma(0.67, sp.smoothPeriod, 0.33 * sp.period);
      /* Compute Dominant Cycle Phase */
      DCPeriod = sp.smoothPeriod + 0.5;
      DCPeriodInt = (int)DCPeriod;
      realPart = 0.0;
      imagPart = 0.0;
      /* idx is used to iterate for up to 50 of the last
       * value of smoothPrice.
       */
      idx = sp.smoothPrice_Idx;
      for( i = 0; i < DCPeriodInt; i += 1 ) {
         tempReal = (double)i * sp.constDeg2RadBy360 / (double)DCPeriodInt;
         tempReal2 = sp.cb_smoothPrice[idx];
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
         sp.DCPhase = Math.atan(realPart / imagPart) * sp.rad2Deg;
      } else if( tempReal <= 0.01 ) {
         if( realPart < 0.0 ) {
            sp.DCPhase -= 90.0;
         } else if( realPart > 0.0 ) {
            sp.DCPhase += 90.0;
         }
      }
      sp.DCPhase += 90.0;
      /* Compensate for one bar lag of the weighted moving average */
      sp.DCPhase += 360.0 / sp.smoothPeriod;
      if( imagPart < 0.0 ) {
         sp.DCPhase += 180.0;
      }
      if( sp.DCPhase > 315.0 ) {
         sp.DCPhase -= 360.0;
      }
      sp.cur_outReal = sp.DCPhase;
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
   private RetCode htDcphaseOpenImpl( HtDcphaseStream sp, double inReal[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
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
      /* Constant */
      tempReal = Math.atan(1);
      rad2Deg = 45.0 / tempReal;
      constDeg2RadBy360 = tempReal * 8.0;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = 63 + this.unstablePeriod[FuncUnstId.HT_DCPHASE.ordinal()];
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
         return RetCode.InsufficientHistory ;
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
            outReal[outIdx++ * outStride] = DCPhase;
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
      sp.period = period;
      sp.periodWMASum = periodWMASum;
      sp.periodWMASub = periodWMASub;
      sp.trailingWMAValue = trailingWMAValue;
      sp.a = a;
      sp.b = b;
      sp.hilbertIdx = hilbertIdx;
      sp.detrender_Odd = detrender_Odd;
      sp.detrender_Even = detrender_Even;
      sp.prev_detrender_Odd = prev_detrender_Odd;
      sp.prev_detrender_Even = prev_detrender_Even;
      sp.prev_detrender_input_Odd = prev_detrender_input_Odd;
      sp.prev_detrender_input_Even = prev_detrender_input_Even;
      sp.Q1_Odd = Q1_Odd;
      sp.Q1_Even = Q1_Even;
      sp.prev_Q1_Odd = prev_Q1_Odd;
      sp.prev_Q1_Even = prev_Q1_Even;
      sp.prev_Q1_input_Odd = prev_Q1_input_Odd;
      sp.prev_Q1_input_Even = prev_Q1_input_Even;
      sp.jI_Odd = jI_Odd;
      sp.jI_Even = jI_Even;
      sp.prev_jI_Odd = prev_jI_Odd;
      sp.prev_jI_Even = prev_jI_Even;
      sp.prev_jI_input_Odd = prev_jI_input_Odd;
      sp.prev_jI_input_Even = prev_jI_input_Even;
      sp.jQ_Odd = jQ_Odd;
      sp.jQ_Even = jQ_Even;
      sp.prev_jQ_Odd = prev_jQ_Odd;
      sp.prev_jQ_Even = prev_jQ_Even;
      sp.prev_jQ_input_Odd = prev_jQ_input_Odd;
      sp.prev_jQ_input_Even = prev_jQ_input_Even;
      sp.prevQ2 = prevQ2;
      sp.prevI2 = prevI2;
      sp.Re = Re;
      sp.Im = Im;
      sp.I1ForOddPrev2 = I1ForOddPrev2;
      sp.I1ForOddPrev3 = I1ForOddPrev3;
      sp.I1ForEvenPrev2 = I1ForEvenPrev2;
      sp.I1ForEvenPrev3 = I1ForEvenPrev3;
      sp.rad2Deg = rad2Deg;
      sp.constDeg2RadBy360 = constDeg2RadBy360;
      sp.smoothPeriod = smoothPeriod;
      sp.DCPhase = DCPhase;
      sp.smoothPrice_Idx = smoothPrice_Idx;
      sp.maxIdx_smoothPrice = maxIdx_smoothPrice;
      sp.streamParity = historyLen % 2;
      sp.ringPos_trailingWMAIdx = 0;
      sp.ringCap_trailingWMAIdx = cap_trailingWMAIdx;
      sp.ring_trailingWMAIdx_inReal = capRing_trailingWMAIdx_inReal;
      sp.cbSize_smoothPrice = capCb_smoothPrice;
      sp.cb_smoothPrice = smoothPrice;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* htDcphaseOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   HtDcphaseStream htDcphaseOpenAndFillInternal( double inReal[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      HtDcphaseStream sp = new HtDcphaseStream(this);
      RetCode retCode = htDcphaseOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("HT_DCPHASE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("HT_DCPHASE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("HT_DCPHASE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind htDcphaseOpen (composition seam). */
   HtDcphaseStream htDcphaseOpenInternal( double inReal[], int startIdx )
   {
      HtDcphaseStream sp = new HtDcphaseStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = htDcphaseOpenImpl(sp, inReal, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("HT_DCPHASE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("HT_DCPHASE open: internal error", retCode);
      }
      throw new TaLibArgumentException("HT_DCPHASE open: " + retCode, retCode);
   }
   /**
    * Open a live HT_DCPHASE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#HT_DCPHASE} at that bar.
    * <p>The history must hold at least {@code HT_DCPHASE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public HtDcphaseStream htDcphaseOpen( double inReal[] )
   {
      requireArgument("HT_DCPHASE open", "inReal", inReal);
      requireHistory("HT_DCPHASE open", inReal.length);
      return htDcphaseOpenInternal(inReal, 0);
   }
   /**
    * {@link Core#htDcphaseOpen} that also fills the output array(s) bit-identically
    * to {@link Core#HT_DCPHASE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link HtDcphaseStream#outRange()}.
    */
   public HtDcphaseStream htDcphaseOpenAndFill( double inReal[], double outReal[] )
   {
      requireArgument("HT_DCPHASE openAndFill", "inReal", inReal);
      requireHistory("HT_DCPHASE openAndFill", inReal.length);
      int guardOutLen = openFillCount("HT_DCPHASE openAndFill", inReal.length, HT_DCPHASE_Lookback());
      requireLength("HT_DCPHASE openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("HT_DCPHASE openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return htDcphaseOpenAndFillInternal(inReal, 0, outBegIdx, outNBElement, outReal);
   }
