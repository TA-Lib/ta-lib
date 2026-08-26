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
 *  081126 MF,CC Creation (synthetic gate: PRAGMA TA_ALT batch alternate, #190)
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth6.md — one copy, so there is one thing to keep true.
 */

int synth6_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode synth6(int    startIdx,
                  int    endIdx,
                  const double inReal[],
                  int    optInTimePeriod,
                  int   *outBegIdx,
                  int   *outNBElement,
                  int    outInteger[])
{
   int outIdx;
   int today, trailingIdx, v, nbInitialElementNeeded;
   double barLo, barHi;

   nbInitialElementNeeded = (optInTimePeriod-1);
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Walk the window with a trailing cursor: an ordinary T3 ring over inReal.
    * This is the base, so it is what every cell no alternate claims resolves
    * to -- here, the streaming tier.
    *
    * Bars are clamped to [0,1000000) BEFORE any arithmetic: the fuzz corpus
    * carries non-finite, negative and 1e9-magnitude bars, and only a bounded
    * non-negative sum keeps the (int) cast identical across C, Rust, Java
    * and C#.
    */
   outIdx = 0;
   today = startIdx;
   trailingIdx = startIdx - nbInitialElementNeeded;
   while( today <= endIdx )
   {
      barLo = inReal[trailingIdx];
      if( !(barLo >= 0.0) || !(barLo < 1000000.0) )
         barLo = 0.0;
      barHi = inReal[today];
      if( !(barHi >= 0.0) || !(barHi < 1000000.0) )
         barHi = 0.0;

      /* The cast must be the WHOLE right-hand side (#160). The clamps above
       * keep the sum in [0,2000000), so the truncation is well defined. */
      v = (int)(barLo + barHi);
      v &= 1023;
      outInteger[outIdx] = v;
      outIdx++;
      trailingIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}

/* PRAGMA TA_ALT={BATCH,ALL_LANGUAGES} window-start indexing, batch only */
TA_RetCode synth6_ALT1(int    startIdx,
                       int    endIdx,
                       const double inReal[],
                       int    optInTimePeriod,
                       int   *outBegIdx,
                       int   *outNBElement,
                       int    outInteger[])
{
   int outIdx;
   int i, v, nbInitialElementNeeded;
   double barLo, barHi;

   nbInitialElementNeeded = (optInTimePeriod-1);
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Same window, same addition order, indexed by the window's START instead.
    * `inReal[i + nbInitialElementNeeded]` reads ahead of the emitted bar, so
    * this body could not be streamed -- which is precisely why it is claimed
    * for BATCH only.
    */
   outIdx = 0;
   i = startIdx - nbInitialElementNeeded;
   while( i + nbInitialElementNeeded <= endIdx )
   {
      barLo = inReal[i];
      if( !(barLo >= 0.0) || !(barLo < 1000000.0) )
         barLo = 0.0;
      barHi = inReal[i + nbInitialElementNeeded];
      if( !(barHi >= 0.0) || !(barHi < 1000000.0) )
         barHi = 0.0;

      v = (int)(barLo + barHi);
      v &= 1023;
      outInteger[outIdx] = v;
      outIdx++;
      i++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
