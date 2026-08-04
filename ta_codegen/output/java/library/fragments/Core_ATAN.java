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
    * Number of leading input bars {@link Core#atan} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int atanLookback( )
   {
      return 0 ;

   }
   RetCode atanInternal( int startIdx,
                         int endIdx,
                         double inReal[],
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Default return values */
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.atan(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode atanInternal( int startIdx,
                         int endIdx,
                         float inReal[],
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.atan((double)inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Vector trigonometric arc tangent: applies atan element-wise to each input.
    * Pure math transform with no lookback.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = atan(inReal[i])  (radians, range (-pi/2, pi/2))
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#atanLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input values.
    * @param outReal Arc tangent of each input, in radians. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#tan
    * @see Core#acos
    * @see Core#asin
    */
   public OutRange atan( int startIdx,
                         int endIdx,
                         double inReal[],
                         double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = atanInternal(startIdx, endIdx, inReal, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ATAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Vector trigonometric arc tangent: applies atan element-wise to each input.
    * Pure math transform with no lookback.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = atan(inReal[i])  (radians, range (-pi/2, pi/2))
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#atanLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input values.
    * @param outReal Arc tangent of each input, in radians. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#tan
    * @see Core#acos
    * @see Core#asin
    */
   public OutRange atan( int startIdx,
                         int endIdx,
                         float inReal[],
                         double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = atanInternal(startIdx, endIdx, inReal, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ATAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ATAN stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#atan} over the same series.
    * Open with {@link Core#atanOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class AtanStream {
      final Core core;
      double cur_outReal;
      OutRange fillRange;

      AtanStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#atanOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      AtanStream( AtanStream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.atanStreamStep(this, inReal);
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
         AtanStream scratch = new AtanStream(this);
         core.atanStreamStep(scratch, inReal);
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
      public AtanStream copy() {
         return new AtanStream(this);
      }
   }
   void atanStreamStep( AtanStream sp, double inReal )
   {
      sp.cur_outReal = Math.atan(inReal);
   }
   private RetCode atanOpenBody( AtanStream sp, double inReal[], int startIdx )
   {
      int outIdx = 0;
      int i = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      /* Default return values */
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         lastValue_outReal = Math.atan(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode atanOpenAndFillBody( AtanStream sp, double inReal[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      /* Default return values */
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.atan(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind atanOpen (composition seam). */
   AtanStream atanOpenInternal( double inReal[], int startIdx )
   {
      AtanStream sp = new AtanStream(this);
      RetCode retCode = atanOpenBody(sp, inReal, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ATAN open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ATAN open: internal error");
      }
      throw new IllegalArgumentException("TA_ATAN open: " + retCode);
   }
   /**
    * Open a live ATAN stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#atan} at that bar.
    * <p>The history must hold at least {@code atanLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public AtanStream atanOpen( double inReal[] )
   {
      return atanOpenInternal(inReal, 0);
   }
   /**
    * {@link Core#atanOpen} that also fills the output array(s) bit-identically
    * to {@link Core#atan} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link AtanStream#fillRange()}.
    */
   public AtanStream atanOpenAndFill( double inReal[], double outReal[] )
   {
      AtanStream sp = new AtanStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = atanOpenAndFillBody(sp, inReal, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ATAN openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ATAN openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_ATAN openAndFill: " + retCode);
   }
