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
 *  101902 JV   Speed optimization of the algorithm
 *  102202 MF   Speed optimize a bit further
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *
 */

int min_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode min(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double lowest, tmp;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, lowestIdx, today, i;

   /* Identify the minimum number of price bar needed
    * to identify at least one output over the specified
    * period.
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

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and
    * output to be the same buffer.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;
   lowestIdx   = -1;
   lowest      = 0.0;

   while( today <= endIdx )
   {
      tmp = inReal[today];

      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         lowest = inReal[lowestIdx];
         i = lowestIdx;
         while( ++i<=today )
         {
            tmp = inReal[i];
            if( tmp < lowest )
            {
               lowestIdx = i;
               lowest = tmp;
            }
         }
      }
      else if( tmp <= lowest )
      {
         lowestIdx = today;
         lowest = tmp;
      }

      outReal[outIdx++] = lowest;
      trailingIdx++;
      today++;
   }

   /* Keep the outBegIdx relative to the
    * caller input before returning.
    */
   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
