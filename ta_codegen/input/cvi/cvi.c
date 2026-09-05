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
 *  090426 MF,CC  First version (issue #358).
 */

int cvi_lookback(int optInTimePeriod, int optInROCPeriod)
{
   return ema_lookback( optInTimePeriod ) + rocp_lookback( optInROCPeriod );
}

TA_RetCode cvi(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInTimePeriod,
   int optInROCPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double prevEMA, laggedEMA, tempReal, optInK_1;
   int i, today, outIdx, lookbackTotal;

   /* CVI[t] = 100 * (E[t] - E[t-optInROCPeriod]) / E[t-optInROCPeriod], with E
    * an EMA of the high-low spread. The spread is never materialised and the
    * EMA is anchored optInROCPeriod bars behind startIdx.
    *
    * The arithmetic below is TA_EMA's and TA_ROCP's verbatim -- seed sum
    * accumulated from 0.0 in ascending bar order, ((x-prev)*k)+prev, and
    * 100*((a-b)/b) under an exact zero test. That is what makes this fused pass
    * bit-identical to composing TA_SUB, TA_EMA and TA_ROCP, which test_cvi.c
    * holds it to memcmp-exact; reshaping any of it breaks that silently. The
    * guard stays an exact `!= 0.0` and never TA_IS_ZERO -- an epsilon band
    * carries the quote unit and would zero the indicator for anything priced
    * under it (issue #253).
    */
   CIRCBUF_PROLOG(emaRing,double,32);

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = ema_lookback( optInTimePeriod ) + rocp_lookback( optInROCPeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   CIRCBUF_INIT( emaRing, double, optInROCPeriod );

   optInK_1 = 2.0 / ((double)(optInTimePeriod + 1));

   today = startIdx-lookbackTotal;
   i = optInTimePeriod;
   tempReal = 0.0;
   while( i-- > 0 )
   {
      tempReal += inHigh[today]-inLow[today];
      today++;
   }
   prevEMA = tempReal / optInTimePeriod;

   /* The ring keeps only the newest optInROCPeriod values, so pushing every EMA
    * value from the seed bar on leaves exactly the lagged terms the output
    * loop reads.
    */
   emaRing[emaRing_Idx] = prevEMA;
   CIRCBUF_NEXT(emaRing);

   while( today < startIdx )
   {
      tempReal = inHigh[today]-inLow[today];
      prevEMA = ((tempReal-prevEMA)*optInK_1) + prevEMA;
      today++;
      emaRing[emaRing_Idx] = prevEMA;
      CIRCBUF_NEXT(emaRing);
   }

   /* Read the expiring slot before overwriting it: that is what makes the lag
    * exactly optInROCPeriod rather than one less.
    */
   outIdx = 0;
   while( today <= endIdx )
   {
      tempReal = inHigh[today]-inLow[today];
      prevEMA = ((tempReal-prevEMA)*optInK_1) + prevEMA;
      today++;
      laggedEMA = emaRing[emaRing_Idx];
      emaRing[emaRing_Idx] = prevEMA;
      CIRCBUF_NEXT(emaRing);
      if( laggedEMA != 0.0 )
         outReal[outIdx++] = 100.0*((prevEMA-laggedEMA)/laggedEMA);
      else
         outReal[outIdx++] = 0.0;
   }

   CIRCBUF_DESTROY(emaRing);

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
