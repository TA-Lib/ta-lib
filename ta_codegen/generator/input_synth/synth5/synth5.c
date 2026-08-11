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
 *  081126 MF,CC Creation (synthetic gate: PRAGMA TA_ALT stream alternate, #190)
 *
 * SYNTHETIC GATE FUNCTION - never shipped. Exercises generator constructs
 * no real indicator uses, end to end through every backend (see
 * ta_codegen/generator/input_synth/README.md).
 *
 * The construct under test is `PRAGMA TA_ALT={STREAM,ALL_LANGUAGES}` with the
 * alternate supplying the STREAM tier -- the orientation the six
 * rolling-extremum functions ship. The base indexes its window by the window's
 * START, so it reads `inReal[i + nbInitialElementNeeded]` ahead of the bar it
 * is emitting; that is exactly the access the stream analyzer cannot turn into
 * a per-bar automaton. Without the alternate this function does not generate
 * at all, which is what makes the gate non-vacuous: the alternate is not a
 * nicer spelling of something that already worked.
 *
 * synth5_ALT1 walks the same window with a trailing cursor instead, so it is an
 * ordinary T3 ring, and it adds the two bars in the SAME order -- the two are
 * bit-identical, not merely close, which is the contract an alternate owes its
 * base.
 *
 * Arithmetic is +,- only and the output is quantized to a 10-bit integer, so
 * every gate compares exactly.
 */

int synth5_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode synth5(int    startIdx,
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

   /* Indexed by the window's START. `inReal[i + nbInitialElementNeeded]` reads
    * ahead of the emitted bar, so no per-bar automaton can be derived from
    * this body; the streaming tier runs synth5_ALT1 below.
    *
    * Bars are clamped to [0,1000000) BEFORE any arithmetic: the fuzz corpus
    * carries non-finite, negative and 1e9-magnitude bars, and only a bounded
    * non-negative sum keeps the (int) cast identical across C, Rust, Java
    * and C#.
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

      /* The cast must be the WHOLE right-hand side (#160). The clamps above
       * keep the sum in [0,2000000), so the truncation is well defined. */
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

/* PRAGMA TA_ALT={STREAM,ALL_LANGUAGES} the base reads ahead of the emitted bar */
TA_RetCode synth5_ALT1(int    startIdx,
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

   /* Same window, same addition order, walked with a trailing cursor: an
    * ordinary T3 ring over inReal, which is what the streaming tier needs.
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
