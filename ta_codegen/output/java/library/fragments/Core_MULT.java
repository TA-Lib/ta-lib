/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090807 MF     Initial Version
 */

   public int multLookback( )
   {
      return 0 ;

   }
   public RetCode mult( int startIdx,
                        int endIdx,
                        double inReal0[],
                        double inReal1[],
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         outReal[outIdx] = inReal0[i] * inReal1[i];
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode multUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal0[],
                                 double inReal1[],
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         outReal[outIdx] = inReal0[i] * inReal1[i];
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode mult( int startIdx,
                        int endIdx,
                        float inReal0[],
                        float inReal1[],
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         outReal[outIdx] = (double)inReal0[i] * (double)inReal1[i];
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode multUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal0[],
                                 float inReal1[],
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         outReal[outIdx] = (double)inReal0[i] * (double)inReal1[i];
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live MULT stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#mult} over the same series.
    * Open with {@link Core#multOpen}; there is no close — the handle is
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
   public static final class MultStream {
      final Core core;
      double cur_outReal;

      MultStream( Core core ) { this.core = core; }

      MultStream( MultStream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal0, double inReal1 ) {
         core.multStreamStep(this, inReal0, inReal1);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal0, double inReal1 ) {
         MultStream scratch = new MultStream(this);
         core.multStreamStep(scratch, inReal0, inReal1);
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
      public MultStream copy() {
         return new MultStream(this);
      }
   }
   void multStreamStep( MultStream sp, double inReal0, double inReal1 )
   {
      sp.cur_outReal = inReal0 * inReal1;
   }
   private RetCode multOpenBody( MultStream sp, double inReal0[], double inReal1[], int startIdx )
   {
      int outIdx = 0;
      int i = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
      }
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         lastValue_outReal = inReal0[i] * inReal1[i];
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode multOpenAndFillBody( MultStream sp, double inReal0[], double inReal1[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 ) {
         return RetCode.BadParam;
      }
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         outReal[outIdx] = inReal0[i] * inReal1[i];
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind multOpen (composition seam). */
   MultStream multOpenInternal( double inReal0[], double inReal1[], int startIdx )
   {
      MultStream sp = new MultStream(this);
      RetCode retCode = multOpenBody(sp, inReal0, inReal1, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MULT open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MULT open: internal error");
      }
      throw new IllegalArgumentException("TA_MULT open: " + retCode);
   }
   /**
    * Open a live MULT stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#mult} at that bar.
    * <p>The history must hold at least {@code multLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MultStream multOpen( double inReal0[], double inReal1[] )
   {
      return multOpenInternal(inReal0, inReal1, 0);
   }
   /**
    * {@link Core#multOpen} that also fills the output array(s) bit-identically
    * to {@link Core#mult} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public MultStream multOpenAndFill( double inReal0[], double inReal1[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MultStream sp = new MultStream(this);
      RetCode retCode = multOpenAndFillBody(sp, inReal0, inReal1, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MULT openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MULT openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_MULT openAndFill: " + retCode);
   }
