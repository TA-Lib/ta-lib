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
    * Number of leading input bars {@link Core#SINH} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int SINH_Lookback( )
   {
      return 0 ;

   }
   RetCode SINH_Impl( int startIdx,
                      int endIdx,
                      double inReal[],
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
         outReal[outIdx] = Math.sinh(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode SINH_Impl( int startIdx,
                      int endIdx,
                      float inReal[],
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
         outReal[outIdx] = Math.sinh((double)inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Element-wise hyperbolic sine of the input series. A vector math transform
    * applying sinh() to each value.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = sinh(inReal[i])
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SINH_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series.
    * @param outReal Hyperbolic sine of each input value. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#COSH
    * @see Core#TANH
    */
   public OutRange SINH( int startIdx,
                         int endIdx,
                         double inReal[],
                         double outReal[] )
   {
      requireIndexRange("SINH", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, SINH_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SINH", "inReal", inReal, guardInLen);
      requireLength("SINH", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SINH_Impl(startIdx, endIdx, inReal, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("SINH", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Element-wise hyperbolic sine of the input series. A vector math transform
    * applying sinh() to each value.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = sinh(inReal[i])
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SINH_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series.
    * @param outReal Hyperbolic sine of each input value. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#COSH
    * @see Core#TANH
    */
   public OutRange SINH( int startIdx,
                         int endIdx,
                         float inReal[],
                         double outReal[] )
   {
      requireIndexRange("SINH", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, SINH_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SINH", "inReal", inReal, guardInLen);
      requireLength("SINH", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SINH_Impl(startIdx, endIdx, inReal, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("SINH", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SINH stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#SINH} over the same series.
    * Open with {@link Core#SINH_Open}; there is no close — the handle is
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
   public static final class SINH_Stream {
      Core core;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      SINH_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#SINH_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      SINH_Stream( SINH_Stream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( SINH_Stream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
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
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("SINH update: BadParam", RetCode.BadParam);
         core.SINH_StreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("SINH peek: BadParam", RetCode.BadParam);
         SINH_Stream scratch = new SINH_Stream(this);
         core.SINH_StreamStep(scratch, inReal);
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
      public SINH_Stream copy() {
         return new SINH_Stream(this);
      }
   }
   void SINH_StreamStep( SINH_Stream sp, double inReal )
   {
      sp.cur_outReal = Math.sinh(inReal);
   }
   private RetCode SINH_OpenPass( SINH_Stream sp, double inReal[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int i = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx * outStride] = Math.sinh(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode SINH_OpenImpl( SINH_Stream sp, double inReal[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return SINH_OpenPass( sp, inReal, startIdx, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode SINH_OpenAndFillImpl( SINH_Stream sp, double inReal[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      return SINH_OpenPass( sp, inReal, 0, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode SINH_OpenAndFillInternalImpl( SINH_Stream sp, double inReal[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return SINH_OpenPass(sp, inReal, startIdx, outBegIdx, outNBElement, outReal, 1);
   }
   /* SINH_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SINH_Stream SINH_OpenAndFillInternal( double inReal[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      SINH_Stream sp = new SINH_Stream(this);
      RetCode retCode = SINH_OpenAndFillInternalImpl(sp, inReal, startIdx, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SINH openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SINH openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("SINH openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind SINH_Open (composition seam). */
   SINH_Stream SINH_OpenInternal( double inReal[], int startIdx )
   {
      SINH_Stream sp = new SINH_Stream(this);
      RetCode retCode = SINH_OpenImpl(sp, inReal, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SINH open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SINH open: internal error", retCode);
      }
      throw new TaLibArgumentException("SINH open: " + retCode, retCode);
   }
   /**
    * Open a live SINH stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#SINH} at that bar.
    * <p>The history must hold at least {@code SINH_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public SINH_Stream SINH_Open( double inReal[] )
   {
      return SINH_OpenInternal(inReal, 0);
   }
   /**
    * {@link Core#SINH_Open} that also fills the output array(s) bit-identically
    * to {@link Core#SINH} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link SINH_Stream#fillRange()}.
    */
   public SINH_Stream SINH_OpenAndFill( double inReal[], double outReal[] )
   {
      SINH_Stream sp = new SINH_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SINH_OpenAndFillImpl(sp, inReal, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SINH openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SINH openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("SINH openAndFill: " + retCode, retCode);
   }
