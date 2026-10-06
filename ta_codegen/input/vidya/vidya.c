/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092926 MF,CC  First version (issue #474).
 */

int vidya_lookback(int optInTimePeriod, int optInCMOPeriod)
{
   int root;

   root = (int)sqrt((double)optInCMOPeriod);

   if( optInTimePeriod == 1 )
      return TA_UNSTABLE( TA_FUNC_UNST_VIDYA, 0 );

   return optInCMOPeriod
   + TA_UNSTABLE( TA_FUNC_UNST_VIDYA, ta_auto_stabilization_vidya(X, optInTimePeriod, root) );
}

TA_RetCode vidya(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int optInCMOPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx;
   int today, trailingIdx, lookbackTotal, i;
   int nullRun;
   double upSum, downSum, sum, diff, tempReal, prevValue, trailingValue;
   double alpha, k, prevVIDYA;

   *outBegIdx = 0;
   *outNBElement = 0;

   /* No smoothing at period 1: the output is a copy of the input, as MA gives
    * for every MAType. The unstable period still delays the first output.
    */
   if( optInTimePeriod == 1 )
   {
      lookbackTotal = vidya_lookback( optInTimePeriod, optInCMOPeriod );
      if( startIdx < lookbackTotal )
         startIdx = lookbackTotal;
      if( startIdx > endIdx )
         return TA_SUCCESS;

      *outBegIdx = startIdx;
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx )
         outReal[outIdx++] = inReal[today++];
      *outNBElement = outIdx;
      return TA_SUCCESS;
   }

   lookbackTotal = vidya_lookback( optInTimePeriod, optInCMOPeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   alpha = 2.0 / ((double)(optInTimePeriod + 1));

   /* The CMO below is TA_CMOU's loop, spelling and nullRun reset included, so
    * that from the first full window on VIDYA equals the composite of TA_CMOU
    * bit for bit. The recursion is seeded on the window's first price and
    * steps through the warm-up with the CMO of the changes seen so far. The
    * unstable period enters it that many bars before startIdx.
    */
   today = startIdx - lookbackTotal;
   trailingIdx = today + 1;
   prevValue = inReal[today];
   trailingValue = prevValue;
   prevVIDYA = prevValue;

   upSum = 0.0;
   downSum = 0.0;
   nullRun = 0;
   for( i = 0; i < optInCMOPeriod; i++ )
   {
      today++;
      tempReal = inReal[today];
      diff = tempReal - prevValue;
      prevValue = tempReal;
      if( diff > 0.0 )
         upSum += diff;
      else if( diff < 0.0 )
         downSum -= diff;
      if( diff == 0.0 )
         nullRun++;
      else
         nullRun = 0;

      sum = upSum + downSum;
      if( sum > 0.0 )
         k = alpha * (fabs((100.0 * (upSum - downSum)) / sum) / 100.0);
      else
         k = 0.0;
      prevVIDYA = ((prevValue - prevVIDYA) * k) + prevVIDYA;
   }
   today++;

   /* Skip the unstable period: the whole computation, nothing written. */
   while( today <= startIdx )
   {
      tempReal = inReal[trailingIdx];
      diff = tempReal - trailingValue;
      trailingValue = tempReal;
      trailingIdx++;
      if( diff > 0.0 )
         upSum -= diff;
      else if( diff < 0.0 )
         downSum += diff;

      tempReal = inReal[today];
      diff = tempReal - prevValue;
      prevValue = tempReal;
      if( diff > 0.0 )
         upSum += diff;
      else if( diff < 0.0 )
         downSum -= diff;

      if( diff == 0.0 )
         nullRun++;
      else
         nullRun = 0;
      if( nullRun >= optInCMOPeriod )
      {
         nullRun = optInCMOPeriod;
         upSum = 0.0;
         downSum = 0.0;
      }

      sum = upSum + downSum;
      if( sum > 0.0 )
         k = alpha * (fabs((100.0 * (upSum - downSum)) / sum) / 100.0);
      else
         k = 0.0;
      prevVIDYA = ((prevValue - prevVIDYA) * k) + prevVIDYA;
      today++;
   }

   outReal[0] = prevVIDYA;
   outIdx = 1;

   while( today <= endIdx )
   {
      tempReal = inReal[trailingIdx];
      diff = tempReal - trailingValue;
      trailingValue = tempReal;
      trailingIdx++;
      if( diff > 0.0 )
         upSum -= diff;
      else if( diff < 0.0 )
         downSum += diff;

      tempReal = inReal[today];
      diff = tempReal - prevValue;
      prevValue = tempReal;
      if( diff > 0.0 )
         upSum += diff;
      else if( diff < 0.0 )
         downSum -= diff;

      if( diff == 0.0 )
         nullRun++;
      else
         nullRun = 0;
      if( nullRun >= optInCMOPeriod )
      {
         nullRun = optInCMOPeriod;
         upSum = 0.0;
         downSum = 0.0;
      }

      sum = upSum + downSum;
      if( sum > 0.0 )
         k = alpha * (fabs((100.0 * (upSum - downSum)) / sum) / 100.0);
      else
         k = 0.0;
      prevVIDYA = ((prevValue - prevVIDYA) * k) + prevVIDYA;
      outReal[outIdx++] = prevVIDYA;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
