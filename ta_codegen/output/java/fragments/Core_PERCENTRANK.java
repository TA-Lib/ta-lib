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
 *  090426 MF,CC  Initial version (#369).
 */

   /**
    * Number of leading input bars {@link Core#PERCENTRANK} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of preceding values the current value is
    *        ranked against (default 100; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int PERCENTRANK_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 100;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode PERCENTRANK_Impl( int startIdx,
                             int endIdx,
                             double inReal[],
                             int optInTimePeriod,
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int count = 0;
      double current = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 100;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = PERCENTRANK_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         current = inReal[today];
         count = 0;
         for( i = optInTimePeriod; i >= 1; i -= 1 ) {
            if( inReal[today - i] < current ) {
               count += 1;
            }
         }
         /* Divide, then scale. (count/N)*100 and 100*count/N are different
          * doubles and the reference implementations round the first way.
          */
         outReal[outIdx] = (double)count / (double)optInTimePeriod * 100.0;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode PERCENTRANK_Impl( int startIdx,
                             int endIdx,
                             float inReal[],
                             int optInTimePeriod,
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int count = 0;
      double current = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 100;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = PERCENTRANK_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         current = (double)inReal[today];
         count = 0;
         for( i = optInTimePeriod; i >= 1; i -= 1 ) {
            if( (double)inReal[today - i] < current ) {
               count += 1;
            }
         }
         outReal[outIdx] = (double)count / (double)optInTimePeriod * 100.0;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Percent Rank: where the current value sits inside the distribution of the
    * values that came before it, as a percentage. Each bar is compared against
    * the {@code optInTimePeriod} values immediately preceding it — the current
    * bar is not part of its own comparison set — and the output is the share of
    * them it exceeds. 0 means the current value is at or below every one of
    * them, 100 means it is above all of them, 50 means it is above half. It is
    * a distribution-relative reading rather than a price-relative one: unlike
    * an oscillator built from ranges or averages, it says nothing about how far
    * the value moved, only how many of its recent predecessors it overtook.
    * That makes it flat-scaled across instruments and directly comparable
    * between them. It is best known as the third leg of Connors' ConnorsRSI,
    * applied there to one-day returns rather than to price.
    * <p><b>Formula</b>
    * <pre>{@code
    * For each bar t, over the previous optInTimePeriod values:
    * count = number of j in [t-optInTimePeriod, t-1] with inReal[j] < inReal[t]
    * PERCENTRANK[t] = ( count / optInTimePeriod ) * 100
    * The comparison is strictly less-than, so a value tied with a predecessor does not count that predecessor.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Ties are strict: a predecessor equal to the current value is not counted. A constant series therefore reports 0 on every bar. TradingView's {@code ta.percentrank} counts ties as well (less-than-or-equal), which reports 100 on that same series; Pine parity is a different function, not a variant of this one, and the two agree only on windows with no repeated values.</li>
    * <li>+0.0 and -0.0 compare equal, so a sign-only difference never contributes to the count.</li>
    * <li>Finite input is a precondition. A NaN in the window fails every comparison, so it silently lowers the rank instead of propagating: the output stays finite and no error is reported.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PERCENTRANK_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series.
    * @param optInTimePeriod Number of preceding values the current value is
    *        ranked against (default 100; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal Percentage of the preceding window strictly below the
    *        current value, 0 to 100. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
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
    * @see Core#WILLR
    * @see Core#STDDEV
    */
   public OutRange PERCENTRANK( int startIdx,
                                int endIdx,
                                double inReal[],
                                int optInTimePeriod,
                                double outReal[] )
   {
      requireIndexRange("PERCENTRANK", startIdx, endIdx);
      int guardStart = clampedStart("PERCENTRANK", startIdx, PERCENTRANK_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PERCENTRANK", "inReal", inReal, guardInLen);
      requireLength("PERCENTRANK", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PERCENTRANK_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PERCENTRANK", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Percent Rank: where the current value sits inside the distribution of the
    * values that came before it, as a percentage. Each bar is compared against
    * the {@code optInTimePeriod} values immediately preceding it — the current
    * bar is not part of its own comparison set — and the output is the share of
    * them it exceeds. 0 means the current value is at or below every one of
    * them, 100 means it is above all of them, 50 means it is above half. It is
    * a distribution-relative reading rather than a price-relative one: unlike
    * an oscillator built from ranges or averages, it says nothing about how far
    * the value moved, only how many of its recent predecessors it overtook.
    * That makes it flat-scaled across instruments and directly comparable
    * between them. It is best known as the third leg of Connors' ConnorsRSI,
    * applied there to one-day returns rather than to price.
    * <p><b>Formula</b>
    * <pre>{@code
    * For each bar t, over the previous optInTimePeriod values:
    * count = number of j in [t-optInTimePeriod, t-1] with inReal[j] < inReal[t]
    * PERCENTRANK[t] = ( count / optInTimePeriod ) * 100
    * The comparison is strictly less-than, so a value tied with a predecessor does not count that predecessor.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Ties are strict: a predecessor equal to the current value is not counted. A constant series therefore reports 0 on every bar. TradingView's {@code ta.percentrank} counts ties as well (less-than-or-equal), which reports 100 on that same series; Pine parity is a different function, not a variant of this one, and the two agree only on windows with no repeated values.</li>
    * <li>+0.0 and -0.0 compare equal, so a sign-only difference never contributes to the count.</li>
    * <li>Finite input is a precondition. A NaN in the window fails every comparison, so it silently lowers the rank instead of propagating: the output stays finite and no error is reported.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PERCENTRANK_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series.
    * @param optInTimePeriod Number of preceding values the current value is
    *        ranked against (default 100; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal Percentage of the preceding window strictly below the
    *        current value, 0 to 100. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
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
    * @see Core#WILLR
    * @see Core#STDDEV
    */
   public OutRange PERCENTRANK( int startIdx,
                                int endIdx,
                                float inReal[],
                                int optInTimePeriod,
                                double outReal[] )
   {
      requireIndexRange("PERCENTRANK", startIdx, endIdx);
      int guardStart = clampedStart("PERCENTRANK", startIdx, PERCENTRANK_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PERCENTRANK", "inReal", inReal, guardInLen);
      requireLength("PERCENTRANK", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PERCENTRANK_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PERCENTRANK", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live PERCENTRANK stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#PERCENTRANK} over the same series.
    * Open with {@link Core#percentrankOpen}; there is no close — the handle is
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
   public static final class PercentrankStream {
      Core core;
      int optInTimePeriod;
      int winPos_i;
      int winCap_i;
      double[] win_i_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      PercentrankStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#PERCENTRANK} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      PercentrankStream( PercentrankStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.winPos_i = other.winPos_i;
         this.winCap_i = other.winCap_i;
         this.win_i_inReal = other.win_i_inReal.clone();
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
            throw new TaLibArgumentException("PERCENTRANK update: BadParam", RetCode.BadParam);
         }
         core.percentrankStepImpl(this, inReal);
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
         requireArgument("PERCENTRANK updateAndFill", "inReal", inReal);
         requireArgument("PERCENTRANK updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("PERCENTRANK updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("PERCENTRANK updateAndFill: BadParam", RetCode.BadParam);
            }
            core.percentrankStepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("PERCENTRANK peek: BadParam", RetCode.BadParam);
         PercentrankStream sp = this;
         int i = 0;
         int count = 0;
         double current = 0.0;
         double cur_outReal = 0.0;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.winPos_i;
         pkVal0 = inReal;
         current = inReal;
         count = 0;
         for( i = sp.optInTimePeriod; i >= 1; i -= 1 ) {
            if( ((((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) != pkSlot0) ? sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0) < current ) {
               count += 1;
            }
         }
         /* Divide, then scale. (count/N)*100 and 100*count/N are different
          * doubles and the reference implementations round the first way.
          */
         cur_outReal = (double)count / (double)sp.optInTimePeriod * 100.0;
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
      public PercentrankStream clone() {
         return new PercentrankStream(this);
      }
   }
   void percentrankStepImpl( PercentrankStream sp, double inReal )
   {
      int i = 0;
      int count = 0;
      double current = 0.0;
      sp.win_i_inReal[sp.winPos_i] = inReal;
      current = inReal;
      count = 0;
      for( i = sp.optInTimePeriod; i >= 1; i -= 1 ) {
         if( sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] < current ) {
            count += 1;
         }
      }
      /* Divide, then scale. (count/N)*100 and 100*count/N are different
       * doubles and the reference implementations round the first way.
       */
      sp.cur_outReal = (double)count / (double)sp.optInTimePeriod * 100.0;
      sp.winPos_i = sp.winPos_i + 1;
      if( sp.winPos_i >= sp.winCap_i ) {
         sp.winPos_i = 0;
      }
   }
   private RetCode percentrankOpenImpl( PercentrankStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int count = 0;
      double current = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 100;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = PERCENTRANK_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         current = inReal[today];
         count = 0;
         for( i = optInTimePeriod; i >= 1; i -= 1 ) {
            if( inReal[today - i] < current ) {
               count += 1;
            }
         }
         /* Divide, then scale. (count/N)*100 and 100*count/N are different
          * doubles and the reference implementations round the first way.
          */
         outReal[outIdx * outStride] = (double)count / (double)optInTimePeriod * 100.0;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInTimePeriod + 1);
      if( cap_i < 1 || cap_i > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_i_inReal = new double[cap_i];
      System.arraycopy(inReal, historyLen - cap_i, capWin_i_inReal, 0, cap_i);
      sp.optInTimePeriod = optInTimePeriod;
      sp.winPos_i = 0;
      sp.winCap_i = cap_i;
      sp.win_i_inReal = capWin_i_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* percentrankOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   PercentrankStream percentrankOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PercentrankStream sp = new PercentrankStream(this);
      RetCode retCode = percentrankOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PERCENTRANK openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PERCENTRANK openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("PERCENTRANK openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind percentrankOpen (composition seam). */
   PercentrankStream percentrankOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      PercentrankStream sp = new PercentrankStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = percentrankOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PERCENTRANK open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PERCENTRANK open: internal error", retCode);
      }
      throw new TaLibArgumentException("PERCENTRANK open: " + retCode, retCode);
   }
   /**
    * Open a live PERCENTRANK stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#PERCENTRANK} at that bar.
    * <p>The history must hold at least {@code PERCENTRANK_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public PercentrankStream percentrankOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("PERCENTRANK open", "inReal", inReal);
      requireHistory("PERCENTRANK open", inReal.length);
      return percentrankOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#percentrankOpen} that also fills the output array(s) bit-identically
    * to {@link Core#PERCENTRANK} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link PercentrankStream#outRange()}.
    */
   public PercentrankStream percentrankOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("PERCENTRANK openAndFill", "inReal", inReal);
      requireHistory("PERCENTRANK openAndFill", inReal.length);
      int guardOutLen = openFillCount("PERCENTRANK openAndFill", inReal.length, PERCENTRANK_Lookback(optInTimePeriod));
      requireLength("PERCENTRANK openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("PERCENTRANK openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return percentrankOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
