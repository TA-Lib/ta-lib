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
 *  120802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *
 */

int adosc_lookback(int optInFastPeriod, int optInSlowPeriod)
{
   int slowestPeriod;

   /* Use the slowest EMA period to evaluate the total lookback. */
   if( optInFastPeriod < optInSlowPeriod )
      slowestPeriod = optInSlowPeriod;
   else
      slowestPeriod = optInFastPeriod;

   /* Adjust startIdx to account for the lookback period. */
   return ema_lookback( slowestPeriod );
}

TA_RetCode adosc(int startIdx, int endIdx, const double inHigh[], const double inLow[], const double inClose[], const double inVolume[], int optInFastPeriod, int optInSlowPeriod, int *outBegIdx, int *outNBElement, double outReal[])
{
   int today, outIdx, lookbackTotal;
   int slowestPeriod;
   double high, low, close, tmp;

   double slowEMA, slowk, one_minus_slowk;
   double fastEMA, fastk, one_minus_fastk;
   double ad;

   /* Implementation Note:
    *     The fastEMA varaible is not neceseraly the
    *     fastest EMA.
    *     In the same way, slowEMA is not neceseraly the
    *     slowest EMA.
    *
    *     The ADOSC is always the (fastEMA - slowEMA) regardless
    *     of the period specified. In other word:
    *
    *     ADOSC(3,10) = EMA(3,AD) - EMA(10,AD)
    *
    *        while
    *
    *     ADOSC(10,3) = EMA(10,AD)- EMA(3,AD)
    *
    *     In the first case the EMA(3) is truly a faster EMA,
    *     while in the second case, the EMA(10) is still call
    *     fastEMA in the algorithm, even if it is in fact slower.
    *
    *     This gives more flexibility to the user if they want to
    *     experiment with unusual parameter settings.
    */

   /* Identify the slowest period.
    * This infomration is used soleley to bootstrap
    * the algorithm (skip the lookback period).
    */
   if( optInFastPeriod < optInSlowPeriod )
      slowestPeriod = optInSlowPeriod;
   else
      slowestPeriod = optInFastPeriod;

   /* Adjust startIdx to account for the lookback period. */
   lookbackTotal = ema_lookback( slowestPeriod );
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx = startIdx;
   today  = startIdx-lookbackTotal;

   /* The following variables are used to
    * calculate the "ad".
    */
   ad = 0.0;

   /* Constants for EMA */
   fastk = (2.0 / ((double)( optInFastPeriod ) + 1.0));
   one_minus_fastk = 1.0 - fastk;

   slowk = (2.0 / ((double)( optInSlowPeriod ) + 1.0));
   one_minus_slowk = 1.0 - slowk;

   /* Initialize the two EMA
    *
    * Use the same range of initialization inputs for
    * both EMA and simply seed with the first A/D value.
    *
    * Note: Metastock do the same.
    */
   high  = inHigh[today];
   low   = inLow[today];
   tmp   = high-low;
   close = inClose[today];
   if( tmp > 0.0 )
      ad += (((close-low)-(high-close))/tmp)*((double)inVolume[today]);
   today++;
   fastEMA = ad;
   slowEMA = ad;

   /* Initialize the EMA and skip the unstable period. */
   while( today < startIdx )
   {
      high  = inHigh[today];
      low   = inLow[today];
      tmp   = high-low;
      close = inClose[today];
      if( tmp > 0.0 )
         ad += (((close-low)-(high-close))/tmp)*((double)inVolume[today]);
      today++;
      fastEMA = (fastk*ad)+(one_minus_fastk*fastEMA);
      slowEMA = (slowk*ad)+(one_minus_slowk*slowEMA);
   }

   /* Perform the calculation for the requested range */
   outIdx = 0;
   while( today <= endIdx )
   {
      high  = inHigh[today];
      low   = inLow[today];
      tmp   = high-low;
      close = inClose[today];
      if( tmp > 0.0 )
         ad += (((close-low)-(high-close))/tmp)*((double)inVolume[today]);
      today++;
      fastEMA = (fastk*ad)+(one_minus_fastk*fastEMA);
      slowEMA = (slowk*ad)+(one_minus_slowk*slowEMA);

      outReal[outIdx++] = fastEMA - slowEMA;
   }
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
