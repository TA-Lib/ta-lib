/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  112605 MF    Initial coding.
 *  082326 MF,CC Fix #253. Test the bar range exactly instead of against the
 *               fixed TA_IS_ZERO_OR_NEG band, which zeroed the output for any
 *               instrument quoted small enough to fall under it.
 */

   /**
    * Number of leading input bars {@link Core#BOP} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int BOP_Lookback( )
   {
      return 0 ;

   }
   RetCode BOP_Impl( int startIdx,
                     int endIdx,
                     double inOpen[],
                     double inHigh[],
                     double inLow[],
                     double inClose[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* BOP = (Close - Open)/(High - Low) */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         /* BOP is a fraction of the bar's own range, so it is scale-free and the
          * divisor only has to be positive. An exact test, not the fixed
          * TA_IS_ZERO_OR_NEG band it used to be: the range carries the quote unit,
          * and that band zeroed the output for any instrument quoted below it
          * (issue #253).
          */
         tempReal = inHigh[i] - inLow[i];
         if( tempReal <= 0.0 ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = (inClose[i] - inOpen[i]) / tempReal;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode BOP_Impl( int startIdx,
                     int endIdx,
                     float inOpen[],
                     float inHigh[],
                     float inLow[],
                     float inClose[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = (double)inHigh[i] - (double)inLow[i];
         if( tempReal <= 0.0 ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = ((double)inClose[i] - (double)inOpen[i]) / tempReal;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Balance Of Power compares where the close sits relative to the open,
    * normalized by the bar's high-low range. A per-bar oscillator with no
    * smoothing. Positive: close above open (buyers dominated); negative:
    * sellers dominated.
    * <p><b>Formula</b>
    * <pre>{@code
    * BOP = (Close - Open) / (High - Low)
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#BOP_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outReal Balance of Power value per bar. Must hold at least
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
    */
   public OutRange BOP( int startIdx,
                        int endIdx,
                        double inOpen[],
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        double outReal[] )
   {
      requireIndexRange("BOP", startIdx, endIdx);
      int guardStart = clampedStart("BOP", startIdx, BOP_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("BOP", "inOpen", inOpen, guardInLen);
      requireLength("BOP", "inHigh", inHigh, guardInLen);
      requireLength("BOP", "inLow", inLow, guardInLen);
      requireLength("BOP", "inClose", inClose, guardInLen);
      requireLength("BOP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = BOP_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("BOP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Balance Of Power compares where the close sits relative to the open,
    * normalized by the bar's high-low range. A per-bar oscillator with no
    * smoothing. Positive: close above open (buyers dominated); negative:
    * sellers dominated.
    * <p><b>Formula</b>
    * <pre>{@code
    * BOP = (Close - Open) / (High - Low)
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#BOP_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outReal Balance of Power value per bar. Must hold at least
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
    */
   public OutRange BOP( int startIdx,
                        int endIdx,
                        float inOpen[],
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        double outReal[] )
   {
      requireIndexRange("BOP", startIdx, endIdx);
      int guardStart = clampedStart("BOP", startIdx, BOP_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("BOP", "inOpen", inOpen, guardInLen);
      requireLength("BOP", "inHigh", inHigh, guardInLen);
      requireLength("BOP", "inLow", inLow, guardInLen);
      requireLength("BOP", "inClose", inClose, guardInLen);
      requireLength("BOP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = BOP_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("BOP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live BOP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#BOP} over the same series.
    * Open with {@link Core#bopOpen}; there is no close — the handle is
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
   public static final class BopStream {
      Core core;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      BopStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#BOP} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      BopStream( BopStream other ) {
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
      public double update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("BOP update: BadParam", RetCode.BadParam);
         }
         core.bopStepImpl(this, inOpen, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inOpen.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], double outReal[] ) {
         requireArgument("BOP updateAndFill", "inOpen", inOpen);
         requireArgument("BOP updateAndFill", "inHigh", inHigh);
         requireArgument("BOP updateAndFill", "inLow", inLow);
         requireArgument("BOP updateAndFill", "inClose", inClose);
         requireArgument("BOP updateAndFill", "outReal", outReal);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inOpen || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("BOP updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("BOP updateAndFill: BadParam", RetCode.BadParam);
            }
            core.bopStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
      public double peek( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("BOP peek: BadParam", RetCode.BadParam);
         BopStream sp = this;
         double tempReal = 0.0;
         double cur_outReal = 0.0;
         /* BOP is a fraction of the bar's own range, so it is scale-free and the
          * divisor only has to be positive. An exact test, not the fixed
          * TA_IS_ZERO_OR_NEG band it used to be: the range carries the quote unit,
          * and that band zeroed the output for any instrument quoted below it
          * (issue #253).
          */
         tempReal = inHigh - inLow;
         if( tempReal <= 0.0 ) {
            cur_outReal = 0.0;
         } else {
            cur_outReal = (inClose - inOpen) / tempReal;
         }
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
      public BopStream clone() {
         return new BopStream(this);
      }
   }
   void bopStepImpl( BopStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      double tempReal = 0.0;
      /* BOP is a fraction of the bar's own range, so it is scale-free and the
       * divisor only has to be positive. An exact test, not the fixed
       * TA_IS_ZERO_OR_NEG band it used to be: the range carries the quote unit,
       * and that band zeroed the output for any instrument quoted below it
       * (issue #253).
       */
      tempReal = inHigh - inLow;
      if( tempReal <= 0.0 ) {
         sp.cur_outReal = 0.0;
      } else {
         sp.cur_outReal = (inClose - inOpen) / tempReal;
      }
   }
   private RetCode bopOpenImpl( BopStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int i = 0;
      double tempReal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* BOP = (Close - Open)/(High - Low) */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         /* BOP is a fraction of the bar's own range, so it is scale-free and the
          * divisor only has to be positive. An exact test, not the fixed
          * TA_IS_ZERO_OR_NEG band it used to be: the range carries the quote unit,
          * and that band zeroed the output for any instrument quoted below it
          * (issue #253).
          */
         tempReal = inHigh[i] - inLow[i];
         if( tempReal <= 0.0 ) {
            outReal[outIdx++ * outStride] = 0.0;
         } else {
            outReal[outIdx++ * outStride] = (inClose[i] - inOpen[i]) / tempReal;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* bopOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   BopStream bopOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      BopStream sp = new BopStream(this);
      RetCode retCode = bopOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("BOP openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("BOP openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("BOP openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind bopOpen (composition seam). */
   BopStream bopOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      BopStream sp = new BopStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = bopOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("BOP open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("BOP open: internal error", retCode);
      }
      throw new TaLibArgumentException("BOP open: " + retCode, retCode);
   }
   /**
    * Open a live BOP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#BOP} at that bar.
    * <p>The history must hold at least {@code BOP_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public BopStream bopOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("BOP open", "inOpen", inOpen);
      requireHistory("BOP open", inOpen.length);
      requireArgument("BOP open", "inHigh", inHigh);
      requireArgument("BOP open", "inLow", inLow);
      requireArgument("BOP open", "inClose", inClose);
      requireHistoryLength("BOP open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("BOP open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("BOP open", "inClose", inClose.length, inOpen.length);
      return bopOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#bopOpen} that also fills the output array(s) bit-identically
    * to {@link Core#BOP} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link BopStream#outRange()}.
    */
   public BopStream bopOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], double outReal[] )
   {
      requireArgument("BOP openAndFill", "inOpen", inOpen);
      requireHistory("BOP openAndFill", inOpen.length);
      requireArgument("BOP openAndFill", "inHigh", inHigh);
      requireArgument("BOP openAndFill", "inLow", inLow);
      requireArgument("BOP openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("BOP openAndFill", inOpen.length, BOP_Lookback());
      requireHistoryLength("BOP openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("BOP openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("BOP openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("BOP openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inOpen || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("BOP openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return bopOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outReal);
   }
