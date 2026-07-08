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
 *
 */

int wma_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1;
}

TA_RetCode wma(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int inIdx, outIdx, i, trailingIdx, divider;
   double periodSum, periodSub, tempReal, trailingValue;
   int lookbackTotal;

   lookbackTotal = optInTimePeriod-1;

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

   /* To make the rest more efficient, handle exception
    * case where the user is asking for a period of '1'.
    * In that case outputs equals inputs for the requested
    * range.
    */
   if( optInTimePeriod == 1 )
   {
      *outBegIdx    = startIdx;
      *outNBElement = endIdx-startIdx+1;

      memmove(outReal, &inReal[startIdx], ((int)*outNBElement) * sizeof(double));

      return TA_SUCCESS;
   }

   /* Calculate the divider (always an integer value).
    * By induction: 1+2+3+4+'n' = n(n+1)/2
    * '>>1' is usually faster than '/2' for unsigned.
    */
   divider = (optInTimePeriod*(optInTimePeriod+1))>>1;

   /* The algo used here use a very basic property of
    * multiplication/addition: (x*2) = x+x
    *
    * As an example, a 3 period weighted can be
    * interpreted in two way:
    *  (x1*1)+(x2*2)+(x3*3)
    *      OR
    *  x1+x2+x2+x3+x3+x3 (this is the periodSum)
    *
    * When you move forward in the time serie
    * you can quickly adjust the periodSum for the
    * period by substracting:
    *   x1+x2+x3 (This is the periodSub)
    * Making the new periodSum equals to:
    *   x2+x3+x3
    *
    * You can then add the new price bar
    * which is x4+x4+x4 giving:
    *   x2+x3+x3+x4+x4+x4
    *
    * At this point one iteration is completed and you can
    * see that we are back to the step 1 of this example.
    *
    * Why making it so un-intuitive? The number of memory
    * access and floating point operations are kept to a
    * minimum with this algo.
    */
   outIdx      = 0;
   trailingIdx = startIdx - lookbackTotal;

   /* Evaluate the initial periodSum/periodSub and trailingValue. */
   periodSum = periodSub = (double)0.0;
   inIdx=trailingIdx;
   i = 1;
   while( inIdx < startIdx )
   {
      tempReal = inReal[inIdx++];
      periodSub += tempReal;
      periodSum += tempReal*i;
      i++;
   }
   trailingValue = 0.0;

   /* Tight loop for the requested range. */
   while( inIdx <= endIdx )
   {
      /* Add the current price bar to the sum
       * who are carried through the iterations.
       */
      tempReal = inReal[inIdx++];
      periodSub += tempReal;
      periodSub -= trailingValue;
      periodSum += tempReal*optInTimePeriod;

      /* Save the trailing value for being substract at
       * the next iteration.
       * (must be saved here just in case outReal and
       *  inReal are the same buffer).
       */
      trailingValue = inReal[trailingIdx++];

      /* Calculate the WMA for this price bar. */
      outReal[outIdx++] = periodSum / divider;

      /* Prepare the periodSum for the next iteration. */
      periodSum -= periodSub;
   }

   /* Set output limits. */
   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
