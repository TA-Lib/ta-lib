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
 */

int swak_bp_lookback(int optInTimePeriod, double optInDelta)
{
   (void)optInTimePeriod;
   (void)optInDelta;

   /* No structural lookback: the two input slots and the two output slots are
    * seeded from the first bar rather than read from before it, and there is
    * no callee whose lookback could be inherited (ha.c:16-19).
    */
   return TA_GetUnstablePeriod(TA_FUNC_UNST_SWAK_BP);
}

TA_RetCode swak_bp(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInDelta,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx, today, lookbackTotal;
   double w, beta, t, abp, c0, a1, a2;
   double x0, x1, x2, y, y1, y2;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = swak_bp_lookback( optInTimePeriod, optInDelta );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* The band-pass row (Ehlers, Stocks & Commodities January 2006, Figure 5).
    * The paper's 360/P is a full turn, so 2*pi/P; beta is the cosine of the
    * centre frequency and t is the half-bandwidth angle.
    */
   w    = (2.0 * 3.14159265358979323846) / (double)optInTimePeriod;
   beta = cos(w);
   t    = (4.0 * 3.14159265358979323846 * optInDelta) / (double)optInTimePeriod;

   /* abp is written as (1 - sin t)/cos t rather than the paper's
    * gamma - sqrt(gamma^2 - 1) with gamma = 1/cos t. The two are equal for
    * 0 < t < pi/2, which this function's ranges guarantee (t <= 0.4*pi), but
    * the published form cancels as t -> 0 because gamma^2 - 1 goes as t^2.
    * Against a 60-digit reference the published form is already 4.5e-13 off
    * at the period cap; this one is not, and that is what lets the cap be
    * 2000 rather than 1000.
    */
   abp = (1.0 - sin(t)) / cos(t);

   /* The numerator (1, 0, -1) is zero both on a constant and on a Nyquist
    * alternation, so this row answers 0 at DC and at Nyquist, and exactly 1
    * with zero phase at the centre period itself.
    */
   c0 = (1.0 - abp) / 2.0;
   a1 = beta * (1.0 + abp);
   a2 = -abp;

   today = startIdx - lookbackTotal;

   /* Start in the steady state of a constant input equal to the first bar: a
    * band-pass of a constant is 0, so both output slots start there while both
    * input slots hold the bar. The input slots are carried in locals and never
    * re-read from inReal, because outReal may alias it.
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
      y  = a1 * y1 + (a2 * y2 + c0 * (x0 - x2));
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
      /* The evaluation order is the bit-exactness contract across backends:
       * the numerator as c0*(x0 - x2), then the a1 feedback, then a2. The
       * middle tap is zero and dropped rather than added, which costs an
       * operation per bar and nothing else: adding 0.0*x1 would land on the
       * same bits, +0.0 for a cancelled numerator included.
       */
      x0 = inReal[today];
      y  = a1 * y1 + (a2 * y2 + c0 * (x0 - x2));
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
