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
 *  062804 MF   Resolve div by zero bug on limit case.
 */

int rsi_lookback(int optInTimePeriod)
{
   int retValue;

   retValue = optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_RSI);
   if( TA_GetCompatibility() == TA_COMPATIBILITY_METASTOCK )
   {
      retValue = retValue - 1;
   }

   return retValue;
}

TA_RetCode rsi(int startIdx, int endIdx, const double inReal[], int optInTimePeriod, int *outBegIdx, int *outNBElement, double outReal[])
{
   size_t outIdx;
   size_t today;
   size_t lookbackTotal;
   int unstablePeriod;
   int i;
   double prevGain;
   double prevLoss;
   double prevValue;
   double savePrevValue;
   double tempValue1;
   double tempValue2;

   /* The following algorithm is base on the original
    * work from Wilder's and shall represent the
    * original idea behind the classic RSI.
    *
    * Metastock is starting the calculation one price
    * bar earlier. To make this possible, they assume
    * that the very first bar will be identical to the
    * previous one (no gain or loss).
    */

   /* If changing this function, please check also CMO
    * which is mostly identical (just different in one step
    * of calculation).
    */

   *outBegIdx = 0;
   *outNBElement = 0;

   /* Adjust startIdx to account for the lookback period. */
   lookbackTotal = (int)(rsi_lookback( optInTimePeriod ));

   if( startIdx < lookbackTotal )
   {
      startIdx = lookbackTotal;
   }

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      return TA_SUCCESS;
   }

   outIdx = 0; /* Index into the output. */

   /* Trap special case where the period is '1'.
    * In that case, just copy the input into the
    * output for the requested range (as-is !)
    */
   if( optInTimePeriod == 1 )
   {
      *outBegIdx = startIdx;
      i = (int)((endIdx-startIdx)+1);
      *outNBElement = (size_t)i;
      /* memmove, not memcpy: an in-place caller (outReal == inReal) with
       * startIdx > 0 overlaps source and destination (issue #94; matches WMA). */
      memmove(&outReal[0], &inReal[startIdx], (i) * sizeof(double));
      return TA_SUCCESS;
   }

   /* Accumulate Wilder's "Average Gain" and "Average Loss"
    * among the initial period.
    */
   today = startIdx-lookbackTotal;
   prevValue = (double)(inReal[today]);

   unstablePeriod = TA_GetUnstablePeriod(TA_FUNC_UNST_RSI);

   /* If there is no unstable period,
    * calculate the 'additional' initial
    * price bar who is particuliar to
    * metastock.
    * If there is an unstable period,
    * no need to calculate since this
    * first value will be surely skip.
    */
   if( (unstablePeriod == 0) &&
      (TA_GetCompatibility() == TA_COMPATIBILITY_METASTOCK))
   {
      /* Preserve prevValue because it may get
       * overwritten by the output.
       *(because output ptr could be the same as input ptr).
       */
      savePrevValue = prevValue;

      /* No unstable period, so must calculate first output
       * particular to Metastock.
       * (Metastock re-use the first price bar, so there
       *  is no loss/gain at first. Beats me why they
       *  are doing all this).
       */
      prevGain = 0.0;
      prevLoss = 0.0;
      for( i = optInTimePeriod; i > 0; i-- ) {
         tempValue1 = (double)(inReal[today]); today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue  = tempValue1;
         if( tempValue2 < 0.0 )
         {
            prevLoss -= tempValue2;
         }
         else
         {
            prevGain += tempValue2;
         }
      }

      tempValue1 = prevLoss/(double)optInTimePeriod;
      tempValue2 = prevGain/(double)optInTimePeriod;

      /* Write the output. */
      tempValue1 = tempValue2+tempValue1;
      if( !TA_IS_ZERO(tempValue1) )
      {
         outReal[outIdx] = 100.0*(tempValue2/tempValue1); outIdx = outIdx + 1;
      }
      else
      {
         outReal[outIdx] = 0.0; outIdx = outIdx + 1;
      }

      /* Are we done? */
      if( today > endIdx )
      {
         *outBegIdx    = startIdx;
         *outNBElement = outIdx;
         return TA_SUCCESS;
      }

      /* Start over for the next price bar. */
      today = today - (size_t)optInTimePeriod;
      prevValue = savePrevValue;
   }

   /* Remaining of the processing is identical
    * for both Classic calculation and Metastock.
    */
   prevGain = 0.0;
   prevLoss = 0.0;
   today = today + 1;
   for( i = optInTimePeriod; i > 0; i-- ) {
      tempValue1 = (double)(inReal[today]); today = today + 1;
      tempValue2 = tempValue1 - prevValue;
      prevValue  = tempValue1;
      if( tempValue2 < 0.0 )
      {
         prevLoss -= tempValue2;
      }
      else
      {
         prevGain += tempValue2;
      }
   }

   /* Subsequent prevLoss and prevGain are smoothed
    * using the previous values (Wilder's approach).
    *  1) Multiply the previous by 'period-1'.
    *  2) Add today value.
    *  3) Divide by 'period'.
    */
   prevLoss /= (double)optInTimePeriod;
   prevGain /= (double)optInTimePeriod;

   /* Often documentation present the RSI calculation as follow:
    *    RSI = 100 - (100 / 1 + (prevGain/prevLoss))
    *
    * The following is equivalent:
    *    RSI = 100 * (prevGain/(prevGain+prevLoss))
    *
    * The second equation is used here for speed optimization.
    */
   if( today > startIdx )
   {
      tempValue1 = prevGain+prevLoss;
      if( !TA_IS_ZERO(tempValue1) )
      {
         outReal[outIdx] = 100.0*(prevGain/tempValue1); outIdx = outIdx + 1;
      }
      else
      {
         outReal[outIdx] = 0.0; outIdx = outIdx + 1;
      }
   }
   else
   {
      /* Skip the unstable period. Do the processing
       * but do not write it in the output.
       */
      while( today < startIdx )
      {
         tempValue1 = (double)(inReal[today]);
         tempValue2 = tempValue1 - prevValue;
         prevValue  = tempValue1;

         prevLoss *= (double)(optInTimePeriod-1);
         prevGain *= (double)(optInTimePeriod-1);
         if( tempValue2 < 0.0 )
         {
            prevLoss -= tempValue2;
         }
         else
         {
            prevGain += tempValue2;
         }

         prevLoss /= (double)optInTimePeriod;
         prevGain /= (double)optInTimePeriod;

         today = today + 1;
      }
   }

   /* Unstable period skipped... now continue
    * processing if needed.
    */
   while( today <= endIdx )
   {
      tempValue1 = (double)(inReal[today]); today = today + 1;
      tempValue2 = tempValue1 - prevValue;
      prevValue  = tempValue1;

      prevLoss *= (double)(optInTimePeriod-1);
      prevGain *= (double)(optInTimePeriod-1);
      if( tempValue2 < 0.0 )
      {
         prevLoss -= tempValue2;
      }
      else
      {
         prevGain += tempValue2;
      }

      prevLoss /= (double)optInTimePeriod;
      prevGain /= (double)optInTimePeriod;
      tempValue1 = prevGain+prevLoss;
      if( !TA_IS_ZERO(tempValue1) )
      {
         outReal[outIdx] = 100.0*(prevGain/tempValue1); outIdx = outIdx + 1;
      }
      else
      {
         outReal[outIdx] = 0.0; outIdx = outIdx + 1;
      }
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
