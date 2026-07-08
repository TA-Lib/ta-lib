/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  062704 MF     Fix limit case to avoid divid by zero (or by
 *                a value close to zero induce by the imprecision
 *                of floating points).
 *  070226 MF,CC  Allow period of 1: output is a copy of the input,
 *                consistent with TA_MA (issues #48, #59). The natural
 *                KAMA math at period=1 would be a fixed-alpha EMA
 *                (efficiency ratio is always 1), which would disagree
 *                with TA_MA's period-1 copy, so identity is explicit.
 */

int kama_lookback(int optInTimePeriod)
{
   if( optInTimePeriod == 1 )
      return TA_GetUnstablePeriod(TA_FUNC_UNST_KAMA);

   return optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_KAMA);
}

TA_RetCode kama(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   const double constMax = 2.0/(30.0+1.0);
   const double constDiff = 2.0/(2.0+1.0) - constMax;

   double tempReal, tempReal2;
   double sumROC1, periodROC, prevKAMA;
   int i, today, outIdx, lookbackTotal;
   int trailingIdx;
   double trailingValue;

   /* Default return values */
   *outBegIdx = 0;
   *outNBElement = 0;

   /* No smoothing at period of 1: the output is a copy of the input
    * (same convention as TA_MA for every MAType). The unstable period
    * still delays the first output for API consistency.
    */
   if( optInTimePeriod == 1 )
   {
      lookbackTotal = TA_GetUnstablePeriod(TA_FUNC_UNST_KAMA);
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

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_KAMA);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Initialize the variables by going through
    * the lookback period.
    */
   sumROC1 = 0.0;
   today = startIdx-lookbackTotal;
   trailingIdx = today;
   i = optInTimePeriod;
   while( i-- > 0 )
   {
      tempReal  = inReal[today++];
      tempReal -= inReal[today];
      sumROC1  += fabs(tempReal);
   }

   /* At this point sumROC1 represent the
    * summation of the 1-day price difference
    * over the (optInTimePeriod-1)
    */

   /* Calculate the first KAMA */

   /* The yesterday price is used here as the previous KAMA. */
   prevKAMA = inReal[today-1];

   tempReal  = inReal[today];
   tempReal2 = inReal[trailingIdx++];
   periodROC = tempReal-tempReal2;

   /* Save the trailing value. Do this because inReal
    * and outReal can be pointers to the same buffer.
    */
   trailingValue = tempReal2;

   /* Calculate the efficiency ratio */
   if( (sumROC1 <= periodROC) || TA_IS_ZERO(sumROC1))
      tempReal = 1.0;
   else
      tempReal = fabs(periodROC/sumROC1);

   /* Calculate the smoothing constant */
   tempReal  = (tempReal*constDiff)+constMax;
   tempReal *= tempReal;

   /* Calculate the KAMA like an EMA, using the
    * smoothing constant as the adaptive factor.
    */
   prevKAMA = ((inReal[today++]-prevKAMA)*tempReal) + prevKAMA;

   /* 'today' keep track of where the processing is within the
    * input.
    */

   /* Skip the unstable period. Do the whole processing
    * needed for KAMA, but do not write it in the output.
    */
   while( today <= startIdx )
   {
      tempReal  = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal-tempReal2;

      /* Adjust sumROC1:
       *  - Remove trailing ROC1
       *  - Add new ROC1
       */
      sumROC1 -= fabs(trailingValue-tempReal2);
      sumROC1 += fabs(tempReal-inReal[today-1]);

      /* Save the trailing value. Do this because inReal
       * and outReal can be pointers to the same buffer.
       */
      trailingValue = tempReal2;

      /* Calculate the efficiency ratio */
      if( (sumROC1 <= periodROC) || TA_IS_ZERO(sumROC1) )
         tempReal = 1.0;
      else
         tempReal = fabs(periodROC/sumROC1);

      /* Calculate the smoothing constant */
      tempReal  = (tempReal*constDiff)+constMax;
      tempReal *= tempReal;

      /* Calculate the KAMA like an EMA, using the
       * smoothing constant as the adaptive factor.
       */
      prevKAMA = ((inReal[today++]-prevKAMA)*tempReal) + prevKAMA;
   }

   /* Write the first value. */
   outReal[0] = prevKAMA;
   outIdx = 1;
   *outBegIdx = today-1;

   /* Do the KAMA calculation for the requested range. */
   while( today <= endIdx )
   {
      tempReal  = inReal[today];
      tempReal2 = inReal[trailingIdx++];
      periodROC = tempReal-tempReal2;

      /* Adjust sumROC1:
       *  - Remove trailing ROC1
       *  - Add new ROC1
       */
      sumROC1 -= fabs(trailingValue-tempReal2);
      sumROC1 += fabs(tempReal-inReal[today-1]);

      /* Save the trailing value. Do this because inReal
       * and outReal can be pointers to the same buffer.
       */
      trailingValue = tempReal2;

      /* Calculate the efficiency ratio */
      if( (sumROC1 <= periodROC) || TA_IS_ZERO(sumROC1) )
         tempReal = 1.0;
      else
         tempReal = fabs(periodROC / sumROC1);

      /* Calculate the smoothing constant */
      tempReal  = (tempReal*constDiff)+constMax;
      tempReal *= tempReal;

      /* Calculate the KAMA like an EMA, using the
       * smoothing constant as the adaptive factor.
       */
      prevKAMA = ((inReal[today++]-prevKAMA)*tempReal) + prevKAMA;
      outReal[outIdx++] = prevKAMA;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
