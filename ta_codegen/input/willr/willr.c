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
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *
 */

int willr_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode willr(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   CIRCBUF_PROLOG(sufHighest,double,30);
   CIRCBUF_PROLOG(preHighest,double,30);
   CIRCBUF_PROLOG(sufLowest,double,30);
   CIRCBUF_PROLOG(preLowest,double,30);
   double lowest, highest, tmp, diff;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx;
   int today, i;
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

   /* Initialize 'diff', just to avoid warning. */
   diff = 0.0;

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
    * into a per-bar automaton, so the streaming tier runs willr_ALT1
    * below. See issue #147.
    */
   outIdx      = 0;
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
      highest = inHigh[i];
      lowest = inLow[i];
      sufHighest[optInTimePeriod - 1] = highest;
      sufLowest[optInTimePeriod - 1] = lowest;
      TA_UNROLL(4)
      while( i > blockStart )
      {
         i--;
         tmp = inHigh[i];
         if( tmp > highest )
         {
            highest = tmp;
         }
         tmp = inLow[i];
         if( tmp < lowest )
         {
            lowest = tmp;
         }
         sufHighest[i - blockStart] = highest;
         sufLowest[i - blockStart] = lowest;
      }

      highest = sufHighest[0];
      lowest = sufLowest[0];
      diff = (highest - lowest)/(-100.0);
      if( diff != 0.0 )
         outReal[outIdx++] = (highest-inClose[today])/diff;
      else
         outReal[outIdx++] = 0.0;
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
         highest = inHigh[blockNext];
         lowest = inLow[blockNext];
         preHighest[0] = highest;
         preLowest[0] = lowest;
         i = 1;
         TA_UNROLL(4)
         while( i < nAvail )
         {
            tmp = inHigh[blockNext + i];
            if( tmp > highest )
            {
               highest = tmp;
            }
            tmp = inLow[blockNext + i];
            if( tmp < lowest )
            {
               lowest = tmp;
            }
            preHighest[i] = highest;
            preLowest[i] = lowest;
            i++;
         }

         /* Combine and emit. The suffix half is the older one, so
          * preferring it on a tie keeps the earliest-wins rule. The
          * bar being emitted for offset m is today+m-1: 'today' was
          * advanced once above and is not touched inside this loop.
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
            diff = (highest - lowest)/(-100.0);
            if( diff != 0.0 )
               outReal[outIdx++] = (highest-inClose[today + m - 1])/diff;
            else
               outReal[outIdx++] = 0.0;
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
TA_RetCode willr_ALT1(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double lowest, highest, tmp, diff;
   int outIdx, nbInitialElementNeeded;
   int trailingIdx, lowestIdx, highestIdx;
   int today, i;

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

   /* Initialize 'diff', just to avoid warning. */
   diff = 0.0;

   /* Proceed with the calculation for the requested range.
    * Note that this algorithm allows the input and
    * output to be the same buffer.
    *
    * The highest high and lowest low of the window are cached with their
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
   outIdx      = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;

   lowestIdx   = highestIdx = -1;
   diff = highest = lowest  = 0.0;

   while( today <= endIdx )
   {
      /* Set the lowest low */
      tmp = inLow[today];
      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         lowest = inLow[lowestIdx];
         i = lowestIdx;
         TA_UNROLL(4)
         while( ++i<=today )
         {
            tmp = inLow[i];
            if( tmp < lowest )
            {
               lowestIdx = i;
               lowest = tmp;
            }
         }
         diff = (highest - lowest)/(-100.0);
      }
      else if( tmp <= lowest )
      {
         lowestIdx = today;
         lowest = tmp;
         diff = (highest - lowest)/(-100.0);
      }

      /* Set the highest high */
      tmp = inHigh[today];
      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inHigh[highestIdx];
         i = highestIdx;
         TA_UNROLL(4)
         while( ++i<=today )
         {
            tmp = inHigh[i];
            if( tmp > highest )
            {
               highestIdx = i;
               highest = tmp;
            }
         }
         diff = (highest - lowest)/(-100.0);
      }
      else if( tmp >= highest )
      {
         highestIdx = today;
         highest = tmp;
         diff = (highest - lowest)/(-100.0);
      }

      if( diff != 0.0 )
         outReal[outIdx++] = (highest-inClose[today])/diff;
      else
         outReal[outIdx++] = 0.0;

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
