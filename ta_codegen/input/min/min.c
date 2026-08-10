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
   CIRCBUF_PROLOG(sufLowest,double,30);
   CIRCBUF_PROLOG(preLowest,double,30);
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
    *
    * Two equivalent algorithms. The batch tier takes the first arm for
    * every period in range (the threshold is the declared maximum), so
    * the second arm exists to be the streaming tier's transition; see
    * issue #147.
    *
    * - Batch: Van Herk / Gil-Werman block scan, block-batched form. The
    *   p outputs belonging to one block boundary are produced together:
    *   one backward pass builds the older block's suffix extrema, one
    *   forward pass builds the newer block's prefix extrema, and a third
    *   pass combines them. All three are straight-line loops with no
    *   data-dependent branching, which is what lets a compiler vectorize
    *   them, and the work is 3 comparisons per bar regardless of period.
    *   Both scratch arrays hold COPIES, so input and output may alias.
    *
    * - Streaming: cache the lowest value with its index; rescan only
    *   when the cached extremum leaves the window. O(1) per bar while the
    *   extremum sits away from the trailing edge, but not amortized O(1):
    *   an extremum on the oldest in-window bar drops out on the very next
    *   bar, so the rescan repeats and the cost stays O(period) per bar for
    *   as long as that persists.
    */
   outIdx = 0;
   today       = startIdx;
   trailingIdx = startIdx-nbInitialElementNeeded;

   if( optInTimePeriod <= 100000 )
   {
      int blockStart, nAvail, m;

      CIRCBUF_INIT( sufLowest, double, optInTimePeriod );
      CIRCBUF_INIT( preLowest, double, optInTimePeriod );

      blockStart = trailingIdx;

      while( today <= endIdx )
      {
         /* Suffix extrema of the block [blockStart, blockStart+p-1], which
          * is fully available here: today == blockStart+p-1 <= endIdx.
          * Scanning backward while keeping the incumbent on a tie
          * leaves the later element holding a tie, which is what lets this
          * compile to a single min instruction.
          */
         i = blockStart + optInTimePeriod - 1;
         lowest = inReal[i];
         sufLowest[optInTimePeriod - 1] = lowest;
         TA_UNROLL(4)
         while( i > blockStart )
         {
            i--;
            tmp = inReal[i];
            if( tmp < lowest )
            {
               lowest = tmp;
            }
            sufLowest[i - blockStart] = lowest;
         }

         lowest = sufLowest[0];
         outReal[outIdx++] = lowest;
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
            lowest = inReal[blockStart + optInTimePeriod];
            preLowest[0] = lowest;
            i = 1;
            TA_UNROLL(4)
            while( i < nAvail )
            {
               tmp = inReal[blockStart + optInTimePeriod + i];
               if( tmp < lowest )
               {
                  lowest = tmp;
               }
               preLowest[i] = lowest;
               i++;
            }

            /* Combine. The suffix half is the older one, so preferring it
             * on a tie keeps the earliest-wins rule.
             */
            m = 1;
            while( m <= nAvail )
            {
               lowest = sufLowest[m];
               if( preLowest[m - 1] < lowest )
               {
                  lowest = preLowest[m - 1];
               }
               outReal[outIdx++] = lowest;
               m++;
            }
            trailingIdx = trailingIdx + nAvail;
            today = today + nAvail;
            blockStart = blockStart + optInTimePeriod;
         }
      }

      CIRCBUF_DESTROY(sufLowest);
      CIRCBUF_DESTROY(preLowest);
   }
   else
   {
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
            TA_UNROLL(4)
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
   }

   /* Keep the outBegIdx relative to the
    * caller input before returning.
    */
   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
