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
 *  090426 MF,CC  Initial version (#367).
 */

int adr_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1;
}

TA_RetCode adr(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
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

   /* This is ta_codegen/input/sma/sma.c with inReal[x] replaced by
    * (inHigh[x] - inLow[x]), and deliberately nothing else -- the same
    * running-sum order, the same read-before-write of the trailing term, and
    * the same divide by the period. Keeping the arithmetic identical is what
    * makes the composed reference in test_adr.c -- TA_SUB followed by TA_SMA
    * -- bit-exact rather than merely close.
    *
    * In particular the last statement divides; it does NOT multiply by a
    * precomputed 1/period, which would cost up to one ULP against TA_SMA and
    * silently downgrade that memcmp into a tolerance argument.
    *
    * Averaging the ranges is not the same computation as differencing two
    * averages: SMA(high) - SMA(low) is algebraically equal but subtracts two
    * price-magnitude means to reach a range-magnitude answer, so it inherits
    * the larger scale's rounding.
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
         periodTotal += (double)(inHigh[i] - inLow[i]);
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
      periodTotal += (double)(inHigh[i] - inLow[i]);
      i = i + 1;
      tempReal = periodTotal;
      periodTotal -= (double)(inHigh[trailingIdx] - inLow[trailingIdx]);
      trailingIdx = trailingIdx + 1;
      outReal[outIdx] = tempReal / (double)optInTimePeriod;
      outIdx = outIdx + 1;
   }

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
