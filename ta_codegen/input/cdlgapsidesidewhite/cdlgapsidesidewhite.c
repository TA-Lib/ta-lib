/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  020605 AC   Creation
 *
 */

int cdlgapsidesidewhite_lookback(void)
{
   return max( Near_avgPeriod, Equal_avgPeriod ) + 2;
}

TA_RetCode cdlgapsidesidewhite(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double NearPeriodTotal, EqualPeriodTotal;
   int i, outIdx, NearTrailingIdx, EqualTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlgapsidesidewhite_lookback();

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

   /* Do the calculation using tight loops. */
   /* Add-up the initial period, except for the last value. */
   NearPeriodTotal = 0;
   EqualPeriodTotal = 0;
   NearTrailingIdx = startIdx - Near_avgPeriod;
   EqualTrailingIdx = startIdx - Equal_avgPeriod;

   i = NearTrailingIdx;
   while( i < startIdx ) {
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = EqualTrailingIdx;
   while( i < startIdx ) {
      EqualPeriodTotal += ta_candlerange(Equal_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - upside or downside gap (between the bodies)
    * - first candle after the window: white candlestick
    * - second candle after the window: white candlestick with similar size (near the same) and about the same
    *   open (equal) of the previous candle
    * - the second candle does not close the window
    * The meaning of "near" and "equal" is specified with TA_SetCandleSettings
    * outInteger is positive (1 to 100) or negative (-1 to -100): the user should consider that upside
    * or downside gap side-by-side white lines is significant when it appears in a trend, while this function
    * does not consider the trend
    */
   outIdx = 0;
   do
   {
      if(
         ( // upside or downside gap between the 1st candle and both the next 2 candles
         ( ta_realbodygapup(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2]) && ta_realbodygapup(inOpen[i], inClose[i], inOpen[i-2], inClose[i-2]) )
         ||
         ( ta_realbodygapdown(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2]) && ta_realbodygapdown(inOpen[i], inClose[i], inOpen[i-2], inClose[i-2]) )
      ) &&
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 &&                                                                 // 2nd: white
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                                                   // 3rd: white
         ta_realbody(inClose[i], inOpen[i]) >= ta_realbody(inClose[i-1], inOpen[i-1]) - ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&   // same size 2 and 3
         ta_realbody(inClose[i], inOpen[i]) <= ta_realbody(inClose[i-1], inOpen[i-1]) + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&
         inOpen[i] >= inOpen[i-1] - ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&           // same open 2 and 3
         inOpen[i] <= inOpen[i-1] + ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      ) {
         outInteger[outIdx++] = ( ta_realbodygapup(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2]) ? 100 : -100 );
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx-1], inHigh[NearTrailingIdx-1], inLow[NearTrailingIdx-1], inClose[NearTrailingIdx-1]);
      EqualPeriodTotal += ta_candlerange(Equal_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(Equal_rangeType, inOpen[EqualTrailingIdx-1], inHigh[EqualTrailingIdx-1], inLow[EqualTrailingIdx-1], inClose[EqualTrailingIdx-1]);
      i++;
      NearTrailingIdx++;
      EqualTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
