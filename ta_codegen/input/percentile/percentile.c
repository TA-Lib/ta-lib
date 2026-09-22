/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090426 MF,CC  First version (issue #368).
 *  092226 MF,CC  O(1) read, binary search from 256 values, one shift per bar (issue #435).
 */

int percentile_lookback(int optInTimePeriod, double optInPercentile)
{
   (void)optInPercentile;
   return optInTimePeriod-1;
}

TA_RetCode percentile(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInPercentile,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double newValue, oldValue, result;
   int lookbackTotal, outIdx, i, j, pos, nbSorted, rank, lo, hi, mid;

   /* The window is carried twice: "ring" by age, "sorted" by value. */
   CIRCBUF_PROLOG(ring,double,30);
   CIRCBUF_PROLOG(sorted,double,30);

   lookbackTotal = (optInTimePeriod-1);

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   CIRCBUF_INIT( ring, double, optInTimePeriod );
   CIRCBUF_INIT( sorted, double, optInTimePeriod );

   /* Never read: set so two handles opened over the same bars hold the same
    * state.
    */
   sorted[lookbackTotal] = 0.0;

   /* Keep the multiply left of the divide. (P*n)/100 reproduces exact integer
    * arithmetic; P/100 is inexact in binary64 and lands the product just above
    * an integer, one order statistic too high, at exactly the round
    * percentages a caller types.
    */
   rank = (int)ceil(optInPercentile*(double)optInTimePeriod/100.0);
   if( rank < 1 )
      rank = 1;
   if( rank > optInTimePeriod )
      rank = optInTimePeriod;

   nbSorted = 0;
   i = startIdx-lookbackTotal;
   while( i < startIdx )
   {
      newValue = inReal[i];
      j = nbSorted;
      while( j > 0 && sorted[j-1] > newValue )
      {
         sorted[j] = sorted[j-1];
         j--;
      }
      sorted[j] = newValue;
      nbSorted++;
      ring[ring_Idx] = newValue;
      i++;
      CIRCBUF_NEXT(ring);
   }

   /* Both scratch buffers hold copies and inReal is never read below i, so
    * inReal and outReal may be the same buffer.
    *
    * Every buffer store sits BELOW the output store: deriving the whole answer
    * read-only above it is what lets the streaming peek frame drop the state
    * update.
    */
   outIdx = 0;
   do
   {
      newValue = inReal[i];

      /* The full window is the retained values with newValue inserted after
       * its equals, so its rank-th value is newValue clamped to
       * [sorted[rank-2], sorted[rank-1]]: a tie with the upper bound yields
       * sorted[rank-1], a tie with the lower bound yields newValue. Spelled as
       * selects, not branches: which side wins is a coin flip.
       */
      result = newValue;
      if( rank <= lookbackTotal )
         result = ( result < sorted[rank-1] ) ? result : sorted[rank-1];
      if( rank > 1 )
         result = ( result < sorted[rank-2] ) ? sorted[rank-2] : result;

      outReal[outIdx] = result;
      outIdx++;

      /* pos counts the retained values <= newValue and j is the first retained
       * value >= oldValue. Below 256 values a linear scan beats a binary
       * search: one mispredicted loop exit costs less than log2(n)
       * unpredictable halvings. From 64 values the scan steps 4 at a time
       * first, which is what pays for that step's own mispredicted exit.
       *
       * Keep every run of equal values in age order (newValue goes after its
       * equals): the oldest of a run is then the departing value bit for bit,
       * which is what keeps -0.0 and 0.0 apart. Inserting before the equals
       * instead changes no value but flips the sign of some zero outputs.
       */
      ring[ring_Idx] = newValue;
      CIRCBUF_NEXT(ring);
      oldValue = ring[ring_Idx];

      if( lookbackTotal < 256 )
      {
         pos = 0;
         if( lookbackTotal >= 64 )
            while( pos+4 <= lookbackTotal && sorted[pos+3] <= newValue )
            pos += 4;
         while( pos < lookbackTotal && sorted[pos] <= newValue )
            pos++;
         j = 0;
         if( lookbackTotal >= 64 )
            while( j+4 <= lookbackTotal && sorted[j+3] < oldValue )
            j += 4;
         while( j < lookbackTotal && sorted[j] < oldValue )
            j++;
      }
      else
      {
         lo = 0;
         hi = lookbackTotal;
         while( lo < hi )
         {
            mid = (lo+hi)/2;
            if( sorted[mid] <= newValue )
               lo = mid+1;
            else
               hi = mid;
         }
         pos = lo;
         lo = 0;
         hi = lookbackTotal;
         while( lo < hi )
         {
            mid = (lo+hi)/2;
            if( sorted[mid] < oldValue )
               lo = mid+1;
            else
               hi = mid;
         }
         j = lo;
      }

      /* Evict oldValue and place newValue with one shift of the slots between
       * them.
       */
      if( j < pos )
      {
         while( j < pos-1 )
         {
            sorted[j] = sorted[j+1];
            j++;
         }
         sorted[pos-1] = newValue;
      }
      else
      {
         while( j > pos )
         {
            sorted[j] = sorted[j-1];
            j--;
         }
         sorted[pos] = newValue;
      }

      i++;
   } while( i <= endIdx );

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   CIRCBUF_DESTROY(ring);
   CIRCBUF_DESTROY(sorted);

   return TA_SUCCESS;
}
