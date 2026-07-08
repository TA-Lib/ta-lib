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
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  112605 MF   Fix outBegIdx when startIdx != 0
 *
 */

int typprice_lookback(void)
{
   /* This function have no lookback needed. */

   return 0;
}

TA_RetCode typprice(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx, i;

   /* Typical price = (High + Low + Close ) / 3 */
   outIdx    = 0;

   for( i= startIdx; i <= endIdx; i++ )
   {
      outReal[outIdx++] = ( inHigh [i] +
         inLow  [i] +
         inClose[i] ) / 3.0;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
