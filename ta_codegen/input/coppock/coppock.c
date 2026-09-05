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
 *  090526 KL     First version (issue #362).
 */

int coppock_lookback(int optInWMAPeriod, int optInROC1Period, int optInROC2Period)
{
   /* The ROC sum needs max(p1,p2) prior bars before it exists at all, and the
    * WMA needs optInWMAPeriod values of that sum -> the first output sits at
    * max(p1,p2) + optInWMAPeriod - 1. The lookback keys off the MAX, which is
    * why optInROC1Period > optInROC2Period is accepted rather than rejected:
    * the formula is symmetric in the two ROCs (issue #362, decision 2).
    */
   if( optInROC1Period > optInROC2Period )
      return optInROC1Period + optInWMAPeriod - 1;
   return optInROC2Period + optInWMAPeriod - 1;
}

TA_RetCode coppock(int startIdx, int endIdx,
   const double inReal[],
   int optInWMAPeriod,
   int optInROC1Period,
   int optInROC2Period,
   int *outBegIdx,
   int *outNBElement,
   double outReal[])
{
   int outIdx, inIdx, lookbackTotal, i, q, rw, ringWalk, ringSize;
   int barsSinceReseed;
   int roc1Idx, roc2Idx;
   double periodSum, periodSub, tempReal, tempReal2, trailingValue, divider;
   double base1, base2, roc1, roc2;
   CIRCBUF_PROLOG(sRing,double,50);

   /* Coppock Curve: a WMA(optInWMAPeriod) of the SUM of two rates of change,
    * ROC(optInROC1Period) + ROC(optInROC2Period). The sum, not the mean:
    * every published definition sums them; Tulip's beta/copp.c averages and
    * therefore reads at exactly half this amplitude. A clean 2.000000x ratio
    * against Tulip is Tulip's variant, not a defect here (issue #362).
    *
    * The smoothed series is the inline expression
    *    S(j) = R(j,p1) + R(j,p2)
    *    R(j,p) = (inReal[j-p] != 0.0) ? ((inReal[j]/inReal[j-p])-1.0)*100.0
    *                                  : 0.0
    * -- TA_ROC's own zero guard included, so a zero price yields 0.0 exactly
    * where TA_ROC yields 0.0, never an inf that pollutes the window. Each
    * lagged denominator goes through its own trailing cursor advanced in
    * lock-step (TA_ROC's own shape) -- a parameter-sized lag subscript is
    * outside the stream classifier's index grammar.
    *
    * The WMA stage reproduces TA_WMA's recurrence verbatim -- the triangle
    * divider computed in double (#142), the periodSum/periodSub carry and the
    * 8*w re-anchor (#254) -- because anything short of verbatim breaks the
    * bit-exact composite differential against TA_ROC + TA_ROC + TA_WMA. S is
    * a DERIVED series that is never materialised, so, exactly as in TA_HMA's
    * outer stage, each S value is computed once and carried in a
    * (optInWMAPeriod-1)-slot ring: the trailing subtraction reads the
    * expiring slot and the re-anchor walks the ring, oldest first, weight
    * counting up from 1, then adds the current bar at weight w.
    */

   lookbackTotal = coppock_lookback( optInWMAPeriod, optInROC1Period, optInROC2Period );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx    = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Triangle divider in double: the int product w*(w+1) overflows int32 at
    * w >= 46341 (#142), exactly as in TA_WMA.
    */
   divider = (double)optInWMAPeriod*(optInWMAPeriod+1)/2.0;

   outIdx = 0;

   /* The S value computed at bar t expires optInWMAPeriod-1 bars later, so a
    * single-cursor ring of optInWMAPeriod-1 slots is enough: read the
    * expiring value, overwrite the slot, advance. At w == 1 the recurrence's
    * state is never consumed (see the identity output below), but the ring
    * still needs one slot for the reads to stay in bounds.
    */
   ringSize = optInWMAPeriod - 1;
   if( ringSize < 1 )
      ringSize = 1;
   CIRCBUF_INIT(sRing,double,ringSize);
   /* At w == 1 the priming loop below never runs, so the first trailing read
    * would see an undefined slot; at w > 1 priming overwrites every slot.
    */
   sRing[0] = 0.0;

   /* One trailing cursor per ROC denominator, advanced in lock-step from the
    * priming scan onward.
    */
   inIdx   = startIdx - (optInWMAPeriod-1);
   roc1Idx = inIdx - optInROC1Period;
   roc2Idx = inIdx - optInROC2Period;

   /* Priming: the w-1 S values before the first output, oldest first with
    * the weight counting up from 1 -- TA_WMA's own priming order, which the
    * re-anchor below must reproduce. They also fill the ring.
    */
   periodSum = periodSub = (double)0.0;
   i = 1;
   while( inIdx < startIdx )
   {
      base1 = inReal[roc1Idx];
      roc1Idx++;
      base2 = inReal[roc2Idx];
      roc2Idx++;
      roc1 = (base1 != 0.0) ? ((inReal[inIdx]/base1)-1.0)*100.0 : 0.0;
      roc2 = (base2 != 0.0) ? ((inReal[inIdx]/base2)-1.0)*100.0 : 0.0;
      tempReal = roc1 + roc2;
      periodSub += tempReal;
      periodSum += tempReal*i;
      i++;
      sRing[sRing_Idx] = tempReal;
      CIRCBUF_NEXT(sRing);
      inIdx++;
   }
   barsSinceReseed = 8 * optInWMAPeriod;
   trailingValue = 0.0;

   /* Tight loop for the requested range. */
   while( inIdx <= endIdx )
   {
      base1 = inReal[roc1Idx];
      roc1Idx++;
      base2 = inReal[roc2Idx];
      roc2Idx++;
      roc1 = (base1 != 0.0) ? ((inReal[inIdx]/base1)-1.0)*100.0 : 0.0;
      roc2 = (base2 != 0.0) ? ((inReal[inIdx]/base2)-1.0)*100.0 : 0.0;
      tempReal = roc1 + roc2;
      periodSub += tempReal;
      periodSub -= trailingValue;
      periodSum += tempReal*optInWMAPeriod;

      /* Re-anchor every 8*w bars: rebuild both totals from the window
       * itself -- TA_WMA's #254 fix, same single trigger, same interval.
       * The window lives in the ring (w-1 prior S values, sRing_Idx the
       * oldest) plus the current bar's tempReal at weight w.
       */
      barsSinceReseed--;
      if( barsSinceReseed <= 0 )
      {
         barsSinceReseed = 8 * optInWMAPeriod;
         periodSub = (double)0.0;
         periodSum = (double)0.0;
         rw = 1;
         ringWalk = sRing_Idx;
         for( q = 0; q < ringSize; q++ )
         {
            tempReal2 = sRing[ringWalk];
            periodSub += tempReal2;
            periodSum += tempReal2*rw;
            rw++;
            ringWalk++;
            if( ringWalk >= ringSize ) ringWalk = 0;
         }
         periodSub += tempReal;
         periodSum += tempReal*optInWMAPeriod;
      }

      /* Read the expiring S value BEFORE overwriting its slot with the
       * current one -- the ring is the aliasing-safe stand-in for TA_WMA's
       * "save the trailing value before the store" rule.
       */
      trailingValue = sRing[sRing_Idx];
      sRing[sRing_Idx] = tempReal;
      CIRCBUF_NEXT(sRing);

      /* Load-bearing, not a rounding nicety: keep it. WMA(1) is the identity
       * and TA_WMA ships an exact copy fast path, but the recurrence here is
       * off by a whole term at w == 1 -- ringSize clamps to 1, so the
       * read-before-write ring hands back the wrong trailing value. Deleting
       * this arm moves TA_SREF bar 16 at (1,11,14) from -11.311839169954585
       * to -6.4591709868291103.
       */
      if( optInWMAPeriod == 1 )
         outReal[outIdx] = tempReal;
      else
         outReal[outIdx] = periodSum / divider;
      outIdx++;
      periodSum -= periodSub;
      inIdx++;
   }

   CIRCBUF_DESTROY(sRing);

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
