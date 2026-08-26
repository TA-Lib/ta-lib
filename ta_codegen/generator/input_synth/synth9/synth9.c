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
 *  081726 MF,CC  Creation (synthetic gate: scientific-notation literals).
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth9.md — one copy, so there is one thing to keep true.
 */

int synth9_lookback(void)
{
   /* Each bar is read on its own; no history is consumed. */
   return 0;
}

TA_RetCode synth9(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   double outNegExp[],
   double outPosExp[],
   double outBigExp[])
{
   double negExp, posExp, bigExp, spread;
   int i, outIdx;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   outIdx = 0;
   i = startIdx;

   do
   {
      /* Negative exponent, plain and with an upper-case E and a fractional
       * mantissa. |inClose| <= ~1e9 in the fuzz corpus, so this stays under
       * 1e-4 and cannot overflow or cancel to a non-finite value.
       */
      negExp = inClose[i] * 1e-13;
      negExp = negExp + 2.5E-3;

      /* Explicit '+' in the exponent; an exponent with no decimal point at
       * all (3e2), which must be a FLOAT and not an integer; and a
       * leading-dot mantissa carrying one (.5e1).
       */
      spread = inHigh[i] - inLow[i];
      posExp = spread / 1.0e+6;
      posExp = posExp + 3e2;
      posExp = posExp + .5e1;

      /* The emitter case. Dividing keeps the output tiny while leaving it a
       * strict function of the exponent, so a literal that came out as 1e301
       * -- or as a 301-digit integer constant that some backend then parsed
       * as something else -- shows up as a value difference here.
       */
      bigExp = fabs( inClose[i] );
      bigExp = bigExp / 1e300;

      /* Every read of bar i is done above, so the stores are safe even when
       * an output aliases an input array.
       */
      outNegExp[outIdx] = negExp;
      outPosExp[outIdx] = posExp;
      outBigExp[outIdx] = bigExp;
      outIdx = outIdx + 1;
      i++;
   } while( i <= endIdx );

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
