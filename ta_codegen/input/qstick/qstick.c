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
 *  081226 KL   Initial version.
 *
 */

int qstick_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1;
}

TA_RetCode qstick(int startIdx, int endIdx,
   const double inOpen[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double periodTotal;
   double tempReal;
   size_t i;
   size_t outIdx;
   size_t trailingIdx;
   size_t lookbackTotal;

   /* Qstick (Chande & Kroll, The New Technical Trader, 1994): a simple moving
    * average of the candle body, close minus open. Above zero means bodies
    * were predominantly bullish over the window; the zero crossings are the
    * signal.
    *
    * This is ta_codegen/input/sma/sma.c with inReal[x] replaced by
    * (inClose[x] - inOpen[x]), and deliberately nothing else: the same
    * running-sum order, the same read-before-write of the trailing term, and
    * the same divide by the period. Keeping the arithmetic identical is what
    * makes the composed reference in test_composite.c -- TA_SUB followed by
    * TA_SMA -- bit-exact rather than merely close, so any future drift in
    * either path is a hard failure instead of a tolerance argument.
    *
    * In particular the last statement divides; it does NOT multiply by a
    * precomputed 1/period. Tulip's qstick.c multiplies, which costs it up to
    * one ULP against TA_SMA. Dividing buys the memcmp differential, which is
    * the stronger of the two gates.
    */

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = (size_t)(optInTimePeriod-1);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal ) {
      startIdx = lookbackTotal;
   }

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Do the MA calculation using tight loops. */
   /* Add-up the initial period, except for the last value. */
   periodTotal = 0.0;
   trailingIdx = startIdx - lookbackTotal;

   i = trailingIdx;
   if( optInTimePeriod > 1 )
   {
      while( i < startIdx ) {
         periodTotal += (double)(inClose[i] - inOpen[i]);
         i = i + 1;
      }
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows outReal to be the same
    * buffer as either input.
    */
   outIdx = 0;
   while( i <= endIdx )
   {
      periodTotal += (double)(inClose[i] - inOpen[i]);
      i = i + 1;
      tempReal = periodTotal;
      periodTotal -= (double)(inClose[trailingIdx] - inOpen[trailingIdx]);
      trailingIdx = trailingIdx + 1;
      outReal[outIdx] = tempReal / (double)optInTimePeriod;
      outIdx = outIdx + 1;
   }

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
