/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AB       Anatoliy Belsky
 *  MF       Mario Fortier
 *  WZ       wony (github @wony-zheng)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  181012 AB    Initial Version
 *  070526 MF,CC  Fix #98: the unstable period grew the summation window
 *                to period+u bars; window is now always 'period'.
 *  070726 WZ,CC  (#14) IMI has no unstable period; drop the unstable-period
 *                term from the lookback so TA_SetUnstablePeriod is a no-op.
 *  071326 MF,CC  Fix #112: an all-flat window (every close==open) leaves
 *                upsum==downsum==0, so 100*(0/0) emitted NaN from a *successful*
 *                call. Guard the divide, returning IMI's neutral center 50.0.
 */

   /**
    * Number of leading input bars {@link Core#IMI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Rolling window length for the up/down body sums
    *        (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int IMI_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode IMI_Impl( int startIdx,
                     int endIdx,
                     double inOpen[],
                     double inClose[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
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
      outIdx = 0;
      lookback = IMI_Lookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = inClose[i];
            double open = inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
             * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
             * oscillator, so no up/down bias returns its neutral center, 50.0.
             */
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode IMI_Impl( int startIdx,
                     int endIdx,
                     float inOpen[],
                     float inClose[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
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
      outIdx = 0;
      lookback = IMI_Lookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = (double)inClose[i];
            double open = (double)inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Intraday Momentum Index: an RSI-like 0-100 oscillator built from the
    * open-to-close body of each bar. Over a rolling window it ratios cumulative
    * up-body moves against total up+down body moves.
    * <p><b>Formula</b>
    * <pre>{@code
    * upsum = Σ(close-open) for bars with close>open; downsum = Σ(open-close) for bars with close<=open, over window [i-lookback, i]; IMI = 100 * upsum/(upsum+downsum)
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#IMI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Rolling window length for the up/down body sums
    *        (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal IMI oscillator value, 0-100. Must hold at least
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
    * @see Core#RSI
    */
   public OutRange IMI( int startIdx,
                        int endIdx,
                        double inOpen[],
                        double inClose[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("IMI", startIdx, endIdx);
      int guardStart = clampedStart("IMI", startIdx, IMI_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("IMI", "inOpen", inOpen, guardInLen);
      requireLength("IMI", "inClose", inClose, guardInLen);
      requireLength("IMI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = IMI_Impl(startIdx, endIdx, inOpen, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("IMI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Intraday Momentum Index: an RSI-like 0-100 oscillator built from the
    * open-to-close body of each bar. Over a rolling window it ratios cumulative
    * up-body moves against total up+down body moves.
    * <p><b>Formula</b>
    * <pre>{@code
    * upsum = Σ(close-open) for bars with close>open; downsum = Σ(open-close) for bars with close<=open, over window [i-lookback, i]; IMI = 100 * upsum/(upsum+downsum)
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#IMI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Rolling window length for the up/down body sums
    *        (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal IMI oscillator value, 0-100. Must hold at least
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
    * @see Core#RSI
    */
   public OutRange IMI( int startIdx,
                        int endIdx,
                        float inOpen[],
                        float inClose[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("IMI", startIdx, endIdx);
      int guardStart = clampedStart("IMI", startIdx, IMI_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("IMI", "inOpen", inOpen, guardInLen);
      requireLength("IMI", "inClose", inClose, guardInLen);
      requireLength("IMI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = IMI_Impl(startIdx, endIdx, inOpen, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("IMI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live IMI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#IMI} over the same series.
    * Open with {@link Core#imiOpen}; there is no close — the handle is
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
   public static final class ImiStream {
      Core core;
      int optInTimePeriod;
      int winPos_i;
      int winCap_i;
      double[] win_i_inOpen;
      double[] win_i_inClose;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      ImiStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#IMI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      ImiStream( ImiStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.winPos_i = other.winPos_i;
         this.winCap_i = other.winCap_i;
         this.win_i_inOpen = other.win_i_inOpen.clone();
         this.win_i_inClose = other.win_i_inClose.clone();
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
      public double update( double inOpen, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("IMI update: BadParam", RetCode.BadParam);
         }
         core.imiStepImpl(this, inOpen, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inOpen.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inOpen[], double inClose[], double outReal[] ) {
         requireArgument("IMI updateAndFill", "inOpen", inOpen);
         requireArgument("IMI updateAndFill", "inClose", inClose);
         requireArgument("IMI updateAndFill", "outReal", outReal);
         final int barCount = inOpen.length;
         if( inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inOpen || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("IMI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("IMI updateAndFill: BadParam", RetCode.BadParam);
            }
            core.imiStepImpl(this, inOpen[i], inClose[i]);
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
      public double peek( double inOpen, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("IMI peek: BadParam", RetCode.BadParam);
         ImiStream sp = this;
         double upsum = 0.0;
         double downsum = 0.0;
         int i = 0;
         double close = 0.0;
         double open = 0.0;
         double cur_outReal = sp.cur_outReal;
         int winPos_i = sp.winPos_i;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         pkSlot0 = winPos_i;
         pkVal0 = inOpen;
         pkSlot1 = winPos_i;
         pkVal1 = inClose;
         upsum = 0.0;
         downsum = 0.0;
         for( i = sp.optInTimePeriod - 1; i >= 0; i -= 1 ) {
            close = (((winPos_i + sp.winCap_i - i >= sp.winCap_i) ? winPos_i + sp.winCap_i - i - sp.winCap_i : winPos_i + sp.winCap_i - i) != pkSlot1) ? sp.win_i_inClose[(winPos_i + sp.winCap_i - i >= sp.winCap_i) ? winPos_i + sp.winCap_i - i - sp.winCap_i : winPos_i + sp.winCap_i - i] : pkVal1;
            open = (((winPos_i + sp.winCap_i - i >= sp.winCap_i) ? winPos_i + sp.winCap_i - i - sp.winCap_i : winPos_i + sp.winCap_i - i) != pkSlot0) ? sp.win_i_inOpen[(winPos_i + sp.winCap_i - i >= sp.winCap_i) ? winPos_i + sp.winCap_i - i - sp.winCap_i : winPos_i + sp.winCap_i - i] : pkVal0;
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
             * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
             * oscillator, so no up/down bias returns its neutral center, 50.0.
             */
            cur_outReal = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         winPos_i = winPos_i + 1;
         if( winPos_i >= sp.winCap_i ) {
            winPos_i = 0;
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
      public ImiStream clone() {
         return new ImiStream(this);
      }
   }
   void imiStepImpl( ImiStream sp, double inOpen, double inClose )
   {
      double upsum = 0.0;
      double downsum = 0.0;
      int i = 0;
      double close = 0.0;
      double open = 0.0;
      sp.win_i_inOpen[sp.winPos_i] = inOpen;
      sp.win_i_inClose[sp.winPos_i] = inClose;
      upsum = 0.0;
      downsum = 0.0;
      for( i = sp.optInTimePeriod - 1; i >= 0; i -= 1 ) {
         close = sp.win_i_inClose[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
         open = sp.win_i_inOpen[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
         if( close > open ) {
            upsum += close - open;
         } else {
            downsum += open - close;
         }
         /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
          * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
          * oscillator, so no up/down bias returns its neutral center, 50.0.
          */
         sp.cur_outReal = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
      }
      sp.winPos_i = sp.winPos_i + 1;
      if( sp.winPos_i >= sp.winCap_i ) {
         sp.winPos_i = 0;
      }
   }
   private RetCode imiOpenImpl( ImiStream sp, double inOpen[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int lookback = 0;
      int outIdx = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      outIdx = 0;
      lookback = IMI_Lookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = inClose[i];
            double open = inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
             * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
             * oscillator, so no up/down bias returns its neutral center, 50.0.
             */
            outReal[outIdx * outStride] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInTimePeriod - 1 + 1);
      if( cap_i < 1 || cap_i > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_i_inOpen = new double[cap_i];
      System.arraycopy(inOpen, historyLen - cap_i, capWin_i_inOpen, 0, cap_i);
      double[] capWin_i_inClose = new double[cap_i];
      System.arraycopy(inClose, historyLen - cap_i, capWin_i_inClose, 0, cap_i);
      sp.optInTimePeriod = optInTimePeriod;
      sp.winPos_i = 0;
      sp.winCap_i = cap_i;
      sp.win_i_inOpen = capWin_i_inOpen;
      sp.win_i_inClose = capWin_i_inClose;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* imiOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   ImiStream imiOpenAndFillInternal( double inOpen[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      ImiStream sp = new ImiStream(this);
      RetCode retCode = imiOpenImpl(sp, inOpen, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("IMI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("IMI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("IMI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind imiOpen (composition seam). */
   ImiStream imiOpenInternal( double inOpen[], double inClose[], int startIdx, int optInTimePeriod )
   {
      ImiStream sp = new ImiStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = imiOpenImpl(sp, inOpen, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("IMI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("IMI open: internal error", retCode);
      }
      throw new TaLibArgumentException("IMI open: " + retCode, retCode);
   }
   /**
    * Open a live IMI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#IMI} at that bar.
    * <p>The history must hold at least {@code IMI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public ImiStream imiOpen( double inOpen[], double inClose[], int optInTimePeriod )
   {
      requireArgument("IMI open", "inOpen", inOpen);
      requireHistory("IMI open", inOpen.length);
      requireArgument("IMI open", "inClose", inClose);
      requireHistoryLength("IMI open", "inClose", inClose.length, inOpen.length);
      return imiOpenInternal(inOpen, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#imiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#IMI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link ImiStream#outRange()}.
    */
   public ImiStream imiOpenAndFill( double inOpen[], double inClose[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("IMI openAndFill", "inOpen", inOpen);
      requireHistory("IMI openAndFill", inOpen.length);
      requireArgument("IMI openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("IMI openAndFill", inOpen.length, IMI_Lookback(optInTimePeriod));
      requireHistoryLength("IMI openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("IMI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inOpen || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("IMI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return imiOpenAndFillInternal(inOpen, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
