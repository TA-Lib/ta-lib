/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  072026 MF,CC  First version.
 *
 */

int vwma_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1;
}

TA_RetCode vwma(int startIdx, int endIdx,
   const double inReal[],
   const double inVolume[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sumPV;
   double sumV;
   double tempPV;
   double tempV;
   double tempReal;
   size_t i;
   size_t outIdx;
   size_t trailingIdx;
   size_t lookbackTotal;

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

   /* Add-up the initial period, except for the last value.
    *
    * The price*volume product is kept in its own statement so no compiler may
    * contract it into an FMA: that would make this function disagree with the
    * Rust/Java backends under the cross-language bitwise gate, and with the
    * two-TA_SMA composite reference.
    */
   sumPV = 0.0;
   sumV = 0.0;
   trailingIdx = startIdx - lookbackTotal;

   i = trailingIdx;
   if( optInTimePeriod > 1 )
   {
      while( i < startIdx ) {
         tempReal = inReal[i] * inVolume[i];
         sumPV += tempReal;
         sumV += inVolume[i];
         i = i + 1;
      }
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the inReal and
    * outReal to be the same buffer.
    */
   outIdx = 0;
   while( i <= endIdx )
   {
      tempReal = inReal[i] * inVolume[i];
      sumPV += tempReal;
      sumV += inVolume[i];
      i = i + 1;

      /* Snapshot both sums before removing the trailing bar, mirroring the
       * add-new / snapshot / subtract-old order of TA_SMA. That order is what
       * makes this bit-identical to SMA(inReal*inVolume)/SMA(inVolume).
       */
      tempPV = sumPV;
      tempV = sumV;

      /* Read the trailing values before writing the output, since the caller
       * may pass the same buffer for an input and the output.
       */
      tempReal = inReal[trailingIdx] * inVolume[trailingIdx];
      sumPV -= tempReal;
      sumV -= inVolume[trailingIdx];

      outReal[outIdx] = (tempPV / (double)optInTimePeriod) / (tempV / (double)optInTimePeriod);

      trailingIdx = trailingIdx + 1;
      outIdx = outIdx + 1;
   }

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
