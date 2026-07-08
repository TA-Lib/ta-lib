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
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  112605 MF   Fix outBegIdx when startIdx != 0
 */

int avgprice_lookback(void)
{
   /* This function have no lookback needed. */
   return 0;
}

TA_RetCode avgprice(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx, i;

   /* Average price = (High + Low + Open + Close) / 4 */

   outIdx = 0;

   for( i=startIdx; i <= endIdx; i++ )
   {
      outReal[outIdx++] = ( inHigh [i] +
         inLow  [i] +
         inClose[i] +
         inOpen [i]) / 4;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
