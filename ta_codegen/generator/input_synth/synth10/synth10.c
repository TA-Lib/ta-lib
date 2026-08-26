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
 *  082526 MF,CC  Creation (synthetic gate: two nullable outputs).
 *
 * SYNTHETIC GATE FUNCTION - never shipped. Exercises generator constructs
 * no real indicator uses, end to end through every backend (see
 * ta_codegen/generator/input_synth/README.md).
 *
 * The construct under test is TWO NULLABLE OUTPUTS IN ONE FUNCTION.
 *
 * MAMA's outFAMA is the only nullable output in the shipped corpus, and it is
 * one of two outputs, so the whole feature has only ever been generated in its
 * simplest arrangement: one declinable output beside one required one. The
 * output-distinctness guard (issue #108) walks output PAIRS and branches on
 * which of a pair is nullable, so with one nullable output there is one pair
 * and one arm. Here there are three pairs and they cover every arm --
 * (nullable, required), (nullable, nullable) and (required, nullable) -- which
 * is why the declaration order in the .yaml is load-bearing rather than
 * cosmetic.
 *
 * What each backend has to get right, and what a wrong answer looks like:
 *
 *   C     `outX != NULL &&` on both operands of the pair guard. Two NULLs
 *         compare EQUAL, so an unguarded `outFirstOptional == outSecondOptional`
 *         rejects a caller who declined both -- the one call this feature
 *         exists to allow. With three pairs the terms also have to parenthesise
 *         (`&&` inside `||`), which one nullable output never showed.
 *   Rust  `Option<&mut [f64]>`, twice, plus the both-nullable arm of the guard.
 *   Java  `outX != null &&`, same reasoning as C.
 *   C#    an empty span IS the declination, and `Overlaps` already answers
 *         false for one -- so the pair guard needs no nullable arm at all.
 *         The thing that has to hold is the LENGTH check being skipped for
 *         both declinable outputs and for neither of the others.
 *
 * Rule B6a of docs/error-handling-spec.md; issue #262.
 *
 * The arithmetic is deliberately boring: the outputs are the subject, so the
 * body is the least that still distinguishes them. Each output is a different
 * function of the same bar, so a store that landed in the wrong buffer -- or a
 * guard that skipped the wrong one -- changes a value the gates compare.
 *
 * `outRequired` owns the `outIdx` advance and the two declinable stores carry
 * none, which is the invariant that makes guarding a store COMPLETE: the write
 * is skipped and the cursor still moves. Reversing that would make the cursor
 * conditional on a caller's choice and silently mis-place every later value.
 * mama.c was reordered for the same reason (issue #125).
 *
 * Real outputs rather than this family's usual integer ones, for two reasons.
 * MAMA's nullable output is real, so this is the mirror of the shipped case --
 * and scaling by a power of two is EXACT, so the values are bitwise identical
 * in all four backends with no `(int)` cast, which is the conversion the
 * README warns is defined differently in each of them. The bar is passed
 * through unchanged and halved and quartered; nothing here can round.
 *
 * The multiplications are separate statements from any addition, so the fusion
 * detector (backends/fma.rs) sees no `a*b+c` site. FMA is covered elsewhere;
 * here it would only add a second variable to any failure.
 */

int synth10_lookback(void)
{
   /* Each bar is read on its own; no history is consumed. */
   return 0;
}

TA_RetCode synth10(int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx, int *outNBElement,
   double outFirstOptional[],
   double outRequired[],
   double outSecondOptional[])
{
   double bar;
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
      bar = inReal[i];

      /* The two declinable stores. Neither advances outIdx. Halving and
       * quartering a double are exact, so these carry no rounding of their
       * own for a gate to attribute to the wrong thing.
       */
      outFirstOptional[outIdx] = bar * 0.5;
      outSecondOptional[outIdx] = bar * 0.25;

      /* The required store owns the advance. */
      outRequired[outIdx++] = bar;
      i++;
   } while( i <= endIdx );

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
