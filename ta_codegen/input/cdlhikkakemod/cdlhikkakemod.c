/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  122605 AC   Creation
 *  071226 MF,CC Streaming-friendly rewrite: carry the confirmation state
 *               (countdown + cached 3rd-candle high/low) instead of the absolute
 *               bar index, so the per-bar logic reads no cursor. Bit-identical
 *               batch results (verified vs v0.6.4).
 */

int cdlhikkakemod_lookback(void)
{
   return max( 1, Near_avgPeriod ) + 5;
}

TA_RetCode cdlhikkakemod(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   int outInteger[])
{
   double NearPeriodTotal;
   int i, outIdx, NearTrailingIdx, lookbackTotal, patternResult;

   /* Confirmation window countdown (replaces the absolute patternIdx guard)
    * and a cache of the 3rd candle's high/low (replaces inHigh/inLow
    * [patternIdx-1]) so nothing in the per-bar logic references the cursor.
    */
   int patternCount;
   double patternHigh, patternLow;

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = cdlhikkakemod_lookback();

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
   NearTrailingIdx = startIdx - 3 - Near_avgPeriod;
   i = NearTrailingIdx;
   while( i < startIdx - 3 ) {
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]);
      i++;
   }

   patternCount = 0;
   patternResult = 0;
   patternHigh = 0.0;
   patternLow = 0.0;

   i = startIdx - 3;
   while( i < startIdx ) {
      /* copy here the pattern recognition code below */
      if( inHigh[i-2] < inHigh[i-3] && inLow[i-2] > inLow[i-3] &&             // 2nd: lower high and higher low than 1st
         inHigh[i-1] < inHigh[i-2] && inLow[i-1] > inLow[i-2] &&             // 3rd: lower high and higher low than 2nd
         ( ( inHigh[i] < inHigh[i-1] && inLow[i] < inLow[i-1] &&             // (bull) 4th: lower high and lower low
         inClose[i-2] <= inLow[i-2] + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2])
         // (bull) 2nd: close near the low
      )
         ||
         ( inHigh[i] > inHigh[i-1] && inLow[i] > inLow[i-1] &&             // (bear) 4th: higher high and higher low
         inClose[i-2] >= inHigh[i-2] - ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2])
         // (bull) 2nd: close near the top
      )
      )
      ) {
         patternResult = 100 * ( inHigh[i] < inHigh[i-1] ? 1 : -1 );
         patternHigh = inHigh[i-1];
         patternLow = inLow[i-1];
         patternCount = 4;
      } else
      /* search for confirmation if modified hikkake was no more than 3 bars ago */
      if( patternCount > 0 &&
         ( ( patternResult > 0 && inClose[i] > patternHigh )    // close higher than the high of 3rd
         ||
         ( patternResult < 0 && inClose[i] < patternLow )     // close lower than the low of 3rd
      )
      ) {
         patternCount = 0;
      }
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) - ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx-2], inHigh[NearTrailingIdx-2], inLow[NearTrailingIdx-2], inClose[NearTrailingIdx-2]);
      NearTrailingIdx++;
      if( patternCount > 0 ) patternCount--;
      i++;
   }

   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle
    * - second candle: candle with range less than first candle and close near the bottom (near the top)
    * - third candle: lower high and higher low than 2nd
    * - fourth candle: lower high and lower low (higher high and higher low) than 3rd
    * outInteger[hikkake bar] is positive (1 to 100) or negative (-1 to -100) meaning bullish or bearish hikkake
    * Confirmation could come in the next 3 days with:
    * - a day that closes higher than the high (lower than the low) of the 3rd candle
    * outInteger[confirmationbar] is equal to 100 + the bullish hikkake result or -100 - the bearish hikkake result
    * Note: if confirmation and a new hikkake come at the same bar, only the new hikkake is reported (the new hikkake
    * overwrites the confirmation of the old hikkake);
    * the user should consider that modified hikkake is a reversal pattern, while hikkake could be both a reversal
    * or a continuation pattern, so bullish (bearish) modified hikkake is significant when appearing in a downtrend
    * (uptrend)
    */
   outIdx = 0;
   do
   {
      if( inHigh[i-2] < inHigh[i-3] && inLow[i-2] > inLow[i-3] &&             // 2nd: lower high and higher low than 1st
         inHigh[i-1] < inHigh[i-2] && inLow[i-1] > inLow[i-2] &&             // 3rd: lower high and higher low than 2nd
         ( ( inHigh[i] < inHigh[i-1] && inLow[i] < inLow[i-1] &&             // (bull) 4th: lower high and lower low
         inClose[i-2] <= inLow[i-2] + ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2])
         // (bull) 2nd: close near the low
      )
         ||
         ( inHigh[i] > inHigh[i-1] && inLow[i] > inLow[i-1] &&             // (bear) 4th: higher high and higher low
         inClose[i-2] >= inHigh[i-2] - ta_candleaverage(Near_rangeType, Near_avgPeriod, Near_factor, NearPeriodTotal, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2])
         // (bull) 2nd: close near the top
      )
      )
      ) {
         patternResult = 100 * ( inHigh[i] < inHigh[i-1] ? 1 : -1 );
         patternHigh = inHigh[i-1];
         patternLow = inLow[i-1];
         patternCount = 4;
         outInteger[outIdx++] = patternResult;
      } else
      /* search for confirmation if modified hikkake was no more than 3 bars ago */
      if( patternCount > 0 &&
         ( ( patternResult > 0 && inClose[i] > patternHigh )    // close higher than the high of 3rd
         ||
         ( patternResult < 0 && inClose[i] < patternLow )     // close lower than the low of 3rd
      )
      ) {
         outInteger[outIdx++] = patternResult + 100 * ( patternResult > 0 ? 1 : -1 );
         patternCount = 0;
      } else {
         outInteger[outIdx++] = 0;
      }
      NearPeriodTotal += ta_candlerange(Near_rangeType, inOpen[i-2], inHigh[i-2], inLow[i-2], inClose[i-2]) - ta_candlerange(Near_rangeType, inOpen[NearTrailingIdx-2], inHigh[NearTrailingIdx-2], inLow[NearTrailingIdx-2], inClose[NearTrailingIdx-2]);
      NearTrailingIdx++;
      if( patternCount > 0 ) patternCount--;
      i++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
