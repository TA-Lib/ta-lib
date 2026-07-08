/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090807 MF     Initial Version
 */

int atan_lookback(void)
{
   return 0;
}

TA_RetCode atan(int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx;
   int i;

   /* Default return values */
   for( i=startIdx, outIdx=0; i <= endIdx; i++, outIdx++ )
   {
      outReal[outIdx] = atan(inReal[i]);
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
