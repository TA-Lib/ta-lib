/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120906 AC   Creation
 *
 */

int minmax_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode minmax(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outMin[],
   double outMax[])
{
   CIRCBUF_PROLOG(sufHighest,double,30);
   CIRCBUF_PROLOG(preHighest,double,30);
   CIRCBUF_PROLOG(sufLowest,double,30);
   CIRCBUF_PROLOG(preLowest,double,30);
   double highest, lowest, tmpHigh;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, today, i;
   int blockStart, nAvail, m, blockNext;

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
    * Both extrema travel in the same passes.
    * All the loops are straight-line with no data-dependent branching,
    * which is what lets a compiler vectorize them, and the work per bar is
    * a fixed number of comparisons regardless of period. Every scratch
    * array holds COPIES, so input and output may alias.
    *
    * Producing a whole block at a time is also why this cannot be turned
    * into a per-bar automaton, so the streaming tier runs minmax_ALT1
    * below. See issue #147.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;

   CIRCBUF_INIT( sufHighest, double, optInTimePeriod );
   CIRCBUF_INIT( preHighest, double, optInTimePeriod );
   CIRCBUF_INIT( sufLowest, double, optInTimePeriod );
   CIRCBUF_INIT( preLowest, double, optInTimePeriod );

   blockStart = trailingIdx;

   while( today <= endIdx )
   {
      /* Suffix extrema of the block [blockStart, blockStart+p-1], which
       * is fully available here: today == blockStart+p-1 <= endIdx.
       * Scanning backward while keeping the incumbent on a tie
       * leaves the later element holding a tie, which is what lets this
       * compile to a single min/max instruction.
       */
      i = blockStart + optInTimePeriod - 1;
      highest = inReal[i];
      lowest = highest;
      sufHighest[optInTimePeriod - 1] = highest;
      sufLowest[optInTimePeriod - 1] = lowest;
      TA_UNROLL(4)
      while( i > blockStart )
      {
         i--;
         tmpHigh = inReal[i];
         if( tmpHigh > highest )
         {
            highest = tmpHigh;
         }
         if( tmpHigh < lowest )
         {
            lowest = tmpHigh;
         }
         sufHighest[i - blockStart] = highest;
         sufLowest[i - blockStart] = lowest;
      }

      outMax[outIdx] = sufHighest[0];
      outMin[outIdx] = sufLowest[0];
      outIdx++;
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
         blockNext = blockStart + optInTimePeriod;
         nAvail = endIdx - blockNext + 1;
         if( nAvail > optInTimePeriod - 1 )
         {
            nAvail = optInTimePeriod - 1;
         }
         highest = inReal[blockNext];
         lowest = highest;
         preHighest[0] = highest;
         preLowest[0] = lowest;
         i = 1;
         TA_UNROLL(4)
         while( i < nAvail )
         {
            tmpHigh = inReal[blockNext + i];
            if( tmpHigh > highest )
            {
               highest = tmpHigh;
            }
            if( tmpHigh < lowest )
            {
               lowest = tmpHigh;
            }
            preHighest[i] = highest;
            preLowest[i] = lowest;
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
            lowest = sufLowest[m];
            if( preLowest[m - 1] < lowest )
            {
               lowest = preLowest[m - 1];
            }
            outMax[outIdx] = highest;
            outMin[outIdx] = lowest;
            outIdx++;
            m++;
         }
         trailingIdx = trailingIdx + nAvail;
         today = today + nAvail;
         blockStart = blockStart + optInTimePeriod;
      }
   }

   CIRCBUF_DESTROY(sufHighest);
   CIRCBUF_DESTROY(preHighest);
   CIRCBUF_DESTROY(sufLowest);
   CIRCBUF_DESTROY(preLowest);

   /* Keep the outBegIdx relative to the
    * caller input before returning.
    */
   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}

/* PRAGMA TA_ALT={STREAM,ALL_LANGUAGES} the block scan cannot be a per-bar automaton */
TA_RetCode minmax_ALT1(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outMin[],
   double outMax[])
{
   double highest, lowest, tmpHigh, tmpLow;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, today, i, highestIdx, lowestIdx;

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
    * The highest and lowest values of the window are cached with their
    * indices; the window is rescanned only when a cached extremum drops out
    * of it. That is O(1)
    * per bar while the extremum sits away from the trailing edge, but it is
    * not amortized O(1): an extremum on the oldest in-window bar drops out
    * on the very next bar, so the rescan repeats and the cost stays
    * O(period) per bar for as long as that persists.
    *
    * Tracking both extrema keeps that state going through a trend: while
    * the high is refreshed by each new bar, the low stays pinned at the
    * oldest bar for the whole leg (and the reverse on the way down). A flat
    * stretch pins both. Random-walk input is the favourable case, where
    * rescans are rare.
    *
    * Slower than the block scan the batch tier runs; it is here because one
    * bar at a time is exactly what the streaming tier needs. See issue #147.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;

   highestIdx  = -1;
   highest     = 0.0;
   lowestIdx   = -1;
   lowest      = 0.0;

   while( today <= endIdx )
   {
      tmpLow = tmpHigh = inReal[today];

      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inReal[highestIdx];
         i = highestIdx;
         TA_UNROLL(4)
         while( ++i<=today )
         {
            tmpHigh = inReal[i];
            if( tmpHigh > highest )
            {
               highestIdx = i;
               highest = tmpHigh;
            }
         }
      }
      else if( tmpHigh >= highest )
      {
         highestIdx = today;
         highest = tmpHigh;
      }

      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         lowest = inReal[lowestIdx];
         i = lowestIdx;
         TA_UNROLL(4)
         while( ++i<=today )
         {
            tmpLow = inReal[i];
            if( tmpLow < lowest )
            {
               lowestIdx = i;
               lowest = tmpLow;
            }
         }
      }
      else if( tmpLow <= lowest )
      {
         lowestIdx = today;
         lowest = tmpLow;
      }

      outMax[outIdx] = highest;
      outMin[outIdx] = lowest;
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
