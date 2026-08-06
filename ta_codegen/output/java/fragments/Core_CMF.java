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
    * Number of leading input bars {@link Core#cmf} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the window (default 20; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int cmfLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode cmfInternal( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
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
   RetCode cmfInternal( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
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
    * valid range shorter than {@link Core#cmfLookback} is a <b>success with no
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
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#ad
    * @see Core#adOsc
    * @see Core#mfi
    * @see Core#obv
    */
   public OutRange cmf( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        double inVolume[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cmfInternal(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
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
    * valid range shorter than {@link Core#cmfLookback} is a <b>success with no
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
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#ad
    * @see Core#adOsc
    * @see Core#mfi
    * @see Core#obv
    */
   public OutRange cmf( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        float inVolume[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cmfInternal(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CMF", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CMF stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cmf} over the same series.
    * Open with {@link Core#cmfOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class CmfStream {
      final Core core;
      int optInTimePeriod;
      double sumMFV;
      double sumVol;
      double high;
      double low;
      double close;
      double tmp;
      double mfv;
      int mfv_Idx;
      int maxIdx_mfv;
      int cbSize_mfv;
      double[] cb_mfv_flow;
      double[] cb_mfv_volume;
      double cur_outReal;
      OutRange fillRange;

      CmfStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#cmfOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      CmfStream( CmfStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.sumMFV = other.sumMFV;
         this.sumVol = other.sumVol;
         this.high = other.high;
         this.low = other.low;
         this.close = other.close;
         this.tmp = other.tmp;
         this.mfv = other.mfv;
         this.mfv_Idx = other.mfv_Idx;
         this.maxIdx_mfv = other.maxIdx_mfv;
         this.cbSize_mfv = other.cbSize_mfv;
         this.cb_mfv_flow = other.cb_mfv_flow.clone();
         this.cb_mfv_volume = other.cb_mfv_volume.clone();
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow, double inClose, double inVolume ) {
         core.cmfStreamStep(this, inHigh, inLow, inClose, inVolume);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow, double inClose, double inVolume ) {
         CmfStream scratch = new CmfStream(this);
         core.cmfStreamStep(scratch, inHigh, inLow, inClose, inVolume);
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
      public CmfStream copy() {
         return new CmfStream(this);
      }
   }
   void cmfStreamStep( CmfStream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      sp.sumMFV -= sp.cb_mfv_flow[sp.mfv_Idx];
      sp.sumVol -= sp.cb_mfv_volume[sp.mfv_Idx];
      sp.high = inHigh;
      sp.low = inLow;
      sp.close = inClose;
      sp.tmp = sp.high - sp.low;
      if( sp.tmp > 0.0 ) {
         sp.mfv = (sp.close - sp.low - (sp.high - sp.close)) / sp.tmp * inVolume;
      } else {
         sp.mfv = 0.0;
      }
      sp.cb_mfv_flow[sp.mfv_Idx] = sp.mfv;
      sp.cb_mfv_volume[sp.mfv_Idx] = inVolume;
      sp.sumMFV += sp.mfv;
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
   private RetCode cmfOpenBody( CmfStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
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
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
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
         lastValue_outReal = sumMFV / sumVol;
      } else {
         lastValue_outReal = 0.0;
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
            lastValue_outReal = sumMFV / sumVol;
         } else {
            lastValue_outReal = 0.0;
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
      sp.high = high;
      sp.low = low;
      sp.close = close;
      sp.tmp = tmp;
      sp.mfv = mfv;
      sp.mfv_Idx = mfv_Idx;
      sp.maxIdx_mfv = maxIdx_mfv;
      sp.cbSize_mfv = capCb_mfv;
      sp.cb_mfv_flow = mfv_flow;
      sp.cb_mfv_volume = mfv_volume;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode cmfOpenAndFillBody( CmfStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
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
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
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
      /* Capture the live batch state into the handle. */
      int capCb_mfv = maxIdx_mfv + 1;
      if( capCb_mfv > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.sumMFV = sumMFV;
      sp.sumVol = sumVol;
      sp.high = high;
      sp.low = low;
      sp.close = close;
      sp.tmp = tmp;
      sp.mfv = mfv;
      sp.mfv_Idx = mfv_Idx;
      sp.maxIdx_mfv = maxIdx_mfv;
      sp.cbSize_mfv = capCb_mfv;
      sp.cb_mfv_flow = mfv_flow;
      sp.cb_mfv_volume = mfv_volume;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cmfOpen (composition seam). */
   CmfStream cmfOpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      CmfStream sp = new CmfStream(this);
      RetCode retCode = cmfOpenBody(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CMF open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CMF open: internal error");
      }
      throw new IllegalArgumentException("TA_CMF open: " + retCode);
   }
   /**
    * Open a live CMF stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cmf} at that bar.
    * <p>The history must hold at least {@code cmfLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CmfStream cmfOpen( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod )
   {
      return cmfOpenInternal(inHigh, inLow, inClose, inVolume, 0, optInTimePeriod);
   }
   /**
    * {@link Core#cmfOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cmf} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CmfStream#fillRange()}.
    */
   public CmfStream cmfOpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod, double outReal[] )
   {
      CmfStream sp = new CmfStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cmfOpenAndFillBody(sp, inHigh, inLow, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CMF openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CMF openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CMF openAndFill: " + retCode);
   }
