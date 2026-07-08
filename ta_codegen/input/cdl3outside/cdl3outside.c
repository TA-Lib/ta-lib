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
 *  121104 AC   Creation
 *
 */

int cdl3outside_lookback(void)
{
   return 3;
}

TA_RetCode cdl3outside(int startIdx, int endIdx,
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

   lookbackTotal = cdl3outside_lookback();

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
    * - third: candle that closes higher (lower) than the second candle
    * outInteger is positive (1 to 100) for the three outside up or negative (-1 to -100) for the three outside down;
    * the user should consider that a three outside up must appear in a downtrend and three outside down must appear
    * in an uptrend, while this function does not consider it
    */
   outIdx = 0;
   do
   {
      if( ( ta_candlecolor(inClose[i-1], inOpen[i-1]) == 1 && ta_candlecolor(inClose[i-2], inOpen[i-2]) == -1 &&          // white engulfs black
         inClose[i-1] > inOpen[i-2] && inOpen[i-1] < inClose[i-2] &&
         inClose[i] > inClose[i-1]                                         // third candle higher
      )
         ||
         ( ta_candlecolor(inClose[i-1], inOpen[i-1]) == -1 && ta_candlecolor(inClose[i-2], inOpen[i-2]) == 1 &&          // black engulfs white
         inOpen[i-1] > inClose[i-2] && inClose[i-1] < inOpen[i-2] &&
         inClose[i] < inClose[i-1]                                         // third candle lower
      )
      )
      {
         outInteger[outIdx++] = ta_candlecolor(inClose[i-1], inOpen[i-1]) * 100;
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
