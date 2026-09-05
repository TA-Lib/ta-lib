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
 *  090626 KL     First version (issue #350).
 */

int er_lookback(int optInTimePeriod)
{
   /* P one-bar changes need P+1 prices: first output at index P. */
   return optInTimePeriod;
}

TA_RetCode er(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx,
   int *outNBElement,
   double outReal[])
{
   int outIdx, today, trailingIdx, lookbackTotal, i;
   int nullRun;
   double sumROC1, periodROC, tempReal, tempReal2, trailingValue;

   /* Kaufman Efficiency Ratio (Perry J. Kaufman, Smarter Trading, 1995):
    * net directional movement over the period divided by the total path
    * travelled,
    *
    *   ER[t] = |c[t] - c[t-P]| / SUM(k = t-P+1 .. t) |c[k] - c[k-1]|
    *
    * This is a verbatim lift of TA_KAMA's inner efficiency ratio
    * (kama.c) so the two stay bit-identical -- the KAMA-reconstruction
    * differential in test_composite2.c exists to keep it that way. Both
    * guards are load-bearing and shared with kama.c:
    *
    *   - `sumROC1 <= periodROC` pins the ratio to exactly 1.0 where FP
    *     would give 1.0000000000000002. The comparison is against the
    *     SIGNED numerator, so it only fires on up-moves; on sustained
    *     declines the raw fabs ratio can exceed 1.0 by a few ULP. Do NOT
    *     "fix" this with fabs -- it changes TA_KAMA's output.
    *   - a genuinely flat window is recognized by COUNTING exactly-zero
    *     one-bar changes (nullRun >= P forces sumROC1 to 0.0, purging the
    *     running sum's rounding residue), after which `0 <= 0` pins the
    *     0/0 to 1.0 -- never NaN (#112). This is kama.c's #253 form; the
    *     absolute TA_IS_ZERO band it replaced fails the QUOTE-UNIT/SCALE
    *     gate (ER is homogeneous of degree 0, and a fixed 1e-14 met a
    *     price-carrying sum).
    *
    * The subtract-then-add update order matches TA_SUM's recurrence,
    * which is what makes the composite differential bit-exact. The
    * trailing value is cached one iteration ahead, which is what keeps
    * outReal == inReal aliasing safe.
    */

   lookbackTotal = er_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx    = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Prime the path sum over the optInTimePeriod one-bar changes ending
    * at the first output bar's predecessor.
    */
   sumROC1 = 0.0;
   nullRun = 0;
   today = startIdx - lookbackTotal;
   trailingIdx = today;
   i = optInTimePeriod;
   while( i-- > 0 )
   {
      tempReal = inReal[today++];
      tempReal -= inReal[today];
      sumROC1 += fabs( tempReal );
      if( tempReal == 0.0 )
         nullRun++;
      else
         nullRun = 0;
   }

   /* First output: today == startIdx. */
   tempReal  = inReal[today];
   tempReal2 = inReal[trailingIdx++];
   periodROC = tempReal - tempReal2;
   trailingValue = tempReal2;
   /* A fully flat priming window sums to an exact 0.0 (no residue yet), so
    * `0 <= 0` already answers 1.0 here without the nullRun purge. */
   if( sumROC1 <= periodROC )
      outReal[0] = 1.0;
   else
      outReal[0] = fabs( periodROC / sumROC1 );
   outIdx = 1;
   today++;

   while( today <= endIdx )
   {
      tempReal  = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal - tempReal2;

      /* Subtract-then-add, TA_SUM's own order. */
      sumROC1 -= fabs( trailingValue - tempReal2 );
      sumROC1 += fabs( tempReal - inReal[today-1] );

      /* Once a whole window of one-bar changes is exactly zero, the sum's
       * only content is rounding residue from the subtract/add carry --
       * purge it so the flat window is recognized exactly (kama.c #253). */
      if( tempReal - inReal[today-1] == 0.0 )
         nullRun++;
      else
         nullRun = 0;
      if( nullRun >= optInTimePeriod )
      {
         nullRun = optInTimePeriod;
         sumROC1 = 0.0;
      }

      /* Save the trailing value: outReal may alias inReal, and the next
       * iteration's subtraction needs the ORIGINAL bar, not the slot the
       * write below may have clobbered.
       */
      trailingValue = tempReal2;

      if( sumROC1 <= periodROC )
         outReal[outIdx++] = 1.0;
      else
         outReal[outIdx++] = fabs( periodROC / sumROC1 );
      today++;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
