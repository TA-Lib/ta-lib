/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090626 KL     First version (issue #349).
 */

   /**
    * Number of leading input bars {@link Core#VORTEX} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the rolling sums (default 14;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int VORTEX_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* Every per-bar term (TR, |H-prevL|, |L-prevH|) needs the prior bar, so
       * bar 0 is consumed exactly as TA_TRANGE consumes it, and the window then
       * needs optInTimePeriod terms: 1 + (optInTimePeriod - 1) = optInTimePeriod.
       * First valid output index is n, not n-1.
       */
      return optInTimePeriod ;

   }
   RetCode VORTEX_Impl( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outPlusVI[],
                        double outMinusVI[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int nullRun = 0;
      double sTR = 0;
      double sVMP = 0;
      double sVMM = 0;
      double curTR = 0;
      double curVMP = 0;
      double curVMM = 0;
      double trueRange = 0;
      double tempDouble = 0;
      double tempLT = 0;
      double tempHT = 0;
      double tempCY = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outPlusVI == outMinusVI ) {
         return RetCode.BadParam ;
      }
      /* Vortex Indicator (Botes & Siepman, TASC 28:1, Jan 2010): two lines,
       * each a rolling sum of "vortex movement" normalized by the rolling sum
       * of true range over the same optInTimePeriod bars.
       *
       *   TR[i]  = max( H[i]-L[i], |C[i-1]-H[i]|, |C[i-1]-L[i]| )   == TA_TRANGE
       *   VMP[i] = |H[i] - L[i-1]|
       *   VMM[i] = |L[i] - H[i-1]|
       *   +VI = SUM(VMP, n) / SUM(TR, n),  -VI = SUM(VMM, n) / SUM(TR, n)
       *
       * No smoothing, no recursion, nothing to seed. The TR expansion below is
       * TA_TRANGE's own operation order, bit for bit -- the differential test
       * composes TA_TRANGE + TA_SUM and asserts equality with memcmp.
       *
       * The trailing terms are recomputed from the inputs rather than carried
       * in a ring; the subtraction re-reads bars trailingIdx and trailingIdx-1,
       * both of which sit at or ahead of the output slot, which is why the
       * outputs are written LAST (see the loop comment).
       */
      lookbackTotal = VORTEX_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Prime the three window sums over the optInTimePeriod-1 terms before the
       * first output bar: [startIdx-optInTimePeriod+1, startIdx). Each term at
       * bar i reads bar i-1, so the earliest read is bar startIdx-optInTimePeriod
       * >= 0.
       */
      sTR = 0.0;
      sVMP = 0.0;
      sVMM = 0.0;
      nullRun = 0;
      for( i = startIdx - optInTimePeriod + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR += trueRange;
         sVMP += Math.abs(inHigh[i] - inLow[i - 1]);
         sVMM += Math.abs(inLow[i] - inHigh[i - 1]);
         if( trueRange == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod + 1;
      while( today <= endIdx ) {
         /* Add on today's terms. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR += trueRange;
         sVMP += Math.abs(inHigh[today] - inLow[today - 1]);
         sVMM += Math.abs(inLow[today] - inHigh[today - 1]);
         if( trueRange == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         /* Once the whole window is flat, every TRUE-RANGE term in it is
          * provably zero (nullRun counts exactly that), so sTR's only content
          * is the running add/subtract's rounding residue -- purge it, and the
          * exact division gate below recognizes the case with no absolute
          * band. ONLY sTR: a zero true range (H == L == prevClose) does NOT
          * zero that bar's vortex terms, which read the PREVIOUS bar's
          * extremes -- a spread bar followed by a halt leaves |H - prevL| and
          * |L - prevH| alive inside the window, and zeroing the numerator sums
          * would poison both lines permanently (an unreachable negative -VI).
          * ULTOSC can reseed all its totals because its predicate covers both
          * of its per-bar terms; VORTEX's covers only the denominator's.
          */
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            sTR = 0.0;
         }
         /* Record the current window sums, then retire the trailing bar's
          * terms so the sums are ready for the next iteration.
          */
         curTR = sTR;
         curVMP = sVMP;
         curVMM = sVMM;
         tempLT = inLow[trailingIdx];
         tempHT = inHigh[trailingIdx];
         tempCY = inClose[trailingIdx - 1];
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR -= trueRange;
         sVMP -= Math.abs(inHigh[trailingIdx] - inLow[trailingIdx - 1]);
         sVMM -= Math.abs(inLow[trailingIdx] - inHigh[trailingIdx - 1]);
         trailingIdx += 1;
         /* Last operation is to write the outputs. Must be done after the
          * trailing bar has been fully consumed: the caller is allowed to pass
          * an output buffer aliasing any input, and the trailing reads above
          * touch bars trailingIdx-1 == outIdx and trailingIdx == outIdx+1 --
          * an emit-first order would have clobbered them (ULTOSC's own rule;
          * ACCBANDS' multi-output form).
          *
          * Zero-denominator gate, on the DENOMINATOR itself and exact: the
          * flat-run reseed above removes the running sums' residue, so
          * `curTR > 0.0` is a precise test -- ULTOSC gates its divisions the
          * same way after the same reseed. The flat-bar count alone is only a
          * proxy for sTR == 0 in exact arithmetic: floating-point absorption
          * can zero the running sum while the window still holds a live term
          * (a large spread swallows a 1-ULP one; the later subtract leaves
          * exactly 0.0 with nullRun far below n), and an ungated division
          * then emits NaN/Inf, which VORTEX does not declare. An absolute
          * TA_IS_ZERO band is no better: it zeroes legitimate ratios on any
          * instrument quoted below 1e-14, and the QUOTE-UNIT/SCALE gate
          * rejects it (VORTEX is homogeneous of degree 0).
          */
         if( curTR > 0.0 ) {
            outPlusVI[outIdx] = curVMP / curTR;
            outMinusVI[outIdx] = curVMM / curTR;
         } else {
            outPlusVI[outIdx] = 0.0;
            outMinusVI[outIdx] = 0.0;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode VORTEX_Impl( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outPlusVI[],
                        double outMinusVI[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int nullRun = 0;
      double sTR = 0;
      double sVMP = 0;
      double sVMM = 0;
      double curTR = 0;
      double curVMP = 0;
      double curVMM = 0;
      double trueRange = 0;
      double tempDouble = 0;
      double tempLT = 0;
      double tempHT = 0;
      double tempCY = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outPlusVI == outMinusVI ) {
         return RetCode.BadParam ;
      }
      lookbackTotal = VORTEX_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sTR = 0.0;
      sVMP = 0.0;
      sVMM = 0.0;
      nullRun = 0;
      for( i = startIdx - optInTimePeriod + 1; i < startIdx; i += 1 ) {
         tempLT = (double)inLow[i];
         tempHT = (double)inHigh[i];
         tempCY = (double)inClose[i - 1];
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR += trueRange;
         sVMP += Math.abs((double)inHigh[i] - (double)inLow[i - 1]);
         sVMM += Math.abs((double)inLow[i] - (double)inHigh[i - 1]);
         if( trueRange == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod + 1;
      while( today <= endIdx ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR += trueRange;
         sVMP += Math.abs((double)inHigh[today] - (double)inLow[today - 1]);
         sVMM += Math.abs((double)inLow[today] - (double)inHigh[today - 1]);
         if( trueRange == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            sTR = 0.0;
         }
         curTR = sTR;
         curVMP = sVMP;
         curVMM = sVMM;
         tempLT = (double)inLow[trailingIdx];
         tempHT = (double)inHigh[trailingIdx];
         tempCY = (double)inClose[trailingIdx - 1];
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR -= trueRange;
         sVMP -= Math.abs((double)inHigh[trailingIdx] - (double)inLow[trailingIdx - 1]);
         sVMM -= Math.abs((double)inLow[trailingIdx] - (double)inHigh[trailingIdx - 1]);
         trailingIdx += 1;
         if( curTR > 0.0 ) {
            outPlusVI[outIdx] = curVMP / curTR;
            outMinusVI[outIdx] = curVMM / curTR;
         } else {
            outPlusVI[outIdx] = 0.0;
            outMinusVI[outIdx] = 0.0;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Vortex Indicator: Etienne Botes and Douglas Siepman's two-line trend
    * indicator (*Technical Analysis of Stocks &amp; Commodities* 28:1, January
    * 2010). Positive and negative "vortex movement" — the reach from today's
    * high to yesterday's low and from today's low to yesterday's high — each
    * summed over the period and normalized by the summed true range. A +VI line
    * crossing above −VI is the bullish signal the authors describe; the two
    * lines are conventionally plotted together.
    * <p><b>Formula</b>
    * <pre>{@code
    * Per bar, `TR[i] = max(H[i]−L[i], |C[i−1]−H[i]|, |C[i−1]−L[i]|)` (exactly [`TRANGE`](/functions/trange)), `VMP[i] = |H[i] − L[i−1]|` and `VMM[i] = |L[i] − H[i−1]|`. Then `+VI = SUM(VMP, n) / SUM(TR, n)` and `−VI = SUM(VMM, n) / SUM(TR, n)`.
    * No smoothing, no recursion, no seeding — three rolling sums over per-bar terms. Every source (the original TASC article, StockCharts, Wikipedia, TradingView) states the identical formula; the only cross-source difference is the suggested period (14 vs Wikipedia's worked 21). A window whose every bar is flat sums the true range to zero; both lines then emit 0.0, the convention the external implementations share.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Bar 0 has no term (all three need a prior bar) and is consumed exactly as [{@code TRANGE}](/functions/trange) consumes it, so the first output sits at index {@code optInTimePeriod}, not {@code optInTimePeriod − 1}.</li>
    * <li>Not start-dependent: each output depends only on the finite trailing window. No unstable period.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#VORTEX_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param optInTimePeriod Number of bars in the rolling sums (default 14;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outPlusVI Positive vortex line (+VI) Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMinusVI Negative vortex line (−VI) Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#TRANGE
    * @see Core#PLUS_DI
    * @see Core#MINUS_DI
    * @see Core#ADX
    */
   public OutRange VORTEX( int startIdx,
                           int endIdx,
                           double inHigh[],
                           double inLow[],
                           double inClose[],
                           int optInTimePeriod,
                           double outPlusVI[],
                           double outMinusVI[] )
   {
      requireIndexRange("VORTEX", startIdx, endIdx);
      int guardStart = clampedStart("VORTEX", startIdx, VORTEX_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VORTEX", "inHigh", inHigh, guardInLen);
      requireLength("VORTEX", "inLow", inLow, guardInLen);
      requireLength("VORTEX", "inClose", inClose, guardInLen);
      requireLength("VORTEX", "outPlusVI", outPlusVI, guardOutLen);
      requireLength("VORTEX", "outMinusVI", outMinusVI, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VORTEX_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outPlusVI, outMinusVI);
      if( retCode != RetCode.Success ) {
         throw failure("VORTEX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Vortex Indicator: Etienne Botes and Douglas Siepman's two-line trend
    * indicator (*Technical Analysis of Stocks &amp; Commodities* 28:1, January
    * 2010). Positive and negative "vortex movement" — the reach from today's
    * high to yesterday's low and from today's low to yesterday's high — each
    * summed over the period and normalized by the summed true range. A +VI line
    * crossing above −VI is the bullish signal the authors describe; the two
    * lines are conventionally plotted together.
    * <p><b>Formula</b>
    * <pre>{@code
    * Per bar, `TR[i] = max(H[i]−L[i], |C[i−1]−H[i]|, |C[i−1]−L[i]|)` (exactly [`TRANGE`](/functions/trange)), `VMP[i] = |H[i] − L[i−1]|` and `VMM[i] = |L[i] − H[i−1]|`. Then `+VI = SUM(VMP, n) / SUM(TR, n)` and `−VI = SUM(VMM, n) / SUM(TR, n)`.
    * No smoothing, no recursion, no seeding — three rolling sums over per-bar terms. Every source (the original TASC article, StockCharts, Wikipedia, TradingView) states the identical formula; the only cross-source difference is the suggested period (14 vs Wikipedia's worked 21). A window whose every bar is flat sums the true range to zero; both lines then emit 0.0, the convention the external implementations share.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Bar 0 has no term (all three need a prior bar) and is consumed exactly as [{@code TRANGE}](/functions/trange) consumes it, so the first output sits at index {@code optInTimePeriod}, not {@code optInTimePeriod − 1}.</li>
    * <li>Not start-dependent: each output depends only on the finite trailing window. No unstable period.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#VORTEX_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param optInTimePeriod Number of bars in the rolling sums (default 14;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outPlusVI Positive vortex line (+VI) Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMinusVI Negative vortex line (−VI) Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#TRANGE
    * @see Core#PLUS_DI
    * @see Core#MINUS_DI
    * @see Core#ADX
    */
   public OutRange VORTEX( int startIdx,
                           int endIdx,
                           float inHigh[],
                           float inLow[],
                           float inClose[],
                           int optInTimePeriod,
                           double outPlusVI[],
                           double outMinusVI[] )
   {
      requireIndexRange("VORTEX", startIdx, endIdx);
      int guardStart = clampedStart("VORTEX", startIdx, VORTEX_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VORTEX", "inHigh", inHigh, guardInLen);
      requireLength("VORTEX", "inLow", inLow, guardInLen);
      requireLength("VORTEX", "inClose", inClose, guardInLen);
      requireLength("VORTEX", "outPlusVI", outPlusVI, guardOutLen);
      requireLength("VORTEX", "outMinusVI", outMinusVI, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VORTEX_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outPlusVI, outMinusVI);
      if( retCode != RetCode.Success ) {
         throw failure("VORTEX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live VORTEX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#VORTEX} over the same series.
    * Open with {@link Core#vortexOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class VortexStream {
      Core core;
      int optInTimePeriod;
      int nullRun;
      double sTR;
      double sVMP;
      double sVMM;
      double lag1_inHigh;
      double lag1_inLow;
      double lag1_inClose;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      int ringLag_trailingIdx;
      double[] ring_trailingIdx_inHigh;
      double[] ring_trailingIdx_inLow;
      double[] ring_trailingIdx_inClose;
      double cur_outPlusVI;
      double cur_outMinusVI;
      int outRangeBegIdx;
      int outRangeCount;

      VortexStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#VORTEX} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      VortexStream( VortexStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.nullRun = other.nullRun;
         this.sTR = other.sTR;
         this.sVMP = other.sVMP;
         this.sVMM = other.sVMM;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ringLag_trailingIdx = other.ringLag_trailingIdx;
         this.ring_trailingIdx_inHigh = other.ring_trailingIdx_inHigh.clone();
         this.ring_trailingIdx_inLow = other.ring_trailingIdx_inLow.clone();
         this.ring_trailingIdx_inClose = other.ring_trailingIdx_inClose.clone();
         this.cur_outPlusVI = other.cur_outPlusVI;
         this.cur_outMinusVI = other.cur_outMinusVI;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(VortexOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inHigh, double inLow, double inClose, VortexOut out ) {
         requireArgument("VORTEX update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("VORTEX update: BadParam", RetCode.BadParam);
         }
         core.vortexStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.plusVI = this.cur_outPlusVI;
         out.minusVI = this.cur_outMinusVI;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outPlusVI[], double outMinusVI[] ) {
         requireArgument("VORTEX updateAndFill", "inHigh", inHigh);
         requireArgument("VORTEX updateAndFill", "inLow", inLow);
         requireArgument("VORTEX updateAndFill", "inClose", inClose);
         requireArgument("VORTEX updateAndFill", "outPlusVI", outPlusVI);
         requireArgument("VORTEX updateAndFill", "outMinusVI", outMinusVI);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outPlusVI.length < barCount || outMinusVI.length < barCount || (Object)outPlusVI == (Object)inHigh || (Object)outPlusVI == (Object)inLow || (Object)outPlusVI == (Object)inClose || (Object)outMinusVI == (Object)inHigh || (Object)outMinusVI == (Object)inLow || (Object)outMinusVI == (Object)inClose || (Object)outPlusVI == (Object)outMinusVI )
            throw new TaLibArgumentException("VORTEX updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("VORTEX updateAndFill: BadParam", RetCode.BadParam);
            }
            core.vortexStepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outPlusVI[i] = this.cur_outPlusVI;
            outMinusVI[i] = this.cur_outMinusVI;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public void peek( double inHigh, double inLow, double inClose, VortexOut out ) {
         requireArgument("VORTEX peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("VORTEX peek: BadParam", RetCode.BadParam);
         VortexStream sp = this;
         double curTR = 0.0;
         double curVMP = 0.0;
         double curVMM = 0.0;
         double trueRange = 0.0;
         double tempDouble = 0.0;
         double tempLT = 0.0;
         double tempHT = 0.0;
         double tempCY = 0.0;
         double cur_outMinusVI = 0.0;
         double cur_outPlusVI = 0.0;
         int nullRun = sp.nullRun;
         double sTR = sp.sTR;
         double sVMM = sp.sVMM;
         double sVMP = sp.sVMP;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         int pkSlot2 = -1;
         double pkVal2 = 0.0;
         pkSlot0 = sp.ringPos_trailingIdx;
         pkVal0 = inHigh;
         pkSlot1 = sp.ringPos_trailingIdx;
         pkVal1 = inLow;
         pkSlot2 = sp.ringPos_trailingIdx;
         pkVal2 = inClose;
         /* Add on today's terms. */
         tempLT = inLow;
         tempHT = inHigh;
         tempCY = sp.lag1_inClose;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR += trueRange;
         sVMP += Math.abs(inHigh - sp.lag1_inLow);
         sVMM += Math.abs(inLow - sp.lag1_inHigh);
         if( trueRange == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         /* Once the whole window is flat, every TRUE-RANGE term in it is
          * provably zero (nullRun counts exactly that), so sTR's only content
          * is the running add/subtract's rounding residue -- purge it, and the
          * exact division gate below recognizes the case with no absolute
          * band. ONLY sTR: a zero true range (H == L == prevClose) does NOT
          * zero that bar's vortex terms, which read the PREVIOUS bar's
          * extremes -- a spread bar followed by a halt leaves |H - prevL| and
          * |L - prevH| alive inside the window, and zeroing the numerator sums
          * would poison both lines permanently (an unreachable negative -VI).
          * ULTOSC can reseed all its totals because its predicate covers both
          * of its per-bar terms; VORTEX's covers only the denominator's.
          */
         if( nullRun >= sp.optInTimePeriod ) {
            nullRun = sp.optInTimePeriod;
            sTR = 0.0;
         }
         /* Record the current window sums, then retire the trailing bar's
          * terms so the sums are ready for the next iteration.
          */
         curTR = sTR;
         curVMP = sVMP;
         curVMM = sVMM;
         tempLT = ((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inLow[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx] : pkVal1;
         tempHT = ((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inHigh[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx] : pkVal0;
         tempCY = ((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx != pkSlot2) ? sp.ring_trailingIdx_inClose[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx] : pkVal2;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR -= trueRange;
         sVMP -= Math.abs((((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inHigh[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx] : pkVal0) - (((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inLow[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx] : pkVal1));
         sVMM -= Math.abs((((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inLow[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx] : pkVal1) - (((sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inHigh[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx] : pkVal0));
         /* Last operation is to write the outputs. Must be done after the
          * trailing bar has been fully consumed: the caller is allowed to pass
          * an output buffer aliasing any input, and the trailing reads above
          * touch bars trailingIdx-1 == outIdx and trailingIdx == outIdx+1 --
          * an emit-first order would have clobbered them (ULTOSC's own rule;
          * ACCBANDS' multi-output form).
          *
          * Zero-denominator gate, on the DENOMINATOR itself and exact: the
          * flat-run reseed above removes the running sums' residue, so
          * `curTR > 0.0` is a precise test -- ULTOSC gates its divisions the
          * same way after the same reseed. The flat-bar count alone is only a
          * proxy for sTR == 0 in exact arithmetic: floating-point absorption
          * can zero the running sum while the window still holds a live term
          * (a large spread swallows a 1-ULP one; the later subtract leaves
          * exactly 0.0 with nullRun far below n), and an ungated division
          * then emits NaN/Inf, which VORTEX does not declare. An absolute
          * TA_IS_ZERO band is no better: it zeroes legitimate ratios on any
          * instrument quoted below 1e-14, and the QUOTE-UNIT/SCALE gate
          * rejects it (VORTEX is homogeneous of degree 0).
          */
         if( curTR > 0.0 ) {
            cur_outPlusVI = curVMP / curTR;
            cur_outMinusVI = curVMM / curTR;
         } else {
            cur_outPlusVI = 0.0;
            cur_outMinusVI = 0.0;
         }
         out.plusVI = cur_outPlusVI;
         out.minusVI = cur_outMinusVI;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( VortexOut out ) {
         requireArgument("VORTEX value", "out", out);
         out.plusVI = this.cur_outPlusVI;
         out.minusVI = this.cur_outMinusVI;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public VortexStream clone() {
         return new VortexStream(this);
      }
   }

   /**
    * The outputs of one VORTEX bar, written by the stream into an object the
    * CALLER owns. Allocate one and reuse it: {@code update}, {@code peek}
    * and {@code value} overwrite its fields, so the sink itself costs
    * nothing per bar.
    *
    * <p><b>Its contents are only valid until the next call that writes it.</b>
    * It is a mutable buffer, not a reading: a reference kept past that call,
    * or one put in a collection, sees the value change underneath it. Copy the
    * fields out if the reading has to outlive the call.
    *
    * <p>Deliberately no {@code equals} or {@code hashCode}: a mutable type
    * with value equality breaks the {@code HashMap}/{@code HashSet}
    * invariant the moment a reused instance becomes a key. Compare the fields.
    */
   public static final class VortexOut {
      /** Positive vortex line (+VI) */
      public double plusVI;
      /** Negative vortex line (−VI) */
      public double minusVI;
   }
   void vortexStepImpl( VortexStream sp, double inHigh, double inLow, double inClose )
   {
      double curTR = 0.0;
      double curVMP = 0.0;
      double curVMM = 0.0;
      double trueRange = 0.0;
      double tempDouble = 0.0;
      double tempLT = 0.0;
      double tempHT = 0.0;
      double tempCY = 0.0;
      sp.ring_trailingIdx_inHigh[sp.ringPos_trailingIdx] = inHigh;
      sp.ring_trailingIdx_inLow[sp.ringPos_trailingIdx] = inLow;
      sp.ring_trailingIdx_inClose[sp.ringPos_trailingIdx] = inClose;
      /* Add on today's terms. */
      tempLT = inLow;
      tempHT = inHigh;
      tempCY = sp.lag1_inClose;
      trueRange = tempHT - tempLT;
      tempDouble = Math.abs(tempCY - tempHT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      tempDouble = Math.abs(tempCY - tempLT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      sp.sTR += trueRange;
      sp.sVMP += Math.abs(inHigh - sp.lag1_inLow);
      sp.sVMM += Math.abs(inLow - sp.lag1_inHigh);
      if( trueRange == 0.0 ) {
         sp.nullRun += 1;
      } else {
         sp.nullRun = 0;
      }
      /* Once the whole window is flat, every TRUE-RANGE term in it is
       * provably zero (nullRun counts exactly that), so sTR's only content
       * is the running add/subtract's rounding residue -- purge it, and the
       * exact division gate below recognizes the case with no absolute
       * band. ONLY sTR: a zero true range (H == L == prevClose) does NOT
       * zero that bar's vortex terms, which read the PREVIOUS bar's
       * extremes -- a spread bar followed by a halt leaves |H - prevL| and
       * |L - prevH| alive inside the window, and zeroing the numerator sums
       * would poison both lines permanently (an unreachable negative -VI).
       * ULTOSC can reseed all its totals because its predicate covers both
       * of its per-bar terms; VORTEX's covers only the denominator's.
       */
      if( sp.nullRun >= sp.optInTimePeriod ) {
         sp.nullRun = sp.optInTimePeriod;
         sp.sTR = 0.0;
      }
      /* Record the current window sums, then retire the trailing bar's
       * terms so the sums are ready for the next iteration.
       */
      curTR = sp.sTR;
      curVMP = sp.sVMP;
      curVMM = sp.sVMM;
      tempLT = sp.ring_trailingIdx_inLow[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx];
      tempHT = sp.ring_trailingIdx_inHigh[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx];
      tempCY = sp.ring_trailingIdx_inClose[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx];
      trueRange = tempHT - tempLT;
      tempDouble = Math.abs(tempCY - tempHT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      tempDouble = Math.abs(tempCY - tempLT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      sp.sTR -= trueRange;
      sp.sVMP -= Math.abs(sp.ring_trailingIdx_inHigh[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx] - sp.ring_trailingIdx_inLow[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx]);
      sp.sVMM -= Math.abs(sp.ring_trailingIdx_inLow[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx) % sp.ringCap_trailingIdx] - sp.ring_trailingIdx_inHigh[(sp.ringPos_trailingIdx + sp.ringCap_trailingIdx - sp.ringLag_trailingIdx - 1) % sp.ringCap_trailingIdx]);
      /* Last operation is to write the outputs. Must be done after the
       * trailing bar has been fully consumed: the caller is allowed to pass
       * an output buffer aliasing any input, and the trailing reads above
       * touch bars trailingIdx-1 == outIdx and trailingIdx == outIdx+1 --
       * an emit-first order would have clobbered them (ULTOSC's own rule;
       * ACCBANDS' multi-output form).
       *
       * Zero-denominator gate, on the DENOMINATOR itself and exact: the
       * flat-run reseed above removes the running sums' residue, so
       * `curTR > 0.0` is a precise test -- ULTOSC gates its divisions the
       * same way after the same reseed. The flat-bar count alone is only a
       * proxy for sTR == 0 in exact arithmetic: floating-point absorption
       * can zero the running sum while the window still holds a live term
       * (a large spread swallows a 1-ULP one; the later subtract leaves
       * exactly 0.0 with nullRun far below n), and an ungated division
       * then emits NaN/Inf, which VORTEX does not declare. An absolute
       * TA_IS_ZERO band is no better: it zeroes legitimate ratios on any
       * instrument quoted below 1e-14, and the QUOTE-UNIT/SCALE gate
       * rejects it (VORTEX is homogeneous of degree 0).
       */
      if( curTR > 0.0 ) {
         sp.cur_outPlusVI = curVMP / curTR;
         sp.cur_outMinusVI = curVMM / curTR;
      } else {
         sp.cur_outPlusVI = 0.0;
         sp.cur_outMinusVI = 0.0;
      }
      sp.lag1_inHigh = inHigh;
      sp.lag1_inLow = inLow;
      sp.lag1_inClose = inClose;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode vortexOpenImpl( VortexStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outPlusVI[], double outMinusVI[], int outStride )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int nullRun = 0;
      double sTR = 0;
      double sVMP = 0;
      double sVMM = 0;
      double curTR = 0;
      double curVMP = 0;
      double curVMM = 0;
      double trueRange = 0;
      double tempDouble = 0;
      double tempLT = 0;
      double tempHT = 0;
      double tempCY = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Vortex Indicator (Botes & Siepman, TASC 28:1, Jan 2010): two lines,
       * each a rolling sum of "vortex movement" normalized by the rolling sum
       * of true range over the same optInTimePeriod bars.
       *
       *   TR[i]  = max( H[i]-L[i], |C[i-1]-H[i]|, |C[i-1]-L[i]| )   == TA_TRANGE
       *   VMP[i] = |H[i] - L[i-1]|
       *   VMM[i] = |L[i] - H[i-1]|
       *   +VI = SUM(VMP, n) / SUM(TR, n),  -VI = SUM(VMM, n) / SUM(TR, n)
       *
       * No smoothing, no recursion, nothing to seed. The TR expansion below is
       * TA_TRANGE's own operation order, bit for bit -- the differential test
       * composes TA_TRANGE + TA_SUM and asserts equality with memcmp.
       *
       * The trailing terms are recomputed from the inputs rather than carried
       * in a ring; the subtraction re-reads bars trailingIdx and trailingIdx-1,
       * both of which sit at or ahead of the output slot, which is why the
       * outputs are written LAST (see the loop comment).
       */
      lookbackTotal = VORTEX_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Prime the three window sums over the optInTimePeriod-1 terms before the
       * first output bar: [startIdx-optInTimePeriod+1, startIdx). Each term at
       * bar i reads bar i-1, so the earliest read is bar startIdx-optInTimePeriod
       * >= 0.
       */
      sTR = 0.0;
      sVMP = 0.0;
      sVMM = 0.0;
      nullRun = 0;
      for( i = startIdx - optInTimePeriod + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR += trueRange;
         sVMP += Math.abs(inHigh[i] - inLow[i - 1]);
         sVMM += Math.abs(inLow[i] - inHigh[i - 1]);
         if( trueRange == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod + 1;
      while( today <= endIdx ) {
         /* Add on today's terms. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR += trueRange;
         sVMP += Math.abs(inHigh[today] - inLow[today - 1]);
         sVMM += Math.abs(inLow[today] - inHigh[today - 1]);
         if( trueRange == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         /* Once the whole window is flat, every TRUE-RANGE term in it is
          * provably zero (nullRun counts exactly that), so sTR's only content
          * is the running add/subtract's rounding residue -- purge it, and the
          * exact division gate below recognizes the case with no absolute
          * band. ONLY sTR: a zero true range (H == L == prevClose) does NOT
          * zero that bar's vortex terms, which read the PREVIOUS bar's
          * extremes -- a spread bar followed by a halt leaves |H - prevL| and
          * |L - prevH| alive inside the window, and zeroing the numerator sums
          * would poison both lines permanently (an unreachable negative -VI).
          * ULTOSC can reseed all its totals because its predicate covers both
          * of its per-bar terms; VORTEX's covers only the denominator's.
          */
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            sTR = 0.0;
         }
         /* Record the current window sums, then retire the trailing bar's
          * terms so the sums are ready for the next iteration.
          */
         curTR = sTR;
         curVMP = sVMP;
         curVMM = sVMM;
         tempLT = inLow[trailingIdx];
         tempHT = inHigh[trailingIdx];
         tempCY = inClose[trailingIdx - 1];
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         sTR -= trueRange;
         sVMP -= Math.abs(inHigh[trailingIdx] - inLow[trailingIdx - 1]);
         sVMM -= Math.abs(inLow[trailingIdx] - inHigh[trailingIdx - 1]);
         trailingIdx += 1;
         /* Last operation is to write the outputs. Must be done after the
          * trailing bar has been fully consumed: the caller is allowed to pass
          * an output buffer aliasing any input, and the trailing reads above
          * touch bars trailingIdx-1 == outIdx and trailingIdx == outIdx+1 --
          * an emit-first order would have clobbered them (ULTOSC's own rule;
          * ACCBANDS' multi-output form).
          *
          * Zero-denominator gate, on the DENOMINATOR itself and exact: the
          * flat-run reseed above removes the running sums' residue, so
          * `curTR > 0.0` is a precise test -- ULTOSC gates its divisions the
          * same way after the same reseed. The flat-bar count alone is only a
          * proxy for sTR == 0 in exact arithmetic: floating-point absorption
          * can zero the running sum while the window still holds a live term
          * (a large spread swallows a 1-ULP one; the later subtract leaves
          * exactly 0.0 with nullRun far below n), and an ungated division
          * then emits NaN/Inf, which VORTEX does not declare. An absolute
          * TA_IS_ZERO band is no better: it zeroes legitimate ratios on any
          * instrument quoted below 1e-14, and the QUOTE-UNIT/SCALE gate
          * rejects it (VORTEX is homogeneous of degree 0).
          */
         if( curTR > 0.0 ) {
            outPlusVI[outIdx * outStride] = curVMP / curTR;
            outMinusVI[outIdx * outStride] = curVMM / curTR;
         } else {
            outPlusVI[outIdx * outStride] = 0.0;
            outMinusVI[outIdx * outStride] = 0.0;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capLag_trailingIdx = today - trailingIdx;
      int cap_trailingIdx = capLag_trailingIdx + 2;
      if( capLag_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inHigh = new double[allocN_trailingIdx];
      for( int fillJ = historyLen - cap_trailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx_inHigh[fillJ % cap_trailingIdx] = inHigh[fillJ];
      }
      double[] capRing_trailingIdx_inLow = new double[allocN_trailingIdx];
      for( int fillJ = historyLen - cap_trailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx_inLow[fillJ % cap_trailingIdx] = inLow[fillJ];
      }
      double[] capRing_trailingIdx_inClose = new double[allocN_trailingIdx];
      for( int fillJ = historyLen - cap_trailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx_inClose[fillJ % cap_trailingIdx] = inClose[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.nullRun = nullRun;
      sp.sTR = sTR;
      sp.sVMP = sVMP;
      sp.sVMM = sVMM;
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_trailingIdx = historyLen % cap_trailingIdx;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ringLag_trailingIdx = capLag_trailingIdx;
      sp.ring_trailingIdx_inHigh = capRing_trailingIdx_inHigh;
      sp.ring_trailingIdx_inLow = capRing_trailingIdx_inLow;
      sp.ring_trailingIdx_inClose = capRing_trailingIdx_inClose;
      sp.cur_outPlusVI = outPlusVI[(outNBElement.value - 1) * outStride];
      sp.cur_outMinusVI = outMinusVI[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* vortexOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   VortexStream vortexOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outPlusVI[], double outMinusVI[] )
   {
      VortexStream sp = new VortexStream(this);
      RetCode retCode = vortexOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outPlusVI, outMinusVI, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VORTEX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VORTEX openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("VORTEX openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind vortexOpen (composition seam). */
   VortexStream vortexOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      VortexStream sp = new VortexStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outPlusVI = new double[1];
      double[] sink_outMinusVI = new double[1];
      RetCode retCode = vortexOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outPlusVI, sink_outMinusVI, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VORTEX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VORTEX open: internal error", retCode);
      }
      throw new TaLibArgumentException("VORTEX open: " + retCode, retCode);
   }
   /**
    * Open a live VORTEX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#VORTEX} at that bar.
    * <p>The history must hold at least {@code VORTEX_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public VortexStream vortexOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      requireArgument("VORTEX open", "inHigh", inHigh);
      requireHistory("VORTEX open", inHigh.length);
      requireArgument("VORTEX open", "inLow", inLow);
      requireArgument("VORTEX open", "inClose", inClose);
      requireHistoryLength("VORTEX open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("VORTEX open", "inClose", inClose.length, inHigh.length);
      return vortexOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#vortexOpen} that also fills the output array(s) bit-identically
    * to {@link Core#VORTEX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link VortexStream#outRange()}.
    */
   public VortexStream vortexOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outPlusVI[], double outMinusVI[] )
   {
      requireArgument("VORTEX openAndFill", "inHigh", inHigh);
      requireHistory("VORTEX openAndFill", inHigh.length);
      requireArgument("VORTEX openAndFill", "inLow", inLow);
      requireArgument("VORTEX openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("VORTEX openAndFill", inHigh.length, VORTEX_Lookback(optInTimePeriod));
      requireHistoryLength("VORTEX openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("VORTEX openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("VORTEX openAndFill", "outPlusVI", outPlusVI, guardOutLen);
      requireLength("VORTEX openAndFill", "outMinusVI", outMinusVI, guardOutLen);
      if( (Object)outPlusVI == (Object)inHigh || (Object)outPlusVI == (Object)inLow || (Object)outPlusVI == (Object)inClose || (Object)outMinusVI == (Object)inHigh || (Object)outMinusVI == (Object)inLow || (Object)outMinusVI == (Object)inClose || (Object)outPlusVI == (Object)outMinusVI ) {
         throw new TaLibArgumentException("VORTEX openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return vortexOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outPlusVI, outMinusVI);
   }
