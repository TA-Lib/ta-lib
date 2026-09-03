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
 *  090126 MF,CC  First version (issue #272).
 *  090326 MF,CC  #338 Two-coefficient Wilder step, in lockstep with TA_ATR.
 */

int supertrend_lookback(int optInTimePeriod, double optInMultiplier)
{
   (void)optInMultiplier;

   /* Every output bar needs the Average True Range at the same bar, and nothing
    * else reaches further back, so the lookback is exactly the callee's. Never
    * restated here, which is what makes SUPERTREND inherit TA_FUNC_UNST_ATR.
    */
   return atr_lookback( optInTimePeriod );
}

TA_RetCode supertrend(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   double optInMultiplier,
   int *outBegIdx, int *outNBElement,
   double outReal[],
   int outInteger[])
{
   int i, today, outIdx, lookbackTotal;
   int isUptrend;

   double prevATR, periodTotal, wAlpha, wBeta;
   double val2, val3, greatest;
   double tempCY, tempLT, tempHT;
   double medianPrice, band, basicUpper, basicLower;
   double finalUpper, finalLower, closeToday, prevClose;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = supertrend_lookback( optInTimePeriod, optInMultiplier );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* The Average True Range is carried inline rather than taken from a call,
    * because the band and the ratchet advance together one bar at a time and a
    * whole-range buffer between them would not stream.
    *
    * The arithmetic order below is the bit-exactness contract with TA_ATR (do
    * not reorder): True Range from high-low, then the two previous-close
    * distances in that order; the seed summed from 0.0 over the first 'period'
    * True Ranges and divided once; the same two Wilder coefficients, wBeta
    * rounded and wAlpha derived from it, in one fused statement.
    */
   wBeta  = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
   wAlpha = 1.0 - wBeta;

   today = startIdx - lookbackTotal + 1;

   periodTotal = 0.0;
   i = optInTimePeriod;
   while( i-- > 0 )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      periodTotal += greatest;
      today++;
   }
   prevATR = periodTotal / optInTimePeriod;

   /* Skip the Average True Range's unstable period. Taking the count from the
    * lookback rather than naming the setting keeps the two from disagreeing.
    */
   i = lookbackTotal - optInTimePeriod;
   while( i != 0 )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      prevATR = wAlpha * greatest + wBeta * prevATR;
      today++;
      i--;
   }

   /* The first bar has no band to ratchet against and no trend to carry, so
    * both bands take their unclamped value and the trend is seeded long, as
    * ta4j's SuperTrendIndicator does. The formula does not settle this and the
    * published implementations are split on it, so the choice is a convention:
    * it stays visible for as long as the first trend lasts, and on a series
    * whose close never leaves the band it never washes out at all.
    */
   medianPrice = (inHigh[startIdx]+inLow[startIdx])/2.0;
   band = optInMultiplier * prevATR;
   finalUpper = medianPrice + band;
   finalLower = medianPrice - band;
   isUptrend = 1;
   prevClose = inClose[startIdx];

   outReal[0] = finalLower;
   outInteger[0] = 1;

   outIdx = 1;
   today = startIdx + 1;

   while( today <= endIdx )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT; /* val1 */

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      prevATR = wAlpha * greatest + wBeta * prevATR;

      medianPrice = (tempHT+tempLT)/2.0;
      band = optInMultiplier * prevATR;
      basicUpper = medianPrice + band;
      basicLower = medianPrice - band;

      /* Each band ratchets toward price and is released only by a close on its
       * far side. Nothing keeps the two ordered -- each is released by its own
       * condition, so the lower one can end up above the upper one -- and where
       * that happens a flip puts the emitted band on the far side of the close.
       * The line is therefore NOT monotone within a trend, and the flag does not
       * always say which side of the line price is on; both read as invariants
       * and neither is one.
       */
      if( (basicUpper < finalUpper) || (prevClose > finalUpper) )
         finalUpper = basicUpper;

      if( (basicLower > finalLower) || (prevClose < finalLower) )
         finalLower = basicLower;

      closeToday = inClose[today];

      /* The trend is carried in its own variable rather than recovered by
       * comparing the previous output against the previous bands. The two
       * carried bands do coincide on some bars -- always on a bar with no true
       * range, and on some of them at a multiplier of zero -- and there that
       * comparison cannot tell them apart, so it would silently lose the
       * hysteresis on exactly the flat input a corpus of real prices lacks.
       */
      if( isUptrend )
      {
         if( closeToday < finalLower )
            isUptrend = 0;
      }
      else
      {
         if( closeToday > finalUpper )
            isUptrend = 1;
      }

      if( isUptrend )
      {
         outReal[outIdx] = finalLower;
         outInteger[outIdx] = 1;
      }
      else
      {
         outReal[outIdx] = finalUpper;
         outInteger[outIdx] = -1;
      }

      prevClose = closeToday;
      outIdx++;
      today++;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
