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
      if( outMAMA == outFAMA ) {
         return RetCode.BadParam ;
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
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
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
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
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
         mama = Math.fma(1 - tempReal, mama, tempReal * todayValue);
         tempReal *= 0.5;
         fama = Math.fma(1 - tempReal, fama, tempReal * mama);
         if( today >= startIdx ) {
            /* FAMA is nullable (issue #125): its write carries no outIdx advance so
             * the codegen can NULL-guard it; outMAMA (never NULL) owns the ++.
             */
            outFAMA[outIdx] = fama;
            outMAMA[outIdx++] = mama;
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
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
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
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForEvenPrev3 - jQ, 0.8 * prevI2);
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
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
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
         mama = Math.fma(1 - tempReal, mama, tempReal * todayValue);
         tempReal *= 0.5;
         fama = Math.fma(1 - tempReal, fama, tempReal * mama);
         if( today >= startIdx ) {
            outFAMA[outIdx] = fama;
            outMAMA[outIdx++] = mama;
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
      if( outMAMA == outFAMA ) {
         return RetCode.BadParam ;
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
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
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
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForEvenPrev3 - jQ, 0.8 * prevI2);
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
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
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
         mama = Math.fma(1 - tempReal, mama, tempReal * todayValue);
         tempReal *= 0.5;
         fama = Math.fma(1 - tempReal, fama, tempReal * mama);
         if( today >= startIdx ) {
            outFAMA[outIdx] = fama;
            outMAMA[outIdx++] = mama;
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
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
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
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForEvenPrev3 - jQ, 0.8 * prevI2);
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
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
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
         mama = Math.fma(1 - tempReal, mama, tempReal * todayValue);
         tempReal *= 0.5;
         fama = Math.fma(1 - tempReal, fama, tempReal * mama);
         if( today >= startIdx ) {
            outFAMA[outIdx] = fama;
            outMAMA[outIdx++] = mama;
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
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live MAMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#mama} over the same series.
    * Open with {@link Core#mamaOpen}; there is no close — the handle is
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
   public static final class MamaStream {
      final Core core;
      double optInFastLimit;
      double optInSlowLimit;
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
      double mama;
      double fama;
      double prevPhase;
      int streamParity;
      int ringPos_trailingWMAIdx;
      int ringCap_trailingWMAIdx;
      double[] ring_trailingWMAIdx_inReal;
      double cur_outMAMA;
      double cur_outFAMA;
      Value cachedValue;

      MamaStream( Core core ) { this.core = core; }

      MamaStream( MamaStream other ) {
         this.core = other.core;
         this.optInFastLimit = other.optInFastLimit;
         this.optInSlowLimit = other.optInSlowLimit;
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
         this.mama = other.mama;
         this.fama = other.fama;
         this.prevPhase = other.prevPhase;
         this.streamParity = other.streamParity;
         this.ringPos_trailingWMAIdx = other.ringPos_trailingWMAIdx;
         this.ringCap_trailingWMAIdx = other.ringCap_trailingWMAIdx;
         this.ring_trailingWMAIdx_inReal = other.ring_trailingWMAIdx_inReal.clone();
         this.cur_outMAMA = other.cur_outMAMA;
         this.cur_outFAMA = other.cur_outFAMA;
         this.cachedValue = other.cachedValue;
      }

      /** One output set, in batch output order. Immutable. */
      public static final class Value {
         public final double mama;
         public final double fama;
         Value( double mama, double fama ) {
            this.mama = mama;
            this.fama = fama;
         }
         @Override public String toString() {
            return "Value[" + "mama=" + mama + ", " + "fama=" + fama + "]";
         }
         @Override public boolean equals( Object o ) {
            if( !(o instanceof Value) ) return false;
            Value v = (Value) o;
            return Double.doubleToLongBits(this.mama) == Double.doubleToLongBits(v.mama) && Double.doubleToLongBits(this.fama) == Double.doubleToLongBits(v.fama);
         }
         @Override public int hashCode() {
            int h = 17;
            h = 31 * h + Double.hashCode(mama);
            h = 31 * h + Double.hashCode(fama);
            return h;
         }
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inReal ) {
         core.mamaStreamStep(this, inReal);
         this.cachedValue = new Value(this.cur_outMAMA, this.cur_outFAMA);
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
         MamaStream scratch = new MamaStream(this);
         core.mamaStreamStep(scratch, inReal);
         return new Value(scratch.cur_outMAMA, scratch.cur_outFAMA);
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
      public MamaStream copy() {
         return new MamaStream(this);
      }
   }
   void mamaStreamStep( MamaStream sp, double inReal )
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
         /* Put Alpha in tempReal2 */
         if( sp.I1ForEvenPrev3 != 0.0 ) {
            sp.tempReal2 = Math.atan(sp.Q1 / sp.I1ForEvenPrev3) * sp.rad2Deg;
         } else {
            sp.tempReal2 = 0.0;
         }
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
          * used by the "odd" logic later.
          */
         sp.I1ForEvenPrev3 = sp.I1ForEvenPrev2;
         sp.I1ForEvenPrev2 = sp.detrender;
         /* Put Alpha in tempReal2 */
         if( sp.I1ForOddPrev3 != 0.0 ) {
            sp.tempReal2 = Math.atan(sp.Q1 / sp.I1ForOddPrev3) * sp.rad2Deg;
         } else {
            sp.tempReal2 = 0.0;
         }
      }
      /* Put Delta Phase into tempReal */
      sp.tempReal = sp.prevPhase - sp.tempReal2;
      sp.prevPhase = sp.tempReal2;
      if( sp.tempReal < 1.0 ) {
         sp.tempReal = 1.0;
      }
      /* Put Alpha into tempReal */
      if( sp.tempReal > 1.0 ) {
         sp.tempReal = sp.optInFastLimit / sp.tempReal;
         if( sp.tempReal < sp.optInSlowLimit ) {
            sp.tempReal = sp.optInSlowLimit;
         }
      } else {
         sp.tempReal = sp.optInFastLimit;
      }
      /* Calculate MAMA, FAMA */
      sp.mama = Math.fma(1 - sp.tempReal, sp.mama, sp.tempReal * todayValue);
      sp.tempReal *= 0.5;
      sp.fama = Math.fma(1 - sp.tempReal, sp.fama, sp.tempReal * sp.mama);
      /* FAMA is nullable (issue #125): its write carries no outIdx advance so
       * the codegen can NULL-guard it; outMAMA (never NULL) owns the ++.
       */
      sp.cur_outFAMA = sp.fama;
      sp.cur_outMAMA = sp.mama;
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
      /* Ooof... let's do the next price bar now! */
      sp.ring_trailingWMAIdx_inReal[sp.ringPos_trailingWMAIdx] = inReal;
      sp.ringPos_trailingWMAIdx = sp.ringPos_trailingWMAIdx + 1;
      if( sp.ringPos_trailingWMAIdx >= sp.ringCap_trailingWMAIdx ) {
         sp.ringPos_trailingWMAIdx = 0;
      }
      sp.streamParity = 1 - sp.streamParity;
   }
   private RetCode mamaOpenBody( MamaStream sp, double inReal[], int startIdx, double optInFastLimit, double optInSlowLimit )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outMAMA = 0.0;
      double lastValue_outFAMA = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
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
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
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
         mama = Math.fma(1 - tempReal, mama, tempReal * todayValue);
         tempReal *= 0.5;
         fama = Math.fma(1 - tempReal, fama, tempReal * mama);
         if( today >= startIdx ) {
            /* FAMA is nullable (issue #125): its write carries no outIdx advance so
             * the codegen can NULL-guard it; outMAMA (never NULL) owns the ++.
             */
            lastValue_outFAMA = fama;
            lastValue_outMAMA = mama;
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
         /* Ooof... let's do the next price bar now! */
         today += 1;
      }
      /* Default return values */
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingWMAIdx = today - trailingWMAIdx;
      if( cap_trailingWMAIdx < 0 || cap_trailingWMAIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingWMAIdx = (cap_trailingWMAIdx > 0)? cap_trailingWMAIdx : 1;
      double[] capRing_trailingWMAIdx_inReal = new double[allocN_trailingWMAIdx];
      System.arraycopy(inReal, historyLen - cap_trailingWMAIdx, capRing_trailingWMAIdx_inReal, 0, cap_trailingWMAIdx);
      sp.optInFastLimit = optInFastLimit;
      sp.optInSlowLimit = optInSlowLimit;
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
      sp.mama = mama;
      sp.fama = fama;
      sp.prevPhase = prevPhase;
      sp.streamParity = historyLen % 2;
      sp.ringPos_trailingWMAIdx = 0;
      sp.ringCap_trailingWMAIdx = cap_trailingWMAIdx;
      sp.ring_trailingWMAIdx_inReal = capRing_trailingWMAIdx_inReal;
      sp.cur_outMAMA = lastValue_outMAMA;
      sp.cur_outFAMA = lastValue_outFAMA;
      sp.cachedValue = new MamaStream.Value(sp.cur_outMAMA, sp.cur_outFAMA);
      return RetCode.Success;
   }
   private RetCode mamaOpenAndFillBody( MamaStream sp, double inReal[], double optInFastLimit, double optInSlowLimit, MInteger outBegIdx, MInteger outNBElement, double outMAMA[], double outFAMA[] )
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
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
      if( (Object)outMAMA == (Object)inReal || (Object)outFAMA == (Object)inReal || (Object)outMAMA == (Object)outFAMA ) {
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
         adjustedPrevPeriod = Math.fma(0.075, period, 0.54);
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
            Q2 = Math.fma(0.2, Q1 + jI, 0.8 * prevQ2);
            I2 = Math.fma(0.2, I1ForOddPrev3 - jQ, 0.8 * prevI2);
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
         mama = Math.fma(1 - tempReal, mama, tempReal * todayValue);
         tempReal *= 0.5;
         fama = Math.fma(1 - tempReal, fama, tempReal * mama);
         if( today >= startIdx ) {
            /* FAMA is nullable (issue #125): its write carries no outIdx advance so
             * the codegen can NULL-guard it; outMAMA (never NULL) owns the ++.
             */
            outFAMA[outIdx] = fama;
            outMAMA[outIdx++] = mama;
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
         /* Ooof... let's do the next price bar now! */
         today += 1;
      }
      /* Default return values */
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingWMAIdx = today - trailingWMAIdx;
      if( cap_trailingWMAIdx < 0 || cap_trailingWMAIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingWMAIdx = (cap_trailingWMAIdx > 0)? cap_trailingWMAIdx : 1;
      double[] capRing_trailingWMAIdx_inReal = new double[allocN_trailingWMAIdx];
      System.arraycopy(inReal, historyLen - cap_trailingWMAIdx, capRing_trailingWMAIdx_inReal, 0, cap_trailingWMAIdx);
      sp.optInFastLimit = optInFastLimit;
      sp.optInSlowLimit = optInSlowLimit;
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
      sp.mama = mama;
      sp.fama = fama;
      sp.prevPhase = prevPhase;
      sp.streamParity = historyLen % 2;
      sp.ringPos_trailingWMAIdx = 0;
      sp.ringCap_trailingWMAIdx = cap_trailingWMAIdx;
      sp.ring_trailingWMAIdx_inReal = capRing_trailingWMAIdx_inReal;
      sp.cur_outMAMA = outMAMA[outNBElement.value - 1];
      sp.cur_outFAMA = outFAMA[outNBElement.value - 1];
      sp.cachedValue = new MamaStream.Value(sp.cur_outMAMA, sp.cur_outFAMA);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind mamaOpen (composition seam). */
   MamaStream mamaOpenInternal( double inReal[], int startIdx, double optInFastLimit, double optInSlowLimit )
   {
      MamaStream sp = new MamaStream(this);
      RetCode retCode = mamaOpenBody(sp, inReal, startIdx, optInFastLimit, optInSlowLimit);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MAMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MAMA open: internal error");
      }
      throw new IllegalArgumentException("TA_MAMA open: " + retCode);
   }
   /**
    * Open a live MAMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#mama} at that bar.
    * <p>The history must hold at least {@code mamaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MamaStream mamaOpen( double inReal[], double optInFastLimit, double optInSlowLimit )
   {
      return mamaOpenInternal(inReal, 0, optInFastLimit, optInSlowLimit);
   }
   /**
    * {@link Core#mamaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#mama} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public MamaStream mamaOpenAndFill( double inReal[], double optInFastLimit, double optInSlowLimit, MInteger outBegIdx, MInteger outNBElement, double outMAMA[], double outFAMA[] )
   {
      MamaStream sp = new MamaStream(this);
      RetCode retCode = mamaOpenAndFillBody(sp, inReal, optInFastLimit, optInSlowLimit, outBegIdx, outNBElement, outMAMA, outFAMA);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MAMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MAMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_MAMA openAndFill: " + retCode);
   }
