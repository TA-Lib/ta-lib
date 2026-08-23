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
 *  101003 MF   Initial Coding
 *  062804 MF   Resolve div by zero bug on limit case.
 *  082326 MF   Fix #242. Cancellation-free sums (shifted data + reseed, as
 *              TA_VAR does since #118), per-factor degeneracy test and a
 *              range clamp.
 *
 */

int correl_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode correl(int startIdx, int endIdx,
   const double inReal0[],
   const double inReal1[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sumXY, sumX, sumY, sumX2, sumY2, x, y, trailingX, trailingY;
   double shiftX, shiftY, ssX, ssY, spXY, leavingX, leavingY;
   double tempReal, invPeriod;
   int lookbackTotal, today, trailingIdx, outIdx, j, windowStart, barsSinceReseed;

   /* Move up the start index if there is not
    * enough initial data.
    */
   /* One reciprocal instead of three divisions per bar, as TA_VAR does. The
    * extra rounding it costs is invisible next to what the shift recovers, and
    * it is what keeps this form cheaper than the one it replaces.
    */
   invPeriod = 1.0 / (double)optInTimePeriod;

   lookbackTotal = optInTimePeriod-1;
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx  = startIdx;
   trailingIdx = startIdx - lookbackTotal;

   /* Measure both series against a shift near the window, exactly as TA_VAR
    * does (#118). The running sums then hold deviations rather than raw levels,
    * so ssX = sumX2-(sumX*sumX)*invPeriod is no longer a difference of two
    * ~period*mean^2 quantities. Without this the extracted sum of squares keeps
    * only the digits that survive that subtraction: at a $100 price level with a
    * 1e-5 spread that is three of them, and the correlation of two perfectly
    * correlated series came back as 0, as -1, or as -1.73 (#242).
    *
    * Anchor on the first window value here; every later re-anchor uses the
    * window mean, which is better centred but costs a pass this one cannot
    * afford before the sums exist.
    */
   shiftX = inReal0[trailingIdx];
   shiftY = inReal1[trailingIdx];

   /* Calculate the initial values (the window less its last bar). */
   sumXY = sumX = sumY = sumX2 = sumY2 = 0.0;
   for( j=trailingIdx; j < startIdx; j++ )
   {
      x = inReal0[j] - shiftX;
      sumX  += x;
      sumX2 += x*x;

      y = inReal1[j] - shiftY;
      sumXY += x*y;
      sumY  += y;
      sumY2 += y*y;
   }

   today = startIdx;
   outIdx = 0;
   barsSinceReseed = 32 * optInTimePeriod;
   leavingX = 0.0;
   leavingY = 0.0;

   do
   {
      /* Add the incoming value, measured against the shift. */
      x = inReal0[today] - shiftX;
      sumX  += x;
      sumX2 += x*x;

      y = inReal1[today] - shiftY;
      sumXY += x*y;
      sumY  += y;
      sumY2 += y*y;

      ssX  = sumX2 - ((sumX*sumX)*invPeriod);
      ssY  = sumY2 - ((sumY*sumY)*invPeriod);
      spXY = sumXY - ((sumX*sumY)*invPeriod);

      /* Re-anchor and rebuild with a fresh two-pass when the shift has gone
       * stale. Same three triggers as TA_VAR: either sum of squares has shrunk
       * below 1e-6 of the squared deviations it is extracted from; OR the value
       * the PREVIOUS bar removed sat so far from the shift that its squared term
       * dwarfs what remains (a large outlier transiting the window buries the
       * small terms below its ulp, and the residue it leaves is cancellation
       * garbage); OR at least every 32 windows, so a slow drift stays bounded
       * however long the series runs.
       *
       * One bar late is correct, not a compromise. leavingX/leavingY are set by
       * the removal at the BOTTOM of the loop, so the bar on which the outlier
       * actually leaves still computes its own output from sums that legitimately
       * contain it. The trigger then fires on the NEXT bar -- the first one whose
       * sums carry the residue -- and the reseed below recomputes that bar's
       * output before it is written. No bar is ever emitted from the residue.
       *
       * The triggers watch ssX and ssY only, never spXY. A vanishing spXY is a
       * legitimate answer - two uncorrelated series - not a loss of digits, and
       * reseeding on it would rebuild the window on every bar of ordinary data.
       * This is where the analogy with TA_VAR stops: variance has one extracted
       * quantity and all of it is signal.
       *
       * Reading the window here is safe when outReal aliases an input: the
       * outputs written so far occupy [0, outIdx-1] while windowStart is
       * startIdx-lookbackTotal+outIdx, which is >= outIdx.
       */
      barsSinceReseed--;
      if( ssX < 0.000001 * sumX2
         || ssY < 0.000001 * sumY2
         || leavingX > 1000000.0 * sumX2
         || leavingY > 1000000.0 * sumY2
         || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInTimePeriod;
         windowStart = today - lookbackTotal;

         /* Both means in one pass over the window: the rebuild below is the
          * only O(period) work on this function's hot path, so it is walked
          * twice, not three times.
          */
         tempReal = 0.0;
         shiftY = 0.0;
         for( j=windowStart; j <= today; j++ )
         {
            tempReal += inReal0[j];
            shiftY   += inReal1[j];
         }
         shiftX = tempReal*invPeriod;
         shiftY = shiftY*invPeriod;

         sumXY = sumX = sumY = sumX2 = sumY2 = 0.0;
         for( j=windowStart; j <= today; j++ )
         {
            x = inReal0[j] - shiftX;
            sumX  += x;
            sumX2 += x*x;

            y = inReal1[j] - shiftY;
            sumXY += x*y;
            sumY  += y;
            sumY2 += y*y;
         }

         ssX  = sumX2 - ((sumX*sumX)*invPeriod);
         ssY  = sumY2 - ((sumY*sumY)*invPeriod);
         spXY = sumXY - ((sumX*sumY)*invPeriod);

         /* A sum of squares is non-negative by definition, but this one is
          * extracted as a difference, so its SIGN is not guaranteed on a window
          * sitting inside a flat stretch. Enforce the invariant HERE and not at
          * the divide: a negative ssX always reseeds on the same bar (it makes
          * the first trigger's `negative < non-negative` true whenever sumX2 is
          * positive, and sumX2 == 0 reduces that trigger to `ssX < 0`), so the
          * divide below can rely on both being >= 0 and needs no sign test of
          * its own. CHANGING THE TRIGGERS MEANS RE-CHECKING THIS.
          */
         if( ssX < 0.0 )
            ssX = 0.0;
         if( ssY < 0.0 )
            ssY = 0.0;
      }

      /* Save the trailing values before writing the output, since the input
       * and output might be the same array.
       */
      trailingX = inReal0[trailingIdx] - shiftX;
      trailingY = inReal1[trailingIdx] - shiftY;
      trailingIdx++;

      /* Output the new coefficient.
       *
       * Each sum of squares is tested against its OWN scale, not the pair
       * against a fixed band. The product ssX*ssY carries the fourth power of
       * the window's spread, so an absolute threshold on it rejects a perfectly
       * well-defined correlation as soon as the data is small - and, worse,
       * lets a pair of NEGATIVE sums through, their signs cancelling into a
       * plausible-looking result of the wrong sign. Testing each factor
       * separately is what forecloses both.
       *
       * The literal is TA_EPSILON. This is deliberately NOT TA_IS_ZERO_SCALED,
       * whose fabs() would admit a LARGE NEGATIVE ssX -- exactly the operand
       * that must never reach the square root. A plain `>` rejects it, and it
       * is also the cheaper test: the two fabs() cost ~7% of this function's
       * runtime, and buy a wrong answer.
       *
       * sqrt(ssX*ssY) rather than sqrt(ssX)*sqrt(ssY): the guard has already
       * established both are positive, so the product needs no protection from
       * a negative operand, and the second square root is worth ~25% of the
       * runtime.
       *
       * The product CAN overflow to +Inf, and the one-root form is chosen with
       * that known. TA_REAL_MAX bounds optional PARAMETERS; a batch call's input
       * arrays are not range-checked, so ssX and ssY are bounded only by the
       * double range and their product exceeds it once |x| passes ~1e154. The
       * two-root form would not overflow there -- but the form this replaces
       * built exactly the same product (it tested ssX*ssY against TA_EPSILON), so
       * the exposure is unchanged, and an Inf here yields 0.0 rather than a wrong
       * correlation. Trading a quarter of the runtime for a case that already
       * behaved this way, on inputs 117 orders past any price, is not a trade
       * worth making. Revisit only if input range-checking is ever added.
       */
      if( ssX > 0.00000000000001*sumX2 && ssY > 0.00000000000001*sumY2 )
      {
         tempReal = spXY / sqrt(ssX*ssY);

         /* A correlation coefficient cannot leave [-1,1]; rounding in the
          * three sums can still put it a few ulp outside.
          */
         if( tempReal > 1.0 )
            tempReal = 1.0;
         else if( tempReal < -1.0 )
            tempReal = -1.0;
         outReal[outIdx++] = tempReal;
      }
      else
         outReal[outIdx++] = 0.0;

      /* Remove the trailing values (prepares the next window). */
      leavingX = trailingX*trailingX;
      leavingY = trailingY*trailingY;
      sumX  -= trailingX;
      sumX2 -= leavingX;

      sumXY -= trailingX*trailingY;
      sumY  -= trailingY;
      sumY2 -= leavingY;

      today++;
   } while( today <= endIdx );

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
