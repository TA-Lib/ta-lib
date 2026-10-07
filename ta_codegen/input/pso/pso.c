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
 *  100626 KL,CC  Initial version (#473).
 *  100726 MF,CC  Batch tier: block scan of the Fast-K window (#473).
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
   CIRCBUF_PROLOG(sufHighest,double,30);
   CIRCBUF_PROLOG(preHighest,double,30);
   CIRCBUF_PROLOG(sufLowest,double,30);
   CIRCBUF_PROLOG(preLowest,double,30);
   double emaK, emaBeta;
   double highest, lowest, tmp, tempReal, fastK, nsk;
   double ema1, ema2, sum1, sum2;
   int lookbackTotal, lookbackEMA, warmBars;
   int today, i, m, blockStart, blockNext, nAvail, outIdx;
   int nBar, n2, nOut;

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

   /* Same values as pso_ALT1 below, which carries the formula. Only the
    * Fast-K window differs: a Van Herk / Gil-Werman block scan (WILLR's,
    * issue #147), so the cost per bar does not depend on the period or on
    * the shape of the input, where the cached extremum of pso_ALT1 rescans
    * its whole window on every bar of a flat or trending stretch. Every
    * scratch array holds copies, so the output may alias an input.
    */
   emaBeta = ((double)(optInEMAPeriod - 1)) / ((double)(optInEMAPeriod + 1));
   emaK    = 1.0 - emaBeta;
   if( emaBeta < 0.5 ) emaBeta = 1.0 - emaK;

   lookbackEMA = ema_lookback( optInEMAPeriod );
   warmBars    = lookbackEMA + lookbackEMA;

   ema1 = 0.0;
   ema2 = 0.0;
   sum1 = 0.0;
   sum2 = 0.0;
   nBar = 0;

   today      = startIdx - warmBars;
   blockStart = today - (optInFastK_Period - 1);
   outIdx     = 0;

   CIRCBUF_INIT( sufHighest, double, optInFastK_Period );
   CIRCBUF_INIT( preHighest, double, optInFastK_Period );
   CIRCBUF_INIT( sufLowest, double, optInFastK_Period );
   CIRCBUF_INIT( preLowest, double, optInFastK_Period );

   while( today <= endIdx )
   {
      /* Suffix extrema of the block [blockStart, today]. */
      i = today;
      highest = inHigh[i];
      lowest = inLow[i];
      sufHighest[optInFastK_Period - 1] = highest;
      sufLowest[optInFastK_Period - 1] = lowest;
      TA_UNROLL(4)
      while( i > blockStart )
      {
         i--;
         tmp = inHigh[i];
         if( tmp > highest )
         {
            highest = tmp;
         }
         tmp = inLow[i];
         if( tmp < lowest )
         {
            lowest = tmp;
         }
         sufHighest[i - blockStart] = highest;
         sufLowest[i - blockStart] = lowest;
      }

      /* Prefix extrema of the next block, clamped to what remains, stored
       * one slot up: slot 0 repeats the suffix so that bar 'today', whose
       * window is the block itself, runs the same combine as the others.
       */
      blockNext = blockStart + optInFastK_Period;
      nAvail = endIdx + 1 - blockNext;
      if( nAvail > optInFastK_Period - 1 )
      {
         nAvail = optInFastK_Period - 1;
      }
      preHighest[0] = sufHighest[0];
      preLowest[0] = sufLowest[0];
      if( nAvail > 0 )
      {
         i = 1;
         highest = inHigh[blockNext];
         lowest = inLow[blockNext];
         preHighest[1] = highest;
         preLowest[1] = lowest;
         TA_UNROLL(4)
         while( i < nAvail )
         {
            tmp = inHigh[blockNext + i];
            if( tmp > highest )
            {
               highest = tmp;
            }
            tmp = inLow[blockNext + i];
            if( tmp < lowest )
            {
               lowest = tmp;
            }
            preHighest[i + 1] = highest;
            preLowest[i + 1] = lowest;
            i++;
         }
      }

      /* The squash runs as a pass of its own over what the block wrote:
       * a call inside the loop above it would have every carried value
       * saved and restored around it on each bar.
       */
      nOut = 0;
      m = 0;
      while( m <= nAvail )
      {
         highest = sufHighest[m];
         if( preHighest[m] > highest )
         {
            highest = preHighest[m];
         }
         lowest = sufLowest[m];
         if( preLowest[m] < lowest )
         {
            lowest = preLowest[m];
         }

         tempReal = highest - lowest;
         if( !TA_IS_ZERO_SCALED( tempReal, fabs(highest) + fabs(lowest) ) )
            fastK = ((inClose[today + m] - lowest) / tempReal) * 100.0;
         else
            fastK = 50.0;

         nsk = 0.1 * (fastK - 50.0);

         if( nBar > warmBars )
         {
            ema1 = emaK * nsk + emaBeta * ema1;
            ema2 = emaK * ema1 + emaBeta * ema2;
            outReal[outIdx + nOut] = 0.5 * ema2;
            nOut = nOut + 1;
         }
         else
         {
            /* The two seeds, staged as pso_ALT1 stages them. */
            if( nBar < optInEMAPeriod )
            {
               sum1 = sum1 + nsk;
               if( nBar == optInEMAPeriod - 1 )
                  ema1 = sum1 / optInEMAPeriod;
            }
            else
               ema1 = emaK * nsk + emaBeta * ema1;

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

            if( nBar == warmBars )
            {
               outReal[outIdx + nOut] = 0.5 * ema2;
               nOut = nOut + 1;
            }
         }
         nBar = nBar + 1;
         m++;
      }

      i = 0;
      while( i < nOut )
      {
         outReal[outIdx] = tanh( outReal[outIdx] );
         outIdx = outIdx + 1;
         i++;
      }

      today = today + nAvail + 1;
      blockStart = blockNext;
   }

   CIRCBUF_DESTROY(sufHighest);
   CIRCBUF_DESTROY(preHighest);
   CIRCBUF_DESTROY(sufLowest);
   CIRCBUF_DESTROY(preLowest);

   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}

/* PRAGMA TA_ALT={STREAM,ALL_LANGUAGES} the block scan cannot be a per-bar automaton */
TA_RetCode pso_ALT1(int startIdx, int endIdx,
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
    * At optInEMAPeriod == 1 the recursion runs at a k of 1.0 and a beta of
    * 0.0 where the composed chain copies: the same bits while every Fast-K
    * is finite.
    */
   emaBeta = ((double)(optInEMAPeriod - 1)) / ((double)(optInEMAPeriod + 1));
   emaK    = 1.0 - emaBeta;
   if( emaBeta < 0.5 ) emaBeta = 1.0 - emaK;

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

   /* Warm-up. Runs through startIdx inclusive: its last pass produces the
    * first output.
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

      /* Pass 2, over what pass 1 publishes. Keep the comparison ahead of
       * the subtraction: the counters are unsigned in Rust.
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
