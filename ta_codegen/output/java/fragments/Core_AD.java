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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      int guardStart = clampedStart("AD", startIdx, AD_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      int guardStart = clampedStart("AD", startIdx, AD_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    * Open with {@link Core#adOpen}; there is no close — the handle is
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
   public static final class AdStream {
      Core core;
      double ad;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      AdStream( Core core ) { this.core = core; }

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

      AdStream( AdStream other ) {
         this.core = other.core;
         this.ad = other.ad;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( AdStream other ) {
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
         core.adStepImpl(this, inHigh, inLow, inClose, inVolume);
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
         requireArgument("AD updateAndFill", "inHigh", inHigh);
         requireArgument("AD updateAndFill", "inLow", inLow);
         requireArgument("AD updateAndFill", "inClose", inClose);
         requireArgument("AD updateAndFill", "inVolume", inVolume);
         requireArgument("AD updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || inVolume.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("AD updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) || !Double.isFinite(inVolume[i]) )
               throw new TaLibArgumentException("AD updateAndFill: BadParam", RetCode.BadParam);
            core.adStepImpl(this, inHigh[i], inLow[i], inClose[i], inVolume[i]);
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
         AdStream scratch = new AdStream(this);
         core.adStepImpl(scratch, inHigh, inLow, inClose, inVolume);
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
      public AdStream copy() {
         return new AdStream(this);
      }
   }
   void adStepImpl( AdStream sp, double inHigh, double inLow, double inClose, double inVolume )
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
   private RetCode adOpenImpl( AdStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
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
   /* adOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AdStream adOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AdStream sp = new AdStream(this);
      RetCode retCode = adOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
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
   /* Internal startIdx-anchored open behind adOpen (composition seam). */
   AdStream adOpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx )
   {
      AdStream sp = new AdStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = adOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
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
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public AdStream adOpen( double inHigh[], double inLow[], double inClose[], double inVolume[] )
   {
      requireArgument("AD open", "inHigh", inHigh);
      requireHistory("AD open", inHigh.length);
      requireArgument("AD open", "inLow", inLow);
      requireArgument("AD open", "inClose", inClose);
      requireArgument("AD open", "inVolume", inVolume);
      requireHistoryLength("AD open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("AD open", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("AD open", "inVolume", inVolume.length, inHigh.length);
      return adOpenInternal(inHigh, inLow, inClose, inVolume, 0);
   }
   /**
    * {@link Core#adOpen} that also fills the output array(s) bit-identically
    * to {@link Core#AD} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AdStream#outRange()}.
    */
   public AdStream adOpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], double outReal[] )
   {
      requireArgument("AD openAndFill", "inHigh", inHigh);
      requireHistory("AD openAndFill", inHigh.length);
      requireArgument("AD openAndFill", "inLow", inLow);
      requireArgument("AD openAndFill", "inClose", inClose);
      requireArgument("AD openAndFill", "inVolume", inVolume);
      int guardOutLen = openFillCount("AD openAndFill", inHigh.length, AD_Lookback());
      requireHistoryLength("AD openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("AD openAndFill", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("AD openAndFill", "inVolume", inVolume.length, inHigh.length);
      requireLength("AD openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("AD openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return adOpenAndFillInternal(inHigh, inLow, inClose, inVolume, 0, outBegIdx, outNBElement, outReal);
   }
