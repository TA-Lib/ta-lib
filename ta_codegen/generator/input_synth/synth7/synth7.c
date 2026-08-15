/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081426 KL   Creation (synthetic gate: ta_candlerange values, #216).
 *
 * SYNTHETIC GATE FUNCTION - never shipped. Exercises generator constructs
 * no real indicator uses, end to end through every backend (see
 * ta_codegen/generator/input_synth/README.md).
 *
 * The construct under test is ta_candlerange with its VALUE as the output.
 *
 * Why the value has to be the output. Every shipped consumer of this helper
 * is a candlestick, whose output is 3-valued (-100/0/+100). A 1-ULP shift in
 * the helper essentially never straddles a comparison boundary, so a gate
 * that hashes candlestick outputs cannot see a helper divergence: CDLLONGLINE
 * is already swept by --xlang-hash at default settings, where TA_ShadowShort
 * is Shadows-typed and the divergent arm genuinely executes, and 44M bars of
 * it produced zero output differences while the intermediates differed on
 * ~10k of 51,200 bars (#216). Making the intermediate the output is what
 * gives the existing bitwise machinery something to compare.
 *
 * This is the one synth family with real outputs rather than integer ones,
 * and deliberately so: an integer output here would reproduce exactly the
 * blindness the gate exists to remove. The fuzz corpus is entirely finite
 * (bounded by ~1e9, see fuzz_data.h), so no NaN or infinity can reach the
 * output and the bitwise comparison has no payload ambiguity to trip over.
 *
 * All three rangeType arms are emitted on EVERY bar, one per output, rather
 * than selected by a parameter -- so every swept bar covers every arm, and a
 * divergence in one arm cannot be masked by another. Arm coverage comes from
 * NAMED settings at their defaults, which already span the three:
 *
 *    outRealBodyRange   BodyLong      RealBody
 *    outHighLowRange    BodyDoji      HighLow
 *    outShadowsRange    ShadowShort   Shadows     <- the divergent arm
 *
 * (The fourth arm of the switch, the default 0.0, is unreachable through a
 * named setting: TA_SetCandleSettings rejects a rangeType above Shadows.)
 *
 * Named settings are not a stylistic choice: extract_candle_setting_name
 * (backends/c.rs) requires the first argument to be literally the variable
 * <NAME>_rangeType, because the C backend maps this helper onto the
 * TA_CANDLERANGE macro rather than emitting the helper body. A runtime
 * rangeType would emit a bare ta_candlerange(...) call that has no C
 * definition, so it fails to compile -- loud, but worth knowing.
 *
 * Nothing is clamped, because clamping is what would hide the defect: the
 * divergence lives in bars whose low sits below half their high (Sterbenz'
 * lemma makes every subtraction exact above that line, and only there), which
 * in this corpus is the negative-low bars of FUZZ_WITH_ZEROS and the
 * tiny-magnitude bars of FUZZ_EXTREME.
 *
 * EVERY READ OF A BAR HAPPENS BEFORE ANY OUTPUT IS WRITTEN, which is why the
 * three values go to locals first. outReal may alias an input array, and the
 * C backend maps these helpers onto macros that read inOpen[i]/inHigh[i]/...
 * directly -- so storing the first output before computing the second would
 * hand the second helper a clobbered bar. No shipped candlestick can hit this:
 * their outputs are int, which the alias gate never aliases onto a double
 * input. These are the first OHLC consumers with real outputs, so they are
 * also the first to be held to it.
 */

int synth7_lookback(void)
{
   /* Each bar is read on its own; no history is consumed. */
   return 0;
}

TA_RetCode synth7(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   double outRealBodyRange[],
   double outHighLowRange[],
   double outShadowsRange[])
{
   double bodyRange, highLowRange, shadowsRange;
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
      bodyRange    = ta_candlerange(BodyLong_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      highLowRange = ta_candlerange(BodyDoji_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);
      shadowsRange = ta_candlerange(ShadowShort_rangeType, inOpen[i], inHigh[i], inLow[i], inClose[i]);

      /* Bar i is fully consumed above; only now is it safe to store. */
      outRealBodyRange[outIdx] = bodyRange;
      outHighLowRange[outIdx]  = highLowRange;
      outShadowsRange[outIdx]  = shadowsRange;
      outIdx = outIdx + 1;
      i++;
   } while( i <= endIdx );

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
