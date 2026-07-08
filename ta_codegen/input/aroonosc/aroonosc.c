/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel <michel@pacbell.net>
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  050703 MF   Fix algorithm base on Adrian Michel bug report #748163
 */

int aroonosc_lookback(int optInTimePeriod)
{
   return optInTimePeriod;
}

TA_RetCode aroonosc(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double lowest, highest, tmp, factor, aroon;
   int outIdx;
   int trailingIdx, lowestIdx, highestIdx, today, i;

   /* This code is almost identical to the TA_AROON function
    * except that instead of outputing ArroonUp and AroonDown
    * individually, an oscillator is build from both.
    *
    *  AroonOsc = AroonUp- AroonDown;
    *
    */

   /* This function is using a speed optimized algorithm
    * for the min/max logic.
    *
    * You might want to first look at how TA_MIN/TA_MAX works
    * and this function will become easier to understand.
    */

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < optInTimePeriod )
      startIdx = optInTimePeriod;

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
   trailingIdx = startIdx-optInTimePeriod;
   lowestIdx   = -1;
   highestIdx  = -1;
   lowest      = 0.0;
   highest     = 0.0;
   factor      = (double)100.0/(double)optInTimePeriod;

   while( today <= endIdx )
   {
      /* Keep track of the lowestIdx */
      tmp = inLow[today];
      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         lowest = inLow[lowestIdx];
         i = lowestIdx;
         while( ++i<=today )
         {
            tmp = inLow[i];
            if( tmp <= lowest )
            {
               lowestIdx = i;
               lowest = tmp;
            }
         }
      }
      else if( tmp <= lowest )
      {
         lowestIdx = today;
         lowest    = tmp;
      }

      /* Keep track of the highestIdx */
      tmp = inHigh[today];
      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inHigh[highestIdx];
         i = highestIdx;
         while( ++i<=today )
         {
            tmp = inHigh[i];
            if( tmp >= highest )
            {
               highestIdx = i;
               highest = tmp;
            }
         }
      }
      else if( tmp >= highest )
      {
         highestIdx = today;
         highest = tmp;
      }

      /* The oscillator is the following:
       *  AroonUp   = factor*(optInTimePeriod-(today-highestIdx));
       *  AroonDown = factor*(optInTimePeriod-(today-lowestIdx));
       *  AroonOsc  = AroonUp-AroonDown;
       *
       * An arithmetic simplification give us:
       *  Aroon = factor*(highestIdx-lowestIdx)
       */
      aroon = factor*(highestIdx-lowestIdx);

      /* Note: Do not forget that input and output buffer can be the same,
       *       so writing to the output is the last thing being done here.
       */
      outReal[outIdx] = aroon;

      outIdx++;
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
