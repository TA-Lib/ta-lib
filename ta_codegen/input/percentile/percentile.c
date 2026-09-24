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
 *  092326 MF,CC  Branchless update kernels, merge-sorted first window (issue #435).
 *  092426 MF,CC  Rust stream tier takes the branchless kernels too (issue #439).
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
   double newValue, oldValue, result, loV, hiV, last, pendNew, pendOld, held;
   double cur, nxt, nx2, ins, in2, res, prv;
   int lookbackTotal, outIdx, i, j, k, e, nbSorted, rank, hiRank, loRank;
   int s, t, w, lo, mid, hi, a, b;
   int tail, lim, run, dnStep, same, dPrev, trend, jh, jl;
   int layout, head, pendPos, pendDel, runLen, saving, gain, pp, hiSlot, loSlot;
   int pos, del, dp, dq, len, half, o2, o3, t1, t2, t3, u1, u2, u3;
   int freeSlot, maxSlot, nextSlot, bp, bq, q1, q2, q3, p1, p2, p3, sP, sQ, slot, cnt, dn, seg, room;

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

   /* Never read as a value, but from 2048 values it is the first free slot and
    * travels with the ring: set so two handles over the same bars hold the same
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
   hiRank = rank-1;
   loRank = ( hiRank > 0 ) ? hiRank-1 : 0;

   /* The retained values are sorted ascending with every run of equal values
    * in age order: the departing value is then the first of its run bit for
    * bit, and a new value goes after its equals. Only the sign of a zero can
    * observe that order, and it does.
    */
   i = startIdx-lookbackTotal;
   if( lookbackTotal < 100 )
   {
      nbSorted = 0;
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
   }
   else
   {
      /* Bottom-up merge sort, stable, with ring as the other half until it
       * is filled by age below.
       */
      sorted[0] = inReal[i];
      j = 1;
      while( j < lookbackTotal && inReal[i+j-1] <= inReal[i+j] )
      {
         sorted[j] = inReal[i+j];
         j++;
      }
      if( j < lookbackTotal )
      {
         s = 0;
         while( s < lookbackTotal )
         {
            e = s + 16;
            if( e > lookbackTotal )
               e = lookbackTotal;
            t = s;
            while( t < e )
            {
               newValue = inReal[i+t];
               j = t;
               while( j > s && sorted[j-1] > newValue )
               {
                  sorted[j] = sorted[j-1];
                  j--;
               }
               sorted[j] = newValue;
               t++;
            }
            s = e;
         }
         w = 16;
         while( w < lookbackTotal )
         {
            lo = 0;
            while( lo < lookbackTotal )
            {
               mid = lo + w;
               if( mid > lookbackTotal )
                  mid = lookbackTotal;
               hi = mid + w;
               if( hi > lookbackTotal )
                  hi = lookbackTotal;
               a = lo;
               b = mid;
               t = lo;
               if( mid < hi && sorted[mid-1] > sorted[mid] )
               {
                  while( a < mid && b < hi )
                  {
                     if( sorted[b] < sorted[a] )
                     {
                        ring[t] = sorted[b];
                        b++;
                     }
                     else
                     {
                        ring[t] = sorted[a];
                        a++;
                     }
                     t++;
                  }
               }
               while( a < mid )
               {
                  ring[t] = sorted[a];
                  a++;
                  t++;
               }
               while( b < hi )
               {
                  ring[t] = sorted[b];
                  b++;
                  t++;
               }
               lo = hi;
            }
            w += w;
            if( w < lookbackTotal )
            {
               lo = 0;
               while( lo < lookbackTotal )
               {
                  mid = lo + w;
                  if( mid > lookbackTotal )
                     mid = lookbackTotal;
                  hi = mid + w;
                  if( hi > lookbackTotal )
                     hi = lookbackTotal;
                  a = lo;
                  b = mid;
                  t = lo;
                  if( mid < hi && ring[mid-1] > ring[mid] )
                  {
                     while( a < mid && b < hi )
                     {
                        if( ring[b] < ring[a] )
                        {
                           sorted[t] = ring[b];
                           b++;
                        }
                        else
                        {
                           sorted[t] = ring[a];
                           a++;
                        }
                        t++;
                     }
                  }
                  while( a < mid )
                  {
                     sorted[t] = ring[a];
                     a++;
                     t++;
                  }
                  while( b < hi )
                  {
                     sorted[t] = ring[b];
                     b++;
                     t++;
                  }
                  lo = hi;
               }
               w += w;
            }
            else
            {
               t = 0;
               while( t < lookbackTotal )
               {
                  sorted[t] = ring[t];
                  t++;
               }
            }
         }
      }
      t = 0;
      while( t < lookbackTotal )
      {
         ring[t] = inReal[i+t];
         t++;
      }
      ring_Idx = lookbackTotal;
      i = startIdx;
   }

   /* Below 32 values: every bar rewrites the whole window with no
    * data-dependent branch, or, after a long enough run in one direction,
    * reads the two ranks it needs straight off the ring.
    */
   tail = lookbackTotal - lookbackTotal % 2;
   lim = ( lookbackTotal > 13 ) ? lookbackTotal-2 : 11;
   run = -1;
   dPrev = 0;
   trend = 0;
   last = inReal[startIdx-1];

   /* From m = 32 retained values, layout 0 keeps slots 0..m-1 ONE BAR BEHIND:
    * the previous bar's update (remove rank pendDel, insert pendNew at pendPos
    * counted before the removal) is still pending and the spare slot m holds
    * pendNew. A bar searches that array and corrects its counts by two
    * compares, so its search does not wait on the previous bar's shift.
    * Layout 1 is a ring of all m+1 slots, rank r at slot head+r and the free
    * slot just before head; layout 2 is layout 1 with the pending update still
    * to apply. The ring pays for itself on a trending window, where a new
    * extreme arrives as the opposite one departs, and from 2048 values, where
    * moving the shorter way round halves the shift.
    */
   layout = ( lookbackTotal >= 2048 ) ? 1 : 0;
   head = 0;
   runLen = 0;
   saving = 0;
   pendNew = sorted[0];
   pendOld = pendNew;
   pendDel = 0;
   pendPos = 1;

   hiV = ( rank <= lookbackTotal ) ? sorted[hiRank] : 0.0;
   loV = ( rank > 1 ) ? sorted[loRank] : 0.0;

   /* Both scratch buffers hold copies and inReal is never read below i, so
    * inReal and outReal may be the same buffer.
    *
    * Every store to state sits BELOW the output store: the streaming peek
    * frame is this loop cut there.
    */
   outIdx = 0;
   do
   {
      newValue = inReal[i];

      /* The full window is the retained values with newValue inserted after
       * its equals, so its rank-th value is newValue clamped to the retained
       * values of rank rank-1 (loV) and rank (hiV).
       */
      result = newValue;
      if( rank <= lookbackTotal )
         result = ( result < hiV ) ? result : hiV;
      if( rank > 1 )
         result = ( result < loV ) ? loV : result;

      outReal[outIdx] = result;
      outIdx++;

      ring[ring_Idx] = newValue;
      CIRCBUF_NEXT(ring);
      oldValue = ring[ring_Idx];

      if( lookbackTotal < 32 )
      {
         if( lookbackTotal == 1 )
         {
            sorted[0] = newValue;
            hiV = newValue;
            loV = newValue;
         }
         else
         {
            /* 0/1 arithmetic, not conditional assignments: gcc threads those
             * into a branch on newValue < last, a coin flip on random data.
             * run starts at -1 so the first step cannot count twice.
             */
            dnStep = ( newValue < last ) ? 1 : 0;
            same = ( dnStep == dPrev ) ? 1 : 0;
            run = (run+1)*same;
            dPrev = dnStep;
            last = newValue;

            if( run >= lim )
            {
               /* The ring is then the sorted order, from its oldest slot on
                * a rising run and from its newest on a falling one, which
                * must be strict: read newest first, equal values would come
                * out in reverse age order. sorted is left stale until the
                * run breaks.
                */
               run = lim;
               if( dnStep == 0 )
               {
                  trend = 1;
                  jh = ring_Idx + rank;
                  if( jh >= optInTimePeriod )
                     jh -= optInTimePeriod;
                  jl = ( jh == 0 ) ? lookbackTotal : jh-1;
               }
               else
               {
                  trend = 2;
                  jh = ring_Idx + (optInTimePeriod - rank);
                  if( jh >= optInTimePeriod )
                     jh -= optInTimePeriod;
                  jl = ( jh == lookbackTotal ) ? 0 : jh+1;
               }
               if( rank <= lookbackTotal )
                  hiV = ring[jh];
               if( rank > 1 )
                  loV = ring[jl];
            }
            else
            {
               if( trend != 0 )
               {
                  if( trend == 1 )
                  {
                     j = ring_Idx;
                     k = 0;
                     while( k < lookbackTotal )
                     {
                        sorted[k] = ring[j];
                        j = ( j == lookbackTotal ) ? 0 : j+1;
                        k++;
                     }
                  }
                  else
                  {
                     j = ring_Idx + lookbackTotal - 1;
                     if( j >= optInTimePeriod )
                        j -= optInTimePeriod;
                     k = 0;
                     while( k < lookbackTotal )
                     {
                        sorted[k] = ring[j];
                        j = ( j == 0 ) ? lookbackTotal : j-1;
                        k++;
                     }
                  }
                  trend = 0;
               }

               /* With D the retained values less oldValue, slot k becomes
                * max(D[k-1], min(newValue, D[k])). The spare slot stands in
                * for D past the end.
                */
               sorted[lookbackTotal] = newValue;
               cur = sorted[0];
               prv = ( newValue < cur ) ? newValue : cur;
               k = 0;
               do
               {
                  nxt = sorted[k+1];
                  nx2 = sorted[k+2];
                  ins = ( cur < oldValue ) ? cur : nxt;
                  in2 = ( nxt < oldValue ) ? nxt : nx2;
                  res = ( newValue < ins ) ? newValue : ins;
                  sorted[k] = ( prv > res ) ? prv : res;
                  res = ( newValue < in2 ) ? newValue : in2;
                  sorted[k+1] = ( ins > res ) ? ins : res;
                  prv = in2;
                  cur = nx2;
                  k += 2;
               } while( k < tail );
               sorted[tail] = ( prv > newValue ) ? prv : newValue;

               if( rank <= lookbackTotal )
                  hiV = sorted[hiRank];
               if( rank > 1 )
                  loV = sorted[loRank];
            }
         }
      }
      else
      {
         if( layout != 0 )
         {
            if( layout == 2 )
            {
               if( pendDel < pendPos )
               {
                  for( k = pendDel; k < pendPos-1; k++ )
                     sorted[k] = sorted[k+1];
                  sorted[pendPos-1] = pendNew;
               }
               else
               {
                  for( k = pendDel; k > pendPos; k-- )
                     sorted[k] = sorted[k-1];
                  sorted[pendPos] = pendNew;
               }
               layout = 1;
            }

            freeSlot = ( head == 0 ) ? lookbackTotal : head-1;
            maxSlot = ( freeSlot == 0 ) ? lookbackTotal : freeSlot-1;
            nextSlot = ( maxSlot == 0 ) ? lookbackTotal : maxSlot-1;

            /* A new value tied with the minimum belongs after it, so only a
             * strictly smaller one may take the front.
             */
            if( newValue >= sorted[maxSlot] && oldValue <= sorted[head] )
            {
               sorted[freeSlot] = newValue;
               head = ( head == lookbackTotal ) ? 0 : head+1;
            }
            else if( newValue < sorted[head] && oldValue > sorted[nextSlot] )
            {
               sorted[freeSlot] = newValue;
               head = freeSlot;
            }
            else if( saving > 0 || lookbackTotal >= 2048 )
            {
               /* pos = retained values <= newValue, del = retained values <
                * oldValue, both searched from the free slot as rank -1.
                */
               bp = freeSlot;
               bq = bp;
               len = optInTimePeriod;
               while( len > 2 )
               {
                  half = (len+3)/4;
                  o3 = len - half;
                  o2 = half + half;
                  q1 = ( bp < optInTimePeriod - half ) ? bp + half : bp + half - optInTimePeriod;
                  q2 = ( bp < optInTimePeriod - o2 ) ? bp + o2 : bp + o2 - optInTimePeriod;
                  q3 = ( bp < optInTimePeriod - o3 ) ? bp + o3 : bp + o3 - optInTimePeriod;
                  p1 = ( bq < optInTimePeriod - half ) ? bq + half : bq + half - optInTimePeriod;
                  p2 = ( bq < optInTimePeriod - o2 ) ? bq + o2 : bq + o2 - optInTimePeriod;
                  p3 = ( bq < optInTimePeriod - o3 ) ? bq + o3 : bq + o3 - optInTimePeriod;
                  bp = ( sorted[q1] <= newValue ) ? q1 : bp;
                  bp = ( sorted[q2] <= newValue ) ? q2 : bp;
                  bp = ( sorted[q3] <= newValue ) ? q3 : bp;
                  bq = ( sorted[p1] < oldValue ) ? p1 : bq;
                  bq = ( sorted[p2] < oldValue ) ? p2 : bq;
                  bq = ( sorted[p3] < oldValue ) ? p3 : bq;
                  len = half;
               }
               if( len > 1 )
               {
                  sP = ( bp < lookbackTotal ) ? bp + 1 : 0;
                  sQ = ( bq < lookbackTotal ) ? bq + 1 : 0;
                  bp = ( sorted[sP] <= newValue ) ? sP : bp;
                  bq = ( sorted[sQ] < oldValue ) ? sQ : bq;
               }
               pos = ( bp >= freeSlot ) ? bp - freeSlot : bp + optInTimePeriod - freeSlot;
               del = ( bq >= freeSlot ) ? bq - freeSlot : bq + optInTimePeriod - freeSlot;
               gain = ( del < pos ) ? pos-1-del : del-pos;
               seg = lookbackTotal + (lookbackTotal/4)*3;
               saving = ( saving + gain + gain > seg ) ? saving + gain + gain - seg : 0;
               saving = ( saving < lookbackTotal*16 ) ? saving : lookbackTotal*16;

               /* The hole left by oldValue travels to newValue's place the
                * shorter way round; going through the free slot moves head
                * by one.
                */
               slot = head + del;
               if( slot >= optInTimePeriod )
                  slot -= optInTimePeriod;
               if( del < pos )
               {
                  cnt = pos-1-del;
                  dn = 1;
               }
               else
               {
                  cnt = del-pos;
                  dn = 0;
               }
               if( cnt+cnt > lookbackTotal )
               {
                  cnt = lookbackTotal-cnt;
                  dn = 1-dn;
                  if( dn == 1 )
                     head = freeSlot;
                  else
                     head = ( head == lookbackTotal ) ? 0 : head+1;
               }
               if( dn == 1 )
               {
                  room = lookbackTotal - slot;
                  if( cnt <= room )
                  {
                     e = slot + cnt;
                     for( k = slot; k < e; k++ )
                        sorted[k] = sorted[k+1];
                     slot = e;
                  }
                  else
                  {
                     for( k = slot; k < lookbackTotal; k++ )
                        sorted[k] = sorted[k+1];
                     sorted[lookbackTotal] = sorted[0];
                     e = cnt-room-1;
                     for( k = 0; k < e; k++ )
                        sorted[k] = sorted[k+1];
                     slot = e;
                  }
               }
               else
               {
                  if( cnt <= slot )
                  {
                     e = slot - cnt;
                     for( k = slot; k > e; k-- )
                        sorted[k] = sorted[k-1];
                     slot = e;
                  }
                  else
                  {
                     for( k = slot; k > 0; k-- )
                        sorted[k] = sorted[k-1];
                     sorted[0] = sorted[lookbackTotal];
                     e = lookbackTotal-(cnt-slot-1);
                     for( k = lookbackTotal; k > e; k-- )
                        sorted[k] = sorted[k-1];
                     slot = e;
                  }
               }
               sorted[slot] = newValue;
            }
            else
            {
               /* Back to layout 0: rotate head to slot 0 by three
                * reversals, then carry an empty pending update.
                */
               if( head != 0 )
               {
                  k = 0;
                  e = head-1;
                  while( k < e )
                  {
                     held = sorted[k];
                     sorted[k] = sorted[e];
                     sorted[e] = held;
                     k++;
                     e--;
                  }
                  k = head;
                  e = lookbackTotal;
                  while( k < e )
                  {
                     held = sorted[k];
                     sorted[k] = sorted[e];
                     sorted[e] = held;
                     k++;
                     e--;
                  }
                  k = 0;
                  e = lookbackTotal;
                  while( k < e )
                  {
                     held = sorted[k];
                     sorted[k] = sorted[e];
                     sorted[e] = held;
                     k++;
                     e--;
                  }
                  head = 0;
               }
               pendNew = sorted[0];
               pendOld = pendNew;
               pendDel = 0;
               pendPos = 1;
               sorted[lookbackTotal] = pendNew;
               layout = 0;
               runLen = 0;
            }
         }

         if( layout == 0 )
         {
            /* Branchless four-way search of the lagging array, no wrap. */
            pos = 0;
            del = 0;
            len = optInTimePeriod;
            while( len > 2 )
            {
               half = (len+3)/4;
               o3 = len - half;
               o2 = half + half;
               t1 = pos + half;
               t2 = pos + o2;
               t3 = pos + o3;
               u1 = del + half;
               u2 = del + o2;
               u3 = del + o3;
               pos = ( sorted[t1-1] <= newValue ) ? t1 : pos;
               pos = ( sorted[t2-1] <= newValue ) ? t2 : pos;
               pos = ( sorted[t3-1] <= newValue ) ? t3 : pos;
               del = ( sorted[u1-1] < oldValue ) ? u1 : del;
               del = ( sorted[u2-1] < oldValue ) ? u2 : del;
               del = ( sorted[u3-1] < oldValue ) ? u3 : del;
               len = half;
            }
            /* Increments, not selects: gcc turns the select into a jump. */
            if( len > 1 )
            {
               pos += ( sorted[pos] <= newValue ) ? 1 : 0;
               del += ( sorted[del] < oldValue ) ? 1 : 0;
            }
            pos += ( pendNew <= newValue ) ? 1 : 0;
            del += ( pendNew < oldValue ) ? 1 : 0;
            dp = ( pendOld <= newValue ) ? 1 : 0;
            dq = ( pendOld < oldValue ) ? 1 : 0;

            /* Clamped although exact for finite input: a NaN makes the counts
             * disagree with the corrections, and they index the shift below.
             */
            pos = ( pos > dp ) ? pos-dp : 0;
            del = ( del > dq ) ? del-dq : 0;
            pos = ( pos < lookbackTotal ) ? pos : lookbackTotal;
            del = ( del < lookbackTotal ) ? del : lookbackTotal;

            if( pendDel < pendPos )
            {
               for( k = pendDel; k < pendPos-1; k++ )
                  sorted[k] = sorted[k+1];
               sorted[pendPos-1] = pendNew;
            }
            else
            {
               for( k = pendDel; k > pendPos; k-- )
                  sorted[k] = sorted[k-1];
               sorted[pendPos] = pendNew;
            }
            sorted[lookbackTotal] = newValue;
            pendPos = pos;
            pendDel = del;
            pendNew = newValue;
            pendOld = oldValue;

            /* Leave for the ring after four bars that each add a new extreme
             * as the opposite one departs, or, from 300 values, while the
             * shift the ring would save keeps exceeding 3/4 of the window.
             */
            runLen = ( pos == del + lookbackTotal || del == pos + lookbackTotal - 1 ) ? runLen+1 : 0;
            if( lookbackTotal >= 300 )
            {
               gain = ( del < pos ) ? pos-1-del : del-pos;
               seg = lookbackTotal + (lookbackTotal/4)*3;
               saving = ( saving + gain + gain > seg ) ? saving + gain + gain - seg : 0;
            }
            if( runLen >= 4 || saving > lookbackTotal*8 )
               layout = 2;
         }

         if( layout != 1 )
         {
            pp = ( pendDel < pendPos ) ? pendPos-1 : pendPos;
            hiSlot = ( hiRank > pp ) ? hiRank-1 : hiRank;
            hiSlot += ( hiSlot >= pendDel ) ? 1 : 0;
            hiSlot = ( hiRank == pp ) ? lookbackTotal : hiSlot;
            loSlot = ( loRank > pp ) ? loRank-1 : loRank;
            loSlot += ( loSlot >= pendDel ) ? 1 : 0;
            loSlot = ( loRank == pp ) ? lookbackTotal : loSlot;
         }
         else
         {
            hiSlot = head + hiRank;
            if( hiSlot >= optInTimePeriod )
               hiSlot -= optInTimePeriod;
            loSlot = ( hiSlot == 0 ) ? lookbackTotal : hiSlot-1;
         }
         if( rank <= lookbackTotal )
            hiV = sorted[hiSlot];
         if( rank > 1 )
            loV = sorted[loSlot];
      }

      i++;
   } while( i <= endIdx );

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   CIRCBUF_DESTROY(ring);
   CIRCBUF_DESTROY(sorted);

   return TA_SUCCESS;
}

/* Same outputs with a search per value and one shift per bar. The JITs turn the
 * selects above into branches, which mispredict on every bar. Keep this body
 * small: C# compiles the same loop slower inside a larger method.
 */
/* PRAGMA TA_ALT={ALL_API,JAVA} */
/* PRAGMA TA_ALT={ALL_API,CSHARP} */
TA_RetCode percentile_ALT1(int startIdx, int endIdx,
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
