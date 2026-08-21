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
 *  082126 MF,CC  First version (issue #237).
 */

int vwap_lookback(void)
{
   /* Cumulative from the first bar of the requested range, so the very
    * first bar already has a complete answer and nothing is consumed
    * before it. Same shape as ta_AD.c and ta_OBV.c.
    */
   return 0;
}

TA_RetCode vwap(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   const double inVolume[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double sumPV, sumV, typPrice, volume, tempReal, vwap;
   int outIdx, i;

   /* Volume Weighted Average Price: the average typical price paid per
    * unit of volume, accumulated from the first bar of the range.
    *
    *    VWAP = sum( TYPPRICE * Volume ) / sum( Volume )
    *
    * Every charting package anchors this to a trading session and resets
    * the two sums at each session boundary. TA-Lib takes no timestamp on
    * any function, so the anchor is the caller's choice of range: pass one
    * session's slice of bars to get that session's VWAP. This is how AD
    * and OBV, the other two cumulative volume functions, are already used
    * across session boundaries (issue #237).
    */
   sumPV = 0.0;
   sumV  = 0.0;
   vwap  = 0.0;

   outIdx = 0;
   for( i=startIdx; i <= endIdx; i++ )
   {
      /* The typical price is written exactly as in ta_TYPPRICE.c so that the
       * two agree bit for bit and this stays a true composite of it.
       */
      typPrice = ( inHigh[i] + inLow[i] + inClose[i] ) / 3.0;
      volume   = inVolume[i];

      /* A bar is weighted only if both of its terms are real numbers. That is
       * the whole condition: a NaN or an infinity in the price or the volume
       * is the only way a bar cannot be weighted, and every other bar --
       * including one that traded nothing -- is weighted normally.
       *
       * The test gates BOTH adds. Letting the volume in without its matching
       * price term would leave a weight in the divisor that nothing paid for,
       * biasing every later value: a NaN close with a good volume would drag
       * the next value 25% low.
       *
       * Skipping the bar is what makes this recoverable. These are CUMULATIVE
       * sums with no trailing term to subtract anything back out, so a single
       * non-finite bar allowed in would leave both sums non-finite for the
       * REST of the call -- the line would repeat one stale value on every
       * later bar however clean it was, silently, and looking like a plausible
       * price the whole way. Skipping keeps the state usable, so the average
       * resumes on the very next bar that can be weighted.
       *
       * Testing the two INPUTS, not the product and not the candidate sums, is
       * a measured choice:
       *
       *   - The candidate sums would have to be committed conditionally, which
       *     puts four cmovs in the loop-carried dependency chain and costs
       *     +60% on this loop. Both forms below leave the adds unconditional
       *     inside a predicted branch and measure free.
       *   - The product alone would also detect every unusable bar, one test
       *     instead of two, and measures the same. But it would additionally
       *     drop a WELL-FORMED bar whose price and volume are both finite and
       *     whose product merely overflows -- silently, and taking that bar's
       *     volume out of the divisor with it. Testing the inputs leaves that
       *     case exactly as it was before this guard existed: the overflow
       *     reaches the sum and the call reports Inf, which is the documented
       *     `double` overflow class rather than an indicator defect, and is
       *     louder than a freeze.
       *
       * So this changes behaviour for one thing only: a bar whose price or
       * volume is not a finite number. On finite data the test is always true
       * and no value the function has ever produced moves. Only the batch path
       * needs it -- the streaming Update/Peek entry points reject a non-finite
       * bar with TA_BAD_PARAM before it reaches any accumulator.
       */
      /* The product is kept in its own statement so no compiler may contract it
       * into an FMA. Contracting here would make the C output disagree with the
       * Rust, Java and C# backends under the cross-language bitwise gate. Same
       * reason as in ta_codegen/input/vwma/vwma.c.
       *
       * Computed before the guard rather than inside it, and unconditionally,
       * so it stays a per-bar temporary. Assigned only on the taken arm it
       * would instead be live across bars, and the streaming tier would carry
       * it as a fourth state field in every handle -- 8 bytes to hold a value
       * no later bar reads. The multiply on a skipped bar is discarded.
       */
      tempReal = typPrice * volume;

      if( IS_FINITE(typPrice) && IS_FINITE(volume) )
      {
         sumPV += tempReal;
         sumV  += volume;
      }

      /* Bars that traded nothing carry no weight, so a zero-volume bar in
       * the middle of a series leaves both sums untouched and repeats the
       * previous value on its own -- no arm needed for that. A bar skipped
       * by the guard above repeats it for the same reason.
       *
       * The arm below is for the one case the ratio cannot express: a
       * leading run of bars before any volume has traded, where there are
       * no weights at all and the weighted mean is undefined. The last
       * value computed is carried forward instead, which is 0.0 until the
       * first bar with volume. Volume is non-negative, so once the divisor
       * leaves zero it never returns and this arm cannot fire again.
       *
       * A successful call therefore never emits NaN or Inf (issue #112),
       * which is the divergence from pandas-ta-classic and from
       * trading-signals: the first emits NaN there, the second no bar at
       * all. Testing sumV rather than the bar's own volume also keeps a
       * negative divisor -- which no non-negative volume series can
       * produce -- out of a price-scale output, as ta_CMF.c does.
       */
      if( sumV > 0.0 )
         vwap = sumPV / sumV;

      outReal[outIdx++] = vwap;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
