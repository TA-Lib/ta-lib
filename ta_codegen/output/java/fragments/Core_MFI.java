/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  BT       BobTrader (TADoc.org forum user).
 *  MW       github @mw66
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  062704 MF    Prevent divide by zero.
 *  121705 MF    Java port related changes.
 *  060907 MF,BT Fix #1727704. MFI logic bug when no price movement
 *  070726 MW,CC Fix #4. MFI has no unstable period; drop the unstable-period
 *               term (and the now-dead unstable-skip loop) so
 *               TA_SetUnstablePeriod is a no-op for it.
 *  071026 MF,CC Fix #107. Classify money-flow direction with a magnitude-scaled
 *               dead-zone (TA_IS_ZERO_SCALED), not an exact sign test, so an
 *               epsilon-flat typical price is "no movement", not a spurious move.
 *  082326 MF,CC Fix #244. Detect an empty window by counting bars, not by
 *               testing the money-flow sum against a literal 1.0; classify
 *               branchlessly; clamp the emitted ratio into [0,100].
 */

   /**
    * Number of leading input bars {@link Core#MFI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Lookback window for summing money flow (default 14;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MFI_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode MFI_Impl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     double inClose[],
                     double inVolume[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double posSumMF = 0;
      double negSumMF = 0;
      double prevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      double moneyFlow = 0;
      double posFlow = 0;
      double negFlow = 0;
      double posClamped = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      int nullRun = 0;
      double[] mflow_positive;
      double[] mflow_negative;
      int mflow_Idx = 0;
      int maxIdx_mflow = (50)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Id, Type, Static Size */
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      mflow_positive = new double[optInTimePeriod];
      mflow_negative = new double[optInTimePeriod];
      maxIdx_mflow = (optInTimePeriod)-1;
      mflow_Idx = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = optInTimePeriod;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      /* Index into the output. */
      /* Accumulate the positive and negative money flow
       * among the initial period.
       */
      today = startIdx - lookbackTotal;
      prevValue = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
      posSumMF = 0.0;
      negSumMF = 0.0;
      /* Consecutive bars that put nothing into the window, counted so that an
       * empty window can be recognized exactly (issue #244).  The running sums
       * cannot answer that question themselves: they are maintained by
       * add-then-subtract, so when the window empties they hold rounding
       * residue of arbitrary sign, not zero.
       */
      nullRun = 0;
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         /* Dead-zone scaled to the two typical prices being compared (issue #107).
          * Captured before prevValue/tempValue1 are repurposed below.
          */
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= inVolume[today++];
         /* This bar's money flow, and its split into the positive and negative
          * sums.  Selects rather than a three-arm branch: the direction of a
          * price move is a coin flip, so that branch mispredicted on roughly
          * every other bar and dominated the cost of the function.  Adding the
          * unused side's 0.0 to a sum is an exact no-op, so this reproduces the
          * branching form bit for bit.
          *
          * The three quantities are named rather than folded back into
          * tempValue1/2 deliberately, at a known cost: every local in a step body
          * becomes a field of the stream handle, so each name is another store
          * per bar (~10% of MFI's streaming Update, +32 handle bytes).  That is
          * the generator's to fix -- issue #252, which counts 436 such fields
          * across 125 streaming functions -- not something to obfuscate an
          * indicator body over.
          */
         moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
         posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
         negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
         mflow_positive[mflow_Idx] = posFlow;
         mflow_negative[mflow_Idx] = negFlow;
         posSumMF += posFlow;
         negSumMF += negFlow;
         /* A bar contributes nothing when the typical price did not move, or
          * when it moved but carried no volume.  Once a whole period of those
          * has gone by, every slot of the ring is 0.0, so the sums are known to
          * be exactly zero and the residue can be dropped.
          */
         nullRun = (moneyFlow == 0.0) ? nullRun + 1 : 0;
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            posSumMF = 0.0;
            negSumMF = 0.0;
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      /* The following two equations are equivalent:
       *    MFI = 100 - (100 / 1 + (posSumMF/negSumMF))
       *    MFI = 100 * (posSumMF/(posSumMF+negSumMF))
       * The second equation is used here for speed optimization.
       *
       * Both sums are non-negative, so the total is zero only for a window that
       * received no money flow at all -- 0/0, reported as 0.0.  The test is on
       * the total itself, not on a fixed threshold: money flow is a price times
       * a volume, so any constant compared against it is a constant in some
       * arbitrary unit, and would zero a healthy index for any instrument
       * quoted small enough to fall under it (issue #244).
       *
       * Clamping the numerator into [0,total] keeps the result inside the
       * documented 0-100 range: the sums drift by a few ulp as the window
       * slides, and a sum whose true value is near zero can drift negative.
       */
      /* The first full window is complete: emit its output for startIdx here,
       * then slide the window over the remaining bars below.
       */
      tempValue1 = posSumMF + negSumMF;
      posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
      if( tempValue1 <= 0.0 ) {
         outReal[outIdx++] = 0.0;
      } else {
         outReal[outIdx++] = 100.0 * (posClamped / tempValue1);
      }
      /* Now continue processing the remaining bars. */
      while( today <= endIdx ) {
         posSumMF -= mflow_positive[mflow_Idx];
         negSumMF -= mflow_negative[mflow_Idx];
         tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         /* Dead-zone scaled to the two typical prices being compared (issue #107).
          * Captured before prevValue/tempValue1 are repurposed below.
          */
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= inVolume[today++];
         moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
         posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
         negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
         mflow_positive[mflow_Idx] = posFlow;
         mflow_negative[mflow_Idx] = negFlow;
         posSumMF += posFlow;
         negSumMF += negFlow;
         nullRun = (moneyFlow == 0.0) ? nullRun + 1 : 0;
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            posSumMF = 0.0;
            negSumMF = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
         if( tempValue1 <= 0.0 ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = 100.0 * (posClamped / tempValue1);
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode MFI_Impl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     float inClose[],
                     float inVolume[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double posSumMF = 0;
      double negSumMF = 0;
      double prevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      double moneyFlow = 0;
      double posFlow = 0;
      double negFlow = 0;
      double posClamped = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      int nullRun = 0;
      double[] mflow_positive;
      double[] mflow_negative;
      int mflow_Idx = 0;
      int maxIdx_mflow = (50)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      mflow_positive = new double[optInTimePeriod];
      mflow_negative = new double[optInTimePeriod];
      maxIdx_mflow = (optInTimePeriod)-1;
      mflow_Idx = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = optInTimePeriod;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx - lookbackTotal;
      prevValue = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
      posSumMF = 0.0;
      negSumMF = 0.0;
      nullRun = 0;
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= (double)inVolume[today++];
         moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
         posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
         negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
         mflow_positive[mflow_Idx] = posFlow;
         mflow_negative[mflow_Idx] = negFlow;
         posSumMF += posFlow;
         negSumMF += negFlow;
         nullRun = (moneyFlow == 0.0) ? nullRun + 1 : 0;
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            posSumMF = 0.0;
            negSumMF = 0.0;
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      tempValue1 = posSumMF + negSumMF;
      posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
      if( tempValue1 <= 0.0 ) {
         outReal[outIdx++] = 0.0;
      } else {
         outReal[outIdx++] = 100.0 * (posClamped / tempValue1);
      }
      while( today <= endIdx ) {
         posSumMF -= mflow_positive[mflow_Idx];
         negSumMF -= mflow_negative[mflow_Idx];
         tempValue1 = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= (double)inVolume[today++];
         moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
         posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
         negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
         mflow_positive[mflow_Idx] = posFlow;
         mflow_negative[mflow_Idx] = negFlow;
         posSumMF += posFlow;
         negSumMF += negFlow;
         nullRun = (moneyFlow == 0.0) ? nullRun + 1 : 0;
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            posSumMF = 0.0;
            negSumMF = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
         if( tempValue1 <= 0.0 ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = 100.0 * (posClamped / tempValue1);
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Money Flow Index: a volume-weighted momentum oscillator (0-100) comparing
    * positive vs negative money flow over a period. A volume-based analog of
    * RSI. &gt;80 overbought, &lt;20 oversold.
    * <p><b>Formula</b>
    * <pre>{@code
    * TP = (High+Low+Close)/3; MF = TP*Volume, classed positive if TP>prevTP, negative if TP<prevTP, neither if equal. MFI = 100 * posSumMF/(posSumMF+negSumMF).
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When the typical price is unchanged from the prior bar, that bar's money flow is counted as neither positive nor negative.</li>
    * <li>A window in which no bar contributed any money flow — every typical price unchanged, or no volume traded — leaves the index undefined (0/0); 0 is returned. The result does not otherwise depend on the size of the money flow: scaling every volume, or quoting the instrument in a different unit, leaves the index unchanged.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MFI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod Lookback window for summing money flow (default 14;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Money Flow Index. Must hold at least
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
    * @see Core#RSI
    * @see Core#AD
    * @see Core#ADOSC
    */
   public OutRange MFI( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        double inVolume[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("MFI", startIdx, endIdx);
      int guardStart = clampedStart("MFI", startIdx, MFI_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MFI", "inHigh", inHigh, guardInLen);
      requireLength("MFI", "inLow", inLow, guardInLen);
      requireLength("MFI", "inClose", inClose, guardInLen);
      requireLength("MFI", "inVolume", inVolume, guardInLen);
      requireLength("MFI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MFI_Impl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MFI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Money Flow Index: a volume-weighted momentum oscillator (0-100) comparing
    * positive vs negative money flow over a period. A volume-based analog of
    * RSI. &gt;80 overbought, &lt;20 oversold.
    * <p><b>Formula</b>
    * <pre>{@code
    * TP = (High+Low+Close)/3; MF = TP*Volume, classed positive if TP>prevTP, negative if TP<prevTP, neither if equal. MFI = 100 * posSumMF/(posSumMF+negSumMF).
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When the typical price is unchanged from the prior bar, that bar's money flow is counted as neither positive nor negative.</li>
    * <li>A window in which no bar contributed any money flow — every typical price unchanged, or no volume traded — leaves the index undefined (0/0); 0 is returned. The result does not otherwise depend on the size of the money flow: scaling every volume, or quoting the instrument in a different unit, leaves the index unchanged.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MFI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod Lookback window for summing money flow (default 14;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Money Flow Index. Must hold at least
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
    * @see Core#RSI
    * @see Core#AD
    * @see Core#ADOSC
    */
   public OutRange MFI( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        float inVolume[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("MFI", startIdx, endIdx);
      int guardStart = clampedStart("MFI", startIdx, MFI_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MFI", "inHigh", inHigh, guardInLen);
      requireLength("MFI", "inLow", inLow, guardInLen);
      requireLength("MFI", "inClose", inClose, guardInLen);
      requireLength("MFI", "inVolume", inVolume, guardInLen);
      requireLength("MFI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MFI_Impl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MFI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MFI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MFI} over the same series.
    * Open with {@link Core#mfiOpen}; there is no close — the handle is
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
   public static final class MfiStream {
      Core core;
      int optInTimePeriod;
      double posSumMF;
      double negSumMF;
      double prevValue;
      int nullRun;
      int mflow_Idx;
      int maxIdx_mflow;
      int cbSize_mflow;
      double[] cb_mflow_positive;
      double[] cb_mflow_negative;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      MfiStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MFI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MfiStream( MfiStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.posSumMF = other.posSumMF;
         this.negSumMF = other.negSumMF;
         this.prevValue = other.prevValue;
         this.nullRun = other.nullRun;
         this.mflow_Idx = other.mflow_Idx;
         this.maxIdx_mflow = other.maxIdx_mflow;
         this.cbSize_mflow = other.cbSize_mflow;
         this.cb_mflow_positive = other.cb_mflow_positive.clone();
         this.cb_mflow_negative = other.cb_mflow_negative.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inHigh, double inLow, double inClose, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("MFI update: BadParam", RetCode.BadParam);
         }
         core.mfiStepImpl(this, inHigh, inLow, inClose, inVolume);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
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
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], double outReal[] ) {
         requireArgument("MFI updateAndFill", "inHigh", inHigh);
         requireArgument("MFI updateAndFill", "inLow", inLow);
         requireArgument("MFI updateAndFill", "inClose", inClose);
         requireArgument("MFI updateAndFill", "inVolume", inVolume);
         requireArgument("MFI updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || inVolume.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("MFI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) || !Double.isFinite(inVolume[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("MFI updateAndFill: BadParam", RetCode.BadParam);
            }
            core.mfiStepImpl(this, inHigh[i], inLow[i], inClose[i], inVolume[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public double peek( double inHigh, double inLow, double inClose, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("MFI peek: BadParam", RetCode.BadParam);
         MfiStream sp = this;
         double tempValue1 = 0.0;
         double tempValue2 = 0.0;
         double tempValue3 = 0.0;
         double moneyFlow = 0.0;
         double posFlow = 0.0;
         double negFlow = 0.0;
         double posClamped = 0.0;
         double cur_outReal = sp.cur_outReal;
         int mflow_Idx = sp.mflow_Idx;
         double negSumMF = sp.negSumMF;
         int nullRun = sp.nullRun;
         double posSumMF = sp.posSumMF;
         double prevValue = sp.prevValue;
         posSumMF -= sp.cb_mflow_positive[mflow_Idx];
         negSumMF -= sp.cb_mflow_negative[mflow_Idx];
         tempValue1 = (inHigh + inLow + inClose) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         /* Dead-zone scaled to the two typical prices being compared (issue #107).
          * Captured before prevValue/tempValue1 are repurposed below.
          */
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= inVolume;
         moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
         posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
         negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
         posSumMF += posFlow;
         negSumMF += negFlow;
         nullRun = (moneyFlow == 0.0) ? nullRun + 1 : 0;
         if( nullRun >= sp.optInTimePeriod ) {
            nullRun = sp.optInTimePeriod;
            posSumMF = 0.0;
            negSumMF = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
         if( tempValue1 <= 0.0 ) {
            cur_outReal = 0.0;
         } else {
            cur_outReal = 100.0 * (posClamped / tempValue1);
         }
         mflow_Idx = mflow_Idx + 1;
         if( mflow_Idx > sp.maxIdx_mflow ) {
            mflow_Idx = 0;
         }
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
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
      public MfiStream clone() {
         return new MfiStream(this);
      }
   }
   void mfiStepImpl( MfiStream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      double tempValue1 = 0.0;
      double tempValue2 = 0.0;
      double tempValue3 = 0.0;
      double moneyFlow = 0.0;
      double posFlow = 0.0;
      double negFlow = 0.0;
      double posClamped = 0.0;
      sp.posSumMF -= sp.cb_mflow_positive[sp.mflow_Idx];
      sp.negSumMF -= sp.cb_mflow_negative[sp.mflow_Idx];
      tempValue1 = (inHigh + inLow + inClose) / 3.0;
      tempValue2 = tempValue1 - sp.prevValue;
      /* Dead-zone scaled to the two typical prices being compared (issue #107).
       * Captured before prevValue/tempValue1 are repurposed below.
       */
      tempValue3 = Math.abs(tempValue1) + Math.abs(sp.prevValue);
      sp.prevValue = tempValue1;
      tempValue1 *= inVolume;
      moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
      posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
      negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
      sp.cb_mflow_positive[sp.mflow_Idx] = posFlow;
      sp.cb_mflow_negative[sp.mflow_Idx] = negFlow;
      sp.posSumMF += posFlow;
      sp.negSumMF += negFlow;
      sp.nullRun = (moneyFlow == 0.0) ? sp.nullRun + 1 : 0;
      if( sp.nullRun >= sp.optInTimePeriod ) {
         sp.nullRun = sp.optInTimePeriod;
         sp.posSumMF = 0.0;
         sp.negSumMF = 0.0;
      }
      tempValue1 = sp.posSumMF + sp.negSumMF;
      posClamped = (sp.posSumMF < 0.0) ? 0.0 : ((sp.posSumMF > tempValue1) ? tempValue1 : sp.posSumMF);
      if( tempValue1 <= 0.0 ) {
         sp.cur_outReal = 0.0;
      } else {
         sp.cur_outReal = 100.0 * (posClamped / tempValue1);
      }
      sp.mflow_Idx = sp.mflow_Idx + 1;
      if( sp.mflow_Idx > sp.maxIdx_mflow ) {
         sp.mflow_Idx = 0;
      }
   }
   private RetCode mfiOpenImpl( MfiStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double posSumMF = 0;
      double negSumMF = 0;
      double prevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      double moneyFlow = 0;
      double posFlow = 0;
      double negFlow = 0;
      double posClamped = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      int nullRun = 0;
      double[] mflow_positive;
      double[] mflow_negative;
      int mflow_Idx = 0;
      int maxIdx_mflow = (50)-1;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Id, Type, Static Size */
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      mflow_positive = new double[optInTimePeriod];
      mflow_negative = new double[optInTimePeriod];
      maxIdx_mflow = (optInTimePeriod)-1;
      mflow_Idx = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = optInTimePeriod;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      outIdx = 0;
      /* Index into the output. */
      /* Accumulate the positive and negative money flow
       * among the initial period.
       */
      today = startIdx - lookbackTotal;
      prevValue = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
      posSumMF = 0.0;
      negSumMF = 0.0;
      /* Consecutive bars that put nothing into the window, counted so that an
       * empty window can be recognized exactly (issue #244).  The running sums
       * cannot answer that question themselves: they are maintained by
       * add-then-subtract, so when the window empties they hold rounding
       * residue of arbitrary sign, not zero.
       */
      nullRun = 0;
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         /* Dead-zone scaled to the two typical prices being compared (issue #107).
          * Captured before prevValue/tempValue1 are repurposed below.
          */
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= inVolume[today++];
         /* This bar's money flow, and its split into the positive and negative
          * sums.  Selects rather than a three-arm branch: the direction of a
          * price move is a coin flip, so that branch mispredicted on roughly
          * every other bar and dominated the cost of the function.  Adding the
          * unused side's 0.0 to a sum is an exact no-op, so this reproduces the
          * branching form bit for bit.
          *
          * The three quantities are named rather than folded back into
          * tempValue1/2 deliberately, at a known cost: every local in a step body
          * becomes a field of the stream handle, so each name is another store
          * per bar (~10% of MFI's streaming Update, +32 handle bytes).  That is
          * the generator's to fix -- issue #252, which counts 436 such fields
          * across 125 streaming functions -- not something to obfuscate an
          * indicator body over.
          */
         moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
         posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
         negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
         mflow_positive[mflow_Idx] = posFlow;
         mflow_negative[mflow_Idx] = negFlow;
         posSumMF += posFlow;
         negSumMF += negFlow;
         /* A bar contributes nothing when the typical price did not move, or
          * when it moved but carried no volume.  Once a whole period of those
          * has gone by, every slot of the ring is 0.0, so the sums are known to
          * be exactly zero and the residue can be dropped.
          */
         nullRun = (moneyFlow == 0.0) ? nullRun + 1 : 0;
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            posSumMF = 0.0;
            negSumMF = 0.0;
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      /* The following two equations are equivalent:
       *    MFI = 100 - (100 / 1 + (posSumMF/negSumMF))
       *    MFI = 100 * (posSumMF/(posSumMF+negSumMF))
       * The second equation is used here for speed optimization.
       *
       * Both sums are non-negative, so the total is zero only for a window that
       * received no money flow at all -- 0/0, reported as 0.0.  The test is on
       * the total itself, not on a fixed threshold: money flow is a price times
       * a volume, so any constant compared against it is a constant in some
       * arbitrary unit, and would zero a healthy index for any instrument
       * quoted small enough to fall under it (issue #244).
       *
       * Clamping the numerator into [0,total] keeps the result inside the
       * documented 0-100 range: the sums drift by a few ulp as the window
       * slides, and a sum whose true value is near zero can drift negative.
       */
      /* The first full window is complete: emit its output for startIdx here,
       * then slide the window over the remaining bars below.
       */
      tempValue1 = posSumMF + negSumMF;
      posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
      if( tempValue1 <= 0.0 ) {
         outReal[outIdx++ * outStride] = 0.0;
      } else {
         outReal[outIdx++ * outStride] = 100.0 * (posClamped / tempValue1);
      }
      /* Now continue processing the remaining bars. */
      while( today <= endIdx ) {
         posSumMF -= mflow_positive[mflow_Idx];
         negSumMF -= mflow_negative[mflow_Idx];
         tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         /* Dead-zone scaled to the two typical prices being compared (issue #107).
          * Captured before prevValue/tempValue1 are repurposed below.
          */
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= inVolume[today++];
         moneyFlow = (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ? 0.0 : tempValue1;
         posFlow = (tempValue2 < 0.0) ? 0.0 : moneyFlow;
         negFlow = (tempValue2 < 0.0) ? moneyFlow : 0.0;
         mflow_positive[mflow_Idx] = posFlow;
         mflow_negative[mflow_Idx] = negFlow;
         posSumMF += posFlow;
         negSumMF += negFlow;
         nullRun = (moneyFlow == 0.0) ? nullRun + 1 : 0;
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            posSumMF = 0.0;
            negSumMF = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         posClamped = (posSumMF < 0.0) ? 0.0 : ((posSumMF > tempValue1) ? tempValue1 : posSumMF);
         if( tempValue1 <= 0.0 ) {
            outReal[outIdx++ * outStride] = 0.0;
         } else {
            outReal[outIdx++ * outStride] = 100.0 * (posClamped / tempValue1);
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capCb_mflow = maxIdx_mflow + 1;
      if( capCb_mflow > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.posSumMF = posSumMF;
      sp.negSumMF = negSumMF;
      sp.prevValue = prevValue;
      sp.nullRun = nullRun;
      sp.mflow_Idx = mflow_Idx;
      sp.maxIdx_mflow = maxIdx_mflow;
      sp.cbSize_mflow = capCb_mflow;
      sp.cb_mflow_positive = mflow_positive;
      sp.cb_mflow_negative = mflow_negative;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* mfiOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MfiStream mfiOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MfiStream sp = new MfiStream(this);
      RetCode retCode = mfiOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MFI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MFI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MFI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind mfiOpen (composition seam). */
   MfiStream mfiOpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      MfiStream sp = new MfiStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = mfiOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MFI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MFI open: internal error", retCode);
      }
      throw new TaLibArgumentException("MFI open: " + retCode, retCode);
   }
   /**
    * Open a live MFI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MFI} at that bar.
    * <p>The history must hold at least {@code MFI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MfiStream mfiOpen( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod )
   {
      requireArgument("MFI open", "inHigh", inHigh);
      requireHistory("MFI open", inHigh.length);
      requireArgument("MFI open", "inLow", inLow);
      requireArgument("MFI open", "inClose", inClose);
      requireArgument("MFI open", "inVolume", inVolume);
      requireHistoryLength("MFI open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("MFI open", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("MFI open", "inVolume", inVolume.length, inHigh.length);
      return mfiOpenInternal(inHigh, inLow, inClose, inVolume, 0, optInTimePeriod);
   }
   /**
    * {@link Core#mfiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MFI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MfiStream#outRange()}.
    */
   public MfiStream mfiOpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("MFI openAndFill", "inHigh", inHigh);
      requireHistory("MFI openAndFill", inHigh.length);
      requireArgument("MFI openAndFill", "inLow", inLow);
      requireArgument("MFI openAndFill", "inClose", inClose);
      requireArgument("MFI openAndFill", "inVolume", inVolume);
      int guardOutLen = openFillCount("MFI openAndFill", inHigh.length, MFI_Lookback(optInTimePeriod));
      requireHistoryLength("MFI openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("MFI openAndFill", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("MFI openAndFill", "inVolume", inVolume.length, inHigh.length);
      requireLength("MFI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("MFI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return mfiOpenAndFillInternal(inHigh, inLow, inClose, inVolume, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
