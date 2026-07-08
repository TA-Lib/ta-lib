/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  100502 JV   Speed optimization of the algorithm
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

int var_lookback(int optInTimePeriod, double optInNbDev)
{
   (void)optInNbDev;

   return optInTimePeriod-1;
}

TA_RetCode var(int startIdx, int endIdx,
   const double *inReal,
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double *outReal)
{
   double tempReal, periodTotal1, periodTotal2, meanValue1, meanValue2;
   int i, outIdx, trailingIdx, nbInitialElementNeeded;

   /* Validate the calculation method type and
    * identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   nbInitialElementNeeded = (optInTimePeriod-1);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Do the MA calculation using tight loops. */
   /* Add-up the initial periods, except for the last value. */
   periodTotal1 = 0;
   periodTotal2 = 0;
   trailingIdx = startIdx-nbInitialElementNeeded;

   i=trailingIdx;
   if( optInTimePeriod > 1 )
   {
      while( i < startIdx ) {
         tempReal = inReal[i++];
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
   }

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the inReal and
    * outReal to be the same buffer.
    */
   outIdx = 0;
   do
   {
      tempReal = inReal[i++];

      /* Square and add all the deviation over
       * the same periods.
       */

      periodTotal1 += tempReal;
      tempReal *= tempReal;
      periodTotal2 += tempReal;

      /* Square and add all the deviation over
       * the same period.
       */

      meanValue1 = periodTotal1 / optInTimePeriod;
      meanValue2 = periodTotal2 / optInTimePeriod;

      tempReal = inReal[trailingIdx++];
      periodTotal1 -= tempReal;
      tempReal *= tempReal;
      periodTotal2 -= tempReal;

      outReal[outIdx++] = meanValue2-meanValue1*meanValue1;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
