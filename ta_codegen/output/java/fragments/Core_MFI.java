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
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
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
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      /* The following two equations are equivalent:
       *    MFI = 100 - (100 / 1 + (posSumMF/negSumMF))
       *    MFI = 100 * (posSumMF/(posSumMF+negSumMF))
       * The second equation is used here for speed optimization.
       */
      /* The first full window is complete: emit its output for startIdx here,
       * then slide the window over the remaining bars below.
       */
      tempValue1 = posSumMF + negSumMF;
      if( tempValue1 < 1.0 ) {
         outReal[outIdx++] = 0.0;
      } else {
         outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
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
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         if( tempValue1 < 1.0 ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
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
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
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
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= (double)inVolume[today++];
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      tempValue1 = posSumMF + negSumMF;
      if( tempValue1 < 1.0 ) {
         outReal[outIdx++] = 0.0;
      } else {
         outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
      }
      while( today <= endIdx ) {
         posSumMF -= mflow_positive[mflow_Idx];
         negSumMF -= mflow_negative[mflow_Idx];
         tempValue1 = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= (double)inVolume[today++];
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         if( tempValue1 < 1.0 ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
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
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
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
      int guardStart = clampedStart(startIdx, endIdx, MFI_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
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
      int guardStart = clampedStart(startIdx, endIdx, MFI_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    * Open with {@link Core#MFI_Open}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class MFI_Stream {
      Core core;
      int optInTimePeriod;
      double posSumMF;
      double negSumMF;
      double prevValue;
      double tempValue1;
      double tempValue2;
      double tempValue3;
      int mflow_Idx;
      int maxIdx_mflow;
      int cbSize_mflow;
      double[] cb_mflow_positive;
      double[] cb_mflow_negative;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      MFI_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#MFI_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      MFI_Stream( MFI_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.posSumMF = other.posSumMF;
         this.negSumMF = other.negSumMF;
         this.prevValue = other.prevValue;
         this.tempValue1 = other.tempValue1;
         this.tempValue2 = other.tempValue2;
         this.tempValue3 = other.tempValue3;
         this.mflow_Idx = other.mflow_Idx;
         this.maxIdx_mflow = other.maxIdx_mflow;
         this.cbSize_mflow = other.cbSize_mflow;
         this.cb_mflow_positive = other.cb_mflow_positive.clone();
         this.cb_mflow_negative = other.cb_mflow_negative.clone();
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( MFI_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.posSumMF = other.posSumMF;
         this.negSumMF = other.negSumMF;
         this.prevValue = other.prevValue;
         this.tempValue1 = other.tempValue1;
         this.tempValue2 = other.tempValue2;
         this.tempValue3 = other.tempValue3;
         this.mflow_Idx = other.mflow_Idx;
         this.maxIdx_mflow = other.maxIdx_mflow;
         this.cbSize_mflow = other.cbSize_mflow;
         if( this.cb_mflow_positive != null && this.cb_mflow_positive.length == other.cb_mflow_positive.length ) {
            System.arraycopy( other.cb_mflow_positive, 0, this.cb_mflow_positive, 0, other.cb_mflow_positive.length );
         } else {
            this.cb_mflow_positive = other.cb_mflow_positive.clone();
         }
         if( this.cb_mflow_negative != null && this.cb_mflow_negative.length == other.cb_mflow_negative.length ) {
            System.arraycopy( other.cb_mflow_negative, 0, this.cb_mflow_negative, 0, other.cb_mflow_negative.length );
         } else {
            this.cb_mflow_negative = other.cb_mflow_negative.clone();
         }
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<MFI_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the handle is left exactly as it was —
       * the stream stays usable, so skip the bar or re-open on a clean
       * history. This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inHigh, double inLow, double inClose, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("MFI update: BadParam", RetCode.BadParam);
         core.MFI_StreamStep(this, inHigh, inLow, inClose, inVolume);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a scratch handle held per thread and
       * reused, so the copy allocates nothing after the first peek of this
       * indicator on this thread. That scratch is retained for the life of
       * the thread.
       */
      public double peek( double inHigh, double inLow, double inClose, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("MFI peek: BadParam", RetCode.BadParam);
         MFI_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new MFI_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.MFI_StreamStep(scratch, inHigh, inLow, inClose, inVolume);
         return scratch.cur_outReal;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public MFI_Stream copy() {
         return new MFI_Stream(this);
      }
   }
   void MFI_StreamStep( MFI_Stream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      sp.posSumMF -= sp.cb_mflow_positive[sp.mflow_Idx];
      sp.negSumMF -= sp.cb_mflow_negative[sp.mflow_Idx];
      sp.tempValue1 = (inHigh + inLow + inClose) / 3.0;
      sp.tempValue2 = sp.tempValue1 - sp.prevValue;
      /* Dead-zone scaled to the two typical prices being compared (issue #107).
       * Captured before prevValue/tempValue1 are repurposed below.
       */
      sp.tempValue3 = Math.abs(sp.tempValue1) + Math.abs(sp.prevValue);
      sp.prevValue = sp.tempValue1;
      sp.tempValue1 *= inVolume;
      if( (Math.abs(sp.tempValue2) <= 0.00000000000001 * (sp.tempValue3)) ) {
         sp.cb_mflow_positive[sp.mflow_Idx] = 0.0;
         sp.cb_mflow_negative[sp.mflow_Idx] = 0.0;
      } else if( sp.tempValue2 < 0 ) {
         sp.cb_mflow_negative[sp.mflow_Idx] = sp.tempValue1;
         sp.negSumMF += sp.tempValue1;
         sp.cb_mflow_positive[sp.mflow_Idx] = 0.0;
      } else {
         sp.cb_mflow_positive[sp.mflow_Idx] = sp.tempValue1;
         sp.posSumMF += sp.tempValue1;
         sp.cb_mflow_negative[sp.mflow_Idx] = 0.0;
      }
      sp.tempValue1 = sp.posSumMF + sp.negSumMF;
      if( sp.tempValue1 < 1.0 ) {
         sp.cur_outReal = 0.0;
      } else {
         sp.cur_outReal = 100.0 * (sp.posSumMF / sp.tempValue1);
      }
      sp.mflow_Idx = sp.mflow_Idx + 1;
      if( sp.mflow_Idx > sp.maxIdx_mflow ) {
         sp.mflow_Idx = 0;
      }
   }
   private RetCode MFI_OpenPass( MFI_Stream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double posSumMF = 0;
      double negSumMF = 0;
      double prevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      double[] mflow_positive;
      double[] mflow_negative;
      int mflow_Idx = 0;
      int maxIdx_mflow = (50)-1;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      /* The following two equations are equivalent:
       *    MFI = 100 - (100 / 1 + (posSumMF/negSumMF))
       *    MFI = 100 * (posSumMF/(posSumMF+negSumMF))
       * The second equation is used here for speed optimization.
       */
      /* The first full window is complete: emit its output for startIdx here,
       * then slide the window over the remaining bars below.
       */
      tempValue1 = posSumMF + negSumMF;
      if( tempValue1 < 1.0 ) {
         outReal[outIdx++ * outStride] = 0.0;
      } else {
         outReal[outIdx++ * outStride] = 100.0 * (posSumMF / tempValue1);
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
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         if( tempValue1 < 1.0 ) {
            outReal[outIdx++ * outStride] = 0.0;
         } else {
            outReal[outIdx++ * outStride] = 100.0 * (posSumMF / tempValue1);
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
      sp.tempValue1 = tempValue1;
      sp.tempValue2 = tempValue2;
      sp.tempValue3 = tempValue3;
      sp.mflow_Idx = mflow_Idx;
      sp.maxIdx_mflow = maxIdx_mflow;
      sp.cbSize_mflow = capCb_mflow;
      sp.cb_mflow_positive = mflow_positive;
      sp.cb_mflow_negative = mflow_negative;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode MFI_OpenImpl( MFI_Stream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return MFI_OpenPass( sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode MFI_OpenAndFillImpl( MFI_Stream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         return RetCode.BadParam;
      }
      return MFI_OpenPass( sp, inHigh, inLow, inClose, inVolume, 0, optInTimePeriod, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode MFI_OpenAndFillInternalImpl( MFI_Stream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return MFI_OpenPass(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
   }
   /* MFI_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MFI_Stream MFI_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MFI_Stream sp = new MFI_Stream(this);
      RetCode retCode = MFI_OpenAndFillInternalImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
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
   /* Internal startIdx-anchored open behind MFI_Open (composition seam). */
   MFI_Stream MFI_OpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      MFI_Stream sp = new MFI_Stream(this);
      RetCode retCode = MFI_OpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod);
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
    * default, as in the batch API).
    */
   public MFI_Stream MFI_Open( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod )
   {
      return MFI_OpenInternal(inHigh, inLow, inClose, inVolume, 0, optInTimePeriod);
   }
   /**
    * {@link Core#MFI_Open} that also fills the output array(s) bit-identically
    * to {@link Core#MFI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MFI_Stream#fillRange()}.
    */
   public MFI_Stream MFI_OpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod, double outReal[] )
   {
      MFI_Stream sp = new MFI_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MFI_OpenAndFillImpl(sp, inHigh, inLow, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
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
