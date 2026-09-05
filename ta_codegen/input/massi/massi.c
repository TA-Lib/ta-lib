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
 *  090426 MF,CC  First version (issue #359).
 */

int massi_lookback(int optInFastPeriod, int optInSlowPeriod)
{
   /* Two stacked EMA warm-ups over the high-low range, then the summation
    * window. The EMA term is exactly the callee's own lookback, which is what
    * makes MASSI inherit TA_FUNC_UNST_EMA -- and it shifts by 2u, not u.
    */
   return (ema_lookback( optInFastPeriod ) * 2) + (optInSlowPeriod - 1);
}

TA_RetCode massi(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInFastPeriod,
   int optInSlowPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double optInK_1, hl, ema1, ema2, sum1, sum2, ratio, total, tempReal;
   int lookbackTotal, lookbackEma, lookbackEma2;
   int today, outIdx, nBar, n2;

   CIRCBUF_PROLOG(ratioRing,double,32);

   lookbackEma   = ema_lookback( optInFastPeriod );
   lookbackEma2  = lookbackEma * 2;
   lookbackTotal = lookbackEma2 + (optInSlowPeriod - 1);

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   CIRCBUF_INIT( ratioRing, double, optInSlowPeriod );

   *outBegIdx = startIdx;

   /* Dorsey's pipeline in one pass: EMA of the high-low range, EMA of that,
    * their ratio summed over a trailing window.
    *
    * Each stage seeds the way ema.c does -- a simple average of that stage's
    * first optInFastPeriod inputs -- and its boundary below is the callee
    * LOOKBACK, never (optInFastPeriod - 1). The two coincide exactly at
    * unstable period 0, which is where every cross-language gate runs, so
    * confusing them is invisible until TA_SetUnstablePeriod(TA_FUNC_UNST_EMA)
    * is warmed. The seed sums accumulate from 0.0 in production order; do not
    * reorder or fuse them (0.0+x is not x for x=-0.0).
    *
    * Seed from the SMA arm only: ema.c's TA_COMPATIBILITY_METASTOCK arm is
    * unreachable from the Rust, Java and C# APIs, so consulting
    * TA_GetCompatibility() here would make C diverge from three backends for a
    * setting they cannot read.
    */
   optInK_1 = 2.0 / ((double)(optInFastPeriod + 1));

   ema1     = 0.0;
   ema2     = 0.0;
   sum1     = 0.0;
   sum2     = 0.0;
   total    = 0.0;
   tempReal = 0.0;

   today = startIdx - lookbackTotal;
   nBar  = 0;

   /* Runs through startIdx inclusive: that last pass fills the summation
    * window and so produces the first output. */
   while( today <= startIdx )
   {
      hl = inHigh[today] - inLow[today];

      if( nBar < optInFastPeriod )
      {
         sum1 = sum1 + hl;
         if( nBar == optInFastPeriod - 1 )
            ema1 = sum1 / optInFastPeriod;
      }
      else
         ema1 = ((hl - ema1) * optInK_1) + ema1;

      /* The stage counter is compared BEFORE it is subtracted, never after.
       * `n2 = nBar - lookbackEma; if( n2 >= 0 )` is correct in C and broken
       * everywhere else: the Rust backend renders these as usize, so the
       * subtraction underflows for the first lookbackEma bars.
       */
      if( nBar >= lookbackEma )
      {
         n2 = nBar - lookbackEma;
         if( n2 < optInFastPeriod )
         {
            sum2 = sum2 + ema1;
            if( n2 == optInFastPeriod - 1 )
               ema2 = sum2 / optInFastPeriod;
         }
         else
            ema2 = ((ema1 - ema2) * optInK_1) + ema2;
      }

      if( nBar >= lookbackEma2 )
      {
         /* A flat market is the ratio's continuous limit, 1.0, not the zero
          * that an oscillator centred on zero would report: MASSI's own
          * neutral is optInSlowPeriod. Test ema2 exactly and never through an
          * epsilon band -- a smoothed price range carries the quote unit, so a
          * fixed band would pin the whole index at optInSlowPeriod for any
          * instrument quoted under it (issue #253).
          */
         if( ema2 == 0.0 )
            ratio = 1.0;
         else
            ratio = ema1 / ema2;

         /* TA_SUM's accumulation order -- add, publish, subtract -- reproduced
          * over a ring: the slot written here was emptied out of `total` at the
          * end of the previous bar, so nothing has to be read before the store.
          */
         ratioRing[ratioRing_Idx] = ratio;
         total = total + ratio;
         CIRCBUF_NEXT(ratioRing);
         if( nBar == lookbackTotal )
         {
            tempReal = total;
            total = total - ratioRing[ratioRing_Idx];
         }
      }

      nBar = nBar + 1;
      today = today + 1;
   }

   /* In-place safe: this store lands lookbackTotal bars behind the input
    * cursor, so no bar is written under a read still owed to it.
    */
   outReal[0] = tempReal;
   outIdx = 1;

   while( today <= endIdx )
   {
      hl = inHigh[today] - inLow[today];
      ema1 = ((hl - ema1) * optInK_1) + ema1;
      ema2 = ((ema1 - ema2) * optInK_1) + ema2;

      if( ema2 == 0.0 )
         ratio = 1.0;
      else
         ratio = ema1 / ema2;

      ratioRing[ratioRing_Idx] = ratio;
      total = total + ratio;
      CIRCBUF_NEXT(ratioRing);
      tempReal = total;
      total = total - ratioRing[ratioRing_Idx];

      outReal[outIdx] = tempReal;
      outIdx = outIdx + 1;
      today = today + 1;
   }

   *outNBElement = outIdx;

   CIRCBUF_DESTROY(ratioRing);

   return TA_SUCCESS;
}
