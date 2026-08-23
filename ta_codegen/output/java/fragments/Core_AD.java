/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JD       jdoyle
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  111705 MF,JD  Fix#1359452 for handling properly start/end range.
 */

   /**
    * Number of leading input bars {@link Core#AD} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int AD_Lookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   RetCode AD_Impl( int startIdx,
                    int endIdx,
                    double inHigh[],
                    double inLow[],
                    double inClose[],
                    double inVolume[],
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Note: Results from this function might vary slightly
       *       from Metastock outputs. The reason being that
       *       Metastock use float instead of double and this
       *       cause a different floating-point precision to
       *       be used.
       *
       *       For most function, this is not an apparent difference
       *       but for function using large cummulative values (like
       *       this AD function), minor imprecision adds up and becomes
       *       significative.
       *
       *       For better precision, TA-Lib use double in all its
       *       its calculations.
       */
      /* Default return values */
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = inHigh[currentBar];
         low = inLow[currentBar];
         tmp = high - low;
         close = inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      return RetCode.Success ;
   }
   RetCode AD_Impl( int startIdx,
                    int endIdx,
                    float inHigh[],
                    float inLow[],
                    float inClose[],
                    float inVolume[],
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = (double)inHigh[currentBar];
         low = (double)inLow[currentBar];
         tmp = high - low;
         close = (double)inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      return RetCode.Success ;
   }
   /**
    * Chaikin Accumulation/Distribution Line, a cumulative volume-flow
    * indicator. Sums a volume-weighted money-flow multiplier per bar to gauge
    * buying vs. selling pressure. Rising line = accumulation (buying pressure);
    * falling = distribution.
    * <p><b>Formula</b>
    * <pre>{@code
    * MFM = ((close-low) - (high-close)) / (high-low); AD_t = AD_{t-1} + MFM_t * volume_t (running sum, seeded at 0)
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AD_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Cumulative A/D line value per bar. Must hold at least
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
    * @see Core#ADOSC
    * @see Core#OBV
    */
   public OutRange AD( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       double inVolume[],
                       double outReal[] )
   {
      requireIndexRange("AD", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, AD_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AD", "inHigh", inHigh, guardInLen);
      requireLength("AD", "inLow", inLow, guardInLen);
      requireLength("AD", "inClose", inClose, guardInLen);
      requireLength("AD", "inVolume", inVolume, guardInLen);
      requireLength("AD", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AD_Impl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Chaikin Accumulation/Distribution Line, a cumulative volume-flow
    * indicator. Sums a volume-weighted money-flow multiplier per bar to gauge
    * buying vs. selling pressure. Rising line = accumulation (buying pressure);
    * falling = distribution.
    * <p><b>Formula</b>
    * <pre>{@code
    * MFM = ((close-low) - (high-close)) / (high-low); AD_t = AD_{t-1} + MFM_t * volume_t (running sum, seeded at 0)
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AD_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Cumulative A/D line value per bar. Must hold at least
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
    * @see Core#ADOSC
    * @see Core#OBV
    */
   public OutRange AD( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       float inVolume[],
                       double outReal[] )
   {
      requireIndexRange("AD", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, AD_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AD", "inHigh", inHigh, guardInLen);
      requireLength("AD", "inLow", inLow, guardInLen);
      requireLength("AD", "inClose", inClose, guardInLen);
      requireLength("AD", "inVolume", inVolume, guardInLen);
      requireLength("AD", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AD_Impl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live AD stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#AD} over the same series.
    * Open with {@link Core#AD_Open}; there is no close — the handle is
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
   public static final class AD_Stream {
      Core core;
      double ad;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      AD_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#AD} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      AD_Stream( AD_Stream other ) {
         this.core = other.core;
         this.ad = other.ad;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( AD_Stream other ) {
         this.core = other.core;
         this.ad = other.ad;
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
      public double update( double inHigh, double inLow, double inClose, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("AD update: BadParam", RetCode.BadParam);
         core.AD_StepImpl(this, inHigh, inLow, inClose, inVolume);
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
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], double outReal[] ) {
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || inVolume.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("AD updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) || !Double.isFinite(inVolume[i]) )
               throw new TaLibArgumentException("AD updateAndFill: BadParam", RetCode.BadParam);
            core.AD_StepImpl(this, inHigh[i], inLow[i], inClose[i], inVolume[i]);
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
      public double peek( double inHigh, double inLow, double inClose, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("AD peek: BadParam", RetCode.BadParam);
         AD_Stream scratch = new AD_Stream(this);
         core.AD_StepImpl(scratch, inHigh, inLow, inClose, inVolume);
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
      public AD_Stream copy() {
         return new AD_Stream(this);
      }
   }
   void AD_StepImpl( AD_Stream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      double high = 0.0;
      double low = 0.0;
      double close = 0.0;
      double tmp = 0.0;
      high = inHigh;
      low = inLow;
      tmp = high - low;
      close = inClose;
      if( tmp > 0.0 ) {
         sp.ad += (close - low - (high - close)) / tmp * (double)inVolume;
      }
      sp.cur_outReal = sp.ad;
   }
   private RetCode AD_OpenImpl( AD_Stream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Note: Results from this function might vary slightly
       *       from Metastock outputs. The reason being that
       *       Metastock use float instead of double and this
       *       cause a different floating-point precision to
       *       be used.
       *
       *       For most function, this is not an apparent difference
       *       but for function using large cummulative values (like
       *       this AD function), minor imprecision adds up and becomes
       *       significative.
       *
       *       For better precision, TA-Lib use double in all its
       *       its calculations.
       */
      /* Default return values */
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = inHigh[currentBar];
         low = inLow[currentBar];
         tmp = high - low;
         close = inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++ * outStride] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      /* Capture the live batch state into the handle. */
      sp.ad = ad;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* AD_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AD_Stream AD_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AD_Stream sp = new AD_Stream(this);
      RetCode retCode = AD_OpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AD openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AD openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("AD openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind AD_Open (composition seam). */
   AD_Stream AD_OpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx )
   {
      AD_Stream sp = new AD_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = AD_OpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AD open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AD open: internal error", retCode);
      }
      throw new TaLibArgumentException("AD open: " + retCode, retCode);
   }
   /**
    * Open a live AD stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#AD} at that bar.
    * <p>The history must hold at least {@code AD_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public AD_Stream AD_Open( double inHigh[], double inLow[], double inClose[], double inVolume[] )
   {
      return AD_OpenInternal(inHigh, inLow, inClose, inVolume, 0);
   }
   /**
    * {@link Core#AD_Open} that also fills the output array(s) bit-identically
    * to {@link Core#AD} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link AD_Stream#outRange()}.
    */
   public AD_Stream AD_OpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], double outReal[] )
   {
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("AD openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return AD_OpenAndFillInternal(inHigh, inLow, inClose, inVolume, 0, outBegIdx, outNBElement, outReal);
   }
