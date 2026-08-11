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

int max_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode max(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   CIRCBUF_PROLOG(sufHighest,double,30);
   CIRCBUF_PROLOG(preHighest,double,30);
   double highest, tmp;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, today, i;
   int blockStart, nAvail, m;

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
    *
    * Van Herk / Gil-Werman block scan, block-batched form. The p outputs
    * belonging to one block boundary are produced together: one backward
    * pass builds the older block's suffix extrema, one forward pass builds
    * the newer block's prefix extrema, and a third pass combines them.
    * All the loops are straight-line with no data-dependent branching,
    * which is what lets a compiler vectorize them, and the work per bar is
    * a fixed number of comparisons regardless of period. Every scratch
    * array holds COPIES, so input and output may alias.
    *
    * Producing a whole block at a time is also why this cannot be turned
    * into a per-bar automaton, so the streaming tier runs max_ALT1
    * below. See issue #147.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;

   CIRCBUF_INIT( sufHighest, double, optInTimePeriod );
   CIRCBUF_INIT( preHighest, double, optInTimePeriod );

   blockStart = trailingIdx;

   while( today <= endIdx )
   {
      /* Suffix extrema of the block [blockStart, blockStart+p-1], which
       * is fully available here: today == blockStart+p-1 <= endIdx.
       * Scanning backward while keeping the incumbent on a tie
       * leaves the later element holding a tie, which is what lets this
       * compile to a single max instruction.
       */
      i = blockStart + optInTimePeriod - 1;
      highest = inReal[i];
      sufHighest[optInTimePeriod - 1] = highest;
      TA_UNROLL(4)
      while( i > blockStart )
      {
         i--;
         tmp = inReal[i];
         if( tmp > highest )
         {
            highest = tmp;
         }
         sufHighest[i - blockStart] = highest;
      }

      highest = sufHighest[0];
      outReal[outIdx++] = highest;
      trailingIdx++;
      today++;
      if( today > endIdx )
      {
         blockStart = blockStart + optInTimePeriod;
      }
      else
      {
         /* Prefix extrema of the next block, clamped to what remains.
          * Forward, keeping the incumbent on a tie: earliest wins again.
          */
         nAvail = endIdx - (blockStart + optInTimePeriod) + 1;
         if( nAvail > optInTimePeriod - 1 )
         {
            nAvail = optInTimePeriod - 1;
         }
         highest = inReal[blockStart + optInTimePeriod];
         preHighest[0] = highest;
         i = 1;
         TA_UNROLL(4)
         while( i < nAvail )
         {
            tmp = inReal[blockStart + optInTimePeriod + i];
            if( tmp > highest )
            {
               highest = tmp;
            }
            preHighest[i] = highest;
            i++;
         }

         /* Combine. The suffix half is the older one, so preferring it
          * on a tie keeps the earliest-wins rule.
          */
         m = 1;
         while( m <= nAvail )
         {
            highest = sufHighest[m];
            if( preHighest[m - 1] > highest )
            {
               highest = preHighest[m - 1];
            }
            outReal[outIdx++] = highest;
            m++;
         }
         trailingIdx = trailingIdx + nAvail;
         today = today + nAvail;
         blockStart = blockStart + optInTimePeriod;
      }
   }

   CIRCBUF_DESTROY(sufHighest);
   CIRCBUF_DESTROY(preHighest);

   /* Keep the outBegIdx relative to the
    * caller input before returning.
    */
   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}

/* PRAGMA TA_ALT={STREAM,ALL_LANGUAGES} the block scan cannot be a per-bar automaton */
TA_RetCode max_ALT1(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double highest, tmp;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, today, i, highestIdx;

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
    *
    * The highest value of the window is cached with its index; the window is
    * rescanned only when the cached extremum drops out of it. That is O(1)
    * per bar while the extremum sits away from the trailing edge, but it is
    * not amortized O(1): an extremum on the oldest in-window bar drops out
    * on the very next bar, so the rescan repeats and the cost stays
    * O(period) per bar for as long as that persists.
    *
    * Slower than the block scan the batch tier runs; it is here because one
    * bar at a time is exactly what the streaming tier needs. See issue #147.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;

   highestIdx  = -1;
   highest     = 0.0;

   while( today <= endIdx )
   {
      tmp = inReal[today];

      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inReal[highestIdx];
         i = highestIdx;
         TA_UNROLL(4)
         while( ++i<=today )
         {
            tmp = inReal[i];
            if( tmp > highest )
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

      outReal[outIdx++] = highest;
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
