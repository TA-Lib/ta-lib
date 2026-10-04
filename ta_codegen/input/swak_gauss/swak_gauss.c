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

int swak_gauss_lookback(int optInTimePeriod)
{
   (void)optInTimePeriod;

   /* No structural lookback. Every term of the recurrence exists at the first
    * bar -- the two history slots are seeded from that bar rather than read
    * from before it -- and there is no callee whose lookback could be
    * inherited, so the function's own unstable period is the whole of it
    * (ha.c:16-19 takes the same shape for the same reason).
    */
   return TA_GetUnstablePeriod(TA_FUNC_UNST_SWAK_GAUSS);
}

TA_RetCode swak_gauss(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx, today, lookbackTotal;
   double w, b2p, a2p, om, c0, a1, a2;
   double y, y1, y2;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = swak_gauss_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* The Swiss Army Knife two-pole alpha (Ehlers, Stocks & Commodities January
    * 2006, Figure 5). The paper writes the argument as 360/P degrees and says
    * on p.3 to read it as a full turn, so it is 2*pi/P here.
    *
    * a2p is the root of the quadratic that places the double real pole; it is
    * written as -b2p + sqrt(b2p^2 + 2*b2p) rather than the algebraically equal
    * subtraction, which cancels for large P.
    */
   w   = (2.0 * 3.14159265358979323846) / (double)optInTimePeriod;
   b2p = 2.415 * (1.0 - cos(w));
   a2p = -b2p + sqrt( b2p*b2p + 2.0*b2p );

   /* The Gaussian row of Figure 5: numerator a2p^2 on the bar alone, no
    * x[i-1] or x[i-2] term. Its DC gain is 1, so the line sits on price.
    */
   om = 1.0 - a2p;
   c0 = a2p * a2p;
   a1 = 2.0 * om;
   a2 = -( om * om );

   today = startIdx - lookbackTotal;

   /* Start in the steady state of a constant input equal to the first bar: a
    * DC-gain-1 row answers that constant, so both output slots hold it. The
    * input slots would hold it too, but this row's b1 and b2 are zero and
    * never reads them -- which is also why outReal may alias inReal here.
    */
   y1 = inReal[today];
   y2 = y1;

   /* Skip the unstable period: run the recurrence but publish nothing. */
   i = lookbackTotal;
   while( i != 0 )
   {
      y  = a1 * y1 + (a2 * y2 + c0 * inReal[today]);
      y2 = y1;
      y1 = y;
      today++;
      i--;
   }

   outIdx = 0;
   while( today <= endIdx )
   {
      /* The evaluation order is the bit-exactness contract across backends:
       * the numerator first, then the a1 feedback, then the a2 feedback.
       */
      y  = a1 * y1 + (a2 * y2 + c0 * inReal[today]);
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
