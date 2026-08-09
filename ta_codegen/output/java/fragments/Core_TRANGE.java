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
 */

   /**
    * Number of leading input bars {@link Core#TRANGE} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int TRANGE_Lookback( )
   {
      return 1 ;

   }
   RetCode TRANGE_Internal( int startIdx,
                            int endIdx,
                            double inHigh[],
                            double inLow[],
                            double inClose[],
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* True Range is the greatest of the following:
       *
       *  val1 = distance from today's high to today's low.
       *  val2 = distance from yesterday's close to today's high.
       *  val3 = distance from yesterday's close to today's low.
       *
       * Some books and software makes the first TR value to be
       * the (high - low) of the first bar. This function instead
       * ignore the first price bar, and only output starting at the
       * second price bar are valid. This is done for avoiding
       * inconsistency.
       */
      /* Move up the start index if there is not
       * enough initial data.
       * Always one price bar gets consumed.
       */
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         /* Find the greatest of the 3 values. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         outReal[outIdx++] = greatest;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode TRANGE_Internal( int startIdx,
                            int endIdx,
                            float inHigh[],
                            float inLow[],
                            float inClose[],
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         outReal[outIdx++] = greatest;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * True Range: the greatest of today's high-low span and the two gaps between
    * yesterday's close and today's high/low. Base volatility measure used to
    * build ATR/NATR. Larger values mean wider or gappier bars (higher
    * volatility).
    * <p><b>Formula</b>
    * <pre>{@code
    * TR = max( high - low, |prevClose - high|, |prevClose - low| )
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The first bar produces no value because it has no prior close; unlike some definitions, it does not fall back to the high-low range for that bar.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#TRANGE_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outReal True Range value per bar. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#ATR
    * @see Core#NATR
    */
   public OutRange TRANGE( int startIdx,
                           int endIdx,
                           double inHigh[],
                           double inLow[],
                           double inClose[],
                           double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TRANGE_Internal(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("TRANGE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * True Range: the greatest of today's high-low span and the two gaps between
    * yesterday's close and today's high/low. Base volatility measure used to
    * build ATR/NATR. Larger values mean wider or gappier bars (higher
    * volatility).
    * <p><b>Formula</b>
    * <pre>{@code
    * TR = max( high - low, |prevClose - high|, |prevClose - low| )
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The first bar produces no value because it has no prior close; unlike some definitions, it does not fall back to the high-low range for that bar.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#TRANGE_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outReal True Range value per bar. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#ATR
    * @see Core#NATR
    */
   public OutRange TRANGE( int startIdx,
                           int endIdx,
                           float inHigh[],
                           float inLow[],
                           float inClose[],
                           double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TRANGE_Internal(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("TRANGE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live TRANGE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#TRANGE} over the same series.
    * Open with {@link Core#TRANGE_Open}; there is no close — the handle is
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
   public static final class TRANGE_Stream {
      final Core core;
      double val3;
      double lag1_inClose;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      TRANGE_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#TRANGE_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      TRANGE_Stream( TRANGE_Stream other ) {
         this.core = other.core;
         this.val3 = other.val3;
         this.lag1_inClose = other.lag1_inClose;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow, double inClose ) {
         core.TRANGE_StreamStep(this, inHigh, inLow, inClose);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         TRANGE_Stream scratch = new TRANGE_Stream(this);
         core.TRANGE_StreamStep(scratch, inHigh, inLow, inClose);
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
      public TRANGE_Stream copy() {
         return new TRANGE_Stream(this);
      }
   }
   void TRANGE_StreamStep( TRANGE_Stream sp, double inHigh, double inLow, double inClose )
   {
      double val2 = 0.0;
      double greatest = 0.0;
      double tempCY = 0.0;
      double tempLT = 0.0;
      double tempHT = 0.0;
      /* Find the greatest of the 3 values. */
      tempLT = inLow;
      tempHT = inHigh;
      tempCY = sp.lag1_inClose;
      greatest = tempHT - tempLT;
      /* val1 */
      val2 = Math.abs(tempCY - tempHT);
      if( val2 > greatest ) {
         greatest = val2;
      }
      sp.val3 = Math.abs(tempCY - tempLT);
      if( sp.val3 > greatest ) {
         greatest = sp.val3;
      }
      sp.cur_outReal = greatest;
      sp.lag1_inClose = inClose;
   }
   private RetCode TRANGE_OpenBody( TRANGE_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      int today = 0;
      int outIdx = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      /* True Range is the greatest of the following:
       *
       *  val1 = distance from today's high to today's low.
       *  val2 = distance from yesterday's close to today's high.
       *  val3 = distance from yesterday's close to today's low.
       *
       * Some books and software makes the first TR value to be
       * the (high - low) of the first bar. This function instead
       * ignore the first price bar, and only output starting at the
       * second price bar are valid. This is done for avoiding
       * inconsistency.
       */
      /* Move up the start index if there is not
       * enough initial data.
       * Always one price bar gets consumed.
       */
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         /* Find the greatest of the 3 values. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         lastValue_outReal = greatest;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.val3 = val3;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode TRANGE_OpenAndFillBody( TRANGE_Stream sp, double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         return RetCode.BadParam;
      }
      /* True Range is the greatest of the following:
       *
       *  val1 = distance from today's high to today's low.
       *  val2 = distance from yesterday's close to today's high.
       *  val3 = distance from yesterday's close to today's low.
       *
       * Some books and software makes the first TR value to be
       * the (high - low) of the first bar. This function instead
       * ignore the first price bar, and only output starting at the
       * second price bar are valid. This is done for avoiding
       * inconsistency.
       */
      /* Move up the start index if there is not
       * enough initial data.
       * Always one price bar gets consumed.
       */
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         /* Find the greatest of the 3 values. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         outReal[outIdx++] = greatest;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.val3 = val3;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind TRANGE_Open (composition seam). */
   TRANGE_Stream TRANGE_OpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      TRANGE_Stream sp = new TRANGE_Stream(this);
      RetCode retCode = TRANGE_OpenBody(sp, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TRANGE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TRANGE open: internal error");
      }
      throw new IllegalArgumentException("TRANGE open: " + retCode);
   }
   /**
    * Open a live TRANGE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#TRANGE} at that bar.
    * <p>The history must hold at least {@code TRANGE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public TRANGE_Stream TRANGE_Open( double inHigh[], double inLow[], double inClose[] )
   {
      return TRANGE_OpenInternal(inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#TRANGE_Open} that also fills the output array(s) bit-identically
    * to {@link Core#TRANGE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link TRANGE_Stream#fillRange()}.
    */
   public TRANGE_Stream TRANGE_OpenAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] )
   {
      TRANGE_Stream sp = new TRANGE_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TRANGE_OpenAndFillBody(sp, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TRANGE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TRANGE openAndFill: internal error");
      }
      throw new IllegalArgumentException("TRANGE openAndFill: " + retCode);
   }
