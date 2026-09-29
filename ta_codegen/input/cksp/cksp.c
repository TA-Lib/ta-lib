/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  093026 KL,CC  Creation (#477).
 */

int cksp_lookback(int optInTimePeriod, double optInMultiplier, int optInStopPeriod)
{
   (void)optInMultiplier;

   /* Two stages. The first needs the Average True Range at its bar and the
    * extreme of the p bars ending there; atr_lookback(p) is p + unst, which is
    * never below max_lookback(p) = p - 1, so it covers both. The second adds
    * the q - 1 earlier first-stage bars its own window reads.
    *
    * The ATR term is written as the callee's lookback and never restated, which
    * is what makes CKSP inherit TA_FUNC_UNST_ATR rather than own an unstable
    * period of its own (supertrend.c:16-25).
    */
   return atr_lookback( optInTimePeriod ) + (optInStopPeriod - 1);
}

TA_RetCode cksp(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   double optInMultiplier,
   int optInStopPeriod,
   int *outBegIdx, int *outNBElement,
   double outHighStop[],
   double outLowStop[])
{
   int i, jh, jl, kh, kl, today, outIdx, lookbackTotal, stageOneIdx;
   double prevATR, periodTotal, wAlpha, wBeta;
   double val2, val3, greatest, tempCY, tempLT, tempHT;
   double hh, ll, best;

   /* Four windows, all carried as rings and all walked oldest-first, the
    * cci.c:112-117 shape: an index that wrapped would be one more thing the
    * stream derivation has to prove, and the walk is the same values either
    * way.
    */
   CIRCBUF_PROLOG(hRing,double,50);
   CIRCBUF_PROLOG(lRing,double,50);
   CIRCBUF_PROLOG(fhRing,double,50);
   CIRCBUF_PROLOG(flRing,double,50);

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = cksp_lookback( optInTimePeriod, optInMultiplier, optInStopPeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   CIRCBUF_INIT( hRing, double, optInTimePeriod );
   CIRCBUF_INIT( lRing, double, optInTimePeriod );
   CIRCBUF_INIT( fhRing, double, optInStopPeriod );
   CIRCBUF_INIT( flRing, double, optInStopPeriod );

   /* The first stage is entered q-1 bars before the first output, because the
    * second stage's window reaches that far back. Each leg is anchored on its
    * own bar rather than on the caller's startIdx (the kc.c:73-77 rule): the
    * Average True Range is seeded as if TA_ATR had been entered here, and the
    * extremes are the p-windows ending on these same bars.
    */
   stageOneIdx = startIdx - (optInStopPeriod - 1);

   /* The Average True Range, carried inline rather than taken from a call: the
    * two stages advance together one bar at a time and a whole-range buffer
    * between them would not stream (supertrend.c:47-50).
    *
    * The arithmetic order is the bit-exactness contract with TA_ATR and is not
    * to be reordered: the range first, then the two previous-close distances in
    * that order; the seed summed from 0.0 over the first optInTimePeriod True
    * Ranges and divided once; wBeta rounded first and wAlpha derived from it.
    */
   wBeta  = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
   wAlpha = 1.0 - wBeta;

   today = stageOneIdx - atr_lookback( optInTimePeriod ) + 1;

   periodTotal = 0.0;
   i = optInTimePeriod;
   while( i-- > 0 )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT;

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      periodTotal += greatest;
      today++;
   }
   prevATR = periodTotal / optInTimePeriod;

   /* Skip the Average True Range's unstable period. The count comes from the
    * lookback rather than from the setting, so the two cannot disagree.
    */
   i = atr_lookback( optInTimePeriod ) - optInTimePeriod;
   while( i != 0 )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT;

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      prevATR = wAlpha * greatest + wBeta * prevATR;
      today++;
      i--;
   }

   /* `today` is now stageOneIdx and prevATR is the Average True Range of the
    * bar before it. Seed the price rings with the p-1 bars the first extreme
    * window needs behind that bar.
    */
   i = stageOneIdx - optInTimePeriod + 1;
   while( i < stageOneIdx )
   {
      hRing[hRing_Idx] = inHigh[i];
      lRing[lRing_Idx] = inLow[i];
      i++;
      CIRCBUF_NEXT(hRing);
      CIRCBUF_NEXT(lRing);
   }

   /* The prologue leaves prevATR as the Average True Range of stageOneIdx and
    * `today` one past it, so that bar is finished here rather than in the loop:
    * entering the loop with it would apply a second Wilder update and shift the
    * whole series one bar early. supertrend.c takes the same step for the same
    * reason.
    */
   today = stageOneIdx;

   hRing[hRing_Idx] = inHigh[today];
   lRing[lRing_Idx] = inLow[today];

   hh = hRing[hRing_Idx];
   for( jh = hRing_Idx+1; jh < optInTimePeriod; jh++ )
   {
      best = hRing[jh];
      if( best > hh )
         hh = best;
   }
   for( jh = 0; jh < hRing_Idx; jh++ )
   {
      best = hRing[jh];
      if( best > hh )
         hh = best;
   }

   ll = lRing[lRing_Idx];
   for( jl = lRing_Idx+1; jl < optInTimePeriod; jl++ )
   {
      best = lRing[jl];
      if( best < ll )
         ll = best;
   }
   for( jl = 0; jl < lRing_Idx; jl++ )
   {
      best = lRing[jl];
      if( best < ll )
         ll = best;
   }

   fhRing[fhRing_Idx] = hh - optInMultiplier * prevATR;
   flRing[flRing_Idx] = ll + optInMultiplier * prevATR;

   outIdx = 0;
   if( today >= startIdx )
   {
      outHighStop[outIdx] = fhRing[fhRing_Idx];
      outLowStop[outIdx] = flRing[flRing_Idx];
      outIdx = outIdx + 1;
   }

   today++;
   CIRCBUF_NEXT(hRing);
   CIRCBUF_NEXT(lRing);
   CIRCBUF_NEXT(fhRing);
   CIRCBUF_NEXT(flRing);

   while( today <= endIdx )
   {
      tempLT = inLow[today];
      tempHT = inHigh[today];
      tempCY = inClose[today-1];
      greatest = tempHT - tempLT;

      val2 = fabs( tempCY - tempHT );
      if( val2 > greatest )
         greatest = val2;

      val3 = fabs( tempCY - tempLT );
      if( val3 > greatest )
         greatest = val3;

      prevATR = wAlpha * greatest + wBeta * prevATR;

      hRing[hRing_Idx] = tempHT;
      lRing[lRing_Idx] = tempLT;

      /* The extremes of the p bars ending here. The newest sits at the ring's
       * own index, so the oldest is the slot after it and the walk is two
       * straight runs.
       */
      hh = hRing[hRing_Idx];
      for( jh = hRing_Idx+1; jh < optInTimePeriod; jh++ )
      {
         best = hRing[jh];
         if( best > hh )
            hh = best;
      }
      for( jh = 0; jh < hRing_Idx; jh++ )
      {
         best = hRing[jh];
         if( best > hh )
            hh = best;
      }

      ll = lRing[lRing_Idx];
      for( jl = lRing_Idx+1; jl < optInTimePeriod; jl++ )
      {
         best = lRing[jl];
         if( best < ll )
            ll = best;
      }
      for( jl = 0; jl < lRing_Idx; jl++ )
      {
         best = lRing[jl];
         if( best < ll )
            ll = best;
      }

      fhRing[fhRing_Idx] = hh - optInMultiplier * prevATR;
      flRing[flRing_Idx] = ll + optInMultiplier * prevATR;

      if( today >= startIdx )
      {
         /* The second stage, over the q first-stage bars ending here. At q = 1
          * both runs are empty and the value is the bar's own, which is the
          * Chandelier Exit form.
          */
         hh = fhRing[fhRing_Idx];
         for( kh = fhRing_Idx+1; kh < optInStopPeriod; kh++ )
         {
            best = fhRing[kh];
            if( best > hh )
               hh = best;
         }
         for( kh = 0; kh < fhRing_Idx; kh++ )
         {
            best = fhRing[kh];
            if( best > hh )
               hh = best;
         }

         ll = flRing[flRing_Idx];
         for( kl = flRing_Idx+1; kl < optInStopPeriod; kl++ )
         {
            best = flRing[kl];
            if( best < ll )
               ll = best;
         }
         for( kl = 0; kl < flRing_Idx; kl++ )
         {
            best = flRing[kl];
            if( best < ll )
               ll = best;
         }

         outHighStop[outIdx] = hh;
         outLowStop[outIdx] = ll;
         outIdx = outIdx + 1;
      }

      today++;
      CIRCBUF_NEXT(hRing);
      CIRCBUF_NEXT(lRing);
      CIRCBUF_NEXT(fhRing);
      CIRCBUF_NEXT(flRing);
   }

   CIRCBUF_DESTROY(flRing);
   CIRCBUF_DESTROY(fhRing);
   CIRCBUF_DESTROY(lRing);
   CIRCBUF_DESTROY(hRing);

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
