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
 *  011505 AC   Creation
 *
 */

int cdldoji_lookback(void)
{
   return BodyDoji_avgPeriod;
}

TA_RetCode cdldoji(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double BodyDojiPeriodTotal;
   int i, outIdx, BodyDojiTrailingIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdldoji_lookback();

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
   BodyDojiPeriodTotal = 0;
   BodyDojiTrailingIdx = startIdx - BodyDoji_avgPeriod;

   i = BodyDojiTrailingIdx;
   while( i < startIdx ) {
      BodyDojiPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      i++;
   }

   /* Proceed with the calculation for the requested range.
    *
    * Must have:
    * - open quite equal to close
    * How much can be the maximum distance between open and close is specified with TA_SetCandleSettings
    * outInteger is always positive (1 to 100) but this does not mean it is bullish: doji shows uncertainty and it is
    * neither bullish nor bearish when considered alone
    */
   outIdx = 0;
   do
   {
      if( ta_realbody(inClose[i], inOpen[i]) <= ta_candleaverage(BodyDoji_rangeType, BodyDoji_avgPeriod, BodyDoji_factor, BodyDojiPeriodTotal, inOpen[i], inHigh[i], inLow[i], inClose[i]) ) {
         outInteger[outIdx++] = 100;
      }
      else {
         outInteger[outIdx++] = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      BodyDojiPeriodTotal += ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]) - ta_candlerange(BodyDoji_rangeType, inOpen[BodyDojiTrailingIdx], inHigh[BodyDojiTrailingIdx], inLow[BodyDojiTrailingIdx], inClose[BodyDojiTrailingIdx]);
      i++;
      BodyDojiTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
