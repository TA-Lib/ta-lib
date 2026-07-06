/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AB       Anatoliy Belsky
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  181012 AB    Initial Version
 *  070526 MF,CC  Fix #98: the unstable period grew the summation window
 *                to period+u bars; window is now always 'period'.
 */

int imi_lookback(int           optInTimePeriod)
{
   return optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_IMI) - 1;
}

TA_RetCode imi(int startIdx, int endIdx, const double inOpen[], const double inClose[], int optInTimePeriod, int *outBegIdx, int *outNBElement, double outReal[])
{
   int lookback, outIdx = 0;

   lookback = imi_lookback( optInTimePeriod );

   if(startIdx < lookback)
      startIdx = lookback;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx ) {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx = startIdx;

   while (startIdx <= endIdx) {
      double upsum = .0, downsum = .0;
      int i;

      for (i = startIdx - optInTimePeriod + 1; i <= startIdx; i++) {
         double close = inClose[i];
         double open = inOpen[i];

         if (close > open) {
            upsum += (close - open);
         } else {
            downsum += (open - close);
         }

         outReal[outIdx] = 100.0*(upsum/(upsum + downsum));
      }

      startIdx++;
      outIdx++;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
