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
   RetCode CDLXSIDEGAP3METHODS_Internal( int startIdx,
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
   RetCode CDLXSIDEGAP3METHODS_Internal( int startIdx,
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
    * (downside) when they are black. A hit signals trend continuation: +100
    * bullish (uptrend resumes), -100 bearish (downtrend resumes).
    * <p><b>Notes</b>
    * <ul>
    * <li>This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.</li>
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLXSIDEGAP3METHODS_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLXSIDEGAP3METHODS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A three-candle continuation pattern: two same-color candles separated by a
    * real-body gap, followed by an opposite-color candle that fills into the
    * gap. Bullish (upside) when the first two candles are white, bearish
    * (downside) when they are black. A hit signals trend continuation: +100
    * bullish (uptrend resumes), -100 bearish (downtrend resumes).
    * <p><b>Notes</b>
    * <ul>
    * <li>This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.</li>
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLXSIDEGAP3METHODS_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLXSIDEGAP3METHODS", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLXSIDEGAP3METHODS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLXSIDEGAP3METHODS} over the same series.
    * Open with {@link Core#CDLXSIDEGAP3METHODS_Open}; there is no close — the handle is
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
   public static final class CDLXSIDEGAP3METHODS_Stream {
      Core core;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inClose;
      double lag2_inClose;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLXSIDEGAP3METHODS_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLXSIDEGAP3METHODS_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLXSIDEGAP3METHODS_Stream( CDLXSIDEGAP3METHODS_Stream other ) {
         this.core = other.core;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CDLXSIDEGAP3METHODS_Stream other ) {
         this.core = other.core;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.CDLXSIDEGAP3METHODS_StreamStep(this, inOpen, inHigh, inLow, inClose);
         return this.cur_outInteger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         CDLXSIDEGAP3METHODS_Stream scratch = new CDLXSIDEGAP3METHODS_Stream(this);
         core.CDLXSIDEGAP3METHODS_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLXSIDEGAP3METHODS_Stream copy() {
         return new CDLXSIDEGAP3METHODS_Stream(this);
      }
   }
   void CDLXSIDEGAP3METHODS_StreamStep( CDLXSIDEGAP3METHODS_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
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
   private RetCode CDLXSIDEGAP3METHODS_OpenCore( CDLXSIDEGAP3METHODS_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
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
         return RetCode.OutOfRangeEndIndex ;
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
   private RetCode CDLXSIDEGAP3METHODS_OpenBody( CDLXSIDEGAP3METHODS_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      return CDLXSIDEGAP3METHODS_OpenCore( sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0 );
   }
   private RetCode CDLXSIDEGAP3METHODS_OpenAndFillBody( CDLXSIDEGAP3METHODS_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return CDLXSIDEGAP3METHODS_OpenCore( sp, inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger, 1 );
   }
   private RetCode CDLXSIDEGAP3METHODS_OpenAndFillInternalBody( CDLXSIDEGAP3METHODS_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      return CDLXSIDEGAP3METHODS_OpenCore(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
   }
   /* CDLXSIDEGAP3METHODS_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CDLXSIDEGAP3METHODS_Stream CDLXSIDEGAP3METHODS_OpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CDLXSIDEGAP3METHODS_Stream sp = new CDLXSIDEGAP3METHODS_Stream(this);
      RetCode retCode = CDLXSIDEGAP3METHODS_OpenAndFillInternalBody(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLXSIDEGAP3METHODS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLXSIDEGAP3METHODS openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLXSIDEGAP3METHODS openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind CDLXSIDEGAP3METHODS_Open (composition seam). */
   CDLXSIDEGAP3METHODS_Stream CDLXSIDEGAP3METHODS_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLXSIDEGAP3METHODS_Stream sp = new CDLXSIDEGAP3METHODS_Stream(this);
      RetCode retCode = CDLXSIDEGAP3METHODS_OpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLXSIDEGAP3METHODS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLXSIDEGAP3METHODS open: internal error");
      }
      throw new IllegalArgumentException("CDLXSIDEGAP3METHODS open: " + retCode);
   }
   /**
    * Open a live CDLXSIDEGAP3METHODS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLXSIDEGAP3METHODS} at that bar.
    * <p>The history must hold at least {@code CDLXSIDEGAP3METHODS_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLXSIDEGAP3METHODS_Stream CDLXSIDEGAP3METHODS_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLXSIDEGAP3METHODS_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLXSIDEGAP3METHODS_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLXSIDEGAP3METHODS} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLXSIDEGAP3METHODS_Stream#fillRange()}.
    */
   public CDLXSIDEGAP3METHODS_Stream CDLXSIDEGAP3METHODS_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLXSIDEGAP3METHODS_Stream sp = new CDLXSIDEGAP3METHODS_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLXSIDEGAP3METHODS_OpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLXSIDEGAP3METHODS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLXSIDEGAP3METHODS openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLXSIDEGAP3METHODS openAndFill: " + retCode);
   }
