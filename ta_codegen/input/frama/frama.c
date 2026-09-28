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
 *  092826 MF,CC  First version (issue #464).
 */

int frama_lookback(int optInTimePeriod)
{
   /* The range check cannot demand an even period; without this the lookback
    * answers a usable number for a call that cannot run. */
   if( (optInTimePeriod%2) != 0 )
      return -1;

   return optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_FRAMA);
}

TA_RetCode frama(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   /* -4.6/ln(2): alpha = exp(-4.6*(D-1)) with D-1 = log2((R1+R2)/R). */
   const double expScale = -4.6/0.6931471805599453;

   typedef struct { double sufHigh; double sufLow; double oldHigh; double oldLow; } FramaSlot;
   CIRCBUF_PROLOG_CLASS( slot, FramaSlot, 32 ); /* Id, Type, Static Size */

   double tmpHigh, tmpLow, preHigh, preLow;
   double hi1, lo1, hi2, lo2, r1, r2, r, price, alpha, prevFRAMA;
   int i, today, outIdx, lookbackTotal, half, lastSlot, seedIdx;

   *outBegIdx = 0;
   *outNBElement = 0;

   if( (optInTimePeriod%2) != 0 )
      return TA_BAD_PARAM;

   lookbackTotal = optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_FRAMA);

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   half = optInTimePeriod/2;
   lastSlot = half-1;

   /* van Herk / Gil-Werman over blocks of `half` bars. Slot j holds the current
    * block's bar j (or, past the running position, the previous block's suffix
    * extremes), and in oldHigh/oldLow the newer half's extremes from `half` bars
    * ago, which are the older half's extremes now: R2[t] = R1[t-half].
    *
    * Every slot store must stay below the output store: that is what keeps the
    * stream's Peek read-only and O(1). The slots hold copies, so outReal may
    * alias an input.
    */
   CIRCBUF_INIT_CLASS( slot, FramaSlot, half );

   today = startIdx-lookbackTotal+1;
   seedIdx = startIdx-TA_GetUnstablePeriod(TA_FUNC_UNST_FRAMA)-1;

   /* The first block's suffix reads must see a bar inside the window. */
   i = 0;
   while( i < half )
   {
      slot[i].sufHigh = inHigh[today];
      slot[i].sufLow  = inLow[today];
      slot[i].oldHigh = inHigh[today];
      slot[i].oldLow  = inLow[today];
      i++;
   }

   preHigh = 0.0;
   preLow  = 0.0;

   /* Through seedIdx only the blocks are built: the seed discards every value
    * before it, so no alpha is computed there. */
   while( today <= seedIdx )
   {
      tmpHigh = inHigh[today];
      tmpLow  = inLow[today];

      if( slot_Idx == 0 )
      {
         preHigh = tmpHigh;
         preLow  = tmpLow;
      }
      else
      {
         if( tmpHigh > preHigh )
            preHigh = tmpHigh;
         if( tmpLow < preLow )
            preLow = tmpLow;
      }

      if( slot_Idx == lastSlot )
      {
         hi1 = preHigh;
         lo1 = preLow;
      }
      else
      {
         hi1 = slot[slot_Idx+1].sufHigh;
         if( preHigh > hi1 )
            hi1 = preHigh;
         lo1 = slot[slot_Idx+1].sufLow;
         if( preLow < lo1 )
            lo1 = preLow;
      }

      slot[slot_Idx].oldHigh = hi1;
      slot[slot_Idx].oldLow  = lo1;
      slot[slot_Idx].sufHigh = tmpHigh;
      slot[slot_Idx].sufLow  = tmpLow;
      if( slot_Idx == lastSlot )
      {
         i = lastSlot;
         while( i > 0 )
         {
            i--;
            if( slot[i+1].sufHigh > slot[i].sufHigh )
               slot[i].sufHigh = slot[i+1].sufHigh;
            if( slot[i+1].sufLow < slot[i].sufLow )
               slot[i].sufLow = slot[i+1].sufLow;
         }
      }
      CIRCBUF_NEXT(slot);
      today++;
   }
   prevFRAMA = (inHigh[seedIdx]+inLow[seedIdx])/2.0;

   outIdx = 0;
   while( today <= endIdx )
   {
      tmpHigh = inHigh[today];
      tmpLow  = inLow[today];
      price   = (tmpHigh+tmpLow)/2.0;

      if( slot_Idx == 0 )
      {
         preHigh = tmpHigh;
         preLow  = tmpLow;
      }
      else
      {
         if( tmpHigh > preHigh )
            preHigh = tmpHigh;
         if( tmpLow < preLow )
            preLow = tmpLow;
      }

      if( slot_Idx == lastSlot )
      {
         hi1 = preHigh;
         lo1 = preLow;
      }
      else
      {
         hi1 = slot[slot_Idx+1].sufHigh;
         if( preHigh > hi1 )
            hi1 = preHigh;
         lo1 = slot[slot_Idx+1].sufLow;
         if( preLow < lo1 )
            lo1 = preLow;
      }

      hi2 = slot[slot_Idx].oldHigh;
      lo2 = slot[slot_Idx].oldLow;
      r1 = hi1-lo1;
      r2 = hi2-lo2;
      r  = (hi1 > hi2 ? hi1 : hi2) - (lo1 < lo2 ? lo1 : lo2);

      /* R >= max(R1,R2) makes R1+R2 <= R exactly the alpha >= 1 clamp, so a
       * clamped or flat bar returns the price with no transcendental. Keep the
       * recursion as alpha*P + (1-alpha)*prev: a computed alpha of 1.0 then
       * returns the price exactly too. */
      if( r1 > 0.0 && r2 > 0.0 && (r1+r2) > r )
      {
         alpha = exp(expScale*log((r1+r2)/r));
         prevFRAMA = alpha*price + (1.0-alpha)*prevFRAMA;
      }
      else
         prevFRAMA = price;

      if( today >= startIdx )
         outReal[outIdx++] = prevFRAMA;

      slot[slot_Idx].oldHigh = hi1;
      slot[slot_Idx].oldLow  = lo1;
      slot[slot_Idx].sufHigh = tmpHigh;
      slot[slot_Idx].sufLow  = tmpLow;
      if( slot_Idx == lastSlot )
      {
         i = lastSlot;
         while( i > 0 )
         {
            i--;
            if( slot[i+1].sufHigh > slot[i].sufHigh )
               slot[i].sufHigh = slot[i+1].sufHigh;
            if( slot[i+1].sufLow < slot[i].sufLow )
               slot[i].sufLow = slot[i+1].sufLow;
         }
      }
      CIRCBUF_NEXT(slot);
      today++;
   }

   CIRCBUF_DESTROY(slot);

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
