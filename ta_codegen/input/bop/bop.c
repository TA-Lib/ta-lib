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
 *  112605 MF   Initial coding.
 *
 */

int bop_lookback(void)
{
   return 0;
}

TA_RetCode bop(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx, i;
   double tempReal;

   /* BOP = (Close - Open)/(High - Low) */

   outIdx = 0;

   for( i=startIdx; i <= endIdx; i++ )
   {
      tempReal = inHigh[i]-inLow[i];
      if( TA_IS_ZERO_OR_NEG(tempReal) )
         outReal[outIdx++] = 0.0;
      else
         outReal[outIdx++] = (inClose[i] - inOpen[i])/tempReal;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
