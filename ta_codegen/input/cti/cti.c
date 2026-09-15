/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091526 KL     First version (proposal-drafts issue #74).
 */

int cti_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode cti(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sumX, sumX2, sumXY, x, trailingX, leavingX;
   double shift, ssX, spXY, tempReal, invPeriod, dPeriod, sumY, ssY;
   int lookbackTotal, outIdx, today, trailingIdx, windowStart, j;
   int barsSinceReseed;

   lookbackTotal = optInTimePeriod-1;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx  = startIdx;
   trailingIdx = startIdx - lookbackTotal;

   dPeriod   = (double)optInTimePeriod;
   invPeriod = 1.0 / dPeriod;

   /* The ramp side is data-independent and collapses to two constants. With y
    * running 0..optInTimePeriod-1 as BARS AGO,
    *
    *    sumY = n(n-1)/2            ssY = sumY2 - sumY*sumY/n = n(n*n-1)/12
    *
    * Both are computed in double because their integer forms overflow: at the
    * top of the parameter range n*n*n is 1e15, far past what an int holds, and
    * the product would wrap silently rather than fail. ssY is strictly positive
    * for every n >= 2, which is why only the price side can make a window
    * degenerate.
    */
   sumY = dPeriod * (dPeriod-1.0) * 0.5;
   ssY  = dPeriod * (dPeriod*dPeriod - 1.0) / 12.0;

   /* Measure the price side against a shift near the window, as correl.c does
    * (#242). Transcribing the author's listing literally would carry
    * n*Sxx - Sx*Sx on raw price levels, which is the quantity that returned 0,
    * -1 and -1.73 from a perfectly correlated pair: MEASURED on the card, the
    * naive form errs by 5.7e-03 at a 1e2 price level with a 1e-5 spread, on an
    * indicator whose entire range is [-1, +1].
    *
    * Anchor on the first window value here; every later re-anchor uses the
    * window mean, which is better centred but costs a pass this one cannot
    * afford before the sums exist.
    */
   shift = inReal[trailingIdx];

   /* The initial window, less its last bar. This bar's y is startIdx-j, since
    * the first output is computed with `today` at startIdx.
    */
   sumX = sumX2 = sumXY = 0.0;
   for( j=trailingIdx; j < startIdx; j++ )
   {
      x = inReal[j] - shift;
      sumX  += x;
      sumX2 += x*x;
      sumXY += x * (double)(startIdx - j);
   }

   today = startIdx;
   outIdx = 0;
   barsSinceReseed = 32 * optInTimePeriod;
   leavingX = 0.0;

   do
   {
      /* The incoming bar is zero bars ago, so it moves sumX and sumX2 and
       * leaves sumXY alone.
       */
      x = inReal[today] - shift;
      sumX  += x;
      sumX2 += x*x;

      ssX  = sumX2 - ((sumX*sumX)*invPeriod);
      spXY = sumXY - ((sumX*sumY)*invPeriod);

      /* Re-anchor and rebuild when the shift has gone stale: the price sum of
       * squares has shrunk below 1e-6 of the squared deviations it is extracted
       * from; OR the value the PREVIOUS bar removed sat so far from the shift
       * that its squared term dwarfs what remains; OR at least every 32
       * windows. Same three triggers as correl.c, watching the price side only
       * -- the ramp side is exact constants and cannot drift.
       *
       * A vanishing spXY is NOT a trigger. It is a legitimate answer, a window
       * with no linear trend, and reseeding on it would rebuild on every bar of
       * ordinary sideways data.
       */
      barsSinceReseed--;
      if( ssX < 0.000001 * sumX2
         || leavingX > 1000000.0 * sumX2
         || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInTimePeriod;
         windowStart = today - lookbackTotal;

         tempReal = 0.0;
         for( j=windowStart; j <= today; j++ )
            tempReal += inReal[j];
         shift = tempReal*invPeriod;

         sumX = sumX2 = sumXY = 0.0;
         for( j=windowStart; j <= today; j++ )
         {
            x = inReal[j] - shift;
            sumX  += x;
            sumX2 += x*x;
            sumXY += x * (double)(today - j);
         }

         ssX  = sumX2 - ((sumX*sumX)*invPeriod);
         spXY = sumXY - ((sumX*sumY)*invPeriod);

         /* A sum of squares is non-negative by definition, but this one is
          * extracted as a difference, so its SIGN is not guaranteed on a window
          * sitting inside a flat stretch. Enforced here rather than at the
          * divide, exactly as correl.c does: a negative ssX always reseeds on
          * the same bar, so the divide below can rely on it being >= 0.
          */
         if( ssX < 0.0 )
            ssX = 0.0;
      }

      /* Save the trailing value before writing the output, since the input and
       * output might be the same array.
       */
      trailingX = inReal[trailingIdx] - shift;
      trailingIdx++;

      /* THE SIGN. y here is BARS AGO, so it runs backward in time and a rising
       * series correlates NEGATIVELY with it. Ehlers' listing counts the same
       * way and takes Y = -count, which is the positively-sloped line the
       * indicator is defined against; negating the coefficient once is the same
       * thing, and it keeps the O(1) slide identity below written the way
       * linearreg.c:102-114 states it. Dropping this negation silently inverts
       * the whole indicator -- Pearson r is odd in either variable -- and a
       * magnitude or |r| assertion cannot see it.
       *
       * ssY is a positive constant, so only ssX can make the window degenerate.
       * It is tested against its own scale rather than an absolute band: the
       * product carries the fourth power of the window's spread, so a fixed
       * threshold rejects a well-defined correlation as soon as the data is
       * small. The product is tested on its own because neither factor's test
       * implies it -- at that fourth power it can underflow to exactly 0.0
       * while both are still ordinary normals, and a zero divisor there gives
       * NaN, which the clamp does not catch.
       *
       * An all-flat window therefore emits exactly 0.0 rather than NaN, which
       * is correl.c's precedent and what #112 requires of a successful call.
       */
      if( ssX > 0.00000000000001*sumX2 && ssX*ssY > 0.0 )
      {
         tempReal = -spXY / sqrt(ssX*ssY);

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

      /* Slide the window one bar in O(1). Advancing it ages every retained
       * value by one bar, which raises each of their weights by 1 and so adds
       * the whole deviation sum; the departing value leaves at the full weight
       * optInTimePeriod-1 and also loses its place in that sum, which is the
       * remaining -1. The two together are exactly -optInTimePeriod*trailingX,
       * and sumX must still be the COMPLETE window's sum when it is read here
       * -- it is decremented on the line below, not above.
       */
      leavingX = trailingX*trailingX;
      sumXY = sumXY + sumX - dPeriod*trailingX;
      sumX  -= trailingX;
      sumX2 -= leavingX;

      today++;
   } while( today <= endIdx );

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
