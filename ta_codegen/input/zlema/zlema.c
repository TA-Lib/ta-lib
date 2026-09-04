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
 *  090426 MF,CC  Initial version (#347).
 *
 */

int zlema_lookback(int optInTimePeriod)
{
   /* ZLEMA owns no TA_FUNC_UNST_ id. It borrows EMA's through this call, which
    * is why zlema.yaml must not declare `unstable_period`.
    */
   return (optInTimePeriod - 1) / 2 + ema_lookback( optInTimePeriod );
}

TA_RetCode zlema(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double optInK_1 = 2.0 / ((double)(optInTimePeriod + 1));
   double tempReal, prevMA;
   int i, today, trailingIdx, outIdx, lag, lookbackTotal;

   /* KEEP THIS ARITHMETIC EXACTLY AS WRITTEN -- the de-lag in one rounding
    * (2.0*c - l, not c + (c - l)), the seed sum accumulating from 0.0, and
    * ((v - prevMA)*k) + prevMA. Together they make ZLEMA bit-for-bit equal to
    * an EMA over a materialised de-lagged series, which is the strongest gate
    * this function has. Reordering any one breaks that equality silently, and
    * the de-lag spelling is worth more than rounding noise: c + (c - l) rounds
    * twice, which is 5e-12 relative where 2c - l cancels.
    */

   lag = (optInTimePeriod - 1) / 2;
   lookbackTotal = zlema_lookback( optInTimePeriod );

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

   /* No smoothing at period of 1: the output is a copy of the input, the
    * convention TA_MA applies to every MAType. Explicit, because at period 1
    * lag is 0 and optInK_1 is exactly 1.0, so the recursion below reduces to
    * (x-prev)+prev -- which returns x only while consecutive values stay
    * within a factor of two of each other. The unstable period still delays
    * the first output.
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

   *outBegIdx = startIdx;

   /* In-place safe (outReal == inReal): both of a bar's reads precede its
    * write, and trailingIdx >= outIdx + optInTimePeriod - 1, so the trailing
    * read never reaches a slot already written.
    */
   trailingIdx = startIdx - lookbackTotal;
   today = trailingIdx + lag;

   i = optInTimePeriod;
   tempReal = 0.0;
   while( i-- > 0 )
   {
      tempReal += 2.0 * inReal[today] - inReal[trailingIdx];
      today++;
      trailingIdx++;
   }

   prevMA = tempReal / optInTimePeriod;

   while( today <= startIdx )
   {
      prevMA = (((2.0 * inReal[today] - inReal[trailingIdx]) - prevMA) * optInK_1) + prevMA;
      today++;
      trailingIdx++;
   }

   outReal[0] = prevMA;
   outIdx = 1;

   while( today <= endIdx )
   {
      prevMA = (((2.0 * inReal[today] - inReal[trailingIdx]) - prevMA) * optInK_1) + prevMA;
      today++;
      trailingIdx++;
      outReal[outIdx++] = prevMA;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
