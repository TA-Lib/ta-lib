/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091526 KL     First version (proposal-drafts issue #73).
 *  092226 MF,CC  Binary search above 128 values, one shift per bar (issue #432).
 */

int median_lookback(int optInTimePeriod)
{
   return optInTimePeriod-1;
}

TA_RetCode median(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double newValue, oldValue, result, lower, upper;
   int lookbackTotal, outIdx, i, j, pos, nbSorted, lowerIdx, upperIdx;
   int lo, hi, mid;

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

   /* The two central ordinals, zero-based; the same slot at odd n. */
   lowerIdx = (optInTimePeriod-1)/2;
   upperIdx = optInTimePeriod/2;

   nbSorted = 0;
   i = startIdx-lookbackTotal;
   while( i < startIdx )
   {
      newValue = inReal[i];
      if( lookbackTotal < 128 )
      {
         j = nbSorted;
         while( j > 0 && sorted[j-1] > newValue )
         {
            sorted[j] = sorted[j-1];
            j--;
         }
      }
      else
      {
         lo = 0;
         hi = nbSorted;
         while( lo < hi )
         {
            mid = (lo+hi)/2;
            if( sorted[mid] <= newValue )
               lo = mid+1;
            else
               hi = mid;
         }
         j = nbSorted;
         while( j > lo )
         {
            sorted[j] = sorted[j-1];
            j--;
         }
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
    *
    * Below 128 values a linear scan beats a binary search: one mispredicted
    * loop exit costs less than log2(n) unpredictable halvings.
    */
   outIdx = 0;
   do
   {
      newValue = inReal[i];

      /* pos counts the retained values <= newValue, so the full window is
       * sorted[0..pos-1], newValue, sorted[pos..].
       */
      if( lookbackTotal < 128 )
      {
         pos = 0;
         while( pos < lookbackTotal && sorted[pos] <= newValue )
            pos++;
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
      }

      if( lowerIdx < pos )
         lower = sorted[lowerIdx];
      else if( lowerIdx == pos )
         lower = newValue;
      else
         lower = sorted[lowerIdx-1];

      if( upperIdx < pos )
         upper = sorted[upperIdx];
      else if( upperIdx == pos )
         upper = newValue;
      else
         upper = sorted[upperIdx-1];

      /* At odd n both reads are one value; averaging it would overflow above
       * DBL_MAX/2.
       */
      if( lowerIdx == upperIdx )
         result = lower;
      else
         result = ( lower + upper ) / 2.0;

      outReal[outIdx] = result;
      outIdx++;

      ring[ring_Idx] = newValue;
      CIRCBUF_NEXT(ring);
      oldValue = ring[ring_Idx];

      /* j is the first retained value >= oldValue. Keep every run of equal
       * values in age order (newValue goes after its equals, as above): the
       * oldest of a run is then the departing value bit for bit, which is what
       * keeps -0.0 and 0.0 apart. Inserting before the equals instead changes
       * no value but flips the sign of some zero outputs.
       */
      if( lookbackTotal < 128 )
      {
         j = 0;
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
