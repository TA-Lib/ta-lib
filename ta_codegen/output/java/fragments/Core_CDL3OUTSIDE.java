/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  121104 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDL3OUTSIDE} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDL3OUTSIDE_Lookback( )
   {
      return 3 ;

   }
   RetCode CDL3OUTSIDE_Impl( int startIdx,
                             int endIdx,
                             double inOpen[],
                             double inHigh[],
                             double inLow[],
                             double inClose[],
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDL3OUTSIDE_Lookback();
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first: black (white) real body
       * - second: white (black) real body that engulfs the prior real body
       * - third: candle that closes higher (lower) than the second candle
       * outInteger is positive (1 to 100) for the three outside up or negative (-1 to -100) for the three outside down;
       * the user should consider that a three outside up must appear in a downtrend and three outside down must appear
       * in an uptrend, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && inClose[i - 1] > inOpen[i - 2] && inOpen[i - 1] < inClose[i - 2] && inClose[i] > inClose[i - 1] || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && inOpen[i - 1] > inClose[i - 2] && inClose[i - 1] < inOpen[i - 2] && inClose[i] < inClose[i - 1] ) {
            /* white engulfs black */
            /* third candle higher */
            /* black engulfs white */
            /* third candle lower */
            outInteger[outIdx++] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDL3OUTSIDE_Impl( int startIdx,
                             int endIdx,
                             float inOpen[],
                             float inHigh[],
                             float inLow[],
                             float inClose[],
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDL3OUTSIDE_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (double)inClose[i - 1] > (double)inOpen[i - 2] && (double)inOpen[i - 1] < (double)inClose[i - 2] && (double)inClose[i] > (double)inClose[i - 1] || (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (double)inOpen[i - 1] > (double)inClose[i - 2] && (double)inClose[i - 1] < (double)inOpen[i - 2] && (double)inClose[i] < (double)inClose[i - 1] ) {
            outInteger[outIdx++] = (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A three-candle pattern: an engulfing pair (candle 2's body fully engulfs
    * candle 1's body) followed by a third candle that confirms in the engulfing
    * direction. Signals a bullish reversal (Three Outside Up) or bearish
    * reversal (Three Outside Down).
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend the pattern classically assumes (three outside up is meaningful in a downtrend, three outside down in an uptrend).</li>
    * <li>Bulkowski's testing puts Three Outside Up at a 75% bullish-reversal success rate versus 69% for Three Outside Down — both notably higher than the closely related Three Inside Up/Down (65%/60%), i.e. the engulfing "outside" variant tests as more reliable than the harami "inside" variant. ([thepatternsite.com](https://thepatternsite.com/ThreeOutsideUp.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDL3OUTSIDE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 for Three Outside Up (bullish), -100 for Three
    *        Outside Down (bearish), 0 when no pattern. Emits both signs; value is
    *        candle i-1's color * 100. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
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
    * @see Core#CDL3INSIDE
    * @see Core#CDLENGULFING
    * @see Core#CDL3LINESTRIKE
    */
   public OutRange CDL3OUTSIDE( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                int outInteger[] )
   {
      requireIndexRange("CDL3OUTSIDE", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, CDL3OUTSIDE_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDL3OUTSIDE", "inOpen", inOpen, guardInLen);
      requireLength("CDL3OUTSIDE", "inClose", inClose, guardInLen);
      requireLength("CDL3OUTSIDE", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDL3OUTSIDE_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDL3OUTSIDE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A three-candle pattern: an engulfing pair (candle 2's body fully engulfs
    * candle 1's body) followed by a third candle that confirms in the engulfing
    * direction. Signals a bullish reversal (Three Outside Up) or bearish
    * reversal (Three Outside Down).
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend the pattern classically assumes (three outside up is meaningful in a downtrend, three outside down in an uptrend).</li>
    * <li>Bulkowski's testing puts Three Outside Up at a 75% bullish-reversal success rate versus 69% for Three Outside Down — both notably higher than the closely related Three Inside Up/Down (65%/60%), i.e. the engulfing "outside" variant tests as more reliable than the harami "inside" variant. ([thepatternsite.com](https://thepatternsite.com/ThreeOutsideUp.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDL3OUTSIDE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 for Three Outside Up (bullish), -100 for Three
    *        Outside Down (bearish), 0 when no pattern. Emits both signs; value is
    *        candle i-1's color * 100. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
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
    * @see Core#CDL3INSIDE
    * @see Core#CDLENGULFING
    * @see Core#CDL3LINESTRIKE
    */
   public OutRange CDL3OUTSIDE( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                int outInteger[] )
   {
      requireIndexRange("CDL3OUTSIDE", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, CDL3OUTSIDE_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDL3OUTSIDE", "inOpen", inOpen, guardInLen);
      requireLength("CDL3OUTSIDE", "inClose", inClose, guardInLen);
      requireLength("CDL3OUTSIDE", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDL3OUTSIDE_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDL3OUTSIDE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDL3OUTSIDE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDL3OUTSIDE} over the same series.
    * Open with {@link Core#CDL3OUTSIDE_Open}; there is no close — the handle is
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
   public static final class CDL3OUTSIDE_Stream {
      Core core;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inClose;
      double lag2_inClose;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      CDL3OUTSIDE_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDL3OUTSIDE} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CDL3OUTSIDE_Stream( CDL3OUTSIDE_Stream other ) {
         this.core = other.core;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.cur_outInteger = other.cur_outInteger;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( CDL3OUTSIDE_Stream other ) {
         this.core = other.core;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.cur_outInteger = other.cur_outInteger;
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
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDL3OUTSIDE update: BadParam", RetCode.BadParam);
         core.CDL3OUTSIDE_StepImpl(this, inOpen, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outInteger;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inOpen.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] ) {
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDL3OUTSIDE updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("CDL3OUTSIDE updateAndFill: BadParam", RetCode.BadParam);
            core.CDL3OUTSIDE_StepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
            outInteger[i] = this.cur_outInteger;
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
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDL3OUTSIDE peek: BadParam", RetCode.BadParam);
         CDL3OUTSIDE_Stream scratch = new CDL3OUTSIDE_Stream(this);
         core.CDL3OUTSIDE_StepImpl(scratch, inOpen, inHigh, inLow, inClose);
         return scratch.cur_outInteger;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public int value() {
         return this.cur_outInteger;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public CDL3OUTSIDE_Stream copy() {
         return new CDL3OUTSIDE_Stream(this);
      }
   }
   void CDL3OUTSIDE_StepImpl( CDL3OUTSIDE_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      if( ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 0 - 1 && sp.lag1_inClose > sp.lag2_inOpen && sp.lag1_inOpen < sp.lag2_inClose && inClose > sp.lag1_inClose || ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 1 && sp.lag1_inOpen > sp.lag2_inClose && sp.lag1_inClose < sp.lag2_inOpen && inClose < sp.lag1_inClose ) {
         /* white engulfs black */
         /* third candle higher */
         /* black engulfs white */
         /* third candle lower */
         sp.cur_outInteger = ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
   }
   private RetCode CDL3OUTSIDE_OpenImpl( CDL3OUTSIDE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
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
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDL3OUTSIDE_Lookback();
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first: black (white) real body
       * - second: white (black) real body that engulfs the prior real body
       * - third: candle that closes higher (lower) than the second candle
       * outInteger is positive (1 to 100) for the three outside up or negative (-1 to -100) for the three outside down;
       * the user should consider that a three outside up must appear in a downtrend and three outside down must appear
       * in an uptrend, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && inClose[i - 1] > inOpen[i - 2] && inOpen[i - 1] < inClose[i - 2] && inClose[i] > inClose[i - 1] || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && inOpen[i - 1] > inClose[i - 2] && inClose[i - 1] < inOpen[i - 2] && inClose[i] < inClose[i - 1] ) {
            /* white engulfs black */
            /* third candle higher */
            /* black engulfs white */
            /* third candle lower */
            outInteger[outIdx++ * outStride] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* CDL3OUTSIDE_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDL3OUTSIDE_Stream CDL3OUTSIDE_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDL3OUTSIDE_Stream sp = new CDL3OUTSIDE_Stream(this);
      RetCode retCode = CDL3OUTSIDE_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDL3OUTSIDE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDL3OUTSIDE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDL3OUTSIDE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind CDL3OUTSIDE_Open (composition seam). */
   CDL3OUTSIDE_Stream CDL3OUTSIDE_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDL3OUTSIDE_Stream sp = new CDL3OUTSIDE_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = CDL3OUTSIDE_OpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDL3OUTSIDE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDL3OUTSIDE open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDL3OUTSIDE open: " + retCode, retCode);
   }
   /**
    * Open a live CDL3OUTSIDE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDL3OUTSIDE} at that bar.
    * <p>The history must hold at least {@code CDL3OUTSIDE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDL3OUTSIDE_Stream CDL3OUTSIDE_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDL3OUTSIDE_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDL3OUTSIDE_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDL3OUTSIDE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDL3OUTSIDE_Stream#outRange()}.
    */
   public CDL3OUTSIDE_Stream CDL3OUTSIDE_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDL3OUTSIDE openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return CDL3OUTSIDE_OpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
