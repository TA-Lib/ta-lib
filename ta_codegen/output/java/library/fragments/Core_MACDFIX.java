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
 */

   /**
    * Number of leading input bars {@link Core#macdFix} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInSignalPeriod Smoothing period for the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int macdFixLookback( int optInSignalPeriod )
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
      return emaLookback(26) + emaLookback(optInSignalPeriod) ;

   }
   RetCode macdFixInternal( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(26);
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
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
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
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
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
   RetCode macdFixUnguardedInternal( int startIdx,
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
      optInFastPeriod = 12;
      optInSlowPeriod = 26;
      fastK = 0.15;
      slowK = 0.075;
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(26);
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
      while( today <= startIdx - lookbackSignal ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
      }
      macdValue = prevFast - prevSlow;
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
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
      }
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode macdFixInternal( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(26);
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
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
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
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode macdFixUnguardedInternal( int startIdx,
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
      optInFastPeriod = 12;
      optInSlowPeriod = 26;
      fastK = 0.15;
      slowK = 0.075;
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(26);
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
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
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
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
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
    * valid range shorter than {@link Core#macdFixLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
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
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#macd
    * @see Core#macdExt
    * @see Core#ema
    * @see Core#apo
    */
   public OutRange macdFix( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInSignalPeriod,
                            double outMACD[],
                            double outMACDSignal[],
                            double outMACDHist[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = macdFixInternal(startIdx, endIdx, inReal, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      if( retCode != RetCode.Success ) {
         throw failure("MACDFIX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * MACD with the fast/slow EMAs fixed to the classic 12/26 periods (with the
    * classic fixed smoothing factors 0.15 and 0.075), exposing only the signal
    * period. Signal-line crossovers and histogram sign flag momentum shifts. —
    * <b>unchecked</b> variant of {@link Core#macdFix}.
    * <p>Validates nothing and never throws. The caller guarantees: non-negative
    * {@code startIdx}, {@code endIdx >= startIdx}, non-null arrays, output
    * arrays distinct from each other, and every optional parameter already
    * resolved and within its documented range — a sentinel such as
    * {@code Integer.MIN_VALUE} is <b>not</b> substituted here.
    * <p>Breaking any of those yields an empty {@link OutRange} or undefined
    * output rather than a diagnostic. (C and Rust return a status code from
    * this tier, so their callers can detect it; this one has nowhere to report
    * it.) Use the guarded method unless the arguments are already known good.
    *
    * @return The range written, exactly as the guarded method reports it.
    */
   public OutRange macdFixUnguarded( int startIdx,
                                     int endIdx,
                                     double inReal[],
                                     int optInSignalPeriod,
                                     double outMACD[],
                                     double outMACDSignal[],
                                     double outMACDHist[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      macdFixUnguardedInternal(startIdx, endIdx, inReal, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
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
    * valid range shorter than {@link Core#macdFixLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
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
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#macd
    * @see Core#macdExt
    * @see Core#ema
    * @see Core#apo
    */
   public OutRange macdFix( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInSignalPeriod,
                            double outMACD[],
                            double outMACDSignal[],
                            double outMACDHist[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = macdFixInternal(startIdx, endIdx, inReal, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      if( retCode != RetCode.Success ) {
         throw failure("MACDFIX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * MACD with the fast/slow EMAs fixed to the classic 12/26 periods (with the
    * classic fixed smoothing factors 0.15 and 0.075), exposing only the signal
    * period. Signal-line crossovers and histogram sign flag momentum shifts. —
    * <b>unchecked</b> variant of {@link Core#macdFix}.
    * <p>Validates nothing and never throws. The caller guarantees: non-negative
    * {@code startIdx}, {@code endIdx >= startIdx}, non-null arrays, output
    * arrays distinct from each other, and every optional parameter already
    * resolved and within its documented range — a sentinel such as
    * {@code Integer.MIN_VALUE} is <b>not</b> substituted here.
    * <p>Breaking any of those yields an empty {@link OutRange} or undefined
    * output rather than a diagnostic. (C and Rust return a status code from
    * this tier, so their callers can detect it; this one has nowhere to report
    * it.) Use the guarded method unless the arguments are already known good.
    * <p>This is the {@code float[]} overload; see the guarded method.
    *
    * @return The range written, exactly as the guarded method reports it.
    */
   public OutRange macdFixUnguarded( int startIdx,
                                     int endIdx,
                                     float inReal[],
                                     int optInSignalPeriod,
                                     double outMACD[],
                                     double outMACDSignal[],
                                     double outMACDHist[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      macdFixUnguardedInternal(startIdx, endIdx, inReal, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MACDFIX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#macdFix} over the same series.
    * Open with {@link Core#macdFixOpen}; there is no close — the handle is
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
   public static final class MacdFixStream {
      final Core core;
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
      Value cachedValue;
      OutRange fillRange;

      MacdFixStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#macdFixOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      MacdFixStream( MacdFixStream other ) {
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
         this.cachedValue = other.cachedValue;
         this.fillRange = other.fillRange;
      }

      /** One output set, in batch output order. Immutable. */
      public static final class Value {
         public final double macd;
         public final double macdSignal;
         public final double macdHist;
         Value( double macd, double macdSignal, double macdHist ) {
            this.macd = macd;
            this.macdSignal = macdSignal;
            this.macdHist = macdHist;
         }
         @Override public String toString() {
            return "Value[" + "macd=" + macd + ", " + "macdSignal=" + macdSignal + ", " + "macdHist=" + macdHist + "]";
         }
         @Override public boolean equals( Object o ) {
            if( !(o instanceof Value) ) return false;
            Value v = (Value) o;
            return Double.doubleToLongBits(this.macd) == Double.doubleToLongBits(v.macd) && Double.doubleToLongBits(this.macdSignal) == Double.doubleToLongBits(v.macdSignal) && Double.doubleToLongBits(this.macdHist) == Double.doubleToLongBits(v.macdHist);
         }
         @Override public int hashCode() {
            int h = 17;
            h = 31 * h + Double.hashCode(macd);
            h = 31 * h + Double.hashCode(macdSignal);
            h = 31 * h + Double.hashCode(macdHist);
            return h;
         }
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inReal ) {
         core.macdFixStreamStep(this, inReal);
         this.cachedValue = new Value(this.cur_outMACD, this.cur_outMACDSignal, this.cur_outMACDHist);
         return this.cachedValue;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public Value peek( double inReal ) {
         MacdFixStream scratch = new MacdFixStream(this);
         core.macdFixStreamStep(scratch, inReal);
         return new Value(scratch.cur_outMACD, scratch.cur_outMACDSignal, scratch.cur_outMACDHist);
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public Value value() {
         return this.cachedValue;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public MacdFixStream copy() {
         return new MacdFixStream(this);
      }
   }
   void macdFixStreamStep( MacdFixStream sp, double inReal )
   {
      double macdValue = 0.0;
      double tempReal = 0.0;
      tempReal = inReal;
      sp.prevFast = Math.fma(tempReal - sp.prevFast, sp.fastK, sp.prevFast);
      sp.prevSlow = Math.fma(tempReal - sp.prevSlow, sp.slowK, sp.prevSlow);
      macdValue = sp.prevFast - sp.prevSlow;
      sp.prevSignal = Math.fma(macdValue - sp.prevSignal, sp.signalK, sp.prevSignal);
      sp.cur_outMACD = macdValue;
      sp.cur_outMACDSignal = sp.prevSignal;
      sp.cur_outMACDHist = macdValue - sp.prevSignal;
   }
   private RetCode macdFixOpenBody( MacdFixStream sp, double inReal[], int startIdx, int optInSignalPeriod )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outMACD = 0.0;
      double lastValue_outMACDSignal = 0.0;
      double lastValue_outMACDHist = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
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
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(26);
      /* fixed slow period */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
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
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
      }
      /* Stable zone: keep advancing in lockstep and write the three
       * outputs.
       */
      lastValue_outMACD = macdValue;
      lastValue_outMACDSignal = prevSignal;
      lastValue_outMACDHist = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         lastValue_outMACD = macdValue;
         lastValue_outMACDSignal = prevSignal;
         lastValue_outMACDHist = macdValue - prevSignal;
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
      sp.cur_outMACD = lastValue_outMACD;
      sp.cur_outMACDSignal = lastValue_outMACDSignal;
      sp.cur_outMACDHist = lastValue_outMACDHist;
      sp.cachedValue = new MacdFixStream.Value(sp.cur_outMACD, sp.cur_outMACDSignal, sp.cur_outMACDHist);
      return RetCode.Success;
   }
   private RetCode macdFixOpenAndFillBody( MacdFixStream sp, double inReal[], int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[] )
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
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outMACD == (Object)inReal || (Object)outMACDSignal == (Object)inReal || (Object)outMACDHist == (Object)inReal || (Object)outMACD == (Object)outMACDSignal || (Object)outMACD == (Object)outMACDHist || (Object)outMACDSignal == (Object)outMACDHist ) {
         return RetCode.BadParam;
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
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = lookbackSignal;
      lookbackTotal += emaLookback(26);
      /* fixed slow period */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
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
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
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
         prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
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
      sp.cur_outMACD = outMACD[outNBElement.value - 1];
      sp.cur_outMACDSignal = outMACDSignal[outNBElement.value - 1];
      sp.cur_outMACDHist = outMACDHist[outNBElement.value - 1];
      sp.cachedValue = new MacdFixStream.Value(sp.cur_outMACD, sp.cur_outMACDSignal, sp.cur_outMACDHist);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind macdFixOpen (composition seam). */
   MacdFixStream macdFixOpenInternal( double inReal[], int startIdx, int optInSignalPeriod )
   {
      MacdFixStream sp = new MacdFixStream(this);
      RetCode retCode = macdFixOpenBody(sp, inReal, startIdx, optInSignalPeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MACDFIX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MACDFIX open: internal error");
      }
      throw new IllegalArgumentException("TA_MACDFIX open: " + retCode);
   }
   /**
    * Open a live MACDFIX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#macdFix} at that bar.
    * <p>The history must hold at least {@code macdFixLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MacdFixStream macdFixOpen( double inReal[], int optInSignalPeriod )
   {
      return macdFixOpenInternal(inReal, 0, optInSignalPeriod);
   }
   /**
    * {@link Core#macdFixOpen} that also fills the output array(s) bit-identically
    * to {@link Core#macdFix} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MacdFixStream#fillRange()}.
    */
   public MacdFixStream macdFixOpenAndFill( double inReal[], int optInSignalPeriod, double outMACD[], double outMACDSignal[], double outMACDHist[] )
   {
      MacdFixStream sp = new MacdFixStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = macdFixOpenAndFillBody(sp, inReal, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MACDFIX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MACDFIX openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_MACDFIX openAndFill: " + retCode);
   }
