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
 *  100626 MF,CC  Initial version (#473).
 *
 */

int pso_lookback(int optInFastK_Period, int optInEMAPeriod)
{
   /* One Fast-K window, then the two EMA warm-ups the author stacks on top of
    * it: the first smooths the normalised Fast-K, the second smooths the
    * first. Both terms are exactly the lookback of the function they come
    * from, so neither is restated here -- which is also what makes PSO
    * inherit TA_FUNC_UNST_EMA from its callee rather than take an id of its
    * own, and what carries the Auto warm-up levels of #492 through both
    * passes without this file knowing their rule.
    */
   return (optInFastK_Period - 1)
   + ema_lookback( optInEMAPeriod )
   + ema_lookback( optInEMAPeriod );
}

TA_RetCode pso(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInFastK_Period,
   int optInEMAPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double emaK, emaBeta;
   double highest, lowest, tmp, fastK, nsk;
   double ema1, ema2, sum1, sum2;
   int lookbackTotal, lookbackEMA;
   int today, trailingIdx, highestIdx, lowestIdx, i, outIdx;
   int nBar, n2;

   lookbackTotal = pso_lookback( optInFastK_Period, optInEMAPeriod );

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx = startIdx;

   /* Leibfarth's pipeline in one pass: a Fast-K window, the affine step that
    * centres it on zero, two EMA passes and the squash.
    *
    * The affine step comes BEFORE the smoothing, as the article's listing
    * spells it. Moving it after is equal in real arithmetic and differs by up
    * to 6.0e-16 absolute in doubles, which no golden at a sane tolerance can
    * see; only the composite gate against TA_STOCHF + TA_EMA + TA_EMA can.
    *
    * Each pass seeds the way ema.c does -- a simple average of that pass's
    * first optInEMAPeriod inputs, summed from 0.0 in production order -- so
    * the result is bit-identical to that composed chain. The stage boundary
    * below is the callee LOOKBACK, not (period-1), so that a warm
    * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: the second pass then
    * seeds from the values the first would have published, exactly as the
    * composed form does.
    *
    * At optInEMAPeriod == 1 ema.c takes an explicit copy path, because its
    * recursion at a k of 1.0 and a beta of 0.0 does not keep the sign of a
    * -0.0 input. The recursion below is left to run instead: its input is
    * 0.1*(fastK - 50.0), and x - x is +0.0 in every rounding mode, so -0.0
    * cannot reach it. The composite gate runs 5/1 and compares bitwise.
    */
   emaBeta = ((double)(optInEMAPeriod - 1)) / ((double)(optInEMAPeriod + 1));
   emaK    = 1.0 - emaBeta;
   emaBeta = 1.0 - emaK;

   lookbackEMA = ema_lookback( optInEMAPeriod );

   ema1 = 0.0;
   ema2 = 0.0;
   sum1 = 0.0;
   sum2 = 0.0;

   highest    = 0.0;
   lowest     = 0.0;
   highestIdx = -1;
   lowestIdx  = -1;

   /* The first bar carrying a full Fast-K window. */
   trailingIdx = startIdx - lookbackTotal;
   today       = trailingIdx + (optInFastK_Period - 1);
   nBar        = 0;

   /* Warm-up. Runs through startIdx inclusive: the last pass here is the one
    * that completes the second pass's seed, so it produces the first output.
    */
   while( today <= startIdx )
   {
      /* Set the lowest low */
      tmp = inLow[today];
      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         lowest = inLow[lowestIdx];
         i = lowestIdx;
         while( ++i<=today )
         {
            tmp = inLow[i];
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

      /* Set the highest high */
      tmp = inHigh[today];
      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inHigh[highestIdx];
         i = highestIdx;
         while( ++i<=today )
         {
            tmp = inHigh[i];
            if( tmp > highest )
            {
               highestIdx = i;
               highest = tmp;
            }
         }
      }
      else if( tmp >= highest )
      {
         highestIdx = today;
         highest = tmp;
      }

      /* Fast-K, spelled as stochf.c spells it: divide by the range itself and
       * scale by 100.0 after, guarded by the very expression the division
       * uses, against ITS OWN two extremes rather than a fixed constant
       * (issue #253).
       *
       * Where STOCHF answers 0.0 on a flat window, PSO answers 50.0, the
       * Fast-K midpoint, so that a flat market reads PSO 0 instead of
       * -tanh(2.5) = -0.9866, a near-extreme oversold reading that nothing in
       * the window supports (#473 Q4, the neutral-point rule of #112).
       */
      if( !TA_IS_ZERO_SCALED(highest-lowest, fabs(highest)+fabs(lowest)) )
         fastK = ((inClose[today]-lowest)/(highest-lowest))*100.0;
      else
         fastK = 50.0;

      nsk = 0.1 * (fastK - 50.0);

      /* Pass 1, over the normalised Fast-K. */
      if( nBar < optInEMAPeriod )
      {
         sum1 = sum1 + nsk;
         if( nBar == optInEMAPeriod - 1 )
            ema1 = sum1 / optInEMAPeriod;
      }
      else
         ema1 = emaK * nsk + emaBeta * ema1;

      /* Pass 2, over what pass 1 publishes.
       *
       * The stage counter is compared BEFORE it is subtracted, never after.
       * Writing this as `n2 = nBar - lookbackEMA; if( n2 >= 0 )` is correct in
       * C, where the counters are signed, and broken everywhere else: the Rust
       * backend renders them as usize, so the subtraction underflows for the
       * first lookbackEMA bars -- a panic in a debug build and a wrap in
       * release (the lesson smi.c records).
       */
      if( nBar >= lookbackEMA )
      {
         n2 = nBar - lookbackEMA;
         if( n2 < optInEMAPeriod )
         {
            sum2 = sum2 + ema1;
            if( n2 == optInEMAPeriod - 1 )
               ema2 = sum2 / optInEMAPeriod;
         }
         else
            ema2 = emaK * ema1 + emaBeta * ema2;
      }

      nBar = nBar + 1;
      trailingIdx = trailingIdx + 1;
      today = today + 1;
   }

   /* tanh(ss/2) rather than the published (e^ss - 1)/(e^ss + 1): the same
    * function, equal within 2.2e-16 on normal data, exactly odd and well
    * conditioned at zero. TA-Lib does not validate that a close lies inside
    * its bar, so ss is only bounded by [-5, 5] on well-formed input; the
    * literal form emits NaN from a successful call once ss exceeds 709.78,
    * which the house rule of #112 forbids (#473 Q3).
    */
   outReal[0] = tanh( 0.5 * ema2 );
   outIdx = 1;

   /* Stable zone. Both passes are pure recursions from here on. */
   while( today <= endIdx )
   {
      /* Set the lowest low */
      tmp = inLow[today];
      if( lowestIdx < trailingIdx )
      {
         lowestIdx = trailingIdx;
         lowest = inLow[lowestIdx];
         i = lowestIdx;
         while( ++i<=today )
         {
            tmp = inLow[i];
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

      /* Set the highest high */
      tmp = inHigh[today];
      if( highestIdx < trailingIdx )
      {
         highestIdx = trailingIdx;
         highest = inHigh[highestIdx];
         i = highestIdx;
         while( ++i<=today )
         {
            tmp = inHigh[i];
            if( tmp > highest )
            {
               highestIdx = i;
               highest = tmp;
            }
         }
      }
      else if( tmp >= highest )
      {
         highestIdx = today;
         highest = tmp;
      }

      if( !TA_IS_ZERO_SCALED(highest-lowest, fabs(highest)+fabs(lowest)) )
         fastK = ((inClose[today]-lowest)/(highest-lowest))*100.0;
      else
         fastK = 50.0;

      nsk = 0.1 * (fastK - 50.0);

      ema1 = emaK * nsk + emaBeta * ema1;
      ema2 = emaK * ema1 + emaBeta * ema2;

      outReal[outIdx] = tanh( 0.5 * ema2 );
      outIdx = outIdx + 1;
      trailingIdx = trailingIdx + 1;
      today = today + 1;
   }

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
