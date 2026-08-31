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
 *  072126 MF,CC  First version (issue #134).
 */

   /**
    * Number of leading input bars {@link Core#CMF} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the window (default 20; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CMF_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode CMF_Impl( int startIdx,
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
      double sumMFV = 0;
      double sumVol = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double mfv = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      double[] mfv_flow;
      double[] mfv_volume;
      int mfv_Idx = 0;
      int maxIdx_mfv = (50)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Both the per-bar money flow volume and the volume that produced it are
       * carried in the circular buffer. Keeping the volume here rather than
       * re-reading inVolume[] at the trailing index is what makes outReal safe to
       * alias any input: once a bar has been consumed it is never read again.
       */
      /* Id, Type, Static Size */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod - 1;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      mfv_flow = new double[optInTimePeriod];
      mfv_volume = new double[optInTimePeriod];
      maxIdx_mfv = (optInTimePeriod)-1;
      mfv_Idx = 0;
      outIdx = 0;
      /* Accumulate the money flow volume and the volume over the first
       * complete window, filling the circular buffer as we go.
       *
       * The per-bar multiplier is written exactly as in ta_AD.c so that the
       * Chaikin money flow volume has one definition in the library.
       */
      today = startIdx - lookbackTotal;
      sumMFV = 0.0;
      sumVol = 0.0;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         high = inHigh[today];
         low = inLow[today];
         close = inClose[today];
         tmp = high - low;
         if( tmp > 0.0 ) {
            mfv = (close - low - (high - close)) / tmp * inVolume[today];
         } else {
            mfv = 0.0;
         }
         mfv_flow[mfv_Idx] = mfv;
         mfv_volume[mfv_Idx] = inVolume[today];
         sumMFV += mfv;
         sumVol += inVolume[today];
         today += 1;
         mfv_Idx++;
         if( mfv_Idx > maxIdx_mfv ) { mfv_Idx = 0; }
      }
      /* The first full window is complete: emit its output for startIdx here,
       * then slide the window over the remaining bars below.
       *
       * A window whose volume is entirely zero has no money flow to distribute;
       * report 0.0 rather than propagating a division by zero (issue #112).
       */
      if( sumVol > 0.0 ) {
         outReal[outIdx++] = sumMFV / sumVol;
      } else {
         outReal[outIdx++] = 0.0;
      }
      /* Now continue processing the remaining bars. */
      while( today <= endIdx ) {
         sumMFV -= mfv_flow[mfv_Idx];
         sumVol -= mfv_volume[mfv_Idx];
         high = inHigh[today];
         low = inLow[today];
         close = inClose[today];
         tmp = high - low;
         if( tmp > 0.0 ) {
            mfv = (close - low - (high - close)) / tmp * inVolume[today];
         } else {
            mfv = 0.0;
         }
         mfv_flow[mfv_Idx] = mfv;
         mfv_volume[mfv_Idx] = inVolume[today];
         sumMFV += mfv;
         sumVol += inVolume[today];
         today += 1;
         if( sumVol > 0.0 ) {
            outReal[outIdx++] = sumMFV / sumVol;
         } else {
            outReal[outIdx++] = 0.0;
         }
         mfv_Idx++;
         if( mfv_Idx > maxIdx_mfv ) { mfv_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode CMF_Impl( int startIdx,
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
      double sumMFV = 0;
      double sumVol = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double mfv = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      double[] mfv_flow;
      double[] mfv_volume;
      int mfv_Idx = 0;
      int maxIdx_mfv = (50)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      mfv_flow = new double[optInTimePeriod];
      mfv_volume = new double[optInTimePeriod];
      maxIdx_mfv = (optInTimePeriod)-1;
      mfv_Idx = 0;
      outIdx = 0;
      today = startIdx - lookbackTotal;
      sumMFV = 0.0;
      sumVol = 0.0;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         high = (double)inHigh[today];
         low = (double)inLow[today];
         close = (double)inClose[today];
         tmp = high - low;
         if( tmp > 0.0 ) {
            mfv = (close - low - (high - close)) / tmp * (double)inVolume[today];
         } else {
            mfv = 0.0;
         }
         mfv_flow[mfv_Idx] = mfv;
         mfv_volume[mfv_Idx] = (double)inVolume[today];
         sumMFV += mfv;
         sumVol += (double)inVolume[today];
         today += 1;
         mfv_Idx++;
         if( mfv_Idx > maxIdx_mfv ) { mfv_Idx = 0; }
      }
      if( sumVol > 0.0 ) {
         outReal[outIdx++] = sumMFV / sumVol;
      } else {
         outReal[outIdx++] = 0.0;
      }
      while( today <= endIdx ) {
         sumMFV -= mfv_flow[mfv_Idx];
         sumVol -= mfv_volume[mfv_Idx];
         high = (double)inHigh[today];
         low = (double)inLow[today];
         close = (double)inClose[today];
         tmp = high - low;
         if( tmp > 0.0 ) {
            mfv = (close - low - (high - close)) / tmp * (double)inVolume[today];
         } else {
            mfv = 0.0;
         }
         mfv_flow[mfv_Idx] = mfv;
         mfv_volume[mfv_Idx] = (double)inVolume[today];
         sumMFV += mfv;
         sumVol += (double)inVolume[today];
         today += 1;
         if( sumVol > 0.0 ) {
            outReal[outIdx++] = sumMFV / sumVol;
         } else {
            outReal[outIdx++] = 0.0;
         }
         mfv_Idx++;
         if( mfv_Idx > maxIdx_mfv ) { mfv_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Chaikin Money Flow: over a trailing window of {@code optInTimePeriod}
    * bars, the sum of each bar's money flow volume divided by the sum of its
    * volume. The result is a ratio in {@code [-1, +1]}. A bar's money flow
    * volume is its volume scaled by where the close sat inside the bar's range:
    * a close at the high contributes the full volume, a close at the low
    * contributes minus the full volume, and a close at the midpoint contributes
    * nothing. Summing that over a window and dividing by the window's volume
    * answers "over these N bars, what share of the traded volume closed near
    * the top of its range?" Above zero is accumulation, below zero is
    * distribution, and the distance from zero measures conviction. Because the
    * divisor is the window's own volume, the output is comparable across
    * instruments and across time in a way a raw accumulation total is not.
    * Created by Marc Chaikin, who also authored the [{@code AD}](/functions/ad)
    * line this shares its per-bar multiplier with. CMF is that same multiplier
    * summed over a fixed window and normalised, where AD accumulates it from
    * the start of the series without bound.
    * <p><b>Formula</b>
    * <pre>{@code
    * t = high[i] - low[i]
    * mfv[i] = ((close[i] - low[i]) - (high[i] - close[i])) / t * volume[i], or 0 when t is not positive
    * CMF[i] = ( sum_{k=i-N+1..i} mfv[k] ) / ( sum_{k=i-N+1..i} volume[k] ), N = optInTimePeriod
    * There is no seeding and no recursion, hence no unstable period. Each output depends only on the N bars in its own window.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The output is the raw ratio in {@code [-1, +1]}, matching every published definition. Some retail platforms display it multiplied by 100; that is a presentation choice, not a different indicator.</li>
    * <li>Each bar's close is expected to lie within its own {@code [low, high]}, and its volume to be finite and non-negative. A close outside its bar makes the multiplier exceed ±1 and is passed through unclamped, exactly as [{@code AD}](/functions/ad) does.</li>
    * <li>A bar whose high equals its low has no range for the close to sit inside, so it contributes exactly zero money flow volume rather than dividing by zero. Its volume still counts toward the divisor.</li>
    * <li>A window whose volume is entirely zero has no money flow to distribute and reports 0.0. Published references are silent here and other implementations divide by zero; TA-Lib does not return NaN from a successful call.</li>
    * <li>Bars where the low exceeds the high are malformed rather than degenerate, and also contribute zero.</li>
    * <li>The default period of 20 follows the original write-up, which describes 20 or 21 bars.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CMF_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod Number of bars in the window (default 20; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Chaikin money flow, in the range -1 to +1. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#AD
    * @see Core#ADOSC
    * @see Core#MFI
    * @see Core#OBV
    */
   public OutRange CMF( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        double inVolume[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("CMF", startIdx, endIdx);
      int guardStart = clampedStart("CMF", startIdx, CMF_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CMF", "inHigh", inHigh, guardInLen);
      requireLength("CMF", "inLow", inLow, guardInLen);
      requireLength("CMF", "inClose", inClose, guardInLen);
      requireLength("CMF", "inVolume", inVolume, guardInLen);
      requireLength("CMF", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CMF_Impl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CMF", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Chaikin Money Flow: over a trailing window of {@code optInTimePeriod}
    * bars, the sum of each bar's money flow volume divided by the sum of its
    * volume. The result is a ratio in {@code [-1, +1]}. A bar's money flow
    * volume is its volume scaled by where the close sat inside the bar's range:
    * a close at the high contributes the full volume, a close at the low
    * contributes minus the full volume, and a close at the midpoint contributes
    * nothing. Summing that over a window and dividing by the window's volume
    * answers "over these N bars, what share of the traded volume closed near
    * the top of its range?" Above zero is accumulation, below zero is
    * distribution, and the distance from zero measures conviction. Because the
    * divisor is the window's own volume, the output is comparable across
    * instruments and across time in a way a raw accumulation total is not.
    * Created by Marc Chaikin, who also authored the [{@code AD}](/functions/ad)
    * line this shares its per-bar multiplier with. CMF is that same multiplier
    * summed over a fixed window and normalised, where AD accumulates it from
    * the start of the series without bound.
    * <p><b>Formula</b>
    * <pre>{@code
    * t = high[i] - low[i]
    * mfv[i] = ((close[i] - low[i]) - (high[i] - close[i])) / t * volume[i], or 0 when t is not positive
    * CMF[i] = ( sum_{k=i-N+1..i} mfv[k] ) / ( sum_{k=i-N+1..i} volume[k] ), N = optInTimePeriod
    * There is no seeding and no recursion, hence no unstable period. Each output depends only on the N bars in its own window.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The output is the raw ratio in {@code [-1, +1]}, matching every published definition. Some retail platforms display it multiplied by 100; that is a presentation choice, not a different indicator.</li>
    * <li>Each bar's close is expected to lie within its own {@code [low, high]}, and its volume to be finite and non-negative. A close outside its bar makes the multiplier exceed ±1 and is passed through unclamped, exactly as [{@code AD}](/functions/ad) does.</li>
    * <li>A bar whose high equals its low has no range for the close to sit inside, so it contributes exactly zero money flow volume rather than dividing by zero. Its volume still counts toward the divisor.</li>
    * <li>A window whose volume is entirely zero has no money flow to distribute and reports 0.0. Published references are silent here and other implementations divide by zero; TA-Lib does not return NaN from a successful call.</li>
    * <li>Bars where the low exceeds the high are malformed rather than degenerate, and also contribute zero.</li>
    * <li>The default period of 20 follows the original write-up, which describes 20 or 21 bars.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CMF_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod Number of bars in the window (default 20; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Chaikin money flow, in the range -1 to +1. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#AD
    * @see Core#ADOSC
    * @see Core#MFI
    * @see Core#OBV
    */
   public OutRange CMF( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        float inVolume[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("CMF", startIdx, endIdx);
      int guardStart = clampedStart("CMF", startIdx, CMF_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CMF", "inHigh", inHigh, guardInLen);
      requireLength("CMF", "inLow", inLow, guardInLen);
      requireLength("CMF", "inClose", inClose, guardInLen);
      requireLength("CMF", "inVolume", inVolume, guardInLen);
      requireLength("CMF", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CMF_Impl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CMF", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CMF stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CMF} over the same series.
    * Open with {@link Core#cmfOpen}; there is no close — the handle is
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
   public static final class CmfStream {
      Core core;
      int optInTimePeriod;
      double sumMFV;
      double sumVol;
      int mfv_Idx;
      int maxIdx_mfv;
      int cbSize_mfv;
      double[] cb_mfv_flow;
      double[] cb_mfv_volume;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      CmfStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CMF} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CmfStream( CmfStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.sumMFV = other.sumMFV;
         this.sumVol = other.sumVol;
         this.mfv_Idx = other.mfv_Idx;
         this.maxIdx_mfv = other.maxIdx_mfv;
         this.cbSize_mfv = other.cbSize_mfv;
         this.cb_mfv_flow = other.cb_mfv_flow.clone();
         this.cb_mfv_volume = other.cb_mfv_volume.clone();
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
            throw new TaLibArgumentException("CMF update: BadParam", RetCode.BadParam);
         }
         core.cmfStepImpl(this, inHigh, inLow, inClose, inVolume);
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
         requireArgument("CMF updateAndFill", "inHigh", inHigh);
         requireArgument("CMF updateAndFill", "inLow", inLow);
         requireArgument("CMF updateAndFill", "inClose", inClose);
         requireArgument("CMF updateAndFill", "inVolume", inVolume);
         requireArgument("CMF updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || inVolume.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("CMF updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) || !Double.isFinite(inVolume[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CMF updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cmfStepImpl(this, inHigh[i], inLow[i], inClose[i], inVolume[i]);
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
            throw new TaLibArgumentException("CMF peek: BadParam", RetCode.BadParam);
         CmfStream sp = this;
         double high = 0.0;
         double low = 0.0;
         double close = 0.0;
         double tmp = 0.0;
         double mfv = 0.0;
         double cur_outReal = sp.cur_outReal;
         int mfv_Idx = sp.mfv_Idx;
         double sumMFV = sp.sumMFV;
         double sumVol = sp.sumVol;
         sumMFV -= sp.cb_mfv_flow[mfv_Idx];
         sumVol -= sp.cb_mfv_volume[mfv_Idx];
         high = inHigh;
         low = inLow;
         close = inClose;
         tmp = high - low;
         if( tmp > 0.0 ) {
            mfv = (close - low - (high - close)) / tmp * inVolume;
         } else {
            mfv = 0.0;
         }
         sumMFV += mfv;
         sumVol += inVolume;
         if( sumVol > 0.0 ) {
            cur_outReal = sumMFV / sumVol;
         } else {
            cur_outReal = 0.0;
         }
         mfv_Idx = mfv_Idx + 1;
         if( mfv_Idx > sp.maxIdx_mfv ) {
            mfv_Idx = 0;
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
      public CmfStream clone() {
         return new CmfStream(this);
      }
   }
   void cmfStepImpl( CmfStream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      double high = 0.0;
      double low = 0.0;
      double close = 0.0;
      double tmp = 0.0;
      double mfv = 0.0;
      sp.sumMFV -= sp.cb_mfv_flow[sp.mfv_Idx];
      sp.sumVol -= sp.cb_mfv_volume[sp.mfv_Idx];
      high = inHigh;
      low = inLow;
      close = inClose;
      tmp = high - low;
      if( tmp > 0.0 ) {
         mfv = (close - low - (high - close)) / tmp * inVolume;
      } else {
         mfv = 0.0;
      }
      sp.cb_mfv_flow[sp.mfv_Idx] = mfv;
      sp.cb_mfv_volume[sp.mfv_Idx] = inVolume;
      sp.sumMFV += mfv;
      sp.sumVol += inVolume;
      if( sp.sumVol > 0.0 ) {
         sp.cur_outReal = sp.sumMFV / sp.sumVol;
      } else {
         sp.cur_outReal = 0.0;
      }
      sp.mfv_Idx = sp.mfv_Idx + 1;
      if( sp.mfv_Idx > sp.maxIdx_mfv ) {
         sp.mfv_Idx = 0;
      }
   }
   private RetCode cmfOpenImpl( CmfStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double sumMFV = 0;
      double sumVol = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double mfv = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      double[] mfv_flow;
      double[] mfv_volume;
      int mfv_Idx = 0;
      int maxIdx_mfv = (50)-1;
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
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Both the per-bar money flow volume and the volume that produced it are
       * carried in the circular buffer. Keeping the volume here rather than
       * re-reading inVolume[] at the trailing index is what makes outReal safe to
       * alias any input: once a bar has been consumed it is never read again.
       */
      /* Id, Type, Static Size */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod - 1;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      mfv_flow = new double[optInTimePeriod];
      mfv_volume = new double[optInTimePeriod];
      maxIdx_mfv = (optInTimePeriod)-1;
      mfv_Idx = 0;
      outIdx = 0;
      /* Accumulate the money flow volume and the volume over the first
       * complete window, filling the circular buffer as we go.
       *
       * The per-bar multiplier is written exactly as in ta_AD.c so that the
       * Chaikin money flow volume has one definition in the library.
       */
      today = startIdx - lookbackTotal;
      sumMFV = 0.0;
      sumVol = 0.0;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         high = inHigh[today];
         low = inLow[today];
         close = inClose[today];
         tmp = high - low;
         if( tmp > 0.0 ) {
            mfv = (close - low - (high - close)) / tmp * inVolume[today];
         } else {
            mfv = 0.0;
         }
         mfv_flow[mfv_Idx] = mfv;
         mfv_volume[mfv_Idx] = inVolume[today];
         sumMFV += mfv;
         sumVol += inVolume[today];
         today += 1;
         mfv_Idx++;
         if( mfv_Idx > maxIdx_mfv ) { mfv_Idx = 0; }
      }
      /* The first full window is complete: emit its output for startIdx here,
       * then slide the window over the remaining bars below.
       *
       * A window whose volume is entirely zero has no money flow to distribute;
       * report 0.0 rather than propagating a division by zero (issue #112).
       */
      if( sumVol > 0.0 ) {
         outReal[outIdx++ * outStride] = sumMFV / sumVol;
      } else {
         outReal[outIdx++ * outStride] = 0.0;
      }
      /* Now continue processing the remaining bars. */
      while( today <= endIdx ) {
         sumMFV -= mfv_flow[mfv_Idx];
         sumVol -= mfv_volume[mfv_Idx];
         high = inHigh[today];
         low = inLow[today];
         close = inClose[today];
         tmp = high - low;
         if( tmp > 0.0 ) {
            mfv = (close - low - (high - close)) / tmp * inVolume[today];
         } else {
            mfv = 0.0;
         }
         mfv_flow[mfv_Idx] = mfv;
         mfv_volume[mfv_Idx] = inVolume[today];
         sumMFV += mfv;
         sumVol += inVolume[today];
         today += 1;
         if( sumVol > 0.0 ) {
            outReal[outIdx++ * outStride] = sumMFV / sumVol;
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
         mfv_Idx++;
         if( mfv_Idx > maxIdx_mfv ) { mfv_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capCb_mfv = maxIdx_mfv + 1;
      if( capCb_mfv > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.sumMFV = sumMFV;
      sp.sumVol = sumVol;
      sp.mfv_Idx = mfv_Idx;
      sp.maxIdx_mfv = maxIdx_mfv;
      sp.cbSize_mfv = capCb_mfv;
      sp.cb_mfv_flow = mfv_flow;
      sp.cb_mfv_volume = mfv_volume;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cmfOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CmfStream cmfOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CmfStream sp = new CmfStream(this);
      RetCode retCode = cmfOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CMF openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CMF openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CMF openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cmfOpen (composition seam). */
   CmfStream cmfOpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      CmfStream sp = new CmfStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = cmfOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CMF open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CMF open: internal error", retCode);
      }
      throw new TaLibArgumentException("CMF open: " + retCode, retCode);
   }
   /**
    * Open a live CMF stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CMF} at that bar.
    * <p>The history must hold at least {@code CMF_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CmfStream cmfOpen( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod )
   {
      requireArgument("CMF open", "inHigh", inHigh);
      requireHistory("CMF open", inHigh.length);
      requireArgument("CMF open", "inLow", inLow);
      requireArgument("CMF open", "inClose", inClose);
      requireArgument("CMF open", "inVolume", inVolume);
      requireHistoryLength("CMF open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("CMF open", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("CMF open", "inVolume", inVolume.length, inHigh.length);
      return cmfOpenInternal(inHigh, inLow, inClose, inVolume, 0, optInTimePeriod);
   }
   /**
    * {@link Core#cmfOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CMF} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CmfStream#outRange()}.
    */
   public CmfStream cmfOpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("CMF openAndFill", "inHigh", inHigh);
      requireHistory("CMF openAndFill", inHigh.length);
      requireArgument("CMF openAndFill", "inLow", inLow);
      requireArgument("CMF openAndFill", "inClose", inClose);
      requireArgument("CMF openAndFill", "inVolume", inVolume);
      int guardOutLen = openFillCount("CMF openAndFill", inHigh.length, CMF_Lookback(optInTimePeriod));
      requireHistoryLength("CMF openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("CMF openAndFill", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("CMF openAndFill", "inVolume", inVolume.length, inHigh.length);
      requireLength("CMF openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("CMF openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cmfOpenAndFillInternal(inHigh, inLow, inClose, inVolume, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
