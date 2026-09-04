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
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  071126 MF,CC  Inline the fixed-26/12 MACD lockstep pass (was a
 *                delegation to macd(...,0,0,...)); bit-exact, streamable.
 *  080926 MF,CC  Explicit no-smoothing signal at a signal period of 1.
 */

   /**
    * Number of leading input bars {@link Core#MACDFIX} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInSignalPeriod Smoothing period for the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MACDFIX_Lookback( int optInSignalPeriod )
   {
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      /* The lookback is driven by the signal line output.
       *
       * (must also account for the initial data consume
       *  by the fix 26 period EMA).
       */
      return EMA_Lookback(26) + EMA_Lookback(optInSignalPeriod) ;

   }
   RetCode MACDFIX_Impl( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInSignalPeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outMACD[],
                         double outMACDSignal[],
                         double outMACDHist[] )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevSignal = 0;
      double macdValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int optInFastPeriod = 0;
      int optInSlowPeriod = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      optInFastPeriod = 12;
      optInSlowPeriod = 26;
      /* MACDFIX is the fixed 26/12 MACD: the fast/slow periods and their
       * smoothing factors are hardcoded (the general MACD selects these
       * exact values when its fast/slow period arguments are 0). Only the
       * signal period is caller-provided.
       *    Fix 12 -> fastK = 0.15
       *    Fix 26 -> slowK = 0.075
       */
      fastK = 0.15;
      slowK = 0.075;
      /* A signal period of 1 disables signal-line smoothing: the signal IS the
       * MACD line and the histogram is exactly zero. signalK is then exactly
       * 1.0, so the recursion below reduces to (x-prev)+prev -- which returns x
       * only while consecutive MACD-line values stay within a factor of two of
       * each other. The MACD line oscillates through zero, so it leaves that
       * window on ordinary data; hence the explicit arm at each step.
       */
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = EMA_Lookback(optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = lookbackSignal;
      lookbackTotal += EMA_Lookback(26);
      /* fixed slow period */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Everything is computed in a single lockstep pass: each bar
       * advances the fast and slow EMA (two independent recursions),
       * their difference is the MACD line, and each MACD-line value
       * is immediately fed into the signal EMA. No temporary buffers.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum of
       *    its first 'period' inputs, accumulated from 0.0 in input
       *    order, divided by the period. The fast and slow seed
       *    windows end on the same bar. The signal EMA is seeded the
       *    same way from the first 'signal period' MACD-line values.
       *  - Metastock compatibility: the fast and slow EMA are seeded
       *    from inReal[0], the signal EMA from the first MACD-line
       *    value.
       * Output alignment is identical for all compatibility modes;
       * only the seed values differ.
       *
       * In-place (an output == inReal) is supported: outputs at
       * [outIdx] are written only after inReal[startIdx+outIdx] was
       * read.
       */
      /* Seed each price EMA with a simple average of its first
       * 'period' price bars. The fast window is the tail of the
       * slow window: consume the leading slow-only bars first,
       * then accumulate both over the shared bars.
       */
      today = startIdx - lookbackTotal;
      tempReal = 0.0;
      i = optInSlowPeriod - optInFastPeriod;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevFast = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         prevFast += inReal[today];
         tempReal += inReal[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      prevFast = prevFast / optInFastPeriod;
      /* Advance both EMA through their unstable period, up to the
       * first MACD-line bar.
       */
      while( today <= startIdx - lookbackSignal ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
      }
      macdValue = prevFast - prevSlow;
      /* Seed the signal EMA with a simple average of the first
       * 'signal period' MACD-line values, accumulated as they are
       * produced.
       */
      prevSignal = 0.0;
      prevSignal += macdValue;
      i = optInSignalPeriod - 1;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal += macdValue;
      }
      prevSignal = prevSignal / optInSignalPeriod;
      /* Advance everything in lockstep through the unstable period
       * of the signal EMA, up to the first output bar.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
      }
      /* Stable zone: keep advancing in lockstep and write the three
       * outputs.
       */
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode MACDFIX_Impl( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInSignalPeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outMACD[],
                         double outMACDSignal[],
                         double outMACDHist[] )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevSignal = 0;
      double macdValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int optInFastPeriod = 0;
      int optInSlowPeriod = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      optInFastPeriod = 12;
      optInSlowPeriod = 26;
      fastK = 0.15;
      slowK = 0.075;
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = EMA_Lookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += EMA_Lookback(26);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      tempReal = 0.0;
      i = optInSlowPeriod - optInFastPeriod;
      while( i-- > 0 ) {
         tempReal += (double)inReal[today++];
      }
      prevFast = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         prevFast += (double)inReal[today];
         tempReal += (double)inReal[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      prevFast = prevFast / optInFastPeriod;
      while( today <= startIdx - lookbackSignal ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
      }
      macdValue = prevFast - prevSlow;
      prevSignal = 0.0;
      prevSignal += macdValue;
      i = optInSignalPeriod - 1;
      while( i-- > 0 ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal += macdValue;
      }
      prevSignal = prevSignal / optInSignalPeriod;
      while( today <= startIdx ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
      }
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * MACD with the fast/slow EMAs fixed to the classic 12/26 periods (with the
    * classic fixed smoothing factors 0.15 and 0.075), exposing only the signal
    * period. Signal-line crossovers and histogram sign flag momentum shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * MACD = EMA_12 - EMA_26   (fixed k: 0.15 for 12, 0.075 for 26)
    * Signal = EMA(MACD, signalPeriod),  k = 2/(signalPeriod+1)
    * Hist = MACD - Signal
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A signal period of 1 disables signal-line smoothing: the signal equals the MACD line and the histogram is zero. Before 0.6.5 this parameter value produced misaligned output (issues #48/#59).</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MACDFIX_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series (typically close)
    * @param optInSignalPeriod Smoothing period for the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outMACD Fixed EMA12 minus EMA26. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDSignal EMA of the MACD line. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDHist MACD minus signal. Must hold at least
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
    * @see Core#MACD
    * @see Core#MACDEXT
    * @see Core#EMA
    * @see Core#APO
    */
   public OutRange MACDFIX( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInSignalPeriod,
                            double outMACD[],
                            double outMACDSignal[],
                            double outMACDHist[] )
   {
      requireIndexRange("MACDFIX", startIdx, endIdx);
      int guardStart = clampedStart("MACDFIX", startIdx, MACDFIX_Lookback(optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MACDFIX", "inReal", inReal, guardInLen);
      requireLength("MACDFIX", "outMACD", outMACD, guardOutLen);
      requireLength("MACDFIX", "outMACDSignal", outMACDSignal, guardOutLen);
      requireLength("MACDFIX", "outMACDHist", outMACDHist, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MACDFIX_Impl(startIdx, endIdx, inReal, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      if( retCode != RetCode.Success ) {
         throw failure("MACDFIX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * MACD with the fast/slow EMAs fixed to the classic 12/26 periods (with the
    * classic fixed smoothing factors 0.15 and 0.075), exposing only the signal
    * period. Signal-line crossovers and histogram sign flag momentum shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * MACD = EMA_12 - EMA_26   (fixed k: 0.15 for 12, 0.075 for 26)
    * Signal = EMA(MACD, signalPeriod),  k = 2/(signalPeriod+1)
    * Hist = MACD - Signal
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A signal period of 1 disables signal-line smoothing: the signal equals the MACD line and the histogram is zero. Before 0.6.5 this parameter value produced misaligned output (issues #48/#59).</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MACDFIX_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series (typically close)
    * @param optInSignalPeriod Smoothing period for the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outMACD Fixed EMA12 minus EMA26. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDSignal EMA of the MACD line. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDHist MACD minus signal. Must hold at least
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
    * @see Core#MACD
    * @see Core#MACDEXT
    * @see Core#EMA
    * @see Core#APO
    */
   public OutRange MACDFIX( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInSignalPeriod,
                            double outMACD[],
                            double outMACDSignal[],
                            double outMACDHist[] )
   {
      requireIndexRange("MACDFIX", startIdx, endIdx);
      int guardStart = clampedStart("MACDFIX", startIdx, MACDFIX_Lookback(optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MACDFIX", "inReal", inReal, guardInLen);
      requireLength("MACDFIX", "outMACD", outMACD, guardOutLen);
      requireLength("MACDFIX", "outMACDSignal", outMACDSignal, guardOutLen);
      requireLength("MACDFIX", "outMACDHist", outMACDHist, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MACDFIX_Impl(startIdx, endIdx, inReal, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      if( retCode != RetCode.Success ) {
         throw failure("MACDFIX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MACDFIX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MACDFIX} over the same series.
    * Open with {@link Core#macdfixOpen}; there is no close — the handle is
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
   public static final class MacdfixStream {
      Core core;
      int optInSignalPeriod;
      double prevFast;
      double prevSlow;
      double prevSignal;
      double slowK;
      double fastK;
      double signalK;
      double cur_outMACD;
      double cur_outMACDSignal;
      double cur_outMACDHist;
      int outRangeBegIdx;
      int outRangeCount;

      MacdfixStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MACDFIX} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MacdfixStream( MacdfixStream other ) {
         this.core = other.core;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.prevFast = other.prevFast;
         this.prevSlow = other.prevSlow;
         this.prevSignal = other.prevSignal;
         this.slowK = other.slowK;
         this.fastK = other.fastK;
         this.signalK = other.signalK;
         this.cur_outMACD = other.cur_outMACD;
         this.cur_outMACDSignal = other.cur_outMACDSignal;
         this.cur_outMACDHist = other.cur_outMACDHist;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(MacdfixOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inReal, MacdfixOut out ) {
         requireArgument("MACDFIX update", "out", out);
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("MACDFIX update: BadParam", RetCode.BadParam);
         }
         core.macdfixStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.macd = this.cur_outMACD;
         out.macdSignal = this.cur_outMACDSignal;
         out.macdHist = this.cur_outMACDHist;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal[], double outMACD[], double outMACDSignal[], double outMACDHist[] ) {
         requireArgument("MACDFIX updateAndFill", "inReal", inReal);
         requireArgument("MACDFIX updateAndFill", "outMACD", outMACD);
         requireArgument("MACDFIX updateAndFill", "outMACDSignal", outMACDSignal);
         requireArgument("MACDFIX updateAndFill", "outMACDHist", outMACDHist);
         final int barCount = inReal.length;
         if( outMACD.length < barCount || outMACDSignal.length < barCount || outMACDHist.length < barCount || (Object)outMACD == (Object)inReal || (Object)outMACDSignal == (Object)inReal || (Object)outMACDHist == (Object)inReal || (Object)outMACD == (Object)outMACDSignal || (Object)outMACD == (Object)outMACDHist || (Object)outMACDSignal == (Object)outMACDHist )
            throw new TaLibArgumentException("MACDFIX updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("MACDFIX updateAndFill: BadParam", RetCode.BadParam);
            }
            core.macdfixStepImpl(this, inReal[i]);
            outMACD[i] = this.cur_outMACD;
            outMACDSignal[i] = this.cur_outMACDSignal;
            outMACDHist[i] = this.cur_outMACDHist;
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
      public void peek( double inReal, MacdfixOut out ) {
         requireArgument("MACDFIX peek", "out", out);
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MACDFIX peek: BadParam", RetCode.BadParam);
         MacdfixStream sp = this;
         double macdValue = 0.0;
         double tempReal = 0.0;
         double cur_outMACD = 0.0;
         double cur_outMACDHist = 0.0;
         double cur_outMACDSignal = 0.0;
         double prevFast = sp.prevFast;
         double prevSignal = sp.prevSignal;
         double prevSlow = sp.prevSlow;
         tempReal = inReal;
         prevFast = Math.fma(tempReal - prevFast, sp.fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, sp.slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( sp.optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, sp.signalK, prevSignal);
         }
         cur_outMACD = macdValue;
         cur_outMACDSignal = prevSignal;
         cur_outMACDHist = macdValue - prevSignal;
         out.macd = cur_outMACD;
         out.macdSignal = cur_outMACDSignal;
         out.macdHist = cur_outMACDHist;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( MacdfixOut out ) {
         requireArgument("MACDFIX value", "out", out);
         out.macd = this.cur_outMACD;
         out.macdSignal = this.cur_outMACDSignal;
         out.macdHist = this.cur_outMACDHist;
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
      public MacdfixStream clone() {
         return new MacdfixStream(this);
      }
   }

   /**
    * The outputs of one MACDFIX bar, written by the stream into an object the
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
   public static final class MacdfixOut {
      /** Fixed EMA12 minus EMA26. */
      public double macd;
      /** EMA of the MACD line. */
      public double macdSignal;
      /** MACD minus signal. */
      public double macdHist;
   }
   void macdfixStepImpl( MacdfixStream sp, double inReal )
   {
      double macdValue = 0.0;
      double tempReal = 0.0;
      tempReal = inReal;
      sp.prevFast = Math.fma(tempReal - sp.prevFast, sp.fastK, sp.prevFast);
      sp.prevSlow = Math.fma(tempReal - sp.prevSlow, sp.slowK, sp.prevSlow);
      macdValue = sp.prevFast - sp.prevSlow;
      if( sp.optInSignalPeriod == 1 ) {
         sp.prevSignal = macdValue;
      } else {
         sp.prevSignal = Math.fma(macdValue - sp.prevSignal, sp.signalK, sp.prevSignal);
      }
      sp.cur_outMACD = macdValue;
      sp.cur_outMACDSignal = sp.prevSignal;
      sp.cur_outMACDHist = macdValue - sp.prevSignal;
   }
   private RetCode macdfixOpenImpl( MacdfixStream sp, double inReal[], int startIdx, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[], int outStride )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevSignal = 0;
      double macdValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int optInFastPeriod = 0;
      int optInSlowPeriod = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      optInFastPeriod = 12;
      optInSlowPeriod = 26;
      /* MACDFIX is the fixed 26/12 MACD: the fast/slow periods and their
       * smoothing factors are hardcoded (the general MACD selects these
       * exact values when its fast/slow period arguments are 0). Only the
       * signal period is caller-provided.
       *    Fix 12 -> fastK = 0.15
       *    Fix 26 -> slowK = 0.075
       */
      fastK = 0.15;
      slowK = 0.075;
      /* A signal period of 1 disables signal-line smoothing: the signal IS the
       * MACD line and the histogram is exactly zero. signalK is then exactly
       * 1.0, so the recursion below reduces to (x-prev)+prev -- which returns x
       * only while consecutive MACD-line values stay within a factor of two of
       * each other. The MACD line oscillates through zero, so it leaves that
       * window on ordinary data; hence the explicit arm at each step.
       */
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = EMA_Lookback(optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = lookbackSignal;
      lookbackTotal += EMA_Lookback(26);
      /* fixed slow period */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Everything is computed in a single lockstep pass: each bar
       * advances the fast and slow EMA (two independent recursions),
       * their difference is the MACD line, and each MACD-line value
       * is immediately fed into the signal EMA. No temporary buffers.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum of
       *    its first 'period' inputs, accumulated from 0.0 in input
       *    order, divided by the period. The fast and slow seed
       *    windows end on the same bar. The signal EMA is seeded the
       *    same way from the first 'signal period' MACD-line values.
       *  - Metastock compatibility: the fast and slow EMA are seeded
       *    from inReal[0], the signal EMA from the first MACD-line
       *    value.
       * Output alignment is identical for all compatibility modes;
       * only the seed values differ.
       *
       * In-place (an output == inReal) is supported: outputs at
       * [outIdx] are written only after inReal[startIdx+outIdx] was
       * read.
       */
      /* Seed each price EMA with a simple average of its first
       * 'period' price bars. The fast window is the tail of the
       * slow window: consume the leading slow-only bars first,
       * then accumulate both over the shared bars.
       */
      today = startIdx - lookbackTotal;
      tempReal = 0.0;
      i = optInSlowPeriod - optInFastPeriod;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevFast = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         prevFast += inReal[today];
         tempReal += inReal[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      prevFast = prevFast / optInFastPeriod;
      /* Advance both EMA through their unstable period, up to the
       * first MACD-line bar.
       */
      while( today <= startIdx - lookbackSignal ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
      }
      macdValue = prevFast - prevSlow;
      /* Seed the signal EMA with a simple average of the first
       * 'signal period' MACD-line values, accumulated as they are
       * produced.
       */
      prevSignal = 0.0;
      prevSignal += macdValue;
      i = optInSignalPeriod - 1;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal += macdValue;
      }
      prevSignal = prevSignal / optInSignalPeriod;
      /* Advance everything in lockstep through the unstable period
       * of the signal EMA, up to the first output bar.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
      }
      /* Stable zone: keep advancing in lockstep and write the three
       * outputs.
       */
      outMACD[0 * outStride] = macdValue;
      outMACDSignal[0 * outStride] = prevSignal;
      outMACDHist[0 * outStride] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
         outMACD[outIdx * outStride] = macdValue;
         outMACDSignal[outIdx * outStride] = prevSignal;
         outMACDHist[outIdx * outStride] = macdValue - prevSignal;
         outIdx += 1;
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.prevFast = prevFast;
      sp.prevSlow = prevSlow;
      sp.prevSignal = prevSignal;
      sp.slowK = slowK;
      sp.fastK = fastK;
      sp.signalK = signalK;
      sp.cur_outMACD = outMACD[(outNBElement.value - 1) * outStride];
      sp.cur_outMACDSignal = outMACDSignal[(outNBElement.value - 1) * outStride];
      sp.cur_outMACDHist = outMACDHist[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* macdfixOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MacdfixStream macdfixOpenAndFillInternal( double inReal[], int startIdx, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[] )
   {
      MacdfixStream sp = new MacdfixStream(this);
      RetCode retCode = macdfixOpenImpl(sp, inReal, startIdx, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MACDFIX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MACDFIX openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MACDFIX openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind macdfixOpen (composition seam). */
   MacdfixStream macdfixOpenInternal( double inReal[], int startIdx, int optInSignalPeriod )
   {
      MacdfixStream sp = new MacdfixStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outMACD = new double[1];
      double[] sink_outMACDSignal = new double[1];
      double[] sink_outMACDHist = new double[1];
      RetCode retCode = macdfixOpenImpl(sp, inReal, startIdx, optInSignalPeriod, outBegIdx, outNBElement, sink_outMACD, sink_outMACDSignal, sink_outMACDHist, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MACDFIX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MACDFIX open: internal error", retCode);
      }
      throw new TaLibArgumentException("MACDFIX open: " + retCode, retCode);
   }
   /**
    * Open a live MACDFIX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MACDFIX} at that bar.
    * <p>The history must hold at least {@code MACDFIX_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MacdfixStream macdfixOpen( double inReal[], int optInSignalPeriod )
   {
      requireArgument("MACDFIX open", "inReal", inReal);
      requireHistory("MACDFIX open", inReal.length);
      return macdfixOpenInternal(inReal, 0, optInSignalPeriod);
   }
   /**
    * {@link Core#macdfixOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MACDFIX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MacdfixStream#outRange()}.
    */
   public MacdfixStream macdfixOpenAndFill( double inReal[], int optInSignalPeriod, double outMACD[], double outMACDSignal[], double outMACDHist[] )
   {
      requireArgument("MACDFIX openAndFill", "inReal", inReal);
      requireHistory("MACDFIX openAndFill", inReal.length);
      int guardOutLen = openFillCount("MACDFIX openAndFill", inReal.length, MACDFIX_Lookback(optInSignalPeriod));
      requireLength("MACDFIX openAndFill", "outMACD", outMACD, guardOutLen);
      requireLength("MACDFIX openAndFill", "outMACDSignal", outMACDSignal, guardOutLen);
      requireLength("MACDFIX openAndFill", "outMACDHist", outMACDHist, guardOutLen);
      if( (Object)outMACD == (Object)inReal || (Object)outMACDSignal == (Object)inReal || (Object)outMACDHist == (Object)inReal || (Object)outMACD == (Object)outMACDSignal || (Object)outMACD == (Object)outMACDHist || (Object)outMACDSignal == (Object)outMACDHist ) {
         throw new TaLibArgumentException("MACDFIX openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return macdfixOpenAndFillInternal(inReal, 0, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
   }
