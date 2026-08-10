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
 * SYNTHETIC GATE FUNCTION - never shipped. Exercises generator constructs
 * no real indicator uses, end to end through every backend (see
 * ta_codegen/generator/input_synth/README.md).
 *
 * The construct under test is the explicit `<name>_private()` variant: a
 * second entry point holding the algorithm, taking an extra parameter the
 * guarded entry point pre-computes and passes down. It drives
 * has_explicit_private across the parser, the IR, all four backends, the
 * single-precision tier and the streaming emitter -- including the rule that
 * the extra parameter is derived AFTER the validation prologue has resolved
 * an integer sentinel (deriving it before yields a k computed from INT_MIN,
 * returned with TA_SUCCESS -- the TA_S_EMA defect fixed in 2e9767397).
 *
 * The state is a smoothing recursion so the extra parameter is a float
 * multiply operand, which also pins the FMA site selection: a private extra
 * param is a PARAMETER, never a body VarDecl, so `build_fma_var_sets` cannot
 * see it as Real and NO site here fuses. That is uniform across C, Rust, Java
 * and C# -- and it is the mechanism that kept TA_EMA unfused while every
 * inlined EMA cascade fused, until #183 folded the private away. A backend
 * that started fusing here on its own would break cross-language bit parity,
 * which is what the gate compares.
 *
 * Arithmetic is +,-,* only, so the state is IEEE identical in all four
 * languages, and the output is quantized to a 10-bit integer so every gate
 * compares exactly.
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
