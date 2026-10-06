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
   double emaBeta = ((double)(optInTimePeriod - 1)) / ((double)(optInTimePeriod + 1));
   double optInK_1 = 1.0 - emaBeta;
   double tempReal, prevMA;
   int i, today, outIdx, lookbackTotal;

   /* emaBeta + optInK_1 must be exactly 1.0, or a flat input drifts off
    * its level. Each subtraction is exact only from an operand in
    * [0.5,1): at a period of 2 that is optInK_1, above it emaBeta.
    */
   emaBeta = 1.0 - optInK_1;

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
    * (same convention as TA_MA for every MAType). Explicit because the
    * recursion below, at a k of 1.0 and a beta of 0.0, does not keep the
    * sign of a -0.0 input. The unstable period still delays the first
    * output.
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

   today = startIdx-lookbackTotal;
   i = optInTimePeriod;
   tempReal = 0.0;
   while( i-- > 0 )
      tempReal += inReal[today++];

   prevMA = tempReal / optInTimePeriod;

   while( today <= startIdx )
      prevMA = optInK_1 * inReal[today++] + emaBeta * prevMA;

   outReal[0] = prevMA;
   outIdx = 1;

   while( today <= endIdx )
   {
      prevMA = optInK_1 * inReal[today++] + emaBeta * prevMA;
      outReal[outIdx++] = prevMA;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
