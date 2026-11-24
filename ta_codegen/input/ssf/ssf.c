/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112326 MF,CC  First version (issue #609).
 */

int ssf_lookback(int optInTimePeriod)
{
   int structural;

   if( optInTimePeriod == 1 )
      return TA_UNSTABLE( TA_FUNC_UNST_SSF, 0 );

   /* The seed occupies three bars, so the first output is a recursion step
    * even at periods 2 and 3; from 4 up the lookback is EMA's P-1.
    *
    * Auto: the poles are a complex pair at radius exp(-1.414*3.14159/P), so an
    * e-fold costs P/4.44221 bars, taken as 7P/31. Two output slots carry the
    * seed, so the difference of two starts is R*r^k*cos(k*w + phi) with R and
    * phi free; against the largest difference it ever shows, the worst phase
    * leaves ln(R/S) at most 2.51 (P = 2), 1.39 at a long period. The fixed
    * part 2 covers it at every period with the 7/31 rounding (rules_check.py).
    */
   structural = optInTimePeriod - 1;
   if( structural < 3 )
      structural = 3;

   return structural
   + TA_UNSTABLE( TA_FUNC_UNST_SSF, (7 * (K + 2) * optInTimePeriod + 30) / 31 );
}

TA_RetCode ssf(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx, today, lookbackTotal;
   double a1, s, c1h, c2, c3;
   double x0, x1, y, y1, y2;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = ssf_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* No smoothing at period 1: the output is a copy of the input, as MA gives
    * for every MAType. The unstable period still delays the first output.
    */
   if( optInTimePeriod == 1 )
   {
      *outBegIdx = startIdx;
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx )
         outReal[outIdx++] = inReal[today++];
      *outNBElement = outIdx;
      return TA_SUCCESS;
   }

   /* Ehlers' constants as he prints them: 1.414 and 3.14159 in the radius,
    * and Cosine(1.414*180/P) in degrees, so a full-precision pi in the angle.
    *
    * Keep the cosine as 1 - 2*sin(w/2)^2: cos(w) is close to 1 at a long
    * period, and a one-ULP libm difference in it moves c1 by about
    * 5.6e-18*P^2 relative; the half angle does not.
    */
   a1 = exp( -1.414 * 3.14159 / (double)optInTimePeriod );
   s  = sin( 1.414 * 3.14159265358979323846 / (2.0 * (double)optInTimePeriod) );
   c2 = 2.0 * a1 * (1.0 - 2.0 * s * s);
   c3 = -( a1 * a1 );

   /* c1 = 1 - c2 - c3 makes the DC gain 1 whatever the constants. Halving it
    * here is exact, so c1h*(x + x1) is c1*(x + x1)/2.
    */
   c1h = 0.5 * ((1.0 - c2) - c3);

   /* Ehlers' guard: the first three bars of the history are the price. The
    * input slot is carried in a local and never re-read, because outReal may
    * alias inReal.
    */
   today = startIdx - lookbackTotal;
   y2 = inReal[today + 1];
   y1 = inReal[today + 2];
   x1 = y1;
   today += 3;

   /* Skip the rest of the lookback: run the recurrence, publish nothing. */
   i = lookbackTotal - 3;
   while( i != 0 )
   {
      x0 = inReal[today];
      y  = c2 * y1 + (c3 * y2 + c1h * (x0 + x1));
      x1 = x0;
      y2 = y1;
      y1 = y;
      today++;
      i--;
   }

   outIdx = 0;
   while( today <= endIdx )
   {
      /* c2*y1 stays the outermost term: y1 is the newest output, so that keeps
       * the bar rate at one fused step.
       */
      x0 = inReal[today];
      y  = c2 * y1 + (c3 * y2 + c1h * (x0 + x1));
      x1 = x0;
      y2 = y1;
      y1 = y;

      outReal[outIdx] = y;
      outIdx = outIdx + 1;
      today++;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
