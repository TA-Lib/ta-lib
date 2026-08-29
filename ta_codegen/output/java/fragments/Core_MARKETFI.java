/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   Initial version.
 */

   /**
    * Number of leading input bars {@link Core#MARKETFI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MARKETFI_Lookback( )
   {
      /* Each output depends only on its own bar, so nothing is consumed
       * before the first one can be produced.
       */
      return 0 ;

   }
   RetCode MARKETFI_Impl( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          double inVolume[],
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
      /* Bill Williams' Market Facilitation Index: the price range a bar
       * travelled per unit of volume traded, i.e. how much movement the
       * market "facilitated" per tick.
       *
       *      MARKETFI = ( High - Low ) / Volume
       *
       * Stateless and per-bar: no seeding, no smoothing, no accumulator and
       * no unstable period, so the output for a bar never depends on where
       * the caller started the range.
       *
       * Retail material often abbreviates this "MFI" or "BW MFI". TA_MFI is
       * already the Money Flow Index, so this carries the name Tulip and
       * pandas-ta-classic use.
       */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         /* A zero-volume bar would divide by zero. Neither reference guards
          * it -- they emit +/-Inf, or NaN when the range is zero too -- but
          * issue #112 settled that a successful call never emits NaN or Inf,
          * so an untraded bar facilitated no movement and reports 0.
          *
          * The comparison is an exact != 0.0 rather than TA_IS_ZERO, whose
          * 1e-14 band is an absolute threshold and meaningless against an
          * unbounded volume scale. Same reasoning as the prevClose guard in
          * ta_codegen/input/nvi/nvi.c.
          */
         if( inVolume[i] != 0.0 ) {
            outReal[outIdx++] = (inHigh[i] - inLow[i]) / inVolume[i];
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode MARKETFI_Impl( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          float inVolume[],
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
         if( (double)inVolume[i] != 0.0 ) {
            outReal[outIdx++] = ((double)inHigh[i] - (double)inLow[i]) / (double)inVolume[i];
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Bill Williams' Market Facilitation Index (*Trading Chaos*, 1995): the
    * price range a bar travelled per unit of volume traded — how much movement
    * the market "facilitated" per tick. A rising index on rising volume is read
    * as a move the market is absorbing; a rising index on falling volume as one
    * it is not. Retail material commonly abbreviates this "MFI" or "BW MFI".
    * TA-Lib already ships {@code TA_MFI} for the Money Flow Index, so this
    * carries the {@code MARKETFI} name used by Tulip and pandas-ta-classic.
    * Charting packages often overlay a four-state colour code (green / fade /
    * fake / squat) derived from the signs of the bar-to-bar change in this
    * index and in volume. That is an interpretive layer on top of the series,
    * not part of it; {@code outReal} is the scalar only.
    * <p><b>Formula</b>
    * <pre>{@code
    * MARKETFI_t = (high_t - low_t) / volume_t
    * A bar with zero volume reports 0 rather than dividing: it facilitated no movement, and a successful call never emits NaN or ±Inf.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MARKETFI_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Range travelled per unit of volume, per bar. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#AD
    * @see Core#ADOSC
    * @see Core#NVI
    * @see Core#OBV
    * @see Core#PVI
    */
   public OutRange MARKETFI( int startIdx,
                             int endIdx,
                             double inHigh[],
                             double inLow[],
                             double inVolume[],
                             double outReal[] )
   {
      requireIndexRange("MARKETFI", startIdx, endIdx);
      int guardStart = clampedStart("MARKETFI", startIdx, MARKETFI_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MARKETFI", "inHigh", inHigh, guardInLen);
      requireLength("MARKETFI", "inLow", inLow, guardInLen);
      requireLength("MARKETFI", "inVolume", inVolume, guardInLen);
      requireLength("MARKETFI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MARKETFI_Impl(startIdx, endIdx, inHigh, inLow, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MARKETFI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Bill Williams' Market Facilitation Index (*Trading Chaos*, 1995): the
    * price range a bar travelled per unit of volume traded — how much movement
    * the market "facilitated" per tick. A rising index on rising volume is read
    * as a move the market is absorbing; a rising index on falling volume as one
    * it is not. Retail material commonly abbreviates this "MFI" or "BW MFI".
    * TA-Lib already ships {@code TA_MFI} for the Money Flow Index, so this
    * carries the {@code MARKETFI} name used by Tulip and pandas-ta-classic.
    * Charting packages often overlay a four-state colour code (green / fade /
    * fake / squat) derived from the signs of the bar-to-bar change in this
    * index and in volume. That is an interpretive layer on top of the series,
    * not part of it; {@code outReal} is the scalar only.
    * <p><b>Formula</b>
    * <pre>{@code
    * MARKETFI_t = (high_t - low_t) / volume_t
    * A bar with zero volume reports 0 rather than dividing: it facilitated no movement, and a successful call never emits NaN or ±Inf.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MARKETFI_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Range travelled per unit of volume, per bar. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#AD
    * @see Core#ADOSC
    * @see Core#NVI
    * @see Core#OBV
    * @see Core#PVI
    */
   public OutRange MARKETFI( int startIdx,
                             int endIdx,
                             float inHigh[],
                             float inLow[],
                             float inVolume[],
                             double outReal[] )
   {
      requireIndexRange("MARKETFI", startIdx, endIdx);
      int guardStart = clampedStart("MARKETFI", startIdx, MARKETFI_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MARKETFI", "inHigh", inHigh, guardInLen);
      requireLength("MARKETFI", "inLow", inLow, guardInLen);
      requireLength("MARKETFI", "inVolume", inVolume, guardInLen);
      requireLength("MARKETFI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MARKETFI_Impl(startIdx, endIdx, inHigh, inLow, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MARKETFI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MARKETFI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MARKETFI} over the same series.
    * Open with {@link Core#marketfiOpen}; there is no close — the handle is
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
   public static final class MarketfiStream {
      Core core;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      MarketfiStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MARKETFI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MarketfiStream( MarketfiStream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( MarketfiStream other ) {
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
       * written, so the handle is left exactly as it was —
       * the stream stays usable, so skip the bar or re-open on a clean
       * history. This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inHigh, double inLow, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("MARKETFI update: BadParam", RetCode.BadParam);
         core.marketfiStepImpl(this, inHigh, inLow, inVolume);
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
      public void updateAndFill( double inHigh[], double inLow[], double inVolume[], double outReal[] ) {
         requireArgument("MARKETFI updateAndFill", "inHigh", inHigh);
         requireArgument("MARKETFI updateAndFill", "inLow", inLow);
         requireArgument("MARKETFI updateAndFill", "inVolume", inVolume);
         requireArgument("MARKETFI updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inVolume.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("MARKETFI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inVolume[i]) )
               throw new TaLibArgumentException("MARKETFI updateAndFill: BadParam", RetCode.BadParam);
            core.marketfiStepImpl(this, inHigh[i], inLow[i], inVolume[i]);
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
      public double peek( double inHigh, double inLow, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("MARKETFI peek: BadParam", RetCode.BadParam);
         MarketfiStream scratch = new MarketfiStream(this);
         core.marketfiStepImpl(scratch, inHigh, inLow, inVolume);
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
      public MarketfiStream copy() {
         return new MarketfiStream(this);
      }
   }
   void marketfiStepImpl( MarketfiStream sp, double inHigh, double inLow, double inVolume )
   {
      /* A zero-volume bar would divide by zero. Neither reference guards
       * it -- they emit +/-Inf, or NaN when the range is zero too -- but
       * issue #112 settled that a successful call never emits NaN or Inf,
       * so an untraded bar facilitated no movement and reports 0.
       *
       * The comparison is an exact != 0.0 rather than TA_IS_ZERO, whose
       * 1e-14 band is an absolute threshold and meaningless against an
       * unbounded volume scale. Same reasoning as the prevClose guard in
       * ta_codegen/input/nvi/nvi.c.
       */
      if( inVolume != 0.0 ) {
         sp.cur_outReal = (inHigh - inLow) / inVolume;
      } else {
         sp.cur_outReal = 0.0;
      }
   }
   private RetCode marketfiOpenImpl( MarketfiStream sp, double inHigh[], double inLow[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
      if( inLow.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Bill Williams' Market Facilitation Index: the price range a bar
       * travelled per unit of volume traded, i.e. how much movement the
       * market "facilitated" per tick.
       *
       *      MARKETFI = ( High - Low ) / Volume
       *
       * Stateless and per-bar: no seeding, no smoothing, no accumulator and
       * no unstable period, so the output for a bar never depends on where
       * the caller started the range.
       *
       * Retail material often abbreviates this "MFI" or "BW MFI". TA_MFI is
       * already the Money Flow Index, so this carries the name Tulip and
       * pandas-ta-classic use.
       */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         /* A zero-volume bar would divide by zero. Neither reference guards
          * it -- they emit +/-Inf, or NaN when the range is zero too -- but
          * issue #112 settled that a successful call never emits NaN or Inf,
          * so an untraded bar facilitated no movement and reports 0.
          *
          * The comparison is an exact != 0.0 rather than TA_IS_ZERO, whose
          * 1e-14 band is an absolute threshold and meaningless against an
          * unbounded volume scale. Same reasoning as the prevClose guard in
          * ta_codegen/input/nvi/nvi.c.
          */
         if( inVolume[i] != 0.0 ) {
            outReal[outIdx++ * outStride] = (inHigh[i] - inLow[i]) / inVolume[i];
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* marketfiOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MarketfiStream marketfiOpenAndFillInternal( double inHigh[], double inLow[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MarketfiStream sp = new MarketfiStream(this);
      RetCode retCode = marketfiOpenImpl(sp, inHigh, inLow, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MARKETFI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MARKETFI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MARKETFI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind marketfiOpen (composition seam). */
   MarketfiStream marketfiOpenInternal( double inHigh[], double inLow[], double inVolume[], int startIdx )
   {
      MarketfiStream sp = new MarketfiStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = marketfiOpenImpl(sp, inHigh, inLow, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MARKETFI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MARKETFI open: internal error", retCode);
      }
      throw new TaLibArgumentException("MARKETFI open: " + retCode, retCode);
   }
   /**
    * Open a live MARKETFI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MARKETFI} at that bar.
    * <p>The history must hold at least {@code MARKETFI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MarketfiStream marketfiOpen( double inHigh[], double inLow[], double inVolume[] )
   {
      requireArgument("MARKETFI open", "inHigh", inHigh);
      requireHistory("MARKETFI open", inHigh.length);
      requireArgument("MARKETFI open", "inLow", inLow);
      requireArgument("MARKETFI open", "inVolume", inVolume);
      requireHistoryLength("MARKETFI open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("MARKETFI open", "inVolume", inVolume.length, inHigh.length);
      return marketfiOpenInternal(inHigh, inLow, inVolume, 0);
   }
   /**
    * {@link Core#marketfiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MARKETFI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MarketfiStream#outRange()}.
    */
   public MarketfiStream marketfiOpenAndFill( double inHigh[], double inLow[], double inVolume[], double outReal[] )
   {
      requireArgument("MARKETFI openAndFill", "inHigh", inHigh);
      requireHistory("MARKETFI openAndFill", inHigh.length);
      requireArgument("MARKETFI openAndFill", "inLow", inLow);
      requireArgument("MARKETFI openAndFill", "inVolume", inVolume);
      int guardOutLen = openFillCount("MARKETFI openAndFill", inHigh.length, MARKETFI_Lookback());
      requireHistoryLength("MARKETFI openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("MARKETFI openAndFill", "inVolume", inVolume.length, inHigh.length);
      requireLength("MARKETFI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("MARKETFI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return marketfiOpenAndFillInternal(inHigh, inLow, inVolume, 0, outBegIdx, outNBElement, outReal);
   }
