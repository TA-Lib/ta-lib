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

int sub_lookback(void)
{
   return 0;
}

TA_RetCode sub(int startIdx, int endIdx,
   const double inReal0[],
   const double inReal1[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx;
   int i;

   /* Default return values */
   for( i=startIdx, outIdx=0; i <= endIdx; i++, outIdx++ )
   {
      outReal[outIdx] = inReal0[i]-inReal1[i];
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
