/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092926 KL,CC  Creation (#468).
 *  092926 MF,CC  Store the quotient, then overwrite it, so the loop vectorizes.
 */

   /**
    * Number of leading input bars {@link Core#ibs} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ibsLookback( )
   {
      /* One bar in, one bar out: the value reads only its own bar, so there is
       * nothing to warm up. bop.c:19-22's lookback, and stochf_lookback(1,1,SMA)
       * agrees, since ma_lookback returns 0 for a period of 1 (ma.c:28-29).
       */
      return 0 ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#ibs}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int ibsDisplayShift( int outputIdx )
   {
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode ibsImpl( int startIdx,
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
      double range = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      /* IBS = (Close - Low)/(High - Low): where the close sits inside its own
       * bar, 0 at the low and 1 at the high.
       */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         /* An exact test: the range of one bar carries the quote unit, so any
          * band would flip every bar of a small enough instrument to 0.5. Spelled
          * <= so a NaN input keeps the quotient's NaN.
          *
          * Store, then overwrite: gcc keeps a guarded or selected quotient
          * scalar under -ftrapping-math.
          */
         range = inHigh[i] - inLow[i];
         outReal[outIdx] = (inClose[i] - inLow[i]) / range;
         if( range <= 0.0 ) {
            outReal[outIdx] = 0.5;
         }
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode ibsImpl( int startIdx,
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
      double range = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         range = (double)inHigh[i] - (double)inLow[i];
         outReal[outIdx] = ((double)inClose[i] - (double)inLow[i]) / range;
         if( range <= 0.0 ) {
            outReal[outIdx] = 0.5;
         }
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Internal Bar Strength: where the close sits inside its own bar's range. It
    * is 0 when the bar closes at its low, 1 at its high and 0.5 at the
    * midpoint, and it reads one bar only, with no window and no state. It is
    * the closing half of a stochastic oscillator taken over a single bar, which
    * is how the paper that named the effect describes it. Low readings say the
    * session ended on weakness, high readings on strength, and the published
    * use is mean reversion on the next bar rather than trend confirmation.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/ibs">ta-lib.org/functions/ibs</a>.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#ibsLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outReal Position of the close within the bar's range, 0 to 1 on a
    *        bar with range. Must hold at least
    *        {@code endIdx - max(startIdx, ibsLookback(...)) + 1} values, the count the
    *        call produces (none when that is not positive).
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
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
   public OutRange ibs( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        double outReal[] )
   {
      requireIndexRange("IBS", startIdx, endIdx);
      int guardStart = clampedStart("IBS", startIdx, ibsLookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("IBS", "inHigh", inHigh, guardInLen);
      requireLength("IBS", "inLow", inLow, guardInLen);
      requireLength("IBS", "inClose", inClose, guardInLen);
      requireLength("IBS", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ibsImpl(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("IBS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Internal Bar Strength: where the close sits inside its own bar's range. It
    * is 0 when the bar closes at its low, 1 at its high and 0.5 at the
    * midpoint, and it reads one bar only, with no window and no state. It is
    * the closing half of a stochastic oscillator taken over a single bar, which
    * is how the paper that named the effect describes it. Low readings say the
    * session ended on weakness, high readings on strength, and the published
    * use is mean reversion on the next bar rather than trend confirmation.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/ibs">ta-lib.org/functions/ibs</a>.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#ibsLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outReal Position of the close within the bar's range, 0 to 1 on a
    *        bar with range. Must hold at least
    *        {@code endIdx - max(startIdx, ibsLookback(...)) + 1} values, the count the
    *        call produces (none when that is not positive).
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
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
   public OutRange ibs( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        double outReal[] )
   {
      requireIndexRange("IBS", startIdx, endIdx);
      int guardStart = clampedStart("IBS", startIdx, ibsLookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("IBS", "inHigh", inHigh, guardInLen);
      requireLength("IBS", "inLow", inLow, guardInLen);
      requireLength("IBS", "inClose", inClose, guardInLen);
      requireLength("IBS", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ibsImpl(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("IBS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live IBS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ibs} over the same series.
    * Open with {@link Core#ibsOpen}; there is no close — the handle is
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
   public static final class IbsStream {
      private Core core;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private IbsStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ibs} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("IBS advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private IbsStream( IbsStream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inHigh, double inLow, double inClose ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("IBS update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("IBS update", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.ibsStepImpl(this, inHigh, inLow, inClose);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("IBS peek", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         IbsStream sp = this;
         double range = 0.0;
         double cur_outReal = 0.0;
         /* An exact test: the range of one bar carries the quote unit, so any
          * band would flip every bar of a small enough instrument to 0.5. Spelled
          * <= so a NaN input keeps the quotient's NaN.
          *
          * Store, then overwrite: gcc keeps a guarded or selected quotient
          * scalar under -ftrapping-math.
          */
         range = inHigh - inLow;
         cur_outReal = (inClose - inLow) / range;
         if( range <= 0.0 ) {
            cur_outReal = 0.5;
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
      public IbsStream clone() {
         return new IbsStream(this);
      }
   }
   private void ibsStepImpl( IbsStream sp, double inHigh, double inLow, double inClose )
   {
      double range = 0.0;
      /* An exact test: the range of one bar carries the quote unit, so any
       * band would flip every bar of a small enough instrument to 0.5. Spelled
       * <= so a NaN input keeps the quotient's NaN.
       *
       * Store, then overwrite: gcc keeps a guarded or selected quotient
       * scalar under -ftrapping-math.
       */
      range = inHigh - inLow;
      sp.cur_outReal = (inClose - inLow) / range;
      if( range <= 0.0 ) {
         sp.cur_outReal = 0.5;
      }
   }
   private RetCode ibsOpenImpl( IbsStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int i = 0;
      double range = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* IBS = (Close - Low)/(High - Low): where the close sits inside its own
       * bar, 0 at the low and 1 at the high.
       */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         /* An exact test: the range of one bar carries the quote unit, so any
          * band would flip every bar of a small enough instrument to 0.5. Spelled
          * <= so a NaN input keeps the quotient's NaN.
          *
          * Store, then overwrite: gcc keeps a guarded or selected quotient
          * scalar under -ftrapping-math.
          */
         range = inHigh[i] - inLow[i];
         outReal[outIdx * outStride] = (inClose[i] - inLow[i]) / range;
         if( range <= 0.0 ) {
            outReal[outIdx * outStride] = 0.5;
         }
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* ibsOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   IbsStream ibsOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      IbsStream sp = new IbsStream(this);
      RetCode retCode = ibsOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("IBS openAndFill", inHigh.length, startIdx, ibsLookback());
      }
      throw streamFailure("IBS openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind ibsOpen (composition seam). */
   IbsStream ibsOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      IbsStream sp = new IbsStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = ibsOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("IBS open", inHigh.length, startIdx, ibsLookback());
      }
      throw streamFailure("IBS open", retCode);
   }
   /**
    * Open a live IBS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ibs} at that bar.
    * <p>The history must hold at least {@code ibsLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public IbsStream ibsOpen( double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("IBS open", "inHigh", inHigh);
      requireHistory("IBS open", inHigh.length);
      requireArgument("IBS open", "inLow", inLow);
      requireArgument("IBS open", "inClose", inClose);
      requireHistoryLength("IBS open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("IBS open", "inClose", inClose.length, inHigh.length);
      return ibsOpenInternal(inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#ibsOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ibs} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link IbsStream#outRange()}.
    */
   public IbsStream ibsOpenAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] )
   {
      requireArgument("IBS openAndFill", "inHigh", inHigh);
      requireHistory("IBS openAndFill", inHigh.length);
      requireArgument("IBS openAndFill", "inLow", inLow);
      requireArgument("IBS openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("IBS openAndFill", inHigh.length, ibsLookback());
      requireHistoryLength("IBS openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("IBS openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("IBS openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw streamFailure("IBS openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ibsOpenAndFillInternal(inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outReal);
   }
