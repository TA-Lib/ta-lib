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
    * Number of leading input bars {@link Core#ADD} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ADD_Lookback( )
   {
      return 0 ;

   }
   RetCode ADD_Impl( int startIdx,
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
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = inReal0[i] + inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode ADD_Impl( int startIdx,
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
         outReal[outIdx] = (double)inReal0[i] + (double)inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Element-wise addition of two input series.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = inReal0[i] + inReal1[i]
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ADD_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 First operand series.
    * @param inReal1 Second operand series.
    * @param outReal Element-wise sum of the two inputs. Must hold at least
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
    * @see Core#SUB
    * @see Core#MULT
    * @see Core#DIV
    */
   public OutRange ADD( int startIdx,
                        int endIdx,
                        double inReal0[],
                        double inReal1[],
                        double outReal[] )
   {
      requireIndexRange("ADD", startIdx, endIdx);
      int guardStart = clampedStart("ADD", startIdx, ADD_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ADD", "inReal0", inReal0, guardInLen);
      requireLength("ADD", "inReal1", inReal1, guardInLen);
      requireLength("ADD", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ADD_Impl(startIdx, endIdx, inReal0, inReal1, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ADD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Element-wise addition of two input series.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = inReal0[i] + inReal1[i]
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ADD_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 First operand series.
    * @param inReal1 Second operand series.
    * @param outReal Element-wise sum of the two inputs. Must hold at least
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
    * @see Core#SUB
    * @see Core#MULT
    * @see Core#DIV
    */
   public OutRange ADD( int startIdx,
                        int endIdx,
                        float inReal0[],
                        float inReal1[],
                        double outReal[] )
   {
      requireIndexRange("ADD", startIdx, endIdx);
      int guardStart = clampedStart("ADD", startIdx, ADD_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ADD", "inReal0", inReal0, guardInLen);
      requireLength("ADD", "inReal1", inReal1, guardInLen);
      requireLength("ADD", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ADD_Impl(startIdx, endIdx, inReal0, inReal1, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ADD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ADD stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ADD} over the same series.
    * Open with {@link Core#addOpen}; there is no close — the handle is
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
   public static final class AddStream {
      Core core;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      AddStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ADD} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      AddStream( AddStream other ) {
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
      public double update( double inReal0, double inReal1 ) {
         if( !Double.isFinite(inReal0) || !Double.isFinite(inReal1) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("ADD update: BadParam", RetCode.BadParam);
         }
         core.addStepImpl(this, inReal0, inReal1);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal0.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal0[], double inReal1[], double outReal[] ) {
         requireArgument("ADD updateAndFill", "inReal0", inReal0);
         requireArgument("ADD updateAndFill", "inReal1", inReal1);
         requireArgument("ADD updateAndFill", "outReal", outReal);
         final int barCount = inReal0.length;
         if( inReal1.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 )
            throw new TaLibArgumentException("ADD updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal0[i]) || !Double.isFinite(inReal1[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("ADD updateAndFill: BadParam", RetCode.BadParam);
            }
            core.addStepImpl(this, inReal0[i], inReal1[i]);
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
      public double peek( double inReal0, double inReal1 ) {
         if( !Double.isFinite(inReal0) || !Double.isFinite(inReal1) )
            throw new TaLibArgumentException("ADD peek: BadParam", RetCode.BadParam);
         AddStream sp = this;
         double cur_outReal = 0.0;
         cur_outReal = inReal0 + inReal1;
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
      public AddStream clone() {
         return new AddStream(this);
      }
   }
   void addStepImpl( AddStream sp, double inReal0, double inReal1 )
   {
      sp.cur_outReal = inReal0 + inReal1;
   }
   private RetCode addOpenImpl( AddStream sp, double inReal0[], double inReal1[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx * outStride] = inReal0[i] + inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* addOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AddStream addOpenAndFillInternal( double inReal0[], double inReal1[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AddStream sp = new AddStream(this);
      RetCode retCode = addOpenImpl(sp, inReal0, inReal1, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ADD openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ADD openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ADD openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind addOpen (composition seam). */
   AddStream addOpenInternal( double inReal0[], double inReal1[], int startIdx )
   {
      AddStream sp = new AddStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = addOpenImpl(sp, inReal0, inReal1, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ADD open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ADD open: internal error", retCode);
      }
      throw new TaLibArgumentException("ADD open: " + retCode, retCode);
   }
   /**
    * Open a live ADD stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ADD} at that bar.
    * <p>The history must hold at least {@code ADD_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public AddStream addOpen( double inReal0[], double inReal1[] )
   {
      requireArgument("ADD open", "inReal0", inReal0);
      requireHistory("ADD open", inReal0.length);
      requireArgument("ADD open", "inReal1", inReal1);
      requireHistoryLength("ADD open", "inReal1", inReal1.length, inReal0.length);
      return addOpenInternal(inReal0, inReal1, 0);
   }
   /**
    * {@link Core#addOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ADD} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AddStream#outRange()}.
    */
   public AddStream addOpenAndFill( double inReal0[], double inReal1[], double outReal[] )
   {
      requireArgument("ADD openAndFill", "inReal0", inReal0);
      requireHistory("ADD openAndFill", inReal0.length);
      requireArgument("ADD openAndFill", "inReal1", inReal1);
      int guardOutLen = openFillCount("ADD openAndFill", inReal0.length, ADD_Lookback());
      requireHistoryLength("ADD openAndFill", "inReal1", inReal1.length, inReal0.length);
      requireLength("ADD openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 ) {
         throw new TaLibArgumentException("ADD openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return addOpenAndFillInternal(inReal0, inReal1, 0, outBegIdx, outNBElement, outReal);
   }
