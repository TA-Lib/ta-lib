/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  RM       Robert Meier
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120307 RM     Initial Version
 *  120907 MF     Handling of a few limit cases
 *  071226 MF,CC  Fused single-loop rewrite: maintain the three band running
 *                sums (close for the middle band; the pointwise High/Low map for
 *                the upper/lower bands) over one shared trailing window, instead
 *                of two scratch buffers + three sma() calls. Enables streaming
 *                and is bit-identical to the prior three-SMA form (verified vs
 *                v0.6.4).
 */

int accbands_lookback(int optInTimePeriod)
{
   return sma_lookback( optInTimePeriod );
}

TA_RetCode accbands(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outRealUpperBand[],
   double outRealMiddleBand[],
   double outRealLowerBand[])
{
   double periodTotalUpper;
   double periodTotalMiddle;
   double periodTotalLower;
   double tempUpper;
   double tempMiddle;
   double tempLower;
   double tempReal;
   int i;
   int outIdx;
   int trailingIdx;
   int lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = sma_lookback( optInTimePeriod );

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

   /* Each band is a simple moving average maintained as a running sum over a
    * shared trailing window (all three share optInTimePeriod, so one trailing
    * index walks all three windows in lockstep):
    *    middle = SMA( close )
    *    upper  = SMA( high * (1 + 4*(high-low)/(high+low)) )
    *    lower  = SMA( low  * (1 - 4*(high-low)/(high+low)) )
    * When high+low is zero the upper/lower map degenerates to high/low.
    * Fusing the three moving averages into one loop is bit-identical to the
    * former "two scratch buffers + three sma() calls": each accumulator's
    * add/record/subtract order is unchanged, and the High/Low map is a pure
    * function recomputed from the raw trailing bar.
    */
   periodTotalUpper = 0.0;
   periodTotalMiddle = 0.0;
   periodTotalLower = 0.0;
   trailingIdx = startIdx - lookbackTotal;

   /* Warm up the running sums with the initial period,
    * except for the last value.
    */
   i = trailingIdx;
   while( i < startIdx )
   {
      tempReal = inHigh[i] + inLow[i];
      if( !TA_IS_ZERO(tempReal) )
      {
         tempReal = 4*(inHigh[i]-inLow[i])/tempReal;
         periodTotalUpper += inHigh[i]*(1+tempReal);
         periodTotalLower += inLow[i]*(1-tempReal);
      }
      else
      {
         periodTotalUpper += inHigh[i];
         periodTotalLower += inLow[i];
      }
      periodTotalMiddle += inClose[i];
      i = i + 1;
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and output to be the
    * same buffer: every trailing bar is read before any output is written.
    */
   outIdx = 0;
   while( i <= endIdx )
   {
      /* Add the incoming bar to each running sum. */
      tempReal = inHigh[i] + inLow[i];
      if( !TA_IS_ZERO(tempReal) )
      {
         tempReal = 4*(inHigh[i]-inLow[i])/tempReal;
         periodTotalUpper += inHigh[i]*(1+tempReal);
         periodTotalLower += inLow[i]*(1-tempReal);
      }
      else
      {
         periodTotalUpper += inHigh[i];
         periodTotalLower += inLow[i];
      }
      periodTotalMiddle += inClose[i];
      i = i + 1;

      /* Record the current window sums. */
      tempUpper = periodTotalUpper;
      tempMiddle = periodTotalMiddle;
      tempLower = periodTotalLower;

      /* Remove the trailing bar from each running sum. */
      tempReal = inHigh[trailingIdx] + inLow[trailingIdx];
      if( !TA_IS_ZERO(tempReal) )
      {
         tempReal = 4*(inHigh[trailingIdx]-inLow[trailingIdx])/tempReal;
         periodTotalUpper -= inHigh[trailingIdx]*(1+tempReal);
         periodTotalLower -= inLow[trailingIdx]*(1-tempReal);
      }
      else
      {
         periodTotalUpper -= inHigh[trailingIdx];
         periodTotalLower -= inLow[trailingIdx];
      }
      periodTotalMiddle -= inClose[trailingIdx];
      trailingIdx = trailingIdx + 1;

      /* Write the three bands. */
      outRealUpperBand[outIdx] = tempUpper / (double)optInTimePeriod;
      outRealMiddleBand[outIdx] = tempMiddle / (double)optInTimePeriod;
      outRealLowerBand[outIdx] = tempLower / (double)optInTimePeriod;
      outIdx = outIdx + 1;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
