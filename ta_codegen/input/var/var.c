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
 *  082326 MF,CC  #243 reseed floor is scale-relative, not `variance < 0`.
 *  092226 MF,CC  #434 rebuild against the peak sum of squares; re-anchor a flat window.
 */

int var_lookback(int optInTimePeriod, double optInNbDev)
{
   (void)optInNbDev;

   return optInTimePeriod-1;
}

TA_RetCode var(int startIdx, int endIdx,
   const double *inReal,
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double *outReal)
{
   double tempReal, shift, periodTotal1, periodTotal2, meanValue1, variance, invPeriod, peakTotal2;
   int i, j, outIdx, trailingIdx, windowStart, nbInitialElementNeeded, barsSinceReseed;

   /* Identify the minimum number of price bar needed to calculate
    * at least one output.
    */
   nbInitialElementNeeded = (optInTimePeriod-1);

   /* Move up the start index if there is not enough initial data. */
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
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
   for( j=trailingIdx; j < startIdx; j++ )
   {
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
   peakTotal2 = periodTotal2;
   do
   {
      /* Add the incoming value, measured against the shift. */
      tempReal = inReal[i] - shift;
      periodTotal1 += tempReal;
      tempReal *= tempReal;
      periodTotal2 += tempReal;
      peakTotal2 = ( periodTotal2 > peakTotal2 ) ? periodTotal2 : peakTotal2;

      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;

      /* Remove the trailing value (prepares the next window). */
      tempReal = inReal[trailingIdx] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
      trailingIdx++;

      /* Rebuild with a fresh two-pass when the variance has shrunk below 1e-6
       * of the LARGEST mean squared deviation held since the last rebuild, or at
       * least every 32 windows. Measure against that peak, not the current sum:
       * the rounding the running sums carry scales with the peak, so once a
       * series settles back near the shift, or an outlier leaves the window,
       * the current sum holds nothing but that rounding. The collapse is seen
       * on the first bar whose sums carry it, and the rebuild recomputes that
       * bar.
       */
      barsSinceReseed--;
      if( variance < 0.000001 * ( peakTotal2 * invPeriod )
         || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInTimePeriod;

         windowStart = i - nbInitialElementNeeded;

         tempReal = 0.0;
         for( j=windowStart; j <= i; j++ )
            tempReal += inReal[j];
         shift = tempReal * invPeriod;

         periodTotal1 = 0.0;
         periodTotal2 = 0.0;
         for( j=windowStart; j <= i; j++ )
         {
            tempReal = inReal[j] - shift;
            periodTotal1 += tempReal;
            tempReal *= tempReal;
            periodTotal2 += tempReal;
         }

         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;

         /* A window flat to within the rounding of its own mean leaves the
          * variance at that rounding, which would fire the trigger again on
          * every bar. Anchored on one of its own values it cannot: the variance
          * is then at least 1/(2n) of the mean square it is extracted from.
          */
         if( variance < 0.000001 * ( periodTotal2 * invPeriod ) )
         {
            shift = inReal[i];
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j=windowStart; j <= i; j++ )
            {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         }
         /* Before the re-remove below: the peak must hold the whole window. */
         peakTotal2 = periodTotal2;

         /* After the re-anchor a window with any spread sits orders above this
          * floor, so it catches only a variance that rounding left at or below
          * 0. That keeps the output non-negative, which lets STDDEV and BBANDS
          * square-root it unconditionally (#243). A negative variance always
          * gets here: the peak is never negative, so the trigger fires on it.
          */
         if( variance < 0.000000000001 * ( periodTotal2 * invPeriod ) )
            variance = 0.0;

         /* Re-remove the trailing value under the new shift so the carried state
          * matches the non-reseed path.
          */
         tempReal = inReal[windowStart] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
      }

      outReal[outIdx++] = variance;
      i++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
