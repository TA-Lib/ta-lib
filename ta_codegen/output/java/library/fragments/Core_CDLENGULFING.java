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
    * Number of leading input bars {@link Core#cdlEngulfing} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int cdlEngulfingLookback( )
   {
      return 2 ;

   }
   RetCode cdlEngulfingInternal( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlEngulfingLookback();
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
         if( ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (inClose[i] >= inOpen[i - 1] && inOpen[i] < inClose[i - 1] || inClose[i] > inOpen[i - 1] && inOpen[i] <= inClose[i - 1]) || ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (inOpen[i] >= inClose[i - 1] && inClose[i] < inOpen[i - 1] || inOpen[i] > inClose[i - 1] && inClose[i] <= inOpen[i - 1]) ) {
            /* white engulfs black */
            /* black engulfs white */
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
   RetCode cdlEngulfingInternal( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlEngulfingLookback();
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
    * black) or bearish (black engulfs white) reversal signal. Bullish reversal
    * at +100/+80, bearish at -100/-80; ideally after a downtrend (bullish) or
    * uptrend (bearish), which the code does not verify.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend (down for bullish, up for bearish) the reversal classically assumes.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#cdlEngulfingLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
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
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#cdlHarami
    * @see Core#cdlCounterAttack
    * @see Core#cdlHaramiCross
    */
   public OutRange cdlEngulfing( int startIdx,
                                 int endIdx,
                                 double inOpen[],
                                 double inHigh[],
                                 double inLow[],
                                 double inClose[],
                                 int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cdlEngulfingInternal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLENGULFING", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A two-candle reversal pattern where the second candle's real body engulfs
    * the first candle's opposite-colored real body. Bullish (white engulfs
    * black) or bearish (black engulfs white) reversal signal. Bullish reversal
    * at +100/+80, bearish at -100/-80; ideally after a downtrend (bullish) or
    * uptrend (bearish), which the code does not verify.
    * <p><b>Notes</b>
    * <ul>
    * <li>Does not verify the prior trend (down for bullish, up for bearish) the reversal classically assumes.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#cdlEngulfingLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
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
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#cdlHarami
    * @see Core#cdlCounterAttack
    * @see Core#cdlHaramiCross
    */
   public OutRange cdlEngulfing( int startIdx,
                                 int endIdx,
                                 float inOpen[],
                                 float inHigh[],
                                 float inLow[],
                                 float inClose[],
                                 int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cdlEngulfingInternal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLENGULFING", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLENGULFING stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlEngulfing} over the same series.
    * Open with {@link Core#cdlEngulfingOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class CdlEngulfingStream {
      final Core core;
      double lag1_inOpen;
      double lag1_inClose;
      int cur_outInteger;
      OutRange fillRange;

      CdlEngulfingStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#cdlEngulfingOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      CdlEngulfingStream( CdlEngulfingStream other ) {
         this.core = other.core;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag1_inClose = other.lag1_inClose;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.cdlEngulfingStreamStep(this, inOpen, inHigh, inLow, inClose);
         return this.cur_outInteger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         CdlEngulfingStream scratch = new CdlEngulfingStream(this);
         core.cdlEngulfingStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CdlEngulfingStream copy() {
         return new CdlEngulfingStream(this);
      }
   }
   void cdlEngulfingStreamStep( CdlEngulfingStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      if( ((inClose >= inOpen) ? 1 : 0 - 1) == 1 && ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 0 - 1 && (inClose >= sp.lag1_inOpen && inOpen < sp.lag1_inClose || inClose > sp.lag1_inOpen && inOpen <= sp.lag1_inClose) || ((inClose >= inOpen) ? 1 : 0 - 1) == 0 - 1 && ((sp.lag1_inClose >= sp.lag1_inOpen) ? 1 : 0 - 1) == 1 && (inOpen >= sp.lag1_inClose && inClose < sp.lag1_inOpen || inOpen > sp.lag1_inClose && inClose <= sp.lag1_inOpen) ) {
         /* white engulfs black */
         /* black engulfs white */
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
   private RetCode cdlEngulfingOpenBody( CdlEngulfingStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int lastValue_outInteger = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlEngulfingLookback();
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
         return RetCode.OutOfRangeEndIndex ;
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
         if( ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (inClose[i] >= inOpen[i - 1] && inOpen[i] < inClose[i - 1] || inClose[i] > inOpen[i - 1] && inOpen[i] <= inClose[i - 1]) || ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (inOpen[i] >= inClose[i - 1] && inClose[i] < inOpen[i - 1] || inOpen[i] > inClose[i - 1] && inClose[i] <= inOpen[i - 1]) ) {
            /* white engulfs black */
            /* black engulfs white */
            if( inOpen[i] != inClose[i - 1] && inClose[i] != inOpen[i - 1] ) {
               lastValue_outInteger = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
            } else {
               lastValue_outInteger = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 80;
            }
         } else {
            lastValue_outInteger = 0;
         }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlEngulfingOpenAndFillBody( CdlEngulfingStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlEngulfingLookback();
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
         return RetCode.OutOfRangeEndIndex ;
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
         if( ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (inClose[i] >= inOpen[i - 1] && inOpen[i] < inClose[i - 1] || inClose[i] > inOpen[i - 1] && inOpen[i] <= inClose[i - 1]) || ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (inOpen[i] >= inClose[i - 1] && inClose[i] < inOpen[i - 1] || inOpen[i] > inClose[i - 1] && inClose[i] <= inOpen[i - 1]) ) {
            /* white engulfs black */
            /* black engulfs white */
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
      /* Capture the live batch state into the handle. */
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlEngulfingOpen (composition seam). */
   CdlEngulfingStream cdlEngulfingOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlEngulfingStream sp = new CdlEngulfingStream(this);
      RetCode retCode = cdlEngulfingOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLENGULFING open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLENGULFING open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLENGULFING open: " + retCode);
   }
   /**
    * Open a live CDLENGULFING stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlEngulfing} at that bar.
    * <p>The history must hold at least {@code cdlEngulfingLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlEngulfingStream cdlEngulfingOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return cdlEngulfingOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlEngulfingOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlEngulfing} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CdlEngulfingStream#fillRange()}.
    */
   public CdlEngulfingStream cdlEngulfingOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CdlEngulfingStream sp = new CdlEngulfingStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cdlEngulfingOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLENGULFING openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLENGULFING openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLENGULFING openAndFill: " + retCode);
   }
