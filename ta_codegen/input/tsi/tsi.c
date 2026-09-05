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
 *  090426 MF,CC  First version (#360).
 */

int tsi_lookback(int optInFirstPeriod, int optInSecondPeriod)
{
   /* One bar forms the first close-to-close change, then the two EMA warm-ups
    * the pipeline stacks on it. Each term is exactly the lookback of the
    * function it comes from, which is also what makes TSI inherit
    * TA_FUNC_UNST_EMA from its callee.
    */
   return 1 + ema_lookback( optInFirstPeriod ) + ema_lookback( optInSecondPeriod );
}

TA_RetCode tsi(int startIdx, int endIdx,
   const double inReal[],
   int optInFirstPeriod,
   int optInSecondPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double kFirst, kSecond;
   double emaFirstNum, emaFirstDen, emaSecondNum, emaSecondDen;
   double sumFirstNum, sumFirstDen, sumSecondNum, sumSecondDen;
   double prevClose, mom, absMom, tsiValue;
   int lookbackTotal, lookbackFirst;
   int today, outIdx, nBar, nSecond;

   lookbackTotal = tsi_lookback( optInFirstPeriod, optInSecondPeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx = startIdx;

   /* Blau's double smoothing in one pass: the signed momentum and its
    * magnitude are carried through the same two EMA stages, then divided.
    *
    * Each stage seeds the way ema.c does -- a simple average of that stage's
    * first 'period' inputs -- so the result is bit-identical to TA_MOM(1)
    * followed by two TA_EMA calls on each chain. The stage boundary below is
    * the callee LOOKBACK, not (period-1), so that a warm
    * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: the second stage then
    * seeds from the values the first would have published. The seed sums
    * accumulate from 0.0 in production order and the recurrence is
    * ((x-prev)*k)+prev rather than the algebraically equal k*x+(1-k)*prev; do
    * not reorder or fuse them (0.0+x is not x for x=-0.0). That order IS the
    * bit-exactness contract against the composed reference.
    *
    * TA_GetCompatibility() is deliberately NOT consulted, for the reason
    * spelled out in efi.c: ema.c's TA_COMPATIBILITY_METASTOCK seeding arm is
    * preserved for the functions that already shipped with it and dropped from
    * new ones, and it is not reachable at all from the Rust, Java and C# APIs.
    *
    * prevClose is carried in a scalar rather than re-read from inReal[t-1]
    * because outReal may alias inReal: the slot holding close[t-1] may already
    * hold an output written a bar earlier.
    */
   kFirst  = 2.0 / ((double)(optInFirstPeriod + 1));
   kSecond = 2.0 / ((double)(optInSecondPeriod + 1));

   lookbackFirst = ema_lookback( optInFirstPeriod );

   emaFirstNum  = 0.0;
   emaFirstDen  = 0.0;
   emaSecondNum = 0.0;
   emaSecondDen = 0.0;
   sumFirstNum  = 0.0;
   sumFirstDen  = 0.0;
   sumSecondNum = 0.0;
   sumSecondDen = 0.0;

   /* The first bar carrying a close-to-close change. */
   today     = startIdx - lookbackTotal + 1;
   prevClose = inReal[today-1];
   nBar      = 0;

   /* Warm-up. Runs through startIdx inclusive: the last pass here completes
    * the second stage's seed, so it produces the first output.
    */
   while( today <= startIdx )
   {
      mom = inReal[today] - prevClose;
      prevClose = inReal[today];
      absMom = fabs( mom );

      /* Stage 1: the first EMA, over the raw momentum and its magnitude. */
      if( nBar < optInFirstPeriod )
      {
         sumFirstNum = sumFirstNum + mom;
         sumFirstDen = sumFirstDen + absMom;
         if( nBar == optInFirstPeriod - 1 )
         {
            emaFirstNum = sumFirstNum / optInFirstPeriod;
            emaFirstDen = sumFirstDen / optInFirstPeriod;
         }
      }
      else
      {
         emaFirstNum = ((mom - emaFirstNum) * kFirst) + emaFirstNum;
         emaFirstDen = ((absMom - emaFirstDen) * kFirst) + emaFirstDen;
      }

      /* Stage 2: the second EMA, over what stage 1 publishes.
       *
       * The stage counter is compared BEFORE it is subtracted, never after.
       * Writing this as `nSecond = nBar - lookbackFirst; if( nSecond >= 0 )`
       * is correct in C, where the counters are signed, and broken everywhere
       * else: the Rust backend renders them as usize, so the subtraction
       * underflows for the first lookbackFirst bars -- a panic in a debug
       * build and a wrap in release. It would also be invisible to the
       * cross-language gate, which runs release servers at unstable period 0,
       * where the branch the wrap wrongly takes happens to be a no-op because
       * both accumulators are still 0.0. smi.c states the same rule. */
      if( nBar >= lookbackFirst )
      {
         nSecond = nBar - lookbackFirst;
         if( nSecond < optInSecondPeriod )
         {
            sumSecondNum = sumSecondNum + emaFirstNum;
            sumSecondDen = sumSecondDen + emaFirstDen;
            if( nSecond == optInSecondPeriod - 1 )
            {
               emaSecondNum = sumSecondNum / optInSecondPeriod;
               emaSecondDen = sumSecondDen / optInSecondPeriod;
            }
         }
         else
         {
            emaSecondNum = ((emaFirstNum - emaSecondNum) * kSecond) + emaSecondNum;
            emaSecondDen = ((emaFirstDen - emaSecondDen) * kSecond) + emaSecondDen;
         }
      }

      nBar = nBar + 1;
      today = today + 1;
   }

   /* The denominator is an EMA of an EMA of |momentum|: every term is
    * non-negative and every weight positive, so it is zero only when every
    * change that reached it was exactly zero -- 0/0, since the numerator is
    * zero with it, reported as the neutral 0.0 by the CCI (#7) and IMI (#112)
    * convention. Tested exactly rather than against a fixed band: a price
    * change carries the quote unit, and TA_IS_ZERO zeroes the oscillator for
    * any instrument quoted below it (issue #253).
    */
   if( emaSecondDen > 0.0 )
      tsiValue = (100.0 * emaSecondNum) / emaSecondDen;
   else
      tsiValue = 0.0;

   outReal[0] = tsiValue;
   outIdx = 1;

   /* Stable zone. Both stages are a pure recursion from here on. */
   while( today <= endIdx )
   {
      mom = inReal[today] - prevClose;
      prevClose = inReal[today];
      absMom = fabs( mom );

      emaFirstNum = ((mom - emaFirstNum) * kFirst) + emaFirstNum;
      emaFirstDen = ((absMom - emaFirstDen) * kFirst) + emaFirstDen;
      emaSecondNum = ((emaFirstNum - emaSecondNum) * kSecond) + emaSecondNum;
      emaSecondDen = ((emaFirstDen - emaSecondDen) * kSecond) + emaSecondDen;

      if( emaSecondDen > 0.0 )
         tsiValue = (100.0 * emaSecondNum) / emaSecondDen;
      else
         tsiValue = 0.0;

      outReal[outIdx] = tsiValue;
      outIdx = outIdx + 1;
      today = today + 1;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
