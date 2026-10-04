/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100126 KL,CC  Creation (#486).
 *  100326 MF,CC  The newest output on one fused step (#486).
 */

int swak_2php_lookback(int optInTimePeriod)
{
   (void)optInTimePeriod;

   /* No structural lookback: the two input slots and the two output slots are
    * seeded from the first bar rather than read from before it, and there is
    * no callee whose lookback could be inherited.
    */
   return TA_GetUnstablePeriod(TA_FUNC_UNST_SWAK_2PHP);
}

TA_RetCode swak_2php(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx, today, lookbackTotal;
   double w, b2p, a2p, om, c0, a1, a2;
   double x0, x1, x2, y, y1, y2;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = swak_2php_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* The two-pole alpha of Ehlers' "Swiss Army Knife Indicator", Figure 5. The
    * paper's 360/P is a full turn, so 2*pi/P.
    */
   w   = (2.0 * 3.14159265358979323846) / (double)optInTimePeriod;
   b2p = 2.415 * (1.0 - cos(w));
   a2p = -b2p + sqrt( b2p*b2p + 2.0*b2p );

   /* The two-pole high-pass row: the same double real pole as the Gaussian
    * and Butterworth rows, with a (1, -2, 1) numerator instead. That numerator is zero on a constant, so
    * the DC gain is 0 and the line is centred on zero; at Nyquist it weighs 4,
    * and c0 is exactly what divides that back to unity -- ((2-a2p)/2)^2 times
    * 4/(2-a2p)^2 is 1. Rolling off twice as steeply as the one-pole row is the
    * whole reason to pay for the second pole.
    */
   om = 1.0 - a2p;
   c0 = (1.0 - a2p / 2.0) * (1.0 - a2p / 2.0);
   a1 = 2.0 * om;
   a2 = -( om * om );

   today = startIdx - lookbackTotal;

   /* Start in the steady state of a constant input equal to the first bar: a
    * DC-gain-0 row answers 0 for a constant, so both output slots start there
    * while both input slots hold the bar. The input slots are carried in
    * locals and never re-read from inReal, because outReal may alias it.
    */
   x1 = inReal[today];
   x2 = x1;
   y1 = 0.0;
   y2 = 0.0;

   /* Skip the unstable period: run the recurrence but publish nothing. */
   i = lookbackTotal;
   while( i != 0 )
   {
      x0 = inReal[today];
      y  = a1 * y1 + (a2 * y2 + c0 * ((x0 - 2.0 * x1) + x2));
      x2 = x1;
      x1 = x0;
      y2 = y1;
      y1 = y;
      today++;
      i--;
   }

   outIdx = 0;
   while( today <= endIdx )
   {
      /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
       * rate is the latency of whatever y1 crosses to become y: outermost, that
       * is one fused step. Nested inside, it is three, and every backend's last
       * bit moves with it.
       */
      x0 = inReal[today];
      y  = a1 * y1 + (a2 * y2 + c0 * ((x0 - 2.0 * x1) + x2));
      x2 = x1;
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
