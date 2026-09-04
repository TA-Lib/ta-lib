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
    * Number of leading input bars {@link Core#WCLPRICE} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int WCLPRICE_Lookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   RetCode WCLPRICE_Impl( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          double inClose[],
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
      /* Weighted Close Price = (High + Low + (Close*2) ) / 4 */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = (Math.fma(inClose[i], 2.0, inHigh[i] + inLow[i])) / 4.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode WCLPRICE_Impl( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          float inClose[],
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
         outReal[outIdx++] = (Math.fma((double)inClose[i], 2.0, (double)inHigh[i] + (double)inLow[i])) / 4.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Weighted Close Price: a per-bar price average giving the close double
    * weight relative to high and low.
    * <p><b>Formula</b>
    * <pre>{@code
    * $\text{WCLPRICE} = \dfrac{\text{High} + \text{Low} + 2\cdot\text{Close}}{4}$
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#WCLPRICE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outReal Weighted close price per bar. Must hold at least
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
    * @see Core#TYPPRICE
    * @see Core#MEDPRICE
    * @see Core#AVGPRICE
    */
   public OutRange WCLPRICE( int startIdx,
                             int endIdx,
                             double inHigh[],
                             double inLow[],
                             double inClose[],
                             double outReal[] )
   {
      requireIndexRange("WCLPRICE", startIdx, endIdx);
      int guardStart = clampedStart("WCLPRICE", startIdx, WCLPRICE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WCLPRICE", "inHigh", inHigh, guardInLen);
      requireLength("WCLPRICE", "inLow", inLow, guardInLen);
      requireLength("WCLPRICE", "inClose", inClose, guardInLen);
      requireLength("WCLPRICE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WCLPRICE_Impl(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("WCLPRICE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Weighted Close Price: a per-bar price average giving the close double
    * weight relative to high and low.
    * <p><b>Formula</b>
    * <pre>{@code
    * $\text{WCLPRICE} = \dfrac{\text{High} + \text{Low} + 2\cdot\text{Close}}{4}$
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#WCLPRICE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outReal Weighted close price per bar. Must hold at least
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
    * @see Core#TYPPRICE
    * @see Core#MEDPRICE
    * @see Core#AVGPRICE
    */
   public OutRange WCLPRICE( int startIdx,
                             int endIdx,
                             float inHigh[],
                             float inLow[],
                             float inClose[],
                             double outReal[] )
   {
      requireIndexRange("WCLPRICE", startIdx, endIdx);
      int guardStart = clampedStart("WCLPRICE", startIdx, WCLPRICE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WCLPRICE", "inHigh", inHigh, guardInLen);
      requireLength("WCLPRICE", "inLow", inLow, guardInLen);
      requireLength("WCLPRICE", "inClose", inClose, guardInLen);
      requireLength("WCLPRICE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WCLPRICE_Impl(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("WCLPRICE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live WCLPRICE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#WCLPRICE} over the same series.
    * Open with {@link Core#wclpriceOpen}; there is no close — the handle is
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
   public static final class WclpriceStream {
      Core core;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      WclpriceStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#WCLPRICE} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      WclpriceStream( WclpriceStream other ) {
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
      public double update( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("WCLPRICE update: BadParam", RetCode.BadParam);
         }
         core.wclpriceStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] ) {
         requireArgument("WCLPRICE updateAndFill", "inHigh", inHigh);
         requireArgument("WCLPRICE updateAndFill", "inLow", inLow);
         requireArgument("WCLPRICE updateAndFill", "inClose", inClose);
         requireArgument("WCLPRICE updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("WCLPRICE updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("WCLPRICE updateAndFill: BadParam", RetCode.BadParam);
            }
            core.wclpriceStepImpl(this, inHigh[i], inLow[i], inClose[i]);
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
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("WCLPRICE peek: BadParam", RetCode.BadParam);
         WclpriceStream sp = this;
         double cur_outReal = 0.0;
         cur_outReal = (Math.fma(inClose, 2.0, inHigh + inLow)) / 4.0;
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
      public WclpriceStream clone() {
         return new WclpriceStream(this);
      }
   }
   void wclpriceStepImpl( WclpriceStream sp, double inHigh, double inLow, double inClose )
   {
      sp.cur_outReal = (Math.fma(inClose, 2.0, inHigh + inLow)) / 4.0;
   }
   private RetCode wclpriceOpenImpl( WclpriceStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int i = 0;
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
      /* Weighted Close Price = (High + Low + (Close*2) ) / 4 */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++ * outStride] = (Math.fma(inClose[i], 2.0, inHigh[i] + inLow[i])) / 4.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* wclpriceOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   WclpriceStream wclpriceOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      WclpriceStream sp = new WclpriceStream(this);
      RetCode retCode = wclpriceOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WCLPRICE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WCLPRICE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("WCLPRICE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind wclpriceOpen (composition seam). */
   WclpriceStream wclpriceOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      WclpriceStream sp = new WclpriceStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = wclpriceOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WCLPRICE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WCLPRICE open: internal error", retCode);
      }
      throw new TaLibArgumentException("WCLPRICE open: " + retCode, retCode);
   }
   /**
    * Open a live WCLPRICE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#WCLPRICE} at that bar.
    * <p>The history must hold at least {@code WCLPRICE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public WclpriceStream wclpriceOpen( double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("WCLPRICE open", "inHigh", inHigh);
      requireHistory("WCLPRICE open", inHigh.length);
      requireArgument("WCLPRICE open", "inLow", inLow);
      requireArgument("WCLPRICE open", "inClose", inClose);
      requireHistoryLength("WCLPRICE open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("WCLPRICE open", "inClose", inClose.length, inHigh.length);
      return wclpriceOpenInternal(inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#wclpriceOpen} that also fills the output array(s) bit-identically
    * to {@link Core#WCLPRICE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link WclpriceStream#outRange()}.
    */
   public WclpriceStream wclpriceOpenAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] )
   {
      requireArgument("WCLPRICE openAndFill", "inHigh", inHigh);
      requireHistory("WCLPRICE openAndFill", inHigh.length);
      requireArgument("WCLPRICE openAndFill", "inLow", inLow);
      requireArgument("WCLPRICE openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("WCLPRICE openAndFill", inHigh.length, WCLPRICE_Lookback());
      requireHistoryLength("WCLPRICE openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("WCLPRICE openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("WCLPRICE openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("WCLPRICE openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return wclpriceOpenAndFillInternal(inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outReal);
   }
