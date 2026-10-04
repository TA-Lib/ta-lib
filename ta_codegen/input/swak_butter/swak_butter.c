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

int swak_butter_lookback(int optInTimePeriod)
{
   (void)optInTimePeriod;

   /* No structural lookback: the two input slots and the two output slots are
    * seeded from the first bar rather than read from before it, and there is
    * no callee whose lookback could be inherited, so the function's own
    * unstable period is the whole of it (ha.c:16-19).
    */
   return TA_GetUnstablePeriod(TA_FUNC_UNST_SWAK_BUTTER);
}

TA_RetCode swak_butter(int startIdx, int endIdx,
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

   lookbackTotal = swak_butter_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* The same two-pole alpha as the Gaussian row (Ehlers, Stocks & Commodities
    * January 2006, Figure 5); the paper's 360/P is a full turn, so 2*pi/P.
    * a2p keeps the sqrt form rather than the algebraically equal subtraction,
    * which cancels for large P.
    */
   w   = (2.0 * 3.14159265358979323846) / (double)optInTimePeriod;
   b2p = 2.415 * (1.0 - cos(w));
   a2p = -b2p + sqrt( b2p*b2p + 2.0*b2p );

   /* The Butterworth row: the Gaussian's double real pole with two zeros added
    * at Nyquist, which is what the (1, 2, 1) numerator is. The quarter in c0
    * keeps the DC gain at 1 against that numerator's weight of 4.
    */
   om = 1.0 - a2p;
   c0 = (a2p * a2p) / 4.0;
   a1 = 2.0 * om;
   a2 = -( om * om );

   today = startIdx - lookbackTotal;

   /* Start in the steady state of a constant input equal to the first bar.
    * Unlike the Gaussian row this one has non-zero b1 and b2, so the two input
    * slots are carried in locals and never re-read from inReal: outReal may
    * alias inReal, and an aliased write would already have overwritten the
    * earlier bars this recurrence needs.
    */
   x0 = inReal[today];
   x1 = x0;
   x2 = x0;
   y1 = x0;
   y2 = x0;

   /* Skip the unstable period: run the recurrence but publish nothing. */
   i = lookbackTotal;
   while( i != 0 )
   {
      x0 = inReal[today];
      y  = a1 * y1 + (a2 * y2 + c0 * ((x0 + 2.0 * x1) + x2));
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
       * the numerator as (x0 + 2*x1) + x2, then the a1 feedback, then a2.
       */
      x0 = inReal[today];
      y  = a1 * y1 + (a2 * y2 + c0 * ((x0 + 2.0 * x1) + x2));
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
