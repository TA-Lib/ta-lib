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
 *  090426 MF,CC  First version (issue #364).
 */

   /**
    * Number of leading input bars {@link Core#PVT} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int PVT_Lookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   RetCode PVT_Impl( int startIdx,
                     int endIdx,
                     double inClose[],
                     double inVolume[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevPVT = 0;
      double prevClose = 0;
      double tempClose = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      prevPVT = 0.0;
      prevClose = inClose[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = inClose[i];
         /* Exact test, never an epsilon band: a band carries the quote unit and
          * would zero the indicator for every instrument priced under it (#253).
          * A zero previous close contributes nothing rather than Inf/NaN (#112).
          */
         if( prevClose != 0.0 ) {
            prevPVT += (tempClose - prevClose) / prevClose * inVolume[i];
         }
         outReal[outIdx++] = prevPVT;
         prevClose = tempClose;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode PVT_Impl( int startIdx,
                     int endIdx,
                     float inClose[],
                     float inVolume[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevPVT = 0;
      double prevClose = 0;
      double tempClose = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      prevPVT = 0.0;
      prevClose = (double)inClose[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = (double)inClose[i];
         if( prevClose != 0.0 ) {
            prevPVT += (tempClose - prevClose) / prevClose * (double)inVolume[i];
         }
         outReal[outIdx++] = prevPVT;
         prevClose = tempClose;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Price Volume Trend: a running cumulative total of each bar's volume
    * weighted by that bar's fractional price change. It is the On Balance
    * Volume idea with partial credit — where OBV adds or subtracts a bar's
    * whole volume on the sign of the move, PVT adds only the fraction of that
    * volume proportional to the size of the move. Read the slope and the
    * divergences, not the level: the total's zero point is arbitrary, so only
    * the shape of the curve carries information. A rising PVT while price is
    * flat says volume is accumulating on the up moves; a falling PVT while
    * price rises is the classic bearish divergence.
    * <p><b>Formula</b>
    * <pre>{@code
    * PVT[i] = PVT[i-1] + inVolume[i] * (inClose[i] - inClose[i-1]) / inClose[i-1]
    * The series starts at zero on the first bar of the requested range.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The absolute level is arbitrary and depends on where the accumulation started, so two ranges over the same data give curves of the same shape at different offsets.</li>
    * <li>Some libraries scale the per-bar term by 100. This implementation follows the fractional definition, which every primary reference below states.</li>
    * <li>A bar whose previous close is exactly zero contributes nothing and the running total is carried forward unchanged, rather than dividing by zero. A flat stretch of the output can therefore mean either genuine zero net accumulation or a run of zero previous closes.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PVT_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Cumulative price volume trend, seeded at zero. Must hold at
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
    * @see Core#OBV
    * @see Core#NVI
    * @see Core#PVI
    * @see Core#PVO
    * @see Core#AD
    */
   public OutRange PVT( int startIdx,
                        int endIdx,
                        double inClose[],
                        double inVolume[],
                        double outReal[] )
   {
      requireIndexRange("PVT", startIdx, endIdx);
      int guardStart = clampedStart("PVT", startIdx, PVT_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PVT", "inClose", inClose, guardInLen);
      requireLength("PVT", "inVolume", inVolume, guardInLen);
      requireLength("PVT", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PVT_Impl(startIdx, endIdx, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PVT", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Price Volume Trend: a running cumulative total of each bar's volume
    * weighted by that bar's fractional price change. It is the On Balance
    * Volume idea with partial credit — where OBV adds or subtracts a bar's
    * whole volume on the sign of the move, PVT adds only the fraction of that
    * volume proportional to the size of the move. Read the slope and the
    * divergences, not the level: the total's zero point is arbitrary, so only
    * the shape of the curve carries information. A rising PVT while price is
    * flat says volume is accumulating on the up moves; a falling PVT while
    * price rises is the classic bearish divergence.
    * <p><b>Formula</b>
    * <pre>{@code
    * PVT[i] = PVT[i-1] + inVolume[i] * (inClose[i] - inClose[i-1]) / inClose[i-1]
    * The series starts at zero on the first bar of the requested range.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The absolute level is arbitrary and depends on where the accumulation started, so two ranges over the same data give curves of the same shape at different offsets.</li>
    * <li>Some libraries scale the per-bar term by 100. This implementation follows the fractional definition, which every primary reference below states.</li>
    * <li>A bar whose previous close is exactly zero contributes nothing and the running total is carried forward unchanged, rather than dividing by zero. A flat stretch of the output can therefore mean either genuine zero net accumulation or a run of zero previous closes.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PVT_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Cumulative price volume trend, seeded at zero. Must hold at
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
    * @see Core#OBV
    * @see Core#NVI
    * @see Core#PVI
    * @see Core#PVO
    * @see Core#AD
    */
   public OutRange PVT( int startIdx,
                        int endIdx,
                        float inClose[],
                        float inVolume[],
                        double outReal[] )
   {
      requireIndexRange("PVT", startIdx, endIdx);
      int guardStart = clampedStart("PVT", startIdx, PVT_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PVT", "inClose", inClose, guardInLen);
      requireLength("PVT", "inVolume", inVolume, guardInLen);
      requireLength("PVT", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PVT_Impl(startIdx, endIdx, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PVT", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live PVT stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#PVT} over the same series.
    * Open with {@link Core#pvtOpen}; there is no close — the handle is
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
   public static final class PvtStream {
      Core core;
      double prevPVT;
      double prevClose;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      PvtStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#PVT} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      PvtStream( PvtStream other ) {
         this.core = other.core;
         this.prevPVT = other.prevPVT;
         this.prevClose = other.prevClose;
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
      public double update( double inClose, double inVolume ) {
         if( !Double.isFinite(inClose) || !Double.isFinite(inVolume) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("PVT update: BadParam", RetCode.BadParam);
         }
         core.pvtStepImpl(this, inClose, inVolume);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inClose.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inClose[], double inVolume[], double outReal[] ) {
         requireArgument("PVT updateAndFill", "inClose", inClose);
         requireArgument("PVT updateAndFill", "inVolume", inVolume);
         requireArgument("PVT updateAndFill", "outReal", outReal);
         final int barCount = inClose.length;
         if( inVolume.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("PVT updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inClose[i]) || !Double.isFinite(inVolume[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("PVT updateAndFill: BadParam", RetCode.BadParam);
            }
            core.pvtStepImpl(this, inClose[i], inVolume[i]);
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
      public double peek( double inClose, double inVolume ) {
         if( !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("PVT peek: BadParam", RetCode.BadParam);
         PvtStream sp = this;
         double tempClose = 0.0;
         double cur_outReal = 0.0;
         double prevPVT = sp.prevPVT;
         tempClose = inClose;
         /* Exact test, never an epsilon band: a band carries the quote unit and
          * would zero the indicator for every instrument priced under it (#253).
          * A zero previous close contributes nothing rather than Inf/NaN (#112).
          */
         if( sp.prevClose != 0.0 ) {
            prevPVT += (tempClose - sp.prevClose) / sp.prevClose * inVolume;
         }
         cur_outReal = prevPVT;
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
      public PvtStream clone() {
         return new PvtStream(this);
      }
   }
   void pvtStepImpl( PvtStream sp, double inClose, double inVolume )
   {
      double tempClose = 0.0;
      tempClose = inClose;
      /* Exact test, never an epsilon band: a band carries the quote unit and
       * would zero the indicator for every instrument priced under it (#253).
       * A zero previous close contributes nothing rather than Inf/NaN (#112).
       */
      if( sp.prevClose != 0.0 ) {
         sp.prevPVT += (tempClose - sp.prevClose) / sp.prevClose * inVolume;
      }
      sp.cur_outReal = sp.prevPVT;
      sp.prevClose = tempClose;
   }
   private RetCode pvtOpenImpl( PvtStream sp, double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      double prevPVT = 0;
      double prevClose = 0;
      double tempClose = 0;
      int historyLen = inClose.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inVolume.length != inClose.length ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      prevPVT = 0.0;
      prevClose = inClose[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = inClose[i];
         /* Exact test, never an epsilon band: a band carries the quote unit and
          * would zero the indicator for every instrument priced under it (#253).
          * A zero previous close contributes nothing rather than Inf/NaN (#112).
          */
         if( prevClose != 0.0 ) {
            prevPVT += (tempClose - prevClose) / prevClose * inVolume[i];
         }
         outReal[outIdx++ * outStride] = prevPVT;
         prevClose = tempClose;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.prevPVT = prevPVT;
      sp.prevClose = prevClose;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* pvtOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   PvtStream pvtOpenAndFillInternal( double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PvtStream sp = new PvtStream(this);
      RetCode retCode = pvtOpenImpl(sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PVT openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PVT openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("PVT openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind pvtOpen (composition seam). */
   PvtStream pvtOpenInternal( double inClose[], double inVolume[], int startIdx )
   {
      PvtStream sp = new PvtStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = pvtOpenImpl(sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PVT open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PVT open: internal error", retCode);
      }
      throw new TaLibArgumentException("PVT open: " + retCode, retCode);
   }
   /**
    * Open a live PVT stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#PVT} at that bar.
    * <p>The history must hold at least {@code PVT_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public PvtStream pvtOpen( double inClose[], double inVolume[] )
   {
      requireArgument("PVT open", "inClose", inClose);
      requireHistory("PVT open", inClose.length);
      requireArgument("PVT open", "inVolume", inVolume);
      requireHistoryLength("PVT open", "inVolume", inVolume.length, inClose.length);
      return pvtOpenInternal(inClose, inVolume, 0);
   }
   /**
    * {@link Core#pvtOpen} that also fills the output array(s) bit-identically
    * to {@link Core#PVT} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link PvtStream#outRange()}.
    */
   public PvtStream pvtOpenAndFill( double inClose[], double inVolume[], double outReal[] )
   {
      requireArgument("PVT openAndFill", "inClose", inClose);
      requireHistory("PVT openAndFill", inClose.length);
      requireArgument("PVT openAndFill", "inVolume", inVolume);
      int guardOutLen = openFillCount("PVT openAndFill", inClose.length, PVT_Lookback());
      requireHistoryLength("PVT openAndFill", "inVolume", inVolume.length, inClose.length);
      requireLength("PVT openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("PVT openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return pvtOpenAndFillInternal(inClose, inVolume, 0, outBegIdx, outNBElement, outReal);
   }
