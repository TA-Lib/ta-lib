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
   double tempReal, shift, periodTotal1, periodTotal2, meanValue1, variance, invPeriod;
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
   do
   {
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
      trailingIdx++;

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
      barsSinceReseed--;
      if( variance < 0.000001 * ( periodTotal2 * invPeriod )
         || tempReal > 1000000.0 * periodTotal2
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
         /* Floor the fresh figure at the same ratio the trigger above uses, now
          * measured against the RE-ANCHORED sums. With the shift AT the window
          * mean the deviations sum to ~0, so a real window has variance ~
          * periodTotal2*invPeriod and a ratio of ~1; the ratio drops toward 0
          * only when every deviation is the same value, i.e. when the spread is
          * at or under the rounding error of the mean itself. There is then no
          * spread the anchor could resolve, the surviving digits are noise, and
          * the honest answer is 0.
          *
          * The constant is 1e-12, NOT the 1e-6 the trigger above uses, and the
          * difference is load-bearing. periodTotal2*invPeriod is not the
          * variance here: it is variance + e^2, where e is the rounding error of
          * the reseed's own left-to-right sum for the mean -- exactly the term
          * the two-pass subtraction then cancels out. So the ratio measures how
          * badly that sum rounded, not how much signal survives, and matching
          * the trigger's 1e-6 fired ten orders before cancellation eats any
          * digits. It zeroed a variance the line above had just computed to nine
          * correct significant figures: 100011 bars at 31498938283.624615 with
          * two small outliers at period 99991 gives 1.0219900060103338e-09
          * (128-bit), and this returned 0 with TA_SUCCESS. At 1e-12 that window
          * survives and every intended bit-zero still zeroes -- the live ratios
          * on flat data are 0 or ~1e-16, six orders the other side.
          *
          * This is the ONE dead-zone in the var/stddev/bbands family, and it is
          * relative rather than the `variance < 0.0` it replaced because two
          * things ride on it:
          *
          *  - SIGN. periodTotal2 is a fresh sum of squares, so the right-hand
          *    side is >= 0 and any negative variance is clamped unconditionally -
          *    where `< 0.0` needed the three-case argument below to know that a
          *    negative one ever reaches this line.
          *  - SCALE. STDDEV and BBANDS square-root this, and each used to zero
          *    anything under a fixed TA_EPSILON first. That compares a SQUARED
          *    quantity to 1e-14, which is a cliff at a price level and not a
          *    noise floor: a $100.00 instrument quoted in 1e-8 ticks has a real
          *    variance around 1e-16 and came back exactly 0 on every bar (#243).
          *    Expressed here in the window's own units, the floor lets both of
          *    them square-root what they are handed unconditionally.
          *
          * Clamping HERE and not at the output write is what keeps this off the
          * per-bar path, and it is sufficient because a negative variance always
          * reseeds on the same bar - the guard above covers all three cases:
          * periodTotal2 > 0 makes its first disjunct `negative < positive`;
          * periodTotal2 < 0 makes the second disjunct's right side negative,
          * which the squared tempReal always exceeds; periodTotal2 == 0 reduces
          * the first to `variance < 0`. CHANGING THAT GUARD MEANS RE-CHECKING
          * THIS - the alternative is an unconditional clamp at the output write,
          * which needs no such argument but does cost ~3%.
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
