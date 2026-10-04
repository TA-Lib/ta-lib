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

int swak_hp_lookback(int optInTimePeriod)
{
   (void)optInTimePeriod;

   /* No structural lookback: the one input slot and the one output slot are
    * seeded from the first bar rather than read from before it, and there is
    * no callee whose lookback could be inherited.
    */
   return TA_GetUnstablePeriod(TA_FUNC_UNST_SWAK_HP);
}

TA_RetCode swak_hp(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx, today, lookbackTotal;
   double w, cw, a1p, c0, a1;
   double x0, x1, y, y1;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = swak_hp_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* The one-pole alpha (Ehlers, "Swiss Army Knife Indicator", Figure 5).
    * The paper says this one "is computed exactly the same as it is for the
    * EMA" -- meaning the cutoff-period formula below, not TA_EMA's
    * 2/(n+1). The paper's 360/P is a full turn, so 2*pi/P.
    *
    * The period range starts at 5 because of what this expression does below
    * it, not for taste: at P = 4, cos(w) is 6.1e-17 and `cos w + sin w - 1`
    * rounds to exactly 0.0, so a1p is 0, c0 and a1 are both 1, and the filter
    * degenerates into the integrator x - x[s]. At P = 2, cos(w) is -1 and c0
    * is 0, a dead filter. No contiguous range below 5 avoids both.
    */
   w   = (2.0 * 3.14159265358979323846) / (double)optInTimePeriod;
   cw  = cos(w);
   a1p = (cw + sin(w) - 1.0) / cw;

   /* The high-pass row: a (1, -1) numerator, so its DC gain is 0 and the line
    * is centred on zero rather than on price.
    */
   c0 = 1.0 - a1p / 2.0;
   a1 = 1.0 - a1p;

   today = startIdx - lookbackTotal;

   /* Start in the steady state of a constant input equal to the first bar: a
    * DC-gain-0 row answers 0 for a constant, so the output slot starts there
    * while the input slot holds the bar. The input slot is carried in a local
    * and never re-read from inReal, because outReal may alias it.
    */
   x1 = inReal[today];
   y1 = 0.0;

   /* Skip the unstable period: run the recurrence but publish nothing. */
   i = lookbackTotal;
   while( i != 0 )
   {
      x0 = inReal[today];
      y  = a1 * y1 + c0 * (x0 - x1);
      x1 = x0;
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
      y  = a1 * y1 + c0 * (x0 - x1);
      x1 = x0;
      y1 = y;

      outReal[outIdx] = y;
      outIdx = outIdx + 1;
      today++;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
