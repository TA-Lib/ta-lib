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
 *  121204 AC   Creation
 *
 */

int cdlthrusting_lookback(void)
{
   return max( Equal_avgPeriod, BodyLong_avgPeriod
   ) + 1;
}

TA_RetCode cdlthrusting(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double EqualPeriodTotal, BodyLongPeriodTotal;
   int i, outIdx, EqualTrailingIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlthrusting_lookback();

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
   BodyLongPeriodTotal = 0;
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;

   i = EqualTrailingIdx;
   while( i < startIdx ) {
      EqualPeriodTotal += ta_candlerange(Equal_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long black candle
    * - second candle: white candle with open below previous day low and close into previous day body under the midpoint;
    * to differentiate it from in-neck the close should not be equal to the black candle's close
    * The meaning of "equal" is specified with TA_SetCandleSettings
    * outInteger is negative (-1 to -100): thrusting pattern is always bearish
    * the user should consider that the thrusting pattern is significant when it appears in a downtrend and it could be
    * even bullish "when coming in an uptrend or occurring twice within several days" (Steve Nison says), while this
    * function does not consider the trend
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&                                                        // 1st: black
         ta_realbody(inClose[i-1], inOpen[i-1]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&     //  long
         ta_candlecolor(inClose[i], inOpen[i]) == 1 &&                                                           // 2nd: white
         inOpen[i] < inLow[i-1] &&                                                           //  open below prior low
         inClose[i] > inClose[i-1] + ta_candleaverage(Equal_rangeType, Equal_avgPeriod, Equal_factor, EqualPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) &&  //  close into prior body
         inClose[i] <= inClose[i-1] + ta_realbody(inClose[i-1], inOpen[i-1]) * 0.5                                 //   under the midpoint
      ) {
         outInteger[outIdx++] = -100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      EqualPeriodTotal += ta_candlerange(Equal_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(Equal_rangeType, inOpen[EqualTrailingIdx-1], inHigh[EqualTrailingIdx-1], inLow[EqualTrailingIdx-1], inClose[EqualTrailingIdx-1]);
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1])
      - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-1], inHigh[BodyLongTrailingIdx-1], inLow[BodyLongTrailingIdx-1], inClose[BodyLongTrailingIdx-1]);
      i++;
      EqualTrailingIdx++;
      BodyLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
