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
    * Number of leading input bars {@link Core#DIV} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int DIV_Lookback( )
   {
      return 0 ;

   }
   RetCode DIV_Internal( int startIdx,
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
         outReal[outIdx] = inReal0[i] / inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode DIV_Internal( int startIdx,
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
         outReal[outIdx] = (double)inReal0[i] / (double)inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Element-wise division of two input series. Computes the quotient of
    * corresponding values from two real inputs.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = inReal0[i] / inReal1[i]
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#DIV_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 Dividend (numerator) series.
    * @param inReal1 Divisor (denominator) series.
    * @param outReal Per-element quotient inReal0/inReal1. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#MULT
    * @see Core#ADD
    * @see Core#SUB
    */
   public OutRange DIV( int startIdx,
                        int endIdx,
                        double inReal0[],
                        double inReal1[],
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = DIV_Internal(startIdx, endIdx, inReal0, inReal1, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("DIV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Element-wise division of two input series. Computes the quotient of
    * corresponding values from two real inputs.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = inReal0[i] / inReal1[i]
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#DIV_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 Dividend (numerator) series.
    * @param inReal1 Divisor (denominator) series.
    * @param outReal Per-element quotient inReal0/inReal1. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#MULT
    * @see Core#ADD
    * @see Core#SUB
    */
   public OutRange DIV( int startIdx,
                        int endIdx,
                        float inReal0[],
                        float inReal1[],
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = DIV_Internal(startIdx, endIdx, inReal0, inReal1, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("DIV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live DIV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#DIV} over the same series.
    * Open with {@link Core#DIV_Open}; there is no close — the handle is
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
   public static final class DIV_Stream {
      final Core core;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      DIV_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#DIV_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      DIV_Stream( DIV_Stream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal0, double inReal1 ) {
         core.DIV_StreamStep(this, inReal0, inReal1);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal0, double inReal1 ) {
         DIV_Stream scratch = new DIV_Stream(this);
         core.DIV_StreamStep(scratch, inReal0, inReal1);
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
      public DIV_Stream copy() {
         return new DIV_Stream(this);
      }
   }
   void DIV_StreamStep( DIV_Stream sp, double inReal0, double inReal1 )
   {
      sp.cur_outReal = inReal0 / inReal1;
   }
   private RetCode DIV_OpenBody( DIV_Stream sp, double inReal0[], double inReal1[], int startIdx )
   {
      int outIdx = 0;
      int i = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         lastValue_outReal = inReal0[i] / inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode DIV_OpenAndFillBody( DIV_Stream sp, double inReal0[], double inReal1[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 ) {
         return RetCode.BadParam;
      }
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = inReal0[i] / inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind DIV_Open (composition seam). */
   DIV_Stream DIV_OpenInternal( double inReal0[], double inReal1[], int startIdx )
   {
      DIV_Stream sp = new DIV_Stream(this);
      RetCode retCode = DIV_OpenBody(sp, inReal0, inReal1, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("DIV open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("DIV open: internal error");
      }
      throw new IllegalArgumentException("DIV open: " + retCode);
   }
   /**
    * Open a live DIV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#DIV} at that bar.
    * <p>The history must hold at least {@code DIV_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public DIV_Stream DIV_Open( double inReal0[], double inReal1[] )
   {
      return DIV_OpenInternal(inReal0, inReal1, 0);
   }
   /**
    * {@link Core#DIV_Open} that also fills the output array(s) bit-identically
    * to {@link Core#DIV} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link DIV_Stream#fillRange()}.
    */
   public DIV_Stream DIV_OpenAndFill( double inReal0[], double inReal1[], double outReal[] )
   {
      DIV_Stream sp = new DIV_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = DIV_OpenAndFillBody(sp, inReal0, inReal1, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("DIV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("DIV openAndFill: internal error");
      }
      throw new IllegalArgumentException("DIV openAndFill: " + retCode);
   }
