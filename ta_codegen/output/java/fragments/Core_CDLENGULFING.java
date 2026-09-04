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
 *  102404 AC   Creation
 *  040309 AC   Increased flexibility to allow real bodies matching
 *              on one end (Greg Morris - "Candlestick charting explained")
 */

   /**
    * Number of leading input bars {@link Core#CDLENGULFING} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLENGULFING_Lookback( )
   {
      return 2 ;

   }
   RetCode CDLENGULFING_Impl( int startIdx,
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
      lookbackTotal = CDLENGULFING_Lookback();
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
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish:
       * - 100 is returned when the second candle's real body begins before and ends after the first candle's real body
       * - 80 is returned when the two real bodies match on one end (Greg Morris contemplate this case in his book
       *   "Candlestick charting explained")
       * The user should consider that an engulfing must appear in a downtrend if bullish or in an uptrend if bearish,
       * while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&
              ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* white engulfs black */
              (inClose[i] >= inOpen[i - 1] &&
                inOpen[i] < inClose[i - 1] ||
               inClose[i] > inOpen[i - 1] &&
                inOpen[i] <= inClose[i - 1]) ||
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&
              ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 &&     /* black engulfs white */
              (inOpen[i] >= inClose[i - 1] &&
                inClose[i] < inOpen[i - 1] ||
               inOpen[i] > inClose[i - 1] &&
                inClose[i] <= inOpen[i - 1]) )
         {
            if( inOpen[i] != inClose[i - 1] && inClose[i] != inOpen[i - 1] ) {
               outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
            } else {
               outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 80;
            }
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
   RetCode CDLENGULFING_Impl( int startIdx,
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
      lookbackTotal = CDLENGULFING_Lookback();
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
         if( (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((double)inClose[i] >= (double)inOpen[i - 1] && (double)inOpen[i] < (double)inClose[i - 1] || (double)inClose[i] > (double)inOpen[i - 1] && (double)inOpen[i] <= (double)inClose[i - 1]) || (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((double)inOpen[i] >= (double)inClose[i - 1] && (double)inClose[i] < (double)inOpen[i - 1] || (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] <= (double)inOpen[i - 1]) ) {
            if( (double)inOpen[i] != (double)inClose[i - 1] && (double)inClose[i] != (double)inOpen[i - 1] ) {
               outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
            } else {
               outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 80;
            }
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
    * A two-candle reversal pattern where the second candle's real body engulfs
    * the first candle's opposite-colored real body. Bullish (white engulfs
    * black) or bearish (black engulfs white) reversal signal; ideally after a
    * downtrend (bullish) or uptrend (bearish), which the code does not verify.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend (down for bullish, up for bearish) the reversal classically assumes.</li>
    * <li>Bulkowski's testing found bearish Engulfing has a strong 79% reversal rate (5th-best of 103 patterns by that measure alone) but a weak overall post-breakout performance rank of 91st of 103 — the reversal fires reliably but rarely sustains. Bullish Engulfing reverses 63% of the time with a similarly weak overall rank of 84th of 103. ([thepatternsite.com](https://thepatternsite.com/BearEngulfing.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLENGULFING_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100/+80 (bullish, white engulfs black), -100/-80
    *        (bearish, black engulfs white), 0 otherwise. Magnitude 100 when the second
    *        body strictly engulfs both ends; 80 when the bodies share an exact
    *        endpoint (open[i]==close[i-1] or close[i]==open[i-1]) Must hold at least
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
    * @see Core#CDLHARAMI
    * @see Core#CDLCOUNTERATTACK
    * @see Core#CDLHARAMICROSS
    */
   public OutRange CDLENGULFING( int startIdx,
                                 int endIdx,
                                 double inOpen[],
                                 double inHigh[],
                                 double inLow[],
                                 double inClose[],
                                 int outInteger[] )
   {
      requireIndexRange("CDLENGULFING", startIdx, endIdx);
      int guardStart = clampedStart("CDLENGULFING", startIdx, CDLENGULFING_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLENGULFING", "inOpen", inOpen, guardInLen);
      requireLength("CDLENGULFING", "inHigh", inHigh, guardInLen);
      requireLength("CDLENGULFING", "inLow", inLow, guardInLen);
      requireLength("CDLENGULFING", "inClose", inClose, guardInLen);
      requireLength("CDLENGULFING", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLENGULFING_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLENGULFING", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A two-candle reversal pattern where the second candle's real body engulfs
    * the first candle's opposite-colored real body. Bullish (white engulfs
    * black) or bearish (black engulfs white) reversal signal; ideally after a
    * downtrend (bullish) or uptrend (bearish), which the code does not verify.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend (down for bullish, up for bearish) the reversal classically assumes.</li>
    * <li>Bulkowski's testing found bearish Engulfing has a strong 79% reversal rate (5th-best of 103 patterns by that measure alone) but a weak overall post-breakout performance rank of 91st of 103 — the reversal fires reliably but rarely sustains. Bullish Engulfing reverses 63% of the time with a similarly weak overall rank of 84th of 103. ([thepatternsite.com](https://thepatternsite.com/BearEngulfing.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLENGULFING_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100/+80 (bullish, white engulfs black), -100/-80
    *        (bearish, black engulfs white), 0 otherwise. Magnitude 100 when the second
    *        body strictly engulfs both ends; 80 when the bodies share an exact
    *        endpoint (open[i]==close[i-1] or close[i]==open[i-1]) Must hold at least
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
    * @see Core#CDLHARAMI
    * @see Core#CDLCOUNTERATTACK
    * @see Core#CDLHARAMICROSS
    */
   public OutRange CDLENGULFING( int startIdx,
                                 int endIdx,
                                 float inOpen[],
                                 float inHigh[],
                                 float inLow[],
                                 float inClose[],
                                 int outInteger[] )
   {
      requireIndexRange("CDLENGULFING", startIdx, endIdx);
      int guardStart = clampedStart("CDLENGULFING", startIdx, CDLENGULFING_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLENGULFING", "inOpen", inOpen, guardInLen);
      requireLength("CDLENGULFING", "inHigh", inHigh, guardInLen);
      requireLength("CDLENGULFING", "inLow", inLow, guardInLen);
      requireLength("CDLENGULFING", "inClose", inClose, guardInLen);
      requireLength("CDLENGULFING", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLENGULFING_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLENGULFING", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLENGULFING stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLENGULFING} over the same series.
    * Open with {@link Core#cdlengulfingOpen}; there is no close — the handle is
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
   public static final class CdlengulfingStream {
      Core core;
      double lag1_inOpen;
      double lag1_inClose;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      CdlengulfingStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLENGULFING} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CdlengulfingStream( CdlengulfingStream other ) {
         this.core = other.core;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inClose = other.lag1_inClose;
         this.cur_outInteger = other.cur_outInteger;
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
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("CDLENGULFING update: BadParam", RetCode.BadParam);
         }
         core.cdlengulfingStepImpl(this, inOpen, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outInteger;
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
      public void updateAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] ) {
         requireArgument("CDLENGULFING updateAndFill", "inOpen", inOpen);
         requireArgument("CDLENGULFING updateAndFill", "inHigh", inHigh);
         requireArgument("CDLENGULFING updateAndFill", "inLow", inLow);
         requireArgument("CDLENGULFING updateAndFill", "inClose", inClose);
         requireArgument("CDLENGULFING updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLENGULFING updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CDLENGULFING updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cdlengulfingStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
            outInteger[i] = this.cur_outInteger;
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
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDLENGULFING peek: BadParam", RetCode.BadParam);
         CdlengulfingStream sp = this;
         int cur_outInteger = 0;
         if( ((inClose >= inOpen) ? 1 : 0 - 1) == 1 &&
              ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* white engulfs black */
              (inClose >= sp.lag1_inOpen &&
                inOpen < sp.lag1_inClose ||
               inClose > sp.lag1_inOpen &&
                inOpen <= sp.lag1_inClose) ||
             ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 &&
              ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 &&     /* black engulfs white */
              (inOpen >= sp.lag1_inClose &&
                inClose < sp.lag1_inOpen ||
               inOpen > sp.lag1_inClose &&
                inClose <= sp.lag1_inOpen) )
         {
            if( inOpen != sp.lag1_inClose && inClose != sp.lag1_inOpen ) {
               cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 100;
            } else {
               cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 80;
            }
         } else {
            cur_outInteger = 0;
         }
         return cur_outInteger;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public int value() {
         return this.cur_outInteger;
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
      public CdlengulfingStream clone() {
         return new CdlengulfingStream(this);
      }
   }
   void cdlengulfingStepImpl( CdlengulfingStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      if( ((inClose >= inOpen) ? 1 : 0 - 1) == 1 &&
           ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && /* white engulfs black */
           (inClose >= sp.lag1_inOpen &&
             inOpen < sp.lag1_inClose ||
            inClose > sp.lag1_inOpen &&
             inOpen <= sp.lag1_inClose) ||
          ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 &&
           ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 &&     /* black engulfs white */
           (inOpen >= sp.lag1_inClose &&
             inClose < sp.lag1_inOpen ||
            inOpen > sp.lag1_inClose &&
             inClose <= sp.lag1_inOpen) )
      {
         if( inOpen != sp.lag1_inClose && inClose != sp.lag1_inOpen ) {
            sp.cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 100;
         } else {
            sp.cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 80;
         }
      } else {
         sp.cur_outInteger = 0;
      }
      sp.lag1_inOpen = inOpen;
      sp.lag1_inClose = inClose;
   }
   private RetCode cdlengulfingOpenImpl( CdlengulfingStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
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
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLENGULFING_Lookback();
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
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish:
       * - 100 is returned when the second candle's real body begins before and ends after the first candle's real body
       * - 80 is returned when the two real bodies match on one end (Greg Morris contemplate this case in his book
       *   "Candlestick charting explained")
       * The user should consider that an engulfing must appear in a downtrend if bullish or in an uptrend if bearish,
       * while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 &&
              ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && /* white engulfs black */
              (inClose[i] >= inOpen[i - 1] &&
                inOpen[i] < inClose[i - 1] ||
               inClose[i] > inOpen[i - 1] &&
                inOpen[i] <= inClose[i - 1]) ||
             ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 &&
              ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 &&     /* black engulfs white */
              (inOpen[i] >= inClose[i - 1] &&
                inClose[i] < inOpen[i - 1] ||
               inOpen[i] > inClose[i - 1] &&
                inClose[i] <= inOpen[i - 1]) )
         {
            if( inOpen[i] != inClose[i - 1] && inClose[i] != inOpen[i - 1] ) {
               outInteger[outIdx++ * outStride] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
            } else {
               outInteger[outIdx++ * outStride] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 80;
            }
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
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cdlengulfingOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CdlengulfingStream cdlengulfingOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlengulfingStream sp = new CdlengulfingStream(this);
      RetCode retCode = cdlengulfingOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLENGULFING openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLENGULFING openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLENGULFING openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cdlengulfingOpen (composition seam). */
   CdlengulfingStream cdlengulfingOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlengulfingStream sp = new CdlengulfingStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdlengulfingOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLENGULFING open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLENGULFING open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLENGULFING open: " + retCode, retCode);
   }
   /**
    * Open a live CDLENGULFING stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLENGULFING} at that bar.
    * <p>The history must hold at least {@code CDLENGULFING_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CdlengulfingStream cdlengulfingOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLENGULFING open", "inOpen", inOpen);
      requireHistory("CDLENGULFING open", inOpen.length);
      requireArgument("CDLENGULFING open", "inHigh", inHigh);
      requireArgument("CDLENGULFING open", "inLow", inLow);
      requireArgument("CDLENGULFING open", "inClose", inClose);
      requireHistoryLength("CDLENGULFING open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLENGULFING open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLENGULFING open", "inClose", inClose.length, inOpen.length);
      return cdlengulfingOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlengulfingOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CDLENGULFING} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CdlengulfingStream#outRange()}.
    */
   public CdlengulfingStream cdlengulfingOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLENGULFING openAndFill", "inOpen", inOpen);
      requireHistory("CDLENGULFING openAndFill", inOpen.length);
      requireArgument("CDLENGULFING openAndFill", "inHigh", inHigh);
      requireArgument("CDLENGULFING openAndFill", "inLow", inLow);
      requireArgument("CDLENGULFING openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLENGULFING openAndFill", inOpen.length, CDLENGULFING_Lookback());
      requireHistoryLength("CDLENGULFING openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLENGULFING openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLENGULFING openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLENGULFING openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLENGULFING openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdlengulfingOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
