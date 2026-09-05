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
 *  090426 MF,CC  Initial version (#370).
 */

   /**
    * Number of leading input bars {@link Core#RVOL} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of preceding bars averaged to form the
    *        baseline (default 20; range 1..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int RVOL_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode RVOL_Impl( int startIdx,
                      int endIdx,
                      double inVolume[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double periodTotal = 0;
      double baseline = 0;
      double todayVolume = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* One bar more than a moving average of the same period: today is excluded
       * from its own baseline.
       */
      lookbackTotal = (int)optInTimePeriod;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotal = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      while( i < startIdx ) {
         periodTotal += (double)inVolume[i];
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         /* Drop the trailing bar BEFORE adding today's. That order makes each
          * baseline bit-identical to the moving average of the same period at the
          * previous bar; the reverse order differs only in the last ulp, so no
          * tolerance can tell the two apart.
          */
         baseline = periodTotal / (double)optInTimePeriod;
         periodTotal -= (double)inVolume[trailingIdx];
         trailingIdx = trailingIdx + 1;
         todayVolume = (double)inVolume[i];
         i = i + 1;
         periodTotal += todayVolume;
         outReal[outIdx] = todayVolume / baseline;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode RVOL_Impl( int startIdx,
                      int endIdx,
                      float inVolume[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double periodTotal = 0;
      double baseline = 0;
      double todayVolume = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = (int)optInTimePeriod;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotal = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      while( i < startIdx ) {
         periodTotal += (double)inVolume[i];
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         baseline = periodTotal / (double)optInTimePeriod;
         periodTotal -= (double)inVolume[trailingIdx];
         trailingIdx = trailingIdx + 1;
         todayVolume = (double)inVolume[i];
         i = i + 1;
         periodTotal += todayVolume;
         outReal[outIdx] = todayVolume / baseline;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Relative Volume: today's volume as a ratio to the average volume of the
    * bars that came before it. A value of 1 means the bar traded exactly its
    * recent average, above 1 means unusual participation, below 1 means the
    * move is thin. Because the current bar is excluded from its own baseline, a
    * volume spike shows up at full size instead of being diluted by the average
    * it is compared against. Read it as a confirmation filter rather than a
    * direction signal: it says how much conviction is behind a price move, not
    * which way. Breakouts on a high ratio are the ones that tend to follow
    * through; the same breakout near 1 is the one to distrust.
    * <p><b>Formula</b>
    * <pre>{@code
    * RVOL_t = Volume_t / ( (1/N) * sum_{i=t-N}^{t-1} Volume_i ), N = optInTimePeriod
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The baseline is the mean of the N bars *preceding* the current one, so RVOL needs one bar more than a moving average of the same period before it emits a value.</li>
    * <li>A window in which every bar traded nothing has a baseline of zero and no defined ratio: that element is ±Inf, or NaN when the current bar is also zero. Real volume is non-negative, so this only happens on a dead window — an instrument that did not trade at all, or a series carrying no volume, such as a cash-index feed.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#RVOL_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod Number of preceding bars averaged to form the
    *        baseline (default 20; range 1..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param outReal Ratio of the current bar's volume to the average of the
    *        preceding window. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#OBV
    * @see Core#PVO
    * @see Core#VWMA
    * @see Core#SMA
    */
   public OutRange RVOL( int startIdx,
                         int endIdx,
                         double inVolume[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("RVOL", startIdx, endIdx);
      int guardStart = clampedStart("RVOL", startIdx, RVOL_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("RVOL", "inVolume", inVolume, guardInLen);
      requireLength("RVOL", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = RVOL_Impl(startIdx, endIdx, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("RVOL", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Relative Volume: today's volume as a ratio to the average volume of the
    * bars that came before it. A value of 1 means the bar traded exactly its
    * recent average, above 1 means unusual participation, below 1 means the
    * move is thin. Because the current bar is excluded from its own baseline, a
    * volume spike shows up at full size instead of being diluted by the average
    * it is compared against. Read it as a confirmation filter rather than a
    * direction signal: it says how much conviction is behind a price move, not
    * which way. Breakouts on a high ratio are the ones that tend to follow
    * through; the same breakout near 1 is the one to distrust.
    * <p><b>Formula</b>
    * <pre>{@code
    * RVOL_t = Volume_t / ( (1/N) * sum_{i=t-N}^{t-1} Volume_i ), N = optInTimePeriod
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The baseline is the mean of the N bars *preceding* the current one, so RVOL needs one bar more than a moving average of the same period before it emits a value.</li>
    * <li>A window in which every bar traded nothing has a baseline of zero and no defined ratio: that element is ±Inf, or NaN when the current bar is also zero. Real volume is non-negative, so this only happens on a dead window — an instrument that did not trade at all, or a series carrying no volume, such as a cash-index feed.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#RVOL_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod Number of preceding bars averaged to form the
    *        baseline (default 20; range 1..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param outReal Ratio of the current bar's volume to the average of the
    *        preceding window. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#OBV
    * @see Core#PVO
    * @see Core#VWMA
    * @see Core#SMA
    */
   public OutRange RVOL( int startIdx,
                         int endIdx,
                         float inVolume[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("RVOL", startIdx, endIdx);
      int guardStart = clampedStart("RVOL", startIdx, RVOL_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("RVOL", "inVolume", inVolume, guardInLen);
      requireLength("RVOL", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = RVOL_Impl(startIdx, endIdx, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("RVOL", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live RVOL stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#RVOL} over the same series.
    * Open with {@link Core#rvolOpen}; there is no close — the handle is
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
   public static final class RvolStream {
      Core core;
      int optInTimePeriod;
      double periodTotal;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inVolume;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      RvolStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#RVOL} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      RvolStream( RvolStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.periodTotal = other.periodTotal;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inVolume = other.ring_trailingIdx_inVolume.clone();
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
      public double update( double inVolume ) {
         if( !Double.isFinite(inVolume) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("RVOL update: BadParam", RetCode.BadParam);
         }
         core.rvolStepImpl(this, inVolume);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inVolume.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inVolume[], double outReal[] ) {
         requireArgument("RVOL updateAndFill", "inVolume", inVolume);
         requireArgument("RVOL updateAndFill", "outReal", outReal);
         final int barCount = inVolume.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("RVOL updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inVolume[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("RVOL updateAndFill: BadParam", RetCode.BadParam);
            }
            core.rvolStepImpl(this, inVolume[i]);
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
      public double peek( double inVolume ) {
         if( !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("RVOL peek: BadParam", RetCode.BadParam);
         RvolStream sp = this;
         double baseline = 0.0;
         double todayVolume = 0.0;
         double cur_outReal = 0.0;
         double periodTotal = sp.periodTotal;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inVolume;
         }
         /* Drop the trailing bar BEFORE adding today's. That order makes each
          * baseline bit-identical to the moving average of the same period at the
          * previous bar; the reverse order differs only in the last ulp, so no
          * tolerance can tell the two apart.
          */
         baseline = periodTotal / (double)sp.optInTimePeriod;
         periodTotal -= (double)((sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx] : pkVal0);
         todayVolume = (double)inVolume;
         periodTotal += todayVolume;
         cur_outReal = todayVolume / baseline;
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
      public RvolStream clone() {
         return new RvolStream(this);
      }
   }
   void rvolStepImpl( RvolStream sp, double inVolume )
   {
      double baseline = 0.0;
      double todayVolume = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inVolume[0] = inVolume;
      }
      /* Drop the trailing bar BEFORE adding today's. That order makes each
       * baseline bit-identical to the moving average of the same period at the
       * previous bar; the reverse order differs only in the last ulp, so no
       * tolerance can tell the two apart.
       */
      baseline = sp.periodTotal / (double)sp.optInTimePeriod;
      sp.periodTotal -= (double)sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx];
      todayVolume = (double)inVolume;
      sp.periodTotal += todayVolume;
      sp.cur_outReal = todayVolume / baseline;
      sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx] = inVolume;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode rvolOpenImpl( RvolStream sp, double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double periodTotal = 0;
      double baseline = 0;
      double todayVolume = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inVolume.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* One bar more than a moving average of the same period: today is excluded
       * from its own baseline.
       */
      lookbackTotal = (int)optInTimePeriod;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      periodTotal = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      while( i < startIdx ) {
         periodTotal += (double)inVolume[i];
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         /* Drop the trailing bar BEFORE adding today's. That order makes each
          * baseline bit-identical to the moving average of the same period at the
          * previous bar; the reverse order differs only in the last ulp, so no
          * tolerance can tell the two apart.
          */
         baseline = periodTotal / (double)optInTimePeriod;
         periodTotal -= (double)inVolume[trailingIdx];
         trailingIdx = trailingIdx + 1;
         todayVolume = (double)inVolume[i];
         i = i + 1;
         periodTotal += todayVolume;
         outReal[outIdx * outStride] = todayVolume / baseline;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = i - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inVolume = new double[allocN_trailingIdx];
      System.arraycopy(inVolume, historyLen - cap_trailingIdx, capRing_trailingIdx_inVolume, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.periodTotal = periodTotal;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inVolume = capRing_trailingIdx_inVolume;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* rvolOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   RvolStream rvolOpenAndFillInternal( double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      RvolStream sp = new RvolStream(this);
      RetCode retCode = rvolOpenImpl(sp, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("RVOL openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("RVOL openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("RVOL openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind rvolOpen (composition seam). */
   RvolStream rvolOpenInternal( double inVolume[], int startIdx, int optInTimePeriod )
   {
      RvolStream sp = new RvolStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = rvolOpenImpl(sp, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("RVOL open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("RVOL open: internal error", retCode);
      }
      throw new TaLibArgumentException("RVOL open: " + retCode, retCode);
   }
   /**
    * Open a live RVOL stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#RVOL} at that bar.
    * <p>The history must hold at least {@code RVOL_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public RvolStream rvolOpen( double inVolume[], int optInTimePeriod )
   {
      requireArgument("RVOL open", "inVolume", inVolume);
      requireHistory("RVOL open", inVolume.length);
      return rvolOpenInternal(inVolume, 0, optInTimePeriod);
   }
   /**
    * {@link Core#rvolOpen} that also fills the output array(s) bit-identically
    * to {@link Core#RVOL} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link RvolStream#outRange()}.
    */
   public RvolStream rvolOpenAndFill( double inVolume[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("RVOL openAndFill", "inVolume", inVolume);
      requireHistory("RVOL openAndFill", inVolume.length);
      int guardOutLen = openFillCount("RVOL openAndFill", inVolume.length, RVOL_Lookback(optInTimePeriod));
      requireLength("RVOL openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("RVOL openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return rvolOpenAndFillInternal(inVolume, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
