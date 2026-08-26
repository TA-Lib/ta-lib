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
 *  080326 MF,CC Creation (synthetic gate: negative (int) casts, #160)
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth2.md — one copy, so there is one thing to keep true.
 */

int synth2_lookback(int optInTimePeriod)
{
   (void)optInTimePeriod;
   return 0;
}

TA_RetCode synth2(int    startIdx,
                  int    endIdx,
                  const double inReal[],
                  int    optInTimePeriod,
                  int   *outBegIdx,
                  int   *outNBElement,
                  int    outInteger[])
{
   int outIdx, i, p, q;
   double barVal; /* deliberately NOT a float-heuristic name: pins the real_vars classifier path */

   outIdx = 0;
   for( i = startIdx; i <= endIdx; i++ )
   {
      /* NEGATIVE-CAPABLE guard: keeps |barVal*4| far inside i32 range but
       * lets negatives through (unlike synth1, which folds them away —
       * here they are the point). */
      barVal = inReal[i];
      if( !(barVal > -1000000.0) || !(barVal < 1000000.0) )
         barVal = 0.0;

      /* Magnitude use: signed clamps on a possibly-negative cast. */
      p = (int)(barVal);
      if( p < 2 )
         p = 2;
      if( p > optInTimePeriod + 2 )
         p = optInTimePeriod + 2;

      /* Bit-pattern use: sign test + two's-complement low bits. A backend
       * that saturates the negative cast to 0 never takes this branch. */
      q = (int)(barVal * 4.0);
      if( q < 0 )
         q = (q & 255) | 1024;

      outInteger[outIdx] = (p << 12) | (q & 4095);
      outIdx++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
