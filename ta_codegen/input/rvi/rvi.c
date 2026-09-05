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
 *  090526 MF,CC  First version (issue #366).
 */

int rvi_lookback(int optInTimePeriod, int optInStdDevPeriod)
{
   return (optInStdDevPeriod-1) + (optInTimePeriod-1)
   + TA_GetUnstablePeriod(TA_FUNC_UNST_RVI);
}

TA_RetCode rvi(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int optInStdDevPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double tempReal, shift, periodTotal1, periodTotal2, meanValue1, variance;
   double invPeriod, sigma, delta, upValue, dnValue, upTotal, dnTotal;
   double prevUp, prevDn, wAlpha, wBeta, total;
   int i, j, outIdx, today, anchorIdx, trailingIdx, windowStart, barsSinceReseed;
   int nbInitialElementNeeded, lookbackTotal;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = rvi_lookback( optInTimePeriod, optInStdDevPeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* wAlpha is derived FROM wBeta, never the reverse (rma.c): only that order
    * makes the pair sum to exactly 1, and TA_RMA over this function's two legs
    * has to be bit for bit what the fused step below computes.
    */
   wBeta  = (double)(optInTimePeriod-1) / (double)optInTimePeriod;
   wAlpha = 1.0 - wBeta;

   /* The per-bar sigma is var.c's step transcribed unchanged -- shifted running
    * sums against a near-window anchor, the reseed trigger, its floor, and the
    * re-remove under the new shift. Any algebraically equal spelling is a
    * different double, and the reference this is differenced against is
    * TA_STDDEV anchored at this call's own start.
    */
   nbInitialElementNeeded = optInStdDevPeriod - 1;
   invPeriod = 1.0 / (double)optInStdDevPeriod;

   anchorIdx = startIdx - lookbackTotal;
   trailingIdx = anchorIdx;
   shift = inReal[anchorIdx];
   periodTotal1 = 0.0;
   periodTotal2 = 0.0;
   today = anchorIdx + nbInitialElementNeeded;
   for( j=anchorIdx; j < today; j++ )
   {
      tempReal = inReal[j] - shift;
      periodTotal1 += tempReal;
      tempReal *= tempReal;
      periodTotal2 += tempReal;
   }

   barsSinceReseed = 32 * optInStdDevPeriod;

   /* Seed both legs with the simple average of the first 'optInTimePeriod'
    * volatilities, as rma.c seeds. optInStdDevPeriod >= 2 is what keeps the
    * inReal[today-1] below in bounds on the very first bar.
    */
   upTotal = 0.0;
   dnTotal = 0.0;
   for( i = optInTimePeriod; i > 0; i-- )
   {
      tempReal = inReal[today] - shift;
      periodTotal1 += tempReal;
      tempReal *= tempReal;
      periodTotal2 += tempReal;

      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;

      tempReal = inReal[trailingIdx] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
      trailingIdx++;

      barsSinceReseed--;
      if( variance < 0.000001 * ( periodTotal2 * invPeriod )
         || tempReal > 1000000.0 * periodTotal2
         || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInStdDevPeriod;

         windowStart = today - nbInitialElementNeeded;

         tempReal = 0.0;
         for( j=windowStart; j <= today; j++ )
            tempReal += inReal[j];
         shift = tempReal * invPeriod;

         periodTotal1 = 0.0;
         periodTotal2 = 0.0;
         for( j=windowStart; j <= today; j++ )
         {
            tempReal = inReal[j] - shift;
            periodTotal1 += tempReal;
            tempReal *= tempReal;
            periodTotal2 += tempReal;
         }

         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         if( variance < 0.000000000001 * ( periodTotal2 * invPeriod ) )
            variance = 0.0;

         tempReal = inReal[windowStart] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
      }

      sigma = sqrt( variance );

      delta = inReal[today] - inReal[today-1];
      if( delta > 0.0 )
         upTotal += sigma;
      else if( delta < 0.0 )
         dnTotal += sigma;

      today++;
   }

   prevUp = upTotal / optInTimePeriod;
   prevDn = dnTotal / optInTimePeriod;

   /* Skip the unstable period. Same step, smoothed but not stored. */
   i = TA_GetUnstablePeriod(TA_FUNC_UNST_RVI);
   while( i != 0 )
   {
      tempReal = inReal[today] - shift;
      periodTotal1 += tempReal;
      tempReal *= tempReal;
      periodTotal2 += tempReal;

      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;

      tempReal = inReal[trailingIdx] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
      trailingIdx++;

      barsSinceReseed--;
      if( variance < 0.000001 * ( periodTotal2 * invPeriod )
         || tempReal > 1000000.0 * periodTotal2
         || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInStdDevPeriod;

         windowStart = today - nbInitialElementNeeded;

         tempReal = 0.0;
         for( j=windowStart; j <= today; j++ )
            tempReal += inReal[j];
         shift = tempReal * invPeriod;

         periodTotal1 = 0.0;
         periodTotal2 = 0.0;
         for( j=windowStart; j <= today; j++ )
         {
            tempReal = inReal[j] - shift;
            periodTotal1 += tempReal;
            tempReal *= tempReal;
            periodTotal2 += tempReal;
         }

         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         if( variance < 0.000000000001 * ( periodTotal2 * invPeriod ) )
            variance = 0.0;

         tempReal = inReal[windowStart] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
      }

      sigma = sqrt( variance );

      delta = inReal[today] - inReal[today-1];
      upValue = 0.0;
      dnValue = 0.0;
      if( delta > 0.0 )
         upValue = sigma;
      else if( delta < 0.0 )
         dnValue = sigma;

      prevUp = wAlpha * upValue + wBeta * prevUp;
      prevDn = wAlpha * dnValue + wBeta * prevDn;

      today++;
      i--;
   }

   /* A tie feeds neither leg, so both can be exactly zero at the same bar --
    * reachable on real data whenever the smoothing has no memory. Test the
    * total exactly: a band would carry the quote unit and zero the oscillator
    * for any instrument priced under it.
    */
   total = prevUp + prevDn;
   outReal[0] = total == 0.0 ? 50.0 : 100.0*(prevUp/total);
   outIdx = 1;

   while( today <= endIdx )
   {
      tempReal = inReal[today] - shift;
      periodTotal1 += tempReal;
      tempReal *= tempReal;
      periodTotal2 += tempReal;

      meanValue1 = periodTotal1 * invPeriod;
      variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;

      tempReal = inReal[trailingIdx] - shift;
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;
      trailingIdx++;

      barsSinceReseed--;
      if( variance < 0.000001 * ( periodTotal2 * invPeriod )
         || tempReal > 1000000.0 * periodTotal2
         || barsSinceReseed <= 0 )
      {
         barsSinceReseed = 32 * optInStdDevPeriod;

         windowStart = today - nbInitialElementNeeded;

         tempReal = 0.0;
         for( j=windowStart; j <= today; j++ )
            tempReal += inReal[j];
         shift = tempReal * invPeriod;

         periodTotal1 = 0.0;
         periodTotal2 = 0.0;
         for( j=windowStart; j <= today; j++ )
         {
            tempReal = inReal[j] - shift;
            periodTotal1 += tempReal;
            tempReal *= tempReal;
            periodTotal2 += tempReal;
         }

         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         if( variance < 0.000000000001 * ( periodTotal2 * invPeriod ) )
            variance = 0.0;

         tempReal = inReal[windowStart] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
      }

      sigma = sqrt( variance );

      delta = inReal[today] - inReal[today-1];
      upValue = 0.0;
      dnValue = 0.0;
      if( delta > 0.0 )
         upValue = sigma;
      else if( delta < 0.0 )
         dnValue = sigma;

      prevUp = wAlpha * upValue + wBeta * prevUp;
      prevDn = wAlpha * dnValue + wBeta * prevDn;

      total = prevUp + prevDn;
      outReal[outIdx++] = total == 0.0 ? 50.0 : 100.0*(prevUp/total);

      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
