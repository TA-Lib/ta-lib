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
 *  102404 AC   Creation
 *  040309 AC   Increased flexibility to allow real bodies matching
 *              on one end (Greg Morris - "Candlestick charting explained")
 *
 */

int cdlengulfing_lookback(void)
{
   return 2;
}

TA_RetCode cdlengulfing(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   int i, outIdx, lookbackTotal;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlengulfing_lookback();

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
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first: black (white) real body
    * - second: white (black) real body that engulfs the prior real body
    * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish:
    * - 100 is returned when the second candle's real body begins before and ends after the first candle's real body
    * - 80 is returned when the two real bodies match on one end (Greg Morris contemplate this case in his book
    *   "Candlestick charting explained")
    * The user should consider that an engulfing must appear in a downtrend if bullish or in an uptrend if bearish,
    * while this function does not consider it
    */
   outIdx = 0;
   do
   {
      if( ( ta_candlecolor(inClose[i], inOpen[i]) == 1 && ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 &&            // white engulfs black
         ( ( inClose[i] >= inOpen[i-1] && inOpen[i] < inClose[i-1] ) ||
         ( inClose[i] > inOpen[i-1] && inOpen[i] <= inClose[i-1] )
      )
      )
         ||
         ( ta_candlecolor(inClose[i], inOpen[i]) == -1 && ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 &&            // black engulfs white
         ( ( inOpen[i] >= inClose[i-1] && inClose[i] < inOpen[i-1] ) ||
         ( inOpen[i] > inClose[i-1] && inClose[i] <= inOpen[i-1] )
      )
      )
      ) {
         if( inOpen[i] != inClose[i-1] && inClose[i] != inOpen[i-1] ) {
            outInteger[outIdx++] = ta_candlecolor(inClose[i], inOpen[i]) * 100;
         }
         else {
            outInteger[outIdx++] = ta_candlecolor(inClose[i], inOpen[i]) * 80;
         }
      }
      else {
         outInteger[outIdx++] = 0;
      }
      i++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
