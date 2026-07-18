/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  100502 JV     Speed optimization of the algorithm
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  071726 MF,CC  #118 cancellation-free variance (shifted sums + reseed); fixes bug 90.
 */

   public int varianceLookback( int optInTimePeriod, double optInNbDev )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode variance( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInTimePeriod,
                            double optInNbDev,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
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
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      /* Identify the minimum number of price bar needed to calculate
       * at least one output.
       */
      nbInitialElementNeeded = optInTimePeriod - 1;
      /* Move up the start index if there is not enough initial data. */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      /* Measure deviations against a shift near the window: the running sums
       * periodTotal1 = sum(inReal-shift) and periodTotal2 = sum((inReal-shift)^2)
       * stay at variance scale, so variance = periodTotal2/period - mean^2 no longer
       * subtracts two ~mean^2 quantities. Anchor the shift to the first window value
       * (also gives an exact 0 for period 1, with no division by period-1).
       */
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = inReal[trailingIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         tempReal = inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      /* inReal and outReal may be the same buffer: each trailing value is consumed
       * before its slot is overwritten by the output.
       */
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      do {
         /* Add the incoming value, measured against the shift. */
         tempReal = inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         /* Remove the trailing value (prepares the next window). */
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         /* Re-anchor the shift and rebuild the running sums with a fresh two-pass
          * when the shift is stale enough that the subtraction loses digits - i.e.
          * the variance has shrunk below 1e-6 of the mean squared deviation it is
          * extracted from (that ratio bounds the cancellation error to ~eps/1e-6 ~
          * 2e-10, so partial cancellation, not just total collapse, is caught); OR
          * when the value just removed sat so far from the shift that its squared term
          * (tempReal) dwarfs the surviving sum (a large outlier passing through the
          * window buries the small terms below its ulp, and the residual left when it
          * leaves is cancellation garbage); OR at least every 32 windows so a slow
          * drift stays bounded regardless of the series length. The strict `<` also
          * leaves an exactly-constant window (variance 0, scale 0) alone instead of
          * reseeding it every bar. Guarantees a non-negative output.
          */
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            /* Re-remove the trailing value under the new shift so the carried state
             * matches the non-reseed path.
             */
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         outReal[outIdx++] = variance;
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode varianceUnguarded( int startIdx,
                                     int endIdx,
                                     double inReal[],
                                     int optInTimePeriod,
                                     double optInNbDev,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = inReal[trailingIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         tempReal = inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      do {
         tempReal = inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         outReal[outIdx++] = variance;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode variance( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInTimePeriod,
                            double optInNbDev,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
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
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = (double)inReal[trailingIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         tempReal = (double)inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      do {
         tempReal = (double)inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = (double)inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += (double)inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal = (double)inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            tempReal = (double)inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         outReal[outIdx++] = variance;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode varianceUnguarded( int startIdx,
                                     int endIdx,
                                     float inReal[],
                                     int optInTimePeriod,
                                     double optInNbDev,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = (double)inReal[trailingIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         tempReal = (double)inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      do {
         tempReal = (double)inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = (double)inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += (double)inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal = (double)inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            tempReal = (double)inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         outReal[outIdx++] = variance;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
