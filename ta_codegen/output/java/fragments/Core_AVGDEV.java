/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AB       Anatoliy Belsky
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090812 AB     Initial Version
 */

   /**
    * Number of leading input bars {@link Core#AVGDEV} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window length (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int AVGDEV_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode AVGDEV_Impl( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
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
      lookback = optInTimePeriod - 1;
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      today = startIdx;
      /* Make sure there is still something to evaluate. */
      if( today > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Process the initial DM and TR */
      outBegIdx.value = today;
      outIdx = 0;
      while( today <= endIdx ) {
         double todaySum;
         double todayDev;
         int i;
         todaySum = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todaySum += inReal[today - i];
         }
         todayDev = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todayDev += Math.abs(inReal[today - i] - todaySum / optInTimePeriod);
         }
         outReal[outIdx] = todayDev / optInTimePeriod;
         outIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode AVGDEV_Impl( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
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
      lookback = optInTimePeriod - 1;
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      today = startIdx;
      if( today > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = today;
      outIdx = 0;
      while( today <= endIdx ) {
         double todaySum;
         double todayDev;
         int i;
         todaySum = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todaySum += (double)inReal[today - i];
         }
         todayDev = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todayDev += Math.abs((double)inReal[today - i] - todaySum / optInTimePeriod);
         }
         outReal[outIdx] = todayDev / optInTimePeriod;
         outIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Rolling average absolute deviation of a series from its own simple moving
    * average over the last N periods. Measures dispersion around the window
    * mean. Higher values indicate greater spread; zero when all values in the
    * window are equal.
    * <p><b>Formula</b>
    * <pre>{@code
    * $mean_t = \frac{1}{N}\sum_{i=0}^{N-1} x_{t-i}$; $AVGDEV_t = \frac{1}{N}\sum_{i=0}^{N-1} |x_{t-i} - mean_t|$ (N = optInTimePeriod)
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AVGDEV_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal source series.
    * @param optInTimePeriod Window length (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal mean absolute deviation over the window. Must hold at least
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
    * @see Core#STDDEV
    * @see Core#VAR
    * @see Core#SMA
    */
   public OutRange AVGDEV( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      requireIndexRange("AVGDEV", startIdx, endIdx);
      int guardStart = clampedStart("AVGDEV", startIdx, AVGDEV_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AVGDEV", "inReal", inReal, guardInLen);
      requireLength("AVGDEV", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AVGDEV_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AVGDEV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rolling average absolute deviation of a series from its own simple moving
    * average over the last N periods. Measures dispersion around the window
    * mean. Higher values indicate greater spread; zero when all values in the
    * window are equal.
    * <p><b>Formula</b>
    * <pre>{@code
    * $mean_t = \frac{1}{N}\sum_{i=0}^{N-1} x_{t-i}$; $AVGDEV_t = \frac{1}{N}\sum_{i=0}^{N-1} |x_{t-i} - mean_t|$ (N = optInTimePeriod)
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AVGDEV_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal source series.
    * @param optInTimePeriod Window length (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal mean absolute deviation over the window. Must hold at least
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
    * @see Core#STDDEV
    * @see Core#VAR
    * @see Core#SMA
    */
   public OutRange AVGDEV( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      requireIndexRange("AVGDEV", startIdx, endIdx);
      int guardStart = clampedStart("AVGDEV", startIdx, AVGDEV_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AVGDEV", "inReal", inReal, guardInLen);
      requireLength("AVGDEV", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AVGDEV_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AVGDEV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live AVGDEV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#AVGDEV} over the same series.
    * Open with {@link Core#avgdevOpen}; there is no close — the handle is
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
   public static final class AvgdevStream {
      Core core;
      int optInTimePeriod;
      int winPos_i;
      int winCap_i;
      double[] win_i_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      AvgdevStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#AVGDEV} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      AvgdevStream( AvgdevStream other ) {
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
            throw new TaLibArgumentException("AVGDEV update: BadParam", RetCode.BadParam);
         }
         core.avgdevStepImpl(this, inReal);
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
         requireArgument("AVGDEV updateAndFill", "inReal", inReal);
         requireArgument("AVGDEV updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("AVGDEV updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("AVGDEV updateAndFill: BadParam", RetCode.BadParam);
            }
            core.avgdevStepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("AVGDEV peek: BadParam", RetCode.BadParam);
         AvgdevStream sp = this;
         double todaySum = 0.0;
         double todayDev = 0.0;
         int i = 0;
         double cur_outReal = sp.cur_outReal;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.winPos_i;
         pkVal0 = inReal;
         todaySum = 0.0;
         for( i = 0; i < sp.optInTimePeriod; i += 1 ) {
            todaySum += (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) != pkSlot0) ? sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0;
         }
         todayDev = 0.0;
         for( i = 0; i < sp.optInTimePeriod; i += 1 ) {
            todayDev += Math.abs(((((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) != pkSlot0) ? sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0) - todaySum / sp.optInTimePeriod);
         }
         cur_outReal = todayDev / sp.optInTimePeriod;
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
      public AvgdevStream clone() {
         return new AvgdevStream(this);
      }
   }
   void avgdevStepImpl( AvgdevStream sp, double inReal )
   {
      double todaySum = 0.0;
      double todayDev = 0.0;
      int i = 0;
      sp.win_i_inReal[sp.winPos_i] = inReal;
      todaySum = 0.0;
      for( i = 0; i < sp.optInTimePeriod; i += 1 ) {
         todaySum += sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
      }
      todayDev = 0.0;
      for( i = 0; i < sp.optInTimePeriod; i += 1 ) {
         todayDev += Math.abs(sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] - todaySum / sp.optInTimePeriod);
      }
      sp.cur_outReal = todayDev / sp.optInTimePeriod;
      sp.winPos_i = sp.winPos_i + 1;
      if( sp.winPos_i >= sp.winCap_i ) {
         sp.winPos_i = 0;
      }
   }
   private RetCode avgdevOpenImpl( AvgdevStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
      lookback = optInTimePeriod - 1;
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      today = startIdx;
      /* Make sure there is still something to evaluate. */
      if( today > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Process the initial DM and TR */
      outBegIdx.value = today;
      outIdx = 0;
      while( today <= endIdx ) {
         double todaySum;
         double todayDev;
         int i;
         todaySum = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todaySum += inReal[today - i];
         }
         todayDev = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todayDev += Math.abs(inReal[today - i] - todaySum / optInTimePeriod);
         }
         outReal[outIdx * outStride] = todayDev / optInTimePeriod;
         outIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInTimePeriod);
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
   /* avgdevOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AvgdevStream avgdevOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AvgdevStream sp = new AvgdevStream(this);
      RetCode retCode = avgdevOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AVGDEV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AVGDEV openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("AVGDEV openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind avgdevOpen (composition seam). */
   AvgdevStream avgdevOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      AvgdevStream sp = new AvgdevStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = avgdevOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AVGDEV open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AVGDEV open: internal error", retCode);
      }
      throw new TaLibArgumentException("AVGDEV open: " + retCode, retCode);
   }
   /**
    * Open a live AVGDEV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#AVGDEV} at that bar.
    * <p>The history must hold at least {@code AVGDEV_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public AvgdevStream avgdevOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("AVGDEV open", "inReal", inReal);
      requireHistory("AVGDEV open", inReal.length);
      return avgdevOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#avgdevOpen} that also fills the output array(s) bit-identically
    * to {@link Core#AVGDEV} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AvgdevStream#outRange()}.
    */
   public AvgdevStream avgdevOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("AVGDEV openAndFill", "inReal", inReal);
      requireHistory("AVGDEV openAndFill", inReal.length);
      int guardOutLen = openFillCount("AVGDEV openAndFill", inReal.length, AVGDEV_Lookback(optInTimePeriod));
      requireLength("AVGDEV openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("AVGDEV openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return avgdevOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
