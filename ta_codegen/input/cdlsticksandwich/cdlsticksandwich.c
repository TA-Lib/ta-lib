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
 *  032005 AC   Creation
 *
 */

int cdlsticksandwich_lookback(void)
{
   return Equal_avgPeriod + 2;
}

TA_RetCode cdlsticksandwich(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double EqualPeriodTotal;
   int i, outIdx, EqualTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlsticksandwich_lookback();

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
   EqualPeriodTotal = 0;
   EqualTrailingIdx = startIdx - Equal_avgPeriod;

   i = EqualTrailingIdx;
   while( i < startIdx ) {
      EqualPeriodTotal += ta_candlerange(Equal_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: black candle
    * - second candle: white candle that trades only above the prior close (low > prior close)
    * - third candle: black candle with the close equal to the first candle's close
    * The meaning of "equal" is specified with TA_SetCandleSettings
    * outInteger is always positive (1 to 100): stick sandwich is always bullish;
    * the user should consider that stick sandwich is significant when coming in a downtrend,
    * while this function does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 &&                                                        // first black
         ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 &&                                                         // second white
         ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                                                          // third black
         inLow[i-1] > inClose[i-2] &&                                                        // 2nd low > prior close
         inClose[i] <= inClose[i-2] + ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) && // 1st and 3rd same close
         inClose[i] >= inClose[i-2] - ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2])
      ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      EqualPeriodTotal += ta_candlerange(Equal_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) - ta_candlerange(Equal_rangeType, inOpen[EqualTrailingIdx-2], inHigh[EqualTrailingIdx-2], inLow[EqualTrailingIdx-2], inClose[EqualTrailingIdx-2]);
      i++;
      EqualTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
