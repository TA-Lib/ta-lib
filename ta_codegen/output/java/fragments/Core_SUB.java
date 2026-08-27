/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090807 MF     Initial Version
 */

   /**
    * Number of leading input bars {@link Core#SUB} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int SUB_Lookback( )
   {
      return 0 ;

   }
   RetCode SUB_Impl( int startIdx,
                     int endIdx,
                     double inReal0[],
                     double inReal1[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Default return values */
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = inReal0[i] - inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode SUB_Impl( int startIdx,
                     int endIdx,
                     float inReal0[],
                     float inReal1[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = (double)inReal0[i] - (double)inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Element-wise subtraction of two input series.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = inReal0[i] - inReal1[i]
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SUB_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 Minuend series.
    * @param inReal1 Subtrahend series.
    * @param outReal Per-element difference inReal0 - inReal1. Must hold at
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
    * @see Core#ADD
    * @see Core#MULT
    * @see Core#DIV
    */
   public OutRange SUB( int startIdx,
                        int endIdx,
                        double inReal0[],
                        double inReal1[],
                        double outReal[] )
   {
      requireIndexRange("SUB", startIdx, endIdx);
      int guardStart = clampedStart("SUB", startIdx, SUB_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SUB", "inReal0", inReal0, guardInLen);
      requireLength("SUB", "inReal1", inReal1, guardInLen);
      requireLength("SUB", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SUB_Impl(startIdx, endIdx, inReal0, inReal1, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("SUB", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Element-wise subtraction of two input series.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = inReal0[i] - inReal1[i]
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SUB_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 Minuend series.
    * @param inReal1 Subtrahend series.
    * @param outReal Per-element difference inReal0 - inReal1. Must hold at
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
    * @see Core#ADD
    * @see Core#MULT
    * @see Core#DIV
    */
   public OutRange SUB( int startIdx,
                        int endIdx,
                        float inReal0[],
                        float inReal1[],
                        double outReal[] )
   {
      requireIndexRange("SUB", startIdx, endIdx);
      int guardStart = clampedStart("SUB", startIdx, SUB_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SUB", "inReal0", inReal0, guardInLen);
      requireLength("SUB", "inReal1", inReal1, guardInLen);
      requireLength("SUB", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SUB_Impl(startIdx, endIdx, inReal0, inReal1, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("SUB", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SUB stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#SUB} over the same series.
    * Open with {@link Core#SUB_Open}; there is no close — the handle is
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
   public static final class SUB_Stream {
      Core core;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      SUB_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#SUB} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      SUB_Stream( SUB_Stream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( SUB_Stream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

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
      public double update( double inReal0, double inReal1 ) {
         if( !Double.isFinite(inReal0) || !Double.isFinite(inReal1) )
            throw new TaLibArgumentException("SUB update: BadParam", RetCode.BadParam);
         core.SUB_StepImpl(this, inReal0, inReal1);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal0.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inReal0[], double inReal1[], double outReal[] ) {
         requireArgument("SUB updateAndFill", "inReal0", inReal0);
         requireArgument("SUB updateAndFill", "inReal1", inReal1);
         requireArgument("SUB updateAndFill", "outReal", outReal);
         final int barCount = inReal0.length;
         if( inReal1.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 )
            throw new TaLibArgumentException("SUB updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal0[i]) || !Double.isFinite(inReal1[i]) )
               throw new TaLibArgumentException("SUB updateAndFill: BadParam", RetCode.BadParam);
            core.SUB_StepImpl(this, inReal0[i], inReal1[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inReal0, double inReal1 ) {
         if( !Double.isFinite(inReal0) || !Double.isFinite(inReal1) )
            throw new TaLibArgumentException("SUB peek: BadParam", RetCode.BadParam);
         SUB_Stream scratch = new SUB_Stream(this);
         core.SUB_StepImpl(scratch, inReal0, inReal1);
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
      public SUB_Stream copy() {
         return new SUB_Stream(this);
      }
   }
   void SUB_StepImpl( SUB_Stream sp, double inReal0, double inReal1 )
   {
      sp.cur_outReal = inReal0 - inReal1;
   }
   private RetCode SUB_OpenImpl( SUB_Stream sp, double inReal0[], double inReal1[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int i = 0;
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Default return values */
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx * outStride] = inReal0[i] - inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* SUB_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SUB_Stream SUB_OpenAndFillInternal( double inReal0[], double inReal1[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      SUB_Stream sp = new SUB_Stream(this);
      RetCode retCode = SUB_OpenImpl(sp, inReal0, inReal1, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SUB openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SUB openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("SUB openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind SUB_Open (composition seam). */
   SUB_Stream SUB_OpenInternal( double inReal0[], double inReal1[], int startIdx )
   {
      SUB_Stream sp = new SUB_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = SUB_OpenImpl(sp, inReal0, inReal1, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SUB open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SUB open: internal error", retCode);
      }
      throw new TaLibArgumentException("SUB open: " + retCode, retCode);
   }
   /**
    * Open a live SUB stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#SUB} at that bar.
    * <p>The history must hold at least {@code SUB_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public SUB_Stream SUB_Open( double inReal0[], double inReal1[] )
   {
      requireArgument("SUB open", "inReal0", inReal0);
      requireHistory("SUB open", inReal0.length);
      requireArgument("SUB open", "inReal1", inReal1);
      requireHistoryLength("SUB open", "inReal1", inReal1.length, inReal0.length);
      return SUB_OpenInternal(inReal0, inReal1, 0);
   }
   /**
    * {@link Core#SUB_Open} that also fills the output array(s) bit-identically
    * to {@link Core#SUB} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SUB_Stream#outRange()}.
    */
   public SUB_Stream SUB_OpenAndFill( double inReal0[], double inReal1[], double outReal[] )
   {
      requireArgument("SUB openAndFill", "inReal0", inReal0);
      requireHistory("SUB openAndFill", inReal0.length);
      requireArgument("SUB openAndFill", "inReal1", inReal1);
      int guardOutLen = openFillCount("SUB openAndFill", inReal0.length, SUB_Lookback());
      requireHistoryLength("SUB openAndFill", "inReal1", inReal1.length, inReal0.length);
      requireLength("SUB openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 ) {
         throw new TaLibArgumentException("SUB openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return SUB_OpenAndFillInternal(inReal0, inReal1, 0, outBegIdx, outNBElement, outReal);
   }
