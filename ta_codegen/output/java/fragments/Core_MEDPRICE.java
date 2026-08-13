/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  112605 MF   Fix outBegIdx when startIdx != 0
 */

   /**
    * Number of leading input bars {@link Core#MEDPRICE} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MEDPRICE_Lookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   RetCode MEDPRICE_Internal( int startIdx,
                              int endIdx,
                              double inHigh[],
                              double inLow[],
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
      /* MEDPRICE = (High + Low ) / 2
       * This is the high and low of the same price bar.
       *
       * See MIDPRICE to use instead the highest high and lowest
       * low over multiple price bar.
       */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = (inHigh[i] + inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode MEDPRICE_Internal( int startIdx,
                              int endIdx,
                              float inHigh[],
                              float inLow[],
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
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Median Price: the midpoint of each bar's high and low. A price-transform
    * overlay.
    * <p><b>Formula</b>
    * <pre>{@code
    * $MEDPRICE_i = (High_i + Low_i) / 2$
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MEDPRICE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param outReal Midpoint of each bar's high and low. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#MIDPRICE
    * @see Core#AVGPRICE
    * @see Core#TYPPRICE
    * @see Core#WCLPRICE
    */
   public OutRange MEDPRICE( int startIdx,
                             int endIdx,
                             double inHigh[],
                             double inLow[],
                             double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MEDPRICE_Internal(startIdx, endIdx, inHigh, inLow, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MEDPRICE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Median Price: the midpoint of each bar's high and low. A price-transform
    * overlay.
    * <p><b>Formula</b>
    * <pre>{@code
    * $MEDPRICE_i = (High_i + Low_i) / 2$
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MEDPRICE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param outReal Midpoint of each bar's high and low. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#MIDPRICE
    * @see Core#AVGPRICE
    * @see Core#TYPPRICE
    * @see Core#WCLPRICE
    */
   public OutRange MEDPRICE( int startIdx,
                             int endIdx,
                             float inHigh[],
                             float inLow[],
                             double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MEDPRICE_Internal(startIdx, endIdx, inHigh, inLow, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MEDPRICE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MEDPRICE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MEDPRICE} over the same series.
    * Open with {@link Core#MEDPRICE_Open}; there is no close — the handle is
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
   public static final class MEDPRICE_Stream {
      Core core;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      MEDPRICE_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#MEDPRICE_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      MEDPRICE_Stream( MEDPRICE_Stream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( MEDPRICE_Stream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow ) {
         core.MEDPRICE_StreamStep(this, inHigh, inLow);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inHigh, double inLow ) {
         MEDPRICE_Stream scratch = new MEDPRICE_Stream(this);
         core.MEDPRICE_StreamStep(scratch, inHigh, inLow);
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
      public MEDPRICE_Stream copy() {
         return new MEDPRICE_Stream(this);
      }
   }
   void MEDPRICE_StreamStep( MEDPRICE_Stream sp, double inHigh, double inLow )
   {
      sp.cur_outReal = (inHigh + inLow) / 2.0;
   }
   private RetCode MEDPRICE_OpenCore( MEDPRICE_Stream sp, double inHigh[], double inLow[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int i = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      /* MEDPRICE = (High + Low ) / 2
       * This is the high and low of the same price bar.
       *
       * See MIDPRICE to use instead the highest high and lowest
       * low over multiple price bar.
       */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++ * outStride] = (inHigh[i] + inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode MEDPRICE_OpenBody( MEDPRICE_Stream sp, double inHigh[], double inLow[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return MEDPRICE_OpenCore( sp, inHigh, inLow, startIdx, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode MEDPRICE_OpenAndFillBody( MEDPRICE_Stream sp, double inHigh[], double inLow[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         return RetCode.BadParam;
      }
      return MEDPRICE_OpenCore( sp, inHigh, inLow, 0, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode MEDPRICE_OpenAndFillInternalBody( MEDPRICE_Stream sp, double inHigh[], double inLow[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return MEDPRICE_OpenCore(sp, inHigh, inLow, startIdx, outBegIdx, outNBElement, outReal, 1);
   }
   /* MEDPRICE_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MEDPRICE_Stream MEDPRICE_OpenAndFillInternal( double inHigh[], double inLow[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MEDPRICE_Stream sp = new MEDPRICE_Stream(this);
      RetCode retCode = MEDPRICE_OpenAndFillInternalBody(sp, inHigh, inLow, startIdx, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MEDPRICE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MEDPRICE openAndFill: internal error");
      }
      throw new IllegalArgumentException("MEDPRICE openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind MEDPRICE_Open (composition seam). */
   MEDPRICE_Stream MEDPRICE_OpenInternal( double inHigh[], double inLow[], int startIdx )
   {
      MEDPRICE_Stream sp = new MEDPRICE_Stream(this);
      RetCode retCode = MEDPRICE_OpenBody(sp, inHigh, inLow, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MEDPRICE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MEDPRICE open: internal error");
      }
      throw new IllegalArgumentException("MEDPRICE open: " + retCode);
   }
   /**
    * Open a live MEDPRICE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MEDPRICE} at that bar.
    * <p>The history must hold at least {@code MEDPRICE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MEDPRICE_Stream MEDPRICE_Open( double inHigh[], double inLow[] )
   {
      return MEDPRICE_OpenInternal(inHigh, inLow, 0);
   }
   /**
    * {@link Core#MEDPRICE_Open} that also fills the output array(s) bit-identically
    * to {@link Core#MEDPRICE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MEDPRICE_Stream#fillRange()}.
    */
   public MEDPRICE_Stream MEDPRICE_OpenAndFill( double inHigh[], double inLow[], double outReal[] )
   {
      MEDPRICE_Stream sp = new MEDPRICE_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MEDPRICE_OpenAndFillBody(sp, inHigh, inLow, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MEDPRICE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MEDPRICE openAndFill: internal error");
      }
      throw new IllegalArgumentException("MEDPRICE openAndFill: " + retCode);
   }
