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
 *  080326 MF,CC Creation (synthetic gate: bitwise operator family, #157)
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth1.md — one copy, so there is one thing to keep true.
 */

int synth1_lookback(int optInTimePeriod)
{
   return (optInTimePeriod-1);
}

TA_RetCode synth1(int    startIdx,
                  int    endIdx,
                  const double inReal[],
                  int    optInTimePeriod,
                  int   *outBegIdx,
                  int   *outNBElement,
                  int    outInteger[])
{
   int outIdx;
   int i, j, acc, v, mask, neg, parity, nbInitialElementNeeded;
   double tempReal;

   /* Default window warm-up, same idiom as MAX/MAXINDEX. */
   nbInitialElementNeeded = (optInTimePeriod-1);
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Seed path-dependent state from the parameter: ^, &, ~ (kept
    * non-negative), and a signed ~ observed through a comparison. */
   acc  = (optInTimePeriod ^ 21) & 1023;
   mask = (~optInTimePeriod) & 255;
   neg  = ~optInTimePeriod;
   if( neg < 0 )
      acc ^= 5;

   /* Fold the lookback window into the state (path dependence for the
    * streaming variant). */
   i = startIdx - nbInitialElementNeeded;
   while( i < startIdx )
   {
      /* Fold negative, non-finite and extreme bars to 0.0 BEFORE the (int)
       * cast: out-of-range and NEGATIVE double->int conversion diverge
       * across C/Rust/Java/C# (saturate vs truncate vs clamp), and the fuzz
       * corpus includes 1e9-magnitude and signed bars. The comparisons are
       * IEEE-identical in all four languages. */
      tempReal = inReal[i];
      if( !(tempReal >= 0.0) || !(tempReal < 1000000.0) )
         tempReal = 0.0;
      /* The cast must be the WHOLE right-hand side: the generator rejects
       * nested (int)-of-double forms loudly (#160), because it cannot see
       * that the guard above keeps this non-negative. */
      v = (int)(tempReal * 8.0);
      v &= 1023;
      acc ^= v;
      i++;
   }

   outIdx = 0;
   for( i = startIdx; i <= endIdx; i++ )
   {
      tempReal = inReal[i];
      if( !(tempReal >= 0.0) || !(tempReal < 1000000.0) )
         tempReal = 0.0;
      v = (int)(tempReal * 8.0);
      v &= 1023;

      /* 10-bit rotate, then mix: <<, >>, |, ^, &, and the compound forms. */
      acc = ((acc << 3) | (acc >> 7)) & 1023;
      acc ^= v;
      acc &= 1023;

      /* Parity via do-while with int truthiness and >>=. */
      parity = 0;
      j = acc;
      do
      {
         parity ^= j & 1;
         j >>= 1;
      } while( j );

      /* Switch on a bitwise expression; every arm uses a compound form. */
      switch( v & 3 )
      {
      case 0:
         acc |= 1;
         break;
      case 1:
         acc ^= 2;
         break;
      case 2:
         acc >>= 1;
         break;
      default:
         acc <<= 1;
         acc &= 1023;
         break;
      }

      /* Bitwise truthiness inside !, &&, and a ternary condition. */
      if( !(acc & 8) && (v & 4) )
         acc ^= mask;
      acc = (acc & 512) ? (acc ^ 21) : (acc | ((parity << 4) & 16));

      outInteger[outIdx] = acc;
      outIdx++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
