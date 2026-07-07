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
 *  023003 MF   Initial Coding of MAMA.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   public int mamaLookback( double optInFastLimit, double optInSlowLimit )
   {
      if( optInFastLimit == -4e37 ) {
         optInFastLimit = 5e-1;
      } else if( optInFastLimit < 1e-2 || optInFastLimit > 9.9e-1 ) {
         return -1;
      }
      if( optInSlowLimit == -4e37 ) {
         optInSlowLimit = 5e-2;
      } else if( optInSlowLimit < 1e-2 || optInSlowLimit > 9.9e-1 ) {
         return -1;
      }
      /* The two parameters are not a factor to determine
       * the lookback, but are still requested for
       * consistency with all other Lookback functions.
       */
      /* Lookback is a fix amount + the unstable period.
       *
       *
       * The fix lookback is 32 and is establish as follow:
       *
       *         12 price bar to be compatible with the implementation
       *            of TradeStation found in John Ehlers book.
       *          6 price bars for the Detrender
       *          6 price bars for Q1
       *          3 price bars for jI
       *          3 price bars for jQ
       *          1 price bar for Re/Im
       *          1 price bar for the Delta Phase
       *        -------
       *         32 Total
       */
      return 32 + this.unstablePeriod[FuncUnstId.Mama.ordinal()] ;

   }
   public RetCode mama( int startIdx,
                        int endIdx,
                        double inReal[],
                        double optInFastLimit,
                        double optInSlowLimit,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outMAMA[],
                        double outFAMA[] )
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
      double mama = 0;
      double fama = 0;
      double todayValue = 0;
      double prevPhase = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastLimit == -4e37 ) {
         optInFastLimit = 5e-1;
      } else if( optInFastLimit < 1e-2 || optInFastLimit > 9.9e-1 ) {
         return RetCode.BadParam;
      }
      if( optInSlowLimit == -4e37 ) {
         optInSlowLimit = 5e-2;
      } else if( optInSlowLimit < 1e-2 || optInSlowLimit > 9.9e-1 ) {
         return RetCode.BadParam;
      }
      a = 0.0962;
      b = 0.5769;
      /* Variable used for the price smoother (a weighted moving average). */
      /* Variables used for the Hilbert Transormation */
      /* Constant */
      rad2Deg = 180.0 / (4.0 * Math.atan(1));
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = 32 + this.unstablePeriod[FuncUnstId.Mama.ordinal()];
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
      i = 9;
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
      fama = 0.0;
      mama = fama;
      I1ForEvenPrev3 = 0.0;
      I1ForOddPrev3 = I1ForEvenPrev3;
      I1ForEvenPrev2 = 0.0;
      I1ForOddPrev2 = I1ForEvenPrev2;
      prevPhase = 0.0;
      /* The code is speed optimized and is most likely very
       * hard to follow if you do not already know well the
       * original algorithm.
       * To understadn better, it is strongly suggested to look
       * first at the Excel implementation in "test_MAMA.xls" included
       * in this package.
       */
      while( today <= endIdx ) {
         adjustedPrevPeriod = 0.075 * period + 0.54;
         todayValue = inReal[today];
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
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
            Q2 = 0.2 * (Q1 + jI) + 0.8 * prevQ2;
            I2 = 0.2 * (I1ForEvenPrev3 - jQ) + 0.8 * prevI2;
            /* The variable I1 is the detrender delayed for
             * 3 price bars.
             *
             * Save the current detrender value for being
             * used by the "odd" logic later.
             */
            I1ForOddPrev3 = I1ForOddPrev2;
            I1ForOddPrev2 = detrender;
            /* Put Alpha in tempReal2 */
            if( I1ForEvenPrev3 != 0.0 ) {
               tempReal2 = Math.atan(Q1 / I1ForEvenPrev3) * rad2Deg;
            } else {
               tempReal2 = 0.0;
            }
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
            Q2 = 0.2 * (Q1 + jI) + 0.8 * prevQ2;
            I2 = 0.2 * (I1ForOddPrev3 - jQ) + 0.8 * prevI2;
            /* The varaiable I1 is the detrender delayed for
             * 3 price bars.
             *
             * Save the current detrender value for being
             * used by the "odd" logic later.
             */
            I1ForEvenPrev3 = I1ForEvenPrev2;
            I1ForEvenPrev2 = detrender;
            /* Put Alpha in tempReal2 */
            if( I1ForOddPrev3 != 0.0 ) {
               tempReal2 = Math.atan(Q1 / I1ForOddPrev3) * rad2Deg;
            } else {
               tempReal2 = 0.0;
            }
         }
         /* Put Delta Phase into tempReal */
         tempReal = prevPhase - tempReal2;
         prevPhase = tempReal2;
         if( tempReal < 1.0 ) {
            tempReal = 1.0;
         }
         /* Put Alpha into tempReal */
         if( tempReal > 1.0 ) {
            tempReal = optInFastLimit / tempReal;
            if( tempReal < optInSlowLimit ) {
               tempReal = optInSlowLimit;
            }
         } else {
            tempReal = optInFastLimit;
         }
         /* Calculate MAMA, FAMA */
         mama = tempReal * todayValue + (1 - tempReal) * mama;
         tempReal *= 0.5;
         fama = tempReal * mama + (1 - tempReal) * fama;
         if( today >= startIdx ) {
            outMAMA[outIdx] = mama;
            outFAMA[outIdx++] = fama;
         }
         /* Adjust the period for next price bar */
         Re = 0.2 * (I2 * prevI2 + Q2 * prevQ2) + 0.8 * Re;
         Im = 0.2 * (I2 * prevQ2 - Q2 * prevI2) + 0.8 * Im;
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
         period = 0.2 * period + 0.8 * tempReal;
         /* Ooof... let's do the next price bar now! */
         today += 1;
      }
      /* Default return values */
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode mamaUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 double optInFastLimit,
                                 double optInSlowLimit,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outMAMA[],
                                 double outFAMA[] )
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
      double mama = 0;
      double fama = 0;
      double todayValue = 0;
      double prevPhase = 0;
      a = 0.0962;
      b = 0.5769;
      rad2Deg = 180.0 / (4.0 * Math.atan(1));
      lookbackTotal = 32 + this.unstablePeriod[FuncUnstId.Mama.ordinal()];
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
      i = 9;
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
      fama = 0.0;
      mama = fama;
      I1ForEvenPrev3 = 0.0;
      I1ForOddPrev3 = I1ForEvenPrev3;
      I1ForEvenPrev2 = 0.0;
      I1ForOddPrev2 = I1ForEvenPrev2;
      prevPhase = 0.0;
      while( today <= endIdx ) {
         adjustedPrevPeriod = 0.075 * period + 0.54;
         todayValue = inReal[today];
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
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
            Q2 = 0.2 * (Q1 + jI) + 0.8 * prevQ2;
            I2 = 0.2 * (I1ForEvenPrev3 - jQ) + 0.8 * prevI2;
            I1ForOddPrev3 = I1ForOddPrev2;
            I1ForOddPrev2 = detrender;
            if( I1ForEvenPrev3 != 0.0 ) {
               tempReal2 = Math.atan(Q1 / I1ForEvenPrev3) * rad2Deg;
            } else {
               tempReal2 = 0.0;
            }
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
            Q2 = 0.2 * (Q1 + jI) + 0.8 * prevQ2;
            I2 = 0.2 * (I1ForOddPrev3 - jQ) + 0.8 * prevI2;
            I1ForEvenPrev3 = I1ForEvenPrev2;
            I1ForEvenPrev2 = detrender;
            if( I1ForOddPrev3 != 0.0 ) {
               tempReal2 = Math.atan(Q1 / I1ForOddPrev3) * rad2Deg;
            } else {
               tempReal2 = 0.0;
            }
         }
         tempReal = prevPhase - tempReal2;
         prevPhase = tempReal2;
         if( tempReal < 1.0 ) {
            tempReal = 1.0;
         }
         if( tempReal > 1.0 ) {
            tempReal = optInFastLimit / tempReal;
            if( tempReal < optInSlowLimit ) {
               tempReal = optInSlowLimit;
            }
         } else {
            tempReal = optInFastLimit;
         }
         mama = tempReal * todayValue + (1 - tempReal) * mama;
         tempReal *= 0.5;
         fama = tempReal * mama + (1 - tempReal) * fama;
         if( today >= startIdx ) {
            outMAMA[outIdx] = mama;
            outFAMA[outIdx++] = fama;
         }
         Re = 0.2 * (I2 * prevI2 + Q2 * prevQ2) + 0.8 * Re;
         Im = 0.2 * (I2 * prevQ2 - Q2 * prevI2) + 0.8 * Im;
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
         period = 0.2 * period + 0.8 * tempReal;
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode mama( int startIdx,
                        int endIdx,
                        float inReal[],
                        double optInFastLimit,
                        double optInSlowLimit,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outMAMA[],
                        double outFAMA[] )
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
      double mama = 0;
      double fama = 0;
      double todayValue = 0;
      double prevPhase = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastLimit == -4e37 ) {
         optInFastLimit = 5e-1;
      } else if( optInFastLimit < 1e-2 || optInFastLimit > 9.9e-1 ) {
         return RetCode.BadParam;
      }
      if( optInSlowLimit == -4e37 ) {
         optInSlowLimit = 5e-2;
      } else if( optInSlowLimit < 1e-2 || optInSlowLimit > 9.9e-1 ) {
         return RetCode.BadParam;
      }
      a = 0.0962;
      b = 0.5769;
      rad2Deg = 180.0 / (4.0 * Math.atan(1));
      lookbackTotal = 32 + this.unstablePeriod[FuncUnstId.Mama.ordinal()];
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
      i = 9;
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
      fama = 0.0;
      mama = fama;
      I1ForEvenPrev3 = 0.0;
      I1ForOddPrev3 = I1ForEvenPrev3;
      I1ForEvenPrev2 = 0.0;
      I1ForOddPrev2 = I1ForEvenPrev2;
      prevPhase = 0.0;
      while( today <= endIdx ) {
         adjustedPrevPeriod = 0.075 * period + 0.54;
         todayValue = (double)inReal[today];
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = (double)inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
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
            Q2 = 0.2 * (Q1 + jI) + 0.8 * prevQ2;
            I2 = 0.2 * (I1ForEvenPrev3 - jQ) + 0.8 * prevI2;
            I1ForOddPrev3 = I1ForOddPrev2;
            I1ForOddPrev2 = detrender;
            if( I1ForEvenPrev3 != 0.0 ) {
               tempReal2 = Math.atan(Q1 / I1ForEvenPrev3) * rad2Deg;
            } else {
               tempReal2 = 0.0;
            }
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
            Q2 = 0.2 * (Q1 + jI) + 0.8 * prevQ2;
            I2 = 0.2 * (I1ForOddPrev3 - jQ) + 0.8 * prevI2;
            I1ForEvenPrev3 = I1ForEvenPrev2;
            I1ForEvenPrev2 = detrender;
            if( I1ForOddPrev3 != 0.0 ) {
               tempReal2 = Math.atan(Q1 / I1ForOddPrev3) * rad2Deg;
            } else {
               tempReal2 = 0.0;
            }
         }
         tempReal = prevPhase - tempReal2;
         prevPhase = tempReal2;
         if( tempReal < 1.0 ) {
            tempReal = 1.0;
         }
         if( tempReal > 1.0 ) {
            tempReal = optInFastLimit / tempReal;
            if( tempReal < optInSlowLimit ) {
               tempReal = optInSlowLimit;
            }
         } else {
            tempReal = optInFastLimit;
         }
         mama = tempReal * todayValue + (1 - tempReal) * mama;
         tempReal *= 0.5;
         fama = tempReal * mama + (1 - tempReal) * fama;
         if( today >= startIdx ) {
            outMAMA[outIdx] = mama;
            outFAMA[outIdx++] = fama;
         }
         Re = 0.2 * (I2 * prevI2 + Q2 * prevQ2) + 0.8 * Re;
         Im = 0.2 * (I2 * prevQ2 - Q2 * prevI2) + 0.8 * Im;
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
         period = 0.2 * period + 0.8 * tempReal;
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode mamaUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 double optInFastLimit,
                                 double optInSlowLimit,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outMAMA[],
                                 double outFAMA[] )
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
      double mama = 0;
      double fama = 0;
      double todayValue = 0;
      double prevPhase = 0;
      a = 0.0962;
      b = 0.5769;
      rad2Deg = 180.0 / (4.0 * Math.atan(1));
      lookbackTotal = 32 + this.unstablePeriod[FuncUnstId.Mama.ordinal()];
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
      i = 9;
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
      fama = 0.0;
      mama = fama;
      I1ForEvenPrev3 = 0.0;
      I1ForOddPrev3 = I1ForEvenPrev3;
      I1ForEvenPrev2 = 0.0;
      I1ForOddPrev2 = I1ForEvenPrev2;
      prevPhase = 0.0;
      while( today <= endIdx ) {
         adjustedPrevPeriod = 0.075 * period + 0.54;
         todayValue = (double)inReal[today];
         periodWMASub += todayValue;
         periodWMASub -= trailingWMAValue;
         periodWMASum += todayValue * 4.0;
         trailingWMAValue = (double)inReal[trailingWMAIdx++];
         smoothedValue = periodWMASum * 0.1;
         periodWMASum -= periodWMASub;
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
            Q2 = 0.2 * (Q1 + jI) + 0.8 * prevQ2;
            I2 = 0.2 * (I1ForEvenPrev3 - jQ) + 0.8 * prevI2;
            I1ForOddPrev3 = I1ForOddPrev2;
            I1ForOddPrev2 = detrender;
            if( I1ForEvenPrev3 != 0.0 ) {
               tempReal2 = Math.atan(Q1 / I1ForEvenPrev3) * rad2Deg;
            } else {
               tempReal2 = 0.0;
            }
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
            Q2 = 0.2 * (Q1 + jI) + 0.8 * prevQ2;
            I2 = 0.2 * (I1ForOddPrev3 - jQ) + 0.8 * prevI2;
            I1ForEvenPrev3 = I1ForEvenPrev2;
            I1ForEvenPrev2 = detrender;
            if( I1ForOddPrev3 != 0.0 ) {
               tempReal2 = Math.atan(Q1 / I1ForOddPrev3) * rad2Deg;
            } else {
               tempReal2 = 0.0;
            }
         }
         tempReal = prevPhase - tempReal2;
         prevPhase = tempReal2;
         if( tempReal < 1.0 ) {
            tempReal = 1.0;
         }
         if( tempReal > 1.0 ) {
            tempReal = optInFastLimit / tempReal;
            if( tempReal < optInSlowLimit ) {
               tempReal = optInSlowLimit;
            }
         } else {
            tempReal = optInFastLimit;
         }
         mama = tempReal * todayValue + (1 - tempReal) * mama;
         tempReal *= 0.5;
         fama = tempReal * mama + (1 - tempReal) * fama;
         if( today >= startIdx ) {
            outMAMA[outIdx] = mama;
            outFAMA[outIdx++] = fama;
         }
         Re = 0.2 * (I2 * prevI2 + Q2 * prevQ2) + 0.8 * Re;
         Im = 0.2 * (I2 * prevQ2 - Q2 * prevI2) + 0.8 * Im;
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
         period = 0.2 * period + 0.8 * tempReal;
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
