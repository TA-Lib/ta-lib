/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112605 MF   Initial coding.
 */

   public int bopLookback( )
   {
      return 0 ;

   }
   public RetCode bop( int startIdx,
                       int endIdx,
                       double inOpen[],
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      double tempReal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* BOP = (Close - Open)/(High - Low) */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = inHigh[i] - inLow[i];
         if( (tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = (inClose[i] - inOpen[i]) / tempReal;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode bopUnguarded( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      double tempReal = 0;
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = inHigh[i] - inLow[i];
         if( (tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = (inClose[i] - inOpen[i]) / tempReal;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode bop( int startIdx,
                       int endIdx,
                       float inOpen[],
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      double tempReal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = (double)inHigh[i] - (double)inLow[i];
         if( (tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = ((double)inClose[i] - (double)inOpen[i]) / tempReal;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode bopUnguarded( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      double tempReal = 0;
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = (double)inHigh[i] - (double)inLow[i];
         if( (tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = ((double)inClose[i] - (double)inOpen[i]) / tempReal;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live BOP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#bop} over the same series.
    * Open with {@link Core#bopOpen}; there is no close — the handle is
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
   public static final class BopStream {
      final Core core;
      double cur_outReal;

      BopStream( Core core ) { this.core = core; }

      BopStream( BopStream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.bopStreamStep(this, inOpen, inHigh, inLow, inClose);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inOpen, double inHigh, double inLow, double inClose ) {
         BopStream scratch = new BopStream(this);
         core.bopStreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public BopStream copy() {
         return new BopStream(this);
      }
   }
   void bopStreamStep( BopStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      double tempReal = 0.0;
      tempReal = inHigh - inLow;
      if( (tempReal < 0.00000000000001) ) {
         sp.cur_outReal = 0.0;
      } else {
         sp.cur_outReal = (inClose - inOpen) / tempReal;
      }
   }
   private RetCode bopOpenBody( BopStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      int outIdx = 0;
      int i = 0;
      double tempReal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      /* BOP = (Close - Open)/(High - Low) */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = inHigh[i] - inLow[i];
         if( (tempReal < 0.00000000000001) ) {
            lastValue_outReal = 0.0;
         } else {
            lastValue_outReal = (inClose[i] - inOpen[i]) / tempReal;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode bopOpenAndFillBody( BopStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      double tempReal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inOpen || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         return RetCode.BadParam;
      }
      /* BOP = (Close - Open)/(High - Low) */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = inHigh[i] - inLow[i];
         if( (tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = (inClose[i] - inOpen[i]) / tempReal;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind bopOpen (composition seam). */
   BopStream bopOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      BopStream sp = new BopStream(this);
      RetCode retCode = bopOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_BOP open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_BOP open: internal error");
      }
      throw new IllegalArgumentException("TA_BOP open: " + retCode);
   }
   /**
    * Open a live BOP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#bop} at that bar.
    * <p>The history must hold at least {@code bopLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public BopStream bopOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return bopOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#bopOpen} that also fills the output array(s) bit-identically
    * to {@link Core#bop} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public BopStream bopOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      BopStream sp = new BopStream(this);
      RetCode retCode = bopOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_BOP openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_BOP openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_BOP openAndFill: " + retCode);
   }
