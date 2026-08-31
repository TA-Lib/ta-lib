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
 *  011605 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLXSIDEGAP3METHODS} consumes
    * before it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLXSIDEGAP3METHODS_Lookback( )
   {
      return 2 ;

   }
   RetCode CDLXSIDEGAP3METHODS_Impl( int startIdx,
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
      lookbackTotal = CDLXSIDEGAP3METHODS_Lookback();
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
       * - first candle: white (black) candle
       * - second candle: white (black) candle
       * - upside (downside) gap between the first and the second real bodies
       * - third candle: black (white) candle that opens within the second real body and closes within the first real body
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that up/downside gap 3 methods is significant when it appears in a trend, while this
       * function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && /* 1st and 2nd of same color */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) && /* 3rd opposite color */
             inOpen[i] < Math.max(inClose[i - 1], inOpen[i - 1]) &&  /* 3rd opens within 2nd rb */
             inOpen[i] > Math.min(inClose[i - 1], inOpen[i - 1]) &&
             inClose[i] < Math.max(inClose[i - 2], inOpen[i - 2]) && /* 3rd closes within 1st rb */
             inClose[i] > Math.min(inClose[i - 2], inOpen[i - 2]) &&
             (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) || ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2]))) ) /* when 1st is white upside gap when 1st is black downside gap */
         {
            outInteger[outIdx++] = ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLXSIDEGAP3METHODS_Impl( int startIdx,
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
      lookbackTotal = CDLXSIDEGAP3METHODS_Lookback();
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
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) && (double)inOpen[i] < Math.max((double)inClose[i - 1], (double)inOpen[i - 1]) && (double)inOpen[i] > Math.min((double)inClose[i - 1], (double)inOpen[i - 1]) && (double)inClose[i] < Math.max((double)inClose[i - 2], (double)inOpen[i - 2]) && (double)inClose[i] > Math.min((double)inClose[i - 2], (double)inOpen[i - 2]) && ((((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) || (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2]))) ) {
            outInteger[outIdx++] = (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) * 100;
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
    * A three-candle continuation pattern: two same-color candles separated by a
    * real-body gap, followed by an opposite-color candle that fills into the
    * gap. Bullish (upside) when the first two candles are white, bearish
    * (downside) when they are black.
    * <p><b>Notes</b>
    * <ul>
    * <li>This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.</li>
    * <li>Bulkowski's testing found BOTH directions of this pattern actually act as reversals more often than not, opposite the classic continuation label: the upside variant reverses bearish 59% of the time, the downside variant reverses bullish 62% of the time. ([thepatternsite.com](https://thepatternsite.com/UpGap3Methods.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLXSIDEGAP3METHODS_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the two same-color candles are white
    *        (bullish/upside continuation), -100 when black (bearish/downside
    *        continuation), 0 otherwise. Equals candlecolor(1st candle) * 100. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#CDLGAPSIDESIDEWHITE
    * @see Core#CDLTASUKIGAP
    * @see Core#CDLRISEFALL3METHODS
    */
   public OutRange CDLXSIDEGAP3METHODS( int startIdx,
                                        int endIdx,
                                        double inOpen[],
                                        double inHigh[],
                                        double inLow[],
                                        double inClose[],
                                        int outInteger[] )
   {
      requireIndexRange("CDLXSIDEGAP3METHODS", startIdx, endIdx);
      int guardStart = clampedStart("CDLXSIDEGAP3METHODS", startIdx, CDLXSIDEGAP3METHODS_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLXSIDEGAP3METHODS", "inOpen", inOpen, guardInLen);
      requireLength("CDLXSIDEGAP3METHODS", "inHigh", inHigh, guardInLen);
      requireLength("CDLXSIDEGAP3METHODS", "inLow", inLow, guardInLen);
      requireLength("CDLXSIDEGAP3METHODS", "inClose", inClose, guardInLen);
      requireLength("CDLXSIDEGAP3METHODS", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLXSIDEGAP3METHODS_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLXSIDEGAP3METHODS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A three-candle continuation pattern: two same-color candles separated by a
    * real-body gap, followed by an opposite-color candle that fills into the
    * gap. Bullish (upside) when the first two candles are white, bearish
    * (downside) when they are black.
    * <p><b>Notes</b>
    * <ul>
    * <li>This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.</li>
    * <li>Bulkowski's testing found BOTH directions of this pattern actually act as reversals more often than not, opposite the classic continuation label: the upside variant reverses bearish 59% of the time, the downside variant reverses bullish 62% of the time. ([thepatternsite.com](https://thepatternsite.com/UpGap3Methods.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLXSIDEGAP3METHODS_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 when the two same-color candles are white
    *        (bullish/upside continuation), -100 when black (bearish/downside
    *        continuation), 0 otherwise. Equals candlecolor(1st candle) * 100. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#CDLGAPSIDESIDEWHITE
    * @see Core#CDLTASUKIGAP
    * @see Core#CDLRISEFALL3METHODS
    */
   public OutRange CDLXSIDEGAP3METHODS( int startIdx,
                                        int endIdx,
                                        float inOpen[],
                                        float inHigh[],
                                        float inLow[],
                                        float inClose[],
                                        int outInteger[] )
   {
      requireIndexRange("CDLXSIDEGAP3METHODS", startIdx, endIdx);
      int guardStart = clampedStart("CDLXSIDEGAP3METHODS", startIdx, CDLXSIDEGAP3METHODS_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLXSIDEGAP3METHODS", "inOpen", inOpen, guardInLen);
      requireLength("CDLXSIDEGAP3METHODS", "inHigh", inHigh, guardInLen);
      requireLength("CDLXSIDEGAP3METHODS", "inLow", inLow, guardInLen);
      requireLength("CDLXSIDEGAP3METHODS", "inClose", inClose, guardInLen);
      requireLength("CDLXSIDEGAP3METHODS", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLXSIDEGAP3METHODS_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLXSIDEGAP3METHODS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLXSIDEGAP3METHODS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLXSIDEGAP3METHODS} over the same series.
    * Open with {@link Core#cdlxsidegap3methodsOpen}; there is no close — the handle is
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
   public static final class Cdlxsidegap3methodsStream {
      Core core;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inClose;
      double lag2_inClose;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      Cdlxsidegap3methodsStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLXSIDEGAP3METHODS} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      Cdlxsidegap3methodsStream( Cdlxsidegap3methodsStream other ) {
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
            throw new TaLibArgumentException("CDLXSIDEGAP3METHODS update: BadParam", RetCode.BadParam);
         }
         core.cdlxsidegap3methodsStepImpl(this, inOpen, inHigh, inLow, inClose);
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
         requireArgument("CDLXSIDEGAP3METHODS updateAndFill", "inOpen", inOpen);
         requireArgument("CDLXSIDEGAP3METHODS updateAndFill", "inHigh", inHigh);
         requireArgument("CDLXSIDEGAP3METHODS updateAndFill", "inLow", inLow);
         requireArgument("CDLXSIDEGAP3METHODS updateAndFill", "inClose", inClose);
         requireArgument("CDLXSIDEGAP3METHODS updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLXSIDEGAP3METHODS updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CDLXSIDEGAP3METHODS updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cdlxsidegap3methodsStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
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
            throw new TaLibArgumentException("CDLXSIDEGAP3METHODS peek: BadParam", RetCode.BadParam);
         Cdlxsidegap3methodsStream sp = this;
         int cur_outInteger = sp.cur_outInteger;
         double lag1_inClose = sp.lag1_inClose;
         double lag1_inOpen = sp.lag1_inOpen;
         double lag2_inClose = sp.lag2_inClose;
         double lag2_inOpen = sp.lag2_inOpen;
         if( ((lag2_inClose >= lag2_inOpen) ? 1 : 0 - 1) == ((lag1_inClose >= lag1_inOpen) ? 1 : 0 - 1) && /* 1st and 2nd of same color */
             ((lag1_inClose >= lag1_inOpen) ? 1 : 0 - 1) == 0 - ((inClose >= inOpen) ? 1 : 0 - 1) && /* 3rd opposite color */
             inOpen < Math.max(lag1_inClose, lag1_inOpen) &&  /* 3rd opens within 2nd rb */
             inOpen > Math.min(lag1_inClose, lag1_inOpen) &&
             inClose < Math.max(lag2_inClose, lag2_inOpen) && /* 3rd closes within 1st rb */
             inClose > Math.min(lag2_inClose, lag2_inOpen) &&
             (((lag2_inClose >= lag2_inOpen) ? 1 : 0 - 1) == 1 && (Math.min(lag1_inOpen, lag1_inClose) > Math.max(lag2_inOpen, lag2_inClose)) || ((lag2_inClose >= lag2_inOpen) ? 1 : 0 - 1) == 0 - 1 && (Math.max(lag1_inOpen, lag1_inClose) < Math.min(lag2_inOpen, lag2_inClose))) ) /* when 1st is white upside gap when 1st is black downside gap */
         {
            cur_outInteger = ((lag2_inClose >= lag2_inOpen) ? 1 : 0 - 1) * 100;
         } else {
            cur_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         lag2_inOpen = lag1_inOpen;
         lag1_inOpen = inOpen;
         lag2_inClose = lag1_inClose;
         lag1_inClose = inClose;
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
      public Cdlxsidegap3methodsStream clone() {
         return new Cdlxsidegap3methodsStream(this);
      }
   }
   void cdlxsidegap3methodsStepImpl( Cdlxsidegap3methodsStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      if( ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) && /* 1st and 2nd of same color */
          ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - ((inClose >= inOpen) ? 1 : 0 - 1) && /* 3rd opposite color */
          inOpen < Math.max(sp.lag1_inClose, sp.lag1_inOpen) &&  /* 3rd opens within 2nd rb */
          inOpen > Math.min(sp.lag1_inClose, sp.lag1_inOpen) &&
          inClose < Math.max(sp.lag2_inClose, sp.lag2_inOpen) && /* 3rd closes within 1st rb */
          inClose > Math.min(sp.lag2_inClose, sp.lag2_inOpen) &&
          (((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 1 && (Math.min(sp.lag1_inOpen, sp.lag1_inClose) > Math.max(sp.lag2_inOpen, sp.lag2_inClose)) || ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) == 0 - 1 && (Math.max(sp.lag1_inOpen, sp.lag1_inClose) < Math.min(sp.lag2_inOpen, sp.lag2_inClose))) ) /* when 1st is white upside gap when 1st is black downside gap */
      {
         sp.cur_outInteger = ((sp.lag2_inClose >= sp.lag2_inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.lag2_inOpen = sp.lag1_inOpen;
      sp.lag1_inOpen = inOpen;
      sp.lag2_inClose = sp.lag1_inClose;
      sp.lag1_inClose = inClose;
   }
   private RetCode cdlxsidegap3methodsOpenImpl( Cdlxsidegap3methodsStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
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
      lookbackTotal = CDLXSIDEGAP3METHODS_Lookback();
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
       * - first candle: white (black) candle
       * - second candle: white (black) candle
       * - upside (downside) gap between the first and the second real bodies
       * - third candle: black (white) candle that opens within the second real body and closes within the first real body
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that up/downside gap 3 methods is significant when it appears in a trend, while this
       * function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && /* 1st and 2nd of same color */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) && /* 3rd opposite color */
             inOpen[i] < Math.max(inClose[i - 1], inOpen[i - 1]) &&  /* 3rd opens within 2nd rb */
             inOpen[i] > Math.min(inClose[i - 1], inOpen[i - 1]) &&
             inClose[i] < Math.max(inClose[i - 2], inOpen[i - 2]) && /* 3rd closes within 1st rb */
             inClose[i] > Math.min(inClose[i - 2], inOpen[i - 2]) &&
             (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) || ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2]))) ) /* when 1st is white upside gap when 1st is black downside gap */
         {
            outInteger[outIdx++ * outStride] = ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++ * outStride] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
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
   /* cdlxsidegap3methodsOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   Cdlxsidegap3methodsStream cdlxsidegap3methodsOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      Cdlxsidegap3methodsStream sp = new Cdlxsidegap3methodsStream(this);
      RetCode retCode = cdlxsidegap3methodsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLXSIDEGAP3METHODS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLXSIDEGAP3METHODS openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLXSIDEGAP3METHODS openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cdlxsidegap3methodsOpen (composition seam). */
   Cdlxsidegap3methodsStream cdlxsidegap3methodsOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      Cdlxsidegap3methodsStream sp = new Cdlxsidegap3methodsStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdlxsidegap3methodsOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLXSIDEGAP3METHODS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLXSIDEGAP3METHODS open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLXSIDEGAP3METHODS open: " + retCode, retCode);
   }
   /**
    * Open a live CDLXSIDEGAP3METHODS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLXSIDEGAP3METHODS} at that bar.
    * <p>The history must hold at least {@code CDLXSIDEGAP3METHODS_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public Cdlxsidegap3methodsStream cdlxsidegap3methodsOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLXSIDEGAP3METHODS open", "inOpen", inOpen);
      requireHistory("CDLXSIDEGAP3METHODS open", inOpen.length);
      requireArgument("CDLXSIDEGAP3METHODS open", "inHigh", inHigh);
      requireArgument("CDLXSIDEGAP3METHODS open", "inLow", inLow);
      requireArgument("CDLXSIDEGAP3METHODS open", "inClose", inClose);
      requireHistoryLength("CDLXSIDEGAP3METHODS open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLXSIDEGAP3METHODS open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLXSIDEGAP3METHODS open", "inClose", inClose.length, inOpen.length);
      return cdlxsidegap3methodsOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlxsidegap3methodsOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CDLXSIDEGAP3METHODS} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link Cdlxsidegap3methodsStream#outRange()}.
    */
   public Cdlxsidegap3methodsStream cdlxsidegap3methodsOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLXSIDEGAP3METHODS openAndFill", "inOpen", inOpen);
      requireHistory("CDLXSIDEGAP3METHODS openAndFill", inOpen.length);
      requireArgument("CDLXSIDEGAP3METHODS openAndFill", "inHigh", inHigh);
      requireArgument("CDLXSIDEGAP3METHODS openAndFill", "inLow", inLow);
      requireArgument("CDLXSIDEGAP3METHODS openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLXSIDEGAP3METHODS openAndFill", inOpen.length, CDLXSIDEGAP3METHODS_Lookback());
      requireHistoryLength("CDLXSIDEGAP3METHODS openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLXSIDEGAP3METHODS openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLXSIDEGAP3METHODS openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLXSIDEGAP3METHODS openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLXSIDEGAP3METHODS openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdlxsidegap3methodsOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
