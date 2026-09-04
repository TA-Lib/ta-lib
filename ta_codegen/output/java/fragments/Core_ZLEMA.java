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
 *  090426 MF,CC  Initial version (#347).
 */

   /**
    * Number of leading input bars {@link Core#ZLEMA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the exponential average; the
    *        de-lag distance derives from it (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ZLEMA_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* ZLEMA owns no TA_FUNC_UNST_ id. It borrows EMA's through this call, which
       * is why zlema.yaml must not declare `unstable_period`.
       */
      return (optInTimePeriod - 1) / 2 + EMA_Lookback(optInTimePeriod) ;

   }
   RetCode ZLEMA_Impl( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double optInK_1 = 0;
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      int lag = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* KEEP THIS ARITHMETIC EXACTLY AS WRITTEN -- the de-lag in one rounding
       * (2.0*c - l, not c + (c - l)), the seed sum accumulating from 0.0, and
       * ((v - prevMA)*k) + prevMA. Together they make ZLEMA bit-for-bit equal to
       * an EMA over a materialised de-lagged series, which is the strongest gate
       * this function has. Reordering any one breaks that equality silently, and
       * the de-lag spelling is worth more than rounding noise: c + (c - l) rounds
       * twice, which is 5e-12 relative where 2c - l cancels.
       */
      lag = (optInTimePeriod - 1) / 2;
      lookbackTotal = ZLEMA_Lookback(optInTimePeriod);
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
      /* No smoothing at period of 1: the output is a copy of the input, the
       * convention TA_MA applies to every MAType. Explicit, because at period 1
       * lag is 0 and optInK_1 is exactly 1.0, so the recursion below reduces to
       * (x-prev)+prev -- which returns x only while consecutive values stay
       * within a factor of two of each other. The unstable period still delays
       * the first output.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      /* In-place safe (outReal == inReal): both of a bar's reads precede its
       * write, and trailingIdx >= outIdx + optInTimePeriod - 1, so the trailing
       * read never reaches a slot already written.
       */
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + lag;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += 2.0 * inReal[today] - inReal[trailingIdx];
         today += 1;
         trailingIdx += 1;
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = Math.fma(2.0 * inReal[today] - inReal[trailingIdx] - prevMA, optInK_1, prevMA);
         today += 1;
         trailingIdx += 1;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = Math.fma(2.0 * inReal[today] - inReal[trailingIdx] - prevMA, optInK_1, prevMA);
         today += 1;
         trailingIdx += 1;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode ZLEMA_Impl( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double optInK_1 = 0;
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      int lag = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      lag = (optInTimePeriod - 1) / 2;
      lookbackTotal = ZLEMA_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + lag;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += 2.0 * (double)inReal[today] - (double)inReal[trailingIdx];
         today += 1;
         trailingIdx += 1;
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = Math.fma(2.0 * (double)inReal[today] - (double)inReal[trailingIdx] - prevMA, optInK_1, prevMA);
         today += 1;
         trailingIdx += 1;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = Math.fma(2.0 * (double)inReal[today] - (double)inReal[trailingIdx] - prevMA, optInK_1, prevMA);
         today += 1;
         trailingIdx += 1;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Zero-Lag Exponential Moving Average: an [{@code EMA}](/functions/ema)
    * applied not to price but to a de-lagged series that extrapolates price
    * forward by the EMA's own lag, cancelling that lag to first order. It
    * tracks a trend far more closely than an EMA of the same length, at the
    * cost of overshooting sharp reversals — the extrapolation keeps pushing in
    * the old direction for a bar or two. Read it as an EMA that turns sooner:
    * crossings of price and average, and changes in its slope, arrive earlier
    * than the equivalent EMA signal, and its overshoot after a spike is a
    * property of the filter rather than a move in the market. ZLEMA is also
    * selectable as a moving-average type ({@code TA_MAType_ZLEMA}) wherever an
    * {@code optInMAType} parameter is accepted ([{@code MA}](/functions/ma),
    * [{@code BBANDS}](/functions/bbands), [{@code STOCH}](/functions/stoch),
    * [{@code MACDEXT}](/functions/macdext), ...).
    * <p><b>Formula</b>
    * <pre>{@code
    * lag = Integer( (n - 1) / 2 )
    * d = 2 * Price - Price[lag bars ago]
    * ZLEMA(n) = EMA( d, n )
    * The inner average is the standard TA-Lib EMA: smoothing factor 2 / (n + 1), seeded with the simple average of the first n de-lagged values.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>**The paper this indicator is usually credited to describes a different filter.** Ehlers and Way's *Zero Lag (Well, Almost)* specifies an error-correcting EMA with a per-bar gain search; neither the de-lagged series nor the {@code (n-1)/2} lag appears anywhere in it. What TA-Lib ships here is the de-lagged-EMA construction published under the "zero lag" name by Tulip Indicators, pandas-ta, TradingView Pine and others, for which no primary source is traceable.</li>
    * <li>{@code lag} **truncates**: {@code Integer((n-1)/2)}. For an even period that is one bar shorter than the round-to-nearest convention some descriptions use, which moves the whole line, not just its warm-up. Tulip Indicators, pandas-ta and Pine all truncate.</li>
    * <li>The de-lag is computed as {@code 2 * Price - Price[lag]} in one rounding, rather than the algebraically equal {@code Price + (Price - Price[lag])} that Tulip Indicators, TradingView Pine and the Wikipedia statement use. The second form's extra rounding is one unit in the last place of the larger price — negligible against the de-lagged value, except where that value nearly cancels. When price is near double its value {@code lag} bars ago the two forms differ by about 5e-12 relative, so expect that much disagreement against those implementations on a strongly trending series, and do not attribute it to the seed or the smoothing factor.</li>
    * <li>Implementations disagree on how the inner EMA is seeded — TA-Lib uses its own EMA convention (the simple average of the first {@code n} de-lagged values), where Tulip Indicators seeds from a single raw price and so emits its first value earlier and converges to these values only after many bars.</li>
    * <li>ZLEMA inherits EMA's unstable period rather than owning one: {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} moves ZLEMA's first output too.</li>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ZLEMA_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series, close by convention.
    * @param optInTimePeriod Number of bars in the exponential average; the
    *        de-lag distance derives from it (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Zero-lag exponential moving average of the input. Must hold
    *        at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#EMA
    * @see Core#DEMA
    * @see Core#TEMA
    * @see Core#HMA
    * @see Core#MA
    */
   public OutRange ZLEMA( int startIdx,
                          int endIdx,
                          double inReal[],
                          int optInTimePeriod,
                          double outReal[] )
   {
      requireIndexRange("ZLEMA", startIdx, endIdx);
      int guardStart = clampedStart("ZLEMA", startIdx, ZLEMA_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ZLEMA", "inReal", inReal, guardInLen);
      requireLength("ZLEMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ZLEMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ZLEMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Zero-Lag Exponential Moving Average: an [{@code EMA}](/functions/ema)
    * applied not to price but to a de-lagged series that extrapolates price
    * forward by the EMA's own lag, cancelling that lag to first order. It
    * tracks a trend far more closely than an EMA of the same length, at the
    * cost of overshooting sharp reversals — the extrapolation keeps pushing in
    * the old direction for a bar or two. Read it as an EMA that turns sooner:
    * crossings of price and average, and changes in its slope, arrive earlier
    * than the equivalent EMA signal, and its overshoot after a spike is a
    * property of the filter rather than a move in the market. ZLEMA is also
    * selectable as a moving-average type ({@code TA_MAType_ZLEMA}) wherever an
    * {@code optInMAType} parameter is accepted ([{@code MA}](/functions/ma),
    * [{@code BBANDS}](/functions/bbands), [{@code STOCH}](/functions/stoch),
    * [{@code MACDEXT}](/functions/macdext), ...).
    * <p><b>Formula</b>
    * <pre>{@code
    * lag = Integer( (n - 1) / 2 )
    * d = 2 * Price - Price[lag bars ago]
    * ZLEMA(n) = EMA( d, n )
    * The inner average is the standard TA-Lib EMA: smoothing factor 2 / (n + 1), seeded with the simple average of the first n de-lagged values.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>**The paper this indicator is usually credited to describes a different filter.** Ehlers and Way's *Zero Lag (Well, Almost)* specifies an error-correcting EMA with a per-bar gain search; neither the de-lagged series nor the {@code (n-1)/2} lag appears anywhere in it. What TA-Lib ships here is the de-lagged-EMA construction published under the "zero lag" name by Tulip Indicators, pandas-ta, TradingView Pine and others, for which no primary source is traceable.</li>
    * <li>{@code lag} **truncates**: {@code Integer((n-1)/2)}. For an even period that is one bar shorter than the round-to-nearest convention some descriptions use, which moves the whole line, not just its warm-up. Tulip Indicators, pandas-ta and Pine all truncate.</li>
    * <li>The de-lag is computed as {@code 2 * Price - Price[lag]} in one rounding, rather than the algebraically equal {@code Price + (Price - Price[lag])} that Tulip Indicators, TradingView Pine and the Wikipedia statement use. The second form's extra rounding is one unit in the last place of the larger price — negligible against the de-lagged value, except where that value nearly cancels. When price is near double its value {@code lag} bars ago the two forms differ by about 5e-12 relative, so expect that much disagreement against those implementations on a strongly trending series, and do not attribute it to the seed or the smoothing factor.</li>
    * <li>Implementations disagree on how the inner EMA is seeded — TA-Lib uses its own EMA convention (the simple average of the first {@code n} de-lagged values), where Tulip Indicators seeds from a single raw price and so emits its first value earlier and converges to these values only after many bars.</li>
    * <li>ZLEMA inherits EMA's unstable period rather than owning one: {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} moves ZLEMA's first output too.</li>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ZLEMA_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series, close by convention.
    * @param optInTimePeriod Number of bars in the exponential average; the
    *        de-lag distance derives from it (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Zero-lag exponential moving average of the input. Must hold
    *        at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#EMA
    * @see Core#DEMA
    * @see Core#TEMA
    * @see Core#HMA
    * @see Core#MA
    */
   public OutRange ZLEMA( int startIdx,
                          int endIdx,
                          float inReal[],
                          int optInTimePeriod,
                          double outReal[] )
   {
      requireIndexRange("ZLEMA", startIdx, endIdx);
      int guardStart = clampedStart("ZLEMA", startIdx, ZLEMA_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ZLEMA", "inReal", inReal, guardInLen);
      requireLength("ZLEMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ZLEMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ZLEMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ZLEMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ZLEMA} over the same series.
    * Open with {@link Core#zlemaOpen}; there is no close — the handle is
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
   public static final class ZlemaStream {
      Core core;
      int optInTimePeriod;
      double optInK_1;
      double prevMA;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      ZlemaStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ZLEMA} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      ZlemaStream( ZlemaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInK_1 = other.optInK_1;
         this.prevMA = other.prevMA;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
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
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("ZLEMA update: BadParam", RetCode.BadParam);
         }
         core.zlemaStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
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
      public void updateAndFill( double inReal[], double outReal[] ) {
         requireArgument("ZLEMA updateAndFill", "inReal", inReal);
         requireArgument("ZLEMA updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("ZLEMA updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("ZLEMA updateAndFill: BadParam", RetCode.BadParam);
            }
            core.zlemaStepImpl(this, inReal[i]);
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
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("ZLEMA peek: BadParam", RetCode.BadParam);
         ZlemaStream sp = this;
         double cur_outReal = 0.0;
         double prevMA = sp.prevMA;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( sp.optInTimePeriod == 1 ) {
            cur_outReal = inReal;
            return cur_outReal ;
         }
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         prevMA = Math.fma(2.0 * inReal - ((sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal0) - prevMA, sp.optInK_1, prevMA);
         cur_outReal = prevMA;
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
      public ZlemaStream clone() {
         return new ZlemaStream(this);
      }
   }
   void zlemaStepImpl( ZlemaStream sp, double inReal )
   {
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      sp.prevMA = Math.fma(2.0 * inReal - sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] - sp.prevMA, sp.optInK_1, sp.prevMA);
      sp.cur_outReal = sp.prevMA;
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode zlemaOpenImpl( ZlemaStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double optInK_1 = 0;
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      int lag = 0;
      int lookbackTotal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( optInTimePeriod == 1 ) {
         int fillLb = ZLEMA_Lookback(optInTimePeriod);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInK_1 = 0.0;
         sp.prevMA = 0.0;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = 0;
         sp.ring_trailingIdx_inReal = new double[1];
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         if( outStride == 0 ) {
            outReal[0] = inReal[historyLen - 1];
         } else {
            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
               outReal[fillIdx] = inReal[fillLb + fillIdx];
            }
         }
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* KEEP THIS ARITHMETIC EXACTLY AS WRITTEN -- the de-lag in one rounding
       * (2.0*c - l, not c + (c - l)), the seed sum accumulating from 0.0, and
       * ((v - prevMA)*k) + prevMA. Together they make ZLEMA bit-for-bit equal to
       * an EMA over a materialised de-lagged series, which is the strongest gate
       * this function has. Reordering any one breaks that equality silently, and
       * the de-lag spelling is worth more than rounding noise: c + (c - l) rounds
       * twice, which is 5e-12 relative where 2c - l cancels.
       */
      lag = (optInTimePeriod - 1) / 2;
      lookbackTotal = ZLEMA_Lookback(optInTimePeriod);
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
      outBegIdx.value = startIdx;
      /* In-place safe (outReal == inReal): both of a bar's reads precede its
       * write, and trailingIdx >= outIdx + optInTimePeriod - 1, so the trailing
       * read never reaches a slot already written.
       */
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + lag;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += 2.0 * inReal[today] - inReal[trailingIdx];
         today += 1;
         trailingIdx += 1;
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = Math.fma(2.0 * inReal[today] - inReal[trailingIdx] - prevMA, optInK_1, prevMA);
         today += 1;
         trailingIdx += 1;
      }
      outReal[0 * outStride] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = Math.fma(2.0 * inReal[today] - inReal[trailingIdx] - prevMA, optInK_1, prevMA);
         today += 1;
         trailingIdx += 1;
         outReal[outIdx++ * outStride] = prevMA;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInK_1 = optInK_1;
      sp.prevMA = prevMA;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* zlemaOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   ZlemaStream zlemaOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      ZlemaStream sp = new ZlemaStream(this);
      RetCode retCode = zlemaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ZLEMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ZLEMA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ZLEMA openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind zlemaOpen (composition seam). */
   ZlemaStream zlemaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      ZlemaStream sp = new ZlemaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = zlemaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ZLEMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ZLEMA open: internal error", retCode);
      }
      throw new TaLibArgumentException("ZLEMA open: " + retCode, retCode);
   }
   /**
    * Open a live ZLEMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ZLEMA} at that bar.
    * <p>The history must hold at least {@code ZLEMA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public ZlemaStream zlemaOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("ZLEMA open", "inReal", inReal);
      requireHistory("ZLEMA open", inReal.length);
      return zlemaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#zlemaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ZLEMA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link ZlemaStream#outRange()}.
    */
   public ZlemaStream zlemaOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("ZLEMA openAndFill", "inReal", inReal);
      requireHistory("ZLEMA openAndFill", inReal.length);
      int guardOutLen = openFillCount("ZLEMA openAndFill", inReal.length, ZLEMA_Lookback(optInTimePeriod));
      requireLength("ZLEMA openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("ZLEMA openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return zlemaOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
