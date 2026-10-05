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
 *  100526 MF,CC  Carry the window's terms in a ring (#483).
 *  100526 MF,CC  Rebuild trigger compares magnitudes (#483).
 */

int rogerssatchell_lookback(int optInTimePeriod, double optInAnnualization)
{
   (void)optInAnnualization;

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
   double o, h, l, c, p1, p2, term, periodTotal, windowTotal, windowMagnitude, peakTotal, sqrtA;
   int i, j, outIdx, nbInitialElementNeeded, barsSinceRebuild;

   /* Each bar's term costs four logarithms, so it is computed once and kept
    * until it leaves the window. That also makes outReal safe to alias any
    * input: a bar is never read again once it has been consumed.
    */
   CIRCBUF_PROLOG(termRing,double,32);

   nbInitialElementNeeded = optInTimePeriod - 1;

   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   CIRCBUF_INIT( termRing, double, optInTimePeriod );

   /* Rogers and Satchell, The Annals of Applied Probability 1(4):504-512 (1991),
    * eq. (2): with the log price measured from the bar's open, S1 = ln(H/O),
    * I1 = ln(L/O) and X1 = ln(C/O), one bar's estimate of its variance is
    * S1(S1 - X1) + I1(I1 - X1) = ln(H/C)ln(H/O) + ln(L/C)ln(L/O), unbiased
    * whatever the drift. The window mean, the root and the annual scale are
    * convention, not the paper's.
    *
    * Keep sqrt(A) a separate factor applied last: A = 1.0 is then an exact
    * identity and the annualised output is exactly sqrt(A) times the per-bar
    * one. Folding A under the root moves both by an ulp.
    */
   sqrtA = sqrt( optInAnnualization );

   periodTotal = 0.0;
   for( j = startIdx - nbInitialElementNeeded; j < startIdx; j++ )
   {
      /* Keep the two products separate statements. As one expression the
       * first product is fused into the add, which changes the values and
       * breaks bit equality with the same estimator composed from LN, DIV,
       * MULT, ADD and SUM.
       *
       * A bar with any price at or below zero contributes a 0.0 term and
       * still counts toward the window. The test is exact, not a band, so a
       * small-unit quote is not zeroed.
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
      termRing[termRing_Idx] = term;
      periodTotal += term;
      CIRCBUF_NEXT(termRing);
   }

   i = startIdx;
   outIdx = 0;
   barsSinceRebuild = 32 * optInTimePeriod;
   peakTotal = fabs( periodTotal );

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

      /* Add, publish, subtract: the slot written here was taken out of the
       * sum at the end of the previous bar, and the advance lands on the
       * oldest term, the one that leaves next.
       */
      termRing[termRing_Idx] = term;
      periodTotal += term;
      windowTotal = periodTotal;
      windowMagnitude = fabs( windowTotal );
      peakTotal = ( windowMagnitude > peakTotal ) ? windowMagnitude : peakTotal;
      CIRCBUF_NEXT(termRing);
      periodTotal -= termRing[termRing_Idx];

      /* A running sum carries rounding at the scale of the largest window it
       * has held, so it is rebuilt as a fresh sum once it falls below 1e-6 of
       * that peak, and at least every 32 windows. Compare against the peak,
       * not the current sum: after a quiet stretch arrives the current sum
       * can be nothing but that rounding, of either sign, where a fresh sum
       * of an all-flat window is exactly 0.0.
       *
       * Compare magnitudes. A window holding a bar whose high or low sits
       * inside its open and close can sum below zero, and a signed test
       * would then rebuild on every bar for as long as it does.
       *
       * Sum oldest first, so the rebuilt value is the one a fresh pass over
       * the bars gives.
       */
      barsSinceRebuild--;
      if( windowMagnitude < 0.000001 * peakTotal || barsSinceRebuild <= 0 )
      {
         barsSinceRebuild = 32 * optInTimePeriod;

         windowTotal = 0.0;
         for( j = termRing_Idx; j < optInTimePeriod; j++ )
            windowTotal += termRing[j];
         for( j = 0; j < termRing_Idx; j++ )
            windowTotal += termRing[j];

         peakTotal = fabs( windowTotal );
         periodTotal = windowTotal;
         periodTotal -= termRing[termRing_Idx];
      }

      /* Divide, then root, then scale. A sum at or below zero answers 0.0
       * instead of reaching the root: only a bar whose high or low sits
       * strictly inside its open and close can make a fresh sum negative,
       * and the estimator is not defined on such a bar.
       */
      if( windowTotal > 0.0 )
         outReal[outIdx] = sqrtA * sqrt( windowTotal / (double)optInTimePeriod );
      else
         outReal[outIdx] = 0.0;

      outIdx = outIdx + 1;
      i++;
   } while( i <= endIdx );

   CIRCBUF_DESTROY(termRing);

   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
