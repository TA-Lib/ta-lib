/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  JP       John Price <jp_talib@gcfl.net>
 *  MF       Mario Fortier
 *  AM       Adrian Michel <http://amichel.com>
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  070203 JP      Initial.
 *  072106 MF,AM   Fix #1526632. Add missing atan().
 *  071326 MF,CC   O(period) per-bar rescan -> O(1) sliding-sum recurrence
 *                 (numerics-changing). See issue #103.
 *  072026 MF,CC   Read the departing value before the output write so in-place
 *                 (outReal==inReal) calls stay correct. See issue #130.
 *  082426 MF,CC  Fix #254. Re-anchor the running sums: every 32*period bars,
 *                and on the bar a large value leaves the window.
 */

int linearreg_angle_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode linearreg_angle(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx;

   int today, lookbackTotal, trailingIdx;
   double SumX, SumXY, SumY, SumXSqr, Divisor;

   double m;

   int i, j, windowStart, barsSinceReseed;

   double tempValue1, tempValue2, trailingValue, weightedTrailing, sumAbs;

   /* Linear Regression is a concept also known as the
    * "least squares method" or "best fit." Linear
    * Regression attempts to fit a straight line between
    * several data points in such a way that distance
    * between each data point and the line is minimized.
    *
    * For each point, a straight line over the specified
    * previous bar period is determined in terms
    * of y = b + m*x:
    *
    * TA_LINEARREG          : Returns b+m*(period-1)
    * TA_LINEARREG_SLOPE    : Returns 'm'
    * TA_LINEARREG_ANGLE    : Returns 'm' in degree.
    * TA_LINEARREG_INTERCEPT: Returns 'b'
    * TA_TSF                : Returns b+m*(period)
    */

   /* Adjust startIdx to account for the lookback period. */
   lookbackTotal = linearreg_angle_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   outIdx = 0; /* Index into the output. */
   today = startIdx;
   trailingIdx = startIdx - lookbackTotal;

   SumX = (double)optInTimePeriod * ( optInTimePeriod - 1 ) * 0.5;
   SumXSqr = (double)optInTimePeriod * ( optInTimePeriod - 1 ) * ( 2 * optInTimePeriod - 1 ) / 6.0;
   Divisor = SumX * SumX - optInTimePeriod * SumXSqr;

   /* Prime the two data-dependent window sums for the first output with a
    * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
    * SumY = sum of the window, SumXY = sum of i*value (i the reversed
    * 0..period-1 position). */
   SumXY = 0;
   SumY = 0;
   sumAbs = 0;
   for( i = optInTimePeriod; i-- != 0; )
   {
      SumY += tempValue1 = inReal[today - i];
      SumXY += (double)i * tempValue1;
      sumAbs += fabs(tempValue1);
   }
   m = ( optInTimePeriod * SumXY - SumX * SumY) / Divisor;
   barsSinceReseed = 32 * optInTimePeriod;
   trailingValue = inReal[trailingIdx];
   trailingIdx++;
   outReal[outIdx++] = atan(m) * ( 180.0 / 3.14159265358979323846 );
   today++;

   /* Slide the window one bar at a time, keeping both sums in O(1): advancing
    * the window raises every retained value's weight by 1 (adds SumY) and drops
    * the departing value at full weight (subtracts period*trailingValue). Same
    * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
    * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
    * Each departing value is read before the output write of the same bar:
    * with outReal==inReal (in-place, #130) that write lands on the cell the
    * next iteration departs from. */
   while( today <= endIdx )
   {
      weightedTrailing = (double)optInTimePeriod * trailingValue;
      SumXY = SumXY + SumY - weightedTrailing;
      SumY = SumY - trailingValue + inReal[today];
      sumAbs = sumAbs - fabs(trailingValue) + fabs(inReal[today]);

      /* Re-anchor: rebuild both sums from the window itself. #103 left them as
       * running totals that are never rebuilt, so each bar's rounding joins a
       * residue no later bar can subtract -- unbounded in the length of the
       * call, and scaled by the largest value the sums have EVER held rather
       * than by what the window holds now. Two triggers, and they cover
       * different failures (issue #254):
       *
       *   - every 32*period bars, so a slow drift stays bounded however long
       *     the series runs. Same interval as TA_VAR / TA_CORREL / TA_BETA.
       *
       *   - when the value the window just dropped carries more weight than
       *     everything left in it. That is the one the interval cannot cover:
       *     one large print inflates the residue for up to 32*period bars
       *     after it is gone (measured 31x at period 5), and this rebuilds on
       *     the bar it leaves instead.
       *
       * The threshold compares two DEGREE-1 quantities, which is why it is 100
       * and not TA_CORREL's 1e6 -- that guard weighs a squared deviation
       * against a sum of squares. On ordinary prices the ratio is ~1 and this
       * never fires; it is a compare, not work. The constant is 100 rather than
       * 10 because at 10 a zero-mean oscillator rebuilds on 8.8% of bars for no
       * measured accuracy gain.
       *
       * THE DENOMINATOR IS sumAbs, NOT SumY, AND THAT IS THE WHOLE POINT.
       * SumY is a CANCELLING sum: on a zero-mean window it collapses toward 0
       * while the departing value does not, so |weightedTrailing|/|SumY| is
       * unbounded and the rebuild fires on EVERY bar -- an alternating +/-1
       * series measured 10.9x slower at period 30, which is precisely the
       * O(n*period) cost #103 removed. Same shape of error as #242's absolute
       * guard on a quartic quantity: a ratio test is ill-posed when its
       * denominator can cancel. sumAbs is a sum of magnitudes, so it is 0 only
       * when every value in the window is 0 -- and then the numerator is 0 too
       * and the test is false. There is no window it can misjudge.
       *
       * It is also the RIGHT quantity on the merits, not just the safe one: a
       * fresh rebuild's own error is ~eps*sum|y|, so comparing the departing
       * term against sum|y| asks exactly "would rebuilding beat what we are
       * carrying?".
       *
       * Carrying it is free in practice. Measured on the shipped libta-lib.a it
       * costs nothing against the |SumY| form on a price series (1.541 vs 1.605
       * ns/bar at period 14) because the update is INDEPENDENT of the serial
       * SumXY -> SumY dependency chain and fills slots that were idle. The
       * rejected alternative -- keeping |SumY| and rate-limiting the trigger to
       * once per `period` bars -- bounded the cliff at 1.2x rather than removing
       * it, and silently dropped any print departing within `period` bars of a
       * rebuild (~3% of them).
       *
       * The scan walks the window oldest-first with the weight counting DOWN,
       * which is the priming scan's order and weighting -- so a reseeded bar is
       * bit-identical to the same bar computed by a call that started there.
       * That identity is the whole point: it is what the range-stability
       * contract measures.
       *
       * Reading the window is safe when outReal aliases inReal (#130): the
       * outputs written so far occupy [0, outIdx-1], and windowStart is
       * today-lookbackTotal, which is >= outIdx because startIdx was clamped
       * to at least lookbackTotal. */
      barsSinceReseed--;
      if( barsSinceReseed <= 0 || fabs(weightedTrailing) > 100.0 * sumAbs )
      {
         barsSinceReseed = 32 * optInTimePeriod;
         windowStart = today - lookbackTotal;
         SumY = 0;
         SumXY = 0;
         sumAbs = 0;
         tempValue2 = (double)lookbackTotal;
         for( j = windowStart; j <= today; j++ )
         {
            tempValue1 = inReal[j];
            SumY += tempValue1;
            SumXY += tempValue2 * tempValue1;
            sumAbs += fabs(tempValue1);
            tempValue2 -= 1.0;
         }
      }
      m = ( optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      trailingValue = inReal[trailingIdx];
      trailingIdx++;
      outReal[outIdx++] = atan(m) * ( 180.0 / 3.14159265358979323846 );
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
