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
   RetCode AVGDEV_Internal( int startIdx,
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
   RetCode AVGDEV_Internal( int startIdx,
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AVGDEV_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AVGDEV_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AVGDEV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live AVGDEV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#AVGDEV} over the same series.
    * Open with {@link Core#AVGDEV_Open}; there is no close — the handle is
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
   public static final class AVGDEV_Stream {
      final Core core;
      int optInTimePeriod;
      int winPos_i;
      int winCap_i;
      double[] win_i_inReal;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      AVGDEV_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#AVGDEV_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      AVGDEV_Stream( AVGDEV_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.winPos_i = other.winPos_i;
         this.winCap_i = other.winCap_i;
         this.win_i_inReal = other.win_i_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.AVGDEV_StreamStep(this, inReal);
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
         AVGDEV_Stream scratch = new AVGDEV_Stream(this);
         core.AVGDEV_StreamStep(scratch, inReal);
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
      public AVGDEV_Stream copy() {
         return new AVGDEV_Stream(this);
      }
   }
   void AVGDEV_StreamStep( AVGDEV_Stream sp, double inReal )
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
   private RetCode AVGDEV_OpenBody( AVGDEV_Stream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
         return RetCode.OutOfRangeEndIndex ;
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
         lastValue_outReal = todayDev / optInTimePeriod;
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
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode AVGDEV_OpenAndFillBody( AVGDEV_Stream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
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
         return RetCode.OutOfRangeEndIndex ;
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
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind AVGDEV_Open (composition seam). */
   AVGDEV_Stream AVGDEV_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      AVGDEV_Stream sp = new AVGDEV_Stream(this);
      RetCode retCode = AVGDEV_OpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("AVGDEV open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("AVGDEV open: internal error");
      }
      throw new IllegalArgumentException("AVGDEV open: " + retCode);
   }
   /**
    * Open a live AVGDEV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#AVGDEV} at that bar.
    * <p>The history must hold at least {@code AVGDEV_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public AVGDEV_Stream AVGDEV_Open( double inReal[], int optInTimePeriod )
   {
      return AVGDEV_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#AVGDEV_Open} that also fills the output array(s) bit-identically
    * to {@link Core#AVGDEV} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link AVGDEV_Stream#fillRange()}.
    */
   public AVGDEV_Stream AVGDEV_OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      AVGDEV_Stream sp = new AVGDEV_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AVGDEV_OpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("AVGDEV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("AVGDEV openAndFill: internal error");
      }
      throw new IllegalArgumentException("AVGDEV openAndFill: " + retCode);
   }
