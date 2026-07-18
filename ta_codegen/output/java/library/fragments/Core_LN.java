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

   public int lnLookback( )
   {
      return 0 ;

   }
   public RetCode ln( int startIdx,
                      int endIdx,
                      double inReal[],
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
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.log(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode lnUnguarded( int startIdx,
                               int endIdx,
                               double inReal[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.log(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode ln( int startIdx,
                      int endIdx,
                      float inReal[],
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
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.log((double)inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode lnUnguarded( int startIdx,
                               int endIdx,
                               float inReal[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.log((double)inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live LN stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ln} over the same series.
    * Open with {@link Core#lnOpen}; there is no close — the handle is
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
   public static final class LnStream {
      final Core core;
      double cur_outReal;

      LnStream( Core core ) { this.core = core; }

      LnStream( LnStream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.lnStreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         LnStream scratch = new LnStream(this);
         core.lnStreamStep(scratch, inReal);
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
      public LnStream copy() {
         return new LnStream(this);
      }
   }
   void lnStreamStep( LnStream sp, double inReal )
   {
      sp.cur_outReal = Math.log(inReal);
   }
   private RetCode lnOpenBody( LnStream sp, double inReal[], int startIdx )
   {
      int outIdx = 0;
      int i = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         lastValue_outReal = Math.log(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode lnOpenAndFillBody( LnStream sp, double inReal[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.log(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind lnOpen (composition seam). */
   LnStream lnOpenInternal( double inReal[], int startIdx )
   {
      LnStream sp = new LnStream(this);
      RetCode retCode = lnOpenBody(sp, inReal, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_LN open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_LN open: internal error");
      }
      throw new IllegalArgumentException("TA_LN open: " + retCode);
   }
   /**
    * Open a live LN stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ln} at that bar.
    * <p>The history must hold at least {@code lnLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public LnStream lnOpen( double inReal[] )
   {
      return lnOpenInternal(inReal, 0);
   }
   /**
    * {@link Core#lnOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ln} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public LnStream lnOpenAndFill( double inReal[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      LnStream sp = new LnStream(this);
      RetCode retCode = lnOpenAndFillBody(sp, inReal, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_LN openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_LN openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_LN openAndFill: " + retCode);
   }
