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
   int lookbackTotal, outIdx, i, j, pos, nbSorted, rank;

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
    * Every buffer store sits BELOW the output store on purpose: deriving the
    * whole answer read-only above it is what lets the streaming peek frame drop
    * the state update rather than shadow a shift loop, which it cannot do.
    */
   outIdx = 0;
   do
   {
      newValue = inReal[i];

      pos = 0;
      while( pos < lookbackTotal && sorted[pos] <= newValue )
         pos++;

      if( rank-1 < pos )
         result = sorted[rank-1];
      else if( rank-1 == pos )
         result = newValue;
      else
         result = sorted[rank-2];

      outReal[outIdx] = result;
      outIdx++;

      /* Shifting only the strictly greater entries leaves equal values in
       * insertion order, which is age order -- that is what lets the delete
       * below evict the oldest of a run by value alone, with no slot array.
       */
      j = lookbackTotal;
      while( j > pos )
      {
         sorted[j] = sorted[j-1];
         j--;
      }
      sorted[pos] = newValue;

      ring[ring_Idx] = newValue;
      CIRCBUF_NEXT(ring);

      oldValue = ring[ring_Idx];
      j = 0;
      while( j < lookbackTotal && sorted[j] < oldValue )
         j++;
      while( j < lookbackTotal )
      {
         sorted[j] = sorted[j+1];
         j++;
      }

      i++;
   } while( i <= endIdx );

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   CIRCBUF_DESTROY(ring);
   CIRCBUF_DESTROY(sorted);

   return TA_SUCCESS;
}
