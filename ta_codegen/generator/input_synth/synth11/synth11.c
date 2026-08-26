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
 *  082526 MF,CC  Creation (synthetic gate: a third integer output, and a
 *                cross-typed output pair).
 *
 * SYNTHETIC GATE FUNCTION - never shipped. Exercises generator constructs
 * no real indicator uses, end to end through every backend (see
 * ta_codegen/generator/input_synth/README.md).
 *
 * Two constructs under test, both unreachable from the shipped corpus.
 *
 * 1. A THIRD INTEGER OUTPUT.
 *
 * The C harnesses -- ta_codegen_serve, ta_bench, ta_bench_stream and the
 * in-server stream_verify -- hand every function the same file-scope buffers,
 * and the counts were the literals 3 (double) and 2 (int): what the corpus
 * happens to need, MACD/BBANDS/STOCH and MINMAXINDEX. A third integer output
 * compiled to `'g_outIntBuf2' undeclared`, and nothing in the tree could reach
 * it. The counts are now derived from the corpus (`common::max_output_arity`),
 * so this fixture is what makes them move -- and what would fail if anyone
 * wrote a literal back.
 *
 * What is NOT here, deliberately: a real output beside the integer ones. The
 * output-distinctness guard could not compile a cross-typed term in three of
 * the four backends and now skips such pairs (#262, Appendix E of
 * docs/error-handling-spec.md), but that is not the only thing in the way --
 * ta_variant_frame and ta_stream_frame carry ONE outIsInteger flag per
 * function, and `test_variants.c` branches on it. Mixing output types is a
 * feature with its own assert, not a fixture away.
 *
 * 2. A `cond ? 1 : 0` STORED INTO AN INTEGER OUTPUT. Java and C# collapse that
 *    ternary to the bare condition, which is right where a boolean is wanted
 *    and wrong here: C has no booleans, so the destination is an int, and
 *    `outAbove[outIdx] = bar > 0.0;` does not compile in either language. The
 *    corpus writes `? 1 : 0` only inside helper predicates consumed by an `if`,
 *    so nothing reached the bad case. The emitters now keep the ternary on an
 *    assignment's right-hand side (#262); this is the store that proves it.
 *
 * The arithmetic is otherwise deliberately boring: the SHAPE of the output list
 * is the subject. Each output is a different function of the same bar, so a
 * store that landed in the wrong buffer changes a value the gates compare.
 *
 * No `(int)` cast anywhere. The outputs come from COMPARISONS, which are
 * IEEE-identical in all four languages, rather than from a conversion the
 * README warns is defined differently in each of them.
 */

int synth11_lookback(void)
{
   /* Each bar is read on its own; no history is consumed. */
   return 0;
}

TA_RetCode synth11(int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx, int *outNBElement,
   int outAbove[],
   int outBelow[],
   int outLarge[])
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

      /* Comparisons, not conversions: exact and language-neutral. The third
       * store owns the cursor.
       */
      outAbove[outIdx] = bar > 0.0 ? 1 : 0;
      outBelow[outIdx] = bar < 0.0 ? 1 : 0;
      outLarge[outIdx++] = bar > 1000.0 ? 1 : 0;
      i++;
   } while( i <= endIdx );

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
