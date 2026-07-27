/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   /**
    * Number of leading input bars {@link Core#ema} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Number of bars in the average; sets smoothing k =
    *        2/(period+1) (default 30; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int emaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 + this.unstablePeriod[FuncUnstId.Ema.ordinal()] ;

   }
   RetCode emaPrivate( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       double optInK_1,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      /* Internal implementation can be called from any other TA function.
       *
       * Faster because there is no parameter check, but it is a double
       * edge sword.
       *
       * The optInK_1 and optInTimePeriod are usually tightly coupled:
       *
       *    optInK_1  = 2 / (optInTimePeriod + 1).
       *
       * These values are going to be related by this equation 99.9% of the
       * time... but there is some exception, this is why both must be provided.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = emaLookback(optInTimePeriod);
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
      outBegIdx.value = startIdx;
      /* Do the EMA calculation using tight loops. */
      /* The first EMA is calculated differently. It
       * then become the seed for subsequent EMA.
       *
       * The algorithm for this seed vary widely.
       * Only 3 are implemented here:
       *
       * TA_MA_CLASSIC:
       *    Use a simple MA of the first 'period'.
       *    This is the approach most widely documented.
       *
       * TA_MA_METASTOCK:
       *    Use first price bar value as a seed
       *    from the begining of all the available
       *    data.
       *
       * TA_MA_TRADESTATION:
       *    Use 4th price bar as a seed, except when
       *    period is 1 who use 2th price bar or something
       *    like that... (not an obvious one...).
       */
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = (inReal[today++] - prevMA) * optInK_1 + prevMA;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = (inReal[today++] - prevMA) * optInK_1 + prevMA;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode emaPrivate( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       double optInK_1,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      lookbackTotal = emaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += (double)inReal[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode emaInternal( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double optInK_1 = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* Simply call the internal implementation of the EMA. */
      return emaPrivate(startIdx, endIdx, inReal, optInTimePeriod, optInK_1, outBegIdx, outNBElement, outReal) ;
   }
   RetCode emaUnguardedInternal( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double optInK_1 = 0;
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      return emaPrivate(startIdx, endIdx, inReal, optInTimePeriod, optInK_1, outBegIdx, outNBElement, outReal) ;
   }
   RetCode emaInternal( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double optInK_1 = (2.0/(double)((optInTimePeriod+1)));
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = emaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += (double)inReal[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode emaUnguardedInternal( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double optInK_1 = (2.0/(double)((optInTimePeriod+1)));
      lookbackTotal = emaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += (double)inReal[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Exponential moving average that weights recent prices more heavily via a
    * recursive smoothing factor. A core building block seeding or composing
    * many other indicators. Reacts faster than SMA; price above/below EMA
    * suggests up/down trend.
    * <p><b>Formula</b>
    * <pre>{@code
    * k = 2 / (period + 1); EMA_t = (price_t - EMA_{t-1}) * k + EMA_{t-1}. Seed: EMA = SMA of first `period` bars.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#emaLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal price/data series to smooth.
    * @param optInTimePeriod Number of bars in the average; sets smoothing k =
    *        2/(period+1) (default 30; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal the exponential moving average. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#sma
    * @see Core#dema
    * @see Core#tema
    * @see Core#movingAverage
    * @see Core#macd
    * @see Core#t3
    */
   public OutRange ema( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = emaInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("EMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Exponential moving average that weights recent prices more heavily via a
    * recursive smoothing factor. A core building block seeding or composing
    * many other indicators. Reacts faster than SMA; price above/below EMA
    * suggests up/down trend. — <b>unchecked</b> variant of {@link Core#ema}.
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
   public OutRange emaUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      emaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Exponential moving average that weights recent prices more heavily via a
    * recursive smoothing factor. A core building block seeding or composing
    * many other indicators. Reacts faster than SMA; price above/below EMA
    * suggests up/down trend.
    * <p><b>Formula</b>
    * <pre>{@code
    * k = 2 / (period + 1); EMA_t = (price_t - EMA_{t-1}) * k + EMA_{t-1}. Seed: EMA = SMA of first `period` bars.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#emaLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal price/data series to smooth.
    * @param optInTimePeriod Number of bars in the average; sets smoothing k =
    *        2/(period+1) (default 30; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal the exponential moving average. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#sma
    * @see Core#dema
    * @see Core#tema
    * @see Core#movingAverage
    * @see Core#macd
    * @see Core#t3
    */
   public OutRange ema( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = emaInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("EMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Exponential moving average that weights recent prices more heavily via a
    * recursive smoothing factor. A core building block seeding or composing
    * many other indicators. Reacts faster than SMA; price above/below EMA
    * suggests up/down trend. — <b>unchecked</b> variant of {@link Core#ema}.
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
   public OutRange emaUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      emaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live EMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ema} over the same series.
    * Open with {@link Core#emaOpen}; there is no close — the handle is
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
   public static final class EmaStream {
      final Core core;
      int optInTimePeriod;
      double optInK_1;
      double prevMA;
      double cur_outReal;
      OutRange fillRange;

      EmaStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#emaOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      EmaStream( EmaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInK_1 = other.optInK_1;
         this.prevMA = other.prevMA;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.emaStreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         EmaStream scratch = new EmaStream(this);
         core.emaStreamStep(scratch, inReal);
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
      public EmaStream copy() {
         return new EmaStream(this);
      }
   }
   void emaStreamStep( EmaStream sp, double inReal )
   {
      sp.prevMA = (inReal - sp.prevMA) * sp.optInK_1 + sp.prevMA;
      sp.cur_outReal = sp.prevMA;
   }
   private RetCode emaOpenBody( EmaStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      double optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* Internal implementation can be called from any other TA function.
       *
       * Faster because there is no parameter check, but it is a double
       * edge sword.
       *
       * The optInK_1 and optInTimePeriod are usually tightly coupled:
       *
       *    optInK_1  = 2 / (optInTimePeriod + 1).
       *
       * These values are going to be related by this equation 99.9% of the
       * time... but there is some exception, this is why both must be provided.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = emaLookback(optInTimePeriod);
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
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      /* Do the EMA calculation using tight loops. */
      /* The first EMA is calculated differently. It
       * then become the seed for subsequent EMA.
       *
       * The algorithm for this seed vary widely.
       * Only 3 are implemented here:
       *
       * TA_MA_CLASSIC:
       *    Use a simple MA of the first 'period'.
       *    This is the approach most widely documented.
       *
       * TA_MA_METASTOCK:
       *    Use first price bar value as a seed
       *    from the begining of all the available
       *    data.
       *
       * TA_MA_TRADESTATION:
       *    Use 4th price bar as a seed, except when
       *    period is 1 who use 2th price bar or something
       *    like that... (not an obvious one...).
       */
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = (inReal[today++] - prevMA) * optInK_1 + prevMA;
      }
      lastValue_outReal = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = (inReal[today++] - prevMA) * optInK_1 + prevMA;
         lastValue_outReal = prevMA;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInK_1 = optInK_1;
      sp.prevMA = prevMA;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode emaOpenAndFillBody( EmaStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      double optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* Internal implementation can be called from any other TA function.
       *
       * Faster because there is no parameter check, but it is a double
       * edge sword.
       *
       * The optInK_1 and optInTimePeriod are usually tightly coupled:
       *
       *    optInK_1  = 2 / (optInTimePeriod + 1).
       *
       * These values are going to be related by this equation 99.9% of the
       * time... but there is some exception, this is why both must be provided.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = emaLookback(optInTimePeriod);
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
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      /* Do the EMA calculation using tight loops. */
      /* The first EMA is calculated differently. It
       * then become the seed for subsequent EMA.
       *
       * The algorithm for this seed vary widely.
       * Only 3 are implemented here:
       *
       * TA_MA_CLASSIC:
       *    Use a simple MA of the first 'period'.
       *    This is the approach most widely documented.
       *
       * TA_MA_METASTOCK:
       *    Use first price bar value as a seed
       *    from the begining of all the available
       *    data.
       *
       * TA_MA_TRADESTATION:
       *    Use 4th price bar as a seed, except when
       *    period is 1 who use 2th price bar or something
       *    like that... (not an obvious one...).
       */
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = (inReal[today++] - prevMA) * optInK_1 + prevMA;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = (inReal[today++] - prevMA) * optInK_1 + prevMA;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInK_1 = optInK_1;
      sp.prevMA = prevMA;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind emaOpen (composition seam). */
   EmaStream emaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      EmaStream sp = new EmaStream(this);
      RetCode retCode = emaOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_EMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_EMA open: internal error");
      }
      throw new IllegalArgumentException("TA_EMA open: " + retCode);
   }
   /**
    * Open a live EMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ema} at that bar.
    * <p>The history must hold at least {@code emaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public EmaStream emaOpen( double inReal[], int optInTimePeriod )
   {
      return emaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#emaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ema} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link EmaStream#fillRange()}.
    */
   public EmaStream emaOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      EmaStream sp = new EmaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = emaOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_EMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_EMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_EMA openAndFill: " + retCode);
   }
