/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF,CC    Mario Fortier, Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081026 MF,CC Creation (synthetic gate: explicit _private variant, #183)
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth4.md — one copy, so there is one thing to keep true.
 */

int synth4_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode synth4(int    startIdx,
                  int    endIdx,
                  const double inReal[],
                  int    optInTimePeriod,
                  int   *outBegIdx,
                  int   *outNBElement,
                  int    outInteger[])
{
   double optInK_1 = 2.0 / ((double)(optInTimePeriod + 1));

   /* Simply call the internal implementation. */
   return synth4_private(startIdx, endIdx, inReal, optInTimePeriod, optInK_1,
      outBegIdx, outNBElement, outInteger);
}

TA_RetCode synth4_private(int    startIdx,
                          int    endIdx,
                          const double inReal[],
                          int    optInTimePeriod,
                          double optInK_1,
                          int   *outBegIdx,
                          int   *outNBElement,
                          int    outInteger[])
{
   int outIdx;
   int i, v, nbInitialElementNeeded;
   double tempReal, state;

   /* Internal implementation: no parameter check, and optInK_1 arrives
    * pre-computed rather than being re-derived from optInTimePeriod. */
   nbInitialElementNeeded = (optInTimePeriod-1);
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Fold the lookback window into the state, so the streaming variant has
    * real path dependence to reproduce. Bars are clamped to [0,1000000)
    * BEFORE any arithmetic: the fuzz corpus carries non-finite, negative and
    * 1e9-magnitude bars, and only a bounded non-negative state keeps the
    * (int) cast below identical across C, Rust, Java and C#. */
   state = 0.0;
   i = startIdx - nbInitialElementNeeded;
   while( i < startIdx )
   {
      tempReal = inReal[i];
      if( !(tempReal >= 0.0) || !(tempReal < 1000000.0) )
         tempReal = 0.0;
      state = ((tempReal-state)*optInK_1) + state;
      i++;
   }

   outIdx = 0;
   for( i = startIdx; i <= endIdx; i++ )
   {
      tempReal = inReal[i];
      if( !(tempReal >= 0.0) || !(tempReal < 1000000.0) )
         tempReal = 0.0;
      state = ((tempReal-state)*optInK_1) + state;

      /* The cast must be the WHOLE right-hand side (#160). The guard above
       * keeps state in [0,1000000), so the truncation is well defined. */
      v = (int)(state * 8.0);
      v &= 1023;
      outInteger[outIdx] = v;
      outIdx++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
