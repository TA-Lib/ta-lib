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

   public int cdlXSideGap3MethodsLookback( )
   {
      return 2 ;

   }
   public RetCode cdlXSideGap3Methods( int startIdx,
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
      lookbackTotal = cdlXSideGap3MethodsLookback();
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
   public RetCode cdlXSideGap3MethodsUnguarded( int startIdx,
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
      lookbackTotal = cdlXSideGap3MethodsLookback();
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
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) && inOpen[i] < Math.max(inClose[i - 1], inOpen[i - 1]) && inOpen[i] > Math.min(inClose[i - 1], inOpen[i - 1]) && inClose[i] < Math.max(inClose[i - 2], inOpen[i - 2]) && inClose[i] > Math.min(inClose[i - 2], inOpen[i - 2]) && (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) || ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2]))) ) {
            outInteger[outIdx++] = ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlXSideGap3Methods( int startIdx,
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
      lookbackTotal = cdlXSideGap3MethodsLookback();
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
   public RetCode cdlXSideGap3MethodsUnguarded( int startIdx,
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
      lookbackTotal = cdlXSideGap3MethodsLookback();
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
/**** Streaming API *****/

   /**
    * A live CDLXSIDEGAP3METHODS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlXSideGap3Methods} over the same series.
    * Open with {@link Core#cdlXSideGap3MethodsOpen}; there is no close — the handle is
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
   public static final class CdlXSideGap3MethodsStream {
      final Core core;
      double lag1_inOpen;
      double lag2_inOpen;
      double lag1_inClose;
      double lag2_inClose;
      int cur_outInteger;

      CdlXSideGap3MethodsStream( Core core ) { this.core = core; }

      CdlXSideGap3MethodsStream( CdlXSideGap3MethodsStream other ) {
         this.core = other.core;
         this.lag1_inOpen = other.lag1_inOpen;
         this.lag2_inOpen = other.lag2_inOpen;
         this.lag1_inClose = other.lag1_inClose;
         this.lag2_inClose = other.lag2_inClose;
         this.cur_outInteger = other.cur_outInteger;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.cdlXSideGap3MethodsStreamStep(this, inOpen, inHigh, inLow, inClose);
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
         CdlXSideGap3MethodsStream scratch = new CdlXSideGap3MethodsStream(this);
         core.cdlXSideGap3MethodsStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CdlXSideGap3MethodsStream copy() {
         return new CdlXSideGap3MethodsStream(this);
      }
   }
   void cdlXSideGap3MethodsStreamStep( CdlXSideGap3MethodsStream sp, double inOpen, double inHigh, double inLow, double inClose )
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
   private RetCode cdlXSideGap3MethodsOpenBody( CdlXSideGap3MethodsStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
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
      lookbackTotal = cdlXSideGap3MethodsLookback();
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
            lastValue_outInteger = ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) * 100;
         } else {
            lastValue_outInteger = 0;
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
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlXSideGap3MethodsOpenAndFillBody( CdlXSideGap3MethodsStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
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
      lookbackTotal = cdlXSideGap3MethodsLookback();
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
      /* Capture the live batch state into the handle. */
      sp.lag1_inOpen = inOpen[historyLen - 1];
      sp.lag2_inOpen = inOpen[historyLen - 2];
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.lag2_inClose = inClose[historyLen - 2];
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlXSideGap3MethodsOpen (composition seam). */
   CdlXSideGap3MethodsStream cdlXSideGap3MethodsOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlXSideGap3MethodsStream sp = new CdlXSideGap3MethodsStream(this);
      RetCode retCode = cdlXSideGap3MethodsOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLXSIDEGAP3METHODS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLXSIDEGAP3METHODS open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLXSIDEGAP3METHODS open: " + retCode);
   }
   /**
    * Open a live CDLXSIDEGAP3METHODS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlXSideGap3Methods} at that bar.
    * <p>The history must hold at least {@code cdlXSideGap3MethodsLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlXSideGap3MethodsStream cdlXSideGap3MethodsOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return cdlXSideGap3MethodsOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlXSideGap3MethodsOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlXSideGap3Methods} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CdlXSideGap3MethodsStream cdlXSideGap3MethodsOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlXSideGap3MethodsStream sp = new CdlXSideGap3MethodsStream(this);
      RetCode retCode = cdlXSideGap3MethodsOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLXSIDEGAP3METHODS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLXSIDEGAP3METHODS openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLXSIDEGAP3METHODS openAndFill: " + retCode);
   }
