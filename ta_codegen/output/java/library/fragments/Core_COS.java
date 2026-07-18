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

   public int cosLookback( )
   {
      return 0 ;

   }
   public RetCode cos( int startIdx,
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
         outReal[outIdx] = Math.cos(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cosUnguarded( int startIdx,
                                int endIdx,
                                double inReal[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.cos(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cos( int startIdx,
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
         outReal[outIdx] = Math.cos((double)inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cosUnguarded( int startIdx,
                                int endIdx,
                                float inReal[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.cos((double)inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live COS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cos} over the same series.
    * Open with {@link Core#cosOpen}; there is no close — the handle is
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
   public static final class CosStream {
      final Core core;
      double cur_outReal;

      CosStream( Core core ) { this.core = core; }

      CosStream( CosStream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.cosStreamStep(this, inReal);
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
         CosStream scratch = new CosStream(this);
         core.cosStreamStep(scratch, inReal);
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
      public CosStream copy() {
         return new CosStream(this);
      }
   }
   void cosStreamStep( CosStream sp, double inReal )
   {
      sp.cur_outReal = Math.cos(inReal);
   }
   private RetCode cosOpenBody( CosStream sp, double inReal[], int startIdx )
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
         lastValue_outReal = Math.cos(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode cosOpenAndFillBody( CosStream sp, double inReal[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
         outReal[outIdx] = Math.cos(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cosOpen (composition seam). */
   CosStream cosOpenInternal( double inReal[], int startIdx )
   {
      CosStream sp = new CosStream(this);
      RetCode retCode = cosOpenBody(sp, inReal, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_COS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_COS open: internal error");
      }
      throw new IllegalArgumentException("TA_COS open: " + retCode);
   }
   /**
    * Open a live COS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cos} at that bar.
    * <p>The history must hold at least {@code cosLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CosStream cosOpen( double inReal[] )
   {
      return cosOpenInternal(inReal, 0);
   }
   /**
    * {@link Core#cosOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cos} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CosStream cosOpenAndFill( double inReal[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CosStream sp = new CosStream(this);
      RetCode retCode = cosOpenAndFillBody(sp, inReal, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_COS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_COS openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_COS openAndFill: " + retCode);
   }
