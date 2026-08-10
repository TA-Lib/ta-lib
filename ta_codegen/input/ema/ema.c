/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  080926 MF,CC Explicit no-smoothing copy at a period of 1.
 *  081026 MF,CC Fold the internal variant into EMA (issue #183).
 *
 */

int ema_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1 + TA_GetUnstablePeriod(TA_FUNC_UNST_EMA);
}

TA_RetCode ema(int startIdx, int endIdx,
   const double *inReal,
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double *outReal)
{
   double optInK_1 = 2.0 / ((double)(optInTimePeriod + 1));
   double tempReal, prevMA;
   int i, today, outIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = ema_lookback( optInTimePeriod );

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

   /* No smoothing at period of 1: the output is a copy of the input
    * (same convention as TA_MA for every MAType). Explicit because at
    * period 1 optInK_1 is exactly 1.0, so the recursion below reduces to
    * (x-prev)+prev -- which returns x only while consecutive values stay
    * within a factor of two of each other. Two-decimal prices already
    * spend a full mantissa, so a single 3x move breaks it. The unstable
    * period still delays the first output.
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

   /* Do the EMA calculation using tight loops. */

   /* The first EMA is calculated differently. It
    * then become the seed for subsequent EMA.
    *
    * The algorithm for this seed vary widely.
    * Only 3 are implemented here:
    *
    * TA_MA_CLASSIC:
    *    Use a simple MA of the first 'period'.
    *    This is the approach most widely documented.
    *
    * TA_MA_METASTOCK:
    *    Use first price bar value as a seed
    *    from the begining of all the available
    *    data.
    *
    * TA_MA_TRADESTATION:
    *    Use 4th price bar as a seed, except when
    *    period is 1 who use 2th price bar or something
    *    like that... (not an obvious one...).
    */
   if( TA_GetCompatibility() == TA_COMPATIBILITY_DEFAULT )
   {
      today = startIdx-lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 )
         tempReal += inReal[today++];

      prevMA = tempReal / optInTimePeriod;
   }
   else
   {
      prevMA = inReal[0];
      today = 1;
   }

   while( today <= startIdx )
      prevMA = ((inReal[today++]-prevMA)*optInK_1) + prevMA;

   outReal[0] = prevMA;
   outIdx = 1;

   while( today <= endIdx )
   {
      prevMA = ((inReal[today++]-prevMA)*optInK_1) + prevMA;
      outReal[outIdx++] = prevMA;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
