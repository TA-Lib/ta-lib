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
 *  081726 MF,CC  Initial version (#228).
 */

   /**
    * Number of leading input bars {@link Core#AC} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Number of bars in the short moving average of the
    *        median price (default 5; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInSlowPeriod Number of bars in the long moving average of the
    *        median price (default 34; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInSignalPeriod Number of bars in the moving average taken over
    *        the oscillator (default 5; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int AC_Lookback( int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 5;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 34;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 5;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      /* The oscillator's own window, plus the simple moving average taken over
       * the oscillator itself. Both terms are exactly the lookback of the
       * function they come from, so neither is restated here.
       */
      return AO_Lookback(optInFastPeriod, optInSlowPeriod) + SMA_Lookback(optInSignalPeriod) ;

   }
   RetCode AC_Impl( int startIdx,
                    int endIdx,
                    double inHigh[],
                    double inLow[],
                    int optInFastPeriod,
                    int optInSlowPeriod,
                    int optInSignalPeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double sumFast = 0;
      double sumSlow = 0;
      double sumSignal = 0;
      double medianPrice = 0;
      double osc = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingFastIdx = 0;
      int trailingSlowIdx = 0;
      int oscStartIdx = 0;
      int lookbackTotal = 0;
      double[] oscBuffer;
      int oscBuffer_Idx = 0;
      int maxIdx_oscBuffer = (32)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 5;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 34;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 5;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Bill Williams' Accelerator/Decelerator Oscillator (New Trading
       * Dimensions, 1998): how fast the Awesome Oscillator is itself
       * accelerating, drawn as a zero-centred histogram.
       *
       *    AO_t = SMA(median, fast)_t - SMA(median, slow)_t
       *    AC_t = AO_t - SMA(AO, signal)_t
       *
       * Every leg is a plain SMA, so there is no seeding convention to get wrong
       * and no cross-library divergence of the kind an EMA brings.
       *
       * This is ta_codegen/input/ao/ao.c walking one derived series with a third
       * running sum layered on top: the oscillator is never materialised into an
       * array, it goes straight into a ring of the last `signal` values. The
       * order is the one ta_codegen/input/sma/sma.c uses -- add the new value,
       * snapshot the total, subtract the bar leaving the window, divide by the
       * period -- for all three sums, which is what makes the composed reference
       * in test_composite.c (TA_AO, TA_SMA and TA_SUB) bit-exact rather than
       * merely close. Any future drift in either path is then a hard failure
       * instead of a tolerance argument.
       *
       * Each total is DIVIDED by its period; it is not multiplied by a
       * precomputed 1/period, for the reason spelled out in ao.c and #117/#118.
       */
      /* This ptr will point on a circular buffer of at least
       * "optInSignalPeriod" element.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = AC_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Allocate a circular buffer equal to the requested signal period. */
      if( optInSignalPeriod < 1 ) return RetCode.InternalError;
      oscBuffer = new double[optInSignalPeriod];
      maxIdx_oscBuffer = (optInSignalPeriod)-1;
      oscBuffer_Idx = 0;
      /* The first bar the oscillator is evaluated on for this call. It trails
       * the first output by the signal window, because that many oscillator
       * values have to exist before their average does.
       */
      oscStartIdx = startIdx - (optInSignalPeriod - 1);
      sumFast = 0.0;
      sumSlow = 0.0;
      sumSignal = 0.0;
      trailingFastIdx = oscStartIdx - (optInFastPeriod - 1);
      trailingSlowIdx = oscStartIdx - (optInSlowPeriod - 1);
      /* Add-up both initial periods, except for the last value.
       *
       * One pass over the longer warm-up window replaces two overlapping passes.
       * A bar inside the shorter window is added to that total as it is reached,
       * so each total still accumulates exactly the same bars in exactly the same
       * ascending order two separate loops would have given it -- which is what
       * keeps each leg bit-identical to the TA_SMA that TA_AO would have called
       * had it been started at oscStartIdx.
       */
      i = startIdx - lookbackTotal;
      while( i < oscStartIdx ) {
         medianPrice = (inHigh[i] + inLow[i]) / 2.0;
         if( i >= trailingFastIdx ) {
            sumFast += medianPrice;
         }
         if( i >= trailingSlowIdx ) {
            sumSlow += medianPrice;
         }
         i = i + 1;
      }
      /* Fill the signal ring with the oscillator values that precede the first
       * output. This is the same body as the main loop below minus the store --
       * the window is not full yet, so there is nothing to emit.
       */
      while( i < startIdx ) {
         medianPrice = (inHigh[i] + inLow[i]) / 2.0;
         sumFast += medianPrice;
         sumSlow += medianPrice;
         osc = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
         sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
         sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
         trailingFastIdx = trailingFastIdx + 1;
         trailingSlowIdx = trailingSlowIdx + 1;
         i = i + 1;
         oscBuffer[oscBuffer_Idx] = osc;
         sumSignal += osc;
         oscBuffer_Idx++;
         if( oscBuffer_Idx > maxIdx_oscBuffer ) { oscBuffer_Idx = 0; }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows outReal to be the same
       * buffer as either input.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         medianPrice = (inHigh[i] + inLow[i]) / 2.0;
         sumFast += medianPrice;
         sumSlow += medianPrice;
         /* Snapshot the oscillator before either total drops its trailing bar,
          * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
          */
         osc = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
         sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
         sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
         trailingFastIdx = trailingFastIdx + 1;
         trailingSlowIdx = trailingSlowIdx + 1;
         i = i + 1;
         /* Today's oscillator enters the signal window at its own slot, and the
          * bar leaving that window is read only after the ring has advanced onto
          * it -- writing first is what makes the slot the loop is about to
          * overwrite the newest value rather than the oldest one.
          */
         oscBuffer[oscBuffer_Idx] = osc;
         sumSignal += osc;
         tempReal = osc - sumSignal / (double)optInSignalPeriod;
         oscBuffer_Idx++;
         if( oscBuffer_Idx > maxIdx_oscBuffer ) { oscBuffer_Idx = 0; }
         sumSignal -= oscBuffer[oscBuffer_Idx];
         /* Every input read for this bar is done above, so the store is safe
          * when the caller aliases outReal over inHigh or inLow. Unlike ao.c
          * there is slack here -- the signal window puts both trailing indices
          * at least optInSignalPeriod-1 bars ahead of outIdx, so no reachable
          * parameter makes them collide -- but the order is kept anyway, so
          * that admitting a signal period of 1 would not silently reintroduce
          * the collision ao.c has to guard against.
          */
         outReal[outIdx] = tempReal;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode AC_Impl( int startIdx,
                    int endIdx,
                    float inHigh[],
                    float inLow[],
                    int optInFastPeriod,
                    int optInSlowPeriod,
                    int optInSignalPeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double sumFast = 0;
      double sumSlow = 0;
      double sumSignal = 0;
      double medianPrice = 0;
      double osc = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingFastIdx = 0;
      int trailingSlowIdx = 0;
      int oscStartIdx = 0;
      int lookbackTotal = 0;
      double[] oscBuffer;
      int oscBuffer_Idx = 0;
      int maxIdx_oscBuffer = (32)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 5;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 34;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 5;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = AC_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInSignalPeriod < 1 ) return RetCode.InternalError;
      oscBuffer = new double[optInSignalPeriod];
      maxIdx_oscBuffer = (optInSignalPeriod)-1;
      oscBuffer_Idx = 0;
      oscStartIdx = startIdx - (optInSignalPeriod - 1);
      sumFast = 0.0;
      sumSlow = 0.0;
      sumSignal = 0.0;
      trailingFastIdx = oscStartIdx - (optInFastPeriod - 1);
      trailingSlowIdx = oscStartIdx - (optInSlowPeriod - 1);
      i = startIdx - lookbackTotal;
      while( i < oscStartIdx ) {
         medianPrice = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
         if( i >= trailingFastIdx ) {
            sumFast += medianPrice;
         }
         if( i >= trailingSlowIdx ) {
            sumSlow += medianPrice;
         }
         i = i + 1;
      }
      while( i < startIdx ) {
         medianPrice = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
         sumFast += medianPrice;
         sumSlow += medianPrice;
         osc = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
         sumFast -= ((double)inHigh[trailingFastIdx] + (double)inLow[trailingFastIdx]) / 2.0;
         sumSlow -= ((double)inHigh[trailingSlowIdx] + (double)inLow[trailingSlowIdx]) / 2.0;
         trailingFastIdx = trailingFastIdx + 1;
         trailingSlowIdx = trailingSlowIdx + 1;
         i = i + 1;
         oscBuffer[oscBuffer_Idx] = osc;
         sumSignal += osc;
         oscBuffer_Idx++;
         if( oscBuffer_Idx > maxIdx_oscBuffer ) { oscBuffer_Idx = 0; }
      }
      outIdx = 0;
      while( i <= endIdx ) {
         medianPrice = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
         sumFast += medianPrice;
         sumSlow += medianPrice;
         osc = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
         sumFast -= ((double)inHigh[trailingFastIdx] + (double)inLow[trailingFastIdx]) / 2.0;
         sumSlow -= ((double)inHigh[trailingSlowIdx] + (double)inLow[trailingSlowIdx]) / 2.0;
         trailingFastIdx = trailingFastIdx + 1;
         trailingSlowIdx = trailingSlowIdx + 1;
         i = i + 1;
         oscBuffer[oscBuffer_Idx] = osc;
         sumSignal += osc;
         tempReal = osc - sumSignal / (double)optInSignalPeriod;
         oscBuffer_Idx++;
         if( oscBuffer_Idx > maxIdx_oscBuffer ) { oscBuffer_Idx = 0; }
         sumSignal -= oscBuffer[oscBuffer_Idx];
         outReal[outIdx] = tempReal;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Bill Williams' Accelerator/Decelerator Oscillator (*New Trading
    * Dimensions*, 1998): the rate at which market momentum is itself speeding
    * up or slowing down. Where the Awesome Oscillator
    * ([{@code AO}](/functions/ao)) measures momentum, this measures the change
    * in that momentum, by taking the oscillator's distance above or below its
    * own moving average. Because acceleration turns before speed does, the
    * reading changes sign ahead of the oscillator it is built from — it is
    * meant as the early half of a pair, not as a signal on its own. Above zero
    * acceleration is with the bulls, below zero with the bears, and it is drawn
    * as a zero-centred histogram whose colour convention is the bar-to-bar
    * change: rising bars accelerating, falling bars decelerating. Williams'
    * rule of thumb is that two same-coloured bars are what confirms the turn,
    * which is why the sign and the direction matter more than the level. The
    * oscillator is one leg of Williams' Profitunity system, alongside the
    * Awesome Oscillator ([{@code AO}](/functions/ao)) and the Alligator.
    * <p><b>Formula</b>
    * <pre>{@code
    * median_t = ( high_t + low_t ) / 2
    * AO_t = SMA(median, fast)_t − SMA(median, slow)_t
    * AC_t = AO_t − SMA(AO, signal)_t
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AC_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInFastPeriod Number of bars in the short moving average of the
    *        median price (default 5; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInSlowPeriod Number of bars in the long moving average of the
    *        median price (default 34; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInSignalPeriod Number of bars in the moving average taken over
    *        the oscillator (default 5; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal Distance of the Awesome Oscillator
    *        ([{@code AO}](/functions/ao)) from its own moving average, centred on
    *        zero. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#AO
    * @see Core#MACD
    * @see Core#MEDPRICE
    * @see Core#PPO
    * @see Core#SMA
    */
   public OutRange AC( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       int optInSignalPeriod,
                       double outReal[] )
   {
      requireIndexRange("AC", startIdx, endIdx);
      int guardStart = clampedStart("AC", startIdx, AC_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AC", "inHigh", inHigh, guardInLen);
      requireLength("AC", "inLow", inLow, guardInLen);
      requireLength("AC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AC_Impl(startIdx, endIdx, inHigh, inLow, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Bill Williams' Accelerator/Decelerator Oscillator (*New Trading
    * Dimensions*, 1998): the rate at which market momentum is itself speeding
    * up or slowing down. Where the Awesome Oscillator
    * ([{@code AO}](/functions/ao)) measures momentum, this measures the change
    * in that momentum, by taking the oscillator's distance above or below its
    * own moving average. Because acceleration turns before speed does, the
    * reading changes sign ahead of the oscillator it is built from — it is
    * meant as the early half of a pair, not as a signal on its own. Above zero
    * acceleration is with the bulls, below zero with the bears, and it is drawn
    * as a zero-centred histogram whose colour convention is the bar-to-bar
    * change: rising bars accelerating, falling bars decelerating. Williams'
    * rule of thumb is that two same-coloured bars are what confirms the turn,
    * which is why the sign and the direction matter more than the level. The
    * oscillator is one leg of Williams' Profitunity system, alongside the
    * Awesome Oscillator ([{@code AO}](/functions/ao)) and the Alligator.
    * <p><b>Formula</b>
    * <pre>{@code
    * median_t = ( high_t + low_t ) / 2
    * AO_t = SMA(median, fast)_t − SMA(median, slow)_t
    * AC_t = AO_t − SMA(AO, signal)_t
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AC_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInFastPeriod Number of bars in the short moving average of the
    *        median price (default 5; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInSlowPeriod Number of bars in the long moving average of the
    *        median price (default 34; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInSignalPeriod Number of bars in the moving average taken over
    *        the oscillator (default 5; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal Distance of the Awesome Oscillator
    *        ([{@code AO}](/functions/ao)) from its own moving average, centred on
    *        zero. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#AO
    * @see Core#MACD
    * @see Core#MEDPRICE
    * @see Core#PPO
    * @see Core#SMA
    */
   public OutRange AC( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       int optInSignalPeriod,
                       double outReal[] )
   {
      requireIndexRange("AC", startIdx, endIdx);
      int guardStart = clampedStart("AC", startIdx, AC_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AC", "inHigh", inHigh, guardInLen);
      requireLength("AC", "inLow", inLow, guardInLen);
      requireLength("AC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AC_Impl(startIdx, endIdx, inHigh, inLow, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live AC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#AC} over the same series.
    * Open with {@link Core#AC_Open}; there is no close — the handle is
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
   public static final class AC_Stream {
      Core core;
      int optInFastPeriod;
      int optInSlowPeriod;
      int optInSignalPeriod;
      double sumFast;
      double sumSlow;
      double sumSignal;
      int oscBuffer_Idx;
      int maxIdx_oscBuffer;
      int ringPos_trailingFastIdx;
      int ringCap_trailingFastIdx;
      double[] ring_trailingFastIdx_derived;
      int ringPos_trailingSlowIdx;
      int ringCap_trailingSlowIdx;
      double[] ring_trailingSlowIdx_derived;
      int cbSize_oscBuffer;
      double[] cb_oscBuffer;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      AC_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#AC} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      AC_Stream( AC_Stream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.sumFast = other.sumFast;
         this.sumSlow = other.sumSlow;
         this.sumSignal = other.sumSignal;
         this.oscBuffer_Idx = other.oscBuffer_Idx;
         this.maxIdx_oscBuffer = other.maxIdx_oscBuffer;
         this.ringPos_trailingFastIdx = other.ringPos_trailingFastIdx;
         this.ringCap_trailingFastIdx = other.ringCap_trailingFastIdx;
         this.ring_trailingFastIdx_derived = other.ring_trailingFastIdx_derived.clone();
         this.ringPos_trailingSlowIdx = other.ringPos_trailingSlowIdx;
         this.ringCap_trailingSlowIdx = other.ringCap_trailingSlowIdx;
         this.ring_trailingSlowIdx_derived = other.ring_trailingSlowIdx_derived.clone();
         this.cbSize_oscBuffer = other.cbSize_oscBuffer;
         this.cb_oscBuffer = other.cb_oscBuffer.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( AC_Stream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.sumFast = other.sumFast;
         this.sumSlow = other.sumSlow;
         this.sumSignal = other.sumSignal;
         this.oscBuffer_Idx = other.oscBuffer_Idx;
         this.maxIdx_oscBuffer = other.maxIdx_oscBuffer;
         this.ringPos_trailingFastIdx = other.ringPos_trailingFastIdx;
         this.ringCap_trailingFastIdx = other.ringCap_trailingFastIdx;
         if( this.ring_trailingFastIdx_derived != null && this.ring_trailingFastIdx_derived.length == other.ring_trailingFastIdx_derived.length ) {
            System.arraycopy( other.ring_trailingFastIdx_derived, 0, this.ring_trailingFastIdx_derived, 0, other.ring_trailingFastIdx_derived.length );
         } else {
            this.ring_trailingFastIdx_derived = other.ring_trailingFastIdx_derived.clone();
         }
         this.ringPos_trailingSlowIdx = other.ringPos_trailingSlowIdx;
         this.ringCap_trailingSlowIdx = other.ringCap_trailingSlowIdx;
         if( this.ring_trailingSlowIdx_derived != null && this.ring_trailingSlowIdx_derived.length == other.ring_trailingSlowIdx_derived.length ) {
            System.arraycopy( other.ring_trailingSlowIdx_derived, 0, this.ring_trailingSlowIdx_derived, 0, other.ring_trailingSlowIdx_derived.length );
         } else {
            this.ring_trailingSlowIdx_derived = other.ring_trailingSlowIdx_derived.clone();
         }
         this.cbSize_oscBuffer = other.cbSize_oscBuffer;
         if( this.cb_oscBuffer != null && this.cb_oscBuffer.length == other.cb_oscBuffer.length ) {
            System.arraycopy( other.cb_oscBuffer, 0, this.cb_oscBuffer, 0, other.cb_oscBuffer.length );
         } else {
            this.cb_oscBuffer = other.cb_oscBuffer.clone();
         }
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<AC_Stream> PEEK_SCRATCH = new ThreadLocal<>();

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
      public double update( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("AC update: BadParam", RetCode.BadParam);
         core.AC_StepImpl(this, inHigh, inLow);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inHigh[], double inLow[], double outReal[] ) {
         requireArgument("AC updateAndFill", "inHigh", inHigh);
         requireArgument("AC updateAndFill", "inLow", inLow);
         requireArgument("AC updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow )
            throw new TaLibArgumentException("AC updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) )
               throw new TaLibArgumentException("AC updateAndFill: BadParam", RetCode.BadParam);
            core.AC_StepImpl(this, inHigh[i], inLow[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
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
      public double peek( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("AC peek: BadParam", RetCode.BadParam);
         AC_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new AC_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.AC_StepImpl(scratch, inHigh, inLow);
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
      public AC_Stream copy() {
         return new AC_Stream(this);
      }
   }
   void AC_StepImpl( AC_Stream sp, double inHigh, double inLow )
   {
      double medianPrice = 0.0;
      double osc = 0.0;
      double tempReal = 0.0;
      if( sp.ringCap_trailingFastIdx == 0 ) {
         sp.ring_trailingFastIdx_derived[0] = (inHigh + inLow) / 2.0;
      }
      if( sp.ringCap_trailingSlowIdx == 0 ) {
         sp.ring_trailingSlowIdx_derived[0] = (inHigh + inLow) / 2.0;
      }
      medianPrice = (inHigh + inLow) / 2.0;
      sp.sumFast += medianPrice;
      sp.sumSlow += medianPrice;
      /* Snapshot the oscillator before either total drops its trailing bar,
       * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
       */
      osc = sp.sumFast / (double)sp.optInFastPeriod - sp.sumSlow / (double)sp.optInSlowPeriod;
      sp.sumFast -= sp.ring_trailingFastIdx_derived[sp.ringPos_trailingFastIdx];
      sp.sumSlow -= sp.ring_trailingSlowIdx_derived[sp.ringPos_trailingSlowIdx];
      /* Today's oscillator enters the signal window at its own slot, and the
       * bar leaving that window is read only after the ring has advanced onto
       * it -- writing first is what makes the slot the loop is about to
       * overwrite the newest value rather than the oldest one.
       */
      sp.cb_oscBuffer[sp.oscBuffer_Idx] = osc;
      sp.sumSignal += osc;
      tempReal = osc - sp.sumSignal / (double)sp.optInSignalPeriod;
      sp.oscBuffer_Idx = sp.oscBuffer_Idx + 1;
      if( sp.oscBuffer_Idx > sp.maxIdx_oscBuffer ) {
         sp.oscBuffer_Idx = 0;
      }
      sp.sumSignal -= sp.cb_oscBuffer[sp.oscBuffer_Idx];
      /* Every input read for this bar is done above, so the store is safe
       * when the caller aliases outReal over inHigh or inLow. Unlike ao.c
       * there is slack here -- the signal window puts both trailing indices
       * at least optInSignalPeriod-1 bars ahead of outIdx, so no reachable
       * parameter makes them collide -- but the order is kept anyway, so
       * that admitting a signal period of 1 would not silently reintroduce
       * the collision ao.c has to guard against.
       */
      sp.cur_outReal = tempReal;
      sp.ring_trailingFastIdx_derived[sp.ringPos_trailingFastIdx] = (inHigh + inLow) / 2.0;
      sp.ringPos_trailingFastIdx = sp.ringPos_trailingFastIdx + 1;
      if( sp.ringPos_trailingFastIdx >= sp.ringCap_trailingFastIdx ) {
         sp.ringPos_trailingFastIdx = 0;
      }
      sp.ring_trailingSlowIdx_derived[sp.ringPos_trailingSlowIdx] = (inHigh + inLow) / 2.0;
      sp.ringPos_trailingSlowIdx = sp.ringPos_trailingSlowIdx + 1;
      if( sp.ringPos_trailingSlowIdx >= sp.ringCap_trailingSlowIdx ) {
         sp.ringPos_trailingSlowIdx = 0;
      }
   }
   private RetCode AC_OpenImpl( AC_Stream sp, double inHigh[], double inLow[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double sumFast = 0;
      double sumSlow = 0;
      double sumSignal = 0;
      double medianPrice = 0;
      double osc = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingFastIdx = 0;
      int trailingSlowIdx = 0;
      int oscStartIdx = 0;
      int lookbackTotal = 0;
      double[] oscBuffer;
      int oscBuffer_Idx = 0;
      int maxIdx_oscBuffer = (32)-1;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 5;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 34;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 5;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Bill Williams' Accelerator/Decelerator Oscillator (New Trading
       * Dimensions, 1998): how fast the Awesome Oscillator is itself
       * accelerating, drawn as a zero-centred histogram.
       *
       *    AO_t = SMA(median, fast)_t - SMA(median, slow)_t
       *    AC_t = AO_t - SMA(AO, signal)_t
       *
       * Every leg is a plain SMA, so there is no seeding convention to get wrong
       * and no cross-library divergence of the kind an EMA brings.
       *
       * This is ta_codegen/input/ao/ao.c walking one derived series with a third
       * running sum layered on top: the oscillator is never materialised into an
       * array, it goes straight into a ring of the last `signal` values. The
       * order is the one ta_codegen/input/sma/sma.c uses -- add the new value,
       * snapshot the total, subtract the bar leaving the window, divide by the
       * period -- for all three sums, which is what makes the composed reference
       * in test_composite.c (TA_AO, TA_SMA and TA_SUB) bit-exact rather than
       * merely close. Any future drift in either path is then a hard failure
       * instead of a tolerance argument.
       *
       * Each total is DIVIDED by its period; it is not multiplied by a
       * precomputed 1/period, for the reason spelled out in ao.c and #117/#118.
       */
      /* This ptr will point on a circular buffer of at least
       * "optInSignalPeriod" element.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = AC_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Allocate a circular buffer equal to the requested signal period. */
      if( optInSignalPeriod < 1 ) return RetCode.InternalError;
      oscBuffer = new double[optInSignalPeriod];
      maxIdx_oscBuffer = (optInSignalPeriod)-1;
      oscBuffer_Idx = 0;
      /* The first bar the oscillator is evaluated on for this call. It trails
       * the first output by the signal window, because that many oscillator
       * values have to exist before their average does.
       */
      oscStartIdx = startIdx - (optInSignalPeriod - 1);
      sumFast = 0.0;
      sumSlow = 0.0;
      sumSignal = 0.0;
      trailingFastIdx = oscStartIdx - (optInFastPeriod - 1);
      trailingSlowIdx = oscStartIdx - (optInSlowPeriod - 1);
      /* Add-up both initial periods, except for the last value.
       *
       * One pass over the longer warm-up window replaces two overlapping passes.
       * A bar inside the shorter window is added to that total as it is reached,
       * so each total still accumulates exactly the same bars in exactly the same
       * ascending order two separate loops would have given it -- which is what
       * keeps each leg bit-identical to the TA_SMA that TA_AO would have called
       * had it been started at oscStartIdx.
       */
      i = startIdx - lookbackTotal;
      while( i < oscStartIdx ) {
         medianPrice = (inHigh[i] + inLow[i]) / 2.0;
         if( i >= trailingFastIdx ) {
            sumFast += medianPrice;
         }
         if( i >= trailingSlowIdx ) {
            sumSlow += medianPrice;
         }
         i = i + 1;
      }
      /* Fill the signal ring with the oscillator values that precede the first
       * output. This is the same body as the main loop below minus the store --
       * the window is not full yet, so there is nothing to emit.
       */
      while( i < startIdx ) {
         medianPrice = (inHigh[i] + inLow[i]) / 2.0;
         sumFast += medianPrice;
         sumSlow += medianPrice;
         osc = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
         sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
         sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
         trailingFastIdx = trailingFastIdx + 1;
         trailingSlowIdx = trailingSlowIdx + 1;
         i = i + 1;
         oscBuffer[oscBuffer_Idx] = osc;
         sumSignal += osc;
         oscBuffer_Idx++;
         if( oscBuffer_Idx > maxIdx_oscBuffer ) { oscBuffer_Idx = 0; }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows outReal to be the same
       * buffer as either input.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         medianPrice = (inHigh[i] + inLow[i]) / 2.0;
         sumFast += medianPrice;
         sumSlow += medianPrice;
         /* Snapshot the oscillator before either total drops its trailing bar,
          * mirroring the add-new / snapshot / subtract-old order of TA_SMA.
          */
         osc = sumFast / (double)optInFastPeriod - sumSlow / (double)optInSlowPeriod;
         sumFast -= (inHigh[trailingFastIdx] + inLow[trailingFastIdx]) / 2.0;
         sumSlow -= (inHigh[trailingSlowIdx] + inLow[trailingSlowIdx]) / 2.0;
         trailingFastIdx = trailingFastIdx + 1;
         trailingSlowIdx = trailingSlowIdx + 1;
         i = i + 1;
         /* Today's oscillator enters the signal window at its own slot, and the
          * bar leaving that window is read only after the ring has advanced onto
          * it -- writing first is what makes the slot the loop is about to
          * overwrite the newest value rather than the oldest one.
          */
         oscBuffer[oscBuffer_Idx] = osc;
         sumSignal += osc;
         tempReal = osc - sumSignal / (double)optInSignalPeriod;
         oscBuffer_Idx++;
         if( oscBuffer_Idx > maxIdx_oscBuffer ) { oscBuffer_Idx = 0; }
         sumSignal -= oscBuffer[oscBuffer_Idx];
         /* Every input read for this bar is done above, so the store is safe
          * when the caller aliases outReal over inHigh or inLow. Unlike ao.c
          * there is slack here -- the signal window puts both trailing indices
          * at least optInSignalPeriod-1 bars ahead of outIdx, so no reachable
          * parameter makes them collide -- but the order is kept anyway, so
          * that admitting a signal period of 1 would not silently reintroduce
          * the collision ao.c has to guard against.
          */
         outReal[outIdx * outStride] = tempReal;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingFastIdx = i - trailingFastIdx;
      if( cap_trailingFastIdx < 0 || cap_trailingFastIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingFastIdx = (cap_trailingFastIdx > 0)? cap_trailingFastIdx : 1;
      double[] capRing_trailingFastIdx_derived = new double[allocN_trailingFastIdx];
      for( int fillJ = historyLen - cap_trailingFastIdx; fillJ < historyLen; fillJ++ ) {
         capRing_trailingFastIdx_derived[fillJ - (historyLen - cap_trailingFastIdx)] = (inHigh[fillJ] + inLow[fillJ]) / 2.0;
      }
      int cap_trailingSlowIdx = i - trailingSlowIdx;
      if( cap_trailingSlowIdx < 0 || cap_trailingSlowIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingSlowIdx = (cap_trailingSlowIdx > 0)? cap_trailingSlowIdx : 1;
      double[] capRing_trailingSlowIdx_derived = new double[allocN_trailingSlowIdx];
      for( int fillJ = historyLen - cap_trailingSlowIdx; fillJ < historyLen; fillJ++ ) {
         capRing_trailingSlowIdx_derived[fillJ - (historyLen - cap_trailingSlowIdx)] = (inHigh[fillJ] + inLow[fillJ]) / 2.0;
      }
      int capCb_oscBuffer = maxIdx_oscBuffer + 1;
      if( capCb_oscBuffer > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.sumFast = sumFast;
      sp.sumSlow = sumSlow;
      sp.sumSignal = sumSignal;
      sp.oscBuffer_Idx = oscBuffer_Idx;
      sp.maxIdx_oscBuffer = maxIdx_oscBuffer;
      sp.ringPos_trailingFastIdx = 0;
      sp.ringCap_trailingFastIdx = cap_trailingFastIdx;
      sp.ring_trailingFastIdx_derived = capRing_trailingFastIdx_derived;
      sp.ringPos_trailingSlowIdx = 0;
      sp.ringCap_trailingSlowIdx = cap_trailingSlowIdx;
      sp.ring_trailingSlowIdx_derived = capRing_trailingSlowIdx_derived;
      sp.cbSize_oscBuffer = capCb_oscBuffer;
      sp.cb_oscBuffer = oscBuffer;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* AC_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AC_Stream AC_OpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AC_Stream sp = new AC_Stream(this);
      RetCode retCode = AC_OpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AC openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AC openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("AC openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind AC_Open (composition seam). */
   AC_Stream AC_OpenInternal( double inHigh[], double inLow[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      AC_Stream sp = new AC_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = AC_OpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AC open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AC open: internal error", retCode);
      }
      throw new TaLibArgumentException("AC open: " + retCode, retCode);
   }
   /**
    * Open a live AC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#AC} at that bar.
    * <p>The history must hold at least {@code AC_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public AC_Stream AC_Open( double inHigh[], double inLow[], int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      requireArgument("AC open", "inHigh", inHigh);
      requireHistory("AC open", inHigh.length);
      requireArgument("AC open", "inLow", inLow);
      requireHistoryLength("AC open", "inLow", inLow.length, inHigh.length);
      return AC_OpenInternal(inHigh, inLow, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
   }
   /**
    * {@link Core#AC_Open} that also fills the output array(s) bit-identically
    * to {@link Core#AC} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AC_Stream#outRange()}.
    */
   public AC_Stream AC_OpenAndFill( double inHigh[], double inLow[], int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, double outReal[] )
   {
      requireArgument("AC openAndFill", "inHigh", inHigh);
      requireHistory("AC openAndFill", inHigh.length);
      requireArgument("AC openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("AC openAndFill", inHigh.length, AC_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      requireHistoryLength("AC openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("AC openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw new TaLibArgumentException("AC openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return AC_OpenAndFillInternal(inHigh, inLow, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal);
   }
