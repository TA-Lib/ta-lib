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
 * SYNTHETIC GATE FUNCTION - never shipped. Exercises generator constructs
 * no real indicator uses, end to end through every backend (see
 * ta_codegen/generator/input_synth/README.md).
 *
 * The construct under test is the SCIENTIFIC-NOTATION FLOAT LITERAL.
 *
 * Until this fixture existed the lexer did not accept one at all: it read the
 * 'e' as an identifier and aborted with "Expected '=' or compound assignment
 * after 'e'". Nothing in the tree noticed, because the corpus does not contain
 * one. It does contain the strings 1e-14, 1e-6 and 2e-10 -- in var.c, tema.c,
 * t3.c and marketfi.c -- but every one of them is inside a COMMENT, which the
 * lexer skips. So the number path had never been handed the token, and the
 * only thing standing between that and a contributor writing `1e-13` was luck.
 *
 * Every mantissa/exponent shape the lexer distinguishes appears below, because
 * they take different branches: a negative exponent (the one that used to
 * abort), an explicit '+', an upper-case 'E', an exponent with NO decimal point
 * (which must still produce a float -- read as an integer it would change the
 * type of the expression around it), and a leading-dot mantissa with an
 * exponent.
 *
 * 1e300 is here for the emitter, not the lexer. It is whole but too large for
 * the `.0` branch, and Rust's f64 Display never uses exponent notation, so the
 * naive rendering is 301 digits carrying neither '.' nor 'e' -- an INTEGER
 * constant in C, and past LLONG_MAX an ill-formed one. It has to come back out
 * as 1e300, and it has to do so in four languages at once.
 *
 * Real outputs rather than the integer ones this family prefers, for synth7's
 * reason: the value of the literal IS the thing under test, and an integer
 * output would quantize away exactly the difference the gate exists to see.
 * Each output also has to DEPEND on its literal -- `1e300 / (1e300 + x)` would
 * be a tidy bounded expression and a vacuous one, since it answers 1.0 whether
 * the exponent is 300 or 301.
 *
 * Magnitudes are kept small on purpose. --xlang-hash and stream_verify compare
 * bitwise and would not care, but the plain --codegen sweep crosses JSON at
 * %.15g and compares at an epsilon; an output near 1e291 would fail that on
 * transport rounding alone and say nothing about literals. Dividing BY the
 * large literal instead of multiplying by it keeps every output under ~1e3
 * while leaving the answer a strict function of the exponent.
 *
 * The mantissa multiply and the addition are separate statements so the
 * fusion detector (backends/fma.rs) sees no `a*b+c` site. FMA is covered
 * elsewhere; here it would only add a second variable to any failure.
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
