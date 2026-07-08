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
 *  090812 AB     Initial Version
 */

int avgdev_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode avgdev(int startIdx, int endIdx, const double inReal[], int optInTimePeriod, int *outBegIdx, int *outNBElement, double outReal[])
{
   int today, outIdx, lookback;

   lookback = optInTimePeriod - 1;

   if (startIdx < lookback) {
      startIdx = lookback;
   }
   today = startIdx;

   /* Make sure there is still something to evaluate. */
   if( today > endIdx ) {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Process the initial DM and TR */
   *outBegIdx = today;

   outIdx = 0;

   while (today <= endIdx) {
      double todaySum, todayDev;
      int i;

      todaySum = 0.0;
      for (i = 0; i < optInTimePeriod; i++) {
         todaySum += inReal[today-i];
      }

      todayDev = 0.0;
      for (i = 0; i < optInTimePeriod; i++) {
         todayDev += fabs(inReal[today-i] - todaySum/optInTimePeriod);
      }
      outReal[outIdx] = todayDev/optInTimePeriod;

      outIdx++;
      today++;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
