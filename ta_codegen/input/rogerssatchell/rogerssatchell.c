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
 *  100526 KL,CC  Creation (#483).
 */

int rogerssatchell_lookback(int optInTimePeriod, double optInAnnualization)
{
   (void)optInAnnualization;

   /* The window's lookback and nothing else: there is no callee, and a term
    * reads only its own bar -- no previous close -- so no bar is consumed to
    * form it. Same as sum_lookback (sum/sum.c:16-19) and var_lookback
    * (var/var.c:21-26), and one less than an estimator that differences
    * against C[i-1]. optInAnnualization scales the output and cannot move the
    * first bar, as optInNbDev cannot in var.c:23.
    */
   return optInTimePeriod - 1;
}

TA_RetCode rogerssatchell(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   double optInAnnualization,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double o, h, l, c, p1, p2, term, periodTotal, windowTotal, peakTotal, sqrtA;
   int i, j, outIdx, trailingIdx, windowStart, nbInitialElementNeeded, barsSinceRebuild;

   nbInitialElementNeeded = optInTimePeriod - 1;

   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Rogers and Satchell, The Annals of Applied Probability 1(4):504-512 (1991),
    * eq. (2) on p.505: with the log price measured from the bar's open,
    * S1 = ln(H/O), I1 = ln(L/O) and X1 = ln(C/O), one bar's unbiased estimate
    * of its variance is S1(S1 - X1) + I1(I1 - X1), which is ln(H/C)ln(H/O) +
    * ln(L/C)ln(L/O). Eq. (3) is what sets this estimator apart: that
    * expectation is sigma^2 whatever the drift, so a bar that opens at its low
    * and closes at its high -- all drift, no dispersion -- reads exactly zero,
    * where a range-only estimator reads volatility.
    *
    * The paper stops there. The window mean, the root and the annual scale are
    * the convention of every implementation of it, not the authors'.
    */

   /* Once, so that optInAnnualization = 1.0 is an exact identity rather than a
    * multiply by a rounded 1.0, and so the per-bar and annualised outputs
    * differ by exactly this factor.
    */
   sqrtA = sqrt( optInAnnualization );

   trailingIdx = startIdx - nbInitialElementNeeded;

   periodTotal = 0.0;
   for( j = trailingIdx; j < startIdx; j++ )
   {
      /* The two products are SEPARATE statements on purpose. Written as one
       * expression the generator's FMA detector fuses the first product into
       * the add (backends/fma.rs:406-433; a log call counts as a float factor
       * at :252-285), which moves 159 of 243 outputs on the corpus at n = 10
       * and buys nothing measurable. VWMA splits its product for the same
       * reason (vwma/vwma.c:78-81).
       *
       * The guard is the whole bar, tested exactly rather than against a fixed
       * band (#253): a bar with any price at or below zero contributes a 0.0
       * term and still counts toward the window's n. Zeroing only the products
       * that touch the bad price has no implementation behind it, and dropping
       * the bar from the window makes n data-dependent.
       */
      o = inOpen[j];
      h = inHigh[j];
      l = inLow[j];
      c = inClose[j];
      if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 )
      {
         p1 = log( h / c ) * log( h / o );
         p2 = log( l / c ) * log( l / o );
         term = p1 + p2;
      }
      else
         term = 0.0;
      periodTotal += term;
   }

   /* outReal may be any of the four input arrays: the output written on a bar
    * lands at or before the window's own trailing index, so every input slot
    * this loop still reads is one no write has reached yet.
    */
   i = startIdx;
   outIdx = 0;
   barsSinceRebuild = 32 * optInTimePeriod;
   peakTotal = periodTotal;

   do
   {
      o = inOpen[i];
      h = inHigh[i];
      l = inLow[i];
      c = inClose[i];
      if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 )
      {
         p1 = log( h / c ) * log( h / o );
         p2 = log( l / c ) * log( l / o );
         term = p1 + p2;
      }
      else
         term = 0.0;
      periodTotal += term;
      peakTotal = ( periodTotal > peakTotal ) ? periodTotal : peakTotal;

      /* The sum this bar's output is taken from, before the trailing term is
       * removed for the next one.
       */
      windowTotal = periodTotal;

      o = inOpen[trailingIdx];
      h = inHigh[trailingIdx];
      l = inLow[trailingIdx];
      c = inClose[trailingIdx];
      if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 )
      {
         p1 = log( h / c ) * log( h / o );
         p2 = log( l / c ) * log( l / o );
         term = p1 + p2;
      }
      else
         term = 0.0;
      periodTotal -= term;
      trailingIdx++;

      /* Rebuild as a fresh window sum when the running sum has collapsed to
       * below 1e-6 of the largest it has held since the last rebuild, or at
       * least every 32 windows -- VAR's rule (var.c:101-113). Measured against
       * the PEAK, not the current sum: what a running sum of add-then-subtract
       * carries is rounding at the scale of the largest window it has seen, so
       * once a quiet stretch arrives the current sum can be nothing but that
       * rounding. On an all-flat window the rebuild restores an exact 0.0,
       * where a plain running sum leaves a residual that is negative about
       * forty per cent of the time -- and a negative sum under an
       * unconditional root is where the composition of shipped functions
       * produces NaN.
       */
      barsSinceRebuild--;
      if( windowTotal < 0.000001 * peakTotal || barsSinceRebuild <= 0 )
      {
         barsSinceRebuild = 32 * optInTimePeriod;

         windowStart = i - nbInitialElementNeeded;

         windowTotal = 0.0;
         for( j = windowStart; j <= i; j++ )
         {
            o = inOpen[j];
            h = inHigh[j];
            l = inLow[j];
            c = inClose[j];
            if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 )
            {
               p1 = log( h / c ) * log( h / o );
               p2 = log( l / c ) * log( l / o );
               term = p1 + p2;
            }
            else
               term = 0.0;
            windowTotal += term;
         }

         /* The rebuilt window becomes the carried state, with its trailing
          * term removed again so the two paths leave the same thing behind.
          */
         periodTotal = windowTotal;
         peakTotal = windowTotal;

         o = inOpen[windowStart];
         h = inHigh[windowStart];
         l = inLow[windowStart];
         c = inClose[windowStart];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 )
         {
            p1 = log( h / c ) * log( h / o );
            p2 = log( l / c ) * log( l / o );
            term = p1 + p2;
         }
         else
            term = 0.0;
         periodTotal -= term;
      }

      /* The divide, then the root, then the scale -- the spelling every
       * implementation of this estimator uses, and what makes A = 1.0 exact.
       *
       * A sum at or below zero answers 0.0 rather than reaching the root. A
       * fresh sum of terms from consistent bars cannot be negative, so this
       * only catches a bar whose high or low sits strictly inside its open and
       * close, which is not a bar the estimator is defined on. VAR floors its
       * variance for the same reason, so that STDDEV can root it
       * unconditionally (var.c:165-166).
       */
      if( windowTotal > 0.0 )
         outReal[outIdx] = sqrtA * sqrt( windowTotal / (double)optInTimePeriod );
      else
         outReal[outIdx] = 0.0;

      outIdx = outIdx + 1;
      i++;
   } while( i <= endIdx );

   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
