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
 *  090426 MF,CC  First version (issue #348).
 */

   /**
    * Number of leading input bars {@link Core#RMA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Number of bars in the seed window, and the
    *        reciprocal of the smoothing factor (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int RMA_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 + this.unstablePeriod[FuncUnstId.RMA.ordinal()] ;

   }
   RetCode RMA_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbRMA = 0;
      double prevRMA = 0;
      double periodTotal = 0;
      double wAlpha = 0;
      double wBeta = 0;
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
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = RMA_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
       * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
       * measures closer to the exact recursion than the 1/period-first spelling
       * at nearly every period. The order is a gated contract, not a preference:
       * TA_RMA(TA_TRANGE(h,l,c),n) must equal TA_ATR(n) bit for bit.
       * The pair is exactly (1, 0) at period 1 -- hence no period-1 arm.
       */
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      /* The arithmetic order is the rest of that contract: the seed accumulates
       * from 0.0 in input order, and the smoothing step stays ONE statement --
       * splitting it unfuses the multiply-add and puts a second latency on the
       * recurrence's dependency chain.
       *
       * In-place (outReal being inReal) is supported: the output index never
       * passes the bar index of any remaining read.
       */
      today = startIdx - lookbackTotal;
      /* Seed with a simple average of the first 'period' values. */
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         periodTotal += inReal[today];
         today += 1;
      }
      prevRMA = periodTotal / optInTimePeriod;
      /* Skip the unstable period. */
      i = this.unstablePeriod[FuncUnstId.RMA.ordinal()];
      while( i != 0 ) {
         prevRMA = Math.fma(wBeta, prevRMA, wAlpha * inReal[today]);
         today += 1;
         i -= 1;
      }
      /* Now start to write the final RMA in the caller
       * provided outReal.
       */
      outIdx = 1;
      outReal[0] = prevRMA;
      /* Now do the number of requested RMA. */
      nbRMA = endIdx - startIdx + 1;
      while( --nbRMA != 0 ) {
         prevRMA = Math.fma(wBeta, prevRMA, wAlpha * inReal[today]);
         outReal[outIdx++] = prevRMA;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode RMA_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbRMA = 0;
      double prevRMA = 0;
      double periodTotal = 0;
      double wAlpha = 0;
      double wBeta = 0;
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
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = RMA_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      today = startIdx - lookbackTotal;
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         periodTotal += (double)inReal[today];
         today += 1;
      }
      prevRMA = periodTotal / optInTimePeriod;
      i = this.unstablePeriod[FuncUnstId.RMA.ordinal()];
      while( i != 0 ) {
         prevRMA = Math.fma(wBeta, prevRMA, wAlpha * (double)inReal[today]);
         today += 1;
         i -= 1;
      }
      outIdx = 1;
      outReal[0] = prevRMA;
      nbRMA = endIdx - startIdx + 1;
      while( --nbRMA != 0 ) {
         prevRMA = Math.fma(wBeta, prevRMA, wAlpha * (double)inReal[today]);
         outReal[outIdx++] = prevRMA;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Wilder's smoothed moving average: an exponential average whose smoothing
    * factor is the reciprocal of the period rather than the {@code 2/(n+1)} of
    * a classic EMA, seeded with a simple average of the first window. J. Welles
    * Wilder Jr. introduced it in 1978 as the smoothing inside RSI, ATR and the
    * directional-movement family; this exposes it as a moving average in its
    * own right. Because the smoothing factor is smaller than an EMA's at the
    * same period, RMA reacts more slowly and gives noticeably more weight to
    * old data: it takes about twice the period to shed the influence of a bar.
    * Read it as a slow trend line — direction and slope matter, individual
    * crossings much less than on a faster average. It travels under five names
    * for one object: RMA (TradingView, pandas-ta), SMMA (MetaTrader), Wilder's
    * Smoothing or Wilder's Average (thinkorswim), {@code wilders} (Tulip),
    * WilderMA (Wealth-Lab).
    * <p><b>Formula</b>
    * <pre>{@code
    * alpha = 1 / N,  beta = 1 - alpha,  N = optInTimePeriod
    * seed at bar N-1:  RMA = ( x[0] + x[1] + ... + x[N-1] ) / N
    * for i >= N:       RMA[i] = alpha * x[i] + beta * RMA[i-1]
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Wilder's own writing uses a period of 14, and pandas-ta defaults to 10. The default here is the one the rest of the moving-average family carries, so a call that swaps one MA for another keeps its period.</li>
    * <li>The smoothing factor being {@code 1/N} is sometimes quoted as "an RMA of N is an EMA of 2N-1". The factors really are identical, since {@code 2/((2N-1)+1)} is {@code 1/N}, but the two seed over different windows: the series differ through the warm-up and only converge as the seed's influence decays.</li>
    * <li>{@code TA_RMA} over {@code TA_TRANGE} is {@code TA_ATR}, bit for bit. The recurrence is spelled here exactly as ATR spells it.</li>
    * <li>The recurrence is the {@code alpha * x + (1 - alpha) * prev} form, which is TradingView Pine's {@code ta.rma} and pandas' {@code ewm(adjust=False)} kernel. Implementations that spell it {@code prev + (x - prev) * alpha} or {@code (prev * (N-1) + x) / N} are algebraically the same average and differ from this one only at the last bits.</li>
    * <li>Being recursive, an output depends on how much history precedes it: the same bar computed from an earlier start differs while the seed still carries weight, and that difference decays by a factor of {@code 1 - 1/N} per bar. The unstable period is how much of the warm-up to discard.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#RMA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Data on which to compute the average.
    * @param optInTimePeriod Number of bars in the seed window, and the
    *        reciprocal of the smoothing factor (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Wilder's smoothed moving average of the input. Must hold at
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
    * @see Core#EMA
    * @see Core#SMA
    * @see Core#ATR
    * @see Core#RSI
    * @see Core#DEMA
    */
   public OutRange RMA( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("RMA", startIdx, endIdx);
      int guardStart = clampedStart("RMA", startIdx, RMA_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("RMA", "inReal", inReal, guardInLen);
      requireLength("RMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = RMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("RMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Wilder's smoothed moving average: an exponential average whose smoothing
    * factor is the reciprocal of the period rather than the {@code 2/(n+1)} of
    * a classic EMA, seeded with a simple average of the first window. J. Welles
    * Wilder Jr. introduced it in 1978 as the smoothing inside RSI, ATR and the
    * directional-movement family; this exposes it as a moving average in its
    * own right. Because the smoothing factor is smaller than an EMA's at the
    * same period, RMA reacts more slowly and gives noticeably more weight to
    * old data: it takes about twice the period to shed the influence of a bar.
    * Read it as a slow trend line — direction and slope matter, individual
    * crossings much less than on a faster average. It travels under five names
    * for one object: RMA (TradingView, pandas-ta), SMMA (MetaTrader), Wilder's
    * Smoothing or Wilder's Average (thinkorswim), {@code wilders} (Tulip),
    * WilderMA (Wealth-Lab).
    * <p><b>Formula</b>
    * <pre>{@code
    * alpha = 1 / N,  beta = 1 - alpha,  N = optInTimePeriod
    * seed at bar N-1:  RMA = ( x[0] + x[1] + ... + x[N-1] ) / N
    * for i >= N:       RMA[i] = alpha * x[i] + beta * RMA[i-1]
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Wilder's own writing uses a period of 14, and pandas-ta defaults to 10. The default here is the one the rest of the moving-average family carries, so a call that swaps one MA for another keeps its period.</li>
    * <li>The smoothing factor being {@code 1/N} is sometimes quoted as "an RMA of N is an EMA of 2N-1". The factors really are identical, since {@code 2/((2N-1)+1)} is {@code 1/N}, but the two seed over different windows: the series differ through the warm-up and only converge as the seed's influence decays.</li>
    * <li>{@code TA_RMA} over {@code TA_TRANGE} is {@code TA_ATR}, bit for bit. The recurrence is spelled here exactly as ATR spells it.</li>
    * <li>The recurrence is the {@code alpha * x + (1 - alpha) * prev} form, which is TradingView Pine's {@code ta.rma} and pandas' {@code ewm(adjust=False)} kernel. Implementations that spell it {@code prev + (x - prev) * alpha} or {@code (prev * (N-1) + x) / N} are algebraically the same average and differ from this one only at the last bits.</li>
    * <li>Being recursive, an output depends on how much history precedes it: the same bar computed from an earlier start differs while the seed still carries weight, and that difference decays by a factor of {@code 1 - 1/N} per bar. The unstable period is how much of the warm-up to discard.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#RMA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Data on which to compute the average.
    * @param optInTimePeriod Number of bars in the seed window, and the
    *        reciprocal of the smoothing factor (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Wilder's smoothed moving average of the input. Must hold at
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
    * @see Core#EMA
    * @see Core#SMA
    * @see Core#ATR
    * @see Core#RSI
    * @see Core#DEMA
    */
   public OutRange RMA( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("RMA", startIdx, endIdx);
      int guardStart = clampedStart("RMA", startIdx, RMA_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("RMA", "inReal", inReal, guardInLen);
      requireLength("RMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = RMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("RMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live RMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#RMA} over the same series.
    * Open with {@link Core#rmaOpen}; there is no close — the handle is
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
   public static final class RmaStream {
      Core core;
      int optInTimePeriod;
      double prevRMA;
      double wAlpha;
      double wBeta;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      RmaStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#RMA} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      RmaStream( RmaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevRMA = other.prevRMA;
         this.wAlpha = other.wAlpha;
         this.wBeta = other.wBeta;
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
            throw new TaLibArgumentException("RMA update: BadParam", RetCode.BadParam);
         }
         core.rmaStepImpl(this, inReal);
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
         requireArgument("RMA updateAndFill", "inReal", inReal);
         requireArgument("RMA updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("RMA updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("RMA updateAndFill: BadParam", RetCode.BadParam);
            }
            core.rmaStepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("RMA peek: BadParam", RetCode.BadParam);
         RmaStream sp = this;
         double cur_outReal = sp.cur_outReal;
         double prevRMA = sp.prevRMA;
         prevRMA = Math.fma(sp.wBeta, prevRMA, sp.wAlpha * inReal);
         cur_outReal = prevRMA;
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
      public RmaStream clone() {
         return new RmaStream(this);
      }
   }
   void rmaStepImpl( RmaStream sp, double inReal )
   {
      sp.prevRMA = Math.fma(sp.wBeta, sp.prevRMA, sp.wAlpha * inReal);
      sp.cur_outReal = sp.prevRMA;
   }
   private RetCode rmaOpenImpl( RmaStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbRMA = 0;
      double prevRMA = 0;
      double periodTotal = 0;
      double wAlpha = 0;
      double wBeta = 0;
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
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = RMA_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      /* wAlpha is derived FROM wBeta, never the reverse: only that order makes
       * wAlpha + wBeta exactly 1 (Sterbenz -- wBeta lands in [0.5, 1)), and it
       * measures closer to the exact recursion than the 1/period-first spelling
       * at nearly every period. The order is a gated contract, not a preference:
       * TA_RMA(TA_TRANGE(h,l,c),n) must equal TA_ATR(n) bit for bit.
       * The pair is exactly (1, 0) at period 1 -- hence no period-1 arm.
       */
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      /* The arithmetic order is the rest of that contract: the seed accumulates
       * from 0.0 in input order, and the smoothing step stays ONE statement --
       * splitting it unfuses the multiply-add and puts a second latency on the
       * recurrence's dependency chain.
       *
       * In-place (outReal being inReal) is supported: the output index never
       * passes the bar index of any remaining read.
       */
      today = startIdx - lookbackTotal;
      /* Seed with a simple average of the first 'period' values. */
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         periodTotal += inReal[today];
         today += 1;
      }
      prevRMA = periodTotal / optInTimePeriod;
      /* Skip the unstable period. */
      i = this.unstablePeriod[FuncUnstId.RMA.ordinal()];
      while( i != 0 ) {
         prevRMA = Math.fma(wBeta, prevRMA, wAlpha * inReal[today]);
         today += 1;
         i -= 1;
      }
      /* Now start to write the final RMA in the caller
       * provided outReal.
       */
      outIdx = 1;
      outReal[0 * outStride] = prevRMA;
      /* Now do the number of requested RMA. */
      nbRMA = endIdx - startIdx + 1;
      while( --nbRMA != 0 ) {
         prevRMA = Math.fma(wBeta, prevRMA, wAlpha * inReal[today]);
         outReal[outIdx++ * outStride] = prevRMA;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.prevRMA = prevRMA;
      sp.wAlpha = wAlpha;
      sp.wBeta = wBeta;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* rmaOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   RmaStream rmaOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      RmaStream sp = new RmaStream(this);
      RetCode retCode = rmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("RMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("RMA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("RMA openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind rmaOpen (composition seam). */
   RmaStream rmaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      RmaStream sp = new RmaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = rmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("RMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("RMA open: internal error", retCode);
      }
      throw new TaLibArgumentException("RMA open: " + retCode, retCode);
   }
   /**
    * Open a live RMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#RMA} at that bar.
    * <p>The history must hold at least {@code RMA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public RmaStream rmaOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("RMA open", "inReal", inReal);
      requireHistory("RMA open", inReal.length);
      return rmaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#rmaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#RMA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link RmaStream#outRange()}.
    */
   public RmaStream rmaOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("RMA openAndFill", "inReal", inReal);
      requireHistory("RMA openAndFill", inReal.length);
      int guardOutLen = openFillCount("RMA openAndFill", inReal.length, RMA_Lookback(optInTimePeriod));
      requireLength("RMA openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("RMA openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return rmaOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
