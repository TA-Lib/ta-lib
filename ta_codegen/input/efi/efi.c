/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   Initial version (#206).
 *
 */

int efi_lookback(int optInTimePeriod)
{
   /* One bar is consumed forming the first close-to-close change, then the
    * EMA's own warm-up on top:
    *    1 + ema_lookback(optInTimePeriod)
    *  = 1 + (optInTimePeriod - 1) + TA_GetUnstablePeriod(TA_FUNC_UNST_EMA)
    */
   return optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_EMA);
}

TA_RetCode efi(int startIdx, int endIdx,
   const double inClose[],
   const double inVolume[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double optInK_1 = 2.0 / ((double)(optInTimePeriod + 1));
   double tempReal, prevMA, prevClose, force;
   int i, today, outIdx, lookbackTotal;

   /* Alexander Elder's Force Index (Trading for a Living, 1993): the one-bar
    * close-to-close move weighted by that bar's volume, then smoothed with an
    * EMA. Elder's 2-period reading is the short-term form and 13 the
    * intermediate-term one -- that is the parameter, not a second formula.
    *
    *    force[t] = ( close[t] - close[t-1] ) * volume[t]
    *    EFI      = EMA( force, optInTimePeriod )
    *
    * The arithmetic below is ema.c's with inReal[t] replaced by force[t], kept
    * in exactly that shape on purpose: the seed accumulates from 0.0 in the
    * same order, and the recurrence is (x - prevMA)*k + prevMA rather than the
    * algebraically equal k*x + (1-k)*prevMA. That order IS the bit-exactness
    * contract against the composed reference in test_composite.c -- MOM, then
    * MULT, then EMA -- so do not tidy it. TRIX carries the same warning.
    *
    * Nothing on the data path divides by an input, so issue #112 is satisfied
    * structurally: a flat close gives force exactly 0.0 and output exactly
    * 0.0, and zero volume likewise. The only division is by the period, a
    * positive integer parameter.
    *
    * prevClose is carried in a scalar rather than re-read from inClose[t-1]
    * because the C API allows outReal to alias an input: at bar t the slot
    * holding close[t-1] may already have been overwritten by the output
    * written a bar earlier. cmou.c carries its trailing value for the same
    * reason.
    */

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = efi_lookback( optInTimePeriod );

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* No smoothing at a period of 1: the output is the raw Force Index.
    * Explicit for the reason spelled out in ema.c -- at period 1 optInK_1 is
    * exactly 1.0, so the recursion reduces to (x-prev)+prev, which returns x
    * only while consecutive values stay within a factor of two of each other.
    * Force values swing by orders of magnitude, far more than the prices EMA
    * warns about.
    */
   if( optInTimePeriod == 1 )
   {
      *outBegIdx = startIdx;
      outIdx = 0;
      today = startIdx;
      prevClose = inClose[today-1];
      while( today <= endIdx )
      {
         force = (inClose[today] - prevClose) * inVolume[today];
         prevClose = inClose[today];
         outReal[outIdx] = force;
         outIdx = outIdx + 1;
         today = today + 1;
      }
      *outNBElement = outIdx;
      return TA_SUCCESS;
   }

   *outBegIdx = startIdx;

   /* The first EMA value is a simple average of the first 'period' force
    * values; it then seeds the recursion. This is ema.c's CLASSIC seeding
    * applied to the force series rather than to the input array.
    *
    * TA_GetCompatibility() is deliberately NOT consulted. ema.c still carries
    * a TA_COMPATIBILITY_METASTOCK seeding arm, but that capability is being
    * deprecated: it is preserved for the functions that already shipped with
    * it and dropped from new ones, and it is not reachable at all from the
    * Rust, Java and C# APIs, which expose no TA_SetCompatibility. Honouring it
    * here would make EFI's C output diverge from the other three backends for
    * a setting they cannot even read.
    */
   today = startIdx - lookbackTotal + 1;
   prevClose = inClose[today-1];
   i = optInTimePeriod;
   tempReal = 0.0;
   while( i-- > 0 )
   {
      force = (inClose[today] - prevClose) * inVolume[today];
      prevClose = inClose[today];
      tempReal += force;
      today = today + 1;
   }

   prevMA = tempReal / optInTimePeriod;

   while( today <= startIdx )
   {
      force = (inClose[today] - prevClose) * inVolume[today];
      prevClose = inClose[today];
      prevMA = ((force - prevMA) * optInK_1) + prevMA;
      today = today + 1;
   }

   outReal[0] = prevMA;
   outIdx = 1;

   while( today <= endIdx )
   {
      force = (inClose[today] - prevClose) * inVolume[today];
      prevClose = inClose[today];
      prevMA = ((force - prevMA) * optInK_1) + prevMA;
      outReal[outIdx] = prevMA;
      outIdx = outIdx + 1;
      today = today + 1;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
