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
   RetCode TRANGE_Impl( int startIdx,
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
   RetCode TRANGE_Impl( int startIdx,
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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      requireIndexRange("TRANGE", startIdx, endIdx);
      int guardStart = clampedStart("TRANGE", startIdx, TRANGE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("TRANGE", "inHigh", inHigh, guardInLen);
      requireLength("TRANGE", "inLow", inLow, guardInLen);
      requireLength("TRANGE", "inClose", inClose, guardInLen);
      requireLength("TRANGE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TRANGE_Impl(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      requireIndexRange("TRANGE", startIdx, endIdx);
      int guardStart = clampedStart("TRANGE", startIdx, TRANGE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("TRANGE", "inHigh", inHigh, guardInLen);
      requireLength("TRANGE", "inLow", inLow, guardInLen);
      requireLength("TRANGE", "inClose", inClose, guardInLen);
      requireLength("TRANGE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TRANGE_Impl(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
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
      Core core;
      double lag1_inClose;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      TRANGE_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#TRANGE} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      TRANGE_Stream( TRANGE_Stream other ) {
         this.core = other.core;
         this.lag1_inClose = other.lag1_inClose;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( TRANGE_Stream other ) {
         this.core = other.core;
         this.lag1_inClose = other.lag1_inClose;
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
      public double update( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("TRANGE update: BadParam", RetCode.BadParam);
         core.TRANGE_StepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] ) {
         requireArgument("TRANGE updateAndFill", "inHigh", inHigh);
         requireArgument("TRANGE updateAndFill", "inLow", inLow);
         requireArgument("TRANGE updateAndFill", "inClose", inClose);
         requireArgument("TRANGE updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("TRANGE updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("TRANGE updateAndFill: BadParam", RetCode.BadParam);
            core.TRANGE_StepImpl(this, inHigh[i], inLow[i], inClose[i]);
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
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("TRANGE peek: BadParam", RetCode.BadParam);
         TRANGE_Stream scratch = new TRANGE_Stream(this);
         core.TRANGE_StepImpl(scratch, inHigh, inLow, inClose);
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
   void TRANGE_StepImpl( TRANGE_Stream sp, double inHigh, double inLow, double inClose )
   {
      double val2 = 0.0;
      double val3 = 0.0;
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
      val3 = Math.abs(tempCY - tempLT);
      if( val3 > greatest ) {
         greatest = val3;
      }
      sp.cur_outReal = greatest;
      sp.lag1_inClose = inClose;
   }
   private RetCode TRANGE_OpenImpl( TRANGE_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
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
         return RetCode.InsufficientHistory ;
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
         outReal[outIdx++ * outStride] = greatest;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* TRANGE_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   TRANGE_Stream TRANGE_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      TRANGE_Stream sp = new TRANGE_Stream(this);
      RetCode retCode = TRANGE_OpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("TRANGE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("TRANGE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("TRANGE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind TRANGE_Open (composition seam). */
   TRANGE_Stream TRANGE_OpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      TRANGE_Stream sp = new TRANGE_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = TRANGE_OpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("TRANGE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("TRANGE open: internal error", retCode);
      }
      throw new TaLibArgumentException("TRANGE open: " + retCode, retCode);
   }
   /**
    * Open a live TRANGE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#TRANGE} at that bar.
    * <p>The history must hold at least {@code TRANGE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public TRANGE_Stream TRANGE_Open( double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("TRANGE open", "inHigh", inHigh);
      requireHistory("TRANGE open", inHigh.length);
      requireArgument("TRANGE open", "inLow", inLow);
      requireArgument("TRANGE open", "inClose", inClose);
      requireHistoryLength("TRANGE open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("TRANGE open", "inClose", inClose.length, inHigh.length);
      return TRANGE_OpenInternal(inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#TRANGE_Open} that also fills the output array(s) bit-identically
    * to {@link Core#TRANGE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link TRANGE_Stream#outRange()}.
    */
   public TRANGE_Stream TRANGE_OpenAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] )
   {
      requireArgument("TRANGE openAndFill", "inHigh", inHigh);
      requireHistory("TRANGE openAndFill", inHigh.length);
      requireArgument("TRANGE openAndFill", "inLow", inLow);
      requireArgument("TRANGE openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("TRANGE openAndFill", inHigh.length, TRANGE_Lookback());
      requireHistoryLength("TRANGE openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("TRANGE openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("TRANGE openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("TRANGE openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return TRANGE_OpenAndFillInternal(inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outReal);
   }
