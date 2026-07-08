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
 *  120904 AC   Creation
 *
 */

int cdldarkcloudcover_lookback(double        optInPenetration)
{
   (void)optInPenetration;

   return BodyLong_avgPeriod + 1;
}

TA_RetCode cdldarkcloudcover(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   double optInPenetration,
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyLongPeriodTotal;
   int i, outIdx, BodyLongTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdldarkcloudcover_lookback(optInPenetration);

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
   BodyLongPeriodTotal = 0;
   BodyLongTrailingIdx = startIdx - BodyLong_avgPeriod;

   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]);
      i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long white candle
    * - second candle: black candle that opens above previous day high and closes within previous day real body;
    * Greg Morris wants the close to be below the midpoint of the previous real body
    * The meaning of "long" is specified with TA_SetCandleSettings, the penetration of the first real body is specified
    * with optInPenetration
    * outInteger is negative (-1 to -100): dark cloud cover is always bearish
    * the user should consider that a dark cloud cover is significant when it appears in an uptrend, while
    * this function does not consider it
    */
   outIdx = 0;
   do
   {
      if( ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 &&                                                     // 1st: white
         ta_realbody(inClose[i-1], inOpen[i-1]) > ta_candleaverage(BodyLong_rangeType, BodyLong_avgPeriod, BodyLong_factor, BodyLongPeriodTotal, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) && //      long
         ta_candlecolor(inClose[i], inOpen[i]) == -1 &&                                                      // 2nd: black
         inOpen[i] > inHigh[i-1] &&                                                      //      open above prior high
         inClose[i] > inOpen[i-1] &&                                                     //      close within prior body
         inClose[i] < inClose[i-1] - ta_realbody(inClose[i-1], inOpen[i-1]) * optInPenetration
      ) {
         outInteger[outIdx++] = -100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyLongPeriodTotal += ta_candlerange(BodyLong_rangeType, inOpen[i-1], inHigh[i-1], inLow[i-1], inClose[i-1]) - ta_candlerange(BodyLong_rangeType, inOpen[BodyLongTrailingIdx-1], inHigh[BodyLongTrailingIdx-1], inLow[BodyLongTrailingIdx-1], inClose[BodyLongTrailingIdx-1]);
      i++;
      BodyLongTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
